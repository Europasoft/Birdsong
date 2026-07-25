#include "core/include/game/MeshNode.h"
#include "shared/IEngine.h"

namespace EngineInterface
{
	void MeshNode::setMesh(std::string_view filepath)
	{
		// calls to IEngine cross the ABI must pass only basic types
		engine->setMeshForNode(this, filepath.data(), filepath.size());
	}
}