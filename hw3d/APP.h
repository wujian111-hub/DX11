#pragma once
#include <string>
#include "Window.h"
#include "ChiliTimer.h"

class App
{
public:
	App();
	int Go();
	~App();
private:
	void DoFrame();
	Window wnd;
	ChiliTimer timer;
};