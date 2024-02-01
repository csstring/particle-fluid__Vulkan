#pragma once
#include "vk_types.hpp"

namespace vkutil{
  bool load_shader_module(const char* filePath,VkDevice device,VkShaderModule* outShaderModule);
}

class PipelineBuilder {
  
  public:
    std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;
    VkPipelineVertexInputStateCreateInfo _vertexInputInfo;
    VkPipelineInputAssemblyStateCreateInfo _inputAssembly;
    VkViewport _viewport;
    VkRect2D _scissor;
    VkPipelineRasterizationStateCreateInfo _rasterizer;
    VkPipelineColorBlendAttachmentState _colorBlendAttachment;
    VkPipelineMultisampleStateCreateInfo _multisampling;
    VkPipelineLayout _pipelineLayout;
    VkPipelineDepthStencilStateCreateInfo _depthStencil;
    
    VkPipeline build_pipeline(VkDevice device, VkRenderPass pass);
};

