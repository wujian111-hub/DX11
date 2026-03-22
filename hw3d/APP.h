#pragma once
#include <string>
#include "Window.h"
#include "Timer.h"

class App
{
public:
	App();
	int Go();
	~App();
private:
	void DoFrame();
	Window wnd;
	Timer timer;
};