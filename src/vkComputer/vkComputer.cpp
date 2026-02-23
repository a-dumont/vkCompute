#include "vkComputer.h"

Computer::Computer(vkTools::ComputePipeline* pipelineIn)
{
	pipeline = pipelineIn;
	logicalDevice = pipeline->getLogicalDevice();
	vkBase = logicalDevice->getVulkanBase();

	createCommandBuffer();
	createSyncObjects();
}

Computer::~Computer()
{
	destroySyncObjects();
	vkDestroyDescriptorPool(logicalDevice->getLogicalDevice(), inOutDescriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(logicalDevice->getLogicalDevice(), descriptorSetLayout, nullptr);
}

void Computer::createSyncObjects()
{
	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	VkResult r;
	r = vkCreateFence(logicalDevice->getLogicalDevice(), 
					&fenceInfo, nullptr, &computeFence);
	if(r != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create fence!");
	}
}

void Computer::destroySyncObjects()
{
    vkDestroyFence(logicalDevice->getLogicalDevice(), computeFence, nullptr);
}


//////////////////////////////////////////////////////////////////////////////////
//            ____                       _____                                  //
//           |  _ \ _ __ __ ___      __ |  ___| __ __ _ _ __ ___   ___          //
//           | | | | '__/ _` \ \ /\ / / | |_ | '__/ _` | '_ ` _ \ / _ \         //
//           | |_| | | | (_| |\ V  V /  |  _|| | | (_| | | | | | |  __/         //
//           |____/|_|  \__,_| \_/\_/   |_|  |_|  \__,_|_| |_| |_|\___|         //
//////////////////////////////////////////////////////////////////////////////////

void Computer::recordCommandBuffer(VkCommandBuffer buffer, uint32_t dataLength)
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
	vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->getPipeline());
	vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_COMPUTE, 
					pipeline->getLayout(), 0, 1, 
					&descriptorSet, 0, nullptr);

	vkCmdPushConstants(buffer, pipeline->getLayout(), 
					VK_SHADER_STAGE_COMPUTE_BIT, 0, 
					sizeof(uint32_t), &dataLength);
	vkCmdDispatch(buffer, (dataLength/256)+1, 1, 1);	

	r = vkEndCommandBuffer(buffer);
	if (r != VK_SUCCESS) 
	{
		throw std::runtime_error("failed to record command buffer!");
	}
}

void Computer::compute(uint32_t dataLength)
{
	vkWaitForFences(logicalDevice->getLogicalDevice(), 1, &computeFence, VK_TRUE, UINT64_MAX);
	vkResetFences(logicalDevice->getLogicalDevice(), 1, &computeFence);

	vkResetCommandBuffer(commandBuffer, 0);
	recordCommandBuffer(commandBuffer, dataLength);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	
	submitInfo.signalSemaphoreCount = 0;
	
	VkBool32 r;
	r = vkQueueSubmit(logicalDevice->getComputeQueue(), 1, &submitInfo, 
					computeFence);
	if(r != VK_SUCCESS)
	{
		throw std::runtime_error("failed to submit compute command buffer!");
	}

	vkWaitForFences(logicalDevice->getLogicalDevice(), 1, &computeFence, VK_TRUE, UINT64_MAX);
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

void Computer::fillBaseWriteDescriptorSet(uint32_t n, VkWriteDescriptorSet* writeDescriptorSet)
{
	// SSBO
	for(uint32_t i=0; i<n;i++)
	{
		writeDescriptorSet[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSet[i].dstSet = descriptorSet;
		writeDescriptorSet[i].dstBinding = i;
		writeDescriptorSet[i].dstArrayElement = 0;
		writeDescriptorSet[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		writeDescriptorSet[i].descriptorCount = 1;
		//writeDescriptorSet[i].pBufferInfo = &bufferInfoInput1;
		writeDescriptorSet[i].pImageInfo = nullptr;
		writeDescriptorSet[i].pTexelBufferView = nullptr;
		writeDescriptorSet[i].pNext = nullptr;
	}
}

void Computer::createDescriptorSetLayout(uint32_t N)
{
	
	VkDescriptorSetLayoutBinding* bindings;
	bindings = (VkDescriptorSetLayoutBinding*) malloc(N*sizeof(VkDescriptorSetLayoutBinding));
	
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

	if(descriptorSetLayoutInit == true)
	{
		vkDestroyDescriptorPool(logicalDevice->getLogicalDevice(), inOutDescriptorPool, nullptr);
		vkDestroyDescriptorSetLayout(logicalDevice->getLogicalDevice(),descriptorSetLayout, nullptr);
		descriptorSetLayoutInit = false;
	}

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = N;
	layoutInfo.pBindings = bindings;

	VkResult r;
	r = vkCreateDescriptorSetLayout(logicalDevice->getLogicalDevice(), 
					&layoutInfo, nullptr, &descriptorSetLayout);
	if(r != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor set layout!");
	}
	descriptorSetLayoutInit = true;

	VkDescriptorPoolSize poolSize;
	poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSize.descriptorCount = N;

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = 1;
	poolInfo.pPoolSizes = &poolSize;
	poolInfo.maxSets = 1;

	r = vkCreateDescriptorPool(logicalDevice->getLogicalDevice(), 
					&poolInfo, nullptr, &inOutDescriptorPool); 
	
	if(r != VK_SUCCESS) 
	{
		throw std::runtime_error("failed to create descriptor pool!");
	}
	free(bindings);
	pipeline->setLayoutDescriptors(1,&descriptorSetLayout);
	pipeline->setPushConstants(VK_SHADER_STAGE_COMPUTE_BIT,sizeof(uint32_t),0);
	pipeline->recreatePipeline();

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
}

