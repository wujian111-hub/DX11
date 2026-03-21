#include "App.h"
#include "stdafx.h"

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
	wnd.Gfx().ClearBuffer(c, c, 1.0f);
	wnd.Gfx().BeginFrame(0.0f, 0.0f, 0.0f);
	wnd.Gfx().DrawTestTriangle();
	wnd.Gfx().EndFrame();
	
}