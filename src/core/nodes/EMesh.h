#pragma once

#include <memory>
#include <vector>
#include <filesystem>

namespace WorldSystem
{
	class SectorCoord;
	class Sector;
	class EngineNodeData;
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

		void prePhysics(EngineNodeData& data);
		void postPhysics(EngineNodeData& data);

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
		uint32_t vertexCount = 0;
		uint32_t indexCount = 0;
		bool hasIndexBuffer = false;

		//void generateOOBB(const std::vector<Vertex>& vertices);
		//Vec extent{};

	};

}