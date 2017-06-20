/*
 * LunarGravity - gravityscenesplash.cpp
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

#include "gravitylogger.hpp"
#include "gravitydevicememory.hpp"
#include "gravitytexture.hpp"
#include "gravityshader.hpp"
#include "gravityscenesplash.hpp"

GravitySceneSplash::GravitySceneSplash(std::string &scene_file, Json::Value &root, GravityInstanceExtIf *inst_ext_if)
    : GravityScene(scene_file, root, inst_ext_if) {
    GravityLogger &logger = GravityLogger::getInstance();
    if (root["texture"].isNull() || root["shader"].isNull() || root["vertices"].isNull()) {
        std::string error_msg = "GravitySceneSplash::GravitySceneSplash - Scene file ";
        error_msg += m_scene_file.c_str();
        error_msg += " invalid \"data\" section";
        logger.LogError(error_msg);
        exit(-1);
    }

    // Read the texture data
    Json::Value texture = root["texture"];
    if (texture["name"].isNull() || texture["file"].isNull() || texture["index"].isNull()) {
        std::string error_msg = "GravitySceneSplash::GravitySceneSplash - texture in scene file ";
        error_msg += m_scene_file.c_str();
        error_msg += " missing sub-sections";
        logger.LogError(error_msg);
        exit(-1);
    }
    m_texture.name = texture["name"].asString();
    m_texture.file = texture["file"].asString();
    m_texture.index = static_cast<uint8_t>(texture["index"].asUInt());
    m_texture.texture = nullptr;

    // Read the shader data
    Json::Value shader_node = root["shader"];
    if (shader_node["name"].isNull() || shader_node["prefix"].isNull()) {
        std::string error_msg = "GravitySceneSplash::GravitySceneSplash - shaderin scene file ";
        error_msg += m_scene_file.c_str();
        error_msg += " missing sub-sections";
        logger.LogError(error_msg);
        exit(-1);
    }
    m_shader.name = shader_node["name"].asString();
    m_shader.prefix = shader_node["prefix"].asString();
    m_shader.shader = nullptr;

    // Read the vertex data
    Json::Value vertices = root["vertices"];
    if (vertices["vertex components"].isNull() || vertices["texture coord groups"].isNull() ||
        vertices["texture coord components"].isNull() || vertices["data"].isNull() || !vertices["data"].isArray()) {
        std::string error_msg = "GravitySceneSplash::GravitySceneSplash - vertices in scene file ";
        error_msg += m_scene_file.c_str();
        error_msg += " missing sub-sections";
        logger.LogError(error_msg);
        exit(-1);
    }
    m_vertices.num_vert_comps = static_cast<uint8_t>(vertices["vertex components"].asUInt());
    m_vertices.num_tex_coords = static_cast<uint8_t>(vertices["texture coord groups"].asUInt());
    m_vertices.num_tex_coord_comps = static_cast<uint8_t>(vertices["texture coord components"].asUInt());

    Json::Value vert_data = vertices["data"];
    m_vertices.data.resize(vert_data.size());
    uint8_t vert = 0;
    if (!vert_data.isNull() && vert_data.isArray()) {
        for (Json::ValueIterator vert_data_it = vert_data.begin(); vert_data_it != vert_data.end(); vert_data_it++) {
            Json::Value cur_data = (*vert_data_it);
            m_vertices.data[vert++] = cur_data.asFloat();
        }
    }
}

GravitySceneSplash::~GravitySceneSplash() {}

bool GravitySceneSplash::Load(GravityDeviceExtIf *dev_ext_if, GravityDeviceMemoryManager *dev_memory_mgr) {
    GravityLogger &logger = GravityLogger::getInstance();

    if (!GravityScene::Load(dev_ext_if, dev_memory_mgr)) {
        logger.LogError("GravitySceneSplash::Load - failed GravityScene::Load");
        return false;
    }

    // Create and read the texture, but don't actually load it until we start this scene.  That way
    // we only use the memory that we need.
    GravityTexture *texture = new GravityTexture(m_inst_ext_if, m_dev_ext_if, m_dev_memory_mgr);
    if (nullptr == texture) {
        logger.LogError("GravitySceneSplash::Load - failed to allocate GravityTexture");
        return false;
    }
    if (!texture->Read(m_texture.file)) {
        logger.LogError("GravitySceneSplash::Load - failed to read GravityTexture");
        return false;
    }

    m_texture.texture = texture;

    // Create and read the shader contents.  Again, don't actually load anything
    GravityShader *shader = new GravityShader(m_inst_ext_if, m_dev_ext_if, m_dev_memory_mgr);
    if (nullptr == shader) {
        logger.LogError("GravitySceneSplash::Load - failed to allocate GravityShader");
        return false;
    }
    if (!shader->Read(m_shader.prefix)) {
        logger.LogError("GravitySceneSplash::Load - failed to read GravityShader");
        return false;
    }
    m_shader.shader = shader;

    return true;
}

bool GravitySceneSplash::Start() {
    GravityLogger &logger = GravityLogger::getInstance();
    if (!m_texture.texture->Load()) {
        logger.LogError("GravitySceneSplash::Start - failed to load GravityTexture");
        return false;
    }
    return true;
}

bool GravitySceneSplash::Update(float comp_time, float game_time) {
    bool success = true;
    return success;
}

bool GravitySceneSplash::Draw() {
    bool success = true;
    return success;
}

bool GravitySceneSplash::End() {
    GravityLogger &logger = GravityLogger::getInstance();
    if (!m_texture.texture->Unload()) {
        logger.LogError("GravitySceneSplash::End - failed to unload GravityTexture");
        return false;
    }
    return true;
}

bool GravitySceneSplash::Unload() {
    if (nullptr != m_texture.texture) {
        delete m_texture.texture;
        m_texture.texture = nullptr;
    }
    return true;
}
