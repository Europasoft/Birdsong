#include "core/draw/InterfaceDrawer.h"
#include "core/draw/FrameContext.h"

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
		// create default UI material
		ShaderFilePaths shaderPaths(makePath("shaders/compiled/ui_test.vert.spv"), makePath("shaders/compiled/ui_test.frag.spv"));
		MaterialCreateInfo materialInfo(shaderPaths, {}, d.samples, d.basePassFormats, sizeof(ShaderPushConstants::InterfaceElementPushConstants));
		materialInfo.shadingProperties.useVertexInput = false;
		materialInfo.shadingProperties.enableDepth = false;
		materialInfo.shadingProperties.cullModeFlags = VK_CULL_MODE_NONE;
		defaultMaterial = std::make_shared<Material>(materialInfo, device);
		defaultMaterial->finalize();

		// add test ui element
		InterfaceElement elem{};
		elem.size = glm::vec2(0.33f, 0.33f);
		elem.position = glm::vec2(0.5f, 0.5f);
		elem.setMaterial(defaultMaterial);
		elements.push_back(elem);

		auto& scene = d.world->getScene();

		// load font
		fonts.push_back(std::make_unique<Font>(device, makePath("fonts/Inter-VariableFont_opsz,wght.ttf"), scene.getTextureManager()));

		ShaderFilePaths textShaderPaths(makePath("shaders/compiled/text.vert.spv"), makePath("shaders/compiled/text.frag.spv"));
		MaterialCreateInfo textMatInfo(textShaderPaths, scene.getDescriptorSetLayouts(), d.samples, d.basePassFormats, sizeof(ShaderPushConstants::TextGlyphPushConstants));
		textMatInfo.shadingProperties.useVertexInput = false;
		textMatInfo.shadingProperties.enableDepth = false;
		textMatInfo.shadingProperties.cullModeFlags = VK_CULL_MODE_NONE;
		textMaterial = std::make_shared<Material>(textMatInfo, device);
		textMaterial->finalize();
	}

	InterfaceDrawer::~InterfaceDrawer() = default;

	void InterfaceDrawer::render(const FrameContext& f)
	{
		const auto globalSets = f.scene->getDescriptorSets(f.bufferIndex);
		vkCmdBindDescriptorSets(f.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, textMaterial->getPipelineLayout(),
			0, static_cast<uint32_t>(globalSets.size()), globalSets.data(), 0, nullptr);

		textMaterial->bindToCommandBuffer(f.commandBuffer);

		std::string text = "HELLO WORLD!";
		const float fontScale = 40;
		const UI::Font& font = *fonts[0];
		float offset = 0;
		drawText(text, font, fontScale, f.viewportExtent, f.commandBuffer);

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

	void InterfaceDrawer::drawText(const std::string& text, const UI::Font& font, float fontScale, VkExtent2D windowExtent, VkCommandBuffer cmdBuf)
	{
		float offset = 0;
		for (auto c : text)
		{
			const GlyphInfo& g = font.getCharacter(c);
			TextGlyphPushConstants push = makePushConstantForGlyph(g, font, offset, fontScale);
			textMaterial->writePushConstants(cmdBuf, push);
			vkCmdDraw(cmdBuf, 6, 1, 0, 0); // bufferless draw (vertex attributes generated in shader)
			offset += g.advance * fontScale * (2.0 / windowExtent.width);
		}
	}

	TextGlyphPushConstants InterfaceDrawer::makePushConstantForGlyph(const GlyphInfo& g, const UI::Font& font, float offset, float fontScale) const
	{
		ShaderPushConstants::TextGlyphPushConstants push{};
		push.uvs.x = g.u0;
		push.uvs.y = g.v0;
		push.uvs.z = g.u1;
		push.uvs.w = g.v1;
		push.vertexBounds.x = g.l;
		push.vertexBounds.y = g.b;
		push.vertexBounds.z = g.r;
		push.vertexBounds.w = g.t;
		push.screenPos_FontScale_TexIdx.x = offset;
		push.screenPos_FontScale_TexIdx.y = 0.1f;
		push.screenPos_FontScale_TexIdx.z = fontScale;
		push.screenPos_FontScale_TexIdx.w = font.getTextureIndex();
		return push;
	}


}