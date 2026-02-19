#include "vkComputer.h"
#include <cmath>
#include <cstring>

Computer::Computer(vkTools::VulkanBase* base, vkTools::LogicalDevice* device) :
		pipeline(device, "../bin/Shaders/base.spv")
{
	vkBase = base;
	logicalDevice = device;

	createInOutBuffers();

	pipeline.setLayoutDescriptors(1,&descriptorSetLayout);
	pipeline.createPipeline();
	createCommandBuffer();
	createSyncObjects();
}

Computer::~Computer()
{
	destroySyncObjects();

	// Chunks staging buffer
	vkUnmapMemory(logicalDevice->getLogicalDevice(), inputMemory);
	vkDestroyBuffer(logicalDevice->getLogicalDevice(), inputBuffers, nullptr);
	vkFreeMemory(logicalDevice->getLogicalDevice(), inputMemory, nullptr);
	
	vkUnmapMemory(logicalDevice->getLogicalDevice(), outputMemory);
	vkDestroyBuffer(logicalDevice->getLogicalDevice(), outputBuffer, nullptr);
	vkFreeMemory(logicalDevice->getLogicalDevice(), outputMemory, nullptr);

	vkDestroyDescriptorPool(logicalDevice->getLogicalDevice(), inOutDescriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(logicalDevice->getLogicalDevice(), descriptorSetLayout, nullptr);
}

void Computer::loadData(uint32_t* in1, uint32_t* in2)
{
	for(uint32_t i=0;i<256;i++)
	{
		((uint32_t*) pInputMemory)[i] = in1[i];
		((uint32_t*) pInputMemory)[i+dataLength] = in2[i];
	}
}

void Computer::readData(uint32_t* out)
{
	for(uint32_t i=0;i<dataLength;i++)
	{
		out[i] = ((uint32_t*)pOutputMemory)[i];
	}
}

void Computer::createSyncObjects()
{
	

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	
	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	VkResult r;
	r = vkCreateSemaphore(logicalDevice->getLogicalDevice(), 
					&semaphoreInfo, nullptr, &computeDoneSemaphore);
	if(r != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create semaphore!");
	}

	r = vkCreateFence(logicalDevice->getLogicalDevice(), 
					&fenceInfo, nullptr, &computeFence);
	if(r != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create fence!");
	}
}

void Computer::destroySyncObjects()
{
	vkDestroySemaphore(logicalDevice->getLogicalDevice(),computeDoneSemaphore, nullptr);
    vkDestroyFence(logicalDevice->getLogicalDevice(), computeFence, nullptr);
}


//////////////////////////////////////////////////////////////////////////////////
//            ____                       _____                                  //
//           |  _ \ _ __ __ ___      __ |  ___| __ __ _ _ __ ___   ___          //
//           | | | | '__/ _` \ \ /\ / / | |_ | '__/ _` | '_ ` _ \ / _ \         //
//           | |_| | | | (_| |\ V  V /  |  _|| | | (_| | | | | | |  __/         //
//           |____/|_|  \__,_| \_/\_/   |_|  |_|  \__,_|_| |_| |_|\___|         //
//////////////////////////////////////////////////////////////////////////////////

void Computer::recordCommandBuffer(VkCommandBuffer buffer)
{
	
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0; // Optional
	beginInfo.pInheritanceInfo = nullptr; // Optional

	VkResult r;
	r = vkBeginCommandBuffer(buffer, &beginInfo);
	if(r != VK_SUCCESS)
	{
		throw std::runtime_error("failed to begin recording command buffer!");
	}

	// Triangles
	vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.getPipeline());
	vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_COMPUTE, 
					pipeline.getLayout(), 0, 1, 
					&descriptorSet, 0, nullptr);

	vkCmdDispatch(buffer, dataLength/256, 1, 1);	

	r = vkEndCommandBuffer(buffer);
	if (r != VK_SUCCESS) 
	{
		throw std::runtime_error("failed to record command buffer!");
	}
}

void Computer::compute()
{
	vkWaitForFences(logicalDevice->getLogicalDevice(), 1, &computeFence, VK_TRUE, UINT64_MAX);
	vkResetFences(logicalDevice->getLogicalDevice(), 1, &computeFence);

	vkResetCommandBuffer(commandBuffer, 0);
	recordCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &computeDoneSemaphore;
	
	VkBool32 r;
	r = vkQueueSubmit(logicalDevice->getComputeQueue(), 1, &submitInfo, 
					computeFence);
	if(r != VK_SUCCESS)
	{
		throw std::runtime_error("failed to submit compute command buffer!");
	}

	vkWaitForFences(logicalDevice->getLogicalDevice(), 1, &computeFence, VK_TRUE, UINT64_MAX);
	//vkResetFences(logicalDevice->getLogicalDevice(), 1, &computeFence);
	//vkResetCommandBuffer(commandBuffer, 0);
}



//////////////////////////////////////////////////////////////////////////////////
//             ____         __  __             _____           _                //
//            | __ ) _   _ / _|/ _| ___ _ __  |_   _|__   ___ | |___            //
//            |  _ \| | | | |_| |_ / _ \ '__|   | |/ _ \ / _ \| / __|           //
//            | |_) | |_| |  _|  _|  __/ |      | | (_) | (_) | \__ \           //
//            |____/ \__,_|_| |_|  \___|_|      |_|\___/ \___/|_|___/           //
//////////////////////////////////////////////////////////////////////////////////

void Computer::createBuffer
(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, 
VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{

	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkResult r;
	r = vkCreateBuffer(logicalDevice->getLogicalDevice(), &bufferInfo, nullptr, &buffer);
	
	if(r != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create buffer!");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(logicalDevice->getLogicalDevice(), buffer, &memRequirements);

	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(logicalDevice->getPhysicalDevice(), &memProperties);

	uint32_t typeFilter = memRequirements.memoryTypeBits;
	bool memoryCompatible = false;
	uint32_t memoryTypeIndex = 0;

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			memoryCompatible = true;
			memoryTypeIndex = i;
			break;
		}
	}

	if(memoryCompatible == false)
	{
		throw std::runtime_error("failed to find suitable memory!");
	}

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = memoryTypeIndex;

	r = vkAllocateMemory(logicalDevice->getLogicalDevice(), &allocInfo, nullptr, &bufferMemory);

	if(r != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate buffer memory!");
	}

	vkBindBufferMemory(logicalDevice->getLogicalDevice(), buffer, bufferMemory, 0);

}

void Computer::copyBuffer
(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, uint32_t dstOffset)
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = logicalDevice->getCommandPool();
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(logicalDevice->getLogicalDevice(), &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = 0; // Optional
	copyRegion.dstOffset = dstOffset; // Optional
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	
	vkQueueSubmit(logicalDevice->getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(logicalDevice->getGraphicsQueue());

	vkFreeCommandBuffers(logicalDevice->getLogicalDevice(), 
					logicalDevice->getCommandPool(), 1, &commandBuffer);
}

void Computer::createCommandBuffer()
{
	//commandBuffers = (VkCommandBuffer*) malloc(MAX_FRAMES_IN_FLIGHT*sizeof(VkCommandBuffer));
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = logicalDevice->getCommandPool();
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	VkResult r;
	r = vkAllocateCommandBuffers(logicalDevice->getLogicalDevice(), &allocInfo, &commandBuffer);
	if (r != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate command buffers!");
	}
}


VkCommandBuffer Computer::beginCommand()
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = logicalDevice->getCommandPool();
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(logicalDevice->getLogicalDevice(), &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);
	return commandBuffer;
}

void Computer::endCommand(VkCommandBuffer commandBuffer)
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(logicalDevice->getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(logicalDevice->getGraphicsQueue());

	vkFreeCommandBuffers(logicalDevice->getLogicalDevice(), 
					logicalDevice->getCommandPool(), 1, &commandBuffer);
}

void Computer::createInOutBuffers()
{
	// In
	createBuffer(8*dataLength, 
					VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
					VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
					inputBuffers, inputMemory);

	vkMapMemory(logicalDevice->getLogicalDevice(), 
					inputMemory, 0, 
					8*dataLength, 0, &pInputMemory);

	// Out
	createBuffer(4*dataLength, 
					VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
					VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
					outputBuffer, outputMemory);

	vkMapMemory(logicalDevice->getLogicalDevice(), 
					outputMemory, 0, 
					4*dataLength, 0, &pOutputMemory);

	VkDescriptorSetLayoutBinding* bindings;
	bindings = (VkDescriptorSetLayoutBinding*) malloc(3*sizeof(VkDescriptorSetLayoutBinding));
	
	// Input 1
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[0].descriptorCount = 1;
	bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	bindings[0].pImmutableSamplers = nullptr;

	// Input 2
    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[1].descriptorCount = 1;
	bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	bindings[1].pImmutableSamplers = nullptr;
	
	// Output
    bindings[2].binding = 2;
    bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[2].descriptorCount = 1;
	bindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	bindings[2].pImmutableSamplers = nullptr;
	
	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 3;
	layoutInfo.pBindings = bindings;

	VkResult r;
	r = vkCreateDescriptorSetLayout(logicalDevice->getLogicalDevice(), 
					&layoutInfo, nullptr, &descriptorSetLayout);
	if(r != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor set layout!");
	}

	VkDescriptorPoolSize* poolSizes;
	poolSizes = (VkDescriptorPoolSize*) malloc(3*sizeof(VkDescriptorPoolSize));
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSizes[0].descriptorCount = 1;
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSizes[1].descriptorCount = 1;
	poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSizes[2].descriptorCount = 1;

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = 3;
	poolInfo.pPoolSizes = poolSizes;
	poolInfo.maxSets = 1;

	r = vkCreateDescriptorPool(logicalDevice->getLogicalDevice(), 
					&poolInfo, nullptr, &inOutDescriptorPool); 
	
	if(r != VK_SUCCESS) 
	{
		throw std::runtime_error("failed to create descriptor pool!");
	}

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = inOutDescriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &descriptorSetLayout;

	r=vkAllocateDescriptorSets(logicalDevice->getLogicalDevice(),&allocInfo,&descriptorSet);
	if(r != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate descriptor sets!");
	}

	VkWriteDescriptorSet* descriptorWrite;
	descriptorWrite = (VkWriteDescriptorSet*) malloc(3*sizeof(VkWriteDescriptorSet));
		
	// Input 1
	VkDescriptorBufferInfo bufferInfoInput1{};
	bufferInfoInput1.buffer = inputBuffers;
	bufferInfoInput1.offset = 0;
	bufferInfoInput1.range = dataLength*4;
	
	// Input 2
	VkDescriptorBufferInfo bufferInfoInput2{};
	bufferInfoInput2.buffer = inputBuffers;
	bufferInfoInput2.offset = dataLength*4;
	bufferInfoInput2.range = dataLength*4;
	
	// Output
	VkDescriptorBufferInfo bufferInfoOutput{};
	bufferInfoOutput.buffer = outputBuffer;
	bufferInfoOutput.offset = 0;
	bufferInfoOutput.range = dataLength*4;

	// UBO
	descriptorWrite[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite[0].dstSet = descriptorSet;
	descriptorWrite[0].dstBinding = 0;
	descriptorWrite[0].dstArrayElement = 0;
	descriptorWrite[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorWrite[0].descriptorCount = 1;
	descriptorWrite[0].pBufferInfo = &bufferInfoInput1;
	descriptorWrite[0].pImageInfo = nullptr;
	descriptorWrite[0].pTexelBufferView = nullptr;
	descriptorWrite[0].pNext = nullptr;

	descriptorWrite[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite[1].dstSet = descriptorSet;
	descriptorWrite[1].dstBinding = 1;
	descriptorWrite[1].dstArrayElement = 0;
	descriptorWrite[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorWrite[1].descriptorCount = 1;
	descriptorWrite[1].pBufferInfo = &bufferInfoInput2;
	descriptorWrite[1].pImageInfo = nullptr;
	descriptorWrite[1].pTexelBufferView = nullptr;
	descriptorWrite[1].pNext = nullptr;

	descriptorWrite[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite[2].dstSet = descriptorSet;
	descriptorWrite[2].dstBinding = 2;
	descriptorWrite[2].dstArrayElement = 0;
	descriptorWrite[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorWrite[2].descriptorCount = 1;
	descriptorWrite[2].pBufferInfo = &bufferInfoOutput;
	descriptorWrite[2].pImageInfo = nullptr;
	descriptorWrite[2].pTexelBufferView = nullptr;
	descriptorWrite[2].pNext = nullptr;

	vkUpdateDescriptorSets(logicalDevice->getLogicalDevice(), 3, descriptorWrite, 0, nullptr);
	free(bindings);
	free(poolSizes);
	free(descriptorWrite);
};

