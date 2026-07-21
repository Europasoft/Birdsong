#include "core/input/Input.h"
#include "core/engine/Window.h"
#include "core/types/CommonTypes.h"

#include <GLFW/glfw3.h> // GL Framework (GLFW) used to create an engine window

#include <cassert>
#include <iostream>

namespace EngineCore
{

	KeyBinding::KeyBinding(uint32_t bindKey, float axisInfluence)
	{
		bindingType = InputBindingType::Axis;
		key = bindKey;
		axisValueInfluence = axisInfluence;
	}

	KeyBinding::KeyBinding(uint32_t bindKey)
	{
		bindingType = InputBindingType::Event;
		key = bindKey;
	}

	void KeyBinding::execute(InputSystem& context)
	{
		context.setAxisValue(axisIndex, axisValueInfluence);	
	}

	struct InputSystem::Mouse
	{
		Vector2D<double> mousePosition{0};
		Vector2D<double> mouseDelta{0};
		bool isFirstMouseMove = true;
		Mouse() = default;
		~Mouse() = default;
	};

	InputSystem::InputSystem(EngineWindow* window) 
		: parentWindow{window}
	{
		assert(parentWindow && "input system: initialized with no window reference");
		mouse = std::make_unique<Mouse>();
		mouse->mousePosition = { 0.f };
		mouse->mouseDelta = { 0.f };
		mouse->isFirstMouseMove = true;
	}

	InputSystem::~InputSystem()
	{}

	/*void InputSystem::keyPressedCallback(const int& key, const int& scancode, const int& action, const int& mods)
	{
		
		if (bindings.empty()) { return; }
		for (auto& kb : bindings)
		{
			// find matching key in bindings
			if (key == kb.getKey())
			{
				kb.execute(*this);
				// this makes a keypress only affect a single binding
				if (kb.consumesKeyEvents) { return; }
			}
		}
	}*/

	void InputSystem::mousePosUpdatedCallback(const double& x, const double& y) 
	{
		if (mouse->isFirstMouseMove)
		{
			// prevent jump at the very first frame
			mouse->mousePosition = { x, y };
			mouse->mouseDelta = { 0.0, 0.0 };
			mouse->isFirstMouseMove = false;
			return;
		}
		// every other frame
		Vector2D oldPos = mouse->mousePosition;
		mouse->mousePosition = { x, y };
		mouse->mouseDelta = mouse->mousePosition - oldPos;
	}

	uint32_t InputSystem::addBinding(KeyBinding binding, const std::string& newAxisName)
	{
		uint32_t axisIndex = static_cast<uint32_t>(axisValues.size());
		axisValues.push_back(InputAxis(newAxisName)); // add input axis
		binding.axisIndex = axisIndex;
		bindings.push_back(binding); // add binding
		return axisIndex;
	}

	void InputSystem::addBinding(KeyBinding binding, const uint32_t& axisIndex) 
	{
		assert(axisValues.size() > axisIndex);
		binding.axisIndex = axisIndex;
		bindings.push_back(binding);
	}

	uint32_t InputSystem::addBinding(KeyBinding binding)
	{
		return 0; // TODO: continue implementing event bindings
	}

	float InputSystem::getAxisValue(const uint32_t& index)
	{
		if (axisValues.empty()) { return 0.f; }
		return axisValues.at(index).value;
	}

	void InputSystem::setAxisValue(const uint32_t& index, const float& v)
	{
		if (axisValues.empty() || index > axisValues.size() - 1) { return; }
		axisValues[index].value = v;
	}

	void InputSystem::resetInputValues() 
	{
		mouse->mouseDelta = { 0.0 };
		//for (auto& axis : axisValues) { axis.value = 0.f; }
	}

	void InputSystem::captureMouseCursor(const bool& capture)
	{
		GLFWwindow* gw = parentWindow->getGLFWwindow();
		assert(gw && "input system: could not access glfw window");
		if (capture)
		{
			// capture (disable) cursor
			mouse->isFirstMouseMove = true;
			glfwSetInputMode(gw, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			if (glfwRawMouseMotionSupported() == GLFW_TRUE)
			{ glfwSetInputMode(gw, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE); }
		}
		else 
		{
			// release cursor (return control to system)
			glfwSetInputMode(gw, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
			glfwSetInputMode(gw, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
	}

	void InputSystem::updateBoundInputs()
	{
		if (bindings.empty() || axisValues.empty()) { return; }

		for (auto& binding : bindings)
		{
			int32_t key = binding.getKey();
			if (key <= -1 || binding.axisIndex <= -1) { continue; }
			assert(parentWindow->getGLFWwindow() && "input system: could not access glfw window");
			float v = glfwGetKey(parentWindow->getGLFWwindow(), key) == GLFW_PRESS 
						? binding.axisValueInfluence : 0.f;
			axisValues[binding.axisIndex].influences.push_back(v);
		}

		for (auto& a : axisValues) { a.applyInfluences(); }
	}

	const Vector2D<double>& InputSystem::getMouseDelta() const
	{
		return mouse->mouseDelta;
	}

	const Vector2D<double>& InputSystem::getMousePosition() const
	{
		return mouse->mousePosition;
	}

}
