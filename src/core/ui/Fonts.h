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

namespace UI
{
	struct GlyphInfo
	{
		float u0, v0, u1, v1; // UV coordinates
		float l, b, r, t; // vertex bounds
		float advance; // distance to next character
	};

	class Font
	{
	public:
		Font(EngineCore::EngineDevice& device, std::string_view filepath, EngineCore::BindlessTextureManager& texManager);
		~Font();

		const GlyphInfo& getCharacter(char32_t c) const;
		uint32_t getTextureIndex() const { return texIndex; }
		uint32_t getPixelRange() const { return sdfPixelRange; }

	private:
		bool generateAtlas(EngineCore::EngineDevice& device, std::string_view filepath, EngineCore::BindlessTextureManager& texManager);

		uint32_t texIndex = 0;
		std::unique_ptr<EngineCore::Image> texture;
		std::unordered_map<char32_t, GlyphInfo> glyphInfos;
		uint32_t sdfPixelRange = 8;
	};
}