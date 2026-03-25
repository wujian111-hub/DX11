#pragma once
#include "Win.h"
#include "Exception.h"
#include "Graphics.h"
#include <string>
#include <optional>
#include <memory>
#include "Mouse.h"
#include <vector>

class Window
{
public:
	// 静态标志
	static bool IsImGuiInitialized() { return s_imguiInitialized; }
	static void SetImGuiInitialized(bool initialized) { s_imguiInitialized = initialized; }
	static bool IsClosing() { return s_isClosing; }
	static void SetClosing(bool closing) { s_isClosing = closing; }

	Mouse mouse;

	class Exception : public std::exception
	{
	public:
		static std::string TranslateErrorCode(HRESULT hr) noexcept;
		Exception(int line, const char* file) noexcept;
		const char* what() const noexcept override;
		virtual const char* GetType() const noexcept;
		std::string GetOriginString() const noexcept;
	private:
		int line;
		std::string file;
		mutable std::string whatBuffer;
	};

	class HrException : public Exception
	{
	public:
		HrException(int line, const char* file, HRESULT hr) noexcept;
		const char* what() const noexcept override;
		const char* GetType() const noexcept override;
		static std::string TranslateErrorCode(HRESULT hr) noexcept;
		HRESULT GetErrorCode() const noexcept;
		std::string GetErrorDescription() const noexcept;
	private:
		HRESULT hr;
		mutable std::string whatBuffer;
	};

	class NoGfxException : public Exception
	{
	public:
		using Exception::Exception;
		const char* GetType() const noexcept override;
	};

private:
	static bool s_imguiInitialized;  // 添加静态成员
	static bool s_isClosing;         // 添加静态成员

	class WindowClass
	{
	public:
		static const wchar_t* GetName() noexcept;
		static HINSTANCE GetInstance() noexcept;
	private:
		WindowClass() noexcept;
		~WindowClass();
		WindowClass(const WindowClass&) = delete;
		WindowClass& operator=(const WindowClass&) = delete;
		static constexpr const wchar_t* wndClassName = L"Chili Window";
		static WindowClass wndClass;
		HINSTANCE hInst;
	};

public:
	Window(int width, int height, const char* name);
	~Window();
	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;

	void SetTitle(const std::string& title);
	static std::optional<int> ProcessMessages() noexcept;
	HWND GetHWND() const noexcept;
	Graphics& Gfx();

private:
	static LRESULT CALLBACK HandleMsgSetup(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept;
	static LRESULT CALLBACK HandleMsgThunk(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept;
	LRESULT HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept;

private:
	int width;
	int height;
	HWND hWnd;
	std::string title;
	bool isRunning;
	std::unique_ptr<Graphics> pGfx;
};