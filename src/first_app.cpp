#include "first_app.hpp"
#include "simple_render_system.hpp"
#include "keyboard_movement_controller.hpp"
#include "cursor_movement_controller.hpp"
#include "lve_buffer.hpp"


#define GLM_FORCE_RADIANS // Force GLM to use radians for all angle arguments
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // Force GLM to use depth range of 0.0 to 1.0 (Vulkan's depth range)
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <stdexcept>
#include <chrono>
#include <array>

namespace lve {

    struct GlobalUbo {
        glm::mat4 projectionView{ 1.f };
        glm::vec4 ambientLightColor{ 1.f, 1.f, 1.f, .02f };
        glm::vec3 lightPosition{ -1.f };
        alignas(16) glm::vec4 lightColor{ 1.f };
    };

	FirstApp::FirstApp() {
        globalPool = LveDescriptorPool::Builder(lveDevice)
            .setMaxSets(LveSwapChain::MAX_FRAMES_IN_FLIGHT)
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, LveSwapChain::MAX_FRAMES_IN_FLIGHT)
            .build();
		loadGameObjects();
	}

	FirstApp::~FirstApp() {}

    /*
        The reason for orthographic projection to include aspect ratio value. 
        The critical problem with having projection value to be orthographic projection(-1, 1, -1, 1, -1, 1), is that from canonical view volume, it maps its content to view port. 
        If the view port is does have 1:1 aspect ratio, lets say that the view is wide, the render will stretch the canonical view volume to match the view port width and then write to the buffer. 
        To compensate for the stretching, however, we cannot change the behavior of canonical view volume to frame buffer because that is a fixed function. 
        Instead of compensating during that stage, we can manipulation the projection before we convert it to canonical view volume. 

        This concept also applies to perspective projection.
    */
	void FirstApp::run() {

        std::vector<std::unique_ptr<LveBuffer>> uboBuffers(LveSwapChain::MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < uboBuffers.size(); i++) {
            uboBuffers[i] = std::make_unique<LveBuffer>(
                lveDevice,
                sizeof(GlobalUbo),
                1,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
            uboBuffers[i]->map();
        }

        auto globalSetLayout = LveDescriptorSetLayout::Builder(lveDevice)
            .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
            .build();

        std::vector<VkDescriptorSet> globalDescriptorSets(LveSwapChain::MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < globalDescriptorSets.size(); i++) {
            auto bufferInfo = uboBuffers[i]->descriptorInfo();
            LveDescriptorWriter(*globalSetLayout, *globalPool)
                .writeBuffer(0, &bufferInfo)
                .build(globalDescriptorSets[i]);
        }

        const float MAX_FRAME_TIME = 1/100;

		SimpleRenderSystem simpleRenderSystem{ lveDevice, lveRenderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout()};
        LveCamera camera{};
        // camera.setViewDirection(glm::vec3(0.f), glm::vec3(0.5f, 0.f, 1.f));
        camera.setViewTarget(glm::vec3(-1.f, -2.f, 2.f), glm::vec3(0.f, 0.f, 2.5f));

        auto viewObject = LveGameObject::createGameObject();
        viewObject.transform.translation.z = -2.5f;
        KeyboardMovementController cameraController{};
        CursorMovementController cursorCameraControl{};

        auto currentTime = std::chrono::high_resolution_clock::now();

		while (!lveWindow.shouldClose()) {

            auto newTime = std::chrono::high_resolution_clock::now();

            float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();

            currentTime = newTime; 

            //frameTime = glm::min(frameTime, MAX_FRAME_TIME);

            cameraController.moveInPlaneXZ(lveWindow.getGLFWwindow(), frameTime, viewObject);
            cursorCameraControl.moveInPlaneXY(lveWindow.getGLFWwindow(), frameTime, viewObject);

            camera.setViewYXZ(viewObject.transform.translation, viewObject.transform.rotation); // Set a matrix that will orient the world space to specific direction. 

			glfwPollEvents(); // Poll for and process events
            float aspect = lveRenderer.getAspectRatio();
            //camera.setOrthographicProjection(-aspect, aspect, -1, 1, -1, 1);
            camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 1000.f); // Set a matrix that will define the view volume (viewing range) and transform it into a canonical view volume (This projection is for Vulkan). 

            if (auto commandBuffer = lveRenderer.beginFrame()) {
                int frameIndex = lveRenderer.getFrameIndex();
                FrameInfo frameInfo{
                    frameIndex,
                    frameTime,
                    commandBuffer,
                    camera, 
                    globalDescriptorSets[frameIndex],
                    gameObjects
                };

                // update
                GlobalUbo ubo{};
                ubo.projectionView = camera.getProjection() * camera.getView();
                uboBuffers[frameIndex]->writeToBuffer(&ubo);
                uboBuffers[frameIndex]->flush();

                // render
				lveRenderer.beginSwapChainRenderPass(commandBuffer);
				simpleRenderSystem.renderGameObjects(frameInfo);
				lveRenderer.endSwapChainRenderPass(commandBuffer);
				lveRenderer.endFrame();
			}
		}

		vkDeviceWaitIdle(lveDevice.device()); // Wait for the device to finish before exiting
	}

    // temporary helper function, creates a 1x1x1 cube centered at offset with an index buffer
    std::unique_ptr<LveModel> createCubeModel(LveDevice& device, glm::vec3 offset) {
        LveModel::Builder modelBuilder{};
        modelBuilder.vertices = {
            // left face (white)
            {{-.5f, -.5f, -.5f}, {.9f, .9f, .9f}},
            {{-.5f, .5f, .5f}, {.9f, .9f, .9f}},
            {{-.5f, -.5f, .5f}, {.9f, .9f, .9f}},
            {{-.5f, .5f, -.5f}, {.9f, .9f, .9f}},

            // right face (yellow)
            {{.5f, -.5f, -.5f}, {.8f, .8f, .1f}},
            {{.5f, .5f, .5f}, {.8f, .8f, .1f}},
            {{.5f, -.5f, .5f}, {.8f, .8f, .1f}},
            {{.5f, .5f, -.5f}, {.8f, .8f, .1f}},

            // top face (orange, remember y axis points down)
            {{-.5f, -.5f, -.5f}, {.9f, .6f, .1f}},
            {{.5f, -.5f, .5f}, {.9f, .6f, .1f}},
            {{-.5f, -.5f, .5f}, {.9f, .6f, .1f}},
            {{.5f, -.5f, -.5f}, {.9f, .6f, .1f}},

            // bottom face (red)
            {{-.5f, .5f, -.5f}, {.8f, .1f, .1f}},
            {{.5f, .5f, .5f}, {.8f, .1f, .1f}},
            {{-.5f, .5f, .5f}, {.8f, .1f, .1f}},
            {{.5f, .5f, -.5f}, {.8f, .1f, .1f}},

            // nose face (blue)
            {{-.5f, -.5f, 0.5f}, {.1f, .1f, .8f}},
            {{.5f, .5f, 0.5f}, {.1f, .1f, .8f}},
            {{-.5f, .5f, 0.5f}, {.1f, .1f, .8f}},
            {{.5f, -.5f, 0.5f}, {.1f, .1f, .8f}},

            // tail face (green)
            {{-.5f, -.5f, -0.5f}, {.1f, .8f, .1f}},
            {{.5f, .5f, -0.5f}, {.1f, .8f, .1f}},
            {{-.5f, .5f, -0.5f}, {.1f, .8f, .1f}},
            {{.5f, -.5f, -0.5f}, {.1f, .8f, .1f}},
        };
        for (auto& v : modelBuilder.vertices) {
            v.position += offset;
        }

        modelBuilder.indices = { 0,  1,  2,  0,  3,  1,  4,  5,  6,  4,  7,  5,  8,  9,  10, 8,  11, 9,
                                12, 13, 14, 12, 15, 13, 16, 17, 18, 16, 19, 17, 20, 21, 22, 20, 23, 21 };

        return std::make_unique<LveModel>(device, modelBuilder);
    }


	void FirstApp::loadGameObjects() {

        std::shared_ptr<LveModel> lveModel = LveModel::createModelFromFile(lveDevice, "models/colored_cube.obj");

        auto cube = LveGameObject::createGameObject();
        cube.model = lveModel;
        cube.transform.translation = { .0f, .5f, 2.5f }; // This is to center the object to the center of view volumn. Vulkan z range is [0.0, 1.0] instead of OpenGL z range [-1.0, 1.0]
        cube.transform.scale = { .5f, .5f, .5f }; 

        gameObjects.emplace(cube.getId(), std::move(cube));

        std::shared_ptr<LveModel> lveModel2 = LveModel::createModelFromFile(lveDevice, "models/FinalBaseMesh.obj");

        auto gameObject = LveGameObject::createGameObject();
        gameObject.model = lveModel2;
        gameObject.transform.translation = { -2.f, .5f, 2.5f }; // This is to center the object to the center of view volumn. Vulkan z range is [0.0, 1.0] instead of OpenGL z range [-1.0, 1.0]
        gameObject.transform.scale = { .5f, .5f, .5f };
        gameObject.transform.rotation = {glm::radians(180.f), 0.f, 0.f};
        

        gameObjects.emplace(gameObject.getId(), std::move(gameObject));

        std::shared_ptr<LveModel> lveModel3 = LveModel::createModelFromFile(lveDevice, "models/smooth_vase.obj");

        auto vase = LveGameObject::createGameObject();
        vase.model = lveModel3;
        vase.transform.scale = glm::vec3{ 3.f };
        vase.transform.translation = { 3.f, 0.5f, 2.5f };

        gameObjects.emplace(vase.getId(), std::move(vase));

        std::shared_ptr<LveModel> lveModel4 = LveModel::createModelFromFile(lveDevice, "models/flat_vase.obj");

        auto flatVase = LveGameObject::createGameObject();
        flatVase.model = lveModel4;
        flatVase.transform.scale = glm::vec3{ 3.f };
        flatVase.transform.translation = { 0.f, 0.5f, .0f };

        gameObjects.emplace(flatVase.getId(), std::move(flatVase));

        std::shared_ptr<LveModel> lveModel5 = LveModel::createModelFromFile(lveDevice, "models/quad.obj");
        
        auto quad = LveGameObject::createGameObject();
        quad.model = lveModel5;
        quad.transform.translation = { .0f, .5f, .0f };
        quad.transform.scale = { 10.f, 1.f, 10.f };

        gameObjects.emplace(quad.getId(), std::move(quad));
	}

}