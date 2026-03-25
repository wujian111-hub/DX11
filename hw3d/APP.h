#pragma once
#include <string>
#include "Window.h"
#include "Timer.h"

class App
{
public:
	App();
	~App();
	int Go();
private:
	void DoFrame();
private:
	Window wnd;
	Timer timer;
	bool m_isRunning = true;  // 运行标志
};