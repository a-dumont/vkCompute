#include "vulkanTools/vulkanTools.h"
#include "vkComputer/vkComputer.h"
#include <chrono>
#include <cstring>
#include <sstream>

std::chrono::time_point<std::chrono::steady_clock> t1, t2;
vkTools::VersionInfo appVersion = vkTools::VersionInfo();
const char* requiredLayers[1];
uint32_t pDevIdx = 0;
uint32_t dataLength = 256;
uint32_t chunkSize = 1<<23;

int main(int argc, char* argv[])
{
	// Device selection from CLI
	if(argc != 0)
	{
		std::stringstream convert{ argv[1] };
		if (!(convert >> pDevIdx)){}
	}
	if(argc > 1)
	{
		std::stringstream convert{ argv[2] };
		if (!(convert >> dataLength)){}
	}

	// Data creation 
	uint32_t* dataIn = (uint32_t*) malloc(2*dataLength*sizeof(uint32_t));
	uint32_t* dataOut = (uint32_t*) malloc(dataLength*sizeof(uint32_t));
	uint32_t chunks = dataLength/chunkSize;
	if(chunks == 0){chunkSize = dataLength; chunks=1;}
	
	for(uint32_t j=0;j<dataLength;j++)
	{
		dataIn[j] = j;
		dataIn[j+dataLength] = dataLength-1-j;
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
	
	// Pipeline init
	t1 = std::chrono::steady_clock::now();
	vkTools::ComputePipeline pipeline = vkTools::ComputePipeline(&logicalDevice, "Shaders/1d_uint_vAdd_256.spv");
	t2 = std::chrono::steady_clock::now();	
	std::cout<<"Pipeline config time: ";
	std::cout<<std::chrono::duration_cast<std::chrono::microseconds>(t2-t1).count();
	std::cout<<"us"<<std::endl;

	// Computer init
	t1 = std::chrono::steady_clock::now();
	Computer computer = Computer(&pipeline,256);
	computer.createDescriptorSetLayout(3);
	t2 = std::chrono::steady_clock::now();	
	std::cout<<"Computer config time: ";
	std::cout<<std::chrono::duration_cast<std::chrono::microseconds>(t2-t1).count();
	std::cout<<"us\n"<<std::endl;

	// Print device
	std::cout<<logicalDevice.getPhysicalDeviceInfo()->getProperties().deviceName<<std::endl;
	//logicalDevice.getPhysicalDeviceInfo()->printDeviceInfo();
	
	// Limits
	uint32_t maxComputeWorkGroupCountX = logicalDevice.getPhysicalDeviceInfo()->getProperties().limits.maxComputeWorkGroupCount[0];
	uint32_t maxComputeWorkGroupCountY = logicalDevice.getPhysicalDeviceInfo()->getProperties().limits.maxComputeWorkGroupCount[1];
	uint32_t maxComputeWorkGroupCountZ = logicalDevice.getPhysicalDeviceInfo()->getProperties().limits.maxComputeWorkGroupCount[2];
	
	uint32_t maxComputeWorkGroupSizeX = logicalDevice.getPhysicalDeviceInfo()->getProperties().limits.maxComputeWorkGroupSize[0];
	uint32_t maxComputeWorkGroupSizeY = logicalDevice.getPhysicalDeviceInfo()->getProperties().limits.maxComputeWorkGroupSize[1];
	uint32_t maxComputeWorkGroupSizeZ = logicalDevice.getPhysicalDeviceInfo()->getProperties().limits.maxComputeWorkGroupSize[2];
	std::cout<<"Max workgroups: "<<maxComputeWorkGroupCountX<<" "<<maxComputeWorkGroupCountY<<" "<<maxComputeWorkGroupCountZ<<std::endl;
	std::cout<<"Workgroups max size: "<<maxComputeWorkGroupSizeX<<" "<<maxComputeWorkGroupSizeY<<" "<<maxComputeWorkGroupSizeZ<<std::endl;
	std::cout<<"Workgroups max invocation size: "<<logicalDevice.getPhysicalDeviceInfo()->getProperties().limits.maxComputeWorkGroupInvocations<<std::endl;
	std::cout<<std::endl;

	// Memory allocation
	VkBuffer gpuBuffer;
	VkDeviceMemory gpuMemory;
	uint32_t *stagingPtr;

	t1 = std::chrono::steady_clock::now();
	for(uint32_t i=0; i<50;i++)
	{
		computer.createBuffer(3*sizeof(uint32_t)*chunkSize, 
					VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |  
					VK_BUFFER_USAGE_TRANSFER_SRC_BIT |	
					VK_BUFFER_USAGE_TRANSFER_DST_BIT,	
					//VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				    VK_MEMORY_PROPERTY_HOST_CACHED_BIT	|
					VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
					gpuBuffer, gpuMemory);

		// CPU-GPU memory mapping
		vkMapMemory(pipeline.getLogicalDevice()->getLogicalDevice(), 
					gpuMemory, 0, 
					3*sizeof(uint32_t)*chunkSize, 0, (void**)&stagingPtr);

		// Define buffer usage
		VkDescriptorBufferInfo bufferInfoInput[3]; // Single buffer split into In1, In2, Out
		VkWriteDescriptorSet descriptorWrite[3]; 
		computer.fillBaseWriteDescriptorSet(3,descriptorWrite);

		bufferInfoInput[0].buffer = gpuBuffer;
		bufferInfoInput[0].offset = 0;
		bufferInfoInput[0].range = sizeof(uint32_t)*chunkSize;
		descriptorWrite[0].pBufferInfo = &bufferInfoInput[0];
	
		bufferInfoInput[1].buffer = gpuBuffer;
		bufferInfoInput[1].offset = sizeof(uint32_t)*chunkSize;
		bufferInfoInput[1].range = sizeof(uint32_t)*chunkSize;
		descriptorWrite[1].pBufferInfo = &bufferInfoInput[1];

		bufferInfoInput[2].buffer = gpuBuffer;
		bufferInfoInput[2].offset = 2*sizeof(uint32_t)*chunkSize;
		bufferInfoInput[2].range = sizeof(uint32_t)*chunkSize;
		descriptorWrite[2].pBufferInfo = &bufferInfoInput[2];

		vkUpdateDescriptorSets(pipeline.getLogicalDevice()->getLogicalDevice(), 3, descriptorWrite, 0, nullptr);

		// First chunk transfer
		std::memcpy(stagingPtr,dataIn,chunkSize*sizeof(uint32_t));	
		std::memcpy(stagingPtr+chunkSize,dataIn+dataLength,chunkSize*sizeof(uint32_t));	

		// Record command buffer	
		computer.recordCommandBuffer(computer.getCommandBuffer(),chunkSize);

		for(uint32_t j=1;j<chunks;j++)
		{

		// Compute previous chunk
		computer.compute();
		
		// Copy next chunk
		std::memcpy(stagingPtr,dataIn+j*chunkSize,chunkSize*sizeof(uint32_t));	
		std::memcpy(stagingPtr+chunkSize,dataIn+dataLength+j*chunkSize,chunkSize*sizeof(uint32_t));	


		// Copy buffer to working memory
		std::memcpy(dataOut+(j-1)*chunkSize,stagingPtr+2*chunkSize,chunkSize*sizeof(uint32_t));

		}
		computer.compute();
		std::memcpy(dataOut+(chunks-1)*chunkSize,stagingPtr+2*chunkSize,chunkSize*sizeof(uint32_t));

		// Cleanup
		vkDestroyBuffer(pipeline.getLogicalDevice()->getLogicalDevice(), gpuBuffer, nullptr);
		vkFreeMemory(pipeline.getLogicalDevice()->getLogicalDevice(), gpuMemory, nullptr);

	}
	t2 = std::chrono::steady_clock::now();	
	std::cout<<"Compute time: ";
	std::cout<<std::chrono::duration_cast<std::chrono::milliseconds>(t2-t1).count()/50;
	std::cout<<"ms\n"<<std::endl;

	
	for(uint32_t j=0;j<dataLength;j++)
	{
		if(dataOut[j] != dataLength-1)
		{
			std::cout<<dataIn[j]+dataIn[j+dataLength]<<std::endl;
			std::cout<<"Error!"<<" "<<j<<" "<<dataOut[j]<<std::endl;
			break;
		}
	}

	free(dataIn);
	free(dataOut);

	return 0;
}
