#pragma once

#include "../vulkanTools/vulkanTools.h"

class Computer
{
	public:
		Computer(vkTools::VulkanBase* base, vkTools::LogicalDevice* device);
		~Computer();
		void compute();
		void loadData(uint32_t* in1, uint32_t* in2);
		void readData(uint32_t* out);

	private:
		// Vulkan backend
		vkTools::VulkanBase* vkBase;
		vkTools::LogicalDevice* logicalDevice;
		vkTools::ComputePipeline pipeline;

		// Sync objects
		VkCommandBuffer commandBuffer;
		VkSemaphore computeDoneSemaphore;
		VkFence computeFence;
		void createSyncObjects();
		void destroySyncObjects();

		// Buffer tools
		void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, 
						VkMemoryPropertyFlags properties,VkBuffer& buffer, 
						VkDeviceMemory& bufferMemory);
		void copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size, uint32_t dstOffset);
		void createCommandBuffer();
		void recordCommandBuffer(VkCommandBuffer buffer);
		VkCommandBuffer beginCommand();
		void endCommand(VkCommandBuffer commandBuffer);

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
