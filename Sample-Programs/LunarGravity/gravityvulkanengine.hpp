/*
 * LunarGravity - lgvulkanengine.hpp
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
 */

#pragma once

#include <vulkan/vulkan.h>
#include <vulkan/vk_layer.h>

#include "gravitygfxengine.hpp"

struct VulkanString;

#define MAX_NUM_BACK_BUFFERS 4

struct GraivtySwapchainImage {
    VkImage image;
    VkImageView image_view;
    VkFence fence;
};

struct GravitySwapchainSurface {
    VkFormat format;
    VkColorSpaceKHR color_space;
    uint32_t frame_index;
    VkSemaphore image_acquired_semaphores[MAX_NUM_BACK_BUFFERS];
    VkSemaphore draw_complete_semaphores[MAX_NUM_BACK_BUFFERS];
    VkSemaphore image_ownership_semaphores[MAX_NUM_BACK_BUFFERS];
    VkFence fences[MAX_NUM_BACK_BUFFERS];
    VkSwapchainKHR vk_swapchain;
    std::vector<GraivtySwapchainImage> swapchain_images;
};

struct GravityDepthStencilSurface{
    VkFormat format;
    VkImage image;
    VkDeviceMemory dev_memory;
    VkImageView image_view;
};

struct GravityCmdBuffer {
    bool recording;
    VkCommandBuffer vk_cmd_buf;
    std::vector<GravityCmdBuffer> child_cmd_bufs;
};

class GravityVulkanEngine : public GravityGraphicsEngine {
    public:

        // Create a protected constructor
        GravityVulkanEngine(const std::string &app_name, uint16_t app_version, bool validate,
                            GravityClock *gravity_clock, GravityWindow *gravity_window,
                            uint32_t num_backbuffers);

        // We don't want any copy constructors
        GravityVulkanEngine(const GravityVulkanEngine &gfx_engine) = delete;
        GravityVulkanEngine &operator=(const GravityVulkanEngine &gfx_engine) = delete;

        // Make the destructor public
        virtual ~GravityVulkanEngine();

        bool SetupInitalGraphicsDevice();

    protected:
 
    private:

        bool QueryWindowSystem(std::vector<VkExtensionProperties> &ext_props,
                               uint32_t &ext_count, const char** desired_extensions);
        int CompareGpus(VkPhysicalDeviceProperties &gpu_0, VkPhysicalDeviceProperties &gpu_1);

        bool SetupSwapchain(GravityLogger &logger);
        void CleanupSwapchain();

        bool SetupDepthStencilSurface(GravityLogger &logger);
        void CleanupDepthStencilSurface();

        // Vulkan Instance items
        VkInstance m_vk_inst;
        bool m_validation_enabled;
        bool m_debug_enabled;
        VkLayerInstanceDispatchTable m_vk_inst_dispatch_table;
        VkDebugReportCallbackEXT m_dbg_report_callback;

        // Swapchain surface info
        GravitySwapchainSurface m_swapchain_surface;

        // Depth/stencil info
        GravityDepthStencilSurface m_depth_stencil_surface;

        // Vulkan Physical Device items
        VkPhysicalDevice m_vk_phys_dev;
        bool m_separate_present_queue;
        uint32_t m_graphics_queue_family_index;
        uint32_t m_present_queue_family_index;

        // Vulkan Logical Device items
        VkDevice m_vk_device;
        VkLayerDispatchTable m_vk_dev_dispatch_table;
        VkPhysicalDeviceMemoryProperties m_vk_dev_mem_props;

        // Command buffer information
        VkCommandPool m_vk_cmd_pool;
        std::vector<GravityCmdBuffer> m_primary_cmd_buffers;
};
