// Copyright 2026 Simon Liimatainen. All rights reserved.
// do not include Box3D headers here, to avoid leaking C types into the global scope where this file is included

#pragma once
#include <cstdint>

namespace b3cpp
{
	class Wrapper
	{
	public:
		const uint32_t* getIdBuffer() const { return idBuffer; }
		uint32_t* getIdBuffer() { return idBuffer; }
		virtual bool isIdValid() = 0;

	protected:
		// 12 byte buffer that should fit any "id" type in Box3D
		uint32_t idBuffer[3] = { 0, 0, 0 };
	};
}