#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <span>
#include <array>
#include <functional>
#include <deque>
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vma/vk_mem_alloc.h>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <iostream>

#define VK_CHECK(result) \
    if (result != VK_SUCCESS) { \
        std::cerr << "Vulkan error: " << result << std::endl; \
        exit(-1); \
    }

struct AllocatedBuffer {
    VkBuffer _buffer;
    VmaAllocation _allocation;
};

struct AllocatedImage {
    VkImage _image;
    VmaAllocation _allocation;
		VkImageView _imageView;
		VkExtent3D _imageExtent;
  	VkFormat _imageFormat;
};

struct MeshPushConstants {
	glm::vec4 data;
	glm::mat4 render_matrix;
};

struct DeletionQueue
{
  std::deque<std::function<void()>> deletors;

  void push_function(std::function<void()>&& functions){
    deletors.push_back(functions);
  }

  void flush(){
    for (auto it = deletors.rbegin(); it != deletors.rend(); ++it){
      (*it)();
    }
    deletors.clear();
  }
};

struct VertexInputDescription {

	std::vector<VkVertexInputBindingDescription> bindings;
	std::vector<VkVertexInputAttributeDescription> attributes;

	VkPipelineVertexInputStateCreateFlags flags = 0;
};


struct Material {
	uint32_t constantSize{0};
	void* constant{VK_NULL_HANDLE};
	uint32_t setLayoutCount{0};
	VkDescriptorSet textureSet{VK_NULL_HANDLE};
	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;
};

struct Mesh;
struct RenderObject {
	Mesh* mesh;
	Material* material;
	glm::mat4 transformMatrix;
};

struct GPUCameraData{
	glm::mat4 view;
	glm::mat4 proj;
	glm::mat4 viewproj;
};

struct ComputeContext {
	VkSemaphore _computeSemaphore;
	VkFence _computeFence;
	VkCommandPool _commandPool;
	VkCommandBuffer _commandBuffer;
};

struct FrameData {
	VkSemaphore _presentSemaphore, _renderSemaphore;
	VkFence _renderFence;	

	VkCommandPool _commandPool;
	VkCommandBuffer _mainCommandBuffer;

	VkDescriptorSet globalDescriptor;

	AllocatedBuffer objectBuffer;
	VkDescriptorSet objectDescriptor;
};

struct GPUSceneData {
	glm::vec4 fogColor; // w is for exponent
	glm::vec4 fogDistances; //x for min, y for max, zw unused.
	glm::vec4 ambientColor;
	glm::vec4 sunlightDirection; //w for sun power
	glm::vec4 sunlightColor;
};

struct GPUObjectData{
	glm::mat4 modelMatrix;
	glm::vec4 modelColor;
};

struct UploadContext {
	VkFence _uploadFence;
	VkCommandPool _commandPool;
	VkCommandBuffer _commandBuffer;
};


struct Texture {
	AllocatedImage image;
	VkImageView imageView;
	VkExtent3D imageExtent;
  VkFormat imageFormat;
};

struct FluidPushConstants {
	glm::vec4 velocity;
	glm::vec4 color;
	glm::vec4 cursorPos;
	float viscosity;
	float dt;
};

struct alignas(16) CloudPushConstants {
	glm::vec4 cursorPos;
	glm::vec4 camPos;
	glm::vec4 uvwOffset;
  glm::vec4 lightDir;
  glm::vec4 lightColor;
  float lightAbsorptionCoeff;
  float densityAbsorption;
  float aniso;
	float dt;
};

class VulkanEngine;
class Scene
{
  public:
    virtual void initialize(VulkanEngine* engine) = 0;
    virtual void update(float dt, uint32_t frameidx) = 0;
    virtual void draw(VkCommandBuffer cmd) = 0;
    virtual void guiRender() = 0;
		virtual void init_sync_structures() = 0;
		virtual void init_descriptors() = 0;
		virtual void init_pipelines() = 0;
    virtual ~Scene(){};
};
