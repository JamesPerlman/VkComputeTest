//
//  VulkanComputeApplication.hpp
//  VkComputeTest
//
//  Created by James Perlman on 10/23/21.
//

#ifndef VulkanComputeApplication_hpp
#define VulkanComputeApplication_hpp

#include <optional>
#include <vector>
#include <vulkan/vulkan.h>


class VulkanComputeApplication {
public:
    VulkanComputeApplication();
    ~VulkanComputeApplication();
    
    void run();
    
private:
    
    VkInstance                  instance;
    VkDebugUtilsMessengerEXT    debugMessenger;
    uint32_t                    computeQueueFamilyIndex;
    VkPhysicalDevice            physicalDevice = VK_NULL_HANDLE;
    VkDevice                    logicalDevice;
    VkQueue                     computeQueue;
    VkDeviceMemory              deviceMemory;
    VkBuffer                    inputBuffer;
    VkBuffer                    outputBuffer;
    VkShaderModule              shaderModule;
    VkDescriptorSetLayout       descriptorSetLayout;
    VkPipelineLayout            pipelineLayout;
    VkPipeline                  pipeline;
    VkDescriptorPool            descriptorPool;
    VkDescriptorSet             descriptorSet;
    VkCommandPool               commandPool;
    VkCommandBuffer             commandBuffer;
    
    // Instance methods
    void createVulkanInstance();
    void destroyVulkanInstance();
    
    void createDebugMessenger();
    void destroyDebugMessenger();
    
    void assignPhysicalDevice();
    
    void createLogicalDevice();
    void destroyLogicalDevice();
    
    void createDeviceMemory();
    void destroyDeviceMemory();
    
    void createStorageBuffers();
    void destroyStorageBuffers();
    
    void createShaderModule();
    void destroyShaderModule();
    
    void createDescriptorSetLayout();
    void destroyDescriptorSetLayout();
    
    void createPipelineLayout();
    void destroyPipelineLayout();
    
    void createPipeline();
    void destroyPipeline();
    
    void createDescriptorPools();
    void destroyDescriptorPools();
    
    void createDescriptorSets();
    void destroyDescriptorSets();
    
    void createCommandPool();
    void destroyCommandPool();
    
    void createCommandBuffer();
    void destroyCommandBuffer();
    
    void recordCommandBuffer();
    
    void submitComputeQueue();
    
};


#endif /* VulkanComputeApplication_hpp */
