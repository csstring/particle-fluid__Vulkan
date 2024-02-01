#pragma once

#include "vk_engine.hpp"

class ParticleScene : public Scene
{
  private:
    VulkanEngine* _engine;

    DeletionQueue _deletionQueue;
    ComputeContext _computeContext[2];
    AllocatedBuffer _particleBuffer[2];
    uint32_t PARTICLE_COUNT;
    VkDescriptorSetLayout _particleSetLayout;
    VkDescriptorSet particleDescriptor[2];
    AllocatedBuffer _cursorBuffer[2];
    uint32_t _curFrameIdx{0};
    void particleUpdate(VkCommandBuffer cmd);
    
  public:
    void initialize(VulkanEngine* engine);

    void load_particle();
    void update(float dt, uint32_t frameidx);
    void draw(VkCommandBuffer cmd);
    void guiRender();
    void init_commands();
		void init_sync_structures();
		void init_descriptors();
		void init_pipelines();
    void init_compute_pipelines();
    ParticleScene(){};
    ~ParticleScene(){
      _deletionQueue.flush();
    };
};