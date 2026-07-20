#pragma once
#include "core/include/shared/Transform.h"
//#include "core/include/shared/IGame.h"

#include <cstdint>
#include <cstring>
#include <utility>

// utilities to safely call functions and pass data that needs to cross the ABI boundary (executable <-> DLL)

#if defined(_WIN32) || defined(_WIN64)
	// unlike linux/mac, cross-boundary calls on windows should be guarded against __stdcall switches
	#define DLL_CALL __cdecl
#else
	#define DLL_CALL
#endif

// name of the factory function that will be defined in DLL code
#define IG_FACTORY CreateIGameInstance
// factory function name as a string, to make absolutely sure we both declare and get the function with the exact same name
#define IG_FAC_STRINGIFY_HELPER(x) #x
#define IG_FAC_STRINGIFY(x) IG_FAC_STRINGIFY_HELPER(x)
static constexpr auto igameFactoryNameString = IG_FAC_STRINGIFY(IG_FACTORY);


namespace BoundaryUtils
{
	template <typename T>
	static inline void packValue(const T& value, uint8_t*& dstPtr)
	{
		std::memcpy(dstPtr, &value, sizeof(T));
		dstPtr += sizeof(T);
	}

	template <typename T>
	static inline void unpackValue(T& value, const uint8_t*& srcPtr)
	{
		std::memcpy(&value, srcPtr, sizeof(T));
		srcPtr += sizeof(T);
	}

	static void packTransform(const Transform& t, uint8_t* outBuffer)
	{
		uint8_t* dst = outBuffer;
		packValue(t.translation.x, dst);
		packValue(t.translation.y, dst);
		packValue(t.translation.z, dst);
		packValue(t.rotation.x, dst);
		packValue(t.rotation.y, dst);
		packValue(t.rotation.z, dst);
		packValue(t.scale.x, dst);
		packValue(t.scale.y, dst);
		packValue(t.scale.z, dst);
		packValue(t.sector.x, dst);
		packValue(t.sector.y, dst);
		packValue(t.sector.z, dst);
	}

	static void unpackTransform(const uint8_t* inBuffer, Transform& t)
	{
		const uint8_t* src = inBuffer;
		unpackValue(t.translation.x, src);
		unpackValue(t.translation.y, src);
		unpackValue(t.translation.z, src);
		unpackValue(t.rotation.x, src);
		unpackValue(t.rotation.y, src);
		unpackValue(t.rotation.z, src);
		unpackValue(t.scale.x, src);
		unpackValue(t.scale.y, src);
		unpackValue(t.scale.z, src);
		unpackValue(t.sector.x, src);
		unpackValue(t.sector.y, src);
		unpackValue(t.sector.z, src);
	}

}