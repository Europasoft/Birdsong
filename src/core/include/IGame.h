#pragma once

namespace EngineInterface
{
	class IGame
	{
	public:
		// lifecycle functions called by the engine executable
		virtual void onLoad() = 0;
		virtual void onUpdate(float deltaTime) = 0;
		virtual void onUnload() = 0;
		virtual void release() = 0; // guarantees deletion happens inside the DLL

	protected:
		virtual ~IGame() = default; // prevent the engine executable from calling "delete gamePtr;" directly
	};

}