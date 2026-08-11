#include "core/ui/Element.h"
#include "core/draw/FrameContext.h"
#include "core/gpu/Material.h"
#include "core/gpu/descriptors/InstanceBuffer.h"

#include "core/types/vk.h"

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
		for (auto& e : nested) e->preDrawRecursive(f);
		hierarchyInstanceBuffer->pushBufferToGPU(f.bufferIndex);
		textGlyphInstanceBuffer->pushBufferToGPU(f.bufferIndex);
		EngineCore::Material* m = nullptr;
		for (auto& e : nested) e->drawRecursive(f, m);
	}

	EngineCore::InstanceBuffer<GlyphInst>& RootElement::getTextGlyphInstanceBuffer() const
	{
		return *textGlyphInstanceBuffer.get();
	}

	void Element::postAddElement(Element& e)
	{
		root->numElements++;
	}

	void Element::preDrawRecursive(const EngineCore::FrameContext& f)
	{
		preDraw(f);
		for (auto& e : nested) e->preDrawRecursive(f);
	}

	void Element::drawRecursive(const EngineCore::FrameContext& f, EngineCore::Material*& m)
	{
		draw(f, m);
		for (auto& e : nested) e->drawRecursive(f, m);
	}

	Vec2 Element::calculatePosition() const
	{
		Vec2 parentPosition(0.f);
		Vec2 parentSize(1.f);
		if (not isNextToRoot())
		{
			const Vec2 parentPivot = parent->pivotPoint * parent->size;
			parentPosition = parent->position - parentPivot;
			parentSize = parent->size;
		}
		return (position * parentSize) + parentPosition;
	}

	Vec2 Element::calculateSize() const
	{
		const Vec2 parentSize = (not isNextToRoot()) ? parent->size : Vec2(1.f);
		return size * parentSize;
	}

	void Element::preDraw(const EngineCore::FrameContext& f)
	{
		UIInst d = {};
		const Vec2 finalPosition = calculatePosition();
		const Vec2 finalSize = calculateSize();
		d.positionAndSize.x = finalPosition.x - (pivotPoint.x * size.x);
		d.positionAndSize.y = finalPosition.y - (pivotPoint.y * size.y);
		d.positionAndSize.z = finalSize.x;
		d.positionAndSize.w = finalSize.y;
		d.backgroundColor.x = backgroundColor.x;
		d.backgroundColor.y = backgroundColor.y;
		d.backgroundColor.z = backgroundColor.z;
		d.backgroundColor.w = backgroundOpacity;
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

	

}