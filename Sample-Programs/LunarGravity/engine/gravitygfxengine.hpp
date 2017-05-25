/*
 * LunarGravity - gravitygfxengine.hpp
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

#include <vector>
#include <string>

#include "gravitylogger.hpp"

class GravityClock;
class GravityWindow;
struct GravitySettingGroup;

enum GravitySystemBatteryStatus {
    GRAVITY_BATTERY_STATUS_NONE = 0,
    GRAVITY_BATTERY_STATUS_DISCHARGING_HIGH,      // > 66%
    GRAVITY_BATTERY_STATUS_DISCHARGING_MID,       // > 33%
    GRAVITY_BATTERY_STATUS_DISCHARGING_LOW,       // < 33%
    GRAVITY_BATTERY_STATUS_DISCHARGING_CRITICAL,  // < 5%
    GRAVITY_BATTERY_STATUS_CHARGING,
};

class GravityGraphicsEngine {
   public:
    // Create a protected constructor
    GravityGraphicsEngine();

    // We don't want any copy constructors
    GravityGraphicsEngine(const GravityGraphicsEngine &gfx_engine) = delete;
    GravityGraphicsEngine &operator=(const GravityGraphicsEngine &gfx_engine) = delete;

    // Make the destructor public
    virtual ~GravityGraphicsEngine();

    virtual bool Init(std::vector<std::string> &arguments);
    virtual void AppendUsageString(std::string &usage);
    void PrintUsage(std::string &usage);

    virtual bool SetupInitalGraphicsDevice() = 0;
    virtual bool ProcessEvents() = 0;

    void Loop();
    virtual bool Update() = 0;
    virtual bool BeginDrawFrame() = 0;
    virtual bool Draw() = 0;
    virtual bool EndDrawFrame() = 0;

   protected:
    GravitySystemBatteryStatus SystemBatteryStatus(void);

    bool m_print_usage;
    bool m_debug_enabled;
    bool m_quit;
    bool m_paused;
    std::string m_app_name;
    int64_t m_app_version;
    std::string m_engine_name;
    int64_t m_engine_version;
    uint64_t m_cur_frame;

    // Settings
    GravitySettingGroup *m_settings;

    // Graphics device items
    uint32_t m_num_phys_devs;
    uint32_t m_num_backbuffers;

    // Clock
    GravityClock *m_clock;

    // Window
    GravityWindow *m_window;

   private:
};
