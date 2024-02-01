#pragma once
#include "vk_types.hpp"
#include "glm/vec4.hpp"

struct Particle {

  glm::vec4 position;
  glm::vec4 velocity;
  glm::vec4 color;
  
  static VertexInputDescription get_particle_description();
};


VertexInputDescription Particle::get_particle_description()
{
	VertexInputDescription description;

	//we will have just 1 vertex buffer binding, with a per-vertex rate
	VkVertexInputBindingDescription mainBinding = {};
	mainBinding.binding = 0;
	mainBinding.stride = sizeof(Particle);
	mainBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	description.bindings.push_back(mainBinding);

	//Position will be stored at Location 0
	VkVertexInputAttributeDescription positionAttribute = {};
	positionAttribute.binding = 0;
	positionAttribute.location = 0;
	positionAttribute.format = VK_FORMAT_R32G32B32A32_SFLOAT;
	positionAttribute.offset = offsetof(Particle, position);

	//Color will be stored at Location 2
	VkVertexInputAttributeDescription colorAttribute = {};
	colorAttribute.binding = 0;
	colorAttribute.location = 1;
	colorAttribute.format = VK_FORMAT_R32G32B32A32_SFLOAT;
	colorAttribute.offset = offsetof(Particle, color);

	description.attributes.push_back(positionAttribute);
	description.attributes.push_back(colorAttribute);
	return description;
}