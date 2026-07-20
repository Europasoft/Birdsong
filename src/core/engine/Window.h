#pragma once
#include "core/types/vk.h"
#include "core/input/Input.h"

#include <string>

struct GLFWwindow;

namespace EngineCore
{
	/* engine window wrapper - use windowPtr to get the actual GLFW window */
	class EngineWindow
	{
	public:
		// default constructor/destructor
		EngineWindow(int w, int h, std::string name);
		~EngineWindow();

		EngineWindow(const EngineWindow&) = delete;
		EngineWindow& operator=(const EngineWindow&) = delete;
		EngineWindow& operator=(EngineWindow&&) = delete;
		EngineWindow(EngineWindow&&) = delete;

		// true if the window should be destroyed
		const bool getCloseWindow() const;

		VkExtent2D getExtent() { return { static_cast<uint32_t>(width), static_cast<uint32_t>(height) }; }
		bool wasWindowResized() { return framebufferResized; }
		void resetWindowResizedFlag() { framebufferResized = false; }
		GLFWwindow* getGLFWwindow() { return windowPtr; }

		void createWindowSurface(VkInstance inst, VkSurfaceKHR* surface);

		void pollEvents() const;

		// middleman function that forwards glfw events to the input system
		static void keypressCallbackHandler(GLFWwindow* window, int key, int scancode, int action, int mods);
		// middleman function for recording glfw mouse position updates
		static void mousePosCallbackHandler(GLFWwindow* window, double x, double y);

		static void mouseButtonCallbackHandler(GLFWwindow* window, int button, int action, int mods);

		// mouse/keyboard events
		InputSystem input{ this };

	private:
		// window initialization using GLFW (on construct)
		void initWindow();
		static void framebufferResizedCallback(GLFWwindow* window, int width, int height);

		int width;
		int height;
		bool framebufferResized = false;

		// pointer to the GL Framework window object
		GLFWwindow* windowPtr;
		// window name
		std::string wndName;
	};

}