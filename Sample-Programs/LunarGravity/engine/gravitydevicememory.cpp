/*
 * LunarGravity - gravitydevicememory.cpp
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
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <vector>
#include <cstring>
#include <string>

#include "gravitylogger.hpp"
#include "gravityinstanceextif.hpp"
#include "gravitydeviceextif.hpp"
#include "gravitydevicememory.hpp"

GravityDeviceMemory::GravityDeviceMemory(GravityInstanceExtIf *inst_ext_if, VkPhysicalDevice *phys_dev) {
    m_inst_ext_if = inst_ext_if;
    m_dev_ext_if = nullptr;
    m_vk_phys_dev = phys_dev;
    m_vk_dev_mem_props = {};

    // Get Memory information and properties
    vkGetPhysicalDeviceMemoryProperties(*phys_dev, &m_vk_dev_mem_props);
}

GravityDeviceMemory::~GravityDeviceMemory() {
    m_inst_ext_if = nullptr;
    m_dev_ext_if = nullptr;
}

void GravityDeviceMemory::SetupDevIf(GravityDeviceExtIf *dev_ext_if) {
    m_dev_ext_if = dev_ext_if;
}

bool GravityDeviceMemory::AllocateMemory(VkMemoryRequirements &mem_reqs, VkDeviceMemory &dev_memory) {
    GravityLogger &logger = GravityLogger::getInstance();
    VkMemoryAllocateInfo mem_alloc_info = {};
    VkResult vk_result = VK_SUCCESS;

    // Allocate the memory, in the correct location
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
            "GravityDeviceMemory::AllocateMemory failed to find device local memory"
            " type for depth stencil surface");
        return false;
    }

    // Allocate memory for the depth/stencil buffer
    vk_result = vkAllocateMemory(m_dev_ext_if->m_device, &mem_alloc_info, nullptr, &dev_memory);
    if (VK_SUCCESS != vk_result) {
        std::string error_msg =
            "GravityDeviceMemory::AllocateMemory failed to allocate device memory "
            "with error ";
        error_msg += vk_result;
        logger.LogError(error_msg);
        return false;
    }

    return true;
}