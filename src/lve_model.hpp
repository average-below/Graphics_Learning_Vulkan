#pragma once

#include "lve_buffer.hpp"
#include "lve_device.hpp"

#define GLM_FORCE_RADIANS // Force GLM to use radians for all angle arguments
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // Force GLM to use depth range of 0.0 to 1.0 (Vulkan's depth range)
#include <glm/glm.hpp>

#include <vector>
#include <memory>

namespace lve {
	class LveModel {
	public:

		struct Vertex { // Interleaving vertex data is more efficient for the GPU to read, as it can fetch all the data for a single vertex in one go, rather than having to fetch position, color, and uv data from separate arrays. This is especially important for large models with many vertices.
			glm::vec3 position{};
			glm::vec3 color{};
			glm::vec3 normal{};
			glm::vec2 uv{};
			static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
			static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

			bool operator==(const Vertex& other) const {
				return position == other.position && color == other.color && normal == other.normal && uv == other.uv;
			}
		};

		struct Builder {
			std::vector<Vertex> vertices{};
			std::vector<uint32_t> indices{};

			void loadModel(const std::string& filepath);
		};

		LveModel(LveDevice &device, const LveModel::Builder &builder);
		~LveModel();

		LveModel(const LveModel&) = delete;
		LveModel& operator=(const LveModel&) = delete;

		static std::unique_ptr<LveModel> createModelFromFile(LveDevice& device, const std::string& filepath);

		void bind(VkCommandBuffer commandBuffer);
		void draw(VkCommandBuffer commandBuffer);


	private: 
		void createVertexBuffers(const std::vector<Vertex>& vertices);
		void createIndexBuffers(const std::vector<uint32_t>& indices);

		LveDevice& lveDevice;
		std::unique_ptr<LveBuffer> vertexBuffer;
		uint32_t vertexCount;

		bool hasIndexBuffer = false;
		std::unique_ptr<LveBuffer> indexBuffer;
		uint32_t indexCount;

	};
}
