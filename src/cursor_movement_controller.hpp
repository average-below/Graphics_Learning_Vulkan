#include "lve_game_object.hpp"
#include "lve_window.hpp"
#include <iostream>
namespace lve {
	class CursorMovementController {
	public:
		struct KeyMappings {
			int lmb = GLFW_MOUSE_BUTTON_LEFT;
			int rmb = GLFW_MOUSE_BUTTON_RIGHT;
		};

		void moveInPlaneXY(GLFWwindow* window, float dt, LveGameObject& gameObject) {
			glm::vec3 rotate{ 0 };
			float deadzone = glm::length(glm::vec3{ 0.1f, 0.1f, 0.f });

			int width, height;
			glfwGetWindowSize(window, &width, &height);
			float aspect = static_cast<float>(width) / static_cast<float>(height);
			float adjustX = static_cast<float>(width) / 2;
			float adjustY = static_cast<float>(height) / 2;


			double x, y;

			if(glfwGetMouseButton(window, keys.lmb) == GLFW_PRESS){
				glfwGetCursorPos(window, &x, &y);
				float normalizedX = ((static_cast<float>(x) - adjustX) / width)*2;
				float normalizedY = ((static_cast<float>(y) - adjustY) / height)*(-2.0);
				rotate.x += normalizedY;
				rotate.y += normalizedX;

				std::cout << "X: " << rotate.x << " Y: " << rotate.y << "\n";
			}

			if (glm::dot(rotate, rotate) > std::numeric_limits<float>::epsilon() && glm::length(rotate) > deadzone) {
				gameObject.transform.rotation += (sensitivity * dt * glm::normalize(rotate));

			}

		}

		KeyMappings keys{};
		float sensitivity{ 1.f };
	};
}