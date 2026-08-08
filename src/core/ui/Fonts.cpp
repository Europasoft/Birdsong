#include "core/ui/Fonts.h"
#include "core/gpu/Device.h"
#include "core/gpu/Image.h"
#include "core/gpu/Buffer.h"
#include "core/gpu/descriptors/BindlessTextureManager.h"

#include "thirdparty/msdf-atlas-gen-lite/msdf-atlas-gen/msdf-atlas-gen.h"

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

	const GlyphInfo& Font::getCharacter(char32_t c) const
	{
		auto it = glyphInfos.find(c);
		if (it == glyphInfos.end()) { it = glyphInfos.find('?'); };

		return it->second;
	}

	bool Font::generateAtlas(EngineDevice& device, std::string_view filepath, EngineCore::BindlessTextureManager& texManager)
	{
		using namespace msdf_atlas;
		bool success = false;
		freetypeHandle = msdfgen::initializeFreetype();
		if (freetypeHandle)
		{
			fontHandle = msdfgen::loadFont(freetypeHandle, filepath.data());
			if (fontHandle)
			{
				std::vector<GlyphGeometry> glyphs;
				FontGeometry fontGeometry(&glyphs);
				fontGeometry.loadCharset(fontHandle, 1.0, Charset::ASCII);
				// Apply MSDF edge coloring. See edge-coloring.h for other coloring strategies.
				const double maxCornerAngle = 3.0;
				for (GlyphGeometry& glyph : glyphs)
					glyph.edgeColoring(&msdfgen::edgeColoringInkTrap, maxCornerAngle, 0);
				TightAtlasPacker packer;
				packer.setDimensionsConstraint(msdf_atlas::DimensionsConstraint::SQUARE);
				// setScale for a fixed size or setMinimumScale to use the largest that fits
				packer.setMinimumScale(36.0);
				// setPixelRange or setUnitRange
				packer.setPixelRange(sdfPixelRange);
				packer.setMiterLimit(1.0);
				// Compute atlas layout - pack glyphs
				packer.pack(glyphs.data(), glyphs.size());
				// Get final atlas dimensions
				int width = 0, height = 0;
				packer.getDimensions(width, height);
				using MtsdfGeneratorFunction = void (*)(const msdfgen::BitmapSection<float, 4>&, const GlyphGeometry&, const GeneratorAttributes&);
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
				generator.setAttributes(attributes);
				generator.setThreadCount(4);
				// Generate atlas bitmap
				// The glyphs array (or fontGeometry) contains positioning data for typesetting text.
				generator.generate(glyphs.data(), glyphs.size());
				auto bitmap = static_cast<msdfgen::BitmapConstRef<byte, 4>>(generator.atlasStorage());
				width = bitmap.width;
				height = bitmap.height;

				// upload atlas texture to GPU
				const size_t size = static_cast<size_t>(width) * static_cast<size_t>(height) * (sizeof(byte) * 4);
				GBuffer stagingBuffer
				{
					device, size, 1, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
				};
				// copy font atlas texture in reverse order
				stagingBuffer.map(size);
				byte* p = reinterpret_cast<byte*>(stagingBuffer.getMappedMemory());
				for (int y = 0; y < height; ++y)
				{
					const byte* src = bitmap.pixels + (height - 1 - y) * width * 4;
					void* dst = (p + y * width * 4);
					memcpy(dst, src, width * 4);
				}
				stagingBuffer.unmap();

				texture = Image::fromFontAtlas(device, stagingBuffer, width, height);
				texIndex = texManager.registerTexture(texture);

				for (const GlyphGeometry& g : glyphs)
				{
					double l, b, r, t;
					g.getQuadAtlasBounds(l, b, r, t);
					GlyphInfo gl = {};
					gl.u0 = float(l / width);
					gl.v0 = float(1.0 - t / height);
					gl.u1 = float(r / width);
					gl.v1 = float(1.0 - b / height);
					g.getQuadPlaneBounds(l, b, r, t);
					gl.l = float(l);
					gl.b = float(b);
					gl.r = float(r);
					gl.t = float(t);
					gl.advance = float(g.getAdvance());

					glyphInfos[g.getCodepoint()] = gl;
				}
			}
		}
		return success;
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
}