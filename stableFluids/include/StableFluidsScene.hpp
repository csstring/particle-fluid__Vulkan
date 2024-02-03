#pragma once

#include "vk_engine.hpp"

enum FULIDTEXTUREID {
  VELOCITYID,
  VORTICITYID,
  PRESSUREID,
  DIVERGENCEID,
  DENSITYID
};

static const std::vector<glm::vec4> rainbow = {
                {1.0f, 0.0f, 0.0f, 1.0f},  // Red
                {1.0f, 0.65f, 0.0f, 1.0f}, // Orange
                {1.0f, 1.0f, 0.0f, 1.0f},  // Yellow
                {0.0f, 1.0f, 0.0f, 1.0f},  // Green
                {0.0f, 0.0f, 1.0f, 1.0f},  // Blue
                {0.3f, 0.0f, 0.5f, 1.0f},  // Indigo
                {0.5f, 0.0f, 1.0f, 1.0f}   // Violet/Purple
            };

class StableFluidsScene : public Scene
{
  private:
    VulkanEngine* _engine;

    DeletionQueue _deletionQueue;
    ComputeContext _computeContext[2];
    AllocatedBuffer _fluidBuffer[2];
    VkDescriptorSet fluidDescriptor[2];
    VkDescriptorSetLayout _fluidSetLayout;
    VkDescriptorSet fluidDrawDescriptor;
    // VkDescriptorSetLayout _fluidDrawSetLayout;
    VkSampler _defaultSamplerLinear;
    VkSampler _defualtSamplerNear;
    VkSampler _unnormSamplerLinear;
    VkSampler _unnormSamplerNear;

    FluidPushConstants constants;

    uint32_t dispatchSize = 16;
    uint32_t imageWidth = 384*2;
    uint32_t imageHeight = 384*2;


    void sourcing();
    void vorticity();
    void diffuse();
    void projection();
    void advection();
    
    void initRenderPipelines();
    void initSourcing();
    void initComputeVorticity();
    void initConfineVorticity();
    void initDiffuse();
    void initDivergence();
    void initJacobi();
    void initApplyPressure();
    void initAdvection();

  public:
    uint32_t _curFrameIdx{0};
    AllocatedImage _fluidImageBuffer[2][5];
    AllocatedImage _fluidDrawImageBuffer;
    AllocatedImage _velocitySamplerImageBuffer;
    AllocatedImage _densitySamplerImageBuffer;
    
    void initialize(VulkanEngine* engine);

    void load_vertex();
    void update(float dt, uint32_t frameidx);
    void draw(VkCommandBuffer cmd);
    void guiRender();
    void init_commands();
		void init_sync_structures();
    void init_image_buffer();
		void init_pipelines();
    void init_compute_pipelines(VkShaderModule compShader, const std::string name);
    
    void init_descriptors();//fix me
    StableFluidsScene(){};
    ~StableFluidsScene(){
      _deletionQueue.flush();
    };
};