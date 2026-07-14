#include "Core/Draw/Text.h"
/*
#include <cassert>
#include <stdexcept>

namespace EngineCore
{

	TextRenderer::TextRenderer()
	{
		if (FT_Init_FreeType(&ft) != 0)
		{
			throw std::runtime_error("FreeType failed to initialize"); 
		}
	}

	bool TextRenderer::loadTypeface(CharOptions opt, std::string_view filepath, uint32_t faceIndex)
	{
		FT_Face face;
		if (FT_New_Face(ft, filepath.data(), faceIndex, &face) != 0)
		{
			return false;
		}
		if (FT_Set_Char_Size(
			face,				// handle to face object         
			0,					// char_width in 1/64 of points  
			opt.glyphSize*64,	// char_height in 1/64 of points 
			opt.screenResH,		// horizontal device resolution  
			opt.screenResV		// vertical device resolution    
			)!= 0)
		{
			return false;
		}
		assert(false);
		return false;
		// https://freetype.org/freetype2/docs/tutorial/step1.html

		// TODO: use function from other project "ConvertUTF32", together with numBytes-deduction function from json-rpg
		// both functions are now in json-rpg, which is integrated into this repo



		
	}


}
*/