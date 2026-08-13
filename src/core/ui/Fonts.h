#pragma once

#include <string>
#include <string_view>
#include <memory>
#include <unordered_map>

namespace EngineCore
{
	class EngineDevice;
	class BindlessTextureManager;
	class Image;
}

namespace msdfgen
{
	class FontHandle;
	class FreetypeHandle;
	struct FontMetrics;
}

namespace UI
{
	struct GlyphInfo
	{
		float u0, v0, u1, v1; // UV coordinates
		float l, b, r, t; // vertex bounds
		float advance; // distance to next character
	};

	struct ScaledFontMetrics
	{
		float ascender; // distance above baseline
		float descender; // distance below baseline
		float lineHeight; // full height between consecutive baselines
		float totalFontHeight; // total span (ascender - descender)
	};

	class Font
	{
	public:
		Font(EngineCore::EngineDevice& device, std::string_view filepath, EngineCore::BindlessTextureManager& texManager);
		~Font();
		static std::shared_ptr<UI::Font> load(EngineCore::EngineDevice& device, std::string_view filepath, EngineCore::BindlessTextureManager& texManager);

		const GlyphInfo& getCharacter(char32_t c) const;
		uint32_t getTextureIndex() const { return texIndex; }
		uint32_t getPixelRange() const { return sdfPixelRange; }
		float getKerning(char32_t a, char32_t b) const;
		ScaledFontMetrics getScaledMetrics(float viewportHeight, float fontScale) const;

	private:
		bool generateAtlas(EngineCore::EngineDevice& device, std::string_view filepath, EngineCore::BindlessTextureManager& texManager);
		void getMetrics();

		uint32_t texIndex = 0;
		std::unique_ptr<EngineCore::Image> texture;
		std::unordered_map<char32_t, GlyphInfo> glyphInfos;
		uint32_t sdfPixelRange = 9;
		msdfgen::FontHandle* fontHandle = nullptr;
		msdfgen::FreetypeHandle* freetypeHandle = nullptr;
		std::unique_ptr<msdfgen::FontMetrics> fontMetrics = nullptr;
	};
}