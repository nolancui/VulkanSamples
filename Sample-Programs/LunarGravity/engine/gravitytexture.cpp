/*
 * LunarGravity - gravitytexture.cpp
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

#include <cstring>
#include <fstream>
#include <string>
#include <sstream>

#include "gravityinstanceextif.hpp"
#include "gravitydeviceextif.hpp"
#include "gravitylogger.hpp"
#include "gravitydevicememory.hpp"
#include "gravitytexture.hpp"

GravityTexture::GravityTexture(GravityInstanceExtIf *inst_ext_if, GravityDeviceExtIf *dev_ext_if,
                               GravityDeviceMemoryManager *dev_memory_mgr) {
    m_read = false;
    m_filename = "";
    m_width = 0;
    m_height = 0;
    m_cpu_data = nullptr;
    m_inst_ext_if = inst_ext_if;
    m_dev_ext_if = dev_ext_if;
    m_dev_memory_mgr = dev_memory_mgr;
    memset(&m_memory, 0, sizeof(GravityDeviceMemory));
    m_memory.vk_device_memory = VK_NULL_HANDLE;
}

GravityTexture::~GravityTexture() {
    Cleanup();
}

void GravityTexture::Cleanup() {
//    vkDestroyImageView(m_dev_ext_if->m_device, demo->textures[i].view, NULL);
//    vkDestroySampler(m_dev_ext_if->m_device, demo->textures[i].sampler, NULL);
    if (VK_NULL_HANDLE != m_memory.vk_device_memory) {
        Unload();
    }
    vkDestroyImage(m_dev_ext_if->m_device, m_image, NULL);

    if (nullptr != m_cpu_data) {
        delete[] m_cpu_data;
        m_cpu_data = nullptr;
    }
    m_read = false;
    m_filename = "";
    m_width = 0;
    m_height = 0;
}

bool GravityTexture::Read(std::string const &filename) {
#if 0 // Brainpain
return true;
#else
    GravityLogger &logger = GravityLogger::getInstance();

    m_filename = filename;

    if (filename.substr(filename.find_last_of(".") + 1) == "ppm") {
        std::string real_texture = "resources/textures/";
        real_texture += filename;
        m_read = ReadPPM(real_texture);
    }

    if (m_read) {
        VkImageCreateInfo image_create_info = {};

        image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_create_info.pNext = nullptr;
        image_create_info.flags = 0;
        image_create_info.imageType = VK_IMAGE_TYPE_2D;
        image_create_info.format = m_format;
        image_create_info.extent.width = m_width;
        image_create_info.extent.height = m_height;
        image_create_info.extent.depth = 1;
        image_create_info.mipLevels = 1;
        image_create_info.arrayLayers = 1;
        image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
        image_create_info.tiling = VK_IMAGE_TILING_LINEAR; // Brainpain - VK_IMAGE_TILING_OPTIMAL;
        image_create_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
        image_create_info.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;

        VkResult vk_result = vkCreateImage(m_dev_ext_if->m_device, &image_create_info, nullptr, &m_image);
        if (VK_SUCCESS != vk_result) {
            std::string error_msg =
                "GravityTexture::Read failed call to vkCreateImage with error ";
            error_msg += vk_result;
            logger.LogError(error_msg);
            m_read = false;
            goto out;
        }

        // Get the memory requirements for this image
        vkGetImageMemoryRequirements(m_dev_ext_if->m_device, m_image, &m_memory.vk_mem_reqs);
    }

out:

    if (!m_read) {
        Cleanup();
    }

    return m_read;
#endif
}

bool GravityTexture::ReadPPM(std::string const &filename) {
    bool read = false;
    std::ifstream infile(filename);
    std::string line;
    std::istringstream iss;
    std::getline(infile, line);
    if (line != "P6") {
        goto out;
    }

    std::getline(infile, line);
    iss.str(line);
    iss >> m_width >> m_height;
    m_num_comps = 4;
    m_comp_bytes = 1;

    uint32_t max_value;
    std::getline(infile, line);
    iss.str(line);
    iss >> max_value;

    m_cpu_data = new uint8_t[m_width * m_height * m_num_comps * m_comp_bytes];
    infile.read(reinterpret_cast<char*>(m_cpu_data), m_width * m_height * m_num_comps * m_comp_bytes);

    m_format = VK_FORMAT_R8G8B8A8_UNORM;

   read = true;

out:
    infile.close();
    return read;
}

bool GravityTexture::Load() {
    GravityLogger &logger = GravityLogger::getInstance();
    if (!m_dev_memory_mgr->AllocateMemory(m_memory, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
        logger.LogError("GravityTexture::Load failed GravityDeviceMemoryManager->AllocateMemory call");
        return false;
    }

#if 0
    /* bind memory */
    err = vkBindImageMemory(demo->device, tex_obj->image, tex_obj->mem, 0);
    assert(!err);

    if (required_props & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
        const VkImageSubresource subres = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .arrayLayer = 0,
        };
        VkSubresourceLayout layout;
        void *data;

        vkGetImageSubresourceLayout(demo->device, tex_obj->image, &subres,
                                    &layout);

        err = vkMapMemory(demo->device, tex_obj->mem, 0,
                          tex_obj->mem_alloc.allocationSize, 0, &data);
        assert(!err);

        if (!loadTexture(filename, data, &layout, &tex_width, &tex_height)) {
            fprintf(stderr, "Error loading texture: %s\n", filename);
        }

        vkUnmapMemory(demo->device, tex_obj->mem);
    }

    tex_obj->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
#endif

    return false;
}

bool GravityTexture::Unload() {
    m_dev_memory_mgr->FreeMemory(m_memory);

#if 0
    /* bind memory */
    err = vkBindImageMemory(demo->device, tex_obj->image, tex_obj->mem, 0);
    assert(!err);

    if (required_props & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
        const VkImageSubresource subres = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .arrayLayer = 0,
        };
        VkSubresourceLayout layout;
        void *data;

        vkGetImageSubresourceLayout(demo->device, tex_obj->image, &subres,
                                    &layout);

        err = vkMapMemory(demo->device, tex_obj->mem, 0,
                          tex_obj->mem_alloc.allocationSize, 0, &data);
        assert(!err);

        if (!loadTexture(filename, data, &layout, &tex_width, &tex_height)) {
            fprintf(stderr, "Error loading texture: %s\n", filename);
        }

        vkUnmapMemory(demo->device, tex_obj->mem);
    }

    tex_obj->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
#endif
    return true;
}
