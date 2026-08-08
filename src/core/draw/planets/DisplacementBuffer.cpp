// Copyright 2026 Simon Liimatainen. All rights reserved.
#include "core/draw/planets/DisplacementBuffer.h"
#include "core/gpu/Buffer.h"
#include "core/gpu/Device.h"

#include <stdexcept>
#include <array>
#include <limits>
#include <utility>
#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>

namespace EngineCore
{
	DisplacementBuffer::DisplacementBuffer(EngineDevice& device)
		: device{ device }
	{
	}

	DisplacementBuffer::~DisplacementBuffer()
	{}

	
}