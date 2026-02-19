#pragma once
#include <glm/ext/vector_float3.hpp>
#include<glm/glm.hpp>
#include <vector>
#include <algorithm>
#include <iostream>

struct Vertex
{
	uint32_t data;
	//uint32_t texture;
};

struct ViewProjection
{
	glm::mat4 view;
	glm::mat4 projection;
};

struct ChunkPosition
{
	int32_t x;
	int32_t y;
	int32_t z;
	uint32_t pad;
};
