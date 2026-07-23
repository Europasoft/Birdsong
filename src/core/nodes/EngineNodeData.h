#pragma once

namespace WorldSystem
{
	class EngineNodeData
	{
	public:
		virtual ~EngineNodeData() = default; // virtual destructor for dynamic_cast
	};

	class EngineNodeData_Mesh : public EngineNodeData // engine-only data (mesh, shader, etc.)
	{
	};

}