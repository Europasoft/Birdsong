#pragma once
#include "core/gpu/Material.h"

#include <glm/gtc/matrix_transform.hpp> // glm

#include <memory>
#include <vector>
#include <cmath> // only used in perspective calculation

class Camera;

namespace EngineCore
{
	class EngineDevice;

	class InterfaceElement 
	{
	public:
		void setMaterial(std::shared_ptr<Material> m) { material = m; }
		Material& getMaterial() { return *material.get(); }
		bool cursorHitTest(glm::vec2 cursor) const;

		glm::vec2 position, size;
		float timeSinceHover, timeSinceClick;

	private:
		std::shared_ptr<Material> material;
	};


	class InterfaceDrawer
	{
	public:

		InterfaceDrawer(EngineDevice& device, const RenderingFormats& formats, VkSampleCountFlagBits samples);
		InterfaceDrawer(const InterfaceDrawer&) = delete;
		~InterfaceDrawer();
		InterfaceDrawer& operator=(const InterfaceDrawer&) = delete;

		void render(VkCommandBuffer cmdBuf, glm::vec2 mousePosition, VkExtent2D windowExtent);

	private:
		EngineDevice& device;
		std::vector<InterfaceElement> elements;
		std::shared_ptr<Material> defaultMaterial;

		

	};

}