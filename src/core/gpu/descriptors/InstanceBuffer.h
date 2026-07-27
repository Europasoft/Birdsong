#pragma once
#include "core/gpu/Descriptors.h"
#include "core/gpu/Device.h"

#include "core/types/vk.h"
#include <glm/glm.hpp>

#include <vector>
#include <queue>
#include <memory>

namespace WorldSystem
{
	class EngineNodeData;
}

namespace EngineCore
{
	class DescriptorPool;
	class DSetLayoutBuilder;
	class DescriptorSetLayout;
	class GBuffer;

	struct alignas(16) InstanceData
	{
		glm::mat4 modelMatrix;
		glm::mat4 normalMatrix;
		uint32_t albedoTexIdx;
		uint32_t normalTexIdx;
		uint32_t roughnessTexIdx;
		uint32_t padding;
	};

	class InstanceBuffer
	{
	public:
		static constexpr uint32_t MAX_INSTANCES = 2000000;

		InstanceBuffer(EngineDevice& device);
		~InstanceBuffer();

		InstanceBuffer(const InstanceBuffer&) = delete;
		InstanceBuffer& operator=(const InstanceBuffer&) = delete;

		void addInstanceData(const InstanceData& d);
		void pushBufferToGPU(uint32_t frameIndex);
		VkDeviceAddress getDeviceAddress(uint32_t currentFrame) const;

	private:
		EngineDevice& device;
		std::unique_ptr<GBuffer> buffer;
		std::vector<InstanceData> instances;
	};

}