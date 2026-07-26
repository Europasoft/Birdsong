#pragma once

#include <memory>
#include <vector>
#include <filesystem>

namespace WorldSystem
{
	class SectorCoord;
	class Sector;
}
namespace EngineCore
{
	class EngineDevice;
	class GBuffer;
	struct MeshBuilder;
	struct Vertex;
	struct MaterialCreateInfo;
	class Material;
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

namespace WorldSystem
{
	class Mesh
	{
	public:
		Mesh(EngineCore::EngineDevice& device);
		~Mesh();

		void setMaterial(std::shared_ptr<EngineCore::Material> newMaterial);
		void setMaterial(const EngineCore::MaterialCreateInfo& info);
		std::shared_ptr<EngineCore::Material> getMaterial() const;

		void build(const std::filesystem::path& meshFilePath);
		void build(const EngineCore::MeshBuilder& meshBuilder);

		// binds the primitive's vertices to a command buffer (preparation to render)
		void bind(VkCommandBuffer commandBuffer) const;
		// records a draw call to the command buffer (final step to render mesh)
		void draw(VkCommandBuffer commandBuffer) const;

		bool useFakeScale = false; //TODO: TMP - FakeScaleTest082

		// TODO: make this cleaner
		std::unique_ptr<b3cpp::Body> physicsBody = nullptr;
		b3cpp::Body& addPhysicsBody(b3cpp::BodyDef def, b3cpp::World& w);
		b3cpp::Body& getPhysicsBody();

		void physicsTick();

	protected:
		friend class Sector;
		bool teleported = true; // true if setTransform was called since the last physics tick

	private:
		EngineCore::EngineDevice& device;

		void createVertexBuffers(const std::vector<EngineCore::Vertex>& vertices);
		void createIndexBuffers(const std::vector<uint32_t>& indices);

		std::shared_ptr<EngineCore::Material> material;

		std::unique_ptr<EngineCore::GBuffer> vertexBuffer;
		std::unique_ptr<EngineCore::GBuffer> indexBuffer;
		uint32_t vertexCount;
		uint32_t indexCount;
		bool hasIndexBuffer = false;

		//void generateOOBB(const std::vector<Vertex>& vertices);
		//Vec extent{};

	};

}