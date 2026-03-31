#include "stdafx.h"
#include "App.h"
#include "Mouse.h"
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

App::App()
	:
	wnd(1280, 720, "Blinn-Phong Sphere"),
	m_isRunning(true)
{
	OutputDebugStringA("App constructor started\n");

	// 确保 Graphics 已经创建（ImGui 已经在 Graphics 构造函数中初始化）
	if (!wnd.Gfx().GetDevice() || !wnd.Gfx().GetContext())
	{
		OutputDebugStringA("Graphics device/context is null!\n");
		return;
	}

	// 设置 ImGui 已初始化标志
	Window::SetImGuiInitialized(true);

	OutputDebugStringA("App constructor completed\n");
}

App::~App()
{
	OutputDebugStringA("App destructor started\n");

	// 先清除标志，防止窗口消息再调用 ImGui
	Window::SetImGuiInitialized(false);

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
	// dt in seconds
	const float dt = timer.Mark();

	// ========== 控制变量 ==========
	static float rotationAngle = 0.0f;
	static float posX = 0.0f, posY = 0.0f, posZ = 0.0f;
	static ImVec4 clearColor = ImVec4(0.1f, 0.1f, 0.2f, 1.0f);

	// 材质控制
	static ImVec4 diffuseColor = ImVec4(0.8f, 0.3f, 0.3f, 1.0f);
	static ImVec4 specularColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	static float shininess = 32.0f;
	static float lightDirX = 1.0f, lightDirY = -1.0f, lightDirZ = 0.5f;

	// 更新材质参数
	wnd.Gfx().SetMaterialDiffuse(diffuseColor.x, diffuseColor.y, diffuseColor.z);
	wnd.Gfx().SetMaterialSpecular(specularColor.x, specularColor.y, specularColor.z);
	wnd.Gfx().SetMaterialShininess(shininess);
	wnd.Gfx().SetLightDir(lightDirX, lightDirY, lightDirZ);

	// ========== 第一步：绘制 3D 场景 ==========
	wnd.Gfx().BeginFrame(clearColor.x, clearColor.y, clearColor.z);
	wnd.Gfx().DrawSolidSphere(dt, rotationAngle * 3.14159f / 180.0f, posX, posY, posZ);

	// ========== 第二步：开始 ImGui 帧 ==========
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// ========== 主控制面板 ==========
	// 设置窗口位置在左上角，避免被遮挡
	ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(400, 520), ImGuiCond_FirstUseEver);

	ImGui::Begin("Blinn-Phong Control Panel", nullptr, ImGuiWindowFlags_NoCollapse);

	ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
	ImGui::Separator();

	// 变换控制
	ImGui::Text("Transform");
	ImGui::SliderAngle("Rotation", &rotationAngle);
	ImGui::SliderFloat("Position X", &posX, -2.0f, 2.0f);
	ImGui::SliderFloat("Position Y", &posY, -2.0f, 2.0f);
	ImGui::SliderFloat("Position Z", &posZ, -2.0f, 2.0f);

	ImGui::Separator();

	// 材质控制
	ImGui::Text("Material");
	ImGui::ColorEdit3("Diffuse Color", (float*)&diffuseColor);
	ImGui::ColorEdit3("Specular Color", (float*)&specularColor);
	ImGui::SliderFloat("Shininess", &shininess, 1.0f, 128.0f);

	ImGui::Separator();

	// 光源控制
	ImGui::Text("Light Direction");
	ImGui::SliderFloat("Light X", &lightDirX, -2.0f, 2.0f);
	ImGui::SliderFloat("Light Y", &lightDirY, -2.0f, 2.0f);
	ImGui::SliderFloat("Light Z", &lightDirZ, -2.0f, 2.0f);

	ImGui::Separator();

	// 背景控制
	ImGui::Text("Environment");
	ImGui::ColorEdit3("Background Color", (float*)&clearColor);

	ImGui::Separator();

	// 信息显示
	ImGui::Text("Info");
	ImGui::Text("Light Direction: (%.2f, %.2f, %.2f)", lightDirX, lightDirY, lightDirZ);
	ImGui::Text("Shininess: %.0f", shininess);

	ImGui::End();

	// ========== 输入测试窗口 ==========
	ImGui::SetNextWindowPos(ImVec2(420, 10), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);

	ImGui::Begin("Input Test", nullptr, ImGuiWindowFlags_NoCollapse);
	static char inputText[128] = "";
	ImGui::Text("Test Text Input:");
	ImGui::InputText("##input", inputText, sizeof(inputText));
	if (ImGui::Button("Submit"))
	{
		OutputDebugStringA(("Input: " + std::string(inputText) + "\n").c_str());
	}
	ImGui::SameLine();
	if (ImGui::Button("Clear"))
	{
		memset(inputText, 0, sizeof(inputText));
	}
	ImGui::Text("Submitted text will appear");
	ImGui::Text("in debug output window.");
	ImGui::End();

	// ========== 可选：ImGui Demo 窗口（默认关闭）==========
	static bool show_demo_window = false;
	if (show_demo_window)
	{
		ImGui::ShowDemoWindow(&show_demo_window);
	}

	// ========== 第三步：渲染 ImGui ==========
	ImGui::Render();
	wnd.Gfx().BeginImGuiRender();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	// ========== 第四步：结束帧 ==========
	wnd.Gfx().EndFrame();
}