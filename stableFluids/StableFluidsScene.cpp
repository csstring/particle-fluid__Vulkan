#include "StableFluidsScene.hpp"
#include "vk_initializers.hpp"
#include <random>
#include "vk_pipelines.hpp"
#include "Camera.hpp"
#include "vk_textures.hpp"

uint32_t dispatchSize = 16;
uint32_t imageWidth = 384*2;
uint32_t imageHeight = 384*2;

void StableFluidsScene::initialize(VulkanEngine* engine)
{
  _engine = engine;
}

void StableFluidsScene::init_commands()
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

void StableFluidsScene::sourcing()
{
	Material* computMaterial = _engine->get_material("SOURCING");
	VkCommandBuffer cmd = _computeContext[_curFrameIdx]._commandBuffer;

  vkCmdPushConstants(cmd, computMaterial->pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(FluidPushConstants), &constants);
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, computMaterial->pipeline);
  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, computMaterial->pipelineLayout, 0, 1, &computMaterial->textureSet, 0, nullptr);
  vkCmdDispatch(cmd, imageWidth/dispatchSize, imageHeight/dispatchSize, 1);
}

void StableFluidsScene::vorticity()
{
	Material* computVorticityMaterial = _engine->get_material("COMPUTEVORTICITY");
	Material* confineVorticityMaterial = _engine->get_material("CONFINEVORTICITY");
	VkCommandBuffer cmd = _computeContext[_curFrameIdx]._commandBuffer;
  
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, computVorticityMaterial->pipeline);
  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, computVorticityMaterial->pipelineLayout, 0, 1, &computVorticityMaterial->textureSet, 0, nullptr);
  vkCmdDispatch(cmd, imageWidth/dispatchSize, imageHeight/dispatchSize, 1);

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, confineVorticityMaterial->pipeline);
  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, confineVorticityMaterial->pipelineLayout, 0, 1, &confineVorticityMaterial->textureSet, 0, nullptr);
  vkCmdDispatch(cmd, imageWidth/dispatchSize, imageHeight/dispatchSize, 1);
}

void StableFluidsScene::diffuse()
{	
	Material* firstMaterial = _engine->get_material("DIFFUSE_first");
	Material* secondMaterial = _engine->get_material("DIFFUSE_second");
	VkCommandBuffer cmd = _computeContext[_curFrameIdx]._commandBuffer;

	vkutil::copyImageNotImmediateSubmit(cmd, _fluidImageBuffer[0][FULIDTEXTUREID::VELOCITYID],  _fluidImageBuffer[1][FULIDTEXTUREID::VELOCITYID]);
	vkutil::copyImageNotImmediateSubmit(cmd, _fluidImageBuffer[0][FULIDTEXTUREID::DENSITYID],  _fluidImageBuffer[1][FULIDTEXTUREID::DENSITYID]);

	vkCmdPushConstants(cmd, firstMaterial->pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(FluidPushConstants), &constants);
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, firstMaterial->pipeline);
	for (int i =0; i < 10; ++i){
		if (i%2 ==0){
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, firstMaterial->pipelineLayout, 0, 1, &firstMaterial->textureSet, 0, nullptr);
		} else {
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, secondMaterial->pipelineLayout, 0, 1, &secondMaterial->textureSet, 0, nullptr);
		}
		vkCmdDispatch(cmd, imageWidth/dispatchSize, imageHeight/dispatchSize, 1);
	}
}

void StableFluidsScene::projection()
{
	Material* divergenceMaterial = _engine->get_material("DIVERGENCE");
	Material* firstJacobiMaterial = _engine->get_material("JACOBI_first");
	Material* secondJacobiMaterial = _engine->get_material("JACOBI_second");
	Material* applyPressureMaterial = _engine->get_material("APPLYPRESSURE");

	VkCommandBuffer cmd = _computeContext[_curFrameIdx]._commandBuffer;

	//divergence
	{
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, divergenceMaterial->pipeline);
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, divergenceMaterial->pipelineLayout, 0, 1, &divergenceMaterial->textureSet, 0, nullptr);
		vkCmdDispatch(cmd, imageWidth/dispatchSize, imageHeight/dispatchSize, 1);
	}

	//jacobi
	{
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, firstJacobiMaterial->pipeline);
		for (int i =0; i < 100; ++i){
			if (i%2 ==0){
				vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, firstJacobiMaterial->pipelineLayout, 0, 1, &firstJacobiMaterial->textureSet, 0, nullptr);
			} else {
				vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, secondJacobiMaterial->pipelineLayout, 0, 1, &secondJacobiMaterial->textureSet, 0, nullptr);
			}
			vkCmdDispatch(cmd, imageWidth/dispatchSize, imageHeight/dispatchSize, 1);
		}
	}

	//applyPressure
	{
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, applyPressureMaterial->pipeline);
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, applyPressureMaterial->pipelineLayout, 0, 1, &applyPressureMaterial->textureSet, 0, nullptr);
		vkCmdDispatch(cmd, imageWidth/dispatchSize, imageHeight/dispatchSize, 1);
	}
}

void StableFluidsScene::advection()
{
	Material* advectMaterial = _engine->get_material("ADVECT");

	VkCommandBuffer cmd = _computeContext[_curFrameIdx]._commandBuffer;

	vkutil::copyImageImmediateSubmitTest(cmd, _fluidImageBuffer[0][FULIDTEXTUREID::VELOCITYID],  _fluidImageBuffer[1][FULIDTEXTUREID::VELOCITYID]);
	vkutil::copyImageImmediateSubmitTest(cmd, _fluidImageBuffer[0][FULIDTEXTUREID::DENSITYID],  _fluidImageBuffer[1][FULIDTEXTUREID::DENSITYID]);

  vkCmdPushConstants(cmd, advectMaterial->pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(FluidPushConstants), &constants);
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, advectMaterial->pipeline);
  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, advectMaterial->pipelineLayout, 0, 1, &advectMaterial->textureSet, 0, nullptr);
  vkCmdDispatch(cmd, imageWidth/dispatchSize, imageHeight/dispatchSize, 1);
	
	vkutil::transitionImageLayout(cmd, _fluidImageBuffer[1][FULIDTEXTUREID::VELOCITYID]._image,
	_fluidImageBuffer[1][FULIDTEXTUREID::VELOCITYID]._imageFormat, 
	VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL
	);
	vkutil::transitionImageLayout(cmd, _fluidImageBuffer[1][FULIDTEXTUREID::DENSITYID]._image,
	_fluidImageBuffer[1][FULIDTEXTUREID::DENSITYID]._imageFormat, 
	VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL
	);

}

void StableFluidsScene::update(float dt, uint32_t frameidx)
{
	_curFrameIdx = frameidx;
	Camera cam = Camera::getInstance();
	static int color = 0;
	static float beforeX = cam._lastX * imageWidth / _engine->_windowExtent.width;
  static float beforeY = cam._lastY * imageHeight / _engine->_windowExtent.height;

	float curX = cam._lastX * imageWidth / _engine->_windowExtent.width;
	float curY = cam._lastY * imageHeight / _engine->_windowExtent.height;

  glm::vec4 vel(curX - beforeX, curY - beforeY, 0.0f, 0.0f);
	if (glm::length(vel) > 1.0f)
		vel = vel / glm::length(vel);

	beforeX = curX;
	beforeY = curY;

	constants.velocity = vel;
	constants.color = rainbow[(color++) % 7] * 2.0f;
	constants.cursorPos = glm::vec4(curX, curY, 0.f,0.f);
	constants.dt = 1.0f/120.0f;
	constants.viscosity = 10.0f;
	if (cam._clickOn == true){
		constants.cursorPos.w = 1.0f;
	}

	VK_CHECK(vkWaitForFences(_engine->_device, 1, &_computeContext[_curFrameIdx]._computeFence, true, 1000000000));
	VK_CHECK(vkResetFences(_engine->_device, 1, &_computeContext[_curFrameIdx]._computeFence));
	VK_CHECK(vkResetCommandBuffer(_computeContext[_curFrameIdx]._commandBuffer, 0));
	VkCommandBuffer cmd = _computeContext[_curFrameIdx]._commandBuffer;
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	VK_CHECK(vkBeginCommandBuffer(cmd, &beginInfo));

	sourcing();
  vorticity();
  diffuse();
  projection();
  advection();

	vkutil::transitionImageLayout(cmd, _fluidImageBuffer[0][FULIDTEXTUREID::DENSITYID]._image,VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	vkutil::transitionImageLayout(cmd, _fluidDrawImageBuffer._image,VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	vkutil::copy_image_to_image(cmd, 
			_fluidImageBuffer[0][FULIDTEXTUREID::DENSITYID]._image,
			_fluidDrawImageBuffer._image,
			_fluidImageBuffer[0][FULIDTEXTUREID::DENSITYID]._imageExtent,
			_fluidDrawImageBuffer._imageExtent);
	vkutil::transitionImageLayout(cmd, _fluidImageBuffer[0][FULIDTEXTUREID::DENSITYID]._image,VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
	vkutil::transitionImageLayout(cmd, _fluidDrawImageBuffer._image,VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
	
	VK_CHECK(vkEndCommandBuffer(cmd));

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &_computeContext[_curFrameIdx]._commandBuffer;
	VK_CHECK(vkQueueSubmit(_engine->_graphicsQueue, 1, &submitInfo, _computeContext[_curFrameIdx]._computeFence));
}

void StableFluidsScene::draw(VkCommandBuffer cmd)
{
	VkDeviceSize offsets[] = {0};
	Material* lastMaterial = _engine->get_material("fluidRenderPipe");
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, lastMaterial->pipelineLayout, 1, lastMaterial->setLayoutCount, &lastMaterial->textureSet, 0, nullptr);
	vkCmdBindVertexBuffers(cmd, 0, 1, &_fluidBuffer[_curFrameIdx]._buffer, offsets);
	vkCmdDraw(cmd, 6, 1, 0, 0);
}

void StableFluidsScene::guiRender(){}
void StableFluidsScene::init_sync_structures()
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

void StableFluidsScene::initRenderPipelines()
{
	VkDescriptorSetLayout fluidDrawSetLayout;

	//init descriptor
	{
	VkDescriptorImageInfo DENSITYDRAWID{};
	DENSITYDRAWID.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	DENSITYDRAWID.imageView = _fluidDrawImageBuffer._imageView;
	DENSITYDRAWID.sampler = _defaultSamplerLinear;
	vkutil::DescriptorBuilder::begin(_engine->_descriptorLayoutCache, _engine->_descriptorAllocator).
											bind_image(0, &DENSITYDRAWID, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,  VK_SHADER_STAGE_FRAGMENT_BIT).
											build(fluidDrawDescriptor, fluidDrawSetLayout);
	}

	//init pipeline
	{
	PipelineBuilder pipelineBuilder;
  pipelineBuilder._depthStencil = vkinit::depth_stencil_create_info(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);
  pipelineBuilder._vertexInputInfo = vkinit::vertex_input_state_create_info();
	//input assembly is the configuration for drawing triangle lists, strips, or individual points.
	//we are just going to draw triangle list
	pipelineBuilder._inputAssembly = vkinit::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
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

	VkDescriptorSetLayout texturedSetLayouts[] = {_engine->_globalSetLayout, fluidDrawSetLayout };

	textured_pipeline_layout_info.setLayoutCount = 2;
	textured_pipeline_layout_info.pSetLayouts = texturedSetLayouts;

	VkPipelineLayout texturedPipeLayout;
	VK_CHECK(vkCreatePipelineLayout(_engine->_device, &textured_pipeline_layout_info, nullptr, &texturedPipeLayout));
	
	VertexInputDescription vertexDescription = Vertex::get_vertex_description();
	pipelineBuilder._vertexInputInfo.pVertexAttributeDescriptions = vertexDescription.attributes.data();
	pipelineBuilder._vertexInputInfo.vertexAttributeDescriptionCount = vertexDescription.attributes.size();

	pipelineBuilder._vertexInputInfo.pVertexBindingDescriptions = vertexDescription.bindings.data();
	pipelineBuilder._vertexInputInfo.vertexBindingDescriptionCount = vertexDescription.bindings.size();

  VkShaderModule colorMeshShader;
  VkShaderModule meshVertShader;

	if (!vkutil::load_shader_module("./spv/fluid.vert.spv", _engine->_device, &meshVertShader)){
		std::cout << "Error when building the triangle vertex shader module" << std::endl;
	}
	else {
		std::cout << "Red Triangle vertex shader successfully loaded" << std::endl;
	}

	if (!vkutil::load_shader_module("./spv/fluid.frag.spv", _engine->_device, &colorMeshShader))
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

	_engine->create_material(texPipeline, texturedPipeLayout, "fluidRenderPipe", 1, fluidDrawDescriptor);

	vkDestroyShaderModule(_engine->_device, colorMeshShader, nullptr);
  vkDestroyShaderModule(_engine->_device, meshVertShader, nullptr);

 	_deletionQueue.push_function([=]() {
		vkDestroyPipeline(_engine->_device, texPipeline, nullptr);
		vkDestroyPipelineLayout(_engine->_device, texturedPipeLayout, nullptr);
  });
	}
}

void StableFluidsScene::initSourcing()
{
	VkDescriptorSetLayout sourcingLayout;
	VkDescriptorSet sourcingDescriptor;
	{
		VkDescriptorImageInfo VELOCITYID{};
		VELOCITYID.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		VELOCITYID.imageView = _fluidImageBuffer[0][FULIDTEXTUREID::VELOCITYID]._imageView;

		VkDescriptorImageInfo DENSITYID{};
		DENSITYID.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		DENSITYID.imageView = _fluidImageBuffer[0][FULIDTEXTUREID::DENSITYID]._imageView;

		vkutil::DescriptorBuilder::begin(_engine->_descriptorLayoutCache, _engine->_descriptorAllocator).
											bind_image(0, &VELOCITYID, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT).
											bind_image(1, &DENSITYID, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT).
											build(sourcingDescriptor, sourcingLayout);
	}
	{
	VkShaderModule compShader;
	if (!vkutil::load_shader_module("./spv/sourcing.comp.spv", _engine->_device, &compShader)){
		std::cout << "Error when building the sourcing comp shader module" << std::endl;
	}
	VkPipeline sourcingPipeline;
	VkPipelineLayout sourcingPipeLayout;

	VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
	computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	computeShaderStageInfo.module = compShader;
	computeShaderStageInfo.pName = "main";

	VkDescriptorSetLayout SetLayouts[] = { sourcingLayout};

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = SetLayouts;
	VkPushConstantRange push_constant;
	push_constant.offset = 0;
	push_constant.size = sizeof(FluidPushConstants);
	push_constant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	pipelineLayoutInfo.pPushConstantRanges = &push_constant;
	pipelineLayoutInfo.pushConstantRangeCount = 1;

	if (vkCreatePipelineLayout(_engine->_device, &pipelineLayoutInfo, nullptr, &sourcingPipeLayout) != VK_SUCCESS) {
	    throw std::runtime_error("failed to create compute pipeline layout!");
	}

	VkComputePipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.layout = sourcingPipeLayout;
	pipelineInfo.stage = computeShaderStageInfo;

	if (vkCreateComputePipelines(_engine->_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &sourcingPipeline) != VK_SUCCESS) {
	    throw std::runtime_error("failed to create compute pipeline!");
	}
	_engine->create_material(sourcingPipeline, sourcingPipeLayout, "SOURCING", pipelineLayoutInfo.setLayoutCount, sourcingDescriptor);
	vkDestroyShaderModule(_engine->_device, compShader, nullptr);
	_deletionQueue.push_function([=]() {
    vkDestroyPipeline(_engine->_device, sourcingPipeline, nullptr);
		vkDestroyPipelineLayout(_engine->_device, sourcingPipeLayout, nullptr);
  });
	}
}

void StableFluidsScene::initComputeVorticity()
{
	VkDescriptorSetLayout computeVorticityLayout;
	VkDescriptorSet computeVorticityDescriptor;
	{
		VkDescriptorImageInfo VELOCITYID{};
		VELOCITYID.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		VELOCITYID.imageView = _fluidImageBuffer[0][FULIDTEXTUREID::VELOCITYID]._imageView;

		VkDescriptorImageInfo DENSITYID{};
		DENSITYID.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		DENSITYID.imageView = _fluidImageBuffer[0][FULIDTEXTUREID::VORTICITYID]._imageView;

		vkutil::DescriptorBuilder::begin(_engine->_descriptorLayoutCache, _engine->_descriptorAllocator).
											bind_image(0, &VELOCITYID, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT).
											bind_image(1, &DENSITYID, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT).
											build(computeVorticityDescriptor, computeVorticityLayout);
	}

	{
	VkShaderModule compShader;
	if (!vkutil::load_shader_module("./spv/computeVorticity.comp.spv", _engine->_device, &compShader)){
		std::cout << "Error when building the computeVorticity comp shader module" << std::endl;
	}
	VkPipeline vorticityPipeline;
	VkPipelineLayout vorticityPipeLayout;

	VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
	computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	computeShaderStageInfo.module = compShader;
	computeShaderStageInfo.pName = "main";

	VkDescriptorSetLayout SetLayouts[] = { computeVorticityLayout};

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = SetLayouts;

	if (vkCreatePipelineLayout(_engine->_device, &pipelineLayoutInfo, nullptr, &vorticityPipeLayout) != VK_SUCCESS) {
	    throw std::runtime_error("failed to create compute pipeline layout!");
	}

	VkComputePipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.layout = vorticityPipeLayout;
	pipelineInfo.stage = computeShaderStageInfo;

	if (vkCreateComputePipelines(_engine->_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &vorticityPipeline) != VK_SUCCESS) {
	    throw std::runtime_error("failed to create compute pipeline!");
	}
	_engine->create_material(vorticityPipeline, vorticityPipeLayout, "COMPUTEVORTICITY", pipelineLayoutInfo.setLayoutCount, computeVorticityDescriptor);
	vkDestroyShaderModule(_engine->_device, compShader, nullptr);
	_deletionQueue.push_function([=]() {
    vkDestroyPipeline(_engine->_device, vorticityPipeline, nullptr);
		vkDestroyPipelineLayout(_engine->_device, vorticityPipeLayout, nullptr);
  });
	}
}

void StableFluidsScene::initConfineVorticity()
{
	VkDescriptorSetLayout confineVorticityLayout;
	VkDescriptorSet confineVorticityDescriptor;
	{
		VkDescriptorImageInfo VELOCITYID{};
		VELOCITYID.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		VELOCITYID.imageView = _fluidImageBuffer[0][FULIDTEXTUREID::VELOCITYID]._imageView;

		VkDescriptorImageInfo DENSITYID{};
		DENSITYID.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		DENSITYID.imageView = _fluidImageBuffer[0][FULIDTEXTUREID::VORTICITYID]._imageView;

		vkutil::DescriptorBuilder::begin(_engine->_descriptorLayoutCache, _engine->_descriptorAllocator).
											bind_image(0, &VELOCITYID, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT).
											bind_image(1, &DENSITYID, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT).
											build(confineVorticityDescriptor, confineVorticityLayout);
	}

	{
	VkShaderModule compShader;
	if (!vkutil::load_shader_module("./spv/confineVorticity.comp.spv", _engine->_device, &compShader)){
		std::cout << "Error when building the confineVorticity comp shader module" << std::endl;
	}
	VkPipeline vorticityPipeline;
	VkPipelineLayout vorticityPipeLayout;

	VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
	computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	computeShaderStageInfo.module = compShader;
	computeShaderStageInfo.pName = "main";

	VkDescriptorSetLayout SetLayouts[] = { confineVorticityLayout};

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = SetLayouts;

	if (vkCreatePipelineLayout(_engine->_device, &pipelineLayoutInfo, nullptr, &vorticityPipeLayout) != VK_SUCCESS) {
	    throw std::runtime_error("failed to create compute pipeline layout!");
	}

	VkComputePipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.layout = vorticityPipeLayout;
	pipelineInfo.stage = computeShaderStageInfo;

	if (vkCreateComputePipelines(_engine->_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &vorticityPipeline) != VK_SUCCESS) {
	    throw std::runtime_error("failed to create compute pipeline!");
	}
	_engine->create_material(vorticityPipeline, vorticityPipeLayout, "CONFINEVORTICITY", pipelineLayoutInfo.setLayoutCount, confineVorticityDescriptor);
	vkDestroyShaderModule(_engine->_device, compShader, nullptr);
	_deletionQueue.push_function([=]() {
    vkDestroyPipeline(_engine->_device, vorticityPipeline, nullptr);
		vkDestroyPipelineLayout(_engine->_device, vorticityPipeLayout, nullptr);
  });
	}
}

void StableFluidsScene::initDiffuse()
{
	VkDescriptorSetLayout diffuseLayout;
	VkDescriptorSet diffuseOddDescriptor;
	VkDescriptorSet diffuseEvenDescriptor;
	{
		VkDescriptorImageInfo VELOCITYID{};
		VELOCITYID.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		VELOCITYID.imageView = _fluidImageBuffer[0][FULIDTEXTUREID::VELOCITYID]._imageView;

		VkDescriptorImageInfo DENSITYID{};
		DENSITYID.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		DENSITYID.imageView = _fluidImageBuffer[0][FULIDTEXTUREID::DENSITYID]._imageView;

		VkDescriptorImageInfo VELOCITYTEMPID{};
		VELOCITYTEMPID.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		VELOCITYTEMPID.imageView = _fluidImageBuffer[1][FULIDTEXTUREID::VELOCITYID]._imageView;

		VkDescriptorImageInfo DENSITYTEMPID{};
		DENSITYTEMPID.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		DENSITYTEMPID.imageView =_fluidImageBuffer[1][FULIDTEXTUREID::DENSITYID]._imageView;

		vkutil::DescriptorBuilder::begin(_engine->_descriptorLayoutCache, _engine->_descriptorAllocator).
											bind_image(0, &VELOCITYID, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT).
											bind_image(1, &DENSITYID, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT).
											bind_image(2, &VELOCITYTEMPID, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT).
											bind_image(3, &DENSITYTEMPID, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT).
											build(diffuseOddDescriptor, diffuseLayout);
	}
	{
		VkDescriptorImageInfo VELOCITYID{};
		VELOCITYID.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		VELOCITYID.imageView = _fluidImageBuffer[1][FULIDTEXTUREID::VELOCITYID]._imageView;

		VkDescriptorImageInfo DENSITYID{};
		DENSITYID.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		DENSITYID.imageView = _fluidImageBuffer[1][FULIDTEXTUREID::DENSITYID]._imageView;

		VkDescriptorImageInfo VELOCITYTEMPID{};
		VELOCITYTEMPID.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		VELOCITYTEMPID.imageView = _fluidImageBuffer[0][FULIDTEXTUREID::VELOCITYID]._imageView;

		VkDescriptorImageInfo DENSITYTEMPID{};
		DENSITYTEMPID.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		DENSITYTEMPID.imageView =_fluidImageBuffer[0][FULIDTEXTUREID::DENSITYID]._imageView;

		vkutil::DescriptorBuilder::begin(_engine->_descriptorLayoutCache, _engine->_descriptorAllocator).
											bind_image(0, &VELOCITYID, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT).
											bind_image(1, &DENSITYID, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT).
											bind_image(2, &VELOCITYTEMPID, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT).
											bind_image(3, &DENSITYTEMPID, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT).
											build(diffuseEvenDescriptor, diffuseLayout);
	}
	{
	VkShaderModule compShader;
	if (!vkutil::load_shader_module("./spv/diffuse.comp.spv", _engine->_device, &compShader)){
		std::cout << "Error when building the diffuse comp shader module" << std::endl;
	}
	VkPipeline advectPipeline;
	VkPipelineLayout advectPipeLayout;

	VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
	computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	computeShaderStageInfo.module = compShader;
	computeShaderStageInfo.pName = "main";

	VkDescriptorSetLayout SetLayouts[] = { diffuseLayout};

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = SetLayouts;
	VkPushConstantRange push_constant;
	push_constant.offset = 0;
	push_constant.size = sizeof(FluidPushConstants);
	push_constant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	pipelineLayoutInfo.pPushConstantRanges = &push_constant;
	pipelineLayoutInfo.pushConstantRangeCount = 1;

	if (vkCreatePipelineLayout(_engine->_device, &pipelineLayoutInfo, nullptr, &advectPipeLayout) != VK_SUCCESS) {
	    throw std::runtime_error("failed to create compute pipeline layout!");
	}

	VkComputePipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.layout = advectPipeLayout;
	pipelineInfo.stage = computeShaderStageInfo;

	if (vkCreateComputePipelines(_engine->_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &advectPipeline) != VK_SUCCESS) {
	    throw std::runtime_error("failed to create compute pipeline!");
	}
	_engine->create_material(advectPipeline, advectPipeLayout, "DIFFUSE_first", pipelineLayoutInfo.setLayoutCount, diffuseOddDescriptor);
	_engine->create_material(advectPipeline, advectPipeLayout, "DIFFUSE_second", pipelineLayoutInfo.setLayoutCount, diffuseEvenDescriptor);
	vkDestroyShaderModule(_engine->_device, compShader, nullptr);
	_deletionQueue.push_function([=]() {
    vkDestroyPipeline(_engine->_device, advectPipeline, nullptr);
		vkDestroyPipelineLayout(_engine->_device, advectPipeLayout, nullptr);
  });
	}
}

void StableFluidsScene::initDivergence()
{
	VkDescriptorSetLayout DsecriptorLayout;
	VkDescriptorSet descriptorSet;
	{
		VkDescriptorImageInfo VELOCITYID{};
		VELOCITYID.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		VELOCITYID.imageView = _fluidImageBuffer[0][FULIDTEXTUREID::VELOCITYID]._imageView;

		VkDescriptorImageInfo DIVERGENCE{};
		DIVERGENCE.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		DIVERGENCE.imageView = _fluidImageBuffer[0][FULIDTEXTUREID::DIVERGENCEID]._imageView;

		VkDescriptorImageInfo PRESSURE{};
		PRESSURE.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		PRESSURE.imageView = _fluidImageBuffer[0][FULIDTEXTUREID::PRESSUREID]._imageView;

		VkDescriptorImageInfo PRESSURETEMPID{};
		PRESSURETEMPID.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		PRESSURETEMPID.imageView =_fluidImageBuffer[1][FULIDTEXTUREID::PRESSUREID]._imageView;

		vkutil::DescriptorBuilder::begin(_engine->_descriptorLayoutCache, _engine->_descriptorAllocator).
											bind_image(0, &VELOCITYID, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT).
											bind_image(1, &DIVERGENCE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT).
											bind_image(2, &PRESSURE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT).
											bind_image(3, &PRESSURETEMPID, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT).
											build(descriptorSet, DsecriptorLayout);
	}
	{
	VkShaderModule compShader;
	if (!vkutil::load_shader_module("./spv/divergence.comp.spv", _engine->_device, &compShader)){
		std::cout << "Error when building the divergence comp shader module" << std::endl;
	}
	VkPipeline pipeline;
	VkPipelineLayout pipeLayout;

	VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
	computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	computeShaderStageInfo.module = compShader;
	computeShaderStageInfo.pName = "main";

	VkDescriptorSetLayout SetLayouts[] = { DsecriptorLayout};

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = SetLayouts;

	if (vkCreatePipelineLayout(_engine->_device, &pipelineLayoutInfo, nullptr, &pipeLayout) != VK_SUCCESS) {
	    throw std::runtime_error("failed to create compute pipeline layout!");
	}

	VkComputePipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.layout = pipeLayout;
	pipelineInfo.stage = computeShaderStageInfo;

	if (vkCreateComputePipelines(_engine->_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
	    throw std::runtime_error("failed to create compute pipeline!");
	}
	_engine->create_material(pipeline, pipeLayout, "DIVERGENCE", pipelineLayoutInfo.setLayoutCount, descriptorSet);
	vkDestroyShaderModule(_engine->_device, compShader, nullptr);
	_deletionQueue.push_function([=]() {
    vkDestroyPipeline(_engine->_device, pipeline, nullptr);
		vkDestroyPipelineLayout(_engine->_device, pipeLayout, nullptr);
  });
	}
}

void StableFluidsScene::initJacobi()
{
	VkDescriptorSetLayout DsecriptorLayout;
	VkDescriptorSet firstdescriptorSet, secondDescriptor;
	{
		VkDescriptorImageInfo PRESSURE{};
		PRESSURE.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		PRESSURE.imageView = _fluidImageBuffer[0][FULIDTEXTUREID::PRESSUREID]._imageView;

		VkDescriptorImageInfo PRESSURETEMPID{};
		PRESSURETEMPID.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		PRESSURETEMPID.imageView =_fluidImageBuffer[1][FULIDTEXTUREID::PRESSUREID]._imageView;

		VkDescriptorImageInfo DIVERGENCE{};
		DIVERGENCE.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		DIVERGENCE.imageView = _fluidImageBuffer[0][FULIDTEXTUREID::DIVERGENCEID]._imageView;

		vkutil::DescriptorBuilder::begin(_engine->_descriptorLayoutCache, _engine->_descriptorAllocator).
											bind_image(0, &PRESSURE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT).
											bind_image(1, &PRESSURETEMPID, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT).
											bind_image(2, &DIVERGENCE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT).
											build(firstdescriptorSet, DsecriptorLayout);
	}
	{
		VkDescriptorImageInfo PRESSURE{};
		PRESSURE.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		PRESSURE.imageView = _fluidImageBuffer[1][FULIDTEXTUREID::PRESSUREID]._imageView;

		VkDescriptorImageInfo PRESSURETEMPID{};
		PRESSURETEMPID.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		PRESSURETEMPID.imageView =_fluidImageBuffer[0][FULIDTEXTUREID::PRESSUREID]._imageView;

		VkDescriptorImageInfo DIVERGENCE{};
		DIVERGENCE.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		DIVERGENCE.imageView = _fluidImageBuffer[0][FULIDTEXTUREID::DIVERGENCEID]._imageView;

		vkutil::DescriptorBuilder::begin(_engine->_descriptorLayoutCache, _engine->_descriptorAllocator).
											bind_image(0, &PRESSURE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT).
											bind_image(1, &PRESSURETEMPID, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT).
											bind_image(2, &DIVERGENCE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT).
											build(secondDescriptor, DsecriptorLayout);
	}
	{
	VkShaderModule compShader;
	if (!vkutil::load_shader_module("./spv/jacobi.comp.spv", _engine->_device, &compShader)){
		std::cout << "Error when building the jacobi comp shader module" << std::endl;
	}
	VkPipeline pipeline;
	VkPipelineLayout pipeLayout;

	VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
	computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	computeShaderStageInfo.module = compShader;
	computeShaderStageInfo.pName = "main";

	VkDescriptorSetLayout SetLayouts[] = { DsecriptorLayout};

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = SetLayouts;

	if (vkCreatePipelineLayout(_engine->_device, &pipelineLayoutInfo, nullptr, &pipeLayout) != VK_SUCCESS) {
	    throw std::runtime_error("failed to create compute pipeline layout!");
	}

	VkComputePipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.layout = pipeLayout;
	pipelineInfo.stage = computeShaderStageInfo;

	if (vkCreateComputePipelines(_engine->_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
	    throw std::runtime_error("failed to create compute pipeline!");
	}
	_engine->create_material(pipeline, pipeLayout, "JACOBI_first", pipelineLayoutInfo.setLayoutCount, firstdescriptorSet);
	_engine->create_material(pipeline, pipeLayout, "JACOBI_second", pipelineLayoutInfo.setLayoutCount, secondDescriptor);
	vkDestroyShaderModule(_engine->_device, compShader, nullptr);
	_deletionQueue.push_function([=]() {
    vkDestroyPipeline(_engine->_device, pipeline, nullptr);
		vkDestroyPipelineLayout(_engine->_device, pipeLayout, nullptr);
  });
	}
}

void StableFluidsScene::initApplyPressure()
{
	VkDescriptorSetLayout DsecriptorLayout;
	VkDescriptorSet descriptorSet;
	{
		VkDescriptorImageInfo VELOCITY{};
		VELOCITY.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		VELOCITY.imageView =_fluidImageBuffer[0][FULIDTEXTUREID::VELOCITYID]._imageView;

		VkDescriptorImageInfo PRESSURE{};
		PRESSURE.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		PRESSURE.imageView = _fluidImageBuffer[0][FULIDTEXTUREID::PRESSUREID]._imageView;


		vkutil::DescriptorBuilder::begin(_engine->_descriptorLayoutCache, _engine->_descriptorAllocator).
											bind_image(0, &VELOCITY, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT).
											bind_image(1, &PRESSURE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT).
											build(descriptorSet, DsecriptorLayout);
	}
	{
	VkShaderModule compShader;
	if (!vkutil::load_shader_module("./spv/applyPressure.comp.spv", _engine->_device, &compShader)){
		std::cout << "Error when building the applyPressure comp shader module" << std::endl;
	}
	VkPipeline pipeline;
	VkPipelineLayout pipeLayout;

	VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
	computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	computeShaderStageInfo.module = compShader;
	computeShaderStageInfo.pName = "main";

	VkDescriptorSetLayout SetLayouts[] = { DsecriptorLayout};

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = SetLayouts;

	if (vkCreatePipelineLayout(_engine->_device, &pipelineLayoutInfo, nullptr, &pipeLayout) != VK_SUCCESS) {
	    throw std::runtime_error("failed to create compute pipeline layout!");
	}

	VkComputePipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.layout = pipeLayout;
	pipelineInfo.stage = computeShaderStageInfo;

	if (vkCreateComputePipelines(_engine->_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
	    throw std::runtime_error("failed to create compute pipeline!");
	}
	_engine->create_material(pipeline, pipeLayout, "APPLYPRESSURE", pipelineLayoutInfo.setLayoutCount, descriptorSet);
	vkDestroyShaderModule(_engine->_device, compShader, nullptr);
	_deletionQueue.push_function([=]() {
    vkDestroyPipeline(_engine->_device, pipeline, nullptr);
		vkDestroyPipelineLayout(_engine->_device, pipeLayout, nullptr);
  });
	}
}

void StableFluidsScene::initAdvection()
{
	VkDescriptorSetLayout advectLayout;
	VkDescriptorSet advectDescriptor;
	{
		VkDescriptorImageInfo VELOCITYID{};
		VELOCITYID.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		VELOCITYID.imageView = _fluidImageBuffer[0][FULIDTEXTUREID::VELOCITYID]._imageView;

		VkDescriptorImageInfo DENSITYID{};
		DENSITYID.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		DENSITYID.imageView = _fluidImageBuffer[0][FULIDTEXTUREID::DENSITYID]._imageView;

		VkDescriptorImageInfo VELOCITYTEMPID{};
		VELOCITYTEMPID.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		VELOCITYTEMPID.imageView = _fluidImageBuffer[1][FULIDTEXTUREID::VELOCITYID]._imageView;
		VELOCITYTEMPID.sampler = _defaultSamplerLinear;

		VkDescriptorImageInfo DENSITYTEMPID{};
		DENSITYTEMPID.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		DENSITYTEMPID.imageView =_fluidImageBuffer[1][FULIDTEXTUREID::DENSITYID]._imageView;
		DENSITYTEMPID.sampler = _defaultSamplerLinear;

		VkDescriptorImageInfo VELOCITYSAMPLENEAR{};
		VELOCITYSAMPLENEAR.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		VELOCITYSAMPLENEAR.imageView = _fluidImageBuffer[1][FULIDTEXTUREID::VELOCITYID]._imageView;
		VELOCITYSAMPLENEAR.sampler = _defualtSamplerNear;

		vkutil::DescriptorBuilder::begin(_engine->_descriptorLayoutCache, _engine->_descriptorAllocator).
											bind_image(0, &VELOCITYID, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT).
											bind_image(1, &DENSITYID, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT).
											bind_image(2, &VELOCITYTEMPID, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT).
											bind_image(3, &DENSITYTEMPID, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT).
											bind_image(4, &VELOCITYSAMPLENEAR, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT).
											build(advectDescriptor, advectLayout);
	}
	{
	VkShaderModule compShader;
	if (!vkutil::load_shader_module("./spv/advect.comp.spv", _engine->_device, &compShader)){
		std::cout << "Error when building the sourcing comp shader module" << std::endl;
	}
	VkPipeline advectPipeline;
	VkPipelineLayout advectPipeLayout;

	VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
	computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	computeShaderStageInfo.module = compShader;
	computeShaderStageInfo.pName = "main";

	VkDescriptorSetLayout SetLayouts[] = { advectLayout};

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = SetLayouts;
	VkPushConstantRange push_constant;
	push_constant.offset = 0;
	push_constant.size = sizeof(FluidPushConstants);
	push_constant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	pipelineLayoutInfo.pPushConstantRanges = &push_constant;
	pipelineLayoutInfo.pushConstantRangeCount = 1;

	if (vkCreatePipelineLayout(_engine->_device, &pipelineLayoutInfo, nullptr, &advectPipeLayout) != VK_SUCCESS) {
	    throw std::runtime_error("failed to create compute pipeline layout!");
	}

	VkComputePipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.layout = advectPipeLayout;
	pipelineInfo.stage = computeShaderStageInfo;

	if (vkCreateComputePipelines(_engine->_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &advectPipeline) != VK_SUCCESS) {
	    throw std::runtime_error("failed to create compute pipeline!");
	}
	_engine->create_material(advectPipeline, advectPipeLayout, "ADVECT", pipelineLayoutInfo.setLayoutCount, advectDescriptor);
	vkDestroyShaderModule(_engine->_device, compShader, nullptr);
	_deletionQueue.push_function([=]() {
    vkDestroyPipeline(_engine->_device, advectPipeline, nullptr);
		vkDestroyPipelineLayout(_engine->_device, advectPipeLayout, nullptr);
  });
	}
}

void StableFluidsScene::init_pipelines()
{
	initRenderPipelines();
	initSourcing();
	initComputeVorticity();
	initConfineVorticity();
	initDiffuse();
	initDivergence();
	initJacobi();
	initApplyPressure();
	initAdvection();
}

void StableFluidsScene::init_compute_pipelines(VkShaderModule compShader, const std::string name)
{
	VkPipeline particlePipeline;
	VkPipelineLayout fulidPipeLayout;

	VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
	computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	computeShaderStageInfo.module = compShader;
	computeShaderStageInfo.pName = "main";

	VkDescriptorSetLayout SetLayouts[] = { _fluidSetLayout, _fluidSetLayout };

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 2;
	pipelineLayoutInfo.pSetLayouts = SetLayouts;
	VkPushConstantRange push_constant;
	push_constant.offset = 0;
	push_constant.size = sizeof(FluidPushConstants);
	push_constant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	pipelineLayoutInfo.pPushConstantRanges = &push_constant;
	pipelineLayoutInfo.pushConstantRangeCount = 1;

	if (vkCreatePipelineLayout(_engine->_device, &pipelineLayoutInfo, nullptr, &fulidPipeLayout) != VK_SUCCESS) {
	    throw std::runtime_error("failed to create compute pipeline layout!");
	}

	VkComputePipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.layout = fulidPipeLayout;
	pipelineInfo.stage = computeShaderStageInfo;

	if (vkCreateComputePipelines(_engine->_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &particlePipeline) != VK_SUCCESS) {
	    throw std::runtime_error("failed to create compute pipeline!");
	}
	_engine->create_material(particlePipeline, fulidPipeLayout, name, pipelineLayoutInfo.setLayoutCount);
	vkDestroyShaderModule(_engine->_device, compShader, nullptr);
	_deletionQueue.push_function([=]() {
    vkDestroyPipeline(_engine->_device, particlePipeline, nullptr);
		vkDestroyPipelineLayout(_engine->_device, fulidPipeLayout, nullptr);
  });
}

void StableFluidsScene::load_vertex()
{
	const std::vector<Vertex> vertices = {
    {{-1.f, -1.f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{1.f, -1.f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{1.f, 1.f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
		{{1.f, 1.f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
		{{-1.f, 1.f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
    {{-1.f, -1.f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}
};

	const size_t bufferSize = vertices.size() * sizeof(Vertex);
	VkBufferCreateInfo stagingBufferInfo = {};
	stagingBufferInfo.pNext = nullptr;
	stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	stagingBufferInfo.size = bufferSize;
	stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

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
  memcpy(data, vertices.data(), bufferSize);
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
			&_fluidBuffer[i]._buffer,
			&_fluidBuffer[i]._allocation,
			nullptr));

		_engine->immediate_submit([=](VkCommandBuffer cmd){
			VkBufferCopy copy;
			copy.dstOffset = 0;
			copy.srcOffset = 0;
			copy.size = bufferSize;
			vkCmdCopyBuffer(cmd, stagingBuffer._buffer, _fluidBuffer[i]._buffer, 1, &copy);
		});

		_deletionQueue.push_function([=]() {
		vmaDestroyBuffer(_engine->_allocator, _fluidBuffer[i]._buffer, _fluidBuffer[i]._allocation);
		});
	}
	vmaDestroyBuffer(_engine->_allocator, stagingBuffer._buffer, stagingBuffer._allocation);
}

void StableFluidsScene::init_image_buffer()
{
	for (int i = 0; i < 5; ++i){
		_fluidImageBuffer[0][i] = _engine->create_image(VkExtent3D{ imageWidth, imageHeight, 1 }, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT, false);
		_fluidImageBuffer[1][i] = _engine->create_image(VkExtent3D{ imageWidth, imageHeight, 1 }, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT, false);
		_engine->immediate_submit([=](VkCommandBuffer cmd){
			vkutil::transitionImageLayout(cmd, _fluidImageBuffer[0][i]._image,VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
			vkutil::transitionImageLayout(cmd, _fluidImageBuffer[1][i]._image,VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
		});
	}

	_fluidDrawImageBuffer = _engine->create_image(VkExtent3D{ imageWidth, imageHeight, 1 }, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT, false);
	_velocitySamplerImageBuffer = _engine->create_image(VkExtent3D{ imageWidth, imageHeight, 1 }, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT, false);
	_densitySamplerImageBuffer = _engine->create_image(VkExtent3D{ imageWidth, imageHeight, 1 }, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT, false);
	_engine->immediate_submit([=](VkCommandBuffer cmd){
	vkutil::transitionImageLayout(cmd, _fluidDrawImageBuffer._image,VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
	vkutil::transitionImageLayout(cmd, _velocitySamplerImageBuffer._image,VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
	vkutil::transitionImageLayout(cmd, _densitySamplerImageBuffer._image,VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
	});

	VkSamplerCreateInfo sampl = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
	sampl.magFilter = VK_FILTER_LINEAR;
	sampl.minFilter = VK_FILTER_LINEAR;
	VK_CHECK(vkCreateSampler(_engine->_device, &sampl, nullptr, &_defaultSamplerLinear));
	
	sampl.magFilter = VK_FILTER_NEAREST;
	sampl.minFilter = VK_FILTER_NEAREST;
	vkCreateSampler(_engine->_device, &sampl, nullptr, &_defualtSamplerNear);
	// vkCreateSampler(_engine->_device, &sampl, nullptr, &_densitySamplerLinear);

	
	_deletionQueue.push_function([=]() {
		vkDestroySampler(_engine->_device, _defaultSamplerLinear, nullptr);
		vkDestroySampler(_engine->_device, _defualtSamplerNear, nullptr);


		vkDestroyImageView(_engine->_device, _fluidDrawImageBuffer._imageView, nullptr);
		vmaDestroyImage(_engine->_allocator, _fluidDrawImageBuffer._image, _fluidDrawImageBuffer._allocation);
		vkDestroyImageView(_engine->_device, _velocitySamplerImageBuffer._imageView, nullptr);
		vmaDestroyImage(_engine->_allocator, _velocitySamplerImageBuffer._image, _velocitySamplerImageBuffer._allocation);
		vkDestroyImageView(_engine->_device, _densitySamplerImageBuffer._imageView, nullptr);
		vmaDestroyImage(_engine->_allocator, _densitySamplerImageBuffer._image, _densitySamplerImageBuffer._allocation);
		for (int i =0; i < 5;++i){
			vkDestroyImageView(_engine->_device, _fluidImageBuffer[0][i]._imageView, nullptr);
			vkDestroyImageView(_engine->_device, _fluidImageBuffer[1][i]._imageView, nullptr);
			vmaDestroyImage(_engine->_allocator, _fluidImageBuffer[0][i]._image, _fluidImageBuffer[0][i]._allocation);
			vmaDestroyImage(_engine->_allocator, _fluidImageBuffer[1][i]._image, _fluidImageBuffer[1][i]._allocation);
		}
	});
}

void StableFluidsScene::init_descriptors(){}//fix me