#pragma once
#include "core/draw/DrawBase.h"

#include <glm/gtc/matrix_transform.hpp> // glm

#include <memory>
#include <vector>
#include <string>
#include <cmath> // only used in perspective calculation

class Camera;

namespace WorldSystem
{
	class Scene;
}

namespace UI
{
	class RootElement;
	class Font;
	struct GlyphInfo;
	class TextBox;
}

namespace EngineCore
{
	class Material;
	template <typename T>
	class InstanceBuffer;
	namespace ShaderInstanceData
	{
		struct TextGlyphInstanceData;
	}
}

namespace EngineCore
{
	class InterfaceElement 
	{
	public:
		void setMaterial(std::shared_ptr<Material> m) { material = m; }
		Material& getMaterial() { return *material.get(); }

		glm::vec2 position, size;
		float timeSinceHover, timeSinceClick;

	private:
		std::shared_ptr<Material> material;
	};


	class InterfaceDrawer : public DrawBase
	{
	public:
		InterfaceDrawer(EngineDevice& device, const DrawContext& d);
		~InterfaceDrawer();

		virtual void render(const FrameContext& f) override;

	private:
		std::unique_ptr<UI::RootElement> root;
		std::shared_ptr<Material> defaultMaterial;

		std::vector<std::shared_ptr<UI::Font>> fonts;

		void createDemoUI();

	};

}