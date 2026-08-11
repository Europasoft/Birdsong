#include "core/draw/InterfaceDrawer.h"
#include "core/draw/FrameContext.h"

#include "core/gpu/Material.h"
#include "core/gpu/descriptors/InstanceBuffer.h"
#include "core/ui/Element.h"
#include "core/ui/Fonts.h"
#include "core/gpu/Device.h"
#include "core/engine/Camera.h"
#include "core/gpu/descriptors/BindlessTextureManager.h"
#include "core/world/Scene.h"

#include <stdexcept>
#include <array>
#include <limits>
#include <string>
#include <iostream> // temporary

// glm
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace EngineCore
{
	using namespace UI;
	using namespace WorldSystem;
	using namespace ShaderPushConstants;

	InterfaceDrawer::InterfaceDrawer(EngineDevice& device, const DrawContext& d)
		: DrawBase(device, d)
	{
		textGlyphInstanceBuffer = InstanceBufferUtil::allocate<ShaderInstanceData::TextGlyphInstanceData>(device, 10000);
		root = RootElement::create(device);
		
		// create default UI material
		ShaderFilePaths shaderPaths(makePath("shaders/compiled/ui_test.vert.spv"), makePath("shaders/compiled/ui_test.frag.spv"));
		MaterialCreateInfo materialInfo(shaderPaths, {}, d.samples, d.basePassFormats, 
			sizeof(ShaderPushConstants::InterfaceElementPushConstants));
		materialInfo.shadingProperties.useVertexInput = false;
		materialInfo.shadingProperties.enableDepth = false;
		materialInfo.shadingProperties.cullModeFlags = VK_CULL_MODE_NONE;
		defaultMaterial = std::make_shared<Material>(materialInfo, device);
		defaultMaterial->finalize();

		// load font
		auto& scene = d.world->getScene();
		fonts.push_back(std::make_unique<Font>(device, makePath("fonts/Inter-VariableFont_opsz,wght.ttf"), scene.getTextureManager()));

		ShaderFilePaths textShaderPaths(makePath("shaders/compiled/text.vert.spv"), makePath("shaders/compiled/text.frag.spv"));
		MaterialCreateInfo textMatInfo(textShaderPaths, scene.getDescriptorSetLayouts(), d.samples, d.basePassFormats, 
			sizeof(ShaderPushConstants::MeshPushConstants));
		textMatInfo.shadingProperties.useVertexInput = false;
		textMatInfo.shadingProperties.enableDepth = false;
		textMatInfo.shadingProperties.cullModeFlags = VK_CULL_MODE_NONE;
		textMaterial = std::make_shared<Material>(textMatInfo, device);
		textMaterial->finalize();

		VerticalBox& box = root->addElement<VerticalBox>();
		box.loadMaterial(device, d);
		box.size = Vec2(0.2);
		box.position = Vec2(0.5);
		box.backgroundColor = Vec(0.1f, 0.1f, 0.6f);
		box.pivotPoint = (0.5, 0.5);

		VerticalBox& box2 = box.addElement<VerticalBox>();
		box2.size = Vec2(0.02);
		box2.position = Vec2(0.5);
		box2.backgroundColor = Vec(0.2f, 0.2f, 0.5f);
		box2.pivotPoint = (1);
	}

	InterfaceDrawer::~InterfaceDrawer() = default;

	void InterfaceDrawer::render(const FrameContext& f)
	{
		const auto globalSets = f.scene->getDescriptorSets(f.bufferIndex);
		vkCmdBindDescriptorSets(f.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, textMaterial->getPipelineLayout(),
			0, static_cast<uint32_t>(globalSets.size()), globalSets.data(), 0, nullptr);

		textMaterial->bindToCommandBuffer(f.commandBuffer);

		std::string text = "Hello world!";
		const float fontScale = 24;
		const UI::Font& font = *fonts[0];
		float offset = 0;
		drawText(f, text, font, fontScale);


		root->drawAll(f);


		/*for (InterfaceElement& elem : elements)
		{
			mousePosition.x /= windowExtent.width;
			mousePosition.y /= windowExtent.height;
			float timeSinceHover = elem.cursorHitTest(mousePosition) ? 0.f : 10.f; // TODO: actually accumulate time
			float timeSinceClick = 10.f; // TODO
			ShaderPushConstants::InterfaceElementPushConstants push{ elem.position, elem.size, timeSinceHover, timeSinceClick };
			elem.getMaterial().bindToCommandBuffer(cmdBuf);
			elem.getMaterial().writePushConstants(cmdBuf, push);
			vkCmdDraw(cmdBuf, 6, 1, 0, 0); // bufferless draw (vertex attributes generated in shader)
		}*/
	}

	bool InterfaceElement::cursorHitTest(glm::vec2 cursor) const
	{
		return (cursor.x <= position.x + size.x/2) && (cursor.x >= position.x) &&
				(cursor.y <= position.y + size.y/2) && (cursor.y >= position.y);
	}

	void InterfaceDrawer::drawText(const FrameContext& f, const std::string& text, const UI::Font& font, float fontScale)
	{
		float offset = 0;
		for (size_t i = 0; i < text.size(); i++)
		{
			const GlyphInfo& g = font.getCharacter(text[i]);

			MeshPushConstants push = {};
			push.instanceBufferAddress = textGlyphInstanceBuffer->getDeviceAddress(f.bufferIndex);
			push.instanceID = addGlyphToInstanceBuffer(g, font, offset, fontScale);
			textMaterial->writePushConstants(f.commandBuffer, push);

			vkCmdDraw(f.commandBuffer, 6, 1, 0, 0); // bufferless draw (vertex attributes generated in shader)

			const auto kerning = (i == text.size() - 1) ? 0.f : font.getKerning(text[i], text[i + 1]);
			offset += (g.advance + kerning) * fontScale * (2.0 / f.viewportExtent.width);
		}

		textGlyphInstanceBuffer->pushBufferToGPU(f.bufferIndex);
	}

	uint32_t InterfaceDrawer::addGlyphToInstanceBuffer(const UI::GlyphInfo& g, const UI::Font& font, float offset, float fontScale)
	{
		ShaderInstanceData::TextGlyphInstanceData d = {};
		d.uvs.x = g.u0;
		d.uvs.y = g.v0;
		d.uvs.z = g.u1;
		d.uvs.w = g.v1;
		d.vertexBounds.x = g.l;
		d.vertexBounds.y = g.b;
		d.vertexBounds.z = g.r;
		d.vertexBounds.w = g.t;
		d.basePos.x = 0.5f + (-1.0f + offset);
		d.basePos.y = 0.f; // temporarily hardcoded position
		d.fontScale = fontScale;
		d.textureIndex = font.getTextureIndex();
		return textGlyphInstanceBuffer->addInstanceData(d);
	}


}