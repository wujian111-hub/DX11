#include "Window.h"
#include "App.h" 
#include "Mouse.h"
/*#include "WindowsMessageMap.h"
#include <string>

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static WindowsMessageMap mm;
	OutputDebugStringW(mm(msg, lParam, wParam).c_str());

    switch (msg)
    {
    case WM_CLOSE:
        PostQuitMessage(69);
        break;
    case WM_KEYDOWN:
        if (wParam == 'F')
        {
			SetWindowTextW(hwnd, L"F was pressed");
        }
		break;
    case WM_KEYUP:
		if (wParam == 'F')
        {
            SetWindowTextW(hwnd, L"F was up");
        }
		break;
    case WM_CHAR:
    {
        static std::wstring title;
        title.push_back(static_cast<wchar_t>(wParam));
        SetWindowTextW(hwnd, title.c_str());
    }
        break;
    case WM_LBUTTONDOWN:
       SetWindowTextW(hwnd, L"Left mouse button down");
        {
            POINTS pt = MAKEPOINTS(lParam);
            wchar_t title[100];
            wsprintfW(title, L"(%d, %d)", pt.x, pt.y);
            SetWindowTextW(hwnd, title);
            return 0;
    }
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}*/
int CALLBACK WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nCmdShow)
{
    /*const auto pclassName = L"hw3dbutts";
    WNDCLASSEX wc = { 0 };
    wc.cbSize = sizeof(wc);
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = nullptr;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = nullptr;
    wc.lpszClassName = pclassName;
    wc.hIconSm = nullptr;

    if (!RegisterClassEx(&wc))
        return 0;

    HWND hwnd = CreateWindowEx(
        0,
        pclassName,
        L"Happy Hard Window",
        WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU,
        200, 200, 640, 480,
        nullptr, nullptr, hInstance, nullptr);

    if (!hwnd)
        return 0;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);*/

    std::unique_ptr<Window> pWnd;

    try
    {
        App App;
        return App.Go();
        //Window wnd(640, 480, "My Window");

        //MSG msg;
        //while (GetMessage(&msg, nullptr, 0, 0) > 0)
        //{
        //    TranslateMessage(&msg);
         //   DispatchMessage(&msg);
        //}
        //return (int)msg.wParam;
    }
    catch (const Window::HrException& e)
    {
        MessageBoxA(nullptr, e.what(), "Error", MB_OK | MB_ICONERROR);
        return -1;
    }
    catch (const std::exception& e)
    {
        MessageBoxA(nullptr, e.what(), "Error", MB_OK | MB_ICONERROR);
        return -1;
    }
}