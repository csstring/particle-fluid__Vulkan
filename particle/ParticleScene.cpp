#include "ParticleScene.hpp"
#include "vk_initializers.hpp"
#include "Particle.hpp"
#include <random>
#include "vk_pipelines.hpp"
#include "Camera.hpp"

void ParticleScene::initialize(VulkanEngine* engine)
{
  _engine = engine;
	PARTICLE_COUNT = 3000000;
}

void ParticleScene::init_commands()
{
	VkCommandPoolCreateInfo computePoolInfo = vkinit::command_pool_create_info(_engine->_graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	for (int i = 0; i < FRAME_OVERLAP; i++) {
		VK_CHECK(vkCreateCommandPool(_engine->_device, &computePoolInfo, nullptr, &_computeContext[i]._commandPool));
		VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(_computeContext[i]._commandPool, 1);
		VK_CHECK(vkAllocateCommandBuffers(_engine->_device, &cmdAllocInfo, &_computeContext[i]._commandBuffer));
		_deletionQueue.push_function([=]() {
			vkDestroyCommandPool(_engine->_device, _computeContext[i]._commandPool, nullptr);
		});
	}
}

void ParticleScene::update(float dt, uint32_t frameidx)
{
	_curFrameIdx = frameidx;
	VK_CHECK(vkWaitForFences(_engine->_device, 1, &_computeContext[_curFrameIdx]._computeFence, true, 1000000000));
	VK_CHECK(vkResetFences(_engine->_device, 1, &_computeContext[_curFrameIdx]._computeFence));
	VK_CHECK(vkResetCommandBuffer(_computeContext[_curFrameIdx]._commandBuffer, 0));
	particleUpdate(_computeContext[_curFrameIdx]._commandBuffer);
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &_computeContext[_curFrameIdx]._commandBuffer;
  // submitInfo.signalSemaphoreCount = 1;
  // submitInfo.pSignalSemaphores = &_computeContext[_curFrameIdx]._computeSemaphore;
  VK_CHECK(vkQueueSubmit(_engine->_graphicsQueue, 1, &submitInfo, _computeContext[_curFrameIdx]._computeFence));
}

void ParticleScene::particleUpdate(VkCommandBuffer cmd)
{
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	Material* computMaterial = _engine->get_material("particleCompPipe");
	glm::vec4 cursorPos = Camera::getInstance().getWorldCursorPos(_engine->_windowExtent.width, _engine->_windowExtent.height);
	
	void* cursorData;
	vmaMapMemory(_engine->_allocator, _cursorBuffer[_curFrameIdx]._allocation , &cursorData);
	memcpy(cursorData, &cursorPos, sizeof(glm::vec4));
	vmaFlushAllocation(_engine->_allocator, _cursorBuffer[_curFrameIdx]._allocation, 0, sizeof(glm::vec4));
	vmaUnmapMemory(_engine->_allocator, _cursorBuffer[_curFrameIdx]._allocation);

	VK_CHECK(vkBeginCommandBuffer(cmd, &beginInfo));
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, computMaterial->pipeline);
  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, computMaterial->pipelineLayout, 0, 1, &particleDescriptor[_curFrameIdx], 0, nullptr);
  vkCmdDispatch(cmd, PARTICLE_COUNT / 64, 1, 1);
	VK_CHECK(vkEndCommandBuffer(cmd));
}

void ParticleScene::draw(VkCommandBuffer cmd)
{
	VkDeviceSize offsets[] = {0};
	vkCmdBindVertexBuffers(cmd, 0, 1, &_particleBuffer[_curFrameIdx]._buffer, offsets);
	vkCmdDraw(cmd, PARTICLE_COUNT, 1, 0, 0);
}

void ParticleScene::guiRender(){}
void ParticleScene::init_sync_structures()
{
  VkFenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
	VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info();
  for (int i = 0; i < FRAME_OVERLAP; i++) {     
    VK_CHECK(vkCreateFence(_engine->_device, &fenceCreateInfo, nullptr, &_computeContext[i]._computeFence));
    VK_CHECK(vkCreateSemaphore(_engine->_device, &semaphoreCreateInfo, nullptr, &_computeContext[i]._computeSemaphore));
    _deletionQueue.push_function([=]() {
      vkDestroyFence(_engine->_device, _computeContext[i]._computeFence, nullptr);
      vkDestroySemaphore(_engine->_device, _computeContext[i]._computeSemaphore, nullptr);
    });
	}
}

void ParticleScene::init_pipelines()
{
	PipelineBuilder pipelineBuilder;
  pipelineBuilder._depthStencil = vkinit::depth_stencil_create_info(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);
  pipelineBuilder._vertexInputInfo = vkinit::vertex_input_state_create_info();
	//input assembly is the configuration for drawing triangle lists, strips, or individual points.
	//we are just going to draw triangle list
	pipelineBuilder._inputAssembly = vkinit::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_POINT_LIST);
	//build viewport and scissor from the swapchain extents
	pipelineBuilder._viewport.x = 0.0f;
	pipelineBuilder._viewport.y = 0.0f;
	pipelineBuilder._viewport.width = (float)_engine->_windowExtent.width;
	pipelineBuilder._viewport.height = (float)_engine->_windowExtent.height;
	pipelineBuilder._viewport.minDepth = 0.0f;
	pipelineBuilder._viewport.maxDepth = 1.0f;

	pipelineBuilder._scissor.offset = { 0, 0 };
	pipelineBuilder._scissor.extent = _engine->_windowExtent;

	pipelineBuilder._rasterizer = vkinit::rasterization_state_create_info(VK_POLYGON_MODE_FILL);
	pipelineBuilder._multisampling = vkinit::multisampling_state_create_info();
	//a single blend attachment with no blending and writing to RGBA
	pipelineBuilder._colorBlendAttachment = vkinit::color_blend_attachment_state();

  VkPipelineLayoutCreateInfo textured_pipeline_layout_info = vkinit::pipeline_layout_create_info();

	VkDescriptorSetLayout texturedSetLayouts[] = { _engine->_globalSetLayout };

	textured_pipeline_layout_info.setLayoutCount = 1;
	textured_pipeline_layout_info.pSetLayouts = texturedSetLayouts;

	VkPipelineLayout texturedPipeLayout;
	VK_CHECK(vkCreatePipelineLayout(_engine->_device, &textured_pipeline_layout_info, nullptr, &texturedPipeLayout));
	
	VertexInputDescription vertexDescription = Particle::get_particle_description();
	pipelineBuilder._vertexInputInfo.pVertexAttributeDescriptions = vertexDescription.attributes.data();
	pipelineBuilder._vertexInputInfo.vertexAttributeDescriptionCount = vertexDescription.attributes.size();

	pipelineBuilder._vertexInputInfo.pVertexBindingDescriptions = vertexDescription.bindings.data();
	pipelineBuilder._vertexInputInfo.vertexBindingDescriptionCount = vertexDescription.bindings.size();

  VkShaderModule colorMeshShader;
  VkShaderModule meshVertShader;

	if (!vkutil::load_shader_module("./spv/particle.vert.spv", _engine->_device, &meshVertShader)){
		std::cout << "Error when building the triangle vertex shader module" << std::endl;
	}
	else {
		std::cout << "Red Triangle vertex shader successfully loaded" << std::endl;
	}

	if (!vkutil::load_shader_module("./spv/particle.frag.spv", _engine->_device, &colorMeshShader))
	{
		std::cout << "Error when building the default_lit fragment shader module" << std::endl;
	}
	else {
		std::cout << "Red Triangle default_lit shader successfully loaded" << std::endl;
	}

	pipelineBuilder._shaderStages.push_back(
		vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, meshVertShader));
	pipelineBuilder._shaderStages.push_back(
		vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, colorMeshShader));
	pipelineBuilder._pipelineLayout = texturedPipeLayout;
	VkPipeline texPipeline = pipelineBuilder.build_pipeline(_engine->_device, _engine->_renderPass);
	_engine->create_material(texPipeline, texturedPipeLayout, "particleRenderPipe",textured_pipeline_layout_info.setLayoutCount);

	vkDestroyShaderModule(_engine->_device, colorMeshShader, nullptr);
  vkDestroyShaderModule(_engine->_device, meshVertShader, nullptr);

 	_deletionQueue.push_function([=]() {
		vkDestroyPipeline(_engine->_device, texPipeline, nullptr);
		vkDestroyPipelineLayout(_engine->_device, texturedPipeLayout, nullptr);
  });
}

void ParticleScene::init_compute_pipelines()
{
	VkShaderModule compShader;
	if (!vkutil::load_shader_module("./spv/particle.comp.spv", _engine->_device, &compShader)){
		std::cout << "Error when building the triangle vertex shader module" << std::endl;
	}
	VkPipeline particlePipeline;
	VkPipelineLayout particlePipeLayout;

	VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
	computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	computeShaderStageInfo.module = compShader;
	computeShaderStageInfo.pName = "main";

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &_particleSetLayout;

	if (vkCreatePipelineLayout(_engine->_device, &pipelineLayoutInfo, nullptr, &particlePipeLayout) != VK_SUCCESS) {
	    throw std::runtime_error("failed to create compute pipeline layout!");
	}

	VkComputePipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.layout = particlePipeLayout;
	pipelineInfo.stage = computeShaderStageInfo;

	if (vkCreateComputePipelines(_engine->_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &particlePipeline) != VK_SUCCESS) {
	    throw std::runtime_error("failed to create compute pipeline!");
	}
	_engine->create_material(particlePipeline, particlePipeLayout, "particleCompPipe",pipelineLayoutInfo.setLayoutCount);
	vkDestroyShaderModule(_engine->_device, compShader, nullptr);
	_deletionQueue.push_function([=]() {
    vkDestroyPipeline(_engine->_device, particlePipeline, nullptr);
		vkDestroyPipelineLayout(_engine->_device, particlePipeLayout, nullptr);
  });
}

void ParticleScene::load_particle()
{
	std::vector<Particle> ps;
	ps.resize(PARTICLE_COUNT);
	std::default_random_engine rndEngine((unsigned)time(nullptr));
  std::uniform_real_distribution<float> rndDist(-1.0f, 1.0f);

	for (auto& vertex : ps) {
	    float theta = rndDist(rndEngine) * 20.0f;
			float theta1 = rndDist(rndEngine) * 20.0f;
	    float x = theta;
	    float y = theta1;
	    vertex.position = glm::vec4(x, y, 0.0f, 0.0f);
	    vertex.velocity = glm::normalize(vertex.position) * 0.00025f;
	    vertex.color = glm::vec4(rndDist(rndEngine), rndDist(rndEngine), rndDist(rndEngine), 1.0f);
	}

	const size_t bufferSize = ps.size() * sizeof(Particle);
	//allocate vertex buffer
	VkBufferCreateInfo stagingBufferInfo = {};
	stagingBufferInfo.pNext = nullptr;
	stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	stagingBufferInfo.size = bufferSize;
	stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

	//let the VMA library know that this data should be writeable by CPU, but also readable by GPU
	VmaAllocationCreateInfo vmaallocInfo = {};
	vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

	AllocatedBuffer stagingBuffer;
	//allocate the buffer
	VK_CHECK(vmaCreateBuffer(_engine->_allocator, &stagingBufferInfo, &vmaallocInfo,
		&stagingBuffer._buffer,
		&stagingBuffer._allocation,
		nullptr));

  void* data;
  vmaMapMemory(_engine->_allocator, stagingBuffer._allocation, &data);
  memcpy(data, ps.data(), bufferSize);
  vmaFlushAllocation(_engine->_allocator, stagingBuffer._allocation, 0, VK_WHOLE_SIZE);
  vmaUnmapMemory(_engine->_allocator, stagingBuffer._allocation);

	VkBufferCreateInfo vertexBufferInfo = {};
	vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	vertexBufferInfo.pNext = nullptr;
	vertexBufferInfo.size = bufferSize;
	vertexBufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	vmaallocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	vmaallocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	//allocate the buffer
	for (int i =0; i < FRAME_OVERLAP; ++i){
		VK_CHECK(vmaCreateBuffer(_engine->_allocator, &vertexBufferInfo, &vmaallocInfo,
			&_particleBuffer[i]._buffer,
			&_particleBuffer[i]._allocation,
			nullptr));

		_engine->immediate_submit([=](VkCommandBuffer cmd){
			VkBufferCopy copy;
			copy.dstOffset = 0;
			copy.srcOffset = 0;
			copy.size = bufferSize;
			vkCmdCopyBuffer(cmd, stagingBuffer._buffer, _particleBuffer[i]._buffer, 1, &copy);
		});

		_deletionQueue.push_function([=]() {
		vmaDestroyBuffer(_engine->_allocator, _particleBuffer[i]._buffer, _particleBuffer[i]._allocation);
		});
	}
	vmaDestroyBuffer(_engine->_allocator, stagingBuffer._buffer, stagingBuffer._allocation);
}

void ParticleScene::init_descriptors()
{
	_cursorBuffer[0] = _engine->create_buffer(sizeof(glm::vec4), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	_cursorBuffer[1] = _engine->create_buffer(sizeof(glm::vec4), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	_deletionQueue.push_function([=]() {
		vmaDestroyBuffer(_engine->_allocator, _cursorBuffer[0]._buffer, _cursorBuffer[0]._allocation);
		vmaDestroyBuffer(_engine->_allocator, _cursorBuffer[1]._buffer, _cursorBuffer[1]._allocation);
	});
	for (int i =0; i < FRAME_OVERLAP; ++i){
		VkDescriptorBufferInfo cursorInfo;
		cursorInfo.buffer = _cursorBuffer[i]._buffer;
		cursorInfo.offset = 0;
		cursorInfo.range = sizeof(glm::vec4);

		VkDescriptorBufferInfo curparticleBufferInfo;
		curparticleBufferInfo.buffer = _particleBuffer[i]._buffer;
		curparticleBufferInfo.offset = 0;
		curparticleBufferInfo.range = sizeof(Particle) * PARTICLE_COUNT;

		VkDescriptorBufferInfo prevparticleBufferInfo;
		prevparticleBufferInfo.buffer = _particleBuffer[(i + 1) % FRAME_OVERLAP]._buffer;
		prevparticleBufferInfo.offset = 0;
		prevparticleBufferInfo.range = sizeof(Particle) * PARTICLE_COUNT;

		vkutil::DescriptorBuilder::begin(_engine->_descriptorLayoutCache, _engine->_descriptorAllocator).
											bind_buffer(0, &prevparticleBufferInfo, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT).
											bind_buffer(1, &curparticleBufferInfo, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT).
											bind_buffer(2, &cursorInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT).
											build(particleDescriptor[i], _particleSetLayout);

	}
}