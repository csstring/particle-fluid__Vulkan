#pragma once

#include "vk_types.hpp"
#include <vector>
#include "glm/vec3.hpp"
#include "glm/vec2.hpp"

struct Vertex {

  glm::vec3 position;
  glm::vec3 normal;
  glm::vec3 color;
  glm::vec2 uv;
  
  static VertexInputDescription get_vertex_description();
};

struct Mesh {
	std::vector<Vertex> _vertices;
	AllocatedBuffer _vertexBuffer;

  bool load_from_obj(const char* filename);
};