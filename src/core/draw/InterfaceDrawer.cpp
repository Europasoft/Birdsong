#include "core/draw/InterfaceDrawer.h"
#include "core/ui/Fonts.h"
#include "core/gpu/Device.h"
#include "core/engine/Camera.h"
#include "core/gpu/descriptors/BindlessTextureManager.h"
#include "core/world/Scene.h"

#include <stdexcept>
#include <array>
#include <limits>
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

	InterfaceDrawer::~InterfaceDrawer() = default;

	bool InterfaceElement::cursorHitTest(glm::vec2 cursor) const
	{
		return (cursor.x <= position.x + size.x/2) && (cursor.x >= position.x) &&
				(cursor.y <= position.y + size.y/2) && (cursor.y >= position.y);
	}

	InterfaceDrawer::InterfaceDrawer(EngineDevice& device, const RenderingFormats& formats, VkSampleCountFlagBits samples, WorldSystem::Scene& scene)
		: device(device), scene(scene)
	{
		// create default UI material
		ShaderFilePaths shaderPaths(makePath("shaders/compiled/ui_test.vert.spv"), makePath("shaders/compiled/ui_test.frag.spv"));
		MaterialCreateInfo materialInfo(shaderPaths, {}, samples, formats, sizeof(ShaderPushConstants::InterfaceElementPushConstants));
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

		// add test text letter quads
		for (uint32_t i = 0; i < 10; i++) 
		{
			InterfaceElement elem{};
			elem.size = glm::vec2(0.03f, 0.03f);
			elem.position = glm::vec2(0.2f + (0.025f * i), 0.2f);
			elem.setMaterial(defaultMaterial);
			elements.push_back(elem);
		}

		// load font
		fonts.push_back(std::make_unique<Font>(device, makePath("fonts/Inter-VariableFont_opsz,wght.ttf"), scene.getTextureManager()));

		ShaderFilePaths textShaderPaths(makePath("shaders/compiled/text.vert.spv"), makePath("shaders/compiled/text.frag.spv"));
		MaterialCreateInfo textMatInfo(textShaderPaths, scene.getDescriptorSetLayouts(), samples, formats, sizeof(ShaderPushConstants::TextGlyphPushConstants));
		textMatInfo.shadingProperties.useVertexInput = false;
		textMatInfo.shadingProperties.enableDepth = false;
		textMatInfo.shadingProperties.cullModeFlags = VK_CULL_MODE_NONE;
		textMaterial = std::make_shared<Material>(textMatInfo, device);
		textMaterial->finalize();
	}


	void InterfaceDrawer::render(VkCommandBuffer cmdBuf, glm::vec2 mousePosition, VkExtent2D windowExtent, uint32_t frameIndex)
	{
		const auto globalSets = scene.getDescriptorSets(frameIndex);
		vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, textMaterial->getPipelineLayout(),
			0, static_cast<uint32_t>(globalSets.size()), globalSets.data(), 0, nullptr);

		ShaderPushConstants::TextGlyphPushConstants push{};
		const auto& ch = fonts[0]->getCharacter('A');

		push.uvs.x = ch.u0;
		push.uvs.y = ch.v0;
		push.uvs.z = ch.u1;
		push.uvs.w = ch.v1;
		push.vertexBounds.x = ch.l;
		push.vertexBounds.y = ch.b;
		push.vertexBounds.z = ch.r;
		push.vertexBounds.w = ch.t;
		push.screenPositionAndTextureIndex.x = 0.1f;
		push.screenPositionAndTextureIndex.y = 0.1f;
		push.screenPositionAndTextureIndex.w = fonts[0]->getTextureIndex();
		textMaterial->bindToCommandBuffer(cmdBuf);
		textMaterial->writePushConstants(cmdBuf, push);
		vkCmdDraw(cmdBuf, 6, 1, 0, 0); // bufferless draw (vertex attributes generated in shader)


		for (InterfaceElement& elem : elements) 
		{
			//mousePosition.x /= windowExtent.width;
			//mousePosition.y /= windowExtent.height;
			//float timeSinceHover = elem.cursorHitTest(mousePosition) ? 0.f : 10.f; // TODO: actually accumulate time
			//float timeSinceClick = 10.f; // TODO
			//ShaderPushConstants::InterfaceElementPushConstants push{ elem.position, elem.size, timeSinceHover, timeSinceClick };
			//elem.getMaterial().bindToCommandBuffer(cmdBuf);
			//elem.getMaterial().writePushConstants(cmdBuf, push);
			//vkCmdDraw(cmdBuf, 6, 1, 0, 0); // bufferless draw (vertex attributes generated in shader)
		}
	}


}