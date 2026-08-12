#include "simple_render_system.hpp"

#define GLM_FORCE_RADIANS // Force GLM to use radians for all angle arguments
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // Force GLM to use depth range of 0.0 to 1.0 (Vulkan's depth range)
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <stdexcept>
#include <array>

namespace lve {

	/*
		Beginner's Note:
			When using push constants in Vulkan, by default, any constants must adhere to std430 or std140 padding rule.
			scalar = 4 bytes
			vec2 = 8 bytes
			vec3 = 16 bytes (Must match vec4 padding style)
			vec4 = 16 bytes

			mat3 = 64 bytes (must match vec4 padding style)
			mat4 = 64 bytes

			If other standards are used, seek the padding requirements to avoid mis-alignment of memory when using push constants.


	*/
	struct SimplePushConstantData {
		glm::mat4 modelMatrix{ 1.f }; // Might be intuitive to use mat3 but because of data alignment and tight data packing, mat4 should be used and mat3 should be converted to mat4. 
		// alignas(16) glm::vec3 color{};
		glm::mat4 normalMatrix{ 1.f };
	};

	SimpleRenderSystem::SimpleRenderSystem(LveDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout) : lveDevice{device} {
		createPipelineLayout(globalSetLayout); // Create the pipeline layout
		createPipeline(renderPass);
	}

	SimpleRenderSystem::~SimpleRenderSystem() {
		vkDestroyPipelineLayout(lveDevice.device(), pipelineLayout, nullptr);
	}

	void SimpleRenderSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout) {

		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(SimplePushConstantData);

		std::vector<VkDescriptorSetLayout> descriptorSetLayouts{ globalSetLayout };

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.pNext = nullptr; // Optional
		pipelineLayoutInfo.flags = 0; // Optional
		pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
		pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
		if (vkCreatePipelineLayout(lveDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create pipeline layout!");
		}
	}

	void SimpleRenderSystem::createPipeline(VkRenderPass renderPass) {
		assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout is created");

		PipelineConfigInfo pipelineConfig{}; // Empty config. Information before pipeline.
		LvePipeline::defaultPipelineConfigInfo(pipelineConfig); // First set the default config.
		pipelineConfig.renderPass = renderPass; // Attach renderPass infomation. 
		pipelineConfig.pipelineLayout = pipelineLayout; // Attach pipelineLayout.

		// Set the actual pipline.
		lvePipeline = std::make_unique<LvePipeline>(
			lveDevice,
			"shaders/simple_shader.vert.spv",
			"shaders/simple_shader.frag.spv",
			pipelineConfig
		);
	}

	void SimpleRenderSystem::renderGameObjects(FrameInfo &frameInfo) {
		lvePipeline->bind(frameInfo.commandBuffer);

		vkCmdBindDescriptorSets(
			frameInfo.commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelineLayout,
			0,
			1,
			&frameInfo.globalDescriptorSet,
			0,
			nullptr);

		for (auto& keyValue : frameInfo.gameObjects) {
			auto& obj = keyValue.second;
			if (obj.model == nullptr) continue;
			//obj.transform.rotation.y = glm::mod(obj.transform.rotation.y + 0.01f, glm::two_pi<float>());
			//obj.transform.rotation.x = glm::mod(obj.transform.rotation.x + 0.005f, glm::two_pi<float>());

			SimplePushConstantData push{};
			// push.color = obj.color;
			push.modelMatrix = obj.transform.mat4();
			push.normalMatrix = obj.transform.normalMatrix();

			vkCmdPushConstants(
				frameInfo.commandBuffer,
				pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(SimplePushConstantData),
				&push
			);
			obj.model->bind(frameInfo.commandBuffer);
			obj.model->draw(frameInfo.commandBuffer);
		}
	}
}