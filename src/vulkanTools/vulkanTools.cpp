#include "vulkanTools.h"
#include <glm/ext/vector_float2.hpp>
#include <limits>
#include <stdexcept>
using namespace vkTools;

////////////////////////////////////////////////////////////////////////////////
//       ___                        _____               _ _ _                 //
//      / _ \ _   _  ___ _   _  ___|  ___|_ _ _ __ ___ (_) (_) ___  ___       //
//     | | | | | | |/ _ \ | | |/ _ \ |_ / _` | '_ ` _ \| | | |/ _ \/ __|      //
//     | |_| | |_| |  __/ |_| |  __/  _| (_| | | | | | | | | |  __/\__ \      //
//      \__\_\\__,_|\___|\__,_|\___|_|  \__,_|_| |_| |_|_|_|_|\___||___/      //
////////////////////////////////////////////////////////////////////////////////

void QueueFamilyInfo::init(VkQueueFamilyProperties queue)
{
	queueFamily = queue;
	queueCount = queueFamily.queueCount;
	flags = queueFamily.queueFlags;
	if(VK_QUEUE_GRAPHICS_BIT & flags){graphicsSupport=true;}else{graphicsSupport=false;}
	if(VK_QUEUE_COMPUTE_BIT & flags){computeSupport=true;}else{computeSupport=false;}
	if(VK_QUEUE_TRANSFER_BIT & flags){transferSupport=true;}else{transferSupport=false;}
	if(VK_QUEUE_SPARSE_BINDING_BIT & flags){sparseBindingSupport=true;}
	else{sparseBindingSupport=false;}
	if(VK_QUEUE_PROTECTED_BIT & flags){protectedBit=true;}else{protectedBit=false;}
	if(VK_QUEUE_VIDEO_DECODE_BIT_KHR & flags){videoDecodeSupport=true;}
	else{videoDecodeSupport=false;}
	if(VK_QUEUE_VIDEO_ENCODE_BIT_KHR & flags){videoEncodeSupport=true;}
	else{videoEncodeSupport=false;}
	if(VK_QUEUE_OPTICAL_FLOW_BIT_NV & flags){opticalFlowNVRSupport=true;}
	else{opticalFlowNVRSupport=false;}
}

bool QueueFamilyInfo::hasGraphicsSupport(){return graphicsSupport;}
bool QueueFamilyInfo::hasComputeSupport(){return computeSupport;}
bool QueueFamilyInfo::hasTransferSupport(){return transferSupport;}
bool QueueFamilyInfo::hasSparseBindingSupport(){return sparseBindingSupport;}
bool QueueFamilyInfo::hasVideoDecodeSupport(){return videoDecodeSupport;}
bool QueueFamilyInfo::hasVideoEncodeSupport(){return videoEncodeSupport;}
bool QueueFamilyInfo::hasOpticalFlowNVRSupport(){return opticalFlowNVRSupport;}
bool QueueFamilyInfo::isProtected(){return protectedBit;}

uint32_t QueueFamilyInfo::getQueueCount(){return queueCount;}

VkQueueFamilyProperties QueueFamilyInfo::getQueueFamily(){return queueFamily;}

VkQueueFlags QueueFamilyInfo::getFlags(){return flags;}

////////////////////////////////////////////////////////////////////////////////
//    ____  _               _           _ ____             _                  //
//   |  _ \| |__  _   _ ___(_) ___ __ _| |  _ \  _____   _(_) ___ ___  ___    //
//   | |_) | '_ \| | | / __| |/ __/ _` | | | | |/ _ \ \ / / |/ __/ _ \/ __|   //
//   |  __/| | | | |_| \__ \ | (_| (_| | | |_| |  __/\ V /| | (_|  __/\__ \   //
//   |_|   |_| |_|\__, |___/_|\___\__,_|_|____/ \___| \_/ |_|\___\___||___/   //
//                |___/                                                       //
////////////////////////////////////////////////////////////////////////////////

void PhysicalDeviceInfo::init(VkPhysicalDevice device)
{
	physicalDevice = device;

	features2 = {};
	features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	features12 = {};
	features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
	features11 = {};
	features11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
	features2.pNext = &features12;
	features12.pNext = &features11;
	vkGetPhysicalDeviceFeatures(physicalDevice,&features);
	vkGetPhysicalDeviceFeatures2(physicalDevice,&features2);
	vkGetPhysicalDeviceProperties(physicalDevice,&properties);

	// Get the number of extensions and queue families
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &howmanyExtensions, nullptr);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &howmanyQueueFamilies, nullptr);
	
	// Memory allocation
	extensions = (VkExtensionProperties*) malloc(howmanyExtensions*sizeof(VkExtensionProperties));
	queueFamilies = (VkQueueFamilyProperties*) malloc(howmanyQueueFamilies*sizeof(VkQueueFamilyProperties));
	queueFamiliesInfo = (QueueFamilyInfo*) malloc(howmanyQueueFamilies*sizeof(QueueFamilyInfo));
	graphicsFamilies = (uint32_t*) malloc(howmanyQueueFamilies*sizeof(uint32_t));
	computeFamilies = (uint32_t*) malloc(howmanyQueueFamilies*sizeof(uint32_t));
	
	// Get the extensions and queue families
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &howmanyExtensions, extensions);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &howmanyQueueFamilies, queueFamilies);
	
	howmanyGraphicsFamilies=0;
	howmanyComputeFamilies=0;
	for(uint32_t i=0;i<howmanyQueueFamilies;i++)
	{
		queueFamiliesInfo[i].init(queueFamilies[i]);
		if(queueFamiliesInfo[i].hasGraphicsSupport()==true)
		{
			graphicsFamilies[howmanyGraphicsFamilies] = i;
			howmanyGraphicsFamilies+=1;
		}
		if(queueFamiliesInfo[i].hasComputeSupport()==true)
		{
			computeFamilies[howmanyComputeFamilies] = i; 
			howmanyComputeFamilies+=1;
		}
	}

	for(uint32_t i=0;i<howmanyExtensions;i++)
	{
		if(strcmp(VK_KHR_SWAPCHAIN_EXTENSION_NAME,extensions[i].extensionName)==0)
		{
			swapChainSupport = true;
			break;
		}
	}
	isInit = true;

	
	//if(features12.shaderInputAttachmentArrayNonUniformIndexing == VK_FALSE)
	if(features12.shaderSampledImageArrayNonUniformIndexing == VK_FALSE)
	{
		throw std::runtime_error("Non uniform Indexing not available!");	
	}
}

PhysicalDeviceInfo::~PhysicalDeviceInfo()
{
	if(isInit)
	{
		free(extensions);
		free(queueFamilies);
		free(queueFamiliesInfo);
		free(graphicsFamilies);
		free(computeFamilies);
	}
}

VkPhysicalDevice PhysicalDeviceInfo::getPhysicalDevice(){return physicalDevice;}

VkPhysicalDeviceFeatures PhysicalDeviceInfo::getFeatures(){return features;}
VkPhysicalDeviceFeatures2 PhysicalDeviceInfo::getFeatures2(){return features2;}
VkPhysicalDeviceProperties PhysicalDeviceInfo::getProperties(){return properties;}
VkExtensionProperties* PhysicalDeviceInfo::getExtensions(){return extensions;}
VkQueueFamilyProperties* PhysicalDeviceInfo::getQueueFamilies(){return queueFamilies;}
QueueFamilyInfo* PhysicalDeviceInfo::getQueueFamiliesInfo(){return queueFamiliesInfo;}

uint32_t* PhysicalDeviceInfo::getGraphicsFamilies(){return graphicsFamilies;}
uint32_t* PhysicalDeviceInfo::getComputeFamilies(){return computeFamilies;}

uint32_t PhysicalDeviceInfo::getHowmanyExtensions(){return howmanyExtensions;}
uint32_t PhysicalDeviceInfo::getHowmanyQueueFamilies(){return howmanyQueueFamilies;}

uint32_t PhysicalDeviceInfo::getHowmanyGraphicsFamilies(){return howmanyGraphicsFamilies;}
uint32_t PhysicalDeviceInfo::getHowmanyComputeFamilies(){return howmanyComputeFamilies;}

bool PhysicalDeviceInfo::hasSwapChainSupport(){return swapChainSupport;}

void PhysicalDeviceInfo::printDeviceInfo()
{
	std::cout<<"Device: "<<properties.deviceName<<std::endl;
	std::cout<<"\t"<<"Has swap chain support: "<<std::boolalpha<<swapChainSupport<<std::endl;
	std::cout<<"\t"<<"Has "<<howmanyQueueFamilies<<" queue families."<<std::endl;
	for(uint32_t i=0;i<howmanyQueueFamilies;i++)
	{
		std::cout<<"\tQueue family number "<<i; 
		std::cout<<" has "<<queueFamiliesInfo[i].getQueueCount()<<" queues that support";
		std::cout<<"\n\t "<<string_VkQueueFlags(queueFamiliesInfo[i].getFlags());
		std::cout<<std::endl;
	}
}

void VulkanBase::initPhysicalDevices()
{
	physicalDevicesCount = 0;
	vkEnumeratePhysicalDevices(appInstance, &physicalDevicesCount, nullptr);

	requiredDeviceExtensionsCount = 1;
	requiredDeviceExtensions = (const char**) malloc(requiredExtensionsCount*sizeof(char*));
	requiredDeviceExtensions[0] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
	
	if (physicalDevicesCount == 0)
	{
		throw std::runtime_error("failed to find GPUs with Vulkan support!");
	}
	
	physicalDevices = (VkPhysicalDevice*) malloc(physicalDevicesCount*sizeof(VkPhysicalDevice));
	physicalDevicesInfo = (PhysicalDeviceInfo*) 
	malloc(physicalDevicesCount*sizeof(PhysicalDeviceInfo));
	
	//std::cout<<"Looking for physical devices.\n"<<std::endl;	
	vkEnumeratePhysicalDevices(appInstance, &physicalDevicesCount, physicalDevices);
	for(uint32_t i=0;i<physicalDevicesCount;i++)
	{
		physicalDevicesInfo[i].init(physicalDevices[i]);
		//physicalDevicesInfo[i].printDeviceInfo();
	}
}

VkPhysicalDevice* VulkanBase::getPhysicalDevices(){return physicalDevices;}
PhysicalDeviceInfo* VulkanBase::getPhysicalDevicesInfo(){return physicalDevicesInfo;}

////////////////////////////////////////////////////////////////////////////////
//          __     __     _ _               ____                              //
//          \ \   / /   _| | | ____ _ _ __ | __ )  __ _ ___  ___              //
//           \ \ / / | | | | |/ / _` | '_ \|  _ \ / _` / __|/ _ \             //
//            \ V /| |_| | |   < (_| | | | | |_) | (_| \__ \  __/             //
//             \_/  \__,_|_|_|\_\__,_|_| |_|____/ \__,_|___/\___|             //
////////////////////////////////////////////////////////////////////////////////

VulkanBase::VulkanBase(VersionInfo appVersion, uint32_t nReqLayers, const char** reqLayers)
{
	applicationVersion = appVersion;
	requiredLayersCount = nReqLayers;
	requiredLayers = reqLayers;	

	initLayers();

	initExtensions();

	initVulkanInstance();

	initPhysicalDevices();

}

VulkanBase::~VulkanBase()
{
	free(availableExtensions);	
	
	free(availableLayers);	
	
	free(requiredDeviceExtensions);
	
	free(physicalDevices);	
	free(physicalDevicesInfo);

	vkDestroyInstance(appInstance, nullptr);
}

void VulkanBase::printLayers()
{
	
	std::cout << "Required layers: " << std::endl;
	for(uint32_t i=0;i<requiredLayersCount;i++)
	{
		std::cout << "\t" << requiredLayers[i] << std::endl;
	}

	std::cout << "Available Layers: " << std::endl;
	for(uint32_t i=0;i<availableLayersCount;i++)
	{
		std::cout << "\t" << availableLayers[i].layerName << std::endl;
	}
}

bool VulkanBase::checkLayer(const char* layer)
{
		bool layerFound = false;
		for(uint32_t j=0;j<availableLayersCount;j++)
		{
			if(strcmp(layer,availableLayers[j].layerName)==0)
			{
				layerFound = true;
				break;
			}
		}
		return layerFound;
}

void VulkanBase::initLayers()
{
	vkEnumerateInstanceLayerProperties(&availableLayersCount, nullptr);
	availableLayers = (VkLayerProperties*) malloc(availableLayersCount*sizeof(VkLayerProperties));
	vkEnumerateInstanceLayerProperties(&availableLayersCount, availableLayers);
	
	//vulkanBase::printLayers();

	for(uint32_t i=0;i<requiredLayersCount;i++)
	{
		if(VulkanBase::checkLayer(requiredLayers[i]) == false)
		{
				std::cout<<"Layer: "<<requiredLayers[i]<<" not found."<<std::endl;
				throw std::runtime_error("Missing Layer!");
		}
	}	
	
	//std::cout<<"All required layers found."<<std::endl;
}

void VulkanBase::printExtensions()
{
	std::cout << "Required extensions: " << std::endl;
	for(uint32_t i=0;i<requiredExtensionsCount;i++)
	{
		std::cout << "\t" << requiredExtensions[i] << std::endl;
	}

	std::cout << "Available extensions: " << std::endl;
	for(uint32_t i=0;i<availableExtensionsCount;i++)
	{
		std::cout << "\t" << availableExtensions[i].extensionName << std::endl;
	}
}

bool VulkanBase::checkExtension(const char* extension)
{
		bool extensionFound = false;
		for(uint32_t j=0;j<availableExtensionsCount;j++)
		{
			if(strcmp(extension,availableExtensions[j].extensionName)==0)
			{
				extensionFound = true;
				break;
			}
		}
		return extensionFound;
}

void VulkanBase::initExtensions()
{
	vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionsCount, nullptr);
	availableExtensions = (VkExtensionProperties*) 
						malloc(availableExtensionsCount*sizeof(VkExtensionProperties));
	vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionsCount, availableExtensions);
	
	requiredExtensionsCount = 0;
	//requiredExtensions = std::get<1>(glfwExtensionsInfo);
	
	//vulkanBase::printExtensions();

	for(uint32_t i=0;i<requiredExtensionsCount;i++)
	{
		if(VulkanBase::checkExtension(requiredExtensions[i]) == false)
		{
				std::cout<<"Extension: "<<requiredExtensions[i]<<" not found."<<std::endl;
				throw std::runtime_error("Missing Extension!");
		}
	}	
	
	//std::cout<<"All required extensions found."<<std::endl;
}

void VulkanBase::initVulkanInstance()
{	
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.apiVersion = VK_API_VERSION_1_2;
	
	// Engine
	VersionInfo engine = VersionInfo();
	uint32_t variant = engine.variant;
	uint32_t major = engine.major;
	uint32_t minor = engine.minor;
	uint32_t patch = engine.patch;
    appInfo.pEngineName = engine.name;
    appInfo.engineVersion=VK_MAKE_API_VERSION(variant,major,minor,patch);
    
	// Application
	variant = applicationVersion.variant;
	major = applicationVersion.major;
	minor = applicationVersion.minor;
	patch = applicationVersion.patch;
	appInfo.pApplicationName = applicationVersion.name;
    appInfo.applicationVersion = VK_MAKE_API_VERSION(variant, major, minor, patch);
		
	VkInstanceCreateInfo instanceCreateInfo{};

	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pApplicationInfo = &appInfo;
	instanceCreateInfo.enabledExtensionCount = requiredExtensionsCount;
	instanceCreateInfo.ppEnabledExtensionNames = requiredExtensions;

    instanceCreateInfo.enabledLayerCount = requiredLayersCount;
    instanceCreateInfo.ppEnabledLayerNames = requiredLayers;

	/*
	std::vector<VkValidationFeatureEnableEXT>  validation_feature_enables = {VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT};

    VkValidationFeaturesEXT validation_features{VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT};
    validation_features.enabledValidationFeatureCount = 1;
    validation_features.pEnabledValidationFeatures    = validation_feature_enables.data();
	instanceCreateInfo.pNext = &validation_features;*/

	if (vkCreateInstance(&instanceCreateInfo, nullptr, &appInstance) != VK_SUCCESS) 
	{
    	throw std::runtime_error("failed to create instance!");
	}
	else
	{
		std::cout<<"Application name: "<<applicationVersion.name
				<<"\nVersion: "<<applicationVersion.version
				<<"\nEngine name: "<<engine.name
				<<"\nEngine version: "<<engine.version
				<<"\n"<<std::endl;
	}
}

uint32_t VulkanBase::getRequiredLayersCount(){return requiredLayersCount;}
const char** VulkanBase::getRequiredLayers(){return requiredLayers;}
uint32_t VulkanBase::getPhysicalDevicesCount(){return physicalDevicesCount;}
uint32_t VulkanBase::getRequiredExtensionsCount(){return requiredExtensionsCount;}
const char** VulkanBase::getRequiredExtensions(){return requiredExtensions;}

////////////////////////////////////////////////////////////////////////////////
//     _                _           _ ____             _                      //
//    | |    ___   __ _(_) ___ __ _| |  _ \  _____   _(_) ___ ___  ___        //
//    | |   / _ \ / _` | |/ __/ _` | | | | |/ _ \ \ / / |/ __/ _ \/ __|       //
//    | |__| (_) | (_| | | (_| (_| | | |_| |  __/\ V /| | (_|  __/\__ \       //
//    |_____\___/ \__, |_|\___\__,_|_|____/ \___| \_/ |_|\___\___||___/       //
//                |___/                                                       //
////////////////////////////////////////////////////////////////////////////////

LogicalDevice::LogicalDevice(VulkanBase* base, uint32_t physDevIdx, uint32_t usage)
{
	vkBase = base;
	physicalDeviceIndex = physDevIdx;
	usageBits = usage;
	physicalDevice = base->getPhysicalDevices()[physicalDeviceIndex];
	physicalDeviceInfo = base->getPhysicalDevicesInfo()[physicalDeviceIndex];
	physicalDeviceFeatures = physicalDeviceInfo.getFeatures();
	physicalDeviceFeatures2 = physicalDeviceInfo.getFeatures2();

	//VkSurfaceKHR surface = vkBase->getSurface();

	if(usageBits == 0){throw std::runtime_error("Usage bits are not set!");}

	uint32_t howmanyQueueFamilies = physicalDeviceInfo.getHowmanyQueueFamilies();
	QueueFamilyInfo* queueFamiliesInfo = physicalDeviceInfo.getQueueFamiliesInfo();

	bool queueUsable;
	VkBool32 presentSupport;
	presentSupport = 0;
	for(uint32_t i=0;i<howmanyQueueFamilies;i++)
	{
		queueUsable = true;
		// Graphics Bit
		if(usageBits & 1)
		{
			queueUsable &= queueFamiliesInfo[i].hasGraphicsSupport();
		}
		// Transfer Bit
		if(usageBits & 2)
		{
			queueUsable &= queueFamiliesInfo[i].hasTransferSupport();
		}
		// Compute Bit
		if(usageBits & 4)
		{
			queueUsable &= queueFamiliesInfo[i].hasComputeSupport();
		}
		// Present Bit
		if(usageBits & 8)
		{
			//vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
			queueUsable &= presentSupport;
		}
		if(queueUsable)
		{
			queueFamilyIndex = i;
			break;
		}
	}
	if(!queueUsable)
	{
		throw std::runtime_error("No usable queue family!");
	}

	queueCount = 0;
	if(usageBits & 1)
	{
		graphicsCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		graphicsCreateInfo.queueCount = 1;
		graphicsCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos[queueCount] = graphicsCreateInfo;
		queueCount += 1;
	}
	// Transfer Bit
	if(usageBits & 2)
	{
		transferCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		transferCreateInfo.queueCount = 1;
		transferCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos[queueCount] = transferCreateInfo;
		queueCount += 1;
	}
	// Compute Bit
	if(usageBits & 4)
	{
		computeCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		computeCreateInfo.queueCount = 1;
		computeCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos[queueCount] = computeCreateInfo;
		queueCount += 1;
	}
	// Present Bit
	if(usageBits & 8)
	{
		presentCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		presentCreateInfo.queueCount = 1;
		presentCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos[queueCount] = presentCreateInfo;
		createInfo.enabledExtensionCount = nExtensions;
		requiredExtensions[0] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
		createInfo.ppEnabledExtensionNames = requiredExtensions;
		queueCount += 1;
	}
	
	queueCreateInfos[0].queueFamilyIndex = queueFamilyIndex;

	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = queueCreateInfos;
	createInfo.queueCreateInfoCount = 1;
	//createInfo.pEnabledFeatures = &physicalDeviceFeatures;
	createInfo.enabledLayerCount = base->getRequiredLayersCount();
    createInfo.ppEnabledLayerNames = base->getRequiredLayers();
	createInfo.pNext = &physicalDeviceFeatures2;

	VkResult r;
	r = vkCreateDevice(physicalDevice, &createInfo, nullptr, &logicalDevice);
 	if(r != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create logical device!");
	}
	logicalDeviceInit = true;
	if(usageBits & 1)
	{
		vkGetDeviceQueue(logicalDevice, queueFamilyIndex, 0, &graphicsQueue);
	}
	// Transfer Bit
	if(usageBits & 2)
	{
		vkGetDeviceQueue(logicalDevice, queueFamilyIndex, 0, &transferQueue);
	}
	// Compute Bit
	if(usageBits & 4)
	{
		vkGetDeviceQueue(logicalDevice, queueFamilyIndex, 0, &computeQueue);
	}
	// Present Bit
	if(usageBits & 8)
	{
		vkGetDeviceQueue(logicalDevice, queueFamilyIndex, 0, &presentQueue);
	}
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = queueFamilyIndex;

	r = vkCreateCommandPool(logicalDevice, &poolInfo, nullptr, &commandPool); 
	if (r != VK_SUCCESS) 
	{
		throw std::runtime_error("failed to create command pool!");
	}


}

LogicalDevice::~LogicalDevice()
{
	if(logicalDeviceInit)
	{
		vkDestroyCommandPool(logicalDevice, commandPool, nullptr);
		vkDestroyDevice(logicalDevice, nullptr);
	}
}

VkQueue LogicalDevice::getGraphicsQueue()
{
	if(!(usageBits&1)){throw std::runtime_error("Logical device has no graphics queue!");}
	return graphicsQueue;
}

VkQueue LogicalDevice::getTransferQueue()
{
	if(!(usageBits&2)){throw std::runtime_error("Logical device has no transfer queue!");}
	return transferQueue;
}

VkQueue LogicalDevice::getComputeQueue()
{
	if(!(usageBits&4)){throw std::runtime_error("Logical device has no compute queue!");}
	return computeQueue;
}

VkQueue LogicalDevice::getPresentQueue()
{
	if(!(usageBits&8)){throw std::runtime_error("Logical device has no present queue!");}
	return presentQueue;
}

VulkanBase* LogicalDevice::getVulkanBase(){return vkBase;}
VkPhysicalDevice LogicalDevice::getPhysicalDevice(){return physicalDevice;}
PhysicalDeviceInfo* LogicalDevice::getPhysicalDeviceInfo(){return &physicalDeviceInfo;}
VkDevice LogicalDevice::getLogicalDevice(){return logicalDevice;}
VkCommandPool LogicalDevice::getCommandPool(){return commandPool;}

////////////////////////////////////////////////////////////////////////////////
//  ____                            _       ____  _            _ _            //
// / ___|___  _ __ ___  _ __  _   _| |_ ___|  _ \(_)_ __   ___| (_)_ __   ___ //
//| |   / _ \| '_ ` _ \| '_ \| | | | __/ _ \ |_) | | '_ \ / _ \ | | '_ \ / _ \//
//| |__| (_) | | | | | | |_) | |_| | ||  __/  __/| | |_) |  __/ | | | | |  __///
// \____\___/|_| |_| |_| .__/ \__,_|\__\___|_|   |_| .__/ \___|_|_|_| |_|\___|//
//                     |_|                         |_|                        //
////////////////////////////////////////////////////////////////////////////////

ComputePipeline::ComputePipeline(LogicalDevice* device, const char* sFile)
{
	logicalDevice = device;
	vkBase = logicalDevice->getVulkanBase();
	shaderFile = sFile;

	// Layout
	layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.setLayoutCount = 0; // Optional
	layoutInfo.pSetLayouts = nullptr; // Optional
	layoutInfo.pushConstantRangeCount = 0; // Optional
	layoutInfo.pPushConstantRanges = &pcRange; // Optional

	//createPipeline();
}

ComputePipeline::~ComputePipeline()
{
	cleanup();
}

void ComputePipeline::cleanup()
{
	vkDestroyPipeline(logicalDevice->getLogicalDevice(), pipeline, nullptr);
	vkDestroyPipelineLayout(logicalDevice->getLogicalDevice(), layout, nullptr);
}

void ComputePipeline::recreatePipeline()
{
	cleanup();
	createPipeline();
}

void ComputePipeline::createPipeline()
{
	std::tuple<char*, uint32_t> shaderBuffer = readShaderFile(shaderFile);
	VkShaderModule shaderModule = createShader(shaderBuffer);

	VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
	computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	computeShaderStageInfo.module = shaderModule;
	computeShaderStageInfo.pName = "main";

	VkResult r;
	r = vkCreatePipelineLayout(logicalDevice->getLogicalDevice(), &layoutInfo, nullptr, &layout); 
	if (r != VK_SUCCESS) 
	{
    	throw std::runtime_error("failed to create pipeline layout!");
	}

	// Pipeline
	pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.layout = layout;
	pipelineInfo.stage = computeShaderStageInfo;;
	
	r = vkCreateComputePipelines(logicalDevice->getLogicalDevice(), 
						VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline); 
	if (r != VK_SUCCESS) 
	{
		throw std::runtime_error("failed to create graphics pipeline!");
	}

    vkDestroyShaderModule(logicalDevice->getLogicalDevice(), shaderModule, nullptr);
	free(std::get<0>(shaderBuffer));
}

std::tuple<char*,uint32_t> ComputePipeline::readShaderFile(const char* fileName)
{	
    std::ifstream file(fileName, std::ios::ate | std::ios::binary);
	size_t bufferSize = (size_t) file.tellg();
	char* buffer = (char*) malloc(bufferSize*sizeof(char));
	file.seekg(0);
	file.read(buffer, bufferSize);
	file.close();
	return std::make_tuple(buffer,bufferSize);
}

VkShaderModule ComputePipeline::createShader(std::tuple<char*,uint32_t> bufferInfo)
{
	char* buffer = std::get<0>(bufferInfo); 
	uint32_t bufferSize = std::get<1>(bufferInfo); 
	VkShaderModuleCreateInfo sCreateInfo{};
	sCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	sCreateInfo.codeSize = bufferSize;
	sCreateInfo.pCode = reinterpret_cast<const uint32_t*>(buffer);

	VkResult r;
	VkShaderModule shaderModule;
	r = vkCreateShaderModule(logicalDevice->getLogicalDevice(),&sCreateInfo,nullptr,&shaderModule); 
	if (r != VK_SUCCESS) 
	{
		throw std::runtime_error("failed to create shader module!");
	}
	return shaderModule;
}

void ComputePipeline::setPushConstants(VkPipelineStageFlags flags, uint32_t size, uint32_t offset)
{
	layoutInfo.pushConstantRangeCount = 1;
	pcRange.stageFlags = flags;
	pcRange.offset = offset;
	pcRange.size = size;
}

void ComputePipeline::setLayoutDescriptors(uint32_t n, VkDescriptorSetLayout* descriptors)
{
	layoutInfo.setLayoutCount = 1;
	layoutInfo.pSetLayouts = descriptors;
}

VkPipeline ComputePipeline::getPipeline(){return pipeline;}
VkPipelineLayout ComputePipeline::getLayout(){return layout;}
























































