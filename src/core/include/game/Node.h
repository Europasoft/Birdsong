#pragma once
#include "core/include/shared/INode.h"
#include "core/include/shared/Transform.h"
#include "core/include/shared/BoundaryUtils.h"

namespace EngineInterface
{
	class Node : public INode
	{
	private:
		Transform transform;

	protected:
		virtual void onSpawn() {};
		virtual void tick(float dt) {};

	public:
		// interface functions called by the engine executable, running in the DLLs memory space
		void DLL_CALL release() final override;

		void DLL_CALL onSpawnCall() final override;

		void DLL_CALL tickCall(float dt) final override;

		void DLL_CALL getTransform(uint8_t* buffer) const final override;

		void DLL_CALL setTransform(const uint8_t* buffer) final override;
	};
}