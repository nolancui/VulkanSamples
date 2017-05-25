/*
 * LunarGravity - gravityvulkanengine.hpp
 *
 * Copyright (C) 2017 LunarG, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Author: Mark Young <marky@lunarg.com>
 */

#pragma once

#include <vulkan/vulkan.h>
#include <vulkan/vk_layer.h>

#include "gravitygfxengine.hpp"
#include "gravitylogger.hpp"

struct VulkanString;

#define MAX_NUM_BACK_BUFFERS 4

struct GravityQueue {
    VkQueue vk_queue;
    uint32_t family_index;
};

struct GravityCmdBuffer {
    bool recording;
    VkCommandBuffer vk_cmd_buf;
    std::vector<GravityCmdBuffer> child_cmd_bufs;
};

struct GraivtySwapchainImage {
    VkImage image;
    VkImageView image_view;
    VkFence fence;
    VkCommandBuffer vk_present_cmd_buf;
};

struct GravitySwapchainSurface {
    VkFormat format;
    VkColorSpaceKHR color_space;
    uint32_t frame_index;
    uint32_t cur_framebuffer;
    VkPresentModeKHR present_mode;
    VkSemaphore image_acquired_semaphores[MAX_NUM_BACK_BUFFERS];
    VkSemaphore draw_complete_semaphores[MAX_NUM_BACK_BUFFERS];
    VkSemaphore image_ownership_semaphores[MAX_NUM_BACK_BUFFERS];
    VkFence fences[MAX_NUM_BACK_BUFFERS];
    VkSwapchainKHR vk_swapchain;
    std::vector<GraivtySwapchainImage> swapchain_images;
};

struct GravityDepthStencilSurface {
    VkFormat format;
    VkImage image;
    VkDeviceMemory dev_memory;
    VkImageView image_view;
};

class GravityVulkanEngine : public GravityGraphicsEngine {
   public:
    // Create a protected constructor
    GravityVulkanEngine();

    // We don't want any copy constructors
    GravityVulkanEngine(const GravityVulkanEngine &gfx_engine) = delete;
    GravityVulkanEngine &operator=(const GravityVulkanEngine &gfx_engine) = delete;

    // Make the destructor public
    virtual ~GravityVulkanEngine();

    virtual bool Init(std::vector<std::string> &arguments);
    virtual void AppendUsageString(std::string &usage);

    virtual bool SetupInitalGraphicsDevice();
    virtual bool BeginDrawFrame();
    virtual bool EndDrawFrame();

   protected:
    bool QueryWindowSystem(std::vector<VkExtensionProperties> &ext_props, uint32_t &ext_count, const char **desired_extensions);
    int CompareGpus(VkPhysicalDeviceProperties &gpu_0, VkPhysicalDeviceProperties &gpu_1);

    bool SetupSwapchain(GravityLogger &logger);
    void CleanupSwapchain();

    bool SetupDepthStencilSurface(GravityLogger &logger);
    void CleanupDepthStencilSurface();

    // Vulkan Instance items
    VkInstance m_vk_inst;
    bool m_validation_enabled;
    VkLayerInstanceDispatchTable m_vk_inst_dispatch_table;
    VkDebugReportCallbackEXT m_dbg_report_callback;

    // Swapchain surface info
    GravitySwapchainSurface m_swapchain_surface;

    // Depth/stencil info
    GravityDepthStencilSurface m_depth_stencil_surface;

    // Vulkan Physical Device items
    VkPhysicalDevice m_vk_phys_dev;
    bool m_separate_present_queue;
    GravityQueue m_graphics_queue;
    GravityQueue m_present_queue;

    // Vulkan Logical Device items
    VkDevice m_vk_device;
    VkLayerDispatchTable m_vk_dev_dispatch_table;
    VkPhysicalDeviceMemoryProperties m_vk_dev_mem_props;

    // Command buffer information
    VkCommandPool m_vk_graphics_cmd_pool;
    GravityCmdBuffer m_graphics_cmd_buffer;
    VkCommandPool m_vk_present_cmd_pool;
};
