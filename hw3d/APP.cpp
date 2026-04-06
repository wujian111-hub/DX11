#include "stdafx.h"
#include "App.h"

App::App()
    : wnd(1280, 720, "Skybox Demo"), m_isRunning(true)
{
    OutputDebugStringA("App constructor started\n");

    if (!wnd.Gfx().GetDevice() || !wnd.Gfx().GetContext())
    {
        OutputDebugStringA("Graphics device/context is null!\n");
        return;
    }

    // 初始化天空盒
    wnd.Gfx().InitializeSkybox();
    wnd.Gfx().LoadSkyboxTexture(L"skybox.dds");

    OutputDebugStringA("App constructor completed\n");
}

App::~App()
{
    OutputDebugStringA("App destructor started\n");
    OutputDebugStringA("App destructor completed\n");
}

int App::Go()
{
    while (m_isRunning)
    {
        if (const auto ecode = wnd.ProcessMessages())
        {
            return *ecode;
        }
        DoFrame();
    }
    return 0;
}

void App::DoFrame()
{
    const float dt = timer.Mark();

    // 处理鼠标输入（按住右键旋转相机）
    static bool rightButtonDown = false;

    if (wnd.mouse.IsRightPressed())
    {
        if (!rightButtonDown)
        {
            rightButtonDown = true;
            wnd.ResetMouseDelta();
        }

        float deltaX = wnd.GetMouseDeltaX();
        float deltaY = wnd.GetMouseDeltaY();

        // 旋转相机
        wnd.Gfx().RotateCamera(deltaX * 0.005f, deltaY * 0.005f);
    }
    else
    {
        rightButtonDown = false;
        wnd.ResetMouseDelta();
    }

    // 清屏
    wnd.Gfx().BeginFrame(0.1f, 0.1f, 0.2f);

    // 绘制天空盒
    wnd.Gfx().DrawSkybox();

    wnd.Gfx().EndFrame();
}