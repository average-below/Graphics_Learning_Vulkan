#include "lve_model.hpp"

#include "lve_utils.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <cassert>

#ifndef ENGINE_DIR
#define ENGINE_DIR "../"
#endif

namespace std {
	template<> struct hash<lve::LveModel::Vertex> {
		size_t operator()(lve::LveModel::Vertex const& vertex) const {
			size_t seed = 0;
			lve::hashCombine(seed, vertex.position, vertex.color, vertex.normal, vertex.uv);
			return seed;
		}
	};
}

namespace lve {
	LveModel::LveModel(LveDevice& device, const LveModel::Builder &builder) : lveDevice{ device } {
		createVertexBuffers(builder.vertices);
		createIndexBuffers(builder.indices);
	}

	LveModel::~LveModel() {}

	std::unique_ptr<LveModel> LveModel::createModelFromFile(
		LveDevice& device, const std::string& filepath) {
		Builder builder{};
		builder.loadModel(ENGINE_DIR + filepath);
		return std::make_unique<LveModel>(device, builder);
	}

	/*
		Problem with: 
		lveDevice.createBuffer(
			bufferSize,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
			vertexBuffer,
			vertexBufferMemory
		);
		void* data;
		vkMapMemory(lveDevice.device(), vertexBufferMemory, 0, bufferSize, 0, &data); 
		memcpy(data, vertices.data(), static_cast<size_t>(bufferSize)); 
		vkUnmapMemory(lveDevice.device(), vertexBufferMemory);
		
		With the setup above, it is considered unoptimized because: 
		1. When using host_visible_bit, this enables linked memory region between the CPU and GPU but, this region of memory will not be the best region for GPUs to optimally use. 
		2. We would like to save the vertex and index memory directly to the memory optimal location in the GPU but, direct access to this region is prohibited when host_visible_bit is enabled. 
		3. To save the vertex and index information in the GPU you must link and map between CPU (HOST) - GPU (Unoptimized / Temporary Location) and then link and map between GPU (Unoptimized / Temporary Location) - GPU (Optimized).
		NOTE: The GPU (Unoptimized / Temporary Location) is usually referred to as a stagging buffer. 
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT: Telling Vulkan that this buffer is a temporary location and will be moved else where in the GPU. 
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT: Make this memory region visible to CPU (Host). 
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT: If CPU (Host) makes changes, same changes would be applied to this memory region.
	*/

	void LveModel::createVertexBuffers(const std::vector<Vertex>& vertices) {
		vertexCount = static_cast<uint32_t>(vertices.size());
		assert(vertexCount >= 3 && "Vertex count must be at least 3");
		VkDeviceSize bufferSize = sizeof(vertices[0]) * vertexCount;
		uint32_t vertexSize = sizeof(vertices[0]);

		LveBuffer stagingBuffer{
			lveDevice,
			vertexSize,
			vertexCount,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		};

		stagingBuffer.map();
		stagingBuffer.writeToBuffer((void*)vertices.data());

		vertexBuffer = std::make_unique<LveBuffer>(
			lveDevice,
			vertexSize,
			vertexCount,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);

		lveDevice.copyBuffer(stagingBuffer.getBuffer(), vertexBuffer->getBuffer(), bufferSize);
	}

	void LveModel::createIndexBuffers(const std::vector<uint32_t>& indices) {
		indexCount = static_cast<uint32_t>(indices.size());
		hasIndexBuffer = indexCount > 0; // This will return true or false
		if (!hasIndexBuffer) {
			return;
		}

		VkDeviceSize bufferSize = sizeof(indices[0]) * indexCount;
		uint32_t indexSize = sizeof(indices[0]);

		LveBuffer stagingBuffer{
			lveDevice,
			indexSize,
			indexCount,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		};

		stagingBuffer.map();
		stagingBuffer.writeToBuffer((void*)indices.data());

		indexBuffer = std::make_unique<LveBuffer>(
			lveDevice,
			indexSize,
			indexCount,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);

		lveDevice.copyBuffer(stagingBuffer.getBuffer(), indexBuffer->getBuffer(), bufferSize);

		//void* data;
		//vkMapMemory(lveDevice.device(), stagingBufferMemory, 0, bufferSize, 0, &data); // This sets up a pointer to the memory of the buffer, so we can write to it. The last parameter is a pointer to a pointer, which will point to the mapped memory.
		//memcpy(data, indices.data(), static_cast<size_t>(bufferSize)); // Copy the vertex data to the mapped memory. This is a CPU operation, so we need to make sure the memory is host visible.
		//vkUnmapMemory(lveDevice.device(), stagingBufferMemory); // Unmap the memory, so the GPU can access it. This is a GPU operation, so we need to make sure the memory is device local.
	}

	void LveModel::bind(VkCommandBuffer commandBuffer) {
		VkBuffer buffers[] = { vertexBuffer->getBuffer()};
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

		if (hasIndexBuffer) {
			vkCmdBindIndexBuffer(commandBuffer, indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
		}
	}

	void LveModel::draw(VkCommandBuffer commandBuffer) {
		if (hasIndexBuffer) {
			vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
		}
		else {
			vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
		}
	}

	std::vector<VkVertexInputBindingDescription> LveModel::Vertex::getBindingDescriptions() {
		std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
		bindingDescriptions[0].binding = 0;
		bindingDescriptions[0].stride = sizeof(Vertex); // The number of bytes between each vertex. This is the size of the Vertex struct, which contains position, color, and uv.
		bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // Move to the next data entry after each vertex
		return bindingDescriptions;
	}

	std::vector<VkVertexInputAttributeDescription> LveModel::Vertex::getAttributeDescriptions() {
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

		attributeDescriptions.push_back({ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position) });
		attributeDescriptions.push_back({ 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color) });
		attributeDescriptions.push_back({ 2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal) });
		attributeDescriptions.push_back({ 3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv) });

		//attributeDescriptions[0].binding = 0;
		//attributeDescriptions[0].location = 0;
		//attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3
		//attributeDescriptions[0].offset = offsetof(Vertex, position);
		//attributeDescriptions[1].binding = 0;
		//attributeDescriptions[1].location = 1;
		//attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3
		//attributeDescriptions[1].offset = offsetof(Vertex, color);
		//attributeDescriptions[2].binding = 0;
		//attributeDescriptions[2].location = 2;
		//attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT; // vec2
		//attributeDescriptions[2].offset = offsetof(Vertex, uv);

		return attributeDescriptions;
	}

	void LveModel::Builder::loadModel(const std::string& filepath) {
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str())) {
			throw std::runtime_error(warn + err);
		}

		vertices.clear();
		indices.clear();

		std::unordered_map<Vertex, uint32_t> uniqueVertices{};

		for (const auto& shape : shapes) {
			for (const auto& index : shape.mesh.indices) {
				Vertex vertex{};

				if (index.vertex_index >= 0) {
					vertex.position = {
						attrib.vertices[3 * index.vertex_index + 0],
						attrib.vertices[3 * index.vertex_index + 1],
						attrib.vertices[3 * index.vertex_index + 2],
					};

					vertex.color = {
						attrib.colors[3 * index.vertex_index + 0],
						attrib.colors[3 * index.vertex_index + 1],
						attrib.colors[3 * index.vertex_index + 2],
					};
				}

				if (index.normal_index >= 0) {
					vertex.normal = {
						attrib.normals[3 * index.normal_index + 0],
						attrib.normals[3 * index.normal_index + 1],
						attrib.normals[3 * index.normal_index + 2],
					};
				}

				if (index.texcoord_index >= 0) {
					vertex.uv = {
						attrib.texcoords[2 * index.texcoord_index + 0],
						attrib.texcoords[2 * index.texcoord_index + 1],
					};
				}

				if (uniqueVertices.count(vertex) == 0) {
					uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
					vertices.push_back(vertex);
				}

				indices.push_back(uniqueVertices[vertex]);
			}
		}
	}
} // namespace lve