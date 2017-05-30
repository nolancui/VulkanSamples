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

#include "gravitytexture.hpp"
#include "gravityscenesplash.hpp"

GravitySceneSplash::GravitySceneSplash(Json::Value &root, GravityInstanceExtIf *inst_ext_if, GravityDeviceExtIf *dev_ext_if)
    : GravityScene(root, inst_ext_if, dev_ext_if) {}

GravitySceneSplash::~GravitySceneSplash() {}

bool GravitySceneSplash::Start() {
    bool success = true;
    m_texture = new GravityTexture(m_inst_ext_if, m_dev_ext_if);
    if (nullptr != m_texture) {
        success = m_texture->Read("lunarg.ppm");
    }
    return success;
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
