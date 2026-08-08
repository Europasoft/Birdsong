#pragma once
#include "core/gpu/Material.h"

#include <glm/gtc/matrix_transform.hpp> // glm

#include <memory>
#include <vector>
#include <cmath> // only used in perspective calculation

class Camera;

namespace WorldSystem
{
	class Scene;
}

namespace UI
{
	class Font;
}

namespace EngineCore
{
	class EngineDevice;
	class BindlessTextureManager;

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

		InterfaceDrawer(EngineDevice& device, const RenderingFormats& formats, VkSampleCountFlagBits samples, WorldSystem::Scene& scene);
		InterfaceDrawer(const InterfaceDrawer&) = delete;
		~InterfaceDrawer();
		InterfaceDrawer& operator=(const InterfaceDrawer&) = delete;

		void render(VkCommandBuffer cmdBuf, glm::vec2 mousePosition, VkExtent2D windowExtent, uint32_t frameIndex);

	private:
		EngineDevice& device;
		WorldSystem::Scene& scene;
		std::vector<InterfaceElement> elements;
		std::shared_ptr<Material> defaultMaterial;

		std::vector<std::unique_ptr<UI::Font>> fonts;
		std::shared_ptr<Material> textMaterial;
	};

}