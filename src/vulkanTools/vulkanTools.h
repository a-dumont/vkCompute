#pragma once
#include <vulkan/vulkan.h>
#include<vulkan/vk_enum_string_helper.h>

#include <ios>
#include <stdexcept>
#include <cstring>
#include <algorithm>
#include <limits>
#include <fstream>
#include <glm/glm.hpp>
#include <cmath>
#include <array>
#include <iostream>

//#include "../primitives/primitives.h"

namespace vkTools
{
struct VersionInfo
{
	const char* name = "VkTools";
	const char* version = "1.0.0";
	uint32_t variant = 0;	
	uint32_t major = 1;	
	uint32_t minor = 0;	
	uint32_t patch = 0;	
};

class QueueFamilyInfo
{
	public:
		void init(VkQueueFamilyProperties queue);
		bool hasGraphicsSupport();
		bool hasComputeSupport();
		bool hasTransferSupport();
		bool hasSparseBindingSupport();
		bool hasVideoDecodeSupport();
		bool hasVideoEncodeSupport();
		bool hasOpticalFlowNVRSupport();
		bool isProtected();
		uint32_t getIndex();
		uint32_t getQueueCount();
		VkQueueFlags getFlags();
		VkQueueFamilyProperties getQueueFamily();
	private:
		VkQueueFamilyProperties queueFamily;
		VkQueueFlags flags;
		bool graphicsSupport=false;
		bool computeSupport=false;
		bool transferSupport=false;
		bool sparseBindingSupport=false;
		bool videoDecodeSupport=false;
		bool videoEncodeSupport=false;
		bool opticalFlowNVRSupport=false;
		bool protectedBit=false;
		uint32_t index;
		uint32_t queueCount;
};

class PhysicalDeviceInfo
{
	public:
		void init(VkPhysicalDevice device);
		~PhysicalDeviceInfo();
		
		VkPhysicalDevice getPhysicalDevice();
		
		VkPhysicalDeviceFeatures getFeatures();
		VkPhysicalDeviceFeatures2 getFeatures2();
		VkPhysicalDeviceProperties getProperties();
		VkExtensionProperties* getExtensions();
		VkQueueFamilyProperties* getQueueFamilies();
		QueueFamilyInfo* getQueueFamiliesInfo();

		uint32_t* getGraphicsFamilies();
		uint32_t* getComputeFamilies();
		
		uint32_t getHowmanyExtensions();
		uint32_t getHowmanyQueueFamilies();
		
		uint32_t getHowmanyGraphicsFamilies();
		uint32_t getHowmanyComputeFamilies();

		bool hasSwapChainSupport();

		void printDeviceInfo();

	private:
		VkPhysicalDevice physicalDevice;
		bool isInit = false;
		
		VkPhysicalDeviceFeatures features;
		VkPhysicalDeviceFeatures2 features2;
		VkPhysicalDeviceVulkan12Features features12;
		VkPhysicalDeviceVulkan11Features features11;
		VkPhysicalDeviceProperties properties;
		VkExtensionProperties* extensions;
		VkQueueFamilyProperties* queueFamilies;
		QueueFamilyInfo* queueFamiliesInfo;
		
		uint32_t* graphicsFamilies;
		uint32_t* computeFamilies;

		uint32_t howmanyExtensions;
		uint32_t howmanyQueueFamilies;
		
		uint32_t howmanyGraphicsFamilies;
		uint32_t howmanyComputeFamilies;

		bool swapChainSupport=false;
};

class VulkanBase
{
	public:
		VulkanBase(VersionInfo appVersion, uint32_t nReqLayers, const char** reqLayers);
		~VulkanBase();
		VkPhysicalDevice* getPhysicalDevices();
		PhysicalDeviceInfo* getPhysicalDevicesInfo();
		uint32_t getPhysicalDevicesCount();
		uint32_t getRequiredLayersCount();
		const char** getRequiredLayers();
		uint32_t getRequiredExtensionsCount();
		const char** getRequiredExtensions();

	private:
		// Window and extensions
		uint32_t requiredExtensionsCount;
		const char** requiredExtensions;
		void printExtensions();
		bool checkExtension(const char* extension);
		void initExtensions();
		uint32_t availableExtensionsCount;
		VkExtensionProperties* availableExtensions;
		VersionInfo applicationVersion;
		
		// Layers
		void printLayers();
		bool checkLayer(const char* layer);
		void initLayers();
		uint32_t requiredLayersCount, availableLayersCount;
		const char** requiredLayers;
		VkLayerProperties* availableLayers;

		// Vulkan instance
		void initVulkanInstance();
		VkApplicationInfo appInfo{};
		VkInstance appInstance;

		// Physical devices
		void initPhysicalDevices();
		uint32_t physicalDevicesCount, requiredDeviceExtensionsCount;
		const char** requiredDeviceExtensions;
		VkPhysicalDevice* physicalDevices;
		PhysicalDeviceInfo* physicalDevicesInfo;
};

class LogicalDevice
{
	public:
		LogicalDevice(VulkanBase* base, uint32_t physDevIdx, uint32_t usage);
		~LogicalDevice();
		VkQueue getGraphicsQueue();
		VkQueue getTransferQueue();
		VkQueue getComputeQueue();
		VkQueue getPresentQueue();
		VulkanBase* getVulkanBase();
		VkPhysicalDevice getPhysicalDevice();
	   	PhysicalDeviceInfo* getPhysicalDeviceInfo();
		VkDevice getLogicalDevice();
		VkCommandPool getCommandPool();	

	private:
		VulkanBase* vkBase;
		VkPhysicalDevice physicalDevice;
		PhysicalDeviceInfo physicalDeviceInfo;
		VkPhysicalDeviceFeatures physicalDeviceFeatures;
		VkPhysicalDeviceFeatures2 physicalDeviceFeatures2;
		uint32_t physicalDeviceIndex, queueFamilyIndex, usageBits, queueCount;
		void readUsageBits();
		float queuePriority = 1.0;
		bool logicalDeviceInit;

		uint32_t nExtensions=1;
		const char* requiredExtensions[1];

		VkDeviceCreateInfo createInfo{};
		VkDeviceQueueCreateInfo queueCreateInfos[4];
		VkDeviceQueueCreateInfo graphicsCreateInfo{};
		VkDeviceQueueCreateInfo transferCreateInfo{};
		VkDeviceQueueCreateInfo computeCreateInfo{};
		VkDeviceQueueCreateInfo presentCreateInfo{};
		VkCommandPoolCreateInfo poolInfo{};

		VkDevice logicalDevice;
		VkQueue graphicsQueue, computeQueue, transferQueue, presentQueue;
		VkCommandPool commandPool;
};

class ComputePipeline
{
	public:
		ComputePipeline(LogicalDevice* device, const char* sFile);
		~ComputePipeline();
		void createPipeline();
		void recreatePipeline();
		void setVertexInputAttributes(uint32_t n, VkVertexInputAttributeDescription* att);
		void setTopology(VkPrimitiveTopology topology);
		void setPushConstants(VkPipelineStageFlags flags, uint32_t size, uint32_t offset);
		VkPipeline getPipeline();
		void setLayoutDescriptors(uint32_t n, VkDescriptorSetLayout* descriptors);
		VkPipelineLayout getLayout();
		LogicalDevice* getLogicalDevice();

	private:
		LogicalDevice* logicalDevice;
		VulkanBase* vkBase;
		const char* shaderFile;
		void cleanup();

		bool isCreated = false;

		VkPipelineLayout layout;
		VkPipeline pipeline;

		std::tuple<char*,uint32_t> readShaderFile(const char* fileName);
		VkShaderModule createShader(std::tuple<char*,uint32_t> bufferInfo);

		VkPipelineLayoutCreateInfo layoutInfo{};
		VkComputePipelineCreateInfo pipelineInfo{};
		VkPushConstantRange pcRange{};
};

}
