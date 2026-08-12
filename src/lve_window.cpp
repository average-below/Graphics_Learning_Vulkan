#include <stdexcept>

#include "lve_window.hpp"

namespace lve {
	LveWindow::LveWindow(int w, int h, std::string name) : width{ w }, height{ h }, windowName{ name } {
		initWindow();
	}
	LveWindow::~LveWindow() {
		glfwDestroyWindow(window);
		glfwTerminate();
	}
	void LveWindow::initWindow() {
		glfwInit(); // Initialize the GLFW library
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Specify that we do not want to use OpenGL. This is required for glfwCreateWindowSurface as stated in documentation: https://www.glfw.org/docs/3.3/group__vulkan.html#ga1a24536bec3f80b08ead18e28e6ae965
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE); // Enable window resizing

		/*
			glfwCreateWindow signature: 
			GLFWwindow* glfwCreateWindow(int width, int height, const char* title, GLFWmonitor* monitor, GLFWwindow* share)
				width: The desired width, in screen coordinates, of the window.
				height: The desired height, in screen coordinates, of the window.
				title: The initial, UTF-8 encoded window title. C-style string, so we use c_str() to convert std::string to const char
				monitor: The monitor to use for full screen mode, or NULL for windowed mode.
				share: The window whose context to share resources with, or NULL to not share resources.
				Returns: The handle of the created window, or NULL if an error occurred.
		*/
		window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr); // This creates window from Operating System.
		glfwSetWindowUserPointer(window, this); // Set the user pointer to the current instance of LveWindow
		glfwSetFramebufferSizeCallback(window, framebufferResizeCallback); // Set the framebuffer resize callback function
	}

	void LveWindow::createWindowSurface(VkInstance instance, VkSurfaceKHR* surface) {
		/*
			glfwCreateWindowSurface signature:
				VkResult glfwCreateWindowSurface(VkInstance instance, GLFWwindow* window, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface)
					instance: The Vulkan instance to associate the surface with.
					window: The GLFW window to create the surface for.
					pAllocator: The allocator to use for the surface, or NULL to use the default allocator.

					**pSurface: A pointer to a VkSurfaceKHR handle that will be filled with the created surface.**
		*/

		if (glfwCreateWindowSurface(instance, window, nullptr, surface) != VK_SUCCESS) { 
			throw std::runtime_error("failed to create window surface!");
		}
	}

	void LveWindow::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
		auto lveWindow = reinterpret_cast<LveWindow*>(glfwGetWindowUserPointer(window));
		lveWindow->framebufferResized = true;
		lveWindow->width = width;
		lveWindow->height = height;

	}
}