/*
 * LunarGravity
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
#include <vector>
#include <cstring>

#include "gravitylogger.hpp"
#include "gravityevent.hpp"

#ifdef VK_USE_PLATFORM_XCB_KHR
#include "gravitywindowxcb.hpp"
#include "gravityclocklinux.hpp"
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
#include "gravitywindowwayland.hpp"
#include "gravityclocklinux.hpp"
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
#include "gravitywindowwin32.hpp"
#include "gravityclockwin32.hpp"
#else
#include "gravitywindow.hpp"
#include "gravityclock.hpp"
#endif
#include "gravityvulkanengine.hpp"

#define APPLICATION_NAME "Lunar Gravity Demo"
#define APPLICATION_VERSION 1

int main(int argc, char *argv[]) {
    bool fullscreen = false;
    bool validate = false;
    bool enable_popups = true;
    GravityLogLevel log_level = GRAVITY_LOG_ERROR;
    uint32_t win_width = 320;
    uint32_t win_height = 240;
    uint32_t num_backbuffers = 2;
    bool print_usage = false;
    for (int32_t i = 1; i < argc; i++) {
        if (argv[i][0] != '-' || argv[i][1] != '-') {
            print_usage = true;
            break;
        }
        if (!strcmp(&argv[i][2], "help")) {
            print_usage = true;
        } else if (!strcmp(&argv[i][2], "validate")) {
            validate = true;
        } else if (!strcmp(&argv[i][2], "fullscreen")) {
            fullscreen = true;
        } else if (!strcmp(&argv[i][2], "width")) {
            if (argc >= i + 1) {
                win_width = atoi(argv[++i]);
            } else {
                print_usage = true;
            }
        } else if (!strcmp(&argv[i][2], "height")) {
            if (argc >= i + 1) {
                win_height = atoi(argv[++i]);
            } else {
                print_usage = true;
            }
        } else if (!strcmp(&argv[i][2], "backbuffers")) {
            if (argc >= i + 1) {
                num_backbuffers = atoi(argv[++i]);
            } else {
                print_usage = true;
            }
        } else if (!strcmp(&argv[i][2], "nopopups")) {
            enable_popups = false;
        } else if (!strcmp(&argv[i][2], "loglevel")) {
            if (argc >= i + 1) {
                if (!strcmp(argv[++i], "warn")) {
                    log_level = GRAVITY_LOG_WARN_ERROR;
                } else if (!strcmp(argv[i], "info")) {
                    log_level = GRAVITY_LOG_INFO_WARN_ERROR;
                } else if (!strcmp(argv[i], "all")) {
                    log_level = GRAVITY_LOG_ALL;
                } else {
                    print_usage = true;
                }
            } else {
                print_usage = true;
            }
        }
    }

    if (print_usage) {
        std::cout << "Usage: " << argv[0] << " [OPTIONS]" << std::endl
                  << "\t[OPTIONS]" << std::endl
                  << "\t\t--help\t\t\t\tPrint out this usage information" << std::endl
                  << "\t\t--fullscreen\t\t\tEnable fullscreen render" << std::endl
                  << "\t\t--validate\t\t\tEnable validation" << std::endl
                  << "\t\t--loglevel [warn, info, all]\tEnable logging of provided level and above." << std::endl
                  << "\t\t--nopopups\t\t\tDisable warning/error pop-ups on Windows" << std::endl
                  << "\t\t--height val\t\t\tSet window height to val" << std::endl
                  << "\t\t--width val\t\t\tSet window width to val" << std::endl
                  << "\t\t--backbuffers val\t\tNumber of backbuffers to use (2=double buffering, 3=triple, etc)" << std::endl;
        return -1;
    }

    GravityLogger &logger = GravityLogger::getInstance();
    logger.SetLogLevel(log_level);
    logger.TogglePopups(enable_popups);

    GravityEventList &event_list = GravityEventList::getInstance();
    event_list.Alloc(32);

#ifdef VK_USE_PLATFORM_XCB_KHR
    GravityWindowXcb gravity_window(APPLICATION_NAME, win_width, win_height, fullscreen);
    GravityClockLinux gravity_clock;
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
    GravityWindowWayland gravity_window(APPLICATION_NAME, win_width, win_height, fullscreen);
    GravityClockLinux gravity_clock;
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
    GravityWindowWin32 gravity_window(APPLICATION_NAME, win_width, win_height, fullscreen);
    GravityClockWin32 gravity_clock;
#else
#error "Unsupported Window format!"
#endif

    GravityVulkanEngine engine(APPLICATION_NAME, APPLICATION_VERSION, validate, &gravity_clock, &gravity_window, num_backbuffers);

    if (!engine.SetupInitalGraphicsDevice()) {
        return -1;
    }

    engine.Loop();

    return 0;
}
