
#include "App.h"
#include "stdafx.h"
#include "Mouse.h"
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

App::App()
	:
	wnd(1280, 720, "My Window"),
	m_isRunning(true)  // 初始化运行标志
{
	OutputDebugStringA("App constructor started\n");

	// 确保 Graphics 已经创建
	if (!wnd.Gfx().GetDevice() || !wnd.Gfx().GetContext())
	{
		OutputDebugStringA("Graphics device/context is null!\n");
		return;
	}

	// 初始化 ImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	(void)io;
	ImGui::StyleColorsDark();

	// 初始化 Win32 后端
	if (!ImGui_ImplWin32_Init(wnd.GetHWND()))
	{
		OutputDebugStringA("ImGui_ImplWin32_Init failed!\n");
		return;
	}

	// 初始化 DX11 后端
	if (!ImGui_ImplDX11_Init(wnd.Gfx().GetDevice(), wnd.Gfx().GetContext()))
	{
		OutputDebugStringA("ImGui_ImplDX11_Init failed!\n");
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
		return;
	}

	// 设置 ImGui 已初始化标志（只调用一次）
	Window::SetImGuiInitialized(true);

	OutputDebugStringA("App constructor completed\n");
}

App::~App()
{
	OutputDebugStringA("App destructor started\n");

	// 先清除标志，防止窗口消息再调用 ImGui
	Window::SetImGuiInitialized(false);

	// 清理 ImGui 后端
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();

	// 销毁 ImGui 上下文
	if (ImGui::GetCurrentContext() != nullptr)
	{
		ImGui::DestroyContext();
	}

	// 强制设置上下文为空
	ImGui::SetCurrentContext(nullptr);

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
	// 开始 ImGui 帧
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// ========== 声明所有控制变量 ==========
	static float rotationAngle = 0.0f;
	static bool autoRotate = true;
	static float rotationSpeed = 1.0f;
	static float posX = 0.0f, posY = 0.0f, posZ = 0.0f;
	static float scaleTop = 0.5f;
	static float scaleBottom = 1.0f;
	static float scaleHeight = 1.0f;
	static ImVec4 topColor = ImVec4(1.0f, 0.5f, 0.2f, 1.0f);
	static ImVec4 bottomColor = ImVec4(0.2f, 0.3f, 0.8f, 1.0f);
	static ImVec4 sideColor = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
	static ImVec4 clearColor = ImVec4(0.2f, 0.3f, 0.5f, 1.0f);

	// 投影类型变量
	static int projectionIndex = 0;  // 0:透视投影, 1:正交投影
	const char* projectionItems[] = { "Perspective", "Orthographic" };
	static float orthoViewSize = 5.0f;  // 正交投影视野大小

	// 自动旋转
	if (autoRotate)
	{
		rotationAngle += rotationSpeed * 0.5f;
		if (rotationAngle > 360.0f) rotationAngle -= 360.0f;
	}

	// ========== 控制面板 ==========
	ImGui::Begin("Control Panel");

	ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
	ImGui::Separator();

	// ========== 投影类型选择 ==========
	ImGui::Text("Projection");
	if (ImGui::Combo("Type", &projectionIndex, projectionItems, IM_ARRAYSIZE(projectionItems)))
	{
		if (projectionIndex == 0)
			wnd.Gfx().SetProjectionType(Graphics::ProjectionType::Perspective);
		else
			wnd.Gfx().SetProjectionType(Graphics::ProjectionType::Orthographic);
	}

	// 正交投影视野大小滑块（仅在正交投影时显示）
	if (projectionIndex == 1)
	{
		if (ImGui::SliderFloat("Ortho Size", &orthoViewSize, 2.0f, 15.0f))
		{
			wnd.Gfx().SetOrthoViewSize(orthoViewSize);
		}
	}
	ImGui::Separator();

	// 旋转控制
	ImGui::Text("Rotation");
	ImGui::SliderAngle("Angle", &rotationAngle);
	ImGui::SliderFloat("Speed", &rotationSpeed, 0.0f, 2.0f);
	ImGui::Checkbox("Auto Rotate", &autoRotate);
	ImGui::Separator();

	// 平移控制
	ImGui::Text("Translation");
	ImGui::SliderFloat("X", &posX, -2.0f, 2.0f);
	ImGui::SliderFloat("Y", &posY, -2.0f, 2.0f);
	ImGui::SliderFloat("Z", &posZ, -2.0f, 2.0f);
	ImGui::Separator();

	// 缩放控制
	ImGui::Text("Scale");
	ImGui::SliderFloat("Top Size", &scaleTop, 0.2f, 1.0f);
	ImGui::SliderFloat("Bottom Size", &scaleBottom, 0.5f, 1.5f);
	ImGui::SliderFloat("Height", &scaleHeight, 0.5f, 1.5f);
	ImGui::Separator();

	// 颜色控制
	ImGui::Text("Colors");
	ImGui::ColorEdit3("Top Color", (float*)&topColor);
	ImGui::ColorEdit3("Bottom Color", (float*)&bottomColor);
	ImGui::ColorEdit3("Side Color", (float*)&sideColor);
	ImGui::ColorEdit3("Background", (float*)&clearColor);
	ImGui::Separator();

	// 重置按钮
	if (ImGui::Button("Reset All"))
	{
		rotationAngle = 0.0f;
		autoRotate = true;
		rotationSpeed = 1.0f;
		posX = 0.0f; posY = 0.0f; posZ = 0.0f;
		scaleTop = 0.5f;
		scaleBottom = 1.0f;
		scaleHeight = 1.0f;
		topColor = ImVec4(1.0f, 0.5f, 0.2f, 1.0f);
		bottomColor = ImVec4(0.2f, 0.3f, 0.8f, 1.0f);
		sideColor = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
		clearColor = ImVec4(0.2f, 0.3f, 0.5f, 1.0f);
		projectionIndex = 0;
		orthoViewSize = 5.0f;
		wnd.Gfx().SetProjectionType(Graphics::ProjectionType::Perspective);
		wnd.Gfx().SetOrthoViewSize(5.0f);
	}

	ImGui::End();

	// ========== ImGui Demo 窗口 ==========
	static bool show_demo_window = true;
	if (show_demo_window)
	{
		ImGui::ShowDemoWindow(&show_demo_window);
	}

	// ========== 输入测试窗口 ==========
	ImGui::Begin("Input Test");
	static char inputText[128] = "";
	ImGui::InputText("Text Input", inputText, sizeof(inputText));
	if (ImGui::Button("Submit"))
	{
		OutputDebugStringA(("Input: " + std::string(inputText) + "\n").c_str());
	}
	ImGui::End();

	// ========== 绘制 3D 场景 ==========
	wnd.Gfx().BeginFrame(clearColor.x, clearColor.y, clearColor.z);

	wnd.Gfx().Frustum(
		rotationAngle * 3.14159f / 180.0f,
		posX, posY, posZ,
		scaleTop, scaleBottom, scaleHeight,
		topColor.x, topColor.y, topColor.z,
		bottomColor.x, bottomColor.y, bottomColor.z,
		sideColor.x, sideColor.y, sideColor.z
	);

	// 渲染 ImGui
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	// 结束 3D 场景帧
	wnd.Gfx().EndFrame();
}