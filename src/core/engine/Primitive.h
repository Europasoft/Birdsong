#pragma once

#include "Core/GPU/Device.h"
#include "Core/GPU/Buffer.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <vector>
#include <stdexcept>
#include <memory>

namespace EngineCore 
{
	class Material;
	struct MaterialCreateInfo;

	class Primitive
	{
	public:
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

		Primitive(EngineDevice& device, const MeshBuilder& builder);
		Primitive(EngineDevice& device, const std::vector<Vertex>& vertices);
		Primitive(EngineDevice& device);
		~Primitive() = default;

		Primitive(const Primitive&) = delete;
		Primitive& operator=(const Primitive&) = delete;

		// binds the primitive's vertices to a command buffer (preparation to render)
		void bind(VkCommandBuffer commandBuffer);
		// records a draw call to the command buffer (final step to render mesh)
		void draw(VkCommandBuffer commandBuffer);

		void setMaterial(std::shared_ptr<Material> newMaterial);
		void setMaterial(const MaterialCreateInfo& info);
		std::shared_ptr<Material> getMaterial() const;

		bool useFakeScale = false; //TODO: TMP - FakeScaleTest082

		Transform& getTransform() { return transform; } // this should be changed to virtual, returning const
		void setTransform(const Transform& t) { transform = t; }
		void setTranslation(const Vec& t) { transform.translation = t; }

		bool isPointInsideOOBB(const Vec& point);

	private:
		void createVertexBuffers(const std::vector<Vertex>& vertices);
		void createIndexBuffers(const std::vector<uint32_t>& indices);

		EngineDevice& device;

		Transform transform{};
		std::shared_ptr<Material> material;

		std::unique_ptr<GBuffer> vertexBuffer;
		std::unique_ptr<GBuffer> indexBuffer;
		uint32_t vertexCount;
		uint32_t indexCount;
		bool hasIndexBuffer = false;

		void generateOOBB(const std::vector<Vertex>& vertices);
		Vec extent{};
	};
}