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

	// an input axis accumulates current values from its bound keys, resulting in a total sum
	class InputAxis
	{
	public:
		InputAxis(const std::string& name = "") : name(name) {};
		virtual void onUpdate(EngineWindow* window);

	protected:
		friend InputAxisHandle;
		friend InputEventHandle;

		float value = 0.f;
		std::string name;
		std::vector<KeyBinding> keyBindings;
	};

	// input events are like axes, but they keep their value until explicitly "consumed", they may also require a key combo
	class InputEvent : public InputAxis
	{
	public:
		InputEvent(EInputEvent trigger, const std::string& name = "") : InputAxis(name), trigger(trigger) {};

	protected:
		EInputEvent trigger;

		virtual void onUpdate(EngineWindow* window) override;
	};
	
	class InputSystem 
	{
	public:
		InputSystem(EngineWindow* window);
		~InputSystem();

		InputAxisHandle addInputAxis(const std::string& name = "");
		InputEventHandle addInputEvent(EInputEvent trigger = EInputEvent::COMBO, const std::string& name = "");

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
