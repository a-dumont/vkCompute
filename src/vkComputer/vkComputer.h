#pragma once

#include "../vulkanTools/vulkanTools.h"

class Computer
{
	public:
		Computer(vkTools::ComputePipeline* pipelineIn, uint32_t invocationSizeIn);
		~Computer();
		//void compute(uint32_t dispatchNumber);
		void compute();
		void createDescriptorSetLayout(uint32_t N);
		void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, 
						VkMemoryPropertyFlags properties,VkBuffer& buffer, 
						VkDeviceMemory& bufferMemory);
		void fillBaseWriteDescriptorSet(uint32_t n, VkWriteDescriptorSet* writeDescriptorSet);
		void copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size, uint32_t dstOffset, uint32_t srcOffset, VkQueue queue);
		void recordCommandBuffer(VkCommandBuffer buffer, uint32_t dispatchNumber);
		VkCommandBuffer getCommandBuffer();

	private:
		// Vulkan backend
		vkTools::VulkanBase* vkBase;
		vkTools::LogicalDevice* logicalDevice;
		vkTools::ComputePipeline* pipeline;

		// Workgroup limits
		uint32_t workGroupMaxCount[3];
		uint32_t workGroupMaxSize[3];
		uint32_t maxInvocationSize;
		uint32_t invocationSize;

		// Sync objects
		VkCommandBuffer commandBuffer;
		VkFence computeFence;
		void createSyncObjects();
		void destroySyncObjects();

		// Buffer tools
		void createCommandBuffer();
		//void recordCommandBuffer(VkCommandBuffer buffer, uint32_t dispatchNumber);
		VkCommandBuffer beginCommand();
		void endCommand(VkCommandBuffer commandBuffer);

		// Bool
		bool descriptorSetLayoutInit = false;

		// Buffers
		uint32_t dataLength = 256;
		
		VkBuffer inputBuffers;
		VkBuffer outputBuffer;
		
		VkDeviceMemory inputMemory;
		VkDeviceMemory outputMemory;

		void* pInputMemory;
		void* pOutputMemory;
		
		VkDescriptorSetLayout descriptorSetLayout;
		VkDescriptorSet descriptorSet;
		VkDescriptorPool inOutDescriptorPool;

		void createInOutBuffers();
};
