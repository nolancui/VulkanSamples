/*
 * LunarGravity - gravityvulkandemo.cpp
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

#include "vk_dispatch_table_helper.h"

#include "gravitylogger.hpp"
#include "gravitysettingreader.hpp"
#include "gravityvulkandemo.hpp"
#include "gravitywindow.hpp"
#include "gravityclock.hpp"
#include "gravityevent.hpp"

GravityVulkanDemo::GravityVulkanDemo() : GravityVulkanEngine() {
}

GravityVulkanDemo::~GravityVulkanDemo() {
}

void GravityVulkanDemo::AppendUsageString(std::string &usage) {
    GravityVulkanEngine::AppendUsageString(usage);
}

bool GravityVulkanDemo::ProcessEvents(void) {
    GravityLogger &logger = GravityLogger::getInstance();
    GravityEventList &event_list = GravityEventList::getInstance();
    bool success = true;

    while (event_list.HasEvents()) {
        GravityEvent cur_event;
        if (!event_list.RemoveEvent(cur_event)) {
            logger.LogError("RemoveEvent failed");
            success = false;
            break;
        }

        switch (cur_event.Type()) {
            case GravityEvent::GRAVITY_EVENT_WINDOW_RESIZE:
                logger.LogInfo("GravityGraphicsEngine::ProcessEvents Resize event");
                break;
            case GravityEvent::GRAVITY_EVENT_WINDOW_CLOSE:
                logger.LogInfo("GravityGraphicsEngine::ProcessEvents Close event");
                m_quit = true;
                m_window->TriggerQuit();
                break;
            case GravityEvent::GRAVITY_EVENT_KEY_PRESS:
            case GravityEvent::GRAVITY_EVENT_KEY_RELEASE:
                logger.LogInfo("GravityGraphicsEngine::ProcessEvents Keypress event");
                switch (cur_event.data.key) {
                    case KEYNAME_ESCAPE:
                        m_quit = true;
                        m_window->TriggerQuit();
                        break;
                    case KEYNAME_SPACE:
                        m_paused = !m_paused;
                        break;
                    default:
                        break;
                };
                break;
            default: {
                std::string warn =
                    "GravityGraphicsEngine::ProcessEvents unhandled "
                    " event type ";
                warn += std::to_string(cur_event.Type());
                warn += "\n";
                logger.LogWarning(warn);
                break;
            }
        }
    }

    return success;
}

bool GravityVulkanDemo::Init(std::vector<std::string> &arguments) {
    // Call the parent and do all the necessary setup
    if (!GravityVulkanEngine::Init(arguments)) {
        return false;
    }

    // TODO: Demo specific Init code

    return true;
}

bool GravityVulkanDemo::Update() {
    // TODO: Demo specific Update code

    return true;
}

bool GravityVulkanDemo::BeginDrawFrame() {
    bool success = GravityVulkanEngine::BeginDrawFrame();

    // TODO: Demo specific BeginDrawFrame code

    return success;
}

bool GravityVulkanDemo::Draw() {
    // TODO: Demo specific Draw code

    return true;
}

bool GravityVulkanDemo::EndDrawFrame() {
    // TODO: Demo specific EndDrawFrame code

    return GravityVulkanEngine::EndDrawFrame();
}