#pragma once
#include <unordered_map>
#include <Windows.h>
#include <string>

class WindowsMessageMap
{
public:
	WindowsMessageMap() noexcept;
	std::wstring operator()(DWORD msg, LPARAM lp, WPARAM wp) const noexcept;
private:
	std::unordered_map<DWORD, std::wstring> map;
};
