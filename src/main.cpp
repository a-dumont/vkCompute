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

	// Vulkan init
	t1 = std::chrono::steady_clock::now();
	vkTools::VulkanBase vkBase = vkTools::VulkanBase(appVersion,1,requiredLayers);
	vkTools::LogicalDevice logicalDevice = vkTools::LogicalDevice(&vkBase,pDevIdx,1|2|4);
	Computer computer = Computer(&vkBase,&logicalDevice);
	
	// Print config time
	t2 = std::chrono::steady_clock::now();	
	std::cout<<"Vulkan config time: ";
	std::cout<<std::chrono::duration_cast<std::chrono::milliseconds>(t2-t1).count();
	std::cout<<"ms"<<std::endl;

	// Print device
	std::cout<<logicalDevice.getPhysicalDeviceInfo()->getProperties().deviceName<<std::endl;
	
	// Load data to gpu
	uint32_t dataIn1[256];
	uint32_t dataIn2[256];
	uint32_t dataOut[256];

	for(int i=0;i<256;i++)
	{
		dataIn1[i] = i;
		dataIn2[i] = 2*i;
	}

	computer.loadData(dataIn1,dataIn2);

	// Run
	computer.compute();
	computer.readData(dataOut);
	
	for(int i=0;i<256;i++)
	{
		std::cout<<i<<": "<<dataOut[i]<<"\n";
	}
	std::cout<<std::endl;

	return 0;
}
