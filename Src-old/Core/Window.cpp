#include "Core/Window.h"
#include "Core/Engine.h" // TODO: only used for temporary keyboard input system
#include <stdexcept>

namespace EngineCore
{

	EngineWindow::EngineWindow(int w, int h, std::string name) : width{ w }, height{ h }, wndName{ name }
	{
		initWindow();
	}

	EngineWindow::~EngineWindow()
	{
		glfwDestroyWindow(windowPtr);
		glfwTerminate();
	}

	void EngineWindow::initWindow() 
	{
		glfwInit();
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

}