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
#include "gravityscenesplash.hpp"

GravitySceneSplash::GravitySceneSplash(Json::Value &root, GravityInstanceExtIf *inst_ext_if)
    : GravityScene(root, inst_ext_if) {}

GravitySceneSplash::~GravitySceneSplash() {}

bool GravitySceneSplash::Load(GravityDeviceExtIf *dev_ext_if, GravityDeviceMemoryManager *dev_memory_mgr) {
    GravityLogger &logger = GravityLogger::getInstance();

    if (!GravityScene::Load(dev_ext_if, dev_memory_mgr)) {
        logger.LogError("GravitySceneSplash::Load - failed GravityScene::Load");
        return false;
    }

    m_texture = new GravityTexture(m_inst_ext_if, m_dev_ext_if, m_dev_memory_mgr);
    if (nullptr == m_texture) {
        logger.LogError("GravitySceneSplash::Load - failed to allocate GravityTexture");
        return false;
    }

    if (!m_texture->Read("lunarg.ppm")) {
        logger.LogError("GravitySceneSplash::Load - failed to read GravityTexture");
        return false;
    }

    return true;
}

bool GravitySceneSplash::Start() {
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
    bool success = true;
    return success;
}
