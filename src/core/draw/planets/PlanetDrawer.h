#pragma once
#include "core/gpu/Material.h"
#include "core/include/shared/Transform.h"
#include "core/types/CommonTypes.h"

#include "core/types/vk.h"

#include <memory>
#include <vector>

namespace WorldSystem
{ 
	class EngineNodeData;
	class World;
}

namespace EngineCore
{
	class EngineDevice;
	class Material;

	struct ResRad
	{
		uint32_t resolution;
		double radius;
	};

	enum class FaceDirection : uint32_t
	{
		A,
		B,
		C,
		D,
		E,
		F
	};

	struct Quad
	{
		Vector2D<float> center; // local 2D face coordinates
		float size;  // extent in 2D space
		uint32_t lodLevel;
		FaceDirection face; // +X, -X, +Y, etc.

		std::vector<std::unique_ptr<Quad>> children;

		std::unique_ptr<WorldSystem::EngineNodeData> node; // mesh geometry (only non-null if this node is a leaf)
	};

	struct PlanetNodeContext
	{
		std::vector<std::unique_ptr<Quad>> rootFaces;
		std::shared_ptr<Material> material;
	};

	class PlanetDrawer
	{
	public:
		PlanetDrawer(EngineDevice& device, WorldSystem::World& world, const RenderingFormats& formats, VkSampleCountFlagBits samples);
		~PlanetDrawer();

		PlanetDrawer(const PlanetDrawer&) = delete;
		PlanetDrawer& operator=(const PlanetDrawer&) = delete;

		void regenerate();

		void render(VkCommandBuffer commandBuffer, uint32_t frameIndex, const Transform& cameraTransform, double dt);

	private:
		EngineDevice& device;
		WorldSystem::World& world;
		std::vector<std::unique_ptr<PlanetNodeContext>> planets;

		VkCommandBuffer cmdBuffer = VK_NULL_HANDLE;
		Transform camTransform;
		double delta = 0.0;
		double currentXoffset = 800.f;
		ResRad rr = ResRad{ .resolution = 8, .radius = 100 };
		double tempTimer = 0.6;

		void updateLOD(Quad& quad, const Vec64& cameraPos, std::shared_ptr<Material> material, ResRad& r);

		void splitQuad(Quad& quad, std::shared_ptr<Material> material, ResRad& r);
		void mergeQuad(Quad& quad);

		void recurseDrawQuad(Quad& quad, Material& material);
		void drawLeaf(WorldSystem::EngineNodeData& leaf, Material& material);
	};

}