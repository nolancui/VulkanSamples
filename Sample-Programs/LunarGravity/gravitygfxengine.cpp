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
 */

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <string>

#include "gravitylogger.hpp"
#include "gravitygfxengine.hpp"
#include "gravitywindow.hpp"
#include "gravityclock.hpp"
#include "gravityevent.hpp"

#ifdef _WIN32
//#include <windows.h>
#elif defined __ANDROID__
#else
#include <stdlib.h>
#endif

GravityGraphicsEngine::GravityGraphicsEngine(const std::string &app_name, uint16_t app_version, bool validate,
                                             GravityClock *gravity_clock, GravityWindow *gravity_window,
                                             uint32_t num_backbuffers) {
    m_quit = false;
    m_paused = false;
    m_clock = gravity_clock;
    m_window = gravity_window;
    m_num_phys_devs = 0;
    m_num_backbuffers = num_backbuffers;
}

GravityGraphicsEngine::~GravityGraphicsEngine() {
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

bool GravityGraphicsEngine::ProcessEvents(void) {
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
            default:
            {
                std::string warn = "GravityGraphicsEngine::ProcessEvents unhandled "
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

void GravityGraphicsEngine::Loop(void) {
    GravityLogger &logger = GravityLogger::getInstance();
    float last_update_computer_time_diff = 0.0f;
    float last_update_game_time_diff = 0.0f;

    m_clock->Start();
    logger.LogInfo("GravityGraphicsEngine::Loop starting engine loop");
    while (!m_quit) {
        float computer_time_diff = 0.0f;
        float game_time_diff = 0.0f;
        m_clock->GetTimeDiffMS(computer_time_diff, game_time_diff);
        last_update_computer_time_diff += computer_time_diff;
        last_update_game_time_diff += game_time_diff;
        if (!ProcessEvents()) {
            logger.LogError("GravityGraphicsEngine::Loop failed to process events!");
        }
        if (!m_paused) {
#if 0 // TODO : Brainpain
            demo_draw(demo);
            demo->curFrame++;
            if (demo->frameCount != INT32_MAX && demo->curFrame == demo->frameCount)
                demo->quit = true;
#endif
        }
        last_update_computer_time_diff = 0;
        last_update_game_time_diff = 0;
    }
    logger.LogInfo("GravityGraphicsEngine::Loop engine loop finished");
}
