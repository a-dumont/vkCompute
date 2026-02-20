#include "vulkanTools/vulkanTools.h"
#include "vkComputer/vkComputer.h"
#include <chrono>
#include <sstream>

std::chrono::time_point<std::chrono::steady_clock> t1, t2;
vkTools::VersionInfo appVersion = vkTools::VersionInfo();
const char* requiredLayers[1];
uint32_t pDevIdx = 0;

int main(int argc, char* argv[])
{
	// Device selection from CLI
	if(argc != 0)
	{
		std::stringstream convert{ argv[1] };
		if (!(convert >> pDevIdx)){}
	}

	// App version
	appVersion.name = "vkCompute";
	appVersion.version = "1.0.0";
	appVersion.major = 1;
	appVersion.minor = 0;
	appVersion.patch = 0;

	// Required layers
	requiredLayers[0] = "VK_LAYER_KHRONOS_validation";

	// Vulkan base init
	t1 = std::chrono::steady_clock::now();
	vkTools::VulkanBase vkBase = vkTools::VulkanBase(appVersion,1,requiredLayers);
	t2 = std::chrono::steady_clock::now();	
	std::cout<<"Vulkan base config time: ";
	std::cout<<std::chrono::duration_cast<std::chrono::milliseconds>(t2-t1).count();
	std::cout<<"ms"<<std::endl;

	// Logical device init
	t1 = std::chrono::steady_clock::now();
	vkTools::LogicalDevice logicalDevice = vkTools::LogicalDevice(&vkBase,pDevIdx,1|2|4);
	t2 = std::chrono::steady_clock::now();	
	std::cout<<"Logical device config time: ";
	std::cout<<std::chrono::duration_cast<std::chrono::milliseconds>(t2-t1).count();
	std::cout<<"ms"<<std::endl;

	// Print device
	std::cout<<logicalDevice.getPhysicalDeviceInfo()->getProperties().deviceName<<std::endl;
	
	// Pipeline init
	t1 = std::chrono::steady_clock::now();
	vkTools::ComputePipeline pipeline = vkTools::ComputePipeline(&logicalDevice, "Shaders/base.spv");
	t2 = std::chrono::steady_clock::now();	
	std::cout<<"Pipeline config time: ";
	std::cout<<std::chrono::duration_cast<std::chrono::microseconds>(t2-t1).count();
	std::cout<<"us"<<std::endl;

	// Computer init
	t1 = std::chrono::steady_clock::now();
	Computer computer = Computer(&pipeline);
	computer.createDescriptorSetLayout(3);
	t2 = std::chrono::steady_clock::now();	
	std::cout<<"Computer config time: ";
	std::cout<<std::chrono::duration_cast<std::chrono::microseconds>(t2-t1).count();
	std::cout<<"us"<<std::endl;

	// Memory allocation
	VkBuffer gpuBuffer;
	VkDeviceMemory gpuMemory;
	uint32_t* cpuMemory;
	uint32_t dataLength = 512;

	computer.createBuffer(3*sizeof(uint32_t)*dataLength, 
					VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
					VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
					gpuBuffer, gpuMemory);

	// CPU-GPU memory mapping
	vkMapMemory(pipeline.getLogicalDevice()->getLogicalDevice(), 
					gpuMemory, 0, 
					3*sizeof(uint32_t)*dataLength, 0, (void**)&cpuMemory);

	// Define buffer usage
	VkDescriptorBufferInfo bufferInfoInput[3]; // Single buffer split into In1, In2, Out
	VkWriteDescriptorSet descriptorWrite[3]; 
	computer.fillBaseWriteDescriptorSet(3,descriptorWrite);

	bufferInfoInput[0].buffer = gpuBuffer;
	bufferInfoInput[0].offset = 0;
	bufferInfoInput[0].range = sizeof(uint32_t)*dataLength;
	descriptorWrite[0].pBufferInfo = &bufferInfoInput[0];
	
	bufferInfoInput[1].buffer = gpuBuffer;
	bufferInfoInput[1].offset = sizeof(uint32_t)*dataLength;
	bufferInfoInput[1].range = sizeof(uint32_t)*dataLength;
	descriptorWrite[1].pBufferInfo = &bufferInfoInput[1];

	bufferInfoInput[2].buffer = gpuBuffer;
	bufferInfoInput[2].offset = 2*sizeof(uint32_t)*dataLength;
	bufferInfoInput[2].range = sizeof(uint32_t)*dataLength;
	descriptorWrite[2].pBufferInfo = &bufferInfoInput[2];

	vkUpdateDescriptorSets(pipeline.getLogicalDevice()->getLogicalDevice(), 3, descriptorWrite, 0, nullptr);
	
	// Load data to gpu through mapped cpu memory
	for(uint32_t i=0;i<dataLength;i++)
	{
		cpuMemory[i] = i+1;
		cpuMemory[i+dataLength] = dataLength-1-i;
	}

	// Run the compute shader
	computer.compute(dataLength/256);

	// Read mapped memory for the output
	for(uint32_t i=0;i<dataLength;i++)
	{
		std::cout<<i<<": "<<cpuMemory[2*dataLength+i]<<"\n";
	}
	std::cout<<std::endl;

	// Cleanup
	vkUnmapMemory(pipeline.getLogicalDevice()->getLogicalDevice(), gpuMemory);
	vkDestroyBuffer(pipeline.getLogicalDevice()->getLogicalDevice(), gpuBuffer, nullptr);
	vkFreeMemory(pipeline.getLogicalDevice()->getLogicalDevice(), gpuMemory, nullptr);

	return 0;
}
