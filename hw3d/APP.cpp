#include "App.h"
#include "stdafx.h"
#include "Mouse.h"

App::App()
	:
	wnd(1280, 720, "My Window")  // 初始化窗口
{
	OutputDebugStringA("App constructor completed\n");
}

App::~App()
{
}

int App::Go()
{
	while (true)
	{
		if (const auto ecode = wnd.ProcessMessages())
		{
			return *ecode;
		}
		DoFrame();
	}
}

void App::DoFrame()
{
	const float c = sin(timer.Peek()) / 2.0f + 0.5f;
	wnd.Gfx().BeginFrame(c,c ,1.0f);
	wnd.Gfx().Frustum(
		-timer.Peek(),
		0.0f,
		0.0f
	);
	wnd.Gfx().Frustum(
		timer.Peek(),
		//wnd.mouse.GetPosX() / 400.0f - 1.0f,
		//-wnd.mouse.GetPosY() / 300.0f - 1.0f
		0.0f,
		0.0f
		);
	
		
	wnd.Gfx().EndFrame();
}