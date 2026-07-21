#pragma once

#include "core/nodes/Node.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <vector>
#include <stdexcept>
#include <memory>
#include <filesystem>

namespace EngineCore
{
	class GBuffer;
	class Material;
	struct MaterialCreateInfo;
	struct MeshBuilder;
	struct Vertex;
}

// forward declare for bind/draw functions
struct VkCommandBuffer_T;
typedef struct VkCommandBuffer_T* VkCommandBuffer;

namespace b3cpp
{
	class World;
	class Body;
	struct BodyDef;
}

namespace Nodes
{
	using namespace EngineCore;

	class MeshNode : public Node
	{
	public:
		MeshNode();
		virtual ~MeshNode() override;

		void build(const std::filesystem::path& meshFilePath);
		void build(const MeshBuilder& meshBuilder);
		
		// binds the primitive's vertices to a command buffer (preparation to render)
		void bind(VkCommandBuffer commandBuffer) const;
		// records a draw call to the command buffer (final step to render mesh)
		void draw(VkCommandBuffer commandBuffer) const;

		bool useFakeScale = false; //TODO: TMP - FakeScaleTest082

		//bool isPointInsideOOBB(const Vec& point);

		// TODO: make this cleaner
		std::unique_ptr<b3cpp::Body> physicsBody = nullptr;
		b3cpp::Body& addPhysicsBody(b3cpp::BodyDef def, b3cpp::World& w);
		b3cpp::Body& getPhysicsBody();

		void physicsTick();

	private:
		void createVertexBuffers(const std::vector<Vertex>& vertices);
		void createIndexBuffers(const std::vector<uint32_t>& indices);

		std::shared_ptr<Material> material;

		std::unique_ptr<GBuffer> vertexBuffer;
		std::unique_ptr<GBuffer> indexBuffer;
		uint32_t vertexCount;
		uint32_t indexCount;
		bool hasIndexBuffer = false;

		//void generateOOBB(const std::vector<Vertex>& vertices);
		Vec extent{};
	};
}