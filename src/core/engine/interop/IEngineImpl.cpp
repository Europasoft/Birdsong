#include "core/engine/interop/IEngineImpl.h"
#include "core/engine/Engine.h"
#include "core/gpu/Device.h"
#include "core/world/World.h"
#include "core/world/Scene.h"
#include "core/world/Sector.h"
#include "core/world/NodeContainer.h"
#include "core/nodes/EngineNodeData.h"
#include "core/nodes/EMesh.h"
#include "core/gpu/Material.h"
#include "core/render/Renderer.h"

#include "core/include/shared/BoundaryUtils.h"
#include "core/include/shared/Transform.h"
#include "core/include/shared/INode.h"

#include <memory>
#include <string>
#include <cassert>

namespace EngineCore
{
	using namespace WorldSystem;
	using namespace EngineInterface;

	IEngineImpl::IEngineImpl(EngineApplication& engine)
		: engine(engine), device(*engine.device.get()), world(*engine.world.get())
	{}

	IEngineImpl::~IEngineImpl()
	{}

	// functions called from the game DLL, executes in engine

	void IEngineImpl::registerNode(INode* iNode)
	{
		// add node to the engine registry
		Sector* sector = EngineInteropUtil::getNodeSector(iNode, world.getScene(), ESectorLookup::FIND_OR_CREATE);
		sector->nodes().add(iNode, std::make_unique<EngineNodeData>(iNode, device));
	}

	void IEngineImpl::unregisterNode(INode* iNode)
	{
		// remove node from the engine registry
		Sector* sector = EngineInteropUtil::getNodeSector(iNode, world.getScene(), ESectorLookup::FIND_EXISTING);
		sector->nodes().remove(iNode);
	}

	void IEngineImpl::setMeshForNode(INode* iNode, const char* str, size_t size)
	{
		// called by the game to set which mesh a node uses
		std::string path(str, size);
		Sector* sector = EngineInteropUtil::getNodeSector(iNode, world.getScene(), ESectorLookup::FIND_EXISTING);
		EngineNodeData& eNode = *sector->nodes().getEngineNodeData(iNode);

		// TODO: this is obviously just for testing, all this should be more customizable, currently hardcoded
		// =======================================================================================================

		// create a new mesh object and attach it to the EngineNodeData
		eNode.mesh = std::make_unique<Mesh>(device);
		eNode.mesh->build(path); // load mesh from file

		// info for material
		EngineCore::ShaderFilePaths shaders(makePath("Shaders/shader.vert.spv"), makePath("Shaders/pbr.frag.spv"));
		EngineCore::MaterialCreateInfo matInfo(
				shaders, std::vector<VkDescriptorSetLayout>{ world.getScene().getSceneGlobalDescriptorSet().getLayout() },
				engine.getRenderSettings().sampleCountMSAA, engine.getRenderer().getBasePassFormats(), sizeof(EngineCore::ShaderPushConstants::MeshPushConstants)
			);
			matInfo.shadingProperties.cullModeFlags = VK_CULL_MODE_NONE;

		// create a new material
		auto material = std::make_shared<Material>(matInfo, device);

		// add elements to the material's descriptor set
		EngineCore::UBO_Struct ubo{};
		ubo.add(EngineCore::uelem::vec3); // camera position (doesn't make sense here anymore, supposed to be rendering with camera at center, from shader's perspective)
		ubo.add(EngineCore::uelem::vec3); // light position
		ubo.add(EngineCore::uelem::scalar); // roughness
		material->getDescriptorSet().addUBO(ubo, device);

		// finalize material (also finalizes the descriptor set)
		material->finalize();

		// set material on the mesh
		eNode.mesh->setMaterial(material);
	}

	void IEngineImpl::setTextureForNode(INode* iNode, const char* str, size_t size)
	{
		// TODO: implement setTextureForNode
	}

	void IEngineImpl::getMousePosition(double& x, double& y) const
	{
		const auto mp = engine.window->input.getMousePosition();
		x = mp.x;
		y = mp.y;
	}

}

namespace EngineInteropUtil
{
	using namespace EngineInterface;
	using namespace WorldSystem;

	Transform getNodeTransform(INode* iNode)
	{
		// safely get the node's transform from the game instance
		Transform tf{};
		std::vector<uint8_t> buffer(BoundaryUtils::getTransformDataSize(tf));
		// BOUNDARY CROSSING: briefly call back to the game to fill the buffer
		iNode->getTransform(buffer.data());
		BoundaryUtils::unpackTransform(buffer.data(), tf);
		return tf;
	}

	void setNodeTransform(INode* iNode, const Transform& transform)
	{
		// safely apply transform from engine to node in game memory
		std::vector<uint8_t> buffer(BoundaryUtils::getTransformDataSize(transform));
		BoundaryUtils::packTransform(transform, buffer.data());
		iNode->setTransform(buffer.data());
	}

	Sector* getNodeSector(INode* iNode, Scene& scene, const ESectorLookup& mode)
	{
		// get the sector this node belongs to, or create a new one
		const auto sectorCoord = getNodeTransform(iNode).sector;
		Sector* sector = scene.getSector(sectorCoord, mode);
		assert(sector && "failed to create sector");
		return sector;
	}
}