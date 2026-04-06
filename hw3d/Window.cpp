#include "stdafx.h"
#include "Window.h"
#include <string>
#include <windows.h>
#include "resource.h"
#include <sstream>
#include <optional> 
#include "Mouse.h"

#ifndef COLOR_WINDOW
#define COLOR_WINDOW 5
#endif
#ifndef IDI_APPLICATION
#define IDI_APPLICATION 32512
#endif

// 静态成员初始化
bool Window::s_isClosing = false;

Window::WindowClass Window::WindowClass::wndClass;

Window::WindowClass::WindowClass() noexcept
	:
	hInst(GetModuleHandle(nullptr))
{
	WNDCLASSEXW wc = { 0 };
	wc.cbSize = sizeof(wc);
	wc.style = CS_OWNDC;
	wc.lpfnWndProc = HandleMsgSetup;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = GetInstance();
	// 使用系统默认图标，避免加载失败
	wc.hIcon = LoadIconW(nullptr, MAKEINTRESOURCEW(IDI_APPLICATION));
	wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
	wc.hbrBackground = nullptr;  // 关键：设置为 nullptr，让 DirectX 完全控制背景
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = GetName();
	wc.hIconSm = LoadIconW(nullptr, MAKEINTRESOURCEW(IDI_APPLICATION));
	RegisterClassExW(&wc);
}

Window::WindowClass::~WindowClass()
{
	UnregisterClassW(GetName(), GetInstance());
}

const wchar_t* Window::WindowClass::GetName() noexcept
{
	return wndClassName;
}

HINSTANCE Window::WindowClass::GetInstance() noexcept
{
	return wndClass.hInst;
}

Window::Window(int width, int height, const char* name)
	:
	width(width),
	height(height)
{
	// 转换窗口标题为宽字符串
	std::wstring wname;
	if (name != nullptr)
	{
		int len = (int)strlen(name);
		wname.resize(len);
		for (int i = 0; i < len; i++)
		{
			wname[i] = (wchar_t)name[i];
		}
	}
	isRunning = true;

	// 计算窗口大小
	RECT wr;
	wr.left = 100;
	wr.right = width + wr.left;
	wr.top = 100;
	wr.bottom = height + wr.top;
	if (AdjustWindowRect(&wr, WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU, FALSE) == 0)
	{
		throw Window::HrException(__LINE__, __FILE__, GetLastError());
	}

	// 创建窗口
	hWnd = CreateWindowExW(
		0,
		WindowClass::GetName(),
		wname.c_str(),
		WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU,
		CW_USEDEFAULT, CW_USEDEFAULT, wr.right - wr.left, wr.bottom - wr.top,
		nullptr, nullptr, WindowClass::GetInstance(), this
	);

	if (hWnd == nullptr)
	{
		throw Window::HrException(__LINE__, __FILE__, GetLastError());
	}

	// 显示窗口
	ShowWindow(hWnd, SW_SHOWDEFAULT);

	// 创建图形对象
	pGfx = std::make_unique<Graphics>(hWnd, width, height);
	OutputDebugStringA("Window constructor completed\n");
}

Window::~Window()
{
	// 标记窗口正在关闭
	s_isClosing = true;

	// 先销毁 Graphics
	if (pGfx)
	{
		pGfx.reset();
	}

	// 再销毁窗口
	if (hWnd)
	{
		DestroyWindow(hWnd);
		hWnd = nullptr;
	}
}

void Window::SetTitle(const std::string& title)
{
	std::wstring wtitle;
	if (!title.empty())
	{
		int len = (int)title.length();
		wtitle.resize(len);
		for (int i = 0; i < len; i++)
		{
			wtitle[i] = (wchar_t)title[i];
		}
	}
	SetWindowTextW(hWnd, wtitle.c_str());
}

std::optional<int> Window::ProcessMessages() noexcept
{
	MSG msg;
	while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
	{
		if (msg.message == WM_QUIT)
		{
			return (int)msg.wParam;
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return {};
}

HWND Window::GetHWND() const noexcept
{
	return hWnd;
}

Graphics& Window::Gfx()
{
	if (!pGfx)
	{
		throw NoGfxException(__LINE__, __FILE__);
	}
	return *pGfx;
}

LRESULT CALLBACK Window::HandleMsgSetup(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept
{
	if (msg == WM_NCCREATE)
	{
		const CREATESTRUCTW* const pCreate = reinterpret_cast<CREATESTRUCTW*>(lParam);
		Window* const pWnd = static_cast<Window*>(pCreate->lpCreateParams);
		SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pWnd));
		SetWindowLongPtr(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&Window::HandleMsgThunk));
		return pWnd->HandleMsg(hWnd, msg, wParam, lParam);
	}
	return DefWindowProcW(hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK Window::HandleMsgThunk(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept
{
	Window* const pWnd = reinterpret_cast<Window*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
	return pWnd->HandleMsg(hWnd, msg, wParam, lParam);
}

LRESULT Window::HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept
{
	// 如果窗口正在关闭，直接返回默认处理
	if (s_isClosing)
	{
		return DefWindowProcW(hWnd, msg, wParam, lParam);
	}

	switch (msg)
	{
	case WM_MOUSEMOVE:
	{
		const POINTS pt = MAKEPOINTS(lParam);
		mouse.OnMouseMove(pt.x, pt.y);
		return 0;
	}
	case WM_LBUTTONDOWN:
	{
		const POINTS pt = MAKEPOINTS(lParam);
		mouse.OnLeftPressed(pt.x, pt.y);
		return 0;
	}
	case WM_RBUTTONDOWN:
	{
		const POINTS pt = MAKEPOINTS(lParam);
		mouse.OnRightPressed(pt.x, pt.y);
		return 0;
	}
	case WM_RBUTTONUP:
	{
		const POINTS pt = MAKEPOINTS(lParam);
		mouse.OnRightReleased(pt.x, pt.y);
		return 0;
	}
	case WM_MOUSEWHEEL:
	{
		const POINTS pt = MAKEPOINTS(lParam);
		const int delta = GET_WHEEL_DELTA_WPARAM(wParam);
		mouse.OnWheelDelta(pt.x, pt.y, delta);
		return 0;
	}
	case WM_CLOSE:
		PostQuitMessage(0);
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProcW(hWnd, msg, wParam, lParam);
}

// Exception 实现
Window::Exception::Exception(int line, const char* file) noexcept
	:
	line(line),
	file(file)
{
}

const char* Window::Exception::what() const noexcept
{
	std::ostringstream oss;
	oss << GetType() << "\n" << GetOriginString();
	whatBuffer = oss.str();
	return whatBuffer.c_str();
}

const char* Window::Exception::GetType() const noexcept
{
	return "Window Exception";
}

std::string Window::Exception::GetOriginString() const noexcept
{
	std::ostringstream oss;
	oss << "[File] " << file << "\n[Line] " << line;
	return oss.str();
}

// Window Exception Stuff
std::string Window::Exception::TranslateErrorCode(HRESULT hr) noexcept
{
	char* pMsgBuf = nullptr;
	DWORD nMsgLen = FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr,
		hr,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPSTR)&pMsgBuf,
		0,
		nullptr
	);

	if (nMsgLen == 0)
	{
		return "Unknown error code";
	}

	std::string msg(pMsgBuf, nMsgLen);
	LocalFree(pMsgBuf);

	// 去掉末尾的换行符
	while (!msg.empty() && (msg.back() == '\n' || msg.back() == '\r'))
	{
		msg.pop_back();
	}

	return msg;
}

// HrException 实现
Window::HrException::HrException(int line, const char* file, HRESULT hr) noexcept
	:
	Exception(line, file),
	hr(hr)
{
}

const char* Window::HrException::what() const noexcept
{
	std::ostringstream oss;
	oss << GetType() << std::endl
		<< "[Error Code] 0x" << std::hex << std::uppercase << GetErrorCode()
		<< std::dec << " (" << (unsigned long)GetErrorCode() << ")" << std::endl
		<< "[Description] " << GetErrorDescription() << std::endl
		<< GetOriginString();
	whatBuffer = oss.str();
	return whatBuffer.c_str();
}

const char* Window::HrException::GetType() const noexcept
{
	return "Window HrException";
}

HRESULT Window::HrException::GetErrorCode() const noexcept
{
	return hr;
}

std::string Window::HrException::GetErrorDescription() const noexcept
{
	return TranslateErrorCode(hr);
}

std::string Window::HrException::TranslateErrorCode(HRESULT hr) noexcept
{
	char* pMsgBuf = nullptr;
	FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr,
		hr,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		reinterpret_cast<LPSTR>(&pMsgBuf),
		0,
		nullptr
	);

	if (pMsgBuf == nullptr)
	{
		return "Unknown error";
	}

	std::string errorString = pMsgBuf;
	LocalFree(pMsgBuf);

	while (!errorString.empty() && (errorString.back() == '\n' || errorString.back() == '\r'))
	{
		errorString.pop_back();
	}

	return errorString;
}

// NoGfxException 实现
const char* Window::NoGfxException::GetType() const noexcept
{
	return "Window NoGfxException";
}