// Vulkan0.cpp : Defines the entry point for the application.
//

//includes various vulkan, glm and glfw headers
#include "task_1/VulkanObject.h"
#include "task_1/Vertex.h"
#include "task_1/HelperFunctions.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

//includes C++ headers
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <optional>
#include <set>
#include <unordered_map>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include "task_1/Model.h"

// vector of validation layers to be used.

// by default VK_LAYER_KHRONOS_validation should be included, containing a 
// bundle of common validation layers

// validation layers are used enable optional components which can assist 
// with debugging. The alternative is simply crashing out of potentially trivial bugs.
const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

// vector of required device extensions.

// the default VK_KHR_SWAPCHAIN_EXTENSION_NAME
// checks that a swapchain is supported by the device.

// a swap chain is set of framebuffers that can be swapped for added stability.
const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

// below is a pre-processor directive which when a debug build is run, enables validation
// (and when in any other build type, does not)
#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

// create debug messenger within VkInstance instance
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    // function not automatically loaded, so we look up address
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    // if we found the address
    if (func != nullptr) {
        // call the function
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else {
        // return an error to be dealt with by caller
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

// destroy debug messenger
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {

    // locate address of vkDestroyDebugUtilsMessengerEXT in extension
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

    // if we find it
    if (func != nullptr) {
        // call it!
        func(instance, debugMessenger, pAllocator);
    }
}

void VulkanObject::createCommandPool(VkCommandPool* commandPool, VkCommandPoolCreateFlags flags) {
    VkCommandPoolCreateInfo commandPoolCreateInfo = {};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.queueFamilyIndex = findQueueFamilies(physicalDevice).graphicsFamily.value();
    commandPoolCreateInfo.flags = flags;

    if (vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, commandPool) != VK_SUCCESS) {
        throw std::runtime_error("Could not create graphics command pool");
    }
}

void VulkanObject::createCommandBuffers(VkCommandBuffer* commandBuffer, uint32_t commandBufferCount, VkCommandPool& commandPool) {
    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandPool = commandPool;
    commandBufferAllocateInfo.commandBufferCount = commandBufferCount;
    vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, commandBuffer);
}

void VulkanObject::initVulkan(GLFWwindow* window) {
    this->window = window;

    imgui_clear_value = { 0.6, 0.4, 0.0, 1.0 };

    MODEL_PATH = "../assets/dragon_cow_and_plane/dragon_cow_and_plane.obj";
    TEXTURE_PATH = "../assets/duck/texture.jpg";

    // function to create an instance of the vulkan library
    createInstance();
    // setup our debugger to control output
    setupDebugMessenger();
    // create our surface
    createSurface();
    // pick a physical device to use
    pickPhysicalDevice();
    // create a logical device to use based off physical device
    createLogicalDevice();
    // create a swap chain
    createSwapChain();
    // create our image views
    createImageViews();
    // create render pass object using previous information
    createRenderPass();
    createDescriptorSetLayout();
    // create graphics pipeline
    createGraphicsPipeline();
    // create our command pool
    createCommandPool();
    createDepthResources();
    // function to create framebuffers and populate swapChainFramebuffers vector
    createFramebuffers();

    imgui_frame_buffers.resize(swapChainImages.size());
	
    {
        VkImageView attachment[1];
        VkFramebufferCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        info.renderPass = imgui_render_pass;
        info.attachmentCount = 1;
        info.pAttachments = attachment;
        info.width = swapChainExtent.width;
        info.height = swapChainExtent.height;
        info.layers = 1;
        for (uint32_t i = 0; i < swapChainImages.size(); i++)
        {
            attachment[0] = swapChainImageViews[i];
            vkCreateFramebuffer(device, &info, VK_NULL_HANDLE, &imgui_frame_buffers[i]);
        }
    }

    createTextureImage();
    createTextureImageView();
    createTextureSampler();
    loadModel();
    createVertexBuffer();
    createIndexBuffer();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();

    VkDescriptorPoolSize imgui_pool_sizes[] =
    {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };
    VkDescriptorPoolCreateInfo imgui_pool_info = {};
    imgui_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    imgui_pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    imgui_pool_info.maxSets = 1000 * IM_ARRAYSIZE(imgui_pool_sizes);
    imgui_pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(imgui_pool_sizes);
    imgui_pool_info.pPoolSizes = imgui_pool_sizes;
    vkCreateDescriptorPool(device, &imgui_pool_info, VK_NULL_HANDLE, &imgui_descriptor_pool);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui_ImplGlfw_InitForVulkan(window, true);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = instance;
    init_info.PhysicalDevice = physicalDevice;
    init_info.Device = device;
    init_info.QueueFamily = findQueueFamilies(physicalDevice).graphicsFamily.value();
    init_info.Queue = graphicsQueue;
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = imgui_descriptor_pool;
    init_info.Allocator = VK_NULL_HANDLE;
    init_info.MinImageCount = swapChainImages.size();
    init_info.ImageCount = swapChainImages.size();
    init_info.CheckVkResultFn = VK_NULL_HANDLE;
    ImGui_ImplVulkan_Init(&init_info, imgui_render_pass);

    VkCommandBuffer command_buffer = beginSingleTimeCommands();
    ImGui_ImplVulkan_CreateFontsTexture(command_buffer);
    endSingleTimeCommands(command_buffer);

    createCommandPool(&imgui_command_pool, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    imgui_command_buffers.resize(swapChainImageViews.size());
    createCommandBuffers(imgui_command_buffers.data(), static_cast<uint32_t>(imgui_command_buffers.size()), imgui_command_pool);

    // create command buffers
    createCommandBuffers();
    // create and set up semaphores and fences
    createSyncObjects();
}

VkFormat VulkanObject::findDepthFormat() {
    return findSupportedFormat(
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

bool VulkanObject::hasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

VkFormat VulkanObject::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format!");
}

void VulkanObject::createDepthResources() {
    VkFormat depthFormat = findDepthFormat();
    createImage(swapChainExtent.width, swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
    depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

    transitionImageLayout(depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);


}

void VulkanObject::createTextureSampler() {
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }
}

VkImageView VulkanObject::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    viewInfo.subresourceRange.aspectMask = aspectFlags;

    VkImageView imageView;
    if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture image view!");
    }

    return imageView;
}

void VulkanObject::loadModel()
{
    dragon_model.loadModel("../assets/dragon_cow_and_plane/dragon_cow_and_plane.obj");
}

void VulkanObject::createTextureImageView() {
    textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
}

void VulkanObject::createTextureImage() {
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(device, stagingBufferMemory);

    stbi_image_free(pixels);

    createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);

    transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

    transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void VulkanObject::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(device, image, imageMemory, 0);
}

void VulkanObject::createDescriptorPool() {
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(swapChainImages.size());
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(swapChainImages.size());

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(swapChainImages.size());

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }

    std::array<VkDescriptorPoolSize, 4> lightingPoolSizes{};
    lightingPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    lightingPoolSizes[0].descriptorCount = static_cast<uint32_t>(swapChainImages.size());
    lightingPoolSizes[1].type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    lightingPoolSizes[1].descriptorCount = static_cast<uint32_t>(swapChainImages.size());
    lightingPoolSizes[2].type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    lightingPoolSizes[2].descriptorCount = static_cast<uint32_t>(swapChainImages.size());
    lightingPoolSizes[3].type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    lightingPoolSizes[3].descriptorCount = static_cast<uint32_t>(swapChainImages.size());

    VkDescriptorPoolCreateInfo lightingPoolInfo{};
    lightingPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    lightingPoolInfo.poolSizeCount = static_cast<uint32_t>(lightingPoolSizes.size());
    lightingPoolInfo.pPoolSizes = lightingPoolSizes.data();
    lightingPoolInfo.maxSets = static_cast<uint32_t>(swapChainImages.size());

    if (vkCreateDescriptorPool(device, &lightingPoolInfo, nullptr, &lightingDescriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

void VulkanObject::createUniformBuffers() {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    uniformBuffers.resize(swapChainImages.size());
    uniformBuffersMemory.resize(swapChainImages.size());

    for (size_t i = 0; i < swapChainImages.size(); i++) {
        createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);
    }
}

void VulkanObject::createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }

    VkDescriptorSetLayoutBinding lightingUboLayoutBinding{};
    lightingUboLayoutBinding.binding = 0;
    lightingUboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    lightingUboLayoutBinding.descriptorCount = 1;
    lightingUboLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    lightingUboLayoutBinding.pImmutableSamplers = nullptr;
	
    VkDescriptorSetLayoutBinding colorInputLayoutBinding0{};
    colorInputLayoutBinding0.binding = 1;
    colorInputLayoutBinding0.descriptorCount = 1;
    colorInputLayoutBinding0.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    colorInputLayoutBinding0.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding colorInputLayoutBinding1{};
    colorInputLayoutBinding1.binding = 2;
    colorInputLayoutBinding1.descriptorCount = 1;
    colorInputLayoutBinding1.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    colorInputLayoutBinding1.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding depthInputLayoutBinding1{};
    depthInputLayoutBinding1.binding = 4;
    depthInputLayoutBinding1.descriptorCount = 1;
    depthInputLayoutBinding1.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    depthInputLayoutBinding1.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 4> lightingBindings = { lightingUboLayoutBinding, colorInputLayoutBinding0, colorInputLayoutBinding1, depthInputLayoutBinding1 };
    VkDescriptorSetLayoutCreateInfo lightingLayoutInfo{};
    lightingLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    lightingLayoutInfo.bindingCount = static_cast<uint32_t>(lightingBindings.size());
    lightingLayoutInfo.pBindings = lightingBindings.data();

    if (vkCreateDescriptorSetLayout(device, &lightingLayoutInfo, nullptr, &lightingSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
    
}

void VulkanObject::createIndexBuffer() {
    VkDeviceSize bufferSize = sizeof(dragon_model.getIndices()[0]) * dragon_model.getIndices().size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, dragon_model.getIndices().data(), (size_t)bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

    copyBuffer(stagingBuffer, indexBuffer, bufferSize);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void VulkanObject::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate buffer memory!");
    }

    vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

void VulkanObject::createVertexBuffer() {
    VkDeviceSize bufferSize = sizeof(dragon_model.getVertices()[0]) * dragon_model.getVertices().size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, dragon_model.getVertices().data(), (size_t)bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

    copyBuffer(stagingBuffer, vertexBuffer, bufferSize);
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void VulkanObject::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = 0;

    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        if (hasStencilComponent(format)) {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }
    else {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else {
        throw std::invalid_argument("unsupported layout transition!");
    }

    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    endSingleTimeCommands(commandBuffer);
}

void VulkanObject::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = {
        width,
        height,
        1
    };

    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    vkCmdCopyBufferToImage(
        commandBuffer,
        buffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
    );

    endSingleTimeCommands(commandBuffer);
}

VkCommandBuffer VulkanObject::beginSingleTimeCommands() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void VulkanObject::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void VulkanObject::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(commandBuffer);
}

uint32_t VulkanObject::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

// clean up swap chain for a clean recreate
void VulkanObject::cleanupSwapChain() {
    vkDestroyImageView(device, depthImageView, nullptr);
    vkDestroyImage(device, depthImage, nullptr);
    vkFreeMemory(device, depthImageMemory, nullptr);

    // destroy all framebuffers in swap chain
    for (size_t i = 0; i < swapChainFramebuffers.size(); i++) {
        vkDestroyFramebuffer(device, imgui_frame_buffers[i], nullptr);
        vkDestroyFramebuffer(device, swapChainFramebuffers[i], nullptr);
    }

    vkFreeCommandBuffers(device, imgui_command_pool, static_cast<uint32_t>(imgui_command_buffers.size()), imgui_command_buffers.data());
    vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

    //destroy pipeline
    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    // destroy pipeline layout
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    // destroy render pass resources
    vkDestroyRenderPass(device, imgui_render_pass, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);

    // Destroy each image view we own
    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        vkDestroyImageView(device, swapChainImageViews[i], nullptr);
    }

    // destroy our swapchain
    vkDestroySwapchainKHR(device, swapChain, nullptr);

    for (size_t i = 0; i < uniformBuffers.size(); i++) {
        vkDestroyBuffer(device, uniformBuffers[i], nullptr);
        vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
    }

    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
}

void VulkanObject::cleanup() {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // cleanup swap chain
    cleanupSwapChain();

    vkDestroyImageView(device, offScreenPass.position.view, nullptr);
    vkDestroyImage(device, offScreenPass.position.image, nullptr);
    vkFreeMemory(device, offScreenPass.position.mem, nullptr);

    vkDestroyImageView(device, offScreenPass.albedo.view, nullptr);
    vkDestroyImage(device, offScreenPass.albedo.image, nullptr);
    vkFreeMemory(device, offScreenPass.albedo.mem, nullptr);

    vkDestroyImageView(device, offScreenPass.normal.view, nullptr);
    vkDestroyImage(device, offScreenPass.normal.image, nullptr);
    vkFreeMemory(device, offScreenPass.normal.mem, nullptr);

    vkDestroyImageView(device, offScreenPass.depth.view, nullptr);
    vkDestroyImage(device, offScreenPass.depth.image, nullptr);
    vkFreeMemory(device, offScreenPass.depth.mem, nullptr);

    vkDestroyFramebuffer(device, geometryFrameBuffer, nullptr);

    vkDestroyDescriptorSetLayout(device, lightingSetLayout, nullptr);
    vkDestroyRenderPass(device, geometryPass, nullptr);
    vkDestroyPipelineLayout(device, lightingLayout, nullptr);
    vkDestroyPipeline(device, lightingPipeline, nullptr);

    vkDestroyDescriptorPool(device, lightingDescriptorPool, nullptr);


    vkDestroySampler(device, textureSampler, nullptr);
    vkDestroyImageView(device, textureImageView, nullptr);

    vkDestroyImage(device, textureImage, nullptr);
    vkFreeMemory(device, textureImageMemory, nullptr);

    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

    vkDestroyBuffer(device, indexBuffer, nullptr);
    vkFreeMemory(device, indexBufferMemory, nullptr);

    vkDestroyBuffer(device, vertexBuffer, nullptr);
    vkFreeMemory(device, vertexBufferMemory, nullptr);

    // destroy all semaphores and fences
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device, inFlightFences[i], nullptr);
    }

    // destory command pool memory
    vkDestroyCommandPool(device, commandPool, nullptr);
    vkDestroyCommandPool(device, imgui_command_pool, nullptr);

    vkDestroyDescriptorPool(device, imgui_descriptor_pool, VK_NULL_HANDLE);

    // destory logical device
    vkDestroyDevice(device, nullptr);

    // if we're using validation layers we will have to clean up debug messenger
    if (enableValidationLayers) {
        // destroy debug messenger
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }

    // destory our surface BEFORE our instance
    vkDestroySurfaceKHR(instance, surface, nullptr);

    // destory our instance of vulkan using the default deallocator
    vkDestroyInstance(instance, nullptr);

    // These were created first, so we delete them last!
    // frees the memory allocated for our memory and invalidates the pointer
    glfwDestroyWindow(window);

    // frees all resources GLFW had taken up
    glfwTerminate();
}

void VulkanObject::createInstance() {
    // if we want to enable validation AND we cant support all the layers we want
    if (enableValidationLayers && !checkValidationLayerSupport()) {
        // throw an error
        throw std::runtime_error("validation layers requested, but not available!");
    }

    // struct to be filled with all information about app that vulkan needs.
    // technically optional, but could provide optimisation options to GPU
    // {} is value (zero) initialisation
    VkApplicationInfo appInfo{};
    // the type of this struct is application information
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    // the name of our application is Hello Triangle
    appInfo.pApplicationName = "Hello Triangle";
    // the version of our application is 1.0.0 (major, minor, patch)
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    // we are not using an engine at this moment in time, otherwise name here
    appInfo.pEngineName = "No Engine";
    // somewhat redundant, but our (non-existant) engine's version is 1.0.0 (major, minor, patch)
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    // we are going to use vulkan 1.0
    appInfo.apiVersion = VK_API_VERSION_1_0;

    // required struct for notifying vulkan of global validation layers and extensions to use
    VkInstanceCreateInfo createInfo{};
    // the type of this struct is instance create info
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    // reference to our application info struct above
    createInfo.pApplicationInfo = &appInfo;

    // generate vector of desired extensions
    std::vector<const char*> extensions = getRequiredExtensions();
    // add count of extensions to creatInfo as a uint32_t
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    // get vector of extensions as pointer to raw array of extension names
    createInfo.ppEnabledExtensionNames = extensions.data();


    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
    // if we are using validation layers
    if (enableValidationLayers) {
        // add count of validation layers to creatInfo as a uint32_t
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        // get vector of validation layers as pointer to raw array of validation layer names
        createInfo.ppEnabledLayerNames = validationLayers.data();

        // special case for debug messenger for before full VkInstance instanciation
        populateDebugMessengerCreateInfo(debugCreateInfo);
        // vulkan allows debug messenger to be attached through pNext before instanciation
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    }
    else {
        // do not enable any validation layers
        createInfo.enabledLayerCount = 0;

        // nothing is extending this structure in this case, so nullptr
        createInfo.pNext = nullptr;
    }

    // we try to create an instance of vulkan.
    // we pass in reference to our createInfo struct, we leave the allocator as 
    // default, and a reference to our instance variable to populate

    // if that didn't work
    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        // throw an exception
        throw std::runtime_error("failed to create instance!");
    }
}

// populate a given struct with callback info
void VulkanObject::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    // zero initialise
    createInfo = {};
    // type of struct
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    // specify which values of severity the debugger should be called for
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    // specify types of message the callback should be called for
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    // pointer to callback function
    createInfo.pfnUserCallback = debugCallback;
}

void VulkanObject::setupDebugMessenger() {
    // if we arent using validation layers, return
    if (!enableValidationLayers) return;

    // create a struct with debugger info in
    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    // populate that structure
    populateDebugMessengerCreateInfo(createInfo);

    // attempt to create debug messenger for instance
    if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
        // throw if fails
        throw std::runtime_error("failed to set up debug messenger!");
    }
}

// create our surface
void VulkanObject::createSurface() {
    // use glfw to create the window surface. This will do all of the OS specific 
    // operations that we need to get a surface and window up and running
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        // if it fails, throw error
        throw std::runtime_error("failed to create window surface!");
    }
}

void VulkanObject::pickPhysicalDevice() {
    // first we find the number of cards
    uint32_t deviceCount = 0;
    // this is the actual query
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    // no GPUs means no Vulkan!
    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    // vector of devices
    std::vector<VkPhysicalDevice> devices(deviceCount);
    // populate vector of devices
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    // for each device
    for (const auto& device : devices) {
        // check if device is suitable
        if (isDeviceSuitable(device)) {
            // if it is, assign it and break
            physicalDevice = device;
            break;
        }
    }

    // if we didn't found a device
    if (physicalDevice == VK_NULL_HANDLE) {
        // throw an error
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}

// create a logical device to use based off physical
void VulkanObject::createLogicalDevice() {
    // indicies of queues on physical device
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

    // vector of device queue info. One for each family
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    // set of our desired queue family's values
    std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    // set queue priority
    float queuePriority = 1.0f;
    // for each queue family we care about
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        // create relavent queue info struct
        VkDeviceQueueCreateInfo queueCreateInfo{};
        // assign queue infor type
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        // assign family index
        queueCreateInfo.queueFamilyIndex = queueFamily;
        // assign queue count
        queueCreateInfo.queueCount = 1;
        // assign previously set priority
        queueCreateInfo.pQueuePriorities = &queuePriority;
        // push back onto our vector of queue infos
        queueCreateInfos.push_back(queueCreateInfo);
    }

    // zero initialise device features
    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    // struct to hold device info
    VkDeviceCreateInfo createInfo{};
    // set device type
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    // number of queues
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    // pointer to queue information
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    // enabled features is set to our struct containing that information
    createInfo.pEnabledFeatures = &deviceFeatures;

    // number of extensions to enable
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    // array of extensions to enable
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    // if we are using validation layers
    if (enableValidationLayers) {
        // number of layers to enable
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        // array of layers to enable
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else {
        // otherwise just say no validation layers
        createInfo.enabledLayerCount = 0;
    }

    // try and create logical device with above data, and if fails
    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
        // throw error
        throw std::runtime_error("failed to create logical device!");
    }

    VkPhysicalDeviceProperties output_props{};
	
    vkGetPhysicalDeviceProperties(physicalDevice, &output_props);

    std::cout << output_props.apiVersion << std::endl;

    // finally, get the graphics queue handle and assign it to graphicsQueue
    vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
    // and get the presentation queue handle and assign it to presentQueue
    vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
}

// create a swap chain
void VulkanObject::createSwapChain() {
    // check that we support a swap chain, and if so, what kind
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

    // store the best surface format for us
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    // store the best presentation mode for us
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    // store the best extent for us
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    // number of images we want in the swap chain
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        // make sure we dont overflow
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    // create a swap chain struct for filling in
    VkSwapchainCreateInfoKHR createInfo{};
    // assign type to swapchain info
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    // assign our surface
    createInfo.surface = surface;

    // assign our image count
    createInfo.minImageCount = imageCount;
    // assign our chosen image format
    createInfo.imageFormat = surfaceFormat.format;
    // assign our image space
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    // assign our chosen extent (size)
    createInfo.imageExtent = extent;
    // assign the number of layers our image will have (almost always 1)
    createInfo.imageArrayLayers = 1;
    // assign what we will use the images in the swap chain for. Here, their colour.
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    // struct that holds queue families
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
    // array of queue family indicies we will use
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    // if our presentation queue is not our graphics queue
    if (indices.graphicsFamily != indices.presentFamily) {
        // make sure that the images can be used across multiple queues without special ownership transfers
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        // specify number of queues for concurrency
        createInfo.queueFamilyIndexCount = 2;
        // specify their indicies
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else {
        // otherwise ownership is explicitely required to access images
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    // we will nto being doing any special transform (rotations etc.)
    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    // ignore alpha channel
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    // assign our chosen presentation mode
    createInfo.presentMode = presentMode;
    // we don't care about obscurred pixels. Optimisation
    createInfo.clipped = VK_TRUE;

    // we presume no swap chain failure, so dont need this
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    // create the swap chain based off of our struct and assign to swapChain
    // if it fails
    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
        // throw error
        throw std::runtime_error("failed to create swap chain!");
    }

    // get the count of swap chain images
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    // resize our handle ot them
    swapChainImages.resize(imageCount);
    // fill the vector with them
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

    // store format and extent of the swap chain images for later use
    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
}

// create our image views
void VulkanObject::createImageViews() {
    // create enough space in our container for the number of images in our swap chain
    swapChainImageViews.resize(swapChainImages.size());

    for (uint32_t i = 0; i < swapChainImages.size(); i++) {
        swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

void VulkanObject::createImguiPass()
{
    ///
    ///
    /// imgui pass
    /// 
    ///

    VkAttachmentDescription imgui_attachment = {};
    imgui_attachment.format = swapChainImageFormat;
    imgui_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    imgui_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    imgui_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    imgui_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    imgui_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    imgui_attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    imgui_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference imgui_color_attachment = {};
    imgui_color_attachment.attachment = 0;
    imgui_color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription imgui_subpass = {};
    imgui_subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    imgui_subpass.colorAttachmentCount = 1;
    imgui_subpass.pColorAttachments = &imgui_color_attachment;

    VkSubpassDependency imgui_dependency = {};
    imgui_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    imgui_dependency.dstSubpass = 0;
    imgui_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    imgui_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    imgui_dependency.srcAccessMask = 0;
    imgui_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    info.attachmentCount = 1;
    info.pAttachments = &imgui_attachment;
    info.subpassCount = 1;
    info.pSubpasses = &imgui_subpass;
    info.dependencyCount = 1;
    info.pDependencies = &imgui_dependency;
    if (vkCreateRenderPass(device, &info, nullptr, &imgui_render_pass) != VK_SUCCESS) {
        throw std::runtime_error("Could not create Dear ImGui's render pass");
    }
}

void VulkanObject::createGeometryPass()
{
	
    std::array<VkAttachmentDescription, 4> attachmentDescriptions{};
    std::array<VkImageView, 4> imageViews{};

    std::array<VkAttachmentReference, 2> colorAttachmentRefs{};
	
	// color 1
    createImage(swapChainExtent.width,
        swapChainExtent.height,
        VK_FORMAT_R32G32B32A32_SFLOAT,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        offScreenPass.albedo.image,
        offScreenPass.albedo.mem);

    imageViews[0] = createImageView(offScreenPass.albedo.image, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);
    offScreenPass.albedo.view = imageViews[0];

    attachmentDescriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attachmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDescriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    colorAttachmentRefs[0].attachment = 0;
    colorAttachmentRefs[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // color 2
    createImage(swapChainExtent.width,
        swapChainExtent.height,
        VK_FORMAT_A2R10G10B10_UNORM_PACK32,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        offScreenPass.normal.image,
        offScreenPass.normal.mem);

    imageViews[1] = createImageView(offScreenPass.normal.image, VK_FORMAT_A2R10G10B10_UNORM_PACK32, VK_IMAGE_ASPECT_COLOR_BIT);
    offScreenPass.normal.view = imageViews[1];

    attachmentDescriptions[1].format = VK_FORMAT_A2R10G10B10_UNORM_PACK32;
    attachmentDescriptions[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachmentDescriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDescriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentDescriptions[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDescriptions[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentDescriptions[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachmentDescriptions[1].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    colorAttachmentRefs[1].attachment = 1;
    colorAttachmentRefs[1].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // output color
    attachmentDescriptions[2].format = swapChainImageFormat;
    attachmentDescriptions[2].samples = VK_SAMPLE_COUNT_1_BIT;
    attachmentDescriptions[2].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDescriptions[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentDescriptions[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDescriptions[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentDescriptions[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachmentDescriptions[2].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// depth
    createImage(swapChainExtent.width,
        swapChainExtent.height,
        findDepthFormat(),
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        offScreenPass.depth.image,
        offScreenPass.depth.mem);

    imageViews[imageViews.size() - 1] = createImageView(offScreenPass.depth.image, findDepthFormat(), VK_IMAGE_ASPECT_DEPTH_BIT);
    offScreenPass.depth.view = imageViews[imageViews.size() - 1];
	
    attachmentDescriptions[attachmentDescriptions.size() - 1].format = findDepthFormat();
    attachmentDescriptions[attachmentDescriptions.size() - 1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachmentDescriptions[attachmentDescriptions.size() - 1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDescriptions[attachmentDescriptions.size() - 1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentDescriptions[attachmentDescriptions.size() - 1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDescriptions[attachmentDescriptions.size() - 1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentDescriptions[attachmentDescriptions.size() - 1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachmentDescriptions[attachmentDescriptions.size() - 1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = attachmentDescriptions.size() - 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	
    std::array<VkSubpassDescription, 2> subpassDescriptions{};
    subpassDescriptions[0].flags = 0;
    subpassDescriptions[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescriptions[0].inputAttachmentCount = 0;
    subpassDescriptions[0].pInputAttachments = nullptr;
    subpassDescriptions[0].colorAttachmentCount = colorAttachmentRefs.size();
    subpassDescriptions[0].pColorAttachments = colorAttachmentRefs.data();
    subpassDescriptions[0].pResolveAttachments = nullptr;
    subpassDescriptions[0].pDepthStencilAttachment = &depthAttachmentRef;
    subpassDescriptions[0].preserveAttachmentCount = 0;
    subpassDescriptions[0].pResolveAttachments = nullptr;

    std::array<VkAttachmentReference, 3> deferredIntputAttachmentRefs{};
    deferredIntputAttachmentRefs[0].attachment = 0;
    deferredIntputAttachmentRefs[0].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    deferredIntputAttachmentRefs[1].attachment = 1;
    deferredIntputAttachmentRefs[1].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    deferredIntputAttachmentRefs[2].attachment = 3;
    deferredIntputAttachmentRefs[2].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference outputAttachmentRef{};
    outputAttachmentRef.attachment = 2;
    outputAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	
    subpassDescriptions[1].flags = 0;
    subpassDescriptions[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescriptions[1].inputAttachmentCount = deferredIntputAttachmentRefs.size();
    subpassDescriptions[1].pInputAttachments = deferredIntputAttachmentRefs.data();
    subpassDescriptions[1].colorAttachmentCount = 1;
    subpassDescriptions[1].pColorAttachments = &outputAttachmentRef;
    subpassDescriptions[1].pResolveAttachments = nullptr;
    subpassDescriptions[1].pDepthStencilAttachment = nullptr;
    subpassDescriptions[1].preserveAttachmentCount = 0;
    subpassDescriptions[1].pResolveAttachments = nullptr;

    // a dependancy. these specify memory and execution dependencies between subpasses
    std::array<VkSubpassDependency, 2> dependencies{};
    // implicit subpass before render pass
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    // this is our subpass
    dependencies[0].dstSubpass = 0;
    // the operation to wait on before we can use image
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    // no mask
    dependencies[0].srcAccessMask = 0;
    // operations which should wait on this subpass are the colour attachment
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    // implicit subpass before render pass
    dependencies[1].srcSubpass = 0;
    // this is our subpass
    dependencies[1].dstSubpass = 1;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.pNext = nullptr;
    renderPassInfo.flags = 0;
    renderPassInfo.attachmentCount = attachmentDescriptions.size();
	renderPassInfo.pAttachments = attachmentDescriptions.data();
    renderPassInfo.subpassCount = subpassDescriptions.size();
    renderPassInfo.pSubpasses = subpassDescriptions.data();
    renderPassInfo.dependencyCount = dependencies.size();
    renderPassInfo.pDependencies = dependencies.data();
    
    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &geometryPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }	
}

// create our render pass object
void VulkanObject::createRenderPass()
{
    createGeometryPass();
    createImguiPass();
}

// recreate swap chain incase it is invalidated
void VulkanObject::recreateSwapChain() {
    // if minimised
    int width = 0, height = 0;
    while (width == 0 || height == 0) {
        // minimise
        glfwGetFramebufferSize(window, &width, &height);
        // pause
        glfwWaitEvents();
    }

    ImGui_ImplVulkan_SetMinImageCount(swapChainImages.size());

    // wait for device to finish
    vkDeviceWaitIdle(device);

    // clear swap chain
    cleanupSwapChain();

    // create swap chain
    createSwapChain();
    // create image views off of swap chain
    createImageViews();
    // create render pass
    createRenderPass();
    // create graphics pipeline
    createGraphicsPipeline();
    createDepthResources();
    // create framebuffers
    createFramebuffers();

    imgui_frame_buffers.resize(swapChainImages.size());

    {
        VkImageView attachment[1];
        VkFramebufferCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        info.renderPass = imgui_render_pass;
        info.attachmentCount = 1;
        info.pAttachments = attachment;
        info.width = swapChainExtent.width;
        info.height = swapChainExtent.height;
        info.layers = 1;
        for (uint32_t i = 0; i < swapChainImages.size(); i++)
        {
            attachment[0] = swapChainImageViews[i];
            vkCreateFramebuffer(device, &info, VK_NULL_HANDLE, &imgui_frame_buffers[i]);
        }
    }

    createUniformBuffers();
    createDescriptorPool();

    ImGui_ImplVulkan_SetMinImageCount(swapChainImages.size());

    VkCommandBuffer command_buffer = beginSingleTimeCommands();
    ImGui_ImplVulkan_CreateFontsTexture(command_buffer);
    endSingleTimeCommands(command_buffer);

    createCommandPool(&imgui_command_pool, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    imgui_command_buffers.resize(swapChainImageViews.size());
    createCommandBuffers(imgui_command_buffers.data(), static_cast<uint32_t>(imgui_command_buffers.size()), imgui_command_pool);

    createDescriptorSets();
    // create command buffers
    createCommandBuffers();
}

void VulkanObject::createDescriptorSets() {
    std::vector<VkDescriptorSetLayout> layouts(swapChainImages.size(), descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(swapChainImages.size());
    allocInfo.pSetLayouts = layouts.data();

    descriptorSets.resize(swapChainImages.size());
    if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    std::vector<VkDescriptorSetLayout> lightingLayouts(swapChainImages.size(), lightingSetLayout);
    VkDescriptorSetAllocateInfo lightingAllocInfo{};
    lightingAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    lightingAllocInfo.descriptorPool = lightingDescriptorPool;
    lightingAllocInfo.descriptorSetCount = static_cast<uint32_t>(swapChainImages.size());
    lightingAllocInfo.pSetLayouts = lightingLayouts.data();

    lightingDescriptorSets.resize(swapChainImages.size());
    if (vkAllocateDescriptorSets(device, &lightingAllocInfo, lightingDescriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    for (size_t i = 0; i < swapChainImages.size(); i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = textureImageView;
        imageInfo.sampler = textureSampler;

        VkDescriptorImageInfo colorDescriptorInfo{};
        colorDescriptorInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        colorDescriptorInfo.imageView = offScreenPass.albedo.view;
        colorDescriptorInfo.sampler = VK_NULL_HANDLE;

        VkDescriptorImageInfo normalDescriptorInfo{};
        normalDescriptorInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        normalDescriptorInfo.imageView = offScreenPass.normal.view;
        normalDescriptorInfo.sampler = VK_NULL_HANDLE;

        VkDescriptorImageInfo depthDescriptorInfo{};
        depthDescriptorInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        depthDescriptorInfo.imageView = offScreenPass.depth.view;
        depthDescriptorInfo.sampler = VK_NULL_HANDLE;

        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

        std::array<VkWriteDescriptorSet, 4> lightingDescriptorWrites{};

        lightingDescriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        lightingDescriptorWrites[0].dstSet = lightingDescriptorSets[i];
        lightingDescriptorWrites[0].dstBinding = 0;
        lightingDescriptorWrites[0].dstArrayElement = 0;
        lightingDescriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        lightingDescriptorWrites[0].descriptorCount = 1;
        lightingDescriptorWrites[0].pBufferInfo = &bufferInfo;

        lightingDescriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        lightingDescriptorWrites[1].dstSet = lightingDescriptorSets[i];
        lightingDescriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        lightingDescriptorWrites[1].descriptorCount = 1;
        lightingDescriptorWrites[1].dstBinding = 1;
        lightingDescriptorWrites[1].pImageInfo = &colorDescriptorInfo;

        lightingDescriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        lightingDescriptorWrites[2].dstSet = lightingDescriptorSets[i];
        lightingDescriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        lightingDescriptorWrites[2].descriptorCount = 1;
        lightingDescriptorWrites[2].dstBinding = 2;
        lightingDescriptorWrites[2].pImageInfo = &normalDescriptorInfo;

        lightingDescriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        lightingDescriptorWrites[3].dstSet = lightingDescriptorSets[i];
        lightingDescriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        lightingDescriptorWrites[3].descriptorCount = 1;
        lightingDescriptorWrites[3].dstBinding = 4;
        lightingDescriptorWrites[3].pImageInfo = &depthDescriptorInfo;

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(lightingDescriptorWrites.size()), lightingDescriptorWrites.data(), 0, nullptr);
    }
}

// create the graphics pipeline.
void VulkanObject::createGraphicsPipeline() {
    // read in our compiled SPIR-V shaders
    auto vertShaderCode = readFile("../shaders/vulkan2/geometry_pass_vert.spv");
    auto fragShaderCode = readFile("../shaders/vulkan2/geometry_pass_frag.spv");

    // create shader module per shader
    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

    // create a shader stage info struct for the vertex shader
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    // assign it's type
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    // assign vertex shader to it's stage
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    // add the shader code
    vertShaderStageInfo.module = vertShaderModule;
    // add standard name
    vertShaderStageInfo.pName = "main";

    // create a shader stage info struct for the fragment shader
    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    // assign it's type
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    // assign fragment shader to it's stage
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    // add the shader code
    fragShaderStageInfo.module = fragShaderModule;
    // add standard name
    fragShaderStageInfo.pName = "main";

    // an array which contains both shaders for convenience 
    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    // a struct to store information about vertex data we will be passing to the vertex shader
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    // set type of struct
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    // for now, this data is hard coded

    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    // create struct to describe how geometry should be drawn. Points, lines, strips, etc.
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    // assign struct type
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    // assign drawing type
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    // we dont intend on splitting geometry up with special operations
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // struct to describe region to be rendered to
    VkViewport viewport{};
    // position in window
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    // width and height in window
    viewport.width = (float)swapChainExtent.width;
    viewport.height = (float)swapChainExtent.height;
    // range of depth values to use
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    // struct to describe scissor rectangle (for cropping the output without transforming)
    VkRect2D scissor{};
    // position in window
    scissor.offset = { 0, 0 };
    // width and ehight
    scissor.extent = swapChainExtent;

    // create viewport state struct as a combination of viewport and scissor
    VkPipelineViewportStateCreateInfo viewportState{};
    // assign type
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    // assign viewport count
    viewportState.viewportCount = 1;
    // set the viewport to be our previously created viewport
    viewportState.pViewports = &viewport;
    // assign scissor count
    viewportState.scissorCount = 1;
    // set the scissor rectangle to be our previously created scissor rectangle
    viewportState.pScissors = &scissor;

    // create struct describing the rasteriser (i will spell it english-style!)
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    // assign type
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    // fragments beyond the near and far plane are not clamped as opposed to deleted
    rasterizer.depthClampEnable = VK_FALSE;
    // we want output data from the rasteriser, so we set this false
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    // we are filling polygons we draw (so generating fragments within boundaries
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    // set the width of our boundary lines
    rasterizer.lineWidth = 1.0f;
    // we will be culling the back faces
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    // we consider vertex order to be clockwise and front facing
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    // no shadow mapping, no need for depth biasing
    rasterizer.depthBiasEnable = VK_FALSE;

    // used for anti-aliasing. struct to store info on multisampling
    VkPipelineMultisampleStateCreateInfo multisampling{};
    // assign struct type
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    // we disable it
    multisampling.sampleShadingEnable = VK_FALSE;
    // one 1 sample count
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // colour blend state per attached frame buffer
    std::array<VkPipelineColorBlendAttachmentState, 2> colorBlendAttachments{};
    // blend RGBA
    colorBlendAttachments[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    // disable
    colorBlendAttachments[0].blendEnable = VK_FALSE;
    // blend RGBA
    colorBlendAttachments[1].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    // disable
    colorBlendAttachments[1].blendEnable = VK_FALSE;

    // used for all framebuffers. allows setting of blend constants that can be used as blend factors.
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    // assign type
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    // disable logicOp
    colorBlending.logicOpEnable = VK_FALSE;
    // bitwise operation specified here
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    // number of attachments
    colorBlending.attachmentCount = colorBlendAttachments.size();
    // set as previously defined attachment
    colorBlending.pAttachments = colorBlendAttachments.data();
    // blend constants
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    // zero initialise pipeline layout struct
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    // set type
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    // set number of layouts
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
    // number of dynamic values that can be pushed ot shader
    pipelineLayoutInfo.pushConstantRangeCount = 0;

    // create pipeline layout and if failed
    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        // throw error
        throw std::runtime_error("failed to create pipeline layout!");
    }

    // create pipeline info struct
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    // assign type
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    // set number of pipeline stages (vertex and fragment in this case)
    pipelineInfo.stageCount = 2;
    // assign list of stages
    pipelineInfo.pStages = shaderStages;
    // assign vertex input info
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    // assign input assembly info
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    // assign viewport data info
    pipelineInfo.pViewportState = &viewportState;
    // assign rasteriser info
    pipelineInfo.pRasterizationState = &rasterizer;
    // assign multisampling info
    pipelineInfo.pMultisampleState = &multisampling;
    // assign colour blend info
    pipelineInfo.pColorBlendState = &colorBlending;
    // assign layout (for passing uniforms)
    pipelineInfo.layout = pipelineLayout;
    // assign renderpass
    pipelineInfo.renderPass = geometryPass;
    // number of subpasses
    pipelineInfo.subpass = 0;
    // we wont fail, so NULL
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f;
    depthStencil.maxDepthBounds = 1.0f;
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = {};
    depthStencil.back = {};

    pipelineInfo.pDepthStencilState = &depthStencil;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions = nullptr;
    vertexInputInfo.pVertexAttributeDescriptions = nullptr;

    // we will be culling the back faces
    rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;
    // we consider vertex order to be clockwise and front facing
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    pipelineLayoutInfo.pSetLayouts = &lightingSetLayout;

    // number of attachments
    colorBlending.attachmentCount = 1;
    // set as previously defined attachment
    colorBlending.pAttachments = &colorBlendAttachments[0];

    // create pipeline layout and if failed
    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &lightingLayout) != VK_SUCCESS) {
        // throw error
        throw std::runtime_error("failed to create pipeline layout!");
    }

    // destroy shader modules (they are elsewhere now)
    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);

    vertShaderCode = std::vector<char>{};
    fragShaderCode = std::vector<char>{};
	
    vertShaderCode = readFile("../shaders/vulkan2/lighting_pass_vert.spv");
    fragShaderCode = readFile("../shaders/vulkan2/lighting_pass_frag.spv");

    // create shader module per shader
    auto lightingVertShaderModule = createShaderModule(vertShaderCode);
    auto lightingFragShaderModule = createShaderModule(fragShaderCode);

    // add the shader code
    vertShaderStageInfo.module = lightingVertShaderModule;
    // add the shader code
    fragShaderStageInfo.module = lightingFragShaderModule;

    shaderStages[0] = vertShaderStageInfo;
	shaderStages[1] = fragShaderStageInfo;

    pipelineInfo.pStages = shaderStages;
    // assign layout (for passing uniforms)
    pipelineInfo.layout = lightingLayout;
    // assign renderpass
    pipelineInfo.renderPass = geometryPass;
    // number of subpasses
    pipelineInfo.subpass = 1;
    // we wont fail, so NULL
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    depthStencil.depthTestEnable = VK_FALSE;
    depthStencil.depthWriteEnable = VK_FALSE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f;
    depthStencil.maxDepthBounds = 1.0f;
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = {};
    depthStencil.back = {};

    pipelineInfo.pDepthStencilState = &depthStencil;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &lightingPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    vkDestroyShaderModule(device, lightingVertShaderModule, nullptr);
    vkDestroyShaderModule(device, lightingFragShaderModule, nullptr);
}

// function to create all of our framebuffers
void VulkanObject::createFramebuffers() {
    // resize our vector to be of adaqute size
    swapChainFramebuffers.resize(swapChainImageViews.size());

    // for each image view
    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        std::array<VkImageView, 4> attachments = {
            offScreenPass.albedo.view,
        	offScreenPass.normal.view,
            swapChainImageViews[i],
            offScreenPass.depth.view,
        };

        // struct to store frame buffer info
        VkFramebufferCreateInfo framebufferInfo{};
        // type of struct
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        // specify our render pass data
        framebufferInfo.renderPass = geometryPass;
        // our attament count
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        // our attachment
        framebufferInfo.pAttachments = attachments.data();
        // width of our frame buffer
        framebufferInfo.width = swapChainExtent.width;
        // height of our frame buffer
        framebufferInfo.height = swapChainExtent.height;
        // number of layers
        framebufferInfo.layers = 1;

        // create the pipeline!! 
        // if it fails
        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
            // throw error
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

// create our command pool
void VulkanObject::createCommandPool() {
    // find qeues we can execute commands through
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

    // struct to contain command pool info
    VkCommandPoolCreateInfo poolInfo{};
    // type of struct
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    // index for our graphics queue to run graphics commands
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    // create command pool
    // if fails
    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        // throws error
        throw std::runtime_error("failed to create command pool!");
    }
}

// create command buffers
void VulkanObject::createCommandBuffers() {
    // resize vector to store all command buffers
    commandBuffers.resize(swapChainFramebuffers.size());

    // struct to specify how to generate command buffers and fill command pool
    VkCommandBufferAllocateInfo allocInfo{};
    // struct type
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    // specify our command pool for storage
    allocInfo.commandPool = commandPool;
    // can be submitted to a queue directly
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    // number of command buffers to generate
    allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

    // create command buffers
    // if fails
    if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        // throw error
        throw std::runtime_error("failed to allocate command buffers!");
    }

    // for each command buffer generated
    for (size_t i = 0; i < commandBuffers.size(); i++) {
        // specify some info about the usage of this command buffer
        VkCommandBufferBeginInfo beginInfo{};
        // assign struct type
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        // create initial command buffer
        // if fails
        if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
            // throw error
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        // struct to specify render pass info
        VkRenderPassBeginInfo renderPassInfo{};
        // assign type
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        // assign our previously created render pass
        renderPassInfo.renderPass = geometryPass;
        // assign the current framebuffer
        renderPassInfo.framebuffer = swapChainFramebuffers[i];
        // screen space offset
        renderPassInfo.renderArea.offset = { 0, 0 };
        // width and height of render
        renderPassInfo.renderArea.extent = swapChainExtent;

        std::array<VkClearValue, 4> clearValues{};
        clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
        clearValues[1].color = { 0.0f, 0.0f, 0.0f, 1.0f };
        clearValues[2].color = { 0.0f, 0.0f, 0.0f, 1.0f };
        clearValues[3].depthStencil = { 1.0f, 0 };

        // number of clear colour
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        // clear colour value
        renderPassInfo.pClearValues = clearValues.data();

        // functions starting in vkCmd record commands. This ebgins the process
        vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        // bind the graphics pipeline we set up
        vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

        VkBuffer vertexBuffers[] = { vertexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[i], 0, nullptr);

        vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(dragon_model.getIndices().size()), 1, 0, 0, 0);

        vkCmdNextSubpass(commandBuffers[i], VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, lightingPipeline);

        vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, lightingLayout, 0, 1, &lightingDescriptorSets[i], 0, nullptr);

        vkCmdDraw(commandBuffers[i], 3, 1, 0, 0);

        // finish the render pass
        vkCmdEndRenderPass(commandBuffers[i]);

        // finish recording commands
        // if fails
        if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
            // throw error
            throw std::runtime_error("failed to record command buffer!");
        }
    }
}

void VulkanObject::createSyncObjects() {
    // resize semaphore and fence vector to appropriate sizes
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    imagesInFlight.resize(swapChainImages.size(), VK_NULL_HANDLE);

    // struct to hold information on semaphore creation
    VkSemaphoreCreateInfo semaphoreInfo{};
    // assign struct type
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    // info for help with creation of fences
    VkFenceCreateInfo fenceInfo{};
    // assign struct type
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        // create semaphores amd fence for each image
        // if fails
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
            // throw error
            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }
}

// get image from swap chain, execute command buffer, put image back in chain
void VulkanObject::drawFrame() {
    // wait for all (VK_TRUE) fences before continueing.
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    // variable to store the index of an available swap chain image
    uint32_t imageIndex;
    // aquire a swap chain image. This takes the device, swap chain, no timeout, semaphore to trigger, and the variable to store the index in
    VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    // if our swap chain is incompatible with surface
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        // recreate swap chain
        recreateSwapChain();
        return;
    }
    // if swap chain no longer matched our surface exactly
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        // throw error
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    // check if previous frame is using this image
    if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }
    // mark image as now being used by this frame
    imagesInFlight[imageIndex] = inFlightFences[currentFrame];

    updateUniformBuffer(imageIndex);

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    
    ImGui::Begin("George Tattersall | HPG: Assessment 3");

    ImGuiColorEditFlags flags = ImGuiColorEditFlags_DisplayRGB;

    ImGui::Checkbox("model", &model_stage_on);
    ImGui::Checkbox("texture", &texture_stage_on);
    ImGui::Checkbox("lighting", &lighting_stage_on);
    ImGui::SliderFloat("ambient", &dragon_model.ambient, 0.0f, 1.0f);
    ImGui::SliderFloat("diffuse", &dragon_model.diffuse, 0.0f, 1.0f);
    ImGui::SliderFloat("specular", &dragon_model.specular, 0.0f, 1.0f);
    ImGui::SliderFloat("zoom", &zoom, 10.0f, 100.0f);
    ImGui::SliderFloat("scale", &scale, 0.1f, 2.0f);
    ImGui::SliderFloat("X offset", &x_offset, -10.0f, 10.0f);
    ImGui::SliderFloat("Y offset", &y_offset, -10.0f, 10.0f);
    ImGui::SliderFloat("Z offset", &z_offset, -10.0f, 10.0f);
    ImGui::SliderFloat("X model rotation", &x_rotation, 0.0f, 2 * glm::pi<float>());
    ImGui::SliderFloat("Y model rotation", &y_rotation, 0.0f, 2 * glm::pi<float>());
    ImGui::SliderFloat("Z model rotation", &z_rotation, 0.0f, 2 * glm::pi<float>());
    ImGui::SliderFloat("Y light rotation", &y_light_rotation, 0.0f, 2 * glm::pi<float>());
    ImGui::SliderFloat("Z light rotation", &z_light_rotation, 0.0f, 2 * glm::pi<float>());
    ImGui::SliderFloat("shininess (Ns)", &dragon_model.Ns, 0.00f, 128.0f);
    ImGui::ColorEdit3("ambient (Ka)", (float*)&dragon_model.Ka[0], flags);
    ImGui::ColorEdit3("diffuse (Kd)", (float*)&dragon_model.Kd[0], flags);
    ImGui::ColorEdit3("specular (Ks)", (float*)&dragon_model.Ks[0], flags);
    ImGui::ColorEdit3("emission (Ke)", (float*)&dragon_model.Ke[0], flags);
    ImGui::RadioButton("normals", &display_mode, 0);
    ImGui::RadioButton("depth", &display_mode, 1);
    ImGui::RadioButton("specularity", &display_mode, 2);
    ImGui::RadioButton("albedo", &display_mode, 3);
    ImGui::RadioButton("composed", &display_mode, 4);
   
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();

    ImGui::Render();

    vkResetCommandBuffer(imgui_command_buffers[imageIndex], 0);

    {
        VkCommandBufferBeginInfo imgui_info = {};
        imgui_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        imgui_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(imgui_command_buffers[imageIndex], &imgui_info);
    }

    {
        VkRenderPassBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        info.renderPass = imgui_render_pass;
        info.framebuffer = imgui_frame_buffers[imageIndex];
        info.renderArea.extent.width = swapChainExtent.width;
        info.renderArea.extent.height = swapChainExtent.height;
        info.clearValueCount = 1;
        info.pClearValues = &imgui_clear_value;
        vkCmdBeginRenderPass(imgui_command_buffers[imageIndex], &info, VK_SUBPASS_CONTENTS_INLINE);
    }

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), imgui_command_buffers[imageIndex]);

    vkCmdEndRenderPass(imgui_command_buffers[imageIndex]);
    vkEndCommandBuffer(imgui_command_buffers[imageIndex]);

    std::array<VkCommandBuffer, 2> submitCommandBuffers = { commandBuffers[imageIndex], imgui_command_buffers[imageIndex] };
    // struct to hold info about queue submissions
    VkSubmitInfo submitInfo{};
    // assign type
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    // which semaphores to wait for before using image
    VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
    // what the correesponding semaphore in waitSemaphores is waiting ot do
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    // semaphore count
    submitInfo.waitSemaphoreCount = 1;
    // list of semaphores
    submitInfo.pWaitSemaphores = waitSemaphores;
    // list of what they do
    submitInfo.pWaitDstStageMask = waitStages;

    // number of command buffers
    submitInfo.commandBufferCount = static_cast<uint32_t>(submitCommandBuffers.size());
    // c array of command buffers
    submitInfo.pCommandBuffers = submitCommandBuffers.data();

    // which semaphores to signal when we are done with image
    VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
    // number of them
    submitInfo.signalSemaphoreCount = 1;
    // assign semaphores
    submitInfo.pSignalSemaphores = signalSemaphores;

    // reset state of all fences
    vkResetFences(device, 1, &inFlightFences[currentFrame]);

    // submit command buffer to graphics queue. takes array of command buffers.
    // if fails
    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
        // throw error
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    // presentation configuration struct
    VkPresentInfoKHR presentInfo{};
    // assign struct type
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    // number of semaphores to wait for before presenting
    presentInfo.waitSemaphoreCount = 1;
    // semaphores to wait for before presenting
    presentInfo.pWaitSemaphores = signalSemaphores;

    // swap chains to present to
    VkSwapchainKHR swapChains[] = { swapChain };
    // number of swap chains
    presentInfo.swapchainCount = 1;
    // swap chains
    presentInfo.pSwapchains = swapChains;

    // image indices
    presentInfo.pImageIndices = &imageIndex;

    // queue the presentation info on the presentation queue!
    result = vkQueuePresentKHR(presentQueue, &presentInfo);

    // if we failed to draw because of changes to surface
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
        framebufferResized = false;
        // recreate chain
        recreateSwapChain();
    }
    // if failed
    else if (result != VK_SUCCESS) {
        // throw error
        throw std::runtime_error("failed to present swap chain image!");
    }

    // update current frame
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanObject::updateUniformBuffer(uint32_t currentImage) {
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo{};
    glm::mat4 translation_matrix = glm::translate(glm::mat4(1.0), glm::vec3(x_offset, y_offset, z_offset));
    glm::mat4 scale_matrix = glm::scale(glm::mat4(1.0), glm::vec3(scale));
    glm::mat4 rotation_matrix = glm::rotate(x_rotation, glm::vec3(1.0, 0.0, 0.0));
    rotation_matrix *= glm::rotate(y_rotation, glm::vec3(0.0, 1.0, 0.0));
    rotation_matrix *= glm::rotate(z_rotation, glm::vec3(0.0, 0.0, 1.0));

    ubo.model = translation_matrix * rotation_matrix * scale_matrix;
    ubo.view = glm::lookAt(glm::vec3(-2.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f);
    ubo.proj[1][1] *= -1;

    ubo.light = glm::rotate(x_light_rotation, glm::vec3(1.0, 0.0, 0.0));
    ubo.light *= glm::rotate(y_light_rotation, glm::vec3(0.0, 1.0, 0.0));
    ubo.light *= glm::rotate(z_light_rotation, glm::vec3(0.0, 0.0, 1.0));

    ubo.Ns = dragon_model.Ns;
    ubo.Ka = glm::vec4(dragon_model.Ka, 1.0f);
    ubo.Kd = glm::vec4(dragon_model.Kd, 1.0f);
    ubo.Ks = glm::vec4(dragon_model.Ks, 1.0f);
    ubo.Ke = glm::vec4(dragon_model.Ke, 1.0f);
    ubo.ambient = dragon_model.ambient;
    ubo.diffuse = dragon_model.diffuse;
    ubo.specular = dragon_model.specular;

    ubo.model_stage_on = model_stage_on;
    ubo.texture_stage_on = texture_stage_on;
    ubo.lighting_stage_on = lighting_stage_on;

    ubo.display_mode = display_mode;

    ubo.win_dim = glm::vec2(swapChainExtent.width, swapChainExtent.height);

    void* data;
    vkMapMemory(device, uniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data);
    memcpy(data, &ubo, sizeof(ubo));
    vkUnmapMemory(device, uniformBuffersMemory[currentImage]);
}

// create a VkShaderModule to encapsulate our shaders
VkShaderModule VulkanObject::createShaderModule(const std::vector<char>& code) {

    // create struct to hold shader module info
    VkShaderModuleCreateInfo createInfo{};
    // assign type to struct
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    // assign the size of our shader code
    createInfo.codeSize = code.size();
    // assign the code itself
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    // declare a shader module
    VkShaderModule shaderModule;
    // create a shader module based off of our info struct
    // if it fails
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        // throw an error
        throw std::runtime_error("failed to create shader module!");
    }

    //return shader module
    return shaderModule;
}

// function for choosing the format to use from available formats
VkSurfaceFormatKHR VulkanObject::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    // for each available format
    for (const auto& availableFormat : availableFormats) {
        // if the format supports 8 bit BGRA_SRGB and SRGB non linear colour space
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            // return suitable format
            return availableFormat;
        }
    }

    // if we dont have the prefered format, just pick the first
    return availableFormats[0];
}

// function to choose a prefere presentation mode
VkPresentModeKHR VulkanObject::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {

    // for all available presentation modes
    for (const auto& availablePresentMode : availablePresentModes) {
        // if it has a FIFO mode which replaces with new frames if full
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            // select this and return
            return availablePresentMode;
        }
    }

    // otherwise default to FIFO  with blocking
    return VK_PRESENT_MODE_FIFO_KHR;
}

// function to choose a good width and height for images
VkExtent2D VulkanObject::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    // if everything is normal
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    }
    else {
        // screen coordinates are not always the same as pixels.
        // to account for this we can use glfwGetFramebufferSize
        // to get a reasonable pixel width and height to fit the
        // screen coordinate size of our window.
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        // we convert to the correct types SAFELY and then assign
        // to a vulkan type for proer processing
        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        // we now make sure that the GPU can actually write to the size we want, cropping if required
        actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

        // return final size
        return actualExtent;
    }
}

// populate swap chain struct
SwapChainSupportDetails VulkanObject::querySwapChainSupport(VkPhysicalDevice device) {
    // swap chain struct to populate
    SwapChainSupportDetails details;

    // get capabilities from a VkPhysicalDevice "device" and VkSurfaceKHR window surface "surface", outputting to our struct
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    // count of available formats
    uint32_t formatCount;
    // get format count
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    // if there are supported formats
    if (formatCount != 0) {
        // store formats in our struct
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    // count of presentation modes
    uint32_t presentModeCount;
    // get presentation mode count
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    // if there are presentation mdoes
    if (presentModeCount != 0) {
        // stroe presentation modes in our struct
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }

    // return our data struct
    return details;
}

// check a physical device to see if it is suitable
bool VulkanObject::isDeviceSuitable(VkPhysicalDevice device) {
    // struct with indicies of our queue families (if available)
    QueueFamilyIndices indices = findQueueFamilies(device);

    // check that we can support all extensions
    bool extensionsSupported = checkDeviceExtensionSupport(device);

    // bool to check if our swap chain is usable
    bool swapChainAdequate = false;
    // if we can support all extensions
    if (extensionsSupported) {
        // check that we have at least one image format and presentation mode to use
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

    // if we have all queues, extension support and swap chain support
    return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}

// check that our device has support for the set of extensions we are interested in
bool VulkanObject::checkDeviceExtensionSupport(VkPhysicalDevice device) {
    // number of extensions
    uint32_t extensionCount;
    // assign to variable
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    // vector of available device extensions
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    // populate vector
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    // use set for easy validation
    // set of our required extensions
    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    // if our device supports them
    for (const auto& extension : availableExtensions) {
        // remove from set
        requiredExtensions.erase(extension.extensionName);
    }

    // return true iff all supported
    return requiredExtensions.empty();
}

// search for queue family support
QueueFamilyIndices VulkanObject::findQueueFamilies(VkPhysicalDevice device) {
    // struct to hold queue family data
    QueueFamilyIndices indices;

    // sotre number of queue families available
    uint32_t queueFamilyCount = 0;
    // assign to that value
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    // vector to store queue families
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    // populate that vector
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    // loop over all available queue families
    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        // if queue family contains graphics flag
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            // assign that queue index to our queue struct
            indices.graphicsFamily = i;
        }

        // check whether we support drawing to surface
        VkBool32 presentSupport = false;
        // actually perform check
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

        // if we can
        if (presentSupport) {
            // set index that we can
            indices.presentFamily = i;
        }

        // early exit check on queue struct that we have found all
        if (indices.isComplete()) {
            break;
        }

        i++;
    }

    return indices;
}

// return required list of extensions
std::vector<const char*> VulkanObject::getRequiredExtensions() {
    // count of GLFW extensions
    uint32_t glfwExtensionCount = 0;
    // C style array of GLFW extensions
    const char** glfwExtensions;

    // GLFW provides this function that returns the extensions required for the interface between vulkan and itself
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    // create vector of extensions
    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    // is we have enabled validation layers (debug)
    if (enableValidationLayers) {
        // add VK_EXT_DEBUG_UTILS_EXTENSION_NAME extension
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    // return the vector of desired extensions
    return extensions;
}

// check whether all requested layers are available. return true iff they are
bool VulkanObject::checkValidationLayerSupport() {
    // number of layers available
    uint32_t layerCount;
    // first get the layercount
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    // vector to store all available layers
    std::vector<VkLayerProperties> availableLayers(layerCount);
    // populate vector with all available layers
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    // for each layer in the available layers
    for (const char* layerName : validationLayers) {
        // bool to store if current layer is found
        bool layerFound = false;

        // for each property in a given layer
        for (const auto& layerProperties : availableLayers) {
            // check if the layer we want is in the availbale layers
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                // if found, set true
                layerFound = true;
                // early break
                break;
            }
        }

        // if we are missing ANY layer
        if (!layerFound) {
            // return false for whole function
            return false;
        }
    }

    // If we are here we have all layers, return true
    return true;
}
