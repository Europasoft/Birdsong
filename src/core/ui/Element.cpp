#include "core/ui/Element.h"
#include "core/draw/FrameContext.h"
#include "core/gpu/Material.h"
#include "core/gpu/descriptors/InstanceBuffer.h"

#include "core/types/vk.h"

#include <algorithm>
#include <cassert>

namespace UI
{
	RootElement::RootElement(EngineCore::EngineDevice& device)
		: hierarchyInstanceBuffer(EngineCore::InstanceBufferUtil::allocate<UIInst>(device, 800)),
		textGlyphInstanceBuffer(EngineCore::InstanceBufferUtil::allocate<GlyphInst>(device, 2000))
	{
		parent = nullptr;
		root = this;
	}

	RootElement::~RootElement()
	{}

	Element::Element()
	{
		pivotPoint = Vec2(0.5, 0.5);
	}

	Element::~Element()
	{}

	std::unique_ptr<RootElement> RootElement::create(EngineCore::EngineDevice& device)
	{
		return std::unique_ptr<RootElement>(new RootElement(device));
	}

	void Element::loadMaterial(EngineCore::EngineDevice& device, const EngineCore::DrawContext& d)
	{
		using namespace EngineCore;
		ShaderFilePaths shaderPaths(makePath("shaders/compiled/ui_test.vert.spv"), makePath("shaders/compiled/ui_test.frag.spv"));
		MaterialCreateInfo materialInfo(shaderPaths, d.world->getScene().getDescriptorSetLayouts(), d.samples, d.basePassFormats,
				sizeof(ShaderPushConstants::MeshPushConstants));
		materialInfo.shadingProperties.useVertexInput = false;
		materialInfo.shadingProperties.enableDepth = false;
		materialInfo.shadingProperties.cullModeFlags = VK_CULL_MODE_NONE;
		material = std::make_shared<Material>(materialInfo, device);
		material->finalize();
	}

	void RootElement::drawAll(const EngineCore::FrameContext& f)
	{
		PreDrawData data =
		{
			.size = Vec2(1.f), // elements directly under root get 100% space
			.position = Vec2(0.f)
		};
		for (auto& e : nested) 
		{
			e->preDrawRecursive(f, data);
		}
		hierarchyInstanceBuffer->pushBufferToGPU(f.bufferIndex);
		textGlyphInstanceBuffer->pushBufferToGPU(f.bufferIndex);

		EngineCore::Material* m = nullptr;
		for (auto& e : nested) 
		{
			e->drawRecursive(f, m);
		}
	}

	EngineCore::InstanceBuffer<GlyphInst>& RootElement::getTextGlyphInstanceBuffer() const
	{
		return *textGlyphInstanceBuffer.get();
	}

	void Element::postAddElement(Element& e)
	{
		root->numElements++;
	}

	void Element::preDrawRecursive(const EngineCore::FrameContext& f, const PreDrawData& parentData)
	{
		PreDrawData currentData = {};
		const Vec2 parentTopLeft = parentData.position - parentData.pivot;
		currentData.size = parentData.size * size; // calculate absolute size
		currentData.position = parentTopLeft + (position * parentData.size); // calculate top-left origin ignoring parent pivot
		currentData.pivot = pivotPoint * currentData.size; // store absolute pivot offset for this element

		preDraw(f, currentData);

		// pass down currentData (children only care about currentData.position, not currentData.pivot)
		preDrawNested(f, currentData);
	}

	void Element::preDrawNested(const EngineCore::FrameContext& f, const PreDrawData& currentData)
	{
		for (auto& e : nested)
		{
			e->preDrawRecursive(f, currentData);
		}
	}

	void Element::drawRecursive(const EngineCore::FrameContext& f, EngineCore::Material*& m)
	{
		draw(f, m);
		for (auto& e : nested) e->drawRecursive(f, m);
	}

	void Element::preDraw(const EngineCore::FrameContext& f, const PreDrawData& data)
	{
		UIInst d = {};
		const Vec2 renderPosition = data.position - data.pivot;
		d.positionAndSize.x = renderPosition.x;
		d.positionAndSize.y = renderPosition.y;
		d.positionAndSize.z = data.size.x;
		d.positionAndSize.w = data.size.y;
		d.backgroundColor.x = backgroundColor.x;
		d.backgroundColor.y = backgroundColor.y;
		d.backgroundColor.z = backgroundColor.z;
		d.backgroundColor.w = backgroundOpacity;
		d.cornerRadius.x = cornerRadiusTop.x;
		d.cornerRadius.y = cornerRadiusTop.y;
		d.cornerRadius.z = cornerRadiusBottom.x;
		d.cornerRadius.w = cornerRadiusBottom.y;
		instanceDataIndex = root->hierarchyInstanceBuffer->addInstanceData(d);
	}

	void Element::draw(const EngineCore::FrameContext& f, EngineCore::Material*& m)
	{
		using namespace EngineCore;
		if (material.get() != m && material)
		{
			m = material.get();
			m->bindToCommandBuffer(f.commandBuffer);
		}
		assert(m);
		if (not m) return;

		ShaderPushConstants::MeshPushConstants push = {};
		push.instanceBufferAddress = root->hierarchyInstanceBuffer->getDeviceAddress(f.bufferIndex);
		push.instanceID = instanceDataIndex;
		m->writePushConstants(f.commandBuffer, push);
		vkCmdDraw(f.commandBuffer, 6, 1, 0, 0); // bufferless draw (vertex attributes generated in shader)
	}

	bool Element::isRoot() const
	{
		return root == this;
	}

	bool Element::isNextToRoot() const
	{
		return parent == root;
	}

	bool Element::isLeaf() const
	{
		return nested.size() == 0;
	}

	void Element::setCornerRadius(float r)
	{
		cornerRadiusTop = Vec2(r);
		cornerRadiusBottom = cornerRadiusTop;
	}

	

}