#include "stdafx.h"
#include "Graphics.h"
#include <string>
#include <d3d11.h>
#include <dxgi.h>
#include <cmath>
#include <d3dcompiler.h>
#include <wrl/client.h> 
#include <DirectXMath.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "D3DCompiler.lib")

namespace dx = DirectX;

// 图形错误处理宏
#ifndef GFX_THROW_INFO
#define GFX_THROW_INFO(hr) \
	if (FAILED(hr)) \
	{ \
		throw std::runtime_error("Graphics error"); \
	}
#endif

// ============================================
// 构造函数：初始化 DirectX 11 设备和交换链
// ============================================
Graphics::Graphics(HWND hWnd, int width, int height)
	:
	width(width),
	height(height)
{
	// 配置交换链描述
	DXGI_SWAP_CHAIN_DESC sd = {};
	sd.BufferDesc.Width = width;                          // 缓冲区宽度
	sd.BufferDesc.Height = height;                         // 缓冲区高度
	sd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;    // 像素格式（BGRA 8位）
	sd.SampleDesc.Count = 1;                              // 多重采样数量（1 = 无 MSAA）
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // 缓冲区用途：渲染目标输出
	sd.BufferCount = 2;                              // 双缓冲
	sd.OutputWindow = hWnd;                           // 关联的窗口句柄
	sd.Windowed = TRUE;                           // 窗口模式
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;  // 翻转效果（现代方式）

	// 创建 Direct3D 11 设备、设备上下文和交换链
	GFX_THROW_INFO(D3D11CreateDeviceAndSwapChain(
		nullptr,                   // 使用默认适配器
		D3D_DRIVER_TYPE_HARDWARE,  // 硬件加速
		nullptr,                   // 不使用软件驱动
		0,                         // 调试标志（0 = 无调试）
		nullptr,                   // 特性级别数组（null = 使用默认列表）
		0,                         // 特性级别数组大小
		D3D11_SDK_VERSION,         // SDK 版本
		&sd,                       // 交换链描述
		&pSwap,                    // 返回交换链
		&pDevice,                  // 返回设备
		nullptr,                   // 返回实际特性级别（此处忽略）
		&pContext                  // 返回设备上下文
	));

	// 获取后缓冲区并创建渲染目标视图
	wrl::ComPtr<ID3D11Texture2D> pBackBuffer;
	GFX_THROW_INFO(pSwap->GetBuffer(0, __uuidof(ID3D11Texture2D), &pBackBuffer));
	GFX_THROW_INFO(pDevice->CreateRenderTargetView(pBackBuffer.Get(), nullptr, &pTarget));
}

// ============================================
// 清理缓冲区（简单版本，带 noexcept）
// ============================================
void Graphics::ClearBuffer(float red, float green, float blue) noexcept
{
	const float color[] = { red, green, blue, 1.0f };
	pContext->ClearRenderTargetView(pTarget.Get(), color);
}

// ============================================
// 开始帧：清除渲染目标并输出调试信息
// ============================================
void Graphics::BeginFrame(float red, float green, float blue)
{
	assert(pTarget != nullptr);
	assert(pContext != nullptr);

	// 清除渲染目标视图（设置背景色）
	const float color[] = { red, green, blue, 1.0f };
	pContext->ClearRenderTargetView(pTarget.Get(), color);

	// 输出清除颜色到调试窗口（用于调试）
	char buf[100];
	sprintf_s(buf, "Clear with color: %.1f, %.1f, %.1f\n", red, green, blue);
	OutputDebugStringA(buf);
}

// ============================================
// 结束帧：交换前后缓冲区，显示渲染结果
// ============================================
void Graphics::EndFrame()
{
	pSwap->Present(1, 0);  // 垂直同步开启，立即显示
}

// ============================================
// 绘制平截头体（棱台）3D 物体
// 参数：angle - 旋转角度，x, y - 平移位置
// ============================================
void Graphics::Frustum(float angle, float x, float y)
{
	// --------------------------------------------------------
	// 1. 定义顶点数据结构（位置 + 颜色）
	// --------------------------------------------------------
	struct Vertex
	{
		struct
		{
			float x;
			float y;
			float z;
		} pos;        // 位置（3D 坐标）
		struct
		{
			unsigned char r;
			unsigned char g;
			unsigned char b;
			unsigned char a;
		} color;      // 颜色（RGBA，0-255）
	};

	// 顶点数据：8 个顶点（前后两个矩形面，各 4 个顶点）
	const Vertex vertices[] =
	{
		// 前面（近平面）- 红色调
		{ -1.0f, -1.0f, -1.0f, 255,   0,   0, 255 },
		{  1.0f, -1.0f, -1.0f,   0, 255,   0, 255 },
		{ -1.0f,  1.0f, -1.0f,   0,   0, 255, 255 },
		{  1.0f,  1.0f, -1.0f, 255, 255,   0, 255 },

		// 后面（远平面）- 青色/白色调
		{ -0.5f, -0.5f,  1.0f, 255,   0, 255, 255 },
		{  0.5f, -0.5f,  1.0f,   0, 255, 255, 255 },
		{ -0.5f,  0.5f,  1.0f,   0,   0,   0, 255 },
		{  0.5f,  0.5f,  1.0f, 255, 255, 255, 255 },
	};

	// 创建顶点缓冲区
	wrl::ComPtr<ID3D11Buffer> pVertexBuffer;
	D3D11_BUFFER_DESC bd = {};
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;  // 绑定为顶点缓冲区
	bd.Usage = D3D11_USAGE_DEFAULT;       // GPU 读写
	bd.CPUAccessFlags = 0u;                        // CPU 不需要访问
	bd.MiscFlags = 0u;
	bd.ByteWidth = sizeof(vertices);          // 缓冲区大小
	bd.StructureByteStride = sizeof(Vertex);            // 每个顶点的步长

	D3D11_SUBRESOURCE_DATA sd = {};
	sd.pSysMem = vertices;                                  // 将顶点数据复制到 GPU

	GFX_THROW_INFO(pDevice->CreateBuffer(&bd, &sd, &pVertexBuffer));

	// --------------------------------------------------------
	// 2. 定义索引数据（三角形图元）
	// --------------------------------------------------------
	const unsigned short indices[] =
	{
		// 前面（2 个三角形）
		0, 2, 1,  1, 2, 3,

		// 后面（2 个三角形）
		4, 5, 6,  5, 7, 6,

		// 左面
		0, 4, 2,  2, 4, 6,

		// 右面
		1, 3, 5,  3, 7, 5,

		// 下面
		0, 1, 4,  1, 5, 4,

		// 上面
		2, 6, 3,  3, 6, 7,
	};

	// 创建索引缓冲区
	wrl::ComPtr<ID3D11Buffer> pIndexBuffer;
	D3D11_BUFFER_DESC ibd = {};
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;   // 绑定为索引缓冲区
	ibd.Usage = D3D11_USAGE_DEFAULT;
	ibd.CPUAccessFlags = 0u;
	ibd.MiscFlags = 0u;
	ibd.ByteWidth = sizeof(indices);           // 缓冲区大小
	ibd.StructureByteStride = sizeof(unsigned short);    // 每个索引的步长

	D3D11_SUBRESOURCE_DATA isd = {};
	isd.pSysMem = indices;                               // 将索引数据复制到 GPU

	GFX_THROW_INFO(pDevice->CreateBuffer(&ibd, &isd, &pIndexBuffer));

	// --------------------------------------------------------
	// 3. 定义常量缓冲区（变换矩阵）
	// --------------------------------------------------------
	struct ConstantBuffer
	{
		dx::XMMATRIX transform;  // 变换矩阵（世界、视图、投影组合）
	};

	// 构建变换矩阵（旋转 + 缩放 + 平移 + 透视投影）
	ConstantBuffer cb =
	{
		{
			dx::XMMatrixTranspose(
				dx::XMMatrixRotationZ(angle) *           // Z 轴旋转
				dx::XMMatrixRotationX(angle) *           // X 轴旋转
				dx::XMMatrixScaling(3.0f / 4.0f, 1.0f, 1.0f) *  // 缩放
				dx::XMMatrixTranslation(x, y, 4.0f) *           // 平移
				dx::XMMatrixPerspectiveLH(1.0f, 3.0f / 4.0f, 0.5f, 10.0f)  // 透视投影
			)
		}
	};

	// 创建常量缓冲区
	wrl::ComPtr<ID3D11Buffer> pConstantBuffer;
	D3D11_BUFFER_DESC cbd = {};
	cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;  // 绑定为常量缓冲区
	cbd.Usage = D3D11_USAGE_DEFAULT;
	cbd.CPUAccessFlags = 0;
	cbd.MiscFlags = 0u;
	cbd.ByteWidth = sizeof(ConstantBuffer);      // 缓冲区大小
	cbd.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA csd = {};
	csd.pSysMem = &cb;                                     // 将矩阵数据复制到 GPU

	GFX_THROW_INFO(pDevice->CreateBuffer(&cbd, &csd, &pConstantBuffer));

	// --------------------------------------------------------
	// 4. 设置着色器和输入布局
	// --------------------------------------------------------

	// 设置常量缓冲区到顶点着色器
	pContext->VSSetConstantBuffers(0, 1, pConstantBuffer.GetAddressOf());

	// 设置顶点缓冲区
	const UINT stride = sizeof(Vertex);    // 顶点步长
	const UINT offset = 0u;                // 偏移量
	pContext->IASetVertexBuffers(0, 1, pVertexBuffer.GetAddressOf(), &stride, &offset);

	// 设置索引缓冲区
	pContext->IASetIndexBuffer(pIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

	// 加载并创建像素着色器
	wrl::ComPtr<ID3D11PixelShader> pPixelShader;
	wrl::ComPtr<ID3DBlob> pBlob;
	GFX_THROW_INFO(D3DReadFileToBlob(L"PixelShader.cso", &pBlob));
	GFX_THROW_INFO(pDevice->CreatePixelShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, &pPixelShader));
	pContext->PSSetShader(pPixelShader.Get(), nullptr, 0);

	// 加载并创建顶点着色器
	wrl::ComPtr<ID3D11VertexShader> pVertexShader;
	GFX_THROW_INFO(D3DReadFileToBlob(L"VertexShader.cso", &pBlob));
	GFX_THROW_INFO(pDevice->CreateVertexShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, &pVertexShader));
	pContext->VSSetShader(pVertexShader.Get(), nullptr, 0);

	// 创建输入布局（定义顶点数据的解析方式）
	wrl::ComPtr<ID3D11InputLayout> pInputLayout;
	const D3D11_INPUT_ELEMENT_DESC ied[] =
	{
		{ "Position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,   D3D11_INPUT_PER_VERTEX_DATA, 0 },  // 位置：3 个 float
		{ "Color",    0, DXGI_FORMAT_R8G8B8A8_UNORM,   0, 12u, D3D11_INPUT_PER_VERTEX_DATA, 0 }   // 颜色：4 个 unsigned char
	};

	GFX_THROW_INFO(pDevice->CreateInputLayout(ied, (UINT)std::size(ied), pBlob->GetBufferPointer(), pBlob->GetBufferSize(), &pInputLayout));
	pContext->IASetInputLayout(pInputLayout.Get());

	// --------------------------------------------------------
	// 5. 设置渲染状态并执行绘制
	// --------------------------------------------------------

	// 设置渲染目标
	pContext->OMSetRenderTargets(1, pTarget.GetAddressOf(), nullptr);

	// 设置图元拓扑类型（三角形列表）
	pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 设置视口
	D3D11_VIEWPORT vp;
	vp.Width = (float)width;
	vp.Height = (float)height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0.0f;
	vp.TopLeftY = 0.0f;
	pContext->RSSetViewports(1, &vp);

	// 执行索引绘制
	pContext->DrawIndexed((UINT)std::size(indices), 0, 0);
}