#pragma once

#include <vector>
#include <string>
#include "Core/Types/CommonTypes.h"

struct GLFWwindow;

namespace EngineCore 
{
	class EngineWindow;
	
	// forward declaration, real class declared below
	class InputSystem;

	enum class InputBindingType { Axis, Event }; // TODO: implement event bindings

	class KeyBinding
	{
	public:
		// axis binding constructor
		KeyBinding(uint32_t bindKey, float axisInfluence = 1.f);
		// event binding constructor
		KeyBinding(uint32_t bindKey);
		
		const int32_t& getKey() { return key; }
		const InputBindingType& getBindingType() { return bindingType; }
		void execute(InputSystem& context);
		bool consumesKeyEvents = true;

		float axisValueInfluence = 1.f;
		int32_t axisIndex = -1;

	private:
		int32_t key = -2;
		InputBindingType bindingType;
		
	};

	struct InputAxis 
	{
		InputAxis() { name = "input_axis"; }
		InputAxis(const std::string& nameIn) { name = nameIn; }
		float value = 0.f;
		std::string name;

		std::vector<float> influences;
		void applyInfluences() 
		{
			if (influences.empty()) { return; }
			float sum = 0.f;
			for (auto& inf : influences) { sum += inf; }
			if (sum != 0.f) { value = sum; }
			else { value = 0.f; }
			influences.clear();
		}
	};

	class InputSystem 
	{
	public:
		InputSystem(EngineWindow* window);

		//void keyPressedCallback(const int& key, const int& scancode, const int& action, const int& mods);
		void mousePosUpdatedCallback(const double& x, const double& y);

		//	returns the axis index if the binding is an axis input
		uint32_t addBinding(KeyBinding binding, const std::string& newAxisName);
		// add another binding to an existing axis
		void addBinding(KeyBinding binding, const uint32_t& axisIndex);
		// add an event binding, returns a binding index
		uint32_t addBinding(KeyBinding binding);


		float getAxisValue(const uint32_t& index);
		void setAxisValue(const uint32_t& index, const float& v);
		void resetInputValues();

		void updateBoundInputs();

		// disables the system cursor, allowing for raw mouse input (use capture=false to release)
		void captureMouseCursor(const bool& capture = true);

		const Vector2D<double>& getMouseDelta() const { return mouseDelta; }
		const Vector2D<double>& getMousePosition() const { return mousePosition; }

	private:
		EngineWindow* parentWindow;
		std::vector<KeyBinding> bindings;
		std::vector<InputAxis> axisValues;
		std::vector<bool> eventBindings;
		Vector2D<double> mousePosition = { 0.f };
		Vector2D<double> mouseDelta = { 0.f };
	};

}
