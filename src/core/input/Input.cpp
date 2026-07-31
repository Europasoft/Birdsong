#include "core/input/Input.h"
#include "core/engine/Window.h"
#include "core/types/CommonTypes.h"

#include <GLFW/glfw3.h> // GL Framework (GLFW) used to create an engine window

#include <cassert>
#include <iostream>

namespace EngineCore
{
	InputAxisHandle InputSystem::addInputAxis(const std::string& name)
	{
		axes.push_back(std::make_unique<InputAxis>(name));
		InputAxisHandle handle{ axes.back() };
		return handle;
	}

	InputEventHandle InputSystem::addInputEvent(EInputEvent trigger, const std::string& name)
	{
		axes.push_back(std::make_unique<InputEvent>(trigger, name));
		InputEventHandle handle{ axes.back() };
		return handle;
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

		Vector2D currentPos = { x, y };
		mouse->mouseDelta.x += (currentPos.x - mouse->mousePosition.x);
		mouse->mouseDelta.y += (currentPos.y - mouse->mousePosition.y);
		mouse->mousePosition = currentPos;
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

	void InputSystem::updateInputs()
	{
		mouse->mouseDelta = { 0 };

		assert(parentWindow->getGLFWwindow() && "input system: could not access glfw window");
		for (std::shared_ptr<InputAxis> axis : axes)
		{
			axis->onUpdate(parentWindow);
		}
	}

	void InputAxis::onUpdate(EngineWindow* window)
	{
		value = 0;
		for (const KeyBinding& binding : keyBindings)
		{
			const bool pressed = glfwGetKey(window->getGLFWwindow(), binding.key) == GLFW_PRESS;
			value += pressed ? binding.influence : 0;
		}
	}

	void InputEvent::onUpdate(EngineWindow* window)
	{
		if (value != 0) return;

		if (trigger == EInputEvent::COMBO)
		{
			// all bound keys must be pressed to fire the event
			for (const KeyBinding& binding : keyBindings)
			{
				if (glfwGetKey(window->getGLFWwindow(), binding.key) != GLFW_PRESS) return;
			}
			value = 1;
		}
		else if (trigger == EInputEvent::ANY)
		{
			// any bound key may fire the event
			for (const KeyBinding& binding : keyBindings)
			{
				if (glfwGetKey(window->getGLFWwindow(), binding.key) != GLFW_PRESS) continue;
				value = 1;
			}
		}
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
