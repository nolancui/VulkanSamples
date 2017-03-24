/*
 * LunarGravity - gravitywindowwin32.hpp
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

#pragma once

#ifdef VK_USE_PLATFORM_WIN32_KHR

#include <thread>

#include "gravitywindow.hpp"

class GravityWindowWin32 : public GravityWindow {
   public:
    // Create a protected constructor
    GravityWindowWin32(const char *win_name, const uint32_t width, const uint32_t height, bool fullscreen);

    // We don't want any copy constructors
    GravityWindowWin32(const GravityWindowWin32 &window) = delete;
    GravityWindowWin32 &operator=(const GravityWindowWin32 &window) = delete;

    // Make the destructor public
    virtual ~GravityWindowWin32();

    virtual bool CreateGfxWindow(VkInstance &instance);
    virtual bool CloseGfxWindow();

    virtual void TriggerQuit();

    HWND GetHwnd() { return m_window; }

   private:
    HINSTANCE m_instance;
    HWND m_window;
    POINT m_minsize;

    std::thread *m_window_thread;
};

#endif  // VK_USE_PLATFORM_WIN32_KHR