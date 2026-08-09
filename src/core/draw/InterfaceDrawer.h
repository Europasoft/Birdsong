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
	class Font;
	struct GlyphInfo;
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
		bool cursorHitTest(glm::vec2 cursor) const;

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
		std::vector<InterfaceElement> elements;
		std::shared_ptr<Material> defaultMaterial;

		std::unique_ptr<InstanceBuffer<ShaderInstanceData::TextGlyphInstanceData>> textGlyphInstanceBuffer;

		std::vector<std::unique_ptr<UI::Font>> fonts;
		std::shared_ptr<Material> textMaterial;

		void drawText(const FrameContext& f, const std::string& text, const UI::Font& font, float fontScale);

		uint32_t addGlyphToInstanceBuffer(const UI::GlyphInfo& g, const UI::Font& font, float offset, float fontScale);
	};

}