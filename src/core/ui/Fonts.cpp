#include "core/ui/Fonts.h"
#include "core/gpu/Device.h"
#include "core/gpu/Image.h"
#include "core/gpu/Buffer.h"
#include "core/gpu/descriptors/BindlessTextureManager.h"
#include "core/types/CommonTypes.h"

#include "thirdparty/msdf-atlas-gen-lite/msdf-atlas-gen/msdf-atlas-gen.h"
#include "thirdparty/msdf-atlas-gen-lite/msdfgen/ext/import-font.h"

#include <cassert>

namespace UI
{
	using namespace EngineCore;

	Font::Font(EngineDevice& device, std::string_view filepath, EngineCore::BindlessTextureManager& texManager)
	{
		generateAtlas(device, filepath, texManager);
	}

	Font::~Font()
	{
		if (fontHandle) msdfgen::destroyFont(fontHandle);
		if (freetypeHandle) msdfgen::deinitializeFreetype(freetypeHandle);
	}

    std::shared_ptr<UI::Font> Font::load(EngineCore::EngineDevice& device, std::string_view filepath, EngineCore::BindlessTextureManager& texManager)
    {
        return std::shared_ptr<UI::Font>(new Font(device, makePath(filepath), texManager));
    }

	bool Font::isReady() const
	{
		return texture.get();
	}

	const GlyphInfo& Font::getCharacter(char32_t c) const
	{
		auto it = glyphInfos.find(c);
		if (it == glyphInfos.end()) { it = glyphInfos.find('?'); };

		return it->second;
	}

	static msdf_atlas::Charset createExtendedLatinCharset()
	{
		msdf_atlas::Charset charset = msdf_atlas::Charset::ASCII; // 0x20 to 0x7E
		// latin-1 supplement: includes western european accents (Swedish, Norwegian, German, etc.)
		for (msdf_atlas::unicode_t c = 0xA0; c <= 0xFF; ++c)
		{
			charset.add(c);
		}
		return charset;
	}

	using byte = msdfgen::byte;

	struct Font::GenResult
	{
		bool success = false;
		int width = 0;
		int height = 0;
		std::vector<msdf_atlas::GlyphGeometry> glyphs;
		std::vector<byte> pixels;
	};

	std::unique_ptr<Font::GenResult> Font::generateSdf(std::string_view filepath)
	{
		using namespace msdf_atlas;
		using namespace msdfgen;

		auto res = std::make_unique<Font::GenResult>();

		freetypeHandle = msdfgen::initializeFreetype();
		if (not freetypeHandle) return res;

		fontHandle = msdfgen::loadFont(freetypeHandle, filepath.data());
		if (not fontHandle) return res;

		auto& glyphs = res->glyphs;
		msdf_atlas::FontGeometry fontGeometry(&glyphs);
		fontGeometry.loadCharset(fontHandle, 1.0, createExtendedLatinCharset(), true, true);
		// Apply MSDF edge coloring. See edge-coloring.h for other coloring strategies.
		const double maxCornerAngle = 3.0;
		for (GlyphGeometry& glyph : glyphs)
			glyph.edgeColoring(&msdfgen::edgeColoringInkTrap, maxCornerAngle, 0);

		msdf_atlas::TightAtlasPacker packer;
		packer.setDimensionsConstraint(msdf_atlas::DimensionsConstraint::SQUARE);
		// setScale for a fixed size or setMinimumScale to use the largest that fits
		packer.setMinimumScale(38.0);
		// setPixelRange or setUnitRange
		packer.setPixelRange(sdfPixelRange);
		packer.setInnerPixelPadding(Padding(2));
		packer.setOuterPixelPadding(Padding(2));
		packer.setMiterLimit(1.0);
		// Compute atlas layout - pack glyphs
		packer.pack(glyphs.data(), glyphs.size());
		// Get final atlas dimensions
		auto& width = res->width;
		auto& height = res->height;
		packer.getDimensions(width, height);
		using MtsdfGeneratorFunction = void (*)(const BitmapSection<float, 4>&, const GlyphGeometry&, const GeneratorAttributes&);
		constexpr MtsdfGeneratorFunction fn = &mtsdfGenerator;
		// The ImmediateAtlasGenerator class facilitates the generation of the atlas bitmap.
		ImmediateAtlasGenerator<
			float, // pixel type of buffer for individual glyphs depends on generator function
			4, // number of atlas color channels
			fn, // function to generate bitmaps for individual glyphs
			BitmapAtlasStorage<byte, 4> // class that stores the atlas bitmap
			// For example, a custom atlas storage class that stores it in VRAM can be used.
		> generator(width, height);
		// GeneratorAttributes can be modified to change the generator's default settings.
		GeneratorAttributes attributes;
		{
			ErrorCorrectionConfig correctionConf(ErrorCorrectionConfig::Mode::EDGE_PRIORITY,
				ErrorCorrectionConfig::DistanceCheckMode::CHECK_DISTANCE_AT_EDGE);
			attributes.config = MSDFGeneratorConfig(true, correctionConf);
		}
		generator.setAttributes(attributes);
		generator.setThreadCount(4);
		// Generate atlas bitmap
		// The glyphs array (or fontGeometry) contains positioning data for typesetting text.
		generator.generate(glyphs.data(), glyphs.size());

		msdfgen::BitmapConstRef<byte, 4> atlasView = generator.atlasStorage();
		width = atlasView.width;
		height = atlasView.height;
		res->pixels.resize(static_cast<size_t>(width) * height * 4);
		std::memcpy(res->pixels.data(), atlasView.pixels, res->pixels.size());

		res->success = true;
		return res;
	}

	bool Font::generateAtlas(EngineDevice& device, std::string_view filepath, EngineCore::BindlessTextureManager& texManager)
	{
		auto res = generateSdf(filepath);
		if (not res->success) return false;
		const auto& w = res->width;
		const auto& h = res->height;

		// upload atlas texture to GPU
		const size_t size = static_cast<size_t>(w) * static_cast<size_t>(h) * (sizeof(byte) * 4);
		GBuffer stagingBuffer
		{
			device, size, 1, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		};
		// copy font atlas texture in reverse order
		stagingBuffer.map(size);
		byte* p = reinterpret_cast<msdfgen::byte*>(stagingBuffer.getMappedMemory());
		for (int y = 0; y < h; ++y)
		{
			const byte* src = res->pixels.data() + (h - 1 - y) * w * 4;
			void* dst = (p + y * w * 4);
			memcpy(dst, src, w * 4);
		}
		stagingBuffer.unmap();

		auto atlasTexture = Image::fromFontAtlas(device, stagingBuffer, w, h);
		texIndex = texManager.registerTexture(atlasTexture);

		for (const msdf_atlas::GlyphGeometry& g : res->glyphs)
		{
			double l, b, r, t;
			g.getQuadAtlasBounds(l, b, r, t);
			GlyphInfo gl = {};
			gl.u0 = float(l / w);
			gl.v0 = float(1.0 - t / h);
			gl.u1 = float(r / w);
			gl.v1 = float(1.0 - b / h);
			g.getQuadPlaneBounds(l, b, r, t);
			gl.l = float(l);
			gl.b = float(b);
			gl.r = float(r);
			gl.t = float(t);
			gl.advance = float(g.getAdvance());

			glyphInfos[g.getCodepoint()] = gl;
		}

		getMetrics();
		texture = std::move(atlasTexture);
		return true;
	}

	float Font::getKerning(char32_t a, char32_t b) const
	{
		msdfgen::GlyphIndex idxA{};
		msdfgen::GlyphIndex idxB{};
		msdfgen::getGlyphIndex(idxA, fontHandle, a);
		msdfgen::getGlyphIndex(idxB, fontHandle, b);

		double k = 0;
		msdfgen::getKerning(k, fontHandle, idxA.getIndex(), idxB.getIndex());
		return static_cast<float>(k);
	}

	void Font::getMetrics()
	{
		if (not fontMetrics)
		{
			fontMetrics = std::make_unique<msdfgen::FontMetrics>();
			const bool r = msdfgen::getFontMetrics(*fontMetrics, fontHandle, msdfgen::FontCoordinateScaling::FONT_SCALING_EM_NORMALIZED);
			assert(r && "could not get font metrics");
		}
	}

	ScaledFontMetrics Font::getScaledMetrics(float viewportHeight, float fontScale) const
	{
		const float scale = fontScale / static_cast<float>(fontMetrics->emSize);
		const float invViewportH = 1.f / viewportHeight;

		ScaledFontMetrics metrics = {};
		metrics.ascender = static_cast<float>(fontMetrics->ascenderY) * scale * invViewportH;
		metrics.descender = static_cast<float>(fontMetrics->descenderY) * scale * invViewportH;
		metrics.lineHeight = static_cast<float>(fontMetrics->lineHeight) * scale * invViewportH;
		metrics.totalFontHeight = metrics.ascender - metrics.descender;
		return metrics;
	}
}