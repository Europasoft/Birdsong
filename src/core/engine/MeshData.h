#pragma once
#include "core/types/vk.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <vector>
#include <string>

struct VkVertexInputBindingDescription;
struct VkVertexInputAttributeDescription;

namespace EngineCore
{
	struct Vertex
	{
		glm::vec3 position{};
		glm::vec3 color{};
		glm::vec3 normal{};
		glm::vec2 uv{};

		// binding/attribute descriptions are read by the pipeline
		static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
		static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
	};

	struct MeshBuilder
	{
		std::vector<Vertex> vertices{};
		std::vector<uint32_t> indices{};
		void makeCubeMesh();
		void makeCubeMeshWireframe();
		void loadFromFile(const std::string& path);
	};

}