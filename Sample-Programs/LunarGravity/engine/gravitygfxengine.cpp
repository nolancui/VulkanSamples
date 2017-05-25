/*
 * LunarGravity - gravitygfxengine.cpp
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
#include <cstdlib>
#include <cstring>
#include <string>

#include "gravitylogger.hpp"
#include "gravitysettingreader.hpp"

#ifdef VK_USE_PLATFORM_XCB_KHR
#include "gravityclocklinux.hpp"
#include "gravitywindowxcb.hpp"
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
#include "gravityclocklinux.hpp"
#include "gravitywindowwayland.hpp"
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
#include "gravityclockwin32.hpp"
#include "gravitywindowwin32.hpp"
#else
#include "gravityclock.hpp"
#include "graivtywindow.hpp"
#endif

#include "gravitygfxengine.hpp"
#include "gravityevent.hpp"

#ifdef _WIN32
//#include <windows.h>
#elif defined __ANDROID__
#else
#include <stdlib.h>
#endif

GravityGraphicsEngine::GravityGraphicsEngine() {
    GravitySettingReader settings_reader;
    GravityLogLevel log_level = GRAVITY_LOG_ERROR;
    m_print_usage = false;
    m_app_name = "";
    m_app_version = 0;
    m_engine_name = "";
    m_engine_version = 0;
    m_debug_enabled = false;
    m_quit = false;
    m_paused = false;
    m_num_phys_devs = 0;
    m_num_backbuffers = 2;

    GravityLogger &logger = GravityLogger::getInstance();
    logger.SetLogLevel(log_level);
    logger.TogglePopups(true);

    GravityEventList &event_list = GravityEventList::getInstance();
    event_list.Alloc(32);

    m_settings = new GravitySettingGroup();
    if (m_settings != nullptr && !settings_reader.ReadFile(logger, "resources/gravity.json", m_settings)) {
        logger.LogWarning("GravityGraphicsEngine failed to read settings file on init");
    }
}

GravityGraphicsEngine::~GravityGraphicsEngine() {
    if (nullptr != m_settings) {
        delete m_settings;
        m_settings = nullptr;
    }
    if (nullptr != m_clock) {
#if defined(VK_USE_PLATFORM_XCB_KHR) || defined(VK_USE_PLATFORM_WAYLAND_KHR)
        GravityClockLinux *clock = reinterpret_cast<GravityClockLinux *>(m_clock);
        delete clock;
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
        GravityClockWin32 *clock = reinterpret_cast<GravityClockWin32 *>(m_clock);
        delete clock;
        m_clock = new GravityClockWin32();
#else
        delete m_clock;
#endif

        m_clock = nullptr;
    }
    if (nullptr != m_window) {
        delete m_window;
        m_window = nullptr;
    }
}

bool GravityGraphicsEngine::Init(std::vector<std::string> &arguments) {
    bool app_name_found = false;
    bool app_version_found = false;
    GravityLogger &logger = GravityLogger::getInstance();

    if (m_settings == nullptr) {
        return false;
    }
    for (auto cur_group : m_settings->groups) {
        // We only want the name and version out of the application data
        if (cur_group.name == "application") {
            for (auto app_setting : cur_group.settings) {
                if (app_setting.name == "name") {
                    m_app_name = app_setting.value;
                    app_name_found = true;
                } else if (app_setting.name == "version") {
                    m_app_version = atoi(app_setting.value.c_str());
                    app_version_found = true;
                }
                if (app_name_found && app_version_found) {
                    break;
                }
            }
        } else if (cur_group.name == "engine") {
            for (auto cur_setting : cur_group.settings) {
                if (cur_setting.name == "name") {
                    m_engine_name = cur_setting.value;
                } else if (cur_setting.name == "version") {
                    m_engine_version = atoi(cur_setting.value.c_str());
                } else if (cur_setting.name == "debug") {
                    if (cur_setting.value == "off") {
                        m_debug_enabled = false;
                    } else if (cur_setting.value == "on") {
                        m_debug_enabled = true;
                    }
                } else if (cur_setting.name == "loglevel") {
                    if (cur_setting.value == "errors") {
                        logger.SetLogLevel(GRAVITY_LOG_ERROR);
                    } else if (cur_setting.value == "warnings") {
                        logger.SetLogLevel(GRAVITY_LOG_WARN_ERROR);
                    } else if (cur_setting.value == "info") {
                        logger.SetLogLevel(GRAVITY_LOG_INFO_WARN_ERROR);
                    } else if (cur_setting.value == "all") {
                        logger.SetLogLevel(GRAVITY_LOG_ALL);
                    }
                }
            }
        }
    }

    for (uint32_t arg = 1; arg < arguments.size(); arg++) {
        const char *parg = arguments[arg].c_str();
        if (arguments[arg] == "--help" || parg[0] != '-' || parg[1] != '-') {
            m_print_usage = true;
            break;
        } else if (arguments[arg] == "--debug") {
            m_debug_enabled = true;
        } else if (arguments[arg] == "--nopopups") {
            logger.TogglePopups(false);
        } else if (arguments[arg] == "--loglevel") {
            if (arg < arguments.size() - 1) {
                arg++;
                if (arguments[arg] == "warn") {
                    logger.SetLogLevel(GRAVITY_LOG_WARN_ERROR);
                } else if (arguments[arg] == "info") {
                    logger.SetLogLevel(GRAVITY_LOG_INFO_WARN_ERROR);
                } else if (arguments[arg] == "all") {
                    logger.SetLogLevel(GRAVITY_LOG_ALL);
                } else {
                    m_print_usage = true;
                }
            } else {
                m_print_usage = true;
            }
        }
    }

#if defined(VK_USE_PLATFORM_XCB_KHR) || defined(VK_USE_PLATFORM_WAYLAND_KHR)
    m_clock = new GravityClockLinux();
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
    m_clock = new GravityClockWin32();
#else
    m_clock = new GravityClock();
#endif

#ifdef VK_USE_PLATFORM_XCB_KHR
    m_window = new GravityWindowXcb(m_app_name, m_settings, arguments, m_clock);
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
    m_window = new GravityWindowWayland(m_app_name, m_settings, arguments, m_clock);
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
    m_window = new GravityWindowWin32(m_app_name, m_settings, arguments, m_clock);
#else
#error "Unsupported Window format!"
#endif

    m_cur_frame = 0;

    if (m_clock == nullptr || m_window == nullptr) {
        return false;
    } else {
        return true;
    }
}

void GravityGraphicsEngine::AppendUsageString(std::string &usage) {
    usage += "\t\t--help\t\t\t\tPrint out this usage information\n";
    usage += "\t\t--debug\t\t\t\tEnable debugging\n";
    usage += "\t\t--loglevel [warn, info, all]\tEnable logging of provided level and above.\n";
    usage += "\t\t--nopopups\t\t\tDisable warning/error pop-ups on Windows\n";
}

void GravityGraphicsEngine::PrintUsage(std::string &usage) {
    std::cout << usage.c_str() << std::endl;
    exit(-1);
}

GravitySystemBatteryStatus GravityGraphicsEngine::SystemBatteryStatus(void) {
    GravitySystemBatteryStatus cur_status = GRAVITY_BATTERY_STATUS_NONE;

#ifdef _WIN32
    SYSTEM_POWER_STATUS status;
    if (GetSystemPowerStatus(&status)) {
        switch (status.BatteryFlag) {
            default:
                break;
            case 0:
                cur_status = GRAVITY_BATTERY_STATUS_DISCHARGING_MID;
                break;
            case 1:
                cur_status = GRAVITY_BATTERY_STATUS_DISCHARGING_HIGH;
                break;
            case 2:
                cur_status = GRAVITY_BATTERY_STATUS_DISCHARGING_LOW;
                break;
            case 4:
                cur_status = GRAVITY_BATTERY_STATUS_DISCHARGING_CRITICAL;
                break;
            case 8:
                cur_status = GRAVITY_BATTERY_STATUS_CHARGING;
                break;
        }
    }

#elif defined __ANDROID__

#error "No Android support!"

#else

    // Check Linux battery level using ACPI:
    //  acpi -b output:
    //      Battery 0: Full, 100%
    //      Battery 0: Discharging, 95%, 10:32:44 remaining
    //      Battery 0: Charging, 94%, rate information unavailable
    const char battery_check_string[] = "acpi -b | awk -F'[, ]' '{ print $3 }'";
    const char power_level_string[] = "acpi -b | grep -P -o '[0-9]+(?=%)'";
    char result[256];
    FILE *fp = popen(battery_check_string, "r");
    if (fp != NULL) {
        bool discharging = false;
        if (fgets(result, sizeof(result) - 1, fp) != NULL) {
            if (NULL != strstr(result, "Charging")) {
                cur_status = GRAVITY_BATTERY_STATUS_CHARGING;
            } else if (NULL != strstr(result, "Discharging")) {
                discharging = true;
            }
        }
        pclose(fp);
        if (discharging) {
            fp = popen(power_level_string, "r");
            if (fp != NULL) {
                if (fgets(result, sizeof(result) - 1, fp) != NULL) {
                    int32_t battery_perc = atoi(result);
                    if (battery_perc > 66) {
                        cur_status = GRAVITY_BATTERY_STATUS_DISCHARGING_HIGH;
                    } else if (battery_perc > 33) {
                        cur_status = GRAVITY_BATTERY_STATUS_DISCHARGING_MID;
                    } else if (battery_perc > 5) {
                        cur_status = GRAVITY_BATTERY_STATUS_DISCHARGING_LOW;
                    } else {
                        cur_status = GRAVITY_BATTERY_STATUS_DISCHARGING_CRITICAL;
                    }
                }
                pclose(fp);
            }
        }
    }

#endif

    return cur_status;
}

void GravityGraphicsEngine::Loop(void) {
    GravityLogger &logger = GravityLogger::getInstance();
    float last_update_computer_time_diff = 0.0f;
    float last_update_game_time_diff = 0.0f;

    // If we hit the loop, we no longer need the settings file if we found one
    if (nullptr != m_settings) {
        delete m_settings;
        m_settings = nullptr;
    }

    m_clock->Start();
    logger.LogInfo("GravityGraphicsEngine::Loop starting engine loop");
    while (!m_quit) {
        float computer_time_diff = 0.0f;
        float game_time_diff = 0.0f;
        m_clock->GetTimeDiffMS(computer_time_diff, game_time_diff);
        last_update_computer_time_diff += computer_time_diff;
        last_update_game_time_diff += game_time_diff;
        ProcessEvents();
        Update();
        if (!m_paused) {
            if (BeginDrawFrame()) {
                Draw();
                EndDrawFrame();
                m_cur_frame++;
            }
        }
        last_update_computer_time_diff = 0;
        last_update_game_time_diff = 0;
    }
    logger.LogInfo("GravityGraphicsEngine::Loop engine loop finished");
}
