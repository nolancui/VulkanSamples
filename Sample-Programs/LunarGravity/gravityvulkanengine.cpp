/*
 * LunarGravity - gravityvulkanengine.cpp
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

#include <iostream>
#include <sstream>
#include <cstdlib>
#include <vector>
#include <cstring>

#include "vk_dispatch_table_helper.h"

#include "gravitylogger.hpp"
#include "gravityvulkanengine.hpp"
#include "gravitywindow.hpp"
#include "gravityclock.hpp"

#ifdef _WIN32
//#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined __ANDROID__
#else
#include <stdlib.h>
#endif

#define ENGINE_NAME "Lunar Gravity Graphics Engine"
#define ENGINE_VERSION 1

GravityVulkanEngine::GravityVulkanEngine(const std::string &app_name, uint16_t app_version, bool validate,
                                         GravityClock *gravity_clock, GravityWindow *gravity_window, uint32_t num_backbuffers)
    : GravityGraphicsEngine(app_name, app_version, validate, gravity_clock, gravity_window, num_backbuffers) {
    VkApplicationInfo vk_app_info = {};
    VkInstanceCreateInfo vk_inst_create_info = {};
    VkResult vk_result;
    uint32_t count = 0;
    std::vector<VkLayerProperties> layer_properties;
    std::vector<VkExtensionProperties> extension_properties;
    uint32_t enable_extension_count = 0;
    const char *extensions_to_enable[VK_MAX_EXTENSION_NAME_SIZE];
    uint32_t enable_layer_count = 0;
    const char *layers_to_enable[VK_MAX_EXTENSION_NAME_SIZE];
    GravityLogger &logger = GravityLogger::getInstance();

    memset(extensions_to_enable, 0, sizeof(extensions_to_enable));
    memset(layers_to_enable, 0, sizeof(layers_to_enable));

    // Define this application info first
    vk_app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    vk_app_info.pNext = nullptr;
    vk_app_info.pApplicationName = app_name.c_str();
    vk_app_info.applicationVersion = app_version;
    vk_app_info.pEngineName = ENGINE_NAME;
    vk_app_info.engineVersion = ENGINE_VERSION;
    vk_app_info.apiVersion = VK_API_VERSION_1_0;

    // Define Vulkan Instance Create Info
    vk_inst_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    vk_inst_create_info.pNext = nullptr;
    vk_inst_create_info.flags = 0;
    vk_inst_create_info.pApplicationInfo = &vk_app_info;
    vk_inst_create_info.enabledExtensionCount = 0;
    vk_inst_create_info.ppEnabledExtensionNames = nullptr;
    vk_inst_create_info.enabledLayerCount = 0;
    vk_inst_create_info.ppEnabledLayerNames = nullptr;

    // If user wants to validate, check to see if we can enable it.
    if (validate) {
        vk_result = vkEnumerateInstanceLayerProperties(&count, nullptr);
        if (vk_result == VK_SUCCESS && count > 0) {
            layer_properties.resize(count);
            vk_result = vkEnumerateInstanceLayerProperties(&count, layer_properties.data());
            if (vk_result == VK_SUCCESS && count > 0) {
                for (uint32_t layer = 0; layer < count; layer++) {
                    if (!strcmp(layer_properties[layer].layerName, "VK_LAYER_LUNARG_standard_validation")) {
                        m_validation_enabled = true;
                        layers_to_enable[enable_layer_count++] = "VK_LAYER_LUNARG_standard_validation";
                        logger.LogInfo("Found standard validation layer");
                    }
                }
            }
        }
    }

    vk_result = vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
    if (VK_SUCCESS != vk_result || count == 0) {
        std::string error_msg =
            "GravityVulkanEngine::GravityVulkanEngine failed to query "
            "vkEnumerateInstanceExtensionProperties first time with error ";
        error_msg += vk_result;
        error_msg += " and count ";
        error_msg += std::to_string(count);
        logger.LogError(error_msg);
        exit(-1);
    }

    extension_properties.resize(count);
    vk_result = vkEnumerateInstanceExtensionProperties(nullptr, &count, extension_properties.data());
    if (VK_SUCCESS != vk_result || count == 0) {
        std::string error_msg =
            "GravityVulkanEngine::GravityVulkanEngine failed to query "
            "vkEnumerateInstanceExtensionProperties with count ";
        error_msg += std::to_string(count);
        error_msg += " error ";
        error_msg += vk_result;
        logger.LogError(error_msg);
        exit(-1);
    }

    enable_extension_count = count;
    if (!QueryWindowSystem(extension_properties, enable_extension_count, extensions_to_enable)) {
        logger.LogError("Failed GravityVulkanEngine::QueryWindowSystem");
        exit(-1);
    }

    for (uint32_t ext = 0; ext < count; ext++) {
        if (!strcmp(extension_properties[ext].extensionName, VK_EXT_DEBUG_REPORT_EXTENSION_NAME)) {
            m_debug_enabled = true;
            extensions_to_enable[enable_extension_count++] = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
            logger.LogInfo("Found debug report extension in Instance Extension list");
        }
    }

    VkDebugReportCallbackCreateInfoEXT dbg_create_info = {};

    GravityLogLevel level = logger.GetLogLevel();
    if (level > GRAVITY_LOG_DISABLE) {
        dbg_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
        dbg_create_info.pNext = nullptr;
        dbg_create_info.pfnCallback = LoggerCallback;
        dbg_create_info.pUserData = this;
        switch (level) {
            case GRAVITY_LOG_ALL:
                dbg_create_info.flags = VK_DEBUG_REPORT_DEBUG_BIT_EXT;
            case GRAVITY_LOG_INFO_WARN_ERROR:
                dbg_create_info.flags |= VK_DEBUG_REPORT_INFORMATION_BIT_EXT;
            case GRAVITY_LOG_WARN_ERROR:
                dbg_create_info.flags |= VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
            case GRAVITY_LOG_ERROR:
                dbg_create_info.flags |= VK_DEBUG_REPORT_ERROR_BIT_EXT;
                break;
            default:
                break;
        }
    }

    vk_inst_create_info.enabledExtensionCount = enable_extension_count;
    vk_inst_create_info.ppEnabledExtensionNames = (const char *const *)extensions_to_enable;
    vk_inst_create_info.enabledLayerCount = enable_layer_count;
    vk_inst_create_info.ppEnabledLayerNames = (const char *const *)layers_to_enable;

    if (level > GRAVITY_LOG_DISABLE) {
        vk_inst_create_info.pNext = &dbg_create_info;
    }

    vk_result = vkCreateInstance(&vk_inst_create_info, nullptr, &m_vk_inst);
    if (vk_result == VK_ERROR_INCOMPATIBLE_DRIVER) {
        logger.LogError(
            "GravityVulkanEngine::GravityVulkanEngine failed vkCreateInstance could not find a "
            "compatible Vulkan ICD");
        exit(-1);
    } else if (vk_result == VK_ERROR_EXTENSION_NOT_PRESENT) {
        logger.LogError(
            "GravityVulkanEngine::GravityVulkanEngine failed vkCreateInstance could not find "
            "one or more extensions");
        exit(-1);
    } else if (vk_result) {
        std::string error_msg = "GravityVulkanEngine::GravityVulkanEngine failed vkCreateInstance ";
        error_msg += vk_result;
        error_msg += " encountered while attempting to create instance";
        logger.LogError(error_msg);
        exit(-1);
    }

    // Hijack the layer dispatch table init and use it
    layer_init_instance_dispatch_table(m_vk_inst, &m_vk_inst_dispatch_table, vkGetInstanceProcAddr);

    if (level > GRAVITY_LOG_DISABLE) {
        vk_result =
            m_vk_inst_dispatch_table.CreateDebugReportCallbackEXT(m_vk_inst, &dbg_create_info, nullptr, &m_dbg_report_callback);
        if (vk_result != VK_SUCCESS) {
            std::string error_msg =
                "GravityVulkanEngine::GravityVulkanEngine failed call to "
                "CreateDebugReportCallback with error";
            error_msg += vk_result;
            logger.LogError(error_msg);
            exit(-1);
        }
    }
    vk_result = vkEnumeratePhysicalDevices(m_vk_inst, &m_num_phys_devs, nullptr);
    if (VK_SUCCESS != vk_result || m_num_phys_devs == 0) {
        std::string error_msg =
            "GravityVulkanEngine::GravityVulkanEngine failed to query "
            "vkEnumeratePhysicalDevices first time with error ";
        error_msg += vk_result;
        error_msg += " and count ";
        error_msg += std::to_string(m_num_phys_devs);
        logger.LogError(error_msg);
        exit(-1);
    }

    m_depth_stencil_surface.image_view = VK_NULL_HANDLE;
    m_depth_stencil_surface.image = VK_NULL_HANDLE;
    m_depth_stencil_surface.dev_memory = VK_NULL_HANDLE;

    m_quit = false;
}

GravityVulkanEngine::~GravityVulkanEngine() {
    GravityLogger &logger = GravityLogger::getInstance();
    GravityLogLevel level = logger.GetLogLevel();
    if (level > GRAVITY_LOG_DISABLE) {
        m_vk_inst_dispatch_table.DestroyDebugReportCallbackEXT(m_vk_inst, m_dbg_report_callback, nullptr);
    }

    CleanupDepthStencilSurface();

    for (uint32_t iii = 0; iii < m_primary_cmd_buffers.size(); ++iii) {
        if (m_primary_cmd_buffers[iii].recording) {
            vkEndCommandBuffer(m_primary_cmd_buffers[iii].vk_cmd_buf);
            vkFreeCommandBuffers(m_vk_device, m_vk_cmd_pool, 1, &m_primary_cmd_buffers[iii].vk_cmd_buf);
        }
    }
    m_primary_cmd_buffers.clear();
    vkDestroyCommandPool(m_vk_device, m_vk_cmd_pool, nullptr);
    CleanupSwapchain();
    vkDestroyDevice(m_vk_device, NULL);
    vkDestroySurfaceKHR(m_vk_inst, m_window->GetSurface(), NULL);
    vkDestroyInstance(m_vk_inst, NULL);
}

bool GravityVulkanEngine::QueryWindowSystem(std::vector<VkExtensionProperties> &ext_props, uint32_t &ext_count,
                                            const char **desired_extensions) {
    bool khr_surface_found = false;
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    bool khr_win32_surface_found = false;
#endif
#if defined(VK_USE_PLATFORM_XLIB_KHR)
    bool khr_xlib_surface_found = false;
#endif
#if defined(VK_USE_PLATFORM_XCB_KHR)
    bool khr_xcb_surface_found = false;
#endif
#if defined(VK_USE_PLATFORM_WAYLAND_KHR)
    bool khr_wayland_surface_found = false;
#endif
#if defined(VK_USE_PLATFORM_MIR_KHR)
    bool khr_mir_surface_found = false;
#endif
#if defined(VK_USE_PLATFORM_DISPLAY_KHR)
    bool khr_display_found = false;
#endif
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
    bool khr_android_surface_found = false;
#endif
#if defined(VK_USE_PLATFORM_IOS_MVK)
    bool khr_mvk_ios_surface_found = false;
#endif
#if defined(VK_USE_PLATFORM_MACOS_MVK)
    bool khr_mvk_macos_surface_found = false;
#endif
    GravityLogger &logger = GravityLogger::getInstance();

    if (ext_count == 0) {
        std::string error_msg =
            "GravityVulkanEngine::QueryWindowSystem incoming `ext_count` needs to contain"
            " size of 'desired_extensions' array";
        logger.LogError(error_msg);
        return false;
    }

    uint32_t count = 0;
    for (uint32_t i = 0; i < ext_props.size(); i++) {
        if (!strcmp(VK_KHR_SURFACE_EXTENSION_NAME, ext_props[i].extensionName)) {
            khr_surface_found = true;
            count++;
#if defined(VK_USE_PLATFORM_WIN32_KHR)
        } else if (!strcmp(VK_KHR_WIN32_SURFACE_EXTENSION_NAME, ext_props[i].extensionName)) {
            khr_win32_surface_found = true;
            count++;
#endif
#if defined(VK_USE_PLATFORM_XLIB_KHR)
        } else if (!strcmp(VK_KHR_XLIB_SURFACE_EXTENSION_NAME, ext_props[i].extensionName)) {
            khr_xlib_surface_found = true;
            count++;
#endif
#if defined(VK_USE_PLATFORM_XCB_KHR)
        } else if (!strcmp(VK_KHR_XCB_SURFACE_EXTENSION_NAME, ext_props[i].extensionName)) {
            khr_xcb_surface_found = true;
            count++;
#endif
#if defined(VK_USE_PLATFORM_WAYLAND_KHR)
        } else if (!strcmp(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME, ext_props[i].extensionName)) {
            khr_wayland_surface_found = true;
            count++;
#endif
#if defined(VK_USE_PLATFORM_MIR_KHR)
        } else if (!strcmp(VK_KHR_MIR_SURFACE_EXTENSION_NAME, ext_props[i].extensionName)) {
            khr_mir_surface_found = true;
            count++;
#endif
#if defined(VK_USE_PLATFORM_DISPLAY_KHR)
        } else if (!strcmp(VK_KHR_DISPLAY_EXTENSION_NAME, ext_props[i].extensionName)) {
            khr_display_found = true;
            count++;
#endif
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
        } else if (!strcmp(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME, ext_props[i].extensionName)) {
            khr_android_surface_found = true;
            count++;
#endif
#if defined(VK_USE_PLATFORM_IOS_MVK)
        } else if (!strcmp(VK_MVK_IOS_SURFACE_EXTENSION_NAME, ext_props[i].extensionName)) {
            khr_mvk_ios_surface_found = true;
            count++;
#endif
#if defined(VK_USE_PLATFORM_MACOS_MVK)
        } else if (!strcmp(VK_MVK_MACOS_SURFACE_EXTENSION_NAME, ext_props[i].extensionName)) {
            khr_mvk_macos_surface_found = true;
            count++;
#endif
        }
    }

    if (count < 2) {
        std::string error_msg = "GravityVulkanEngine::QueryWindowSystem failed to find a platform extension (count = ";
        error_msg += count;
        error_msg += ").";
        logger.LogError(error_msg);
        return false;
    } else if (count > ext_count) {
        std::string error_msg = "GravityVulkanEngine::QueryWindowSystem found too many extensions.  Expected < ";
        error_msg += ext_count;
        error_msg += ", but got ";
        error_msg += count;
        logger.LogError(error_msg);
        return false;
    } else {
        ext_count = 0;

        if (khr_surface_found) {
            desired_extensions[ext_count++] = VK_KHR_SURFACE_EXTENSION_NAME;
        }
#if defined(VK_USE_PLATFORM_WIN32_KHR)
        if (khr_win32_surface_found) {
            desired_extensions[ext_count++] = VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
        }
#endif
#if defined(VK_USE_PLATFORM_XLIB_KHR)
        if (khr_xlib_surface_found) {
            desired_extensions[ext_count++] = VK_KHR_XLIB_SURFACE_EXTENSION_NAME;
        }
#endif
#if defined(VK_USE_PLATFORM_XCB_KHR)
        if (khr_xcb_surface_found) {
            desired_extensions[ext_count++] = VK_KHR_XCB_SURFACE_EXTENSION_NAME;
        }
#endif
#if defined(VK_USE_PLATFORM_WAYLAND_KHR)
        if (khr_wayland_surface_found) {
            desired_extensions[ext_count++] = VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME;
        }
#endif
#if defined(VK_USE_PLATFORM_MIR_KHR)
        if (khr_mir_surface_found) {
            desired_extensions[ext_count++] = VK_KHR_MIR_SURFACE_EXTENSION_NAME;
        }
#endif
#if defined(VK_USE_PLATFORM_DISPLAY_KHR)
        if (khr_display_found) {
            desired_extensions[ext_count++] = VK_KHR_DISPLAY_EXTENSION_NAME;
        }
#endif
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
        if (khr_android_surface_found) {
            desired_extensions[ext_count++] = VK_KHR_ANDROID_SURFACE_EXTENSION_NAME;
        }
#endif
#if defined(VK_USE_PLATFORM_IOS_MVK)
        if (khr_mvk_ios_surface_found) {
            desired_extensions[ext_count++] = VK_MVK_IOS_SURFACE_EXTENSION_NAME;
        }
#endif
#if defined(VK_USE_PLATFORM_MACOS_MVK)
        if (khr_mvk_macos_surface_found) {
            desired_extensions[ext_count++] = VK_MVK_MACOS_SURFACE_EXTENSION_NAME;
        }
#endif
    }

    return true;
}

int GravityVulkanEngine::CompareGpus(VkPhysicalDeviceProperties &gpu_0, VkPhysicalDeviceProperties &gpu_1) {
    int gpu_to_use = 1;
    bool determined = false;

    // For now, use discrete over integrated
    if (gpu_0.deviceType != gpu_1.deviceType) {
        if (gpu_0.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            gpu_to_use = 0;
            determined = true;
        } else if (gpu_1.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            gpu_to_use = 1;
            determined = true;
        } else if (gpu_0.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
            gpu_to_use = 0;
            determined = true;
        } else if (gpu_1.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
            gpu_to_use = 1;
            determined = true;
        }
    }

    // For now, use newest API version if we got this far.
    if (!determined) {
        uint16_t major_0 = VK_VERSION_MAJOR(gpu_0.apiVersion);
        uint16_t minor_0 = VK_VERSION_MINOR(gpu_0.apiVersion);
        uint16_t major_1 = VK_VERSION_MAJOR(gpu_1.apiVersion);
        uint16_t minor_1 = VK_VERSION_MINOR(gpu_1.apiVersion);

        if (major_0 != major_1) {
            if (major_0 > major_1) {
                gpu_to_use = 0;
            } else {
                gpu_to_use = 1;
            }
        } else {
            if (minor_0 > minor_1) {
                gpu_to_use = 0;
            } else {
                gpu_to_use = 1;
            }
        }
    }
    return gpu_to_use;
}

bool GravityVulkanEngine::SetupInitalGraphicsDevice() {
    VkResult vk_result;
    uint32_t count = 0;
    uint32_t enable_extension_count = 0;
    const char *extensions_to_enable[VK_MAX_EXTENSION_NAME_SIZE];
    GravityLogger &logger = GravityLogger::getInstance();
    std::vector<VkPhysicalDevice> physical_devices;
    uint32_t queue_family_count = 0;
    std::vector<VkExtensionProperties> extension_properties;

    memset(extensions_to_enable, 0, sizeof(extensions_to_enable));

    physical_devices.resize(m_num_phys_devs);
    vk_result = vkEnumeratePhysicalDevices(m_vk_inst, &m_num_phys_devs, physical_devices.data());
    if (VK_SUCCESS != vk_result || m_num_phys_devs == 0) {
        std::string error_msg =
            "GravityVulkanEngine::SetupInitalGraphicsDevice failed to query "
            "vkEnumeratePhysicalDevices with count ";
        error_msg += std::to_string(m_num_phys_devs);
        error_msg += " error ";
        error_msg += vk_result;
        logger.LogError(error_msg);
        return false;
    }

    int32_t best_integrated_index = -1;
    int32_t best_discrete_index = -1;
    int32_t best_virtual_index = -1;
    std::vector<VkPhysicalDeviceProperties> phys_dev_props;
    phys_dev_props.resize(m_num_phys_devs);
    for (uint32_t i = 0; i < m_num_phys_devs; ++i) {
        vkGetPhysicalDeviceProperties(physical_devices[i], &phys_dev_props[i]);

        switch (phys_dev_props[i].deviceType) {
            case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                logger.LogInfo("Other device found");
                break;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                logger.LogInfo("Integrated GPU found");
                if (best_integrated_index != -1) {
                    if (CompareGpus(phys_dev_props[best_integrated_index], phys_dev_props[i])) {
                        best_integrated_index = i;
                    }
                } else {
                    best_integrated_index = i;
                }
                break;
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                logger.LogInfo("Discrete GPU found");
                if (best_discrete_index != -1) {
                    if (CompareGpus(phys_dev_props[best_discrete_index], phys_dev_props[i])) {
                        best_discrete_index = i;
                    }
                } else {
                    best_discrete_index = i;
                }
                break;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                logger.LogInfo("Virtual GPU found");
                if (best_virtual_index != -1) {
                    if (CompareGpus(phys_dev_props[best_virtual_index], phys_dev_props[i])) {
                        best_virtual_index = i;
                    }
                } else {
                    best_virtual_index = i;
                }
                break;
            case VK_PHYSICAL_DEVICE_TYPE_CPU:
                logger.LogInfo("CPU found");
                break;
            default:
                break;
        }
    }

    // If we have the choice between discrete and integrated, look at the
    // battery status to help make the decision.  If running on battery, use
    // integrated.  Otherwise, choose discrete.
    if (best_discrete_index != -1 && best_integrated_index != -1) {
        GravitySystemBatteryStatus battery_status = SystemBatteryStatus();
        switch (battery_status) {
            case GRAVITY_BATTERY_STATUS_NONE:
            case GRAVITY_BATTERY_STATUS_CHARGING:
                m_vk_phys_dev = physical_devices[best_discrete_index];
                break;
            default:
                m_vk_phys_dev = physical_devices[best_integrated_index];
                break;
        }
        // Otherwise, we have one or the other.
    } else if (best_discrete_index != -1) {
        m_vk_phys_dev = physical_devices[best_discrete_index];
    } else if (best_integrated_index != -1) {
        m_vk_phys_dev = physical_devices[best_integrated_index];
    } else if (best_virtual_index != -1) {
        m_vk_phys_dev = physical_devices[best_virtual_index];
    } else {
        logger.LogError("Failed to find a GPU of any kind");
        return false;
    }

    // Get Memory information and properties
    vkGetPhysicalDeviceMemoryProperties(m_vk_phys_dev, &m_vk_dev_mem_props);

    vk_result = vkEnumerateDeviceExtensionProperties(m_vk_phys_dev, nullptr, &count, nullptr);
    if (VK_SUCCESS != vk_result || count == 0) {
        std::string error_msg =
            "GravityVulkanEngine::SetupInitalGraphicsDevice failed to query "
            "vkEnumerateDeviceExtensionProperties first time with error ";
        error_msg += vk_result;
        error_msg += " and count ";
        error_msg += std::to_string(count);
        logger.LogError(error_msg);
        return false;
    }

    extension_properties.resize(count);
    vk_result = vkEnumerateDeviceExtensionProperties(m_vk_phys_dev, nullptr, &count, extension_properties.data());
    if (VK_SUCCESS != vk_result || count == 0) {
        std::string error_msg =
            "GravityVulkanEngine::SetupInitalGraphicsDevice failed to query "
            "vkEnumerateDeviceExtensionProperties with count ";
        error_msg += std::to_string(count);
        error_msg += " error ";
        error_msg += vk_result;
        logger.LogError(error_msg);
        return false;
    }

    bool found_swapchain = false;
    enable_extension_count = 0;
    for (uint32_t ext = 0; ext < count; ext++) {
        if (!strcmp(extension_properties[ext].extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME)) {
            found_swapchain = true;
            extensions_to_enable[enable_extension_count++] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
        }
    }

    if (!found_swapchain) {
        std::string error_msg = "GravityVulkanEngine::SetupInitalGraphicsDevice failed to find necessary extension ";
        error_msg += VK_KHR_SWAPCHAIN_EXTENSION_NAME;
        logger.LogError(error_msg);
        return false;
    }

    vkGetPhysicalDeviceQueueFamilyProperties(m_vk_phys_dev, &queue_family_count, nullptr);
    if (queue_family_count == 0) {
        std::string error_msg =
            "GravityVulkanEngine::SetupInitalGraphicsDevice failed to query "
            "vkGetPhysicalDeviceQueueFamilyProperties first time with count ";
        error_msg += std::to_string(queue_family_count);
        logger.LogError(error_msg);
        return false;
    }

    std::vector<VkQueueFamilyProperties> queue_family_props;
    queue_family_props.resize(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(m_vk_phys_dev, &queue_family_count, queue_family_props.data());

    if (!m_window->CreateGfxWindow(m_vk_inst)) {
        return false;
    }

    std::vector<VkBool32> present_support;
    present_support.resize(queue_family_count);
    for (uint32_t queue_family = 0; queue_family < queue_family_count; queue_family++) {
        m_vk_inst_dispatch_table.GetPhysicalDeviceSurfaceSupportKHR(m_vk_phys_dev, queue_family, m_window->GetSurface(),
                                                                    &present_support[queue_family]);
    }

    // Search for a graphics and a present queue in the array of queue
    // families, try to find one that supports both
    m_graphics_queue_family_index = UINT32_MAX;
    m_present_queue_family_index = UINT32_MAX;
    m_separate_present_queue = true;
    for (uint32_t queue_family = 0; queue_family < queue_family_count; queue_family++) {
        if ((queue_family_props[queue_family].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
            m_graphics_queue_family_index = queue_family;
            if (present_support[queue_family] == VK_TRUE) {
                // We found one that supports both
                m_present_queue_family_index = queue_family;
                m_separate_present_queue = false;
                break;
            }
        } else if (VK_TRUE == present_support[queue_family]) {
            m_present_queue_family_index = queue_family;
        }
    }

    if (UINT32_MAX == m_graphics_queue_family_index || UINT32_MAX == m_present_queue_family_index) {
        std::string error_msg =
            "GravityVulkanEngine::SetupInitalGraphicsDevice failed to find either "
            "a graphics or present queue for physical device.";
        logger.LogError(error_msg);
        return false;
    }

    float queue_priorities[1] = {0.0};
    VkDeviceQueueCreateInfo queue_create_info[2] = {};
    queue_create_info[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info[0].pNext = nullptr;
    queue_create_info[0].queueFamilyIndex = m_graphics_queue_family_index;
    queue_create_info[0].queueCount = 1;
    queue_create_info[0].pQueuePriorities = queue_priorities;
    queue_create_info[0].flags = 0;
    if (m_separate_present_queue) {
        queue_create_info[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info[1].pNext = nullptr;
        queue_create_info[1].queueFamilyIndex = m_present_queue_family_index;
        queue_create_info[1].queueCount = 1;
        queue_create_info[1].pQueuePriorities = queue_priorities;
        queue_create_info[1].flags = 0;
    }

    VkDeviceCreateInfo device_create_info = {};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pNext = nullptr;
    device_create_info.queueCreateInfoCount = 1;
    device_create_info.pQueueCreateInfos = queue_create_info;
    device_create_info.enabledLayerCount = 0;
    device_create_info.ppEnabledLayerNames = nullptr;
    device_create_info.enabledExtensionCount = enable_extension_count;
    device_create_info.ppEnabledExtensionNames = (const char *const *)extensions_to_enable;
    device_create_info.pEnabledFeatures = nullptr;
    if (m_separate_present_queue) {
        device_create_info.queueCreateInfoCount = 2;
    }

    vk_result = vkCreateDevice(m_vk_phys_dev, &device_create_info, nullptr, &m_vk_device);
    if (vk_result == VK_ERROR_INCOMPATIBLE_DRIVER) {
        logger.LogError(
            "GravityVulkanEngine::SetupInitalGraphicsDevice failed vkCreateDevice could not find a "
            "compatible Vulkan ICD");
        return false;
    } else if (vk_result == VK_ERROR_EXTENSION_NOT_PRESENT) {
        logger.LogError(
            "GravityVulkanEngine::SetupInitalGraphicsDevice failed vkCreateDevice could not find "
            "one or more extensions");
        return false;
    } else if (VK_SUCCESS != vk_result) {
        std::string error_msg = "GravityVulkanEngine::SetupInitalGraphicsDevice failed vkCreateDevice with ";
        error_msg += vk_result;
        logger.LogError(error_msg);
        return false;
    }

    // Again, hijack the layer mechanism for creating a device dispatch table.
    layer_init_device_dispatch_table(m_vk_device, &m_vk_dev_dispatch_table, vkGetDeviceProcAddr);

    // Create a command pool so we can allocate one or more command buffers
    // out of it.
    VkCommandPoolCreateInfo cmd_pool_create_info = {};
    cmd_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmd_pool_create_info.pNext = nullptr;
    cmd_pool_create_info.queueFamilyIndex = m_graphics_queue_family_index;
    cmd_pool_create_info.flags = 0;
    vk_result = vkCreateCommandPool(m_vk_device, &cmd_pool_create_info, nullptr, &m_vk_cmd_pool);
    if (VK_SUCCESS != vk_result) {
        std::string error_msg = "GravityVulkanEngine::SetupInitalGraphicsDevice failed vkCreateCommandPool with ";
        error_msg += vk_result;
        logger.LogError(error_msg);
        return false;
    }

    // Allocate one primary command buffer for now, then stick it in the m_primary_cmd_buffers
    // vector. We'll always use this as the engine's default primary command buffer.
    VkCommandBuffer cmd_buffer = VK_NULL_HANDLE;
    VkCommandBufferAllocateInfo cmd_buf_alloc_info = {};
    cmd_buf_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_buf_alloc_info.pNext = nullptr;
    cmd_buf_alloc_info.commandPool = m_vk_cmd_pool;
    cmd_buf_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_buf_alloc_info.commandBufferCount = 1;
    vk_result = vkAllocateCommandBuffers(m_vk_device, &cmd_buf_alloc_info, &cmd_buffer);
    if (VK_SUCCESS != vk_result) {
        std::string error_msg = "GravityVulkanEngine::SetupInitalGraphicsDevice failed vkAllocateCommandBuffers with ";
        error_msg += vk_result;
        logger.LogError(error_msg);
        return false;
    }
    m_primary_cmd_buffers.resize(1);
    m_primary_cmd_buffers[0].recording = false;
    m_primary_cmd_buffers[0].vk_cmd_buf = cmd_buffer;

    // We have things to do, that require a running command buffer.  So,
    // start recording with one.
    VkCommandBufferBeginInfo cmd_buf_begin_info = {};
    cmd_buf_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmd_buf_begin_info.pNext = nullptr;
    cmd_buf_begin_info.flags = 0;
    cmd_buf_begin_info.pInheritanceInfo = nullptr;
    vk_result = vkBeginCommandBuffer(cmd_buffer, &cmd_buf_begin_info);
    if (VK_SUCCESS != vk_result) {
        std::string error_msg = "GravityVulkanEngine::SetupInitalGraphicsDevice failed vkBeginCommandBuffer with ";
        error_msg += vk_result;
        logger.LogError(error_msg);
        return false;
    }
    m_primary_cmd_buffers[0].recording = true;

    if (!SetupSwapchain(logger)) {
        return false;
    }

    if (!SetupDepthStencilSurface(logger)) {
        return false;
    }

    return true;
}

bool GravityVulkanEngine::SetupSwapchain(GravityLogger &logger) {
    uint32_t count = 0;
    std::vector<VkSurfaceFormatKHR> surface_formats;
    VkSurfaceCapabilitiesKHR surface_caps = {};
    std::vector<VkPresentModeKHR> present_modes;
    VkExtent2D swapchain_extent = {};
    VkResult vk_result = VK_SUCCESS;

    // Get the list of VkFormat's that are supported:
    vk_result = m_vk_inst_dispatch_table.GetPhysicalDeviceSurfaceFormatsKHR(m_vk_phys_dev, m_window->GetSurface(), &count, nullptr);
    if (VK_SUCCESS != vk_result || count == 0) {
        std::string error_msg =
            "GravityVulkanEngine::SetupSwapchain failed to query "
            "GetPhysicalDeviceSurfaceFormatsKHR with count ";
        error_msg += std::to_string(count);
        error_msg += " error ";
        error_msg += vk_result;
        logger.LogError(error_msg);
        return false;
    }

    surface_formats.resize(count);
    vk_result = m_vk_inst_dispatch_table.GetPhysicalDeviceSurfaceFormatsKHR(m_vk_phys_dev, m_window->GetSurface(), &count,
                                                                            surface_formats.data());
    if (VK_SUCCESS != vk_result || count == 0) {
        std::string error_msg =
            "GravityVulkanEngine::SetupSwapchain failed to query "
            "GetPhysicalDeviceSurfaceFormatsKHR 2nd time with count ";
        error_msg += std::to_string(count);
        error_msg += " error ";
        error_msg += vk_result;
        logger.LogError(error_msg);
        return false;
    }

    // Setup some basic info for the swapchain surface
    memset(&m_swapchain_surface, 0, sizeof(GravitySwapchainSurface));
    m_swapchain_surface.format = VK_FORMAT_B8G8R8A8_UNORM;
    m_swapchain_surface.frame_index = 0;

    // If the format list includes just one entry of VK_FORMAT_UNDEFINED,
    // the surface has no preferred format.  Otherwise, at least one
    if (count == 1 && surface_formats[0].format == VK_FORMAT_UNDEFINED) {
        logger.LogInfo("Forcing surface to BGRA8 format");
        m_swapchain_surface.color_space = surface_formats[0].colorSpace;
    } else {
        for (auto surf_fmt = surface_formats.begin(); surf_fmt != surface_formats.end(); ++surf_fmt) {
            switch (surf_fmt->format) {
                case VK_FORMAT_B8G8R8A8_SRGB:
                    m_swapchain_surface.format = VK_FORMAT_B8G8R8A8_SRGB;
                    m_swapchain_surface.color_space = surf_fmt->colorSpace;
                    logger.LogInfo("Forcing surface to BGRA8 SRGB format");
                    break;
                case VK_FORMAT_B8G8R8A8_UNORM:
                    m_swapchain_surface.color_space = surf_fmt->colorSpace;
                    break;
                default:
                    break;
            }
        }
    }

    // Create semaphores to synchronize acquiring presentable buffers before
    // rendering and waiting for drawing to be complete before presenting
    VkSemaphoreCreateInfo semaphbore_create_info = {};
    semaphbore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphbore_create_info.pNext = nullptr;
    semaphbore_create_info.flags = 0;

    // Create fences that we can use to throttle if we get too far
    // ahead of the image presents
    VkFenceCreateInfo fence_create_info = {};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.pNext = nullptr;
    fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t i = 0; i < m_num_backbuffers; i++) {
        vk_result = vkCreateFence(m_vk_device, &fence_create_info, nullptr, &m_swapchain_surface.fences[i]);
        if (VK_SUCCESS != vk_result) {
            std::string error_msg = "GravityVulkanEngine::SetupSwapchain failed vkCreateFence for buffer ";
            error_msg += std::to_string(i);
            logger.LogError(error_msg);
            return false;
        }

        vk_result =
            vkCreateSemaphore(m_vk_device, &semaphbore_create_info, nullptr, &m_swapchain_surface.image_acquired_semaphores[i]);
        if (VK_SUCCESS != vk_result) {
            std::string error_msg = "GravityVulkanEngine::SetupSwapchain failed vkCreateSemaphore for buffer ";
            error_msg += std::to_string(i);
            error_msg += " image acquire semaphore";
            logger.LogError(error_msg);
            return false;
        }

        vk_result =
            vkCreateSemaphore(m_vk_device, &semaphbore_create_info, nullptr, &m_swapchain_surface.draw_complete_semaphores[i]);
        if (VK_SUCCESS != vk_result) {
            std::string error_msg = "GravityVulkanEngine::SetupSwapchain failed vkCreateSemaphore for buffer ";
            error_msg += std::to_string(i);
            error_msg += " draw complete semaphore";
            logger.LogError(error_msg);
            return false;
        }

        if (m_separate_present_queue) {
            vk_result = vkCreateSemaphore(m_vk_device, &semaphbore_create_info, nullptr,
                                          &m_swapchain_surface.image_ownership_semaphores[i]);
            if (VK_SUCCESS != vk_result) {
                std::string error_msg = "GravityVulkanEngine::SetupSwapchain failed vkCreateSemaphore for buffer ";
                error_msg += std::to_string(i);
                error_msg += " separate present queue image ownership semaphore";
                logger.LogError(error_msg);
                return false;
            }
        }
    }

    // Check the surface capabilities and formats
    vk_result =
        m_vk_inst_dispatch_table.GetPhysicalDeviceSurfaceCapabilitiesKHR(m_vk_phys_dev, m_window->GetSurface(), &surface_caps);
    if (VK_SUCCESS != vk_result) {
        logger.LogError(
            "GravityVulkanEngine::SetupSwapchain failed call to "
            "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
        return false;
    } else if (m_num_backbuffers < surface_caps.minImageCount ||
               (surface_caps.maxImageCount > 0 && m_num_backbuffers > surface_caps.maxImageCount)) {
        std::string error_msg = "GravityVulkanEngine::SetupSwapchain surface can't support ";
        error_msg += std::to_string(m_num_backbuffers);
        if (surface_caps.maxImageCount > 0) {
            error_msg += " backbuffers, number should be between ";
            error_msg += std::to_string(surface_caps.minImageCount);
            error_msg += " and ";
            error_msg += std::to_string(surface_caps.maxImageCount);
        } else {
            error_msg += " backbuffers, number should be at least ";
            error_msg += std::to_string(surface_caps.minImageCount);
        }
        logger.LogError(error_msg);
        return false;
    }

    vk_result =
        m_vk_inst_dispatch_table.GetPhysicalDeviceSurfacePresentModesKHR(m_vk_phys_dev, m_window->GetSurface(), &count, NULL);
    if (VK_SUCCESS != vk_result) {
        logger.LogError(
            "GravityVulkanEngine::SetupSwapchain failed call to "
            "GetPhysicalDeviceSurfacePresentModesKHR for count");
        return false;
    }
    present_modes.resize(count);
    vk_result = m_vk_inst_dispatch_table.GetPhysicalDeviceSurfacePresentModesKHR(m_vk_phys_dev, m_window->GetSurface(), &count,
                                                                                 present_modes.data());
    if (VK_SUCCESS != vk_result) {
        logger.LogError(
            "GravityVulkanEngine::SetupSwapchain failed call to "
            "GetPhysicalDeviceSurfacePresentModesKHR for data");
        return false;
    }

    swapchain_extent.width = m_window->GetWidth();
    swapchain_extent.height = m_window->GetHeight();

    // Width and height are either both 0xFFFFFFFF, or both not 0xFFFFFFFF.
    if (surface_caps.currentExtent.width == 0xFFFFFFFF) {
        // If the surface size is undefined, the size is set to the size
        // of the images requested, which must fit within the minimum and
        // maximum values.

        if (swapchain_extent.width < surface_caps.minImageExtent.width) {
            swapchain_extent.width = surface_caps.minImageExtent.width;
        } else if (swapchain_extent.width > surface_caps.maxImageExtent.width) {
            swapchain_extent.width = surface_caps.maxImageExtent.width;
        }

        if (swapchain_extent.height < surface_caps.minImageExtent.height) {
            swapchain_extent.height = surface_caps.minImageExtent.height;
        } else if (swapchain_extent.height > surface_caps.maxImageExtent.height) {
            swapchain_extent.height = surface_caps.maxImageExtent.height;
        }
    } else if (surface_caps.currentExtent.width < m_window->GetWidth() ||
               surface_caps.currentExtent.height < m_window->GetHeight()) {
        logger.LogError(
            "GravityVulkanEngine::SetupSwapchain surface doesn't support"
            " swapchains of the requested size");
        return false;
    }

    VkPresentModeKHR desired_present_mode = VK_PRESENT_MODE_FIFO_KHR;
    for (uint32_t pm = 0; pm < present_modes.size(); ++pm) {
        if (present_modes[pm] == desired_present_mode) {
            logger.LogInfo("Found present desired mode");
            break;
        }
    }

    // Determine the number of VkImages to use in the swap chain.
    // Application desires to acquire 3 images at a time for triple
    // buffering
    if (m_num_backbuffers < surface_caps.minImageCount) {
        m_num_backbuffers = surface_caps.minImageCount;
    }
    // If maxImageCount is 0, we can ask for as many images as we want;
    // otherwise we're limited to maxImageCount
    if (surface_caps.maxImageCount > 0 && (m_num_backbuffers > surface_caps.maxImageCount)) {
        // Application must settle for fewer images than desired:
        m_num_backbuffers = surface_caps.maxImageCount;
    }

    VkSurfaceTransformFlagBitsKHR surface_transform = {};
    if (surface_caps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
        surface_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    } else {
        surface_transform = surface_caps.currentTransform;
    }

    VkSwapchainCreateInfoKHR swapchain_create_info = {};
    swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_create_info.pNext = nullptr;
    swapchain_create_info.surface = m_window->GetSurface();
    swapchain_create_info.minImageCount = m_num_backbuffers;
    swapchain_create_info.imageFormat = m_swapchain_surface.format;
    swapchain_create_info.imageColorSpace = m_swapchain_surface.color_space;
    swapchain_create_info.imageExtent.width = swapchain_extent.width;
    swapchain_create_info.imageExtent.height = swapchain_extent.height;
    swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, swapchain_create_info.preTransform = surface_transform;
    swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_create_info.imageArrayLayers = 1;
    swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_create_info.queueFamilyIndexCount = 0;
    swapchain_create_info.pQueueFamilyIndices = nullptr;
    swapchain_create_info.presentMode = desired_present_mode;
    swapchain_create_info.oldSwapchain = VK_NULL_HANDLE;
    swapchain_create_info.clipped = true;

    vk_result = vkCreateSwapchainKHR(m_vk_device, &swapchain_create_info, nullptr, &m_swapchain_surface.vk_swapchain);
    if (VK_SUCCESS != vk_result) {
        logger.LogError("GravityVulkanEngine::SetupSwapchain failed call to CreateSwapchainKHR");
        return false;
    }

    std::vector<VkImage> swpchn_images;
    vk_result = vkGetSwapchainImagesKHR(m_vk_device, m_swapchain_surface.vk_swapchain, &count, nullptr);
    if (VK_SUCCESS != vk_result || count == 0) {
        logger.LogError(
            "GravityVulkanEngine::SetupSwapchain failed call to GetSwapchainImagesKHR "
            "querying number of swapchain images");
        return false;
    }

    swpchn_images.resize(count);
    vk_result = vkGetSwapchainImagesKHR(m_vk_device, m_swapchain_surface.vk_swapchain, &count, swpchn_images.data());
    if (VK_SUCCESS != vk_result || count == 0) {
        logger.LogError(
            "GravityVulkanEngine::SetupSwapchain failed call to GetSwapchainImagesKHR "
            "querying of swapchain images");
        return false;
    }

    m_swapchain_surface.swapchain_images.resize(count);

    // Create a fence
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.pNext = nullptr;
    fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t i = 0; i < count; i++) {
        VkImageViewCreateInfo image_view_create_info = {};
        image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        image_view_create_info.pNext = nullptr;
        image_view_create_info.flags = 0;
        image_view_create_info.format = m_swapchain_surface.format;
        image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_R;
        image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_G;
        image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_B;
        image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_A;
        image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_view_create_info.subresourceRange.baseMipLevel = 0;
        image_view_create_info.subresourceRange.levelCount = 1;
        image_view_create_info.subresourceRange.baseArrayLayer = 0;
        image_view_create_info.subresourceRange.layerCount = 1;
        image_view_create_info.image = swpchn_images[i];

        vk_result =
            vkCreateImageView(m_vk_device, &image_view_create_info, nullptr, &m_swapchain_surface.swapchain_images[i].image_view);
        if (VK_SUCCESS != vk_result) {
            std::string message = "GravityVulkanEngine::SetupSwapchain failed ";
            message += std::to_string(i);
            message += " call to vkCreateImageView";
            logger.LogError(message);
            return false;
        }

        vk_result = vkCreateFence(m_vk_device, &fence_create_info, nullptr, &m_swapchain_surface.swapchain_images[i].fence);
        if (VK_SUCCESS != vk_result) {
            std::string message = "GravityVulkanEngine::SetupSwapchain failed ";
            message += std::to_string(i);
            message += " call to vkCreateFence";
            logger.LogError(message);
            return false;
        }

        m_swapchain_surface.swapchain_images[i].image = swpchn_images[i];
    }

    return true;
}

void GravityVulkanEngine::CleanupSwapchain() {
    uint32_t i;

    vkDestroySwapchainKHR(m_vk_device, m_swapchain_surface.vk_swapchain, nullptr);
    for (i = 0; i < m_swapchain_surface.swapchain_images.size(); i++) {
        vkDestroyImageView(m_vk_device, m_swapchain_surface.swapchain_images[i].image_view, nullptr);
        vkDestroyFence(m_vk_device, m_swapchain_surface.swapchain_images[i].fence, nullptr);
    }

    for (i = 0; i < m_num_backbuffers; i++) {
        vkDestroyFence(m_vk_device, m_swapchain_surface.fences[i], nullptr);
        vkDestroySemaphore(m_vk_device, m_swapchain_surface.image_acquired_semaphores[i], nullptr);
        vkDestroySemaphore(m_vk_device, m_swapchain_surface.draw_complete_semaphores[i], nullptr);

        if (m_separate_present_queue) {
            vkDestroySemaphore(m_vk_device, m_swapchain_surface.image_ownership_semaphores[i], nullptr);
        }
    }
}

bool GravityVulkanEngine::SetupDepthStencilSurface(GravityLogger &logger) {
    VkResult vk_result = VK_SUCCESS;
    VkImageCreateInfo image_create_info = {};
    VkImageViewCreateInfo image_view_create_info = {};
    VkMemoryRequirements mem_reqs = {};
    VkMemoryAllocateInfo mem_alloc_info = {};

    m_depth_stencil_surface.format = VK_FORMAT_D24_UNORM_S8_UINT;

    // Create an image for the depth/stencil surface
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext = nullptr;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = m_depth_stencil_surface.format;
    image_create_info.extent.width = m_window->GetWidth();
    image_create_info.extent.height = m_window->GetHeight();
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    vk_result = vkCreateImage(m_vk_device, &image_create_info, nullptr, &m_depth_stencil_surface.image);
    if (VK_SUCCESS != vk_result) {
        std::string error_msg =
            "GravityVulkanEngine::SetupDepthStencilSurface failed to create image "
            "with error ";
        error_msg += vk_result;
        logger.LogError(error_msg);
        return false;
    }

    // See how much memory we require for the depth/stencil surface
    vkGetImageMemoryRequirements(m_vk_device, m_depth_stencil_surface.image, &mem_reqs);
    mem_alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mem_alloc_info.pNext = NULL;
    mem_alloc_info.allocationSize = mem_reqs.size;
    mem_alloc_info.memoryTypeIndex = 0;

    bool found_mem_type = false;
    for (uint32_t mem_type = 0; mem_type < VK_MAX_MEMORY_TYPES; mem_type++) {
        if (mem_reqs.memoryTypeBits & 0x1) {
            if ((m_vk_dev_mem_props.memoryTypes[mem_type].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
                mem_alloc_info.memoryTypeIndex = mem_type;
                found_mem_type = true;
                break;
            }
        }
        mem_reqs.memoryTypeBits >>= 1;
    }
    if (!found_mem_type) {
        logger.LogError(
            "GravityVulkanEngine::SetupDepthStencilSurface failed to find device local memory"
            " type for depth stencil surface");
        return false;
    }

    // Allocate memory for the depth/stencil buffer
    vk_result = vkAllocateMemory(m_vk_device, &mem_alloc_info, nullptr, &m_depth_stencil_surface.dev_memory);
    if (VK_SUCCESS != vk_result) {
        std::string error_msg =
            "GravityVulkanEngine::SetupDepthStencilSurface failed to allocate image memory "
            "with error ";
        error_msg += vk_result;
        logger.LogError(error_msg);
        return false;
    }

    // Bind the image to the memory
    vk_result = vkBindImageMemory(m_vk_device, m_depth_stencil_surface.image, m_depth_stencil_surface.dev_memory, 0);
    if (VK_SUCCESS != vk_result) {
        std::string error_msg =
            "GravityVulkanEngine::SetupDepthStencilSurface failed to bind image memory "
            "with error ";
        error_msg += vk_result;
        logger.LogError(error_msg);
        return false;
    }

    // Create an image view for the depth/stencil surface
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.pNext = nullptr;
    image_view_create_info.flags = 0;
    image_view_create_info.image = m_depth_stencil_surface.image;
    image_view_create_info.format = m_depth_stencil_surface.format;
    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    image_view_create_info.subresourceRange.baseMipLevel = 0;
    image_view_create_info.subresourceRange.levelCount = 1;
    image_view_create_info.subresourceRange.baseArrayLayer = 0;
    image_view_create_info.subresourceRange.layerCount = 1;
    vk_result = vkCreateImageView(m_vk_device, &image_view_create_info, NULL, &m_depth_stencil_surface.image_view);
    if (VK_SUCCESS != vk_result) {
        std::string error_msg =
            "GravityVulkanEngine::SetupDepthStencilSurface failed to create image view "
            "with error ";
        error_msg += vk_result;
        logger.LogError(error_msg);
        return false;
    }

    return true;
}

void GravityVulkanEngine::CleanupDepthStencilSurface() {
    if (VK_NULL_HANDLE != m_depth_stencil_surface.image_view) {
        vkDestroyImageView(m_vk_device, m_depth_stencil_surface.image_view, nullptr);
    }
    if (VK_NULL_HANDLE != m_depth_stencil_surface.image) {
        vkDestroyImage(m_vk_device, m_depth_stencil_surface.image, nullptr);
    }
    if (VK_NULL_HANDLE != m_depth_stencil_surface.dev_memory) {
        vkFreeMemory(m_vk_device, m_depth_stencil_surface.dev_memory, nullptr);
    }
}
