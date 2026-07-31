#include "core/engine/Window.h"

#include <GLFW/glfw3.h> // GL Framework (GLFW) used to create an engine window

#include <stdexcept>
#include <cassert>
#include <iostream>

namespace EngineCore
{

	EngineWindow::EngineWindow(int w, int h, std::string name) : width{ w }, height{ h }, wndName{ name }
	{
		glfwInit();
		initWindow();
	}

	EngineWindow::~EngineWindow()
	{
		glfwDestroyWindow(windowPtr);
		glfwTerminate();
	}

	void EngineWindow::initWindow() 
	{
		/* the call below prevents an OpenGL context from being created (since we're using Vulkan) 
		* otherwise can cause error with glfwCreateWindowSurface returning VK_ERROR_NATIVE_WINDOW_IN_USE_KHR */
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

		windowPtr = glfwCreateWindow(width, height, wndName.c_str(), nullptr, nullptr);
		glfwSetWindowUserPointer(windowPtr, this);
		glfwSetFramebufferSizeCallback(windowPtr, framebufferResizedCallback); // bind framebufferResizedCallback to resize event
		// subscribe to glfw input events
		glfwSetKeyCallback(getGLFWwindow(), keypressCallbackHandler);
		glfwSetCursorPosCallback(getGLFWwindow(), mousePosCallbackHandler);
		glfwSetMouseButtonCallback(getGLFWwindow(), mouseButtonCallbackHandler);
	}

	void EngineWindow::createWindowSurface(VkInstance inst, VkSurfaceKHR* surface)
	{
		if (glfwCreateWindowSurface(inst, windowPtr, nullptr, surface) != VK_SUCCESS) 
		{
			throw std::runtime_error("could not create engine window surface");
		}
	}

	void EngineWindow::updateFpsInTitle(double delta) const
	{
		static float timeAccumulator = 0.0f;
		static int frameCounter = 0;

		timeAccumulator += delta;
		frameCounter++;

		// update the title every N seconds
		if (timeAccumulator >= 0.8f && windowPtr)
		{
			float avgFps = static_cast<float>(frameCounter) / timeAccumulator;
			float frameTimeMs = (timeAccumulator / static_cast<float>(frameCounter)) * 1000.0f;

			std::string title = "Birdsong | FPS: " + std::to_string(static_cast<int>(avgFps)) +
				" (" + std::to_string(frameTimeMs).substr(0, 4) + " ms)";

			glfwSetWindowTitle(windowPtr, title.c_str());

			// reset tracking counters
			timeAccumulator = 0.0f;
			frameCounter = 0;
		}
	}

	void EngineWindow::pollEvents(double delta) const
	{
		glfwPollEvents();
		updateFpsInTitle(delta);
	}

	const bool EngineWindow::getCloseWindow() const
	{
		return glfwWindowShouldClose(windowPtr);
	}

	void EngineWindow::framebufferResizedCallback(GLFWwindow* window, int width, int height) 
	{
		auto thisWindow = reinterpret_cast<EngineWindow*>(glfwGetWindowUserPointer(window));
		thisWindow->framebufferResized = true;
		thisWindow->width = width;
		thisWindow->height = height;
	}

	void EngineWindow::keypressCallbackHandler(GLFWwindow* window, int key, int scancode, int action, int mods) 
	{
		/*	since this is a static function as required by glfw, we need to retrieve
			our InputSystem object through the glfw "window user pointer" as follows */
		EngineWindow* wp = reinterpret_cast<EngineWindow*>(glfwGetWindowUserPointer(window));
		assert(wp != NULL && "failed to process input, glfw window user pointer not set");
		//wp->input.keyPressedCallback(key, scancode, action, mods); // inform the input system
	}

	void EngineWindow::mousePosCallbackHandler(GLFWwindow* window, double x, double y)
	{
		EngineWindow* wp = reinterpret_cast<EngineWindow*>(glfwGetWindowUserPointer(window));
		assert(wp != NULL && "failed to process input, glfw window user pointer not set");
		
		wp->input.mousePosUpdatedCallback(x, y); // send updated mouse coords to input system
	}

	void EngineWindow::mouseButtonCallbackHandler(GLFWwindow* window, int button, int action, int mods) 
	{
		if (action != GLFW_PRESS) { return; }

	}
		
	void EngineWindow::toggleFullscreen()
	{
		isFullscreen = !isFullscreen;
		fullscreenChangedCounter = 20;
		if (isFullscreen)
		{
			// save current windowed metrics
			glfwGetWindowPos(windowPtr, &windowedX, &windowedY);
			glfwGetWindowSize(windowPtr, &windowedWidth, &windowedHeight);

			// query target monitor bounds
			GLFWmonitor* primary = glfwGetPrimaryMonitor();
			const GLFWvidmode* mode = glfwGetVideoMode(primary);
			int monitorX = 0, monitorY = 0;
			glfwGetMonitorPos(primary, &monitorX, &monitorY);

			// remove decorations before moving/resizing
			glfwSetWindowAttrib(windowPtr, GLFW_DECORATED, GLFW_FALSE);

			// move and resize as a standard borderless window
			glfwSetWindowPos(windowPtr, monitorX, monitorY);
			glfwSetWindowSize(windowPtr, mode->width, mode->height);
		}
		else
		{
			// restore window decorations
			glfwSetWindowAttrib(windowPtr, GLFW_DECORATED, GLFW_TRUE);

			// restore saved windowed size and position
			glfwSetWindowSize(windowPtr, windowedWidth, windowedHeight);
			glfwSetWindowPos(windowPtr, windowedX, windowedY);
		}
	}



}