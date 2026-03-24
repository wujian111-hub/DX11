#include "stdafx.h"
#include "Graphics.h"
#include <stdexcept>
#include <string>
#include <d3d11.h>
#include <dxgi.h>
#include <cmath>
#include <cassert>
#include <d3dcompiler.h>
#include <wrl/client.h> 
#include <DirectXMath.h>

// 链接 DirectX 库
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "D3DCompiler.lib")

namespace dx = DirectX;

// 错误处理宏：检查 HRESULT，失败时显示错误信息并抛出异常
#ifndef GFX_THROW_INFO
#define GFX_THROW_INFO(hr) \
	if (FAILED(hr)) \
	{ \
		char buf[256]; \
		sprintf_s(buf, "Graphics error at %s line %d: HRESULT = 0x%08X\n", __FILE__, __LINE__, hr); \
		OutputDebugStringA(buf); \
		MessageBoxA(nullptr, buf, "DirectX Error", MB_OK); \
		throw std::runtime_error(buf); \
	}
#endif

// ============================================================================
// 构造函数：初始化 Direct3D 设备、交换链、渲染目标和深度缓冲区
// ============================================================================
Graphics::Graphics(HWND hWnd, int width, int height)
	: width(width)
	, height(height)
{
	// 1. 配置交换链描述
	DXGI_SWAP_CHAIN_DESC sd = {};
	sd.BufferDesc.Width = width;                    // 缓冲区宽度
	sd.BufferDesc.Height = height;                  // 缓冲区高度
	sd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;  // 像素格式
	sd.SampleDesc.Count = 1;                        // 不使用多重采样
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;   // 用作渲染目标
	sd.BufferCount = 2;                             // 双缓冲
	sd.OutputWindow = hWnd;                         // 输出窗口句柄
	sd.Windowed = TRUE;                             // 窗口模式
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;  // 交换效果

	// 2. 创建设备、设备上下文和交换链
	GFX_THROW_INFO(D3D11CreateDeviceAndSwapChain(
		nullptr,                    // 使用默认适配器
		D3D_DRIVER_TYPE_HARDWARE,   // 使用硬件驱动
		nullptr,                    // 不使用软件设备
		0,                          // 运行时标志
		nullptr,                    // 不指定特性等级列表
		0,                          // 特性等级数量
		D3D11_SDK_VERSION,          // SDK 版本
		&sd,                        // 交换链描述
		&pSwap,                     // 交换链指针
		&pDevice,                   // 设备指针
		nullptr,                    // 获取实际特性等级（不需要）
		&pContext                   // 设备上下文指针
	));

	// 3. 创建渲染目标视图
	wrl::ComPtr<ID3D11Texture2D> pBackBuffer;
	GFX_THROW_INFO(pSwap->GetBuffer(0, __uuidof(ID3D11Texture2D), &pBackBuffer));
	GFX_THROW_INFO(pDevice->CreateRenderTargetView(pBackBuffer.Get(), nullptr, &pTarget));


	D3D11_DEPTH_STENCIL_DESC dsDesc = {};
	dsDesc.DepthEnable = TRUE;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	wrl::ComPtr<ID3D11DepthStencilState>pDSState;
	GFX_THROW_INFO(pDevice->CreateDepthStencilState(&dsDesc, &pDSState));
	pContext->OMSetDepthStencilState(pDSState.Get(), 1u);

	// 4. 创建深度缓冲区纹理
	wrl::ComPtr<ID3D11Texture2D> pDepthStencil;
	D3D11_TEXTURE2D_DESC depthDesc = {};
	depthDesc.Width = width;                        // 宽度
	depthDesc.Height = height;                      // 高度
	depthDesc.MipLevels = 1;                        // 不使用多级纹理
	depthDesc.ArraySize = 1;                        // 纹理数组大小
	depthDesc.Format = DXGI_FORMAT_D32_FLOAT; // 24位深度 + 8位模板
	depthDesc.SampleDesc.Count = 1;                 // 不使用多重采样
	depthDesc.SampleDesc.Quality = 0;
	depthDesc.Usage = D3D11_USAGE_DEFAULT;          // GPU 读写
	depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL; // 绑定为深度模板缓冲区
	GFX_THROW_INFO(pDevice->CreateTexture2D(&depthDesc, nullptr, &pDepthStencil));

	// 5. 创建深度模板视图
	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = depthDesc.Format;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Texture2D.MipSlice = 0;
	GFX_THROW_INFO(pDevice->CreateDepthStencilView(pDepthStencil.Get(), &dsvDesc, &pDSV));

	// 6. 将渲染目标和深度缓冲区绑定到输出合并阶段
	pContext->OMSetRenderTargets(1, pTarget.GetAddressOf(), pDSV.Get());
}

// ============================================================================
// 清空颜色缓冲区
// ============================================================================
void Graphics::ClearBuffer(float red, float green, float blue) noexcept
{
	const float color[] = { red, green, blue, 1.0f };
	pContext->ClearRenderTargetView(pTarget.Get(), color);
	//pContext->ClearDepthStencilView(pDSV.Get(), D3D11_CLEAR_DEPTH,1.0f,0u);
}

// ============================================================================
// 开始帧：清空颜色缓冲区
// ============================================================================
void Graphics::BeginFrame(float red, float green, float blue)
{
	assert(pTarget != nullptr);
	assert(pContext != nullptr);

	// 清空颜色缓冲区
	const float color[] = { red, green, blue, 1.0f };
	pContext->ClearRenderTargetView(pTarget.Get(), color);

	pContext->ClearDepthStencilView(pDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0u);

	// 确保渲染目标已绑定
	pContext->OMSetRenderTargets(1, pTarget.GetAddressOf(), pDSV.Get());
}

// ============================================================================
// 结束帧：交换前后缓冲区，显示图像
// ============================================================================
void Graphics::EndFrame()
{
	pSwap->Present(1, 0);  // 垂直同步，立即呈现
}

// ============================================================================
// 绘制视锥体（彩色立方体）
// 参数：
//   angle - 旋转角度（弧度）
//   x, z  - 在 XZ 平面上的平移量
// ============================================================================
void Graphics::Frustum(float angle, float x, float z)
{
	// ========== 1. 绑定渲染目标 ==========
	pContext->OMSetRenderTargets(1, pTarget.GetAddressOf(), nullptr);

	// ========== 2. 定义数据结构 ==========

	// 顶点结构：包含位置坐标
	struct Vertex
	{
		struct
		{
			float x;
			float y;
			float z;
		} pos;
	};

	// 常量缓冲区结构（像素着色器）：存储6个面的颜色
	struct ConstantBuffer2
	{
		struct
		{
			float r, g, b, a;  // 红、绿、蓝、透明度
		} face_color[6];        // 6个面，每个面一个颜色
	};

	// 常量缓冲区结构（顶点着色器）：存储变换矩阵
	struct ConstantBuffer
	{
		dx::XMMATRIX transform;  // 世界-视图-投影矩阵
	};

	// ========== 3. 初始化颜色数据（6个面，每个面不同颜色） ==========
	const ConstantBuffer2 cb2 =
	{
		{
			{1.0f, 0.0f, 1.0f, 1.0f},  // 紫色 - 前面
			{1.0f, 0.0f, 0.0f, 1.0f},  // 红色 - 右面
			{0.0f, 1.0f, 0.0f, 1.0f},  // 绿色 - 上面
			{0.0f, 0.0f, 1.0f, 1.0f},  // 蓝色 - 后面
			{1.0f, 1.0f, 0.0f, 1.0f},  // 黄色 - 左面
			{0.0f, 1.0f, 1.0f, 1.0f}   // 青色 - 下面
		}
	};

	// ========== 4. 创建颜色常量缓冲区 ==========
	wrl::ComPtr<ID3D11Buffer> pConstantBuffer2;
	D3D11_BUFFER_DESC cbd2 = {};
	cbd2.BindFlags = D3D11_BIND_CONSTANT_BUFFER;  // 绑定为常量缓冲区
	cbd2.Usage = D3D11_USAGE_DEFAULT;              // 默认使用方式
	cbd2.CPUAccessFlags = 0u;                      // CPU 不访问
	cbd2.MiscFlags = 0u;
	cbd2.ByteWidth = sizeof(ConstantBuffer2);      // 缓冲区大小
	cbd2.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA csd2 = {};
	csd2.pSysMem = &cb2;                           // 颜色数据
	GFX_THROW_INFO(pDevice->CreateBuffer(&cbd2, &csd2, &pConstantBuffer2));

	// ========== 5. 定义立方体顶点数据 ==========
	const Vertex vertices[] =
	{
		// 前面 4 个顶点 (z = -1)
		{-1.0f, -1.0f, -1.0f},  // 0: 左下前
		{ 1.0f, -1.0f, -1.0f},  // 1: 右下前
		{-1.0f,  1.0f, -1.0f},  // 2: 左上前
		{ 1.0f,  1.0f, -1.0f},  // 3: 右上前

		// 后面 4 个顶点 (z = 1)
		{-1.0f, -1.0f,  1.0f},  // 4: 左下后
		{ 1.0f, -1.0f,  1.0f},  // 5: 右下后
		{-1.0f,  1.0f,  1.0f},  // 6: 左上后
		{ 1.0f,  1.0f,  1.0f}   // 7: 右上后
	};

	// ========== 6. 创建顶点缓冲区 ==========
	wrl::ComPtr<ID3D11Buffer> pVertexBuffer;
	D3D11_BUFFER_DESC bd = {};
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;       // 绑定为顶点缓冲区
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.CPUAccessFlags = 0u;
	bd.MiscFlags = 0u;
	bd.ByteWidth = sizeof(vertices);               // 缓冲区大小
	bd.StructureByteStride = sizeof(Vertex);       // 每个顶点的大小

	D3D11_SUBRESOURCE_DATA sd = {};
	sd.pSysMem = vertices;                         // 顶点数据
	GFX_THROW_INFO(pDevice->CreateBuffer(&bd, &sd, &pVertexBuffer));

	// ========== 7. 定义索引数据（每个面由2个三角形组成） ==========
	const unsigned short indices[] =
	{
		// 前面 (z = -1)
		0, 1, 2, 2, 1, 3,
		// 右面 (x = 1)
		1, 5, 3, 3, 5, 7,
		// 上面 (y = 1)
		2, 3, 6, 6, 3, 7,
		// 后面 (z = 1)
		4, 6, 5, 5, 6, 7,
		// 左面 (x = -1)
		0, 2, 4, 4, 2, 6,
		// 下面 (y = -1)
		0, 4, 1, 1, 4, 5,
	};

	// ========== 8. 创建索引缓冲区 ==========
	wrl::ComPtr<ID3D11Buffer> pIndexBuffer;
	D3D11_BUFFER_DESC ibd = {};
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;       // 绑定为索引缓冲区
	ibd.Usage = D3D11_USAGE_DEFAULT;
	ibd.CPUAccessFlags = 0u;
	ibd.MiscFlags = 0u;
	ibd.ByteWidth = sizeof(indices);               // 缓冲区大小
	ibd.StructureByteStride = sizeof(unsigned short);

	D3D11_SUBRESOURCE_DATA isd = {};
	isd.pSysMem = indices;                         // 索引数据
	GFX_THROW_INFO(pDevice->CreateBuffer(&ibd, &isd, &pIndexBuffer));

	// ========== 9. 计算变换矩阵 ==========
	ConstantBuffer cb =
	{
		{
			dx::XMMatrixTranspose(
				// 旋转（绕 Z 轴和 X 轴）
				dx::XMMatrixRotationZ(angle) *
				dx::XMMatrixRotationX(angle) *
				// 缩放（使立方体适应屏幕宽高比）
				dx::XMMatrixScaling(3.0f / 4.0f, 1.0f, 1.0f) *
				// 平移（根据鼠标位置移动）
				dx::XMMatrixTranslation(x, 0.0f, z + 5.0f) *
				// 透视投影
				dx::XMMatrixPerspectiveLH(1.0f, 3.0f / 4.0f, 0.5f, 10.0f)
			)
		}
	};

	// ========== 10. 创建变换常量缓冲区 ==========
	wrl::ComPtr<ID3D11Buffer> pConstantBuffer;
	D3D11_BUFFER_DESC cbd = {};
	cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbd.Usage = D3D11_USAGE_DEFAULT;
	cbd.CPUAccessFlags = 0;
	cbd.MiscFlags = 0u;
	cbd.ByteWidth = sizeof(ConstantBuffer);
	cbd.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA csd = {};
	csd.pSysMem = &cb;                             // 矩阵数据
	GFX_THROW_INFO(pDevice->CreateBuffer(&cbd, &csd, &pConstantBuffer));

	// ========== 11. 重新绑定渲染目标和深度缓冲区 ==========
	pContext->OMSetRenderTargets(1, pTarget.GetAddressOf(), pDSV.Get());

	// ========== 12. 清空深度缓冲区 ==========
	//pContext->ClearDepthStencilView(pDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

	// ========== 13. 设置常量缓冲区到着色器 ==========
	pContext->VSSetConstantBuffers(0, 1, pConstantBuffer.GetAddressOf());   // 顶点着色器使用槽位0
	pContext->PSSetConstantBuffers(1, 1, pConstantBuffer2.GetAddressOf());  // 像素着色器使用槽位1

	// ========== 14. 设置顶点和索引缓冲区 ==========
	const UINT stride = sizeof(Vertex);   // 顶点步长
	const UINT offset = 0u;               // 偏移量
	pContext->IASetVertexBuffers(0, 1, pVertexBuffer.GetAddressOf(), &stride, &offset);
	pContext->IASetIndexBuffer(pIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

	// ========== 15. 加载并设置像素着色器 ==========
	wrl::ComPtr<ID3D11PixelShader> pPixelShader;
	wrl::ComPtr<ID3DBlob> pBlob;
	GFX_THROW_INFO(D3DReadFileToBlob(L"PixelShader.cso", &pBlob));
	GFX_THROW_INFO(pDevice->CreatePixelShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, &pPixelShader));
	pContext->PSSetShader(pPixelShader.Get(), nullptr, 0);

	// ========== 16. 加载并设置顶点着色器 ==========
	wrl::ComPtr<ID3D11VertexShader> pVertexShader;
	GFX_THROW_INFO(D3DReadFileToBlob(L"VertexShader.cso", &pBlob));
	GFX_THROW_INFO(pDevice->CreateVertexShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, &pVertexShader));
	pContext->VSSetShader(pVertexShader.Get(), nullptr, 0);

	// ========== 17. 创建并设置输入布局 ==========
	wrl::ComPtr<ID3D11InputLayout> pInputLayout;
	const D3D11_INPUT_ELEMENT_DESC ied[] =
	{
		{ "Position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	GFX_THROW_INFO(pDevice->CreateInputLayout(ied, (UINT)std::size(ied), pBlob->GetBufferPointer(), pBlob->GetBufferSize(), &pInputLayout));
	pContext->IASetInputLayout(pInputLayout.Get());

	// ========== 18. 设置图元拓扑 ==========
	pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// ========== 19. 设置视口 ==========
	D3D11_VIEWPORT vp;
	vp.Width = (float)width;
	vp.Height = (float)height;
	vp.MinDepth = 0.0f;      // 最小深度
	vp.MaxDepth = 1.0f;      // 最大深度
	vp.TopLeftX = 0.0f;
	vp.TopLeftY = 0.0f;
	pContext->RSSetViewports(1, &vp);

	// ========== 20. 设置栅格化状态（禁用背面剔除） ==========
	D3D11_RASTERIZER_DESC rasterDesc = {};
	rasterDesc.FillMode = D3D11_FILL_SOLID;           // 实心填充
	rasterDesc.CullMode = D3D11_CULL_NONE;            // 禁用背面剔除（显示所有面）
	rasterDesc.FrontCounterClockwise = FALSE;         // 顺时针方向为正面
	rasterDesc.DepthClipEnable = TRUE;                // 启用深度裁剪

	wrl::ComPtr<ID3D11RasterizerState> pRasterState;
	pDevice->CreateRasterizerState(&rasterDesc, &pRasterState);
	pContext->RSSetState(pRasterState.Get());

	// ========== 21. 绘制立方体 ==========
	pContext->DrawIndexed((UINT)std::size(indices), 0, 0);
}