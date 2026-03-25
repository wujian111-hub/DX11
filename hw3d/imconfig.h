#pragma once
//-----------------------------------------------------------------------------
// COMPILE-TIME OPTIONS FOR DEAR IMGUI
//-----------------------------------------------------------------------------

#pragma once

//---- 启用 Win32 剪贴板功能（默认就是启用的，但如果你想确保）
#define IMGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCTIONS   // 如果你想自己实现剪贴板，取消这行注释
// 通常不需要，保持注释状态即可

//---- 如果使用 DX11，不需要特殊配置，但可以确保颜色格式正确
#define IMGUI_USE_BGRA_PACKED_COLOR    // 如果你的 DX11 使用 BGRA 格式（通常需要）

//---- 如果你使用 32 位顶点索引（超过 65535 个顶点）
// #define ImDrawIdx unsigned int

//---- 如果你想让 ImGui 使用你的内存分配函数（通常不需要）
// #define IMGUI_DISABLE_DEFAULT_ALLOCATORS

//---- 禁用演示窗口（发布版本可以启用）
// #define IMGUI_DISABLE_DEMO_WINDOWS

//---- 禁用 Win32 函数（如果你自己处理，通常不需要）
// #define IMGUI_DISABLE_WIN32_FUNCTIONS

//---- 可选：让你的数学类型与 ImVec2 互转
// 如果你有自己的 Vector2 类，可以取消注释并修改
/*
#define IM_VEC2_CLASS_EXTRA                                                 \
        ImVec2(const MyVec2& f) { x = f.x; y = f.y; }                       \
        operator MyVec2() const { return MyVec2(x,y); }
*/