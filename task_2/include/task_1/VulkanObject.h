#pragma once

#include "vulkan/vulkan.hpp"
#include "VulkanStructs.h"
#include "UBO.h"
#include "Vertex.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <iostream>
#include <optional>

#include "task_1/Model.h"

class VulkanObject {
public:
    void initVulkan(GLFWwindow* window);
    void drawFrame();
    void cleanup();

    VkDevice device;

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
        VulkanObject* app = reinterpret_cast<VulkanObject*>(glfwGetWindowUserPointer(window));
        app->framebufferResized = true;
    }

private:
    // pointer to GLFW window instance
    GLFWwindow* window;

    VkClearValue imgui_clear_value;

    int MAX_FRAMES_IN_FLIGHT = 2;

    // vulkan library instance
    VkInstance instance;
    // create instance of debug messenger
    VkDebugUtilsMessengerEXT debugMessenger;
    // create a "surface" to interface with any window system
    VkSurfaceKHR surface;

    // physical device to use
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    // logical device to use

    // handle to graphics queue
    VkQueue graphicsQueue;
    // handle to graphics queue
    VkQueue presentQueue;

    // our swap chain object
    VkSwapchainKHR swapChain;
    // vector of our swap chain images 
    std::vector<VkImage> swapChainImages;
    // the format of our swap chain
    VkFormat swapChainImageFormat;
    // the extent (dimensions) of our swap chain images
    VkExtent2D swapChainExtent;

    struct FrameBufferAttachment {
        VkImage image;
        VkDeviceMemory mem;
        VkImageView view;
        VkFormat format;
    };
    struct FrameBuffer {
        int32_t width, height;
        VkFramebuffer frameBuffer;
        FrameBufferAttachment position, normal, albedo;
        FrameBufferAttachment depth;
        VkRenderPass renderPass;
    } offScreenPass;

    struct DepthFrameBuffer {
        int32_t width, height;
        VkFramebuffer frameBuffer;
        FrameBufferAttachment depth;
        VkSampler sampler;
        VkSampler pcfsampler;
        VkRenderPass renderPass;
    } shadowPass;
	
    // vector of image views (to access our images)
    std::vector<VkImageView> swapChainImageViews;
    // vector of all frame buffers
    std::vector<VkFramebuffer> swapChainFramebuffers;

    std::vector<VkFramebuffer> imgui_frame_buffers;

    VkFramebuffer geometryFrameBuffer;

    VkDescriptorSetLayout lightingSetLayout;
    VkDescriptorSetLayout shadowSetLayout;
	
    // render pass object
    VkRenderPass renderPass;
    VkRenderPass geometryPass;
    VkRenderPass imgui_render_pass;
    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout;
    VkPipelineLayout lightingLayout;
    VkPipelineLayout shadowLayout;
    VkPipeline graphicsPipeline;
    VkPipeline lightingPipeline;
    VkPipeline shadowPipeline;

    // create a command pool to manage the memory required for our command buffers
    VkCommandPool commandPool;
    // vector of command buffers. One for each image in swap chain
    std::vector<VkCommandBuffer> commandBuffers;

    VkCommandPool imgui_command_pool;
    std::vector<VkCommandBuffer> imgui_command_buffers;

    // vector of semaphores indicating images have been aquired
    std::vector<VkSemaphore> imageAvailableSemaphores;
    // vector of semaphores indicating images are ready for rendering
    std::vector<VkSemaphore> renderFinishedSemaphores;
    // create a fence for each frame
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;
    // the current frame we are working on
    size_t currentFrame = 0;

    // bool to store if we have resized
    bool framebufferResized = false;

    Model dragon_model;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;

    std::vector<VkBuffer> shadowUniformBuffers;
    std::vector<VkDeviceMemory> shadowUniformBuffersMemory;

    VkDescriptorPool descriptorPool;
    VkDescriptorPool lightingDescriptorPool;
    VkDescriptorPool shadowDescriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;
    std::vector<VkDescriptorSet> lightingDescriptorSets;
    std::vector<VkDescriptorSet> shadowDescriptorSets;
    VkDescriptorPool imgui_descriptor_pool;

    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;
    VkSampler textureSampler;

    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;

    std::string MODEL_PATH;
    std::string TEXTURE_PATH;

    bool model_stage_on = false;
    bool texture_stage_on = false;
    bool lighting_stage_on = false;
    float zoom = 10.0;
    float x_offset = 0;
    float y_offset = 0;
    float z_offset = 0;
    float camera_x_rotation = 0;
    float camera_y_rotation = 0;
    float camera_z_rotation = 0;
    float x_rotation = 0;
    float y_rotation = 0;
    float z_rotation = 0;
    float x_light_rotation = 0;
    float y_light_rotation = 0;
    float z_light_rotation = 0;
    float scale = 1.0f;
    int display_mode = 0;
    float shadow_bias = 0.0;
    bool pcf = false;

    void createImguiPass();
    void createGeometryPass();
    void createShadowPass();
	
    VkFormat findDepthFormat();

    bool hasStencilComponent(VkFormat format);

    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

    void createDepthResources();

    void createTextureSampler();

    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

    void loadModel();

    void createTextureImageView();

    void createTextureImage();

    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);

    void createDescriptorPool();

    void createUniformBuffers();

    void createDescriptorSetLayout();

    void createIndexBuffer();

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

    void createVertexBuffer();

    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

    VkCommandBuffer beginSingleTimeCommands();

    void endSingleTimeCommands(VkCommandBuffer commandBuffer);

    void createCommandPool(VkCommandPool* commandPool, VkCommandPoolCreateFlags flags);

    void createCommandBuffers(VkCommandBuffer* commandBuffer, uint32_t commandBufferCount, VkCommandPool& commandPool);

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    // clean up swap chain for a clean recreate
    void cleanupSwapChain();

    void createInstance();

    // populate a given struct with callback info
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

    void setupDebugMessenger();

    // create our surface
    void createSurface();

    void pickPhysicalDevice();

    // create a logical device to use based off physical
    void createLogicalDevice();

    // create a swap chain
    void createSwapChain();

    // create our image views
    void createImageViews();

    // create our render pass object
    void createRenderPass();

    // recreate swap chain incase it is invalidated
    void recreateSwapChain();

    void createDescriptorSets();

    // create the graphics pipeline.
    void createGraphicsPipeline();

    // function to create all of our framebuffers
    void createFramebuffers();

    // create our command pool
    void createCommandPool();

    // create command buffers
    void createCommandBuffers();

    void createSyncObjects();

    void updateUniformBuffer(uint32_t currentImage);

    // create a VkShaderModule to encapsulate our shaders
    VkShaderModule createShaderModule(const std::vector<char>& code);

    // function for choosing the format to use from available formats
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

    // function to choose a prefere presentation mode
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

    // function to choose a good width and height for images
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    // populate swap chain struct
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

    // check a physical device to see if it is suitable
    bool isDeviceSuitable(VkPhysicalDevice device);

    // check that our device has support for the set of extensions we are interested in
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);

    // search for queue family support
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

    // return required list of extensions
    std::vector<const char*> getRequiredExtensions();

    // check whether all requested layers are available. return true iff they are
    bool checkValidationLayerSupport();

    // VKAPI_ATTR and VKAPI_CALL ensure the function is callable by vulkan

    // We pass in the message severity as VK_DEBUG_UTILS_MESSAGE_SEVERITY_{VERBOSE|INFO|WARNING|ERROR}_BIT_EXT
    // messageType can be VK_DEBUG_UTILS_MESSAGE_TYPE_{GENERAL|VALIDATION|PERFORMANCE}_BIT_EXT
    // pCallbackData contains data related to the message. Importantly: pMessage, pObjects, objectCount
    // pUserData can be used to pass custom data in

    // the function is used for taking control of debug output
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) {
        // output message
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

        // unless testing validation layer itself, return VK_FALSE (0)
        return VK_FALSE;
    }
};
