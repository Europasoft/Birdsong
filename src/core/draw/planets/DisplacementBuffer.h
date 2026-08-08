// Copyright 2026 Simon Liimatainen. All rights reserved.
#pragma once
#include "core/types/CommonTypes.h"
#include "core/engine/MeshData.h"

#include "core/types/vk.h"

#include <memory>
#include <vector>

namespace EngineCore
{
	class EngineDevice;
	class GBuffer;

	struct alignas(16) DisplacedVertex
	{
		glm::mat4 modelMatrix;
		glm::mat4 normalMatrix;
		uint32_t albedoTexIdx;
		uint32_t normalTexIdx;
		uint32_t roughnessTexIdx;
		uint32_t padding;
	};

	class DisplacementBuffer
	{
	public:
		DisplacementBuffer(EngineDevice& device);
		~DisplacementBuffer();

		DisplacementBuffer(const DisplacementBuffer&) = delete;
		DisplacementBuffer& operator=(const DisplacementBuffer&) = delete;


	private:
		EngineDevice& device;
	};

}