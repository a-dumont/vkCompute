cmake_minimum_required(VERSION 3.15)

if(NOT CMAKE_BUILD_TYPE)
		set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose Release or Debug" FORCE)
endif()

# Project name
project(vkCompute VERSION 0.0.1)

# C++ options
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
option(BUILD_SHARED_LIBS "Build using shared libraries" ON)
add_compile_options(-Wall -O3 -march=native -DDEBUG -g -fopenmp -fopenmp-simd)


# primitives
#file(GLOB SRC_FILES_renderer "src/primitives/*.cpp")
#add_library(primitives ${SRC_FILES_renderer})
#set_target_properties( primitives
#    PROPERTIES
#    ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib/primitives"
#    LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib/primitives"
#)

# vulkanTools
file(GLOB SRC_FILES_vulkanTools "src/vulkanTools/*.cpp")
add_library(vulkanTools ${SRC_FILES_vulkanTools})
set_target_properties( vulkanTools
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib/vulkanTools"
    LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib/vulkanTools"
)

# computer
file(GLOB SRC_FILES_computer "src/vkComputer/*.cpp")
add_library(vkComputer ${SRC_FILES_computer})
target_link_libraries(vkComputer -fopenmp -fopenmp-simd)
set_target_properties( vkComputer
    PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib/vkComputer"
	LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib/vkComputer"
)

# Main executable
file(GLOB SRC_FILE "src/main.cpp")
add_executable(vkCompute ${SRC_FILE})
set_target_properties( vkCompute
    PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin"
)

target_link_libraries(vkCompute -lvulkan -ldl -lpthread -lm -lnoise)
#target_link_libraries(vkCraft ${PROJECT_SOURCE_DIR}/lib/primitives/libprimitives.so)
target_link_libraries(vkCompute ${PROJECT_SOURCE_DIR}/lib/vulkanTools/libvulkanTools.so)
target_link_libraries(vkCompute ${PROJECT_SOURCE_DIR}/lib/vkComputer/libvkComputer.so)
