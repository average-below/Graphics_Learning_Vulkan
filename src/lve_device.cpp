#include "lve_device.hpp"

// std headers
#include <cstring>
#include <iostream>
#include <set>
#include <unordered_set>

namespace lve {

	// local callback functions
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData) {
		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

		return VK_FALSE;
	}

	VkResult CreateDebugUtilsMessengerEXT(
		VkInstance instance,
		const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator,
		VkDebugUtilsMessengerEXT* pDebugMessenger
	) {
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
			instance,
			"vkCreateDebugUtilsMessengerEXT");
		if (func != nullptr) {
			return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
		}
		else {
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}

	void DestroyDebugUtilsMessengerEXT(
		VkInstance instance,
		VkDebugUtilsMessengerEXT debugMessenger,
		const VkAllocationCallbacks* pAllocator) {
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
			instance,
			"vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr) {
			func(instance, debugMessenger, pAllocator);
		}
	}

	// class member functions
	LveDevice::LveDevice(LveWindow& window) : window{ window } {
		createInstance(); // By the end of the createInstance function, the "instance" member variable of the LveDevice class will hold a valid VkInstance handle, which represents the connection between the application and the Vulkan library.
		setupDebugMessenger();
		createSurface(); // This stage creates a connection between Vulkan and the Window system. This is a requirement for Vulkan development.
		pickPhysicalDevice(); // At this stage, it seeks all physical devices on the system, check if it is glfw compatible, and set the devices that meets the requirements. 
		createLogicalDevice(); // At this stage, it opens an interface for the picked device.
		createCommandPool();
	}

	LveDevice::~LveDevice() {
		vkDestroyCommandPool(device_, commandPool, nullptr);
		vkDestroyDevice(device_, nullptr);

		if (enableValidationLayers) {
			DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
		}

		vkDestroySurfaceKHR(instance, surface_, nullptr);
		vkDestroyInstance(instance, nullptr);
	}

	// Basically, narrowing down Vulkan Library to include only necessary things. 
	void LveDevice::createInstance() {
		if (enableValidationLayers && !checkValidationLayerSupport()) {
			throw std::runtime_error("validation layers requested, but not available!");
		}

		
		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "LittleVulkanEngine App"; // Set for easier debugging.
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		auto extensions = getRequiredExtensions(); // This function retrieves all the necessary extensions required for GLFW to function properly.
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
		if (enableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();

			populateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
		}
		else {
			createInfo.enabledLayerCount = 0;
			createInfo.pNext = nullptr;
		}

		// instance will be modified to get VkInstance formatted contents. 
		if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) { // Enables the creation of a Vulkan instance, which is the connection between the application and the Vulkan library. 
			throw std::runtime_error("failed to create instance!");
		}

		hasGflwRequiredInstanceExtensions();
	}

	/*
		Beginner's Note:
		Basically,

		This function seeks all available devices and verify whether they are suitable for the next stages.

		The device is considered suitable with the following conditions:
		1. Does the device have "VK_KHR_swap_chain" extension
		2. Are formats available
		3. Are presentation modes available
		4. Is samplerAnisotropy available
	*/
	void LveDevice::pickPhysicalDevice() {
		uint32_t deviceCount = 0;

		/*
		  Beginner's Note:
		  The vkEnumeratePhysicalDevices function uses vulkan loader from the vulkan library to figure out available devices.
		  Remember that the "instance" holds the connection between the vulkan library to enable the vulkan loader.

		  Also, according to Vulkan Documentation (source: https://docs.vulkan.org/refpages/latest/refpages/source/vkEnumeratePhysicalDevices.html):
		  When the "VkPhysicalDevices\* is nullptr (the last parameter for the function), the number of physical devices available is returned to pPhysicalDeviceCount (which is &deviceCount in this context).
		  */
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
		if (deviceCount == 0) {
			throw std::runtime_error("failed to find GPUs with Vulkan support!");
		}
		std::cout << "Device count: " << deviceCount << std::endl;
		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()); // At this stage, devices vector should hold all available device (graphical devices)


		for (const auto& device : devices) {
			if (isDeviceSuitable(device)) {
				physicalDevice = device;
				break;
			}
		}

		if (physicalDevice == VK_NULL_HANDLE) {
			throw std::runtime_error("failed to find a suitable GPU!");
		}

		vkGetPhysicalDeviceProperties(physicalDevice, &properties);
		std::cout << "physical device: " << properties.deviceName << std::endl;
	}

	void LveDevice::createLogicalDevice() {
		QueueFamilyIndices indices = findQueueFamilies(physicalDevice); // Remember that physicalDevices have different queueFamilies and we must figure out whether manufacture provided queueFamilies the necessary properties (support). 

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily, indices.presentFamily }; // If we could not find a queueFamily that supports both graphics and presentation, then, the indices should not match. 

		float queuePriority = 1.0f;
		for (uint32_t queueFamily : uniqueQueueFamilies) {
			VkDeviceQueueCreateInfo queueCreateInfo = {};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily; // This index must match with physical devices queueFamily index, which is set by manufacturer. 
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceFeatures deviceFeatures = {};
		deviceFeatures.samplerAnisotropy = VK_TRUE;

		VkDeviceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()); // This should be stride for each uint32_t pointers. (Most likely 8 bytes)
		createInfo.pQueueCreateInfos = queueCreateInfos.data(); // Pointers to queue family objects.

		createInfo.pEnabledFeatures = &deviceFeatures;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = deviceExtensions.data();

		// might not really be necessary anymore because device specific validation layers
		// have been deprecated
		if (enableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else {
			createInfo.enabledLayerCount = 0;
		}

		if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device_) != VK_SUCCESS) { // The device_ gets assigned.
			throw std::runtime_error("failed to create logical device!");
		}

		vkGetDeviceQueue(device_, indices.graphicsFamily, 0, &graphicsQueue_); // Should return specific queueFamily available to physical device. 
		vkGetDeviceQueue(device_, indices.presentFamily, 0, &presentQueue_); // Should return specific queueFamily available to physical device. 
	}

	void LveDevice::createCommandPool() {
		QueueFamilyIndices queueFamilyIndices = findPhysicalQueueFamilies();

		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		if (vkCreateCommandPool(device_, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create command pool!");
		}
	}

	void LveDevice::createSurface() { window.createWindowSurface(instance, &surface_); };

	bool LveDevice::isDeviceSuitable(VkPhysicalDevice device) {
		QueueFamilyIndices indices = findQueueFamilies(device);

		bool extensionsSupported = checkDeviceExtensionSupport(device);

		bool swapChainAdequate = false;
		if (extensionsSupported) {
			SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
			swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
		}

		VkPhysicalDeviceFeatures supportedFeatures;
		vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

		return indices.isComplete() && extensionsSupported && swapChainAdequate &&
			supportedFeatures.samplerAnisotropy;
	}

	void LveDevice::populateDebugMessengerCreateInfo(
		VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
		// These options can be considered as extension to lines of codes found in the createInstance() function. The createInstance() function is responsible for creating a Vulkan instance, which is the connection between the application and the Vulkan library. The populateDebugMessengerCreateInfo() function is responsible for populating a VkDebugUtilsMessengerCreateInfoEXT structure with the necessary information to create a debug messenger. The debug messenger is used to receive debug messages from the Vulkan validation layers.
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = debugCallback;
		createInfo.pUserData = nullptr;  // Optional
	}

	void LveDevice::setupDebugMessenger() {
		if (!enableValidationLayers) return;
		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		populateDebugMessengerCreateInfo(createInfo);
		if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
			throw std::runtime_error("failed to set up debug messenger!");
		}
	}

	bool LveDevice::checkValidationLayerSupport() {
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (const char* layerName : validationLayers) {
			bool layerFound = false;

			for (const auto& layerProperties : availableLayers) {
				if (strcmp(layerName, layerProperties.layerName) == 0) {
					layerFound = true;
					break;
				}
			}

			if (!layerFound) {
				return false;
			}
		}

		return true;
	}

	// Using GLFW, it fetches required extension. 
	std::vector<const char*> LveDevice::getRequiredExtensions() {
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount); // glfwExtensions gets the list of required extensions. glfwExtensionCount will also be modified to reflect the number of extension count.

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount); // Initialize the vector with glfw extensions size and insert the data about extension names into extensions vector.

		if (enableValidationLayers) {
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return extensions;
	}


	void LveDevice::hasGflwRequiredInstanceExtensions() {

		// Basically, this is just fetching all the enumerated extension available in Vulkan. 
		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr); // Just extracting number of extension. 
		std::vector<VkExtensionProperties> extensions(extensionCount); // Initialize vector with extracted counts. 
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data()); // Initialize vector by directly putting into the vector using .data() function. Note: data() function returns pointers to first element of the vector.

		// Basically, these lines of codes is just displaying all the enumerated extension available by the version of the Vulkan. 
		std::cout << "available extensions:" << std::endl;
		std::unordered_set<std::string> available;
		for (const auto& extension : extensions) {
			std::cout << "\t" << extension.extensionName << std::endl;
			available.insert(extension.extensionName);
		}

		// Basically, these lines of code is verifying whether the set Vulkan version is compatible with current GLFW version.
		std::cout << "required extensions:" << std::endl;
		auto requiredExtensions = getRequiredExtensions(); // This is fetching the minimum requirement by GLFW against Vulkan so that GLFW would function properly.
		for (const auto& required : requiredExtensions) {
			std::cout << "\t" << required << std::endl;
			if (available.find(required) == available.end()) {
				throw std::runtime_error("Missing required glfw extension");
			}
		}
	}

	/*
		Basically, this function goes though a checklist to verify whether the device is suitable for use by developer and Vulkan library.
	*/
	bool LveDevice::checkDeviceExtensionSupport(VkPhysicalDevice device) {
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr); // Similar to vkEnumeratePhysicalDevice, if the last parameter is nullptr, the pPropertyCount (in this context, &extensionCount) will be updated with number of device extensions.

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(
			device,
			nullptr,
			&extensionCount,
			availableExtensions.data()); // At this point, availableExtensions should include all available extensions by the device.

		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end()); // Iterator-based initialization. Basically inserting the "VK_KHR_swap_chain" extension name in requiredExtensions set.

		for (const auto& extension : availableExtensions) {
			requiredExtensions.erase(extension.extensionName); // Note: Remember that .erase function for set performs character-by-character comparison before erasing any data.
		}

		return requiredExtensions.empty();
	}


	QueueFamilyIndices LveDevice::findQueueFamilies(VkPhysicalDevice device) {
		QueueFamilyIndices indices;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data()); // Don't forget, queueFamilies will get the list of Queue Families with distinct properties. 
		/*
			Example: 
			0: {queueFlags=15 queueCount=16 timestampValidBits=64 minImagetransferGranularity={width=1 height=1 depth=1} 
			1: {queueFlags=12 queueCount=2 timestampValidBits=64 minImagetransferGranularity={width=1 height=1 depth=1}
			2: {queueFlags=14 queueCount=8 timestampValidBits=64 minImagetransferGranularity={width=1 height=1 depth=1}
			... Length of vector depends on device and architecture designed by manufacturer.

			queueCount is maximum number of queues for that queue family.
			Need to figure out what is timestampValidBits and minImagetransferGranularity

		*/

		/*
			Beginner's Note:
			Below are available queue properties that a queue could get. 
				VK_QUEUE_GRAPHICS_BIT = 0x00000001,
				VK_QUEUE_COMPUTE_BIT = 0x00000002,
				VK_QUEUE_TRANSFER_BIT = 0x00000004,
				VK_QUEUE_SPARSE_BINDING_BIT = 0x00000008,
				VK_QUEUE_PROTECTED_BIT = 0x00000010,
				VK_QUEUE_VIDEO_DECODE_BIT_KHR = 0x00000020,
				VK_QUEUE_VIDEO_ENCODE_BIT_KHR = 0x00000040,
				VK_QUEUE_OPTICAL_FLOW_BIT_NV = 0x00000100,
				VK_QUEUE_DATA_GRAPH_BIT_ARM = 0x00000400,
				VK_QUEUE_FLAG_BITS_MAX_ENUM = 0x7FFFFFFF

			Not all queue supports all properties. For example: (Taken during debug mode from queueFamilies vector)
			Lets say we get the following vector with the content from vsGetPhysicalDeviceQueueFamilyProperties
			{queueFlags=15 queueCount=16 timestampValidBits=64 minImagetransferGranularity={width=1 height=1 depth=1}
			{queueFlags=12 queueCount=2 timestampValidBits=64 minImagetransferGranularity={width=1 height=1 depth=1}
			{queueFlags=14 queueCount=8 timestampValidBits=64 minImagetransferGranularity={width=1 height=1 depth=1}
			... Length of vector depends on device and architecture designed by manufacturer.
			
			Lets take queueFlags=14. If we convert the unsigned int of 14 to bits, it will be 1110. If we conduct bit-wise operation & with VK_QUEUE_GRAPHICS_BIT (which is 0001). 
			The result is 0000. However, this is not the case with VK_QUEUE_COMPUTE_BIT (0010), VK_QUEUE_TRANSFER_BIT (0100), and VK_QUEUE_SPARSE_BINDING_BIT (1000).

			So, what the for each is doing below is attempting to find queueFamil(ies) that supports graphic and presentation. In some cases, two different queueFamily may be selected. 
			Example: If one family support graphics but not presentation, the for each will continue to find another queueFamily that will support the presentation or, alternatively, find a 
			queueFamily that support both graphics and presentation. 
		*/
		int i = 0;
		for (const auto& queueFamily : queueFamilies) {
			if (queueFamily.queueCount > 0 && (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)) { // Remember, in an if statement, non-zero value will be considered as true and zero to be false. That is the reason why the bit-wise operation works. 
				indices.graphicsFamily = i;
				indices.graphicsFamilyHasValue = true;
			}
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface_, &presentSupport);
			if (queueFamily.queueCount > 0 && presentSupport) {
				indices.presentFamily = i;
				indices.presentFamilyHasValue = true;
			}
			if (indices.isComplete()) {
				break;
			}

			i++;
		}

		return indices;
	}

	SwapChainSupportDetails LveDevice::querySwapChainSupport(VkPhysicalDevice device) {
		SwapChainSupportDetails details;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface_, &details.capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &formatCount, nullptr);

		if (formatCount != 0) {
			details.formats.resize(formatCount); // Changing the vector size according to number of formats. 
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &formatCount, details.formats.data());
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &presentModeCount, nullptr);

		if (presentModeCount != 0) {
			details.presentModes.resize(presentModeCount); // Changing the vector size according to number of formats. 
			vkGetPhysicalDeviceSurfacePresentModesKHR(
				device,
				surface_,
				&presentModeCount,
				details.presentModes.data());
		}
		return details;
	}

	VkFormat LveDevice::findSupportedFormat(
		const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
		for (VkFormat format : candidates) {
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
				return format;
			}
			else if (
				tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
				return format;
			}
		}
		throw std::runtime_error("failed to find supported format!");
	}

	uint32_t LveDevice::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties); // Retrieve memory properties from device.
		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) && // Remember, "1 << i" shift binary number "1" and shift if by i times. Example: 1 << 3 then 0001 -> 1000
				(memProperties.memoryTypes[i].propertyFlags & properties) == properties) { 
				return i;
			}
		}

		throw std::runtime_error("failed to find suitable memory type!");
	}

	void LveDevice::createBuffer(
		VkDeviceSize size,
		VkBufferUsageFlags usage,
		VkMemoryPropertyFlags properties,
		VkBuffer& buffer,
		VkDeviceMemory& bufferMemory) {
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(device_, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to create vertex buffer!");
		}

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device_, buffer, &memRequirements); // memRequirements now holds requirements to allow "buffer" to work accordingly. Must now request physical location down below.

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

		if (vkAllocateMemory(device_, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate vertex buffer memory!");
		}

		vkBindBufferMemory(device_, buffer, bufferMemory, 0);
	}

	VkCommandBuffer LveDevice::beginSingleTimeCommands() {
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = commandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(device_, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);
		return commandBuffer;
	}

	void LveDevice::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(graphicsQueue_, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(graphicsQueue_);

		vkFreeCommandBuffers(device_, commandPool, 1, &commandBuffer);
	}

	void LveDevice::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
		VkCommandBuffer commandBuffer = beginSingleTimeCommands();

		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = 0;  // Optional
		copyRegion.dstOffset = 0;  // Optional
		copyRegion.size = size;
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

		endSingleTimeCommands(commandBuffer);
	}

	void LveDevice::copyBufferToImage(
		VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount) {
		VkCommandBuffer commandBuffer = beginSingleTimeCommands();

		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;

		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = layerCount;

		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = { width, height, 1 };

		vkCmdCopyBufferToImage(
			commandBuffer,
			buffer,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&region);
		endSingleTimeCommands(commandBuffer);
	}

	void LveDevice::createImageWithInfo(
		const VkImageCreateInfo& imageInfo,
		VkMemoryPropertyFlags properties,
		VkImage& image,
		VkDeviceMemory& imageMemory) {
		if (vkCreateImage(device_, &imageInfo, nullptr, &image) != VK_SUCCESS) {
			throw std::runtime_error("failed to create image!");
		}

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(device_, image, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

		if (vkAllocateMemory(device_, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate image memory!");
		}

		if (vkBindImageMemory(device_, image, imageMemory, 0) != VK_SUCCESS) {
			throw std::runtime_error("failed to bind image memory!");
		}
	}

}  // namespace lve