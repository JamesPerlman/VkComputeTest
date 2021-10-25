//
//  VulkanComputeApplication.cpp
//  VkComputeTest
//
//  Created by James Perlman on 10/23/21.
//

#include <set>

#include "VulkanComputeApplication.hpp"

#include "FileUtils.hpp"
#include "VulkanDebugUtils.hpp"

#define VK_ASSERT_SUCCESS(result, message) if (result != VK_SUCCESS) { throw std::runtime_error(message); }

// MARK: - Constructor

VulkanComputeApplication::VulkanComputeApplication()
{
    createVulkanInstance();
    createDebugMessenger();
    assignPhysicalDevice();
    createLogicalDevice();
    createDeviceMemory();
    createStorageBuffers();
    createShaderModule();
    createDescriptorSetLayout();
    createPipelineLayout();
    createPipeline();
    createDescriptorPools();
    createDescriptorSets();
    createCommandPool();
    createCommandBuffer();
    recordCommandBuffer();
}

// MARK: - Destructor

VulkanComputeApplication::~VulkanComputeApplication()
{
    destroyCommandBuffer();
    destroyCommandPool();
    destroyDescriptorSets();
    destroyDescriptorPools();
    destroyPipeline();
    destroyPipelineLayout();
    destroyDescriptorSetLayout();
    destroyShaderModule();
    destroyStorageBuffers();
    destroyDeviceMemory();
    destroyLogicalDevice();
    destroyDebugMessenger();
    destroyVulkanInstance();
}

// MARK: - Run

void VulkanComputeApplication::run()
{
    submitComputeQueue();
}

// MARK: - Vulkan Instance

const std::vector<const char*> baseInstanceExtensions = {
    VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
};

std::vector<const char*> getRequiredInstanceExtensionNames()
{
    std::vector<const char*> extensions(baseInstanceExtensions.begin(), baseInstanceExtensions.end());
    
    if (VulkanDebugUtils::isValidationEnabled())
    {
        extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    
    return extensions;
}

void VulkanComputeApplication::createVulkanInstance()
{
    if (VulkanDebugUtils::isValidationEnabled() && !VulkanDebugUtils::isValidationSupported())
    {
        throw std::runtime_error("Validation layers requested, but not available!");
    }
    
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;
    
    auto requiredExtensionNames = getRequiredInstanceExtensionNames();
    
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensionNames.size());
    createInfo.ppEnabledExtensionNames = requiredExtensionNames.data();
    
    // Add a debugger to the instance, if enabled.
    auto debugCreateInfo = VulkanDebugUtils::getDebugMessengerCreateInfo();
    
    if (VulkanDebugUtils::isValidationEnabled())
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(VulkanDebugUtils::validationLayers.size());
        createInfo.ppEnabledLayerNames = VulkanDebugUtils::validationLayers.data();
        createInfo.pNext = &debugCreateInfo;
    } else
    {
        createInfo.enabledLayerCount = 0;
    }
    
    // Create the instance!
    VK_ASSERT_SUCCESS(vkCreateInstance(&createInfo, nullptr, &instance),
                      "failed to create Vulkan instance!");
}

void VulkanComputeApplication::destroyVulkanInstance()
{
    vkDestroyInstance(instance, nullptr);
}


// MARK: - Debug Messenger
void VulkanComputeApplication::createDebugMessenger()
{
    if (!VulkanDebugUtils::isValidationEnabled())
    {
        return;
    }
    
    auto createInfo = VulkanDebugUtils::getDebugMessengerCreateInfo();
    VK_ASSERT_SUCCESS(VulkanDebugUtils::createDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger),
                      "Failed to set up debug messenger!");
}

void VulkanComputeApplication::destroyDebugMessenger()
{
    if (!VulkanDebugUtils::isValidationEnabled())
    {
        return;
    }
    
    VulkanDebugUtils::destroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
}

// MARK: - Physical Device

const std::vector<const char*> deviceExtensions = {
    "VK_KHR_portability_subset",
};

const std::vector<const char*> requiredDeviceExtensions = {
    // No special extensions for compute
};

bool isPhysicalDeviceExtensionSupportAdequate(VkPhysicalDevice device)
{
    if (requiredDeviceExtensions.size() == 0)
    {
        return true;
    }
    
    // Fetch all device extension properties.
    uint32_t extensionPropertiesCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionPropertiesCount, nullptr);
    
    std::vector<VkExtensionProperties> deviceExtensionProperties(extensionPropertiesCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionPropertiesCount, deviceExtensionProperties.data());
    
    // Create a set from the requiredExtensionNames vector.
    std::set<const char*> requiredExtensionSet(requiredDeviceExtensions.begin(), requiredDeviceExtensions.end());
    
    // Iterate through the deviceExtensionProperties, removing device extension names from the requiredExtensionNamesSet.
    for (const auto& extension : deviceExtensionProperties)
    {
        requiredExtensionSet.erase(extension.extensionName);
    }
    
    // If requiredExtensionNamesSet is empty, then the device fully supports all required extensions.
    return requiredExtensionSet.empty();
}

std::optional<uint32_t> getComputeQueueFamilyIndex(VkPhysicalDevice physicalDevice)
{
    uint32_t queueFamilyPropertiesCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertiesCount, nullptr);
    
    std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyPropertiesCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertiesCount, queueFamilyProperties.data());
    
    for (uint32_t i = 0; i < queueFamilyPropertiesCount; ++i)
    {
        if (queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
        {
            return i;
        }
        
        ++i;
    }
    
    return NULL;
}

bool isPhysicalDeviceSuitable(VkPhysicalDevice device)
{
    // TODO: Check for optimal device, not just the first one with the Compute capability.
     VkPhysicalDeviceProperties properties;
     vkGetPhysicalDeviceProperties(device, &properties);
     
     VkPhysicalDeviceFeatures features;
     vkGetPhysicalDeviceFeatures(device, &features);
  
    auto allRequiredExtensionsSupported = isPhysicalDeviceExtensionSupportAdequate(device);
    
    auto computeQueueFamilyIndex = getComputeQueueFamilyIndex(device);
    
    return allRequiredExtensionsSupported && computeQueueFamilyIndex.has_value();
}

void VulkanComputeApplication::assignPhysicalDevice()
{
    // Find out how many devices there are.
    uint32_t physicalDeviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);
    
    // No devices? Throw an error. :(
    if (physicalDeviceCount == 0)
    {
        throw std::runtime_error("Failed to find any GPUs with Vulkan support!");
    }
    
    // Fetch all of the physical devices
    std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
    vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data());
    
    for (const auto& device : physicalDevices)
    {
        
        // TODO: Find the most suitable device, not just the first
        if (isPhysicalDeviceSuitable(device))
        {
            physicalDevice = device;
            break;
        }
    }
    
    if (physicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Failed to find a suitable GPU!");
    }
    computeQueueFamilyIndex = getComputeQueueFamilyIndex(physicalDevice).value();
}

// MARK: - Logical Device

void VulkanComputeApplication::createLogicalDevice()
{
    float queuePriority = 1.0f;
    
    VkDeviceQueueCreateInfo deviceQueueCreateInfo{};
    deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    deviceQueueCreateInfo.queueFamilyIndex = computeQueueFamilyIndex;
    deviceQueueCreateInfo.queueCount = 1;
    deviceQueueCreateInfo.pQueuePriorities = &queuePriority;
    
    VkPhysicalDeviceFeatures deviceFeatures{};
    
    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
    
    if (VulkanDebugUtils::isValidationEnabled())
    {
        deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(VulkanDebugUtils::validationLayers.size());
        deviceCreateInfo.ppEnabledLayerNames = VulkanDebugUtils::validationLayers.data();
    } else
    {
        deviceCreateInfo.enabledLayerCount = 0;
    }
    
    VK_ASSERT_SUCCESS(vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &logicalDevice),
                      "Failed to create logical device!");
    
    vkGetDeviceQueue(logicalDevice, computeQueueFamilyIndex, 0, &computeQueue);
}

void VulkanComputeApplication::destroyLogicalDevice()
{
    vkDestroyDevice(logicalDevice, nullptr);
}

// MARK: - Device Memory

void VulkanComputeApplication::createDeviceMemory()
{
    // TODO: Figure out how to do this dynamically and repeatably for different memory sizes.
    VkDeviceSize memorySize = 2048;
    
    // Fetch device memory properties.
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
    
    uint32_t memoryTypeIndex = VK_MAX_MEMORY_TYPES;
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
    {
        auto memoryType = memoryProperties.memoryTypes[i];
        
        if (memoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
            && memoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            && memorySize <= memoryProperties.memoryHeaps[memoryType.heapIndex].size)
        {
            memoryTypeIndex = i;
        }
    }
    
    if (memoryTypeIndex == VK_MAX_MEMORY_TYPES)
    {
        throw std::runtime_error("Failed to find suitable memory!");
    }
    
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.pNext = nullptr;
    allocInfo.allocationSize = memorySize;
    allocInfo.memoryTypeIndex = memoryTypeIndex;
    
    VK_ASSERT_SUCCESS(vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &deviceMemory),
                      "Failed to allocate device memory!");
}

void VulkanComputeApplication::destroyDeviceMemory()
{
    vkFreeMemory(logicalDevice, deviceMemory, nullptr);
}

// MARK: - Buffers

void VulkanComputeApplication::createStorageBuffers()
{
    // TODO: change this
    VkDeviceSize bufferSize = 1024;
    
    VkBufferCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    createInfo.size = bufferSize;
    createInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 1;
    createInfo.pQueueFamilyIndices = &computeQueueFamilyIndex;
    
    VK_ASSERT_SUCCESS(vkCreateBuffer(logicalDevice, &createInfo, nullptr, &inputBuffer),
                      "Failed to create input buffer!");
    
    vkBindBufferMemory(logicalDevice, inputBuffer, deviceMemory, 0);
    
    VK_ASSERT_SUCCESS(vkCreateBuffer(logicalDevice, &createInfo, nullptr, &outputBuffer),
                      "Failed to create input buffer!");
    
    vkBindBufferMemory(logicalDevice, outputBuffer, deviceMemory, bufferSize);
    
    
}

void VulkanComputeApplication::destroyStorageBuffers()
{
    vkDestroyBuffer(logicalDevice, inputBuffer, nullptr);
    vkDestroyBuffer(logicalDevice, outputBuffer, nullptr);
}

// MARK: - Shader Modules
void VulkanComputeApplication::createShaderModule()
{
    auto computeShaderCode = FileUtils::readLocalFile("shaders/simple.comp");
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = computeShaderCode.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(computeShaderCode.data());
    
    VK_ASSERT_SUCCESS(vkCreateShaderModule(logicalDevice, &createInfo, nullptr, &shaderModule),
                      "Failed to create shader module!");
}

void VulkanComputeApplication::destroyShaderModule()
{
    vkDestroyShaderModule(logicalDevice, shaderModule, nullptr);
}

// MARK: - Descriptor Set Layout
void VulkanComputeApplication::createDescriptorSetLayout()
{
    VkDescriptorSetLayoutBinding inputLayoutBinding{};
    inputLayoutBinding.binding = 0;
    inputLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    inputLayoutBinding.descriptorCount = 1;
    inputLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    inputLayoutBinding.pImmutableSamplers = nullptr;
    
    VkDescriptorSetLayoutBinding outputLayoutBinding{};
    outputLayoutBinding.binding = 1;
    outputLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    outputLayoutBinding.descriptorCount = 1;
    outputLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    outputLayoutBinding.pImmutableSamplers = nullptr;
    
    VkDescriptorSetLayoutBinding descriptorSetLayoutBindings[2] = {
        inputLayoutBinding,
        outputLayoutBinding,
    };
    
    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
    descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutCreateInfo.pNext = nullptr;
    descriptorSetLayoutCreateInfo.flags = 0;
    descriptorSetLayoutCreateInfo.bindingCount = 2;
    descriptorSetLayoutCreateInfo.pBindings = descriptorSetLayoutBindings;
    
    VK_ASSERT_SUCCESS(vkCreateDescriptorSetLayout(logicalDevice, &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayout),
                      "Failed to create descriptor set layout!");
}

void VulkanComputeApplication::destroyDescriptorSetLayout()
{
    vkDestroyDescriptorSetLayout(logicalDevice, descriptorSetLayout, nullptr);
}

// MARK: - Pipeline Layout

void VulkanComputeApplication::createPipelineLayout()
{
    // Create pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.pNext = nullptr;
    pipelineLayoutCreateInfo.flags = 0;
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
    
    VK_ASSERT_SUCCESS(vkCreatePipelineLayout(logicalDevice, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout),
                      "Failed to create pipeline layout!");
}

void VulkanComputeApplication::destroyPipelineLayout()
{
    vkDestroyPipelineLayout(logicalDevice, pipelineLayout, nullptr);
}

// MARK: - Compute Pipeline

void VulkanComputeApplication::createPipeline()
{
    // Create shader stage
    VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo{};
    pipelineShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineShaderStageCreateInfo.pNext = nullptr;
    pipelineShaderStageCreateInfo.flags = 0;
    pipelineShaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipelineShaderStageCreateInfo.module = shaderModule;
    pipelineShaderStageCreateInfo.pName = "main";
    pipelineShaderStageCreateInfo.pSpecializationInfo = nullptr;
    
    // Create pipeline
    VkComputePipelineCreateInfo pipelineCreateInfo{};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.pNext = nullptr;
    pipelineCreateInfo.flags = 0;
    pipelineCreateInfo.stage = pipelineShaderStageCreateInfo;
    pipelineCreateInfo.layout = pipelineLayout;
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineCreateInfo.basePipelineIndex = 0;
    
    VK_ASSERT_SUCCESS(vkCreateComputePipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline),
                      "Failed to create compute pipeline!");
}

void VulkanComputeApplication::destroyPipeline()
{
    vkDestroyPipeline(logicalDevice, pipeline, nullptr);
}

// MARK: - Descriptor Pools
void VulkanComputeApplication::createDescriptorPools()
{
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSize.descriptorCount = 2;
    
    VkDescriptorPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    createInfo.maxSets = 1;
    createInfo.poolSizeCount = 1;
    createInfo.pPoolSizes = &poolSize;
    
    VK_ASSERT_SUCCESS(vkCreateDescriptorPool(logicalDevice, &createInfo, nullptr, &descriptorPool),
                      "Failed to create descriptor pool!");
}

void VulkanComputeApplication::destroyDescriptorPools()
{
    vkDestroyDescriptorPool(logicalDevice, descriptorPool, nullptr);
}

// MARK: - Descriptor Sets
void VulkanComputeApplication::createDescriptorSets()
{
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.pNext = nullptr;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptorSetLayout;
    
    VK_ASSERT_SUCCESS(vkAllocateDescriptorSets(logicalDevice, &allocInfo, &descriptorSet),
                      "Failed to allocate descriptor set!");
    
    // Now we need to update the descriptor sets with input/output buffer info
    
    // Input
    
    VkDescriptorBufferInfo inputBufferInfo{};
    inputBufferInfo.buffer = inputBuffer;
    inputBufferInfo.offset = 0;
    inputBufferInfo.range = VK_WHOLE_SIZE;
    
    VkWriteDescriptorSet inputWriteDescriptorSet{};
    inputWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    inputWriteDescriptorSet.pNext = nullptr;
    inputWriteDescriptorSet.dstSet = descriptorSet;
    inputWriteDescriptorSet.dstBinding = 0;
    inputWriteDescriptorSet.dstArrayElement = 0;
    inputWriteDescriptorSet.descriptorCount = 1;
    inputWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    inputWriteDescriptorSet.pImageInfo = nullptr;
    inputWriteDescriptorSet.pBufferInfo = &inputBufferInfo;
    inputWriteDescriptorSet.pTexelBufferView = nullptr;
    
    // Output
    
    VkDescriptorBufferInfo outputBufferInfo{};
    outputBufferInfo.buffer = outputBuffer;
    outputBufferInfo.offset = 0;
    outputBufferInfo.range = VK_WHOLE_SIZE;
    
    VkWriteDescriptorSet outputWriteDescriptorSet{};
    outputWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    outputWriteDescriptorSet.pNext = nullptr;
    outputWriteDescriptorSet.dstSet = descriptorSet;
    outputWriteDescriptorSet.dstBinding = 1;
    outputWriteDescriptorSet.dstArrayElement = 0;
    outputWriteDescriptorSet.descriptorCount = 1;
    outputWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    outputWriteDescriptorSet.pImageInfo = nullptr;
    outputWriteDescriptorSet.pBufferInfo = &outputBufferInfo;
    outputWriteDescriptorSet.pTexelBufferView = nullptr;
    
    VkWriteDescriptorSet writeDescriptorSets[2] = {
        inputWriteDescriptorSet,
        outputWriteDescriptorSet,
    };
    
    vkUpdateDescriptorSets(logicalDevice, 2, writeDescriptorSets, 0, nullptr);
}

void VulkanComputeApplication::destroyDescriptorSets()
{
    // It is invalid to call vkFreeDescriptorSets() with a pool created without setting VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT.
    // vkFreeDescriptorSets(logicalDevice, descriptorPool, 1, &descriptorSet);
}

// MARK: - Command Pool

void VulkanComputeApplication::createCommandPool()
{
    VkCommandPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    createInfo.queueFamilyIndex = computeQueueFamilyIndex;
    
    VK_ASSERT_SUCCESS(vkCreateCommandPool(logicalDevice, &createInfo, nullptr, &commandPool),
                      "Failed to create command pool!");
}

void VulkanComputeApplication::destroyCommandPool()
{
    vkDestroyCommandPool(logicalDevice, commandPool, nullptr);
}

// MARK: - Command Buffer

void VulkanComputeApplication::createCommandBuffer()
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.pNext = nullptr;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    
    VK_ASSERT_SUCCESS(vkAllocateCommandBuffers(logicalDevice, &allocInfo, &commandBuffer),
                      "Failed to allocate command buffer!");
}

void VulkanComputeApplication::destroyCommandBuffer()
{
    vkFreeCommandBuffers(logicalDevice, commandPool, 1, &commandBuffer);
}

void VulkanComputeApplication::recordCommandBuffer()
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.pNext = nullptr;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    beginInfo.pInheritanceInfo = nullptr;
    
    VK_ASSERT_SUCCESS(vkBeginCommandBuffer(commandBuffer, &beginInfo),
                      "Failed to begin command buffer!");
    
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
    
    vkCmdDispatch(commandBuffer, 32, 32, 1);
    
    VK_ASSERT_SUCCESS(vkEndCommandBuffer(commandBuffer),
                      "Failed to end command buffer!");
}

// MARK: Submit Compute Queue

void VulkanComputeApplication::submitComputeQueue()
{
    // TODO: Use semaphores and fences
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.pWaitSemaphores = nullptr;
    submitInfo.pWaitDstStageMask = nullptr;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.signalSemaphoreCount = 0;
    submitInfo.pSignalSemaphores = nullptr;
    
    VK_ASSERT_SUCCESS(vkQueueSubmit(computeQueue, 1, &submitInfo, VK_NULL_HANDLE),
                      "Failed to submit compute queue!");
    
    VK_ASSERT_SUCCESS(vkQueueWaitIdle(computeQueue),
                      "Failed to wait for compute queue idle!");
    
    void* payload;
    VK_ASSERT_SUCCESS(vkMapMemory(logicalDevice, deviceMemory, 0, 2048, 0, &payload),
                      "Failed to map memory!");
    
    
    vkUnmapMemory(logicalDevice, deviceMemory);
    
}
