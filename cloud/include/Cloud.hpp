#pragma once

#include "vk_engine.hpp"

enum CLOUDTEXTUREID {
  CLOUDDENSITY,
  CLOUDLIGHT
};

class CloudScene : public Scene
{
  private:
    VulkanEngine* _engine;

    DeletionQueue _deletionQueue;
    ComputeContext _computeContext;

    VkSampler _defaultSamplerLinear;
    VkSampler _defualtSamplerNear;


    void makeLightTexture();
    void genCloud();

    void initRenderPipelines();
    void initGenCloudPipelines();
    void initMakeLightTexturePipelines();

    uint32_t dispatchSize = 4;
    uint32_t imageWidth = 128;
    uint32_t imageHeight = 128;
    uint32_t imageDepth = 128;

    glm::vec3 modelTrans;
  public:
    uint32_t _curFrameIdx{0};
    AllocatedImage _cloudImageBuffer[2][2];
    CloudPushConstants constants;

    void initialize(VulkanEngine* engine);

    void uploadCubeMesh();
    void update(float dt, uint32_t frameidx);
    void draw(VkCommandBuffer cmd);
    void guiRender();
    void init_commands();
		void init_sync_structures();
    void init_image_buffer();
		void init_pipelines();
    void init_descriptors();//fix me

    CloudScene(){};
    ~CloudScene(){
      _deletionQueue.flush();
    };
};