#pragma once

#include <string>
#include <vector>

#include "lve_device.hpp"

namespace lve {

	struct PipelineConfigInfo {
		PipelineConfigInfo() = default;

		PipelineConfigInfo(const PipelineConfigInfo&) = delete;
		PipelineConfigInfo& operator=(const PipelineConfigInfo&) = delete;

		VkPipelineViewportStateCreateInfo viewportInfo;
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
		VkPipelineRasterizationStateCreateInfo rasterizationInfo;
		VkPipelineMultisampleStateCreateInfo multisampleInfo;
		VkPipelineColorBlendAttachmentState colorBlendAttachment;
		VkPipelineColorBlendStateCreateInfo colorBlendInfo;
		VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
		std::vector<VkDynamicState> dynamicStateEnables;
		VkPipelineDynamicStateCreateInfo dynamicStateInfo;
		VkPipelineLayout pipelineLayout = nullptr;
		VkRenderPass renderPass = nullptr;
		uint32_t subpass = 0;

	};

	class LvePipeline {
	public:
		// LvePipeline(const std::string& verFilePath, const std::string& fragFilePath);
		LvePipeline(
			LveDevice& device, 
			const std::string& verFilePath, 
			const std::string& fragFilePath, 
			const PipelineConfigInfo& configInfo
		);

		~LvePipeline();

		LvePipeline(const LvePipeline&) = delete;
		LvePipeline& operator=(const LvePipeline&) = delete;

		void bind(VkCommandBuffer commandBuffer);

		static void defaultPipelineConfigInfo(PipelineConfigInfo& configInfo);

	private: 
		static std::vector<char> readFile(const std::string& filePath); // Ensuring that readFile is a static member function, as it does not depend on any instance-specific data. This allows it to be called without creating an instance of LvePipeline.

		// void createGraphicsPipeline(const std::string& verFilePath, const std::string& fragFilePath);

		void createGraphicsPipeline(
			const std::string& verFilePath,
			const std::string& fragFilePath,
			const PipelineConfigInfo& configInfo
		);

		void createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule);

		LveDevice& lveDevice;

		VkPipeline graphicsPipeline;

		VkShaderModule vertShaderModule;
		VkShaderModule fragShaderModule;
	};
}