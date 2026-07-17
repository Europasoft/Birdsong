#include "core/draw/InterfaceDrawer.h"

#include "core/gpu/Device.h"
#include "core/engine/Camera.h"

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
	InterfaceDrawer::~InterfaceDrawer() = default;

	bool InterfaceElement::cursorHitTest(glm::vec2 cursor) const
	{
		return (cursor.x <= position.x + size.x/2) && (cursor.x >= position.x) &&
				(cursor.y <= position.y + size.y/2) && (cursor.y >= position.y);
	}

	InterfaceDrawer::InterfaceDrawer(EngineDevice& device, const RenderingFormats& formats, VkSampleCountFlagBits samples) 
		: device{ device }
	{
		// create default UI material
		ShaderFilePaths shaderPaths(makePath("shaders/ui_test.vert.spv"), makePath("shaders/ui_test.frag.spv"));
		MaterialCreateInfo materialInfo(shaderPaths, {}, samples, formats, sizeof(ShaderPushConstants::InterfaceElementPushConstants));
		materialInfo.shadingProperties.useVertexInput = false;
		materialInfo.shadingProperties.enableDepth = false;
		materialInfo.shadingProperties.cullModeFlags = VK_CULL_MODE_NONE;
		defaultMaterial = std::make_shared<Material>(materialInfo, device);

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
	}


	void InterfaceDrawer::render(VkCommandBuffer cmdBuf, glm::vec2 mousePosition, VkExtent2D windowExtent)
	{
		for (InterfaceElement& elem : elements) 
		{

			mousePosition.x /= windowExtent.width;
			mousePosition.y /= windowExtent.height;
			float timeSinceHover = elem.cursorHitTest(mousePosition) ? 0.f : 10.f; // TODO: actually accumulate time
			float timeSinceClick = 10.f; // TODO
			ShaderPushConstants::InterfaceElementPushConstants push{ elem.position, elem.size, timeSinceHover, timeSinceClick };
			elem.getMaterial().bindToCommandBuffer(cmdBuf);
			elem.getMaterial().writePushConstants(cmdBuf, push);

			vkCmdDraw(cmdBuf, 6, 1, 0, 0); // bufferless draw (vertex attributes generated in shader)
		}
	}


}