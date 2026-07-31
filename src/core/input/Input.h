#pragma once
#include "core/input/InputInclude.h"

#include <vector>
#include <string>
#include <memory>
//#include "core/types/CommonTypes.h"

template<typename T>
class Vector2D;

namespace EngineCore 
{
	class EngineWindow;
	
	// forward declaration, real class declared below
	class InputSystem;

	class InputAxis
	{
	public:
		InputAxis(const std::string& name = "") : name(name) {};

	private:
		friend InputAxisHandle;
		friend InputSystem;

		float value = 0.f;
		std::string name;
		std::vector<KeyBinding> keyBindings;
	};

	
	class InputSystem 
	{
	public:
		InputSystem(EngineWindow* window);
		~InputSystem();

		InputAxisHandle addInputAxis(const std::string& name = "");

		void updateInputs();

		// disables the system cursor, allowing for raw mouse input (use capture=false to release)
		void captureMouseCursor(const bool& capture = true);

		const Vector2D<double>& getMouseDelta() const;
		const Vector2D<double>& getMousePosition() const;

		void mousePosUpdatedCallback(const double& x, const double& y);

	private:
		EngineWindow* parentWindow;
		std::vector<std::shared_ptr<InputAxis>> axes;
		struct Mouse;
		std::unique_ptr<Mouse> mouse;
	};

}
