#pragma once
#include "core/include/shared/IEngine.h"
#include "core/include/shared/Transform.h"

namespace EngineInterface
{
	class INode;
}
namespace WorldSystem
{
	class Scene;
	class Sector;
	class World;
	enum class ESectorLookup : int32_t;
}

struct Transform;

namespace EngineCore
{
	class EngineApplication;
	class EngineDevice;

	// engine-side implementation of IEngine
	class IEngineImpl : public EngineInterface::IEngine
	{
	public:
		IEngineImpl(EngineApplication& engine);
		~IEngineImpl();
	protected:
		// these functions can be invoked from the game DLL across the ABI boundary
		void DLL_CALL registerNode(EngineInterface::INode* iNode) final override;
		void DLL_CALL unregisterNode(EngineInterface::INode* iNode) final override;
		void DLL_CALL getMousePosition(double& x, double& y) const final override;
		void DLL_CALL setMeshForNode(EngineInterface::INode* iNode, const char* str, size_t size) final override;
		void DLL_CALL setTextureForNode(EngineInterface::INode* iNode, const char* str, size_t size) final override;
		void DLL_CALL setPhysicsBodyForNode(EngineInterface::INode* iNode) final override;

	private:
		EngineApplication& engine;
		EngineDevice& device;
		WorldSystem::World& world;
	};

}

namespace EngineInteropUtil
{
	Transform getNodeTransform(EngineInterface::INode* iNode);
	void setNodeTransform(EngineInterface::INode* iNode, const Transform& transform);
	WorldSystem::Sector* getNodeSector(EngineInterface::INode* iNode, WorldSystem::Scene& scene, const WorldSystem::ESectorLookup& mode);
}