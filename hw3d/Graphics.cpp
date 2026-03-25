#include "stdafx.h"
#include "Graphics.h"
#include <stdexcept>
#include <string>
#include <d3d11.h>
#include <dxgi.h>
#include <cmath>
#include <cassert>
#include <sstream>
#include <system_error>
#include <d3dcompiler.h>
#include <wrl/client.h> 
#include <DirectXMath.h>
#include "imgui.h"
#include "imgui_impl_dx11.h"

// 链接 DirectX 库
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "D3DCompiler.lib")

namespace dx = DirectX;

// ============================================================================
// 构造函数：初始化 Direct3D 设备、交换链、渲染目标和深度缓冲区
// ============================================================================
Graphics::Graphics(HWND hWnd, int width, int height)
	: width(width)
	, height(height)
{
	// 1. 配置交换链描述
	DXGI_SWAP_CHAIN_DESC sd = {};
	sd.BufferDesc.Width = width;
	sd.BufferDesc.Height = height;
	sd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	sd.SampleDesc.Count = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = 2;
	sd.OutputWindow = hWnd;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	// 2. 创建设备、设备上下文和交换链
	HRESULT hr = D3D11CreateDeviceAndSwapChain(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		0,
		nullptr,
		0,
		D3D11_SDK_VERSION,
		&sd,
		&pSwap,
		&pDevice,
		nullptr,
		&pContext
	);
	if (FAILED(hr))
	{
		throw Graphics::HrException(__LINE__, __FILE__, hr);
	}

	// 3. 创建渲染目标视图
	wrl::ComPtr<ID3D11Texture2D> pBackBuffer;
	hr = pSwap->GetBuffer(0, __uuidof(ID3D11Texture2D), &pBackBuffer);
	if (FAILED(hr))
	{
		throw Graphics::HrException(__LINE__, __FILE__, hr);
	}

	hr = pDevice->CreateRenderTargetView(pBackBuffer.Get(), nullptr, &pTarget);
	if (FAILED(hr))
	{
		throw Graphics::HrException(__LINE__, __FILE__, hr);
	}

	// 深度模板状态
	D3D11_DEPTH_STENCIL_DESC dsDesc = {};
	dsDesc.DepthEnable = TRUE;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	wrl::ComPtr<ID3D11DepthStencilState> pDSState;
	hr = pDevice->CreateDepthStencilState(&dsDesc, &pDSState);
	if (FAILED(hr))
	{
		throw Graphics::HrException(__LINE__, __FILE__, hr);
	}
	pContext->OMSetDepthStencilState(pDSState.Get(), 1u);

	// 4. 创建深度缓冲区纹理
	wrl::ComPtr<ID3D11Texture2D> pDepthStencil;
	D3D11_TEXTURE2D_DESC depthDesc = {};
	depthDesc.Width = width;
	depthDesc.Height = height;
	depthDesc.MipLevels = 1;
	depthDesc.ArraySize = 1;
	depthDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthDesc.SampleDesc.Count = 1;
	depthDesc.SampleDesc.Quality = 0;
	depthDesc.Usage = D3D11_USAGE_DEFAULT;
	depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	hr = pDevice->CreateTexture2D(&depthDesc, nullptr, &pDepthStencil);
	if (FAILED(hr))
	{
		throw Graphics::HrException(__LINE__, __FILE__, hr);
	}

	// 5. 创建深度模板视图
	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = depthDesc.Format;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Texture2D.MipSlice = 0;

	hr = pDevice->CreateDepthStencilView(pDepthStencil.Get(), &dsvDesc, &pDSV);
	if (FAILED(hr))
	{
		throw Graphics::HrException(__LINE__, __FILE__, hr);
	}

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
}

// ============================================================================
// 开始帧：清空颜色缓冲区
// ============================================================================
void Graphics::BeginFrame(float red, float green, float blue)
{
	const float color[] = { red, green, blue, 1.0f };
	pContext->ClearRenderTargetView(pTarget.Get(), color);
	pContext->ClearDepthStencilView(pDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0u);
	pContext->OMSetRenderTargets(1, pTarget.GetAddressOf(), pDSV.Get());
}

// ============================================================================
// 结束帧：交换前后缓冲区，显示图像
// ============================================================================
void Graphics::EndFrame()
{
	pSwap->Present(1, 0);
}

// ============================================================================
// 绘制视锥体
// ============================================================================
void Graphics::Frustum(float angle, float x, float y, float z,
	float scaleTop, float scaleBottom, float scaleHeight,
	float topR, float topG, float topB,
	float bottomR, float bottomG, float bottomB,
	float sideR, float sideG, float sideB)
{
	pContext->OMSetRenderTargets(1, pTarget.GetAddressOf(), pDSV.Get());

	struct Vertex
	{
		float x, y, z, r, g, b;
	};

	// 计算上下底面的半宽
	float topHalf = scaleTop;
	float bottomHalf = scaleBottom;
	float halfHeight = scaleHeight;

	// 顶点数据
	Vertex vertices[] =
	{
		// 上底面 (y = halfHeight)
		{-topHalf,  halfHeight, -topHalf, topR, topG, topB},
		{ topHalf,  halfHeight, -topHalf, topR, topG, topB},
		{-topHalf,  halfHeight,  topHalf, topR, topG, topB},
		{ topHalf,  halfHeight,  topHalf, topR, topG, topB},

		// 下底面 (y = -halfHeight)
		{-bottomHalf, -halfHeight, -bottomHalf, bottomR, bottomG, bottomB},
		{ bottomHalf, -halfHeight, -bottomHalf, bottomR, bottomG, bottomB},
		{-bottomHalf, -halfHeight,  bottomHalf, bottomR, bottomG, bottomB},
		{ bottomHalf, -halfHeight,  bottomHalf, bottomR, bottomG, bottomB}
	};

	// 索引数据
	const unsigned short indices[] =
	{
		// 上底面
		0, 1, 2, 2, 1, 3,
		// 下底面
		4, 6, 5, 5, 6, 7,
		// 前面
		0, 1, 4, 4, 1, 5,
		// 后面
		2, 6, 3, 3, 6, 7,
		// 左面
		0, 2, 4, 4, 2, 6,
		// 右面
		1, 5, 3, 3, 5, 7,
	};

	// 创建顶点缓冲区
	wrl::ComPtr<ID3D11Buffer> pVertexBuffer;
	D3D11_BUFFER_DESC bd = {};
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(vertices);
	bd.StructureByteStride = sizeof(Vertex);

	D3D11_SUBRESOURCE_DATA sd = {};
	sd.pSysMem = vertices;

	HRESULT hr = pDevice->CreateBuffer(&bd, &sd, &pVertexBuffer);
	if (FAILED(hr))
	{
		throw Graphics::HrException(__LINE__, __FILE__, hr);
	}

	// 创建索引缓冲区
	wrl::ComPtr<ID3D11Buffer> pIndexBuffer;
	D3D11_BUFFER_DESC ibd = {};
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.Usage = D3D11_USAGE_DEFAULT;
	ibd.ByteWidth = sizeof(indices);
	ibd.StructureByteStride = sizeof(unsigned short);

	D3D11_SUBRESOURCE_DATA isd = {};
	isd.pSysMem = indices;

	hr = pDevice->CreateBuffer(&ibd, &isd, &pIndexBuffer);
	if (FAILED(hr))
	{
		throw Graphics::HrException(__LINE__, __FILE__, hr);
	}

	// ========== 变换矩阵（根据投影类型选择） ==========
	struct ConstantBuffer
	{
		dx::XMMATRIX transform;
	};

	// 根据投影类型选择投影矩阵
	dx::XMMATRIX projectionMatrix;

	if (m_projectionType == ProjectionType::Perspective)
	{
		// 透视投影：近大远小，有景深感
		projectionMatrix = dx::XMMatrixPerspectiveLH(1.0f, 3.0f / 4.0f, 0.5f, 10.0f);
	}
	else
	{
		// 正交投影：物体大小不变，视野大小可调节
		float aspect = (float)width / (float)height;
		float viewWidth = m_orthoViewSize;   // 使用可调节的视野大小
		float viewHeight = viewWidth / aspect;
		projectionMatrix = dx::XMMatrixOrthographicLH(viewWidth, viewHeight, 0.5f, 10.0f);
	}

	ConstantBuffer cb =
	{
		dx::XMMatrixTranspose(
			dx::XMMatrixRotationY(angle) *
			dx::XMMatrixRotationX(angle * 0.5f) *
			dx::XMMatrixTranslation(x, y, z + 5.0f) *
			projectionMatrix
		)
	};

	wrl::ComPtr<ID3D11Buffer> pConstantBuffer;
	D3D11_BUFFER_DESC cbd = {};
	cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbd.Usage = D3D11_USAGE_DEFAULT;
	cbd.ByteWidth = sizeof(ConstantBuffer);

	D3D11_SUBRESOURCE_DATA csd = {};
	csd.pSysMem = &cb;

	hr = pDevice->CreateBuffer(&cbd, &csd, &pConstantBuffer);
	if (FAILED(hr))
	{
		throw Graphics::HrException(__LINE__, __FILE__, hr);
	}

	pContext->VSSetConstantBuffers(0, 1, pConstantBuffer.GetAddressOf());

	// 设置顶点和索引缓冲区
	const UINT stride = sizeof(Vertex);
	const UINT offset = 0u;
	pContext->IASetVertexBuffers(0, 1, pVertexBuffer.GetAddressOf(), &stride, &offset);
	pContext->IASetIndexBuffer(pIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

	// 加载顶点着色器
	wrl::ComPtr<ID3D11VertexShader> pVertexShader;
	wrl::ComPtr<ID3DBlob> pVSBlob;
	hr = D3DReadFileToBlob(L"VertexShader.cso", &pVSBlob);
	if (FAILED(hr))
	{
		throw Graphics::HrException(__LINE__, __FILE__, hr);
	}

	hr = pDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &pVertexShader);
	if (FAILED(hr))
	{
		throw Graphics::HrException(__LINE__, __FILE__, hr);
	}
	pContext->VSSetShader(pVertexShader.Get(), nullptr, 0);

	// 输入布局
	wrl::ComPtr<ID3D11InputLayout> pInputLayout;
	const D3D11_INPUT_ELEMENT_DESC ied[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	hr = pDevice->CreateInputLayout(ied, (UINT)std::size(ied), pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &pInputLayout);
	if (FAILED(hr))
	{
		throw Graphics::HrException(__LINE__, __FILE__, hr);
	}
	pContext->IASetInputLayout(pInputLayout.Get());

	// 加载像素着色器
	wrl::ComPtr<ID3D11PixelShader> pPixelShader;
	wrl::ComPtr<ID3DBlob> pPSBlob;
	hr = D3DReadFileToBlob(L"PixelShader.cso", &pPSBlob);
	if (FAILED(hr))
	{
		throw Graphics::HrException(__LINE__, __FILE__, hr);
	}

	hr = pDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &pPixelShader);
	if (FAILED(hr))
	{
		throw Graphics::HrException(__LINE__, __FILE__, hr);
	}
	pContext->PSSetShader(pPixelShader.Get(), nullptr, 0);

	pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	D3D11_VIEWPORT vp;
	vp.Width = (float)width;
	vp.Height = (float)height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0.0f;
	vp.TopLeftY = 0.0f;
	pContext->RSSetViewports(1, &vp);

	D3D11_RASTERIZER_DESC rasterDesc = {};
	rasterDesc.FillMode = D3D11_FILL_SOLID;
	rasterDesc.CullMode = D3D11_CULL_NONE;
	rasterDesc.FrontCounterClockwise = FALSE;
	rasterDesc.DepthClipEnable = TRUE;

	wrl::ComPtr<ID3D11RasterizerState> pRasterState;
	pDevice->CreateRasterizerState(&rasterDesc, &pRasterState);
	pContext->RSSetState(pRasterState.Get());

	pContext->DrawIndexed((UINT)std::size(indices), 0, 0);
}

// ============================================================================
// Exception 实现
// ============================================================================
// Exception 实现
Graphics::Exception::Exception(int line, const char* file, const std::string& note) noexcept
	: std::runtime_error(note), line(line), file(file)
{
}

const char* Graphics::Exception::what() const noexcept
{
	return std::runtime_error::what();
}

const char* Graphics::Exception::GetType() const noexcept
{
	return "Graphics Exception";
}

int Graphics::Exception::GetLine() const noexcept
{
	return line;
}

const std::string& Graphics::Exception::GetFile() const noexcept
{
	return file;
}

std::string Graphics::Exception::GetOriginString() const noexcept
{
	std::ostringstream oss;
	oss << "[File] " << file << "\n[Line] " << line;
	return oss.str();
}

// HrException 实现
Graphics::HrException::HrException(int line, const char* file, HRESULT hr) noexcept
	: Exception(line, file, "HRESULT Error"), hr(hr)
{
}

Graphics::HrException::HrException(int line, const char* file, HRESULT hr, const std::string& info) noexcept
	: Exception(line, file, "HRESULT Error"), hr(hr), info(info)
{
}

const char* Graphics::HrException::what() const noexcept
{
	std::string fullMsg = GetType();
	fullMsg += ": ";
	fullMsg += GetErrorString();
	if (!info.empty())
	{
		fullMsg += "\nInfo: ";
		fullMsg += info;
	}
	static std::string cached;
	cached = fullMsg;
	return cached.c_str();
}

const char* Graphics::HrException::GetType() const noexcept
{
	return "Graphics HRESULT Exception";
}

HRESULT Graphics::HrException::GetErrorCode() const noexcept
{
	return hr;
}

std::string Graphics::HrException::GetErrorString() const noexcept
{
	return std::system_category().message(hr);
}

std::string Graphics::HrException::GetErrorInfo() const noexcept
{
	return info;
}

// DeviceRemovedException 实现
Graphics::DeviceRemovedException::DeviceRemovedException(int line, const char* file, HRESULT hr, const std::string& info) noexcept
	: HrException(line, file, hr, info)
{
}

const char* Graphics::DeviceRemovedException::GetType() const noexcept
{
	return "Graphics Device Removed Exception";
}

// InfoException 实现
Graphics::InfoException::InfoException(int line, const char* file, const std::string& info) noexcept
	: Exception(line, file, "Info Error"), info(info)
{
}

const char* Graphics::InfoException::what() const noexcept
{
	std::string fullMsg = GetType();
	fullMsg += ": ";
	fullMsg += info;
	static std::string cached;
	cached = fullMsg;
	return cached.c_str();
}

const char* Graphics::InfoException::GetType() const noexcept
{
	return "Graphics Info Exception";
}

const std::string& Graphics::InfoException::GetInfo() const noexcept
{
	return info;
}