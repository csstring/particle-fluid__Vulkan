#pragma once
#include "vk_types.hpp"
#include "vk_descriptors.hpp"
#include "vk_mesh.hpp"
#include <glm/glm.hpp>
#include <unordered_map>

constexpr unsigned int FRAME_OVERLAP = 2;
class ParticleScene;
class StableFluidsScene;
class CloudScene;

class VulkanEngine
{
  private:
    void init_vulkan();
    void init_swapchain();
    void init_commands();
    void init_default_renderpass();
	  void init_framebuffers();
    void init_sync_structures();
    void init_pipelines();
    void init_scene();
    void load_meshes();
    void draw_objects(VkCommandBuffer cmd,RenderObject* first, int count);
    void init_descriptors();
    void init_imgui();

  private:  
    ParticleScene* particleScene;
    StableFluidsScene* fluidScene;
    CloudScene* cloudScene;

  public :
    void init();
    void cleanup();
    void draw();
    void run();
    void load_images();
	  void upload_mesh(Mesh& mesh);
    FrameData& get_current_frame(){return _frames[_frameNumber % FRAME_OVERLAP];}
    Material* create_material(VkPipeline pipeline, VkPipelineLayout layout,
    const std::string& name, 
    uint32_t layoutCount = 0, VkDescriptorSet descriptorSet = VK_NULL_HANDLE,
    uint32_t constantSize = 0, void* constantPtr = nullptr
    );
    Material* get_material(const std::string& name);
    AllocatedImage create_image(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped);
    Mesh* get_mesh(const std::string& name);
    size_t pad_uniform_buffer_size(size_t originalSize);
    AllocatedBuffer create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
    void immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function);


  public :
    bool _isInitialized{ false };
	  int _frameNumber {0};
	  VkExtent2D _windowExtent{ 1920 , 1080 };
    struct SDL_Window* _window{ nullptr };

    DeletionQueue _mainDeletionQueue;
    VkInstance _instance; // Vulkan library handle
    VkDebugUtilsMessengerEXT _debug_messenger; // Vulkan debug output handle
    VkPhysicalDevice _chosenGPU; // GPU chosen as the default device
    VkDevice _device; // Vulkan device for commands
    VkSurfaceKHR _surface; // Vulkan window surface
    VmaAllocator _allocator;
    VkPhysicalDeviceProperties _gpuProperties;

    FrameData _frames[FRAME_OVERLAP];

    VkSwapchainKHR _swapchain;
    VkFormat _swapchainImageFormat;
    std::vector<VkImage> _swapchainImages;
    std::vector<VkImageView> _swapchainImageViews;

    VkQueue _graphicsQueue; //queue we will submit to
    uint32_t _graphicsQueueFamily; //family of that queue
    
    VkRenderPass _renderPass;
	  std::vector<VkFramebuffer> _framebuffers;

    int _selectedShader{ 0 };

    VkImageView _depthImageView;
    VkFormat _depthFormat;
	  AllocatedImage _depthImage;

    std::vector<RenderObject> _renderables;
    std::unordered_map<std::string,Material> _materials;
    std::unordered_map<std::string,Mesh> _meshes;

    VkDescriptorSetLayout _globalSetLayout;
    VkDescriptorSetLayout _objectSetLayout;
    VkDescriptorSetLayout _singleTextureSetLayout;
    
    VkDescriptorPool _descriptorPool;

    GPUSceneData _sceneParameters;
    AllocatedBuffer _sceneParameterBuffer;
    AllocatedBuffer _cameraBuffer;
    
    UploadContext _uploadContext;
    std::unordered_map<std::string, Texture> _loadedTextures;

    vkutil::DescriptorLayoutCache* _descriptorLayoutCache{nullptr};
    vkutil::DescriptorAllocator* _descriptorAllocator{nullptr};

};