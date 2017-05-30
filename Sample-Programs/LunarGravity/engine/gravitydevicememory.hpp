/*
 * LunarGravity - gravitydevicememory.hpp
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

class GravityInstanceExtIf;
class GravityDeviceExtIf;

class GravityDeviceMemory {
   public:
    GravityDeviceMemory(GravityInstanceExtIf *inst_ext_if, VkPhysicalDevice *phys_dev);
    ~GravityDeviceMemory();

    GravityDeviceMemory(GravityDeviceMemory const &) = delete;
    void operator=(GravityDeviceMemory const &) = delete;

    void SetupDevIf(GravityDeviceExtIf *dev_ext_if);

    bool AllocateMemory(VkMemoryRequirements &mem_reqs, VkDeviceMemory &dev_memory);

   private:
    GravityInstanceExtIf *m_inst_ext_if;
    GravityDeviceExtIf *m_dev_ext_if;
    VkPhysicalDevice *m_vk_phys_dev;
    VkPhysicalDeviceMemoryProperties m_vk_dev_mem_props;
};