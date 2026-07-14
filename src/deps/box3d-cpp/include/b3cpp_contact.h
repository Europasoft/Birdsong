// Copyright 2026 Simon Liimatainen. All rights reserved.
// do not include Box3D headers here, to avoid leaking C types into the global scope where this file is included

#pragma once
#include "include/b3cpp_base.h"
#include "include/b3cpp_types.h"
#include <cstdint>

namespace b3cpp
{
	class Contact : public Wrapper
	{
	public:
		Contact();

	protected:
		bool isIdValid() override;
	};
}