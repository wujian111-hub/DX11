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
#include "Vertex.h"
#include "Geometry.h"
#include <vector>
#include <cstdint>

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
		throw GraphicsHrException(__LINE__, __FILE__, hr);
	}

	// 3. 创建渲染目标视图
	wrl::ComPtr<ID3D11Texture2D> pBackBuffer;
	hr = pSwap->GetBuffer(0, __uuidof(ID3D11Texture2D), &pBackBuffer);
	if (FAILED(hr))
	{
		throw GraphicsHrException(__LINE__, __FILE__, hr);
	}

	hr = pDevice->CreateRenderTargetView(pBackBuffer.Get(), nullptr, &pTarget);
	if (FAILED(hr))
	{
		throw GraphicsHrException(__LINE__, __FILE__, hr);
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
		throw GraphicsHrException(__LINE__, __FILE__, hr);
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
		throw GraphicsHrException(__LINE__, __FILE__, hr);
	}

	// 5. 创建深度模板视图
	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = depthDesc.Format;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Texture2D.MipSlice = 0;

	hr = pDevice->CreateDepthStencilView(pDepthStencil.Get(), &dsvDesc, &pDSV);
	if (FAILED(hr))
	{
		throw GraphicsHrException(__LINE__, __FILE__, hr);
	}

	// 6. 将渲染目标和深度缓冲区绑定到输出合并阶段
	pContext->OMSetRenderTargets(1, pTarget.GetAddressOf(), pDSV.Get());

	// ============================================================================
	// Init: sphere mesh + shaders + constant buffer + procedural texture + sampler
	// ============================================================================
	{
		// Sphere mesh (Geometry::CreateSphere)
		auto mesh = Geometry::CreateSphere<VertexPosNormalTex, DWORD>(1.0f, 30u, 40u);
		sphereIndexCount = (UINT)mesh.indexVec.size();

		D3D11_BUFFER_DESC vbd = {};
		vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbd.Usage = D3D11_USAGE_IMMUTABLE;
		vbd.ByteWidth = (UINT)mesh.vertexVec.size() * sizeof(VertexPosNormalTex);
		vbd.StructureByteStride = sizeof(VertexPosNormalTex);
		D3D11_SUBRESOURCE_DATA vsd = {};
		vsd.pSysMem = mesh.vertexVec.data();
		hr = pDevice->CreateBuffer(&vbd, &vsd, &pSphereVB);
		if (FAILED(hr)) { throw GraphicsHrException(__LINE__, __FILE__, hr); }

		D3D11_BUFFER_DESC ibd = {};
		ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		ibd.Usage = D3D11_USAGE_IMMUTABLE;
		ibd.ByteWidth = (UINT)mesh.indexVec.size() * sizeof(DWORD);
		ibd.StructureByteStride = sizeof(DWORD);
		D3D11_SUBRESOURCE_DATA isd = {};
		isd.pSysMem = mesh.indexVec.data();
		hr = pDevice->CreateBuffer(&ibd, &isd, &pSphereIB);
		if (FAILED(hr)) { throw GraphicsHrException(__LINE__, __FILE__, hr); }

		// --- Shader ---
		wrl::ComPtr<ID3DBlob> vsBlob;
		hr = D3DReadFileToBlob(L"Sphere_VS.cso", &vsBlob);
		if (FAILED(hr)) { throw GraphicsHrException(__LINE__, __FILE__, hr); }
		hr = pDevice->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &pSphereVS);
		if (FAILED(hr)) { throw GraphicsHrException(__LINE__, __FILE__, hr); }

		hr = pDevice->CreateInputLayout(
			VertexPosNormalTex::inputLayout,
			(UINT)ARRAYSIZE(VertexPosNormalTex::inputLayout),
			vsBlob->GetBufferPointer(),
			vsBlob->GetBufferSize(),
			&pSphereLayout
		);
		if (FAILED(hr)) { throw GraphicsHrException(__LINE__, __FILE__, hr); }

		wrl::ComPtr<ID3DBlob> psBlob;
		hr = D3DReadFileToBlob(L"Sphere_PS.cso", &psBlob);
		if (FAILED(hr)) { throw GraphicsHrException(__LINE__, __FILE__, hr); }
		hr = pDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pSpherePS);
		if (FAILED(hr)) { throw GraphicsHrException(__LINE__, __FILE__, hr); }

		// --- 常量缓冲 ---
		D3D11_BUFFER_DESC cbd = {};
		cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbd.Usage = D3D11_USAGE_DYNAMIC;
		cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		cbd.ByteWidth = sizeof(SphereCBufData);
		hr = pDevice->CreateBuffer(&cbd, nullptr, &pSphereCBuf);
		if (FAILED(hr)) { throw GraphicsHrException(__LINE__, __FILE__, hr); }

		// Procedural texture (no external file dependency)
		const UINT texW = 512u;
		const UINT texH = 512u;
		std::vector<std::uint32_t> pixels;
		pixels.resize((size_t)texW * texH);

		for (UINT y = 0u; y < texH; y++)
		{
			for (UINT x = 0u; x < texW; x++)
			{
				const float u = (x + 0.5f) / texW;
				const float v = (y + 0.5f) / texH;
				const float cx = u - 0.5f;
				const float cy = v - 0.5f;
				const float r = sqrtf(cx * cx + cy * cy);
				const float a = atan2f(cy, cx);

				const float stripe = 0.5f + 0.5f * cosf(18.0f * a + 24.0f * r);
				const float ring = 0.5f + 0.5f * cosf(32.0f * r);
				const float t = 0.55f * stripe + 0.45f * ring;

				const float baseR = 0.10f + 0.90f * t;
				const float baseG = 0.20f + 0.70f * (1.0f - fabsf(2.0f * t - 1.0f));
				const float baseB = 0.35f + 0.65f * (1.0f - t);

				const UINT8 R = (UINT8)(baseR * 255.0f);
				const UINT8 G = (UINT8)(baseG * 255.0f);
				const UINT8 B = (UINT8)(baseB * 255.0f);
				const UINT8 A = 255u;

				// DXGI_FORMAT_R8G8B8A8_UNORM
				pixels[(size_t)y * texW + x] = (std::uint32_t)(R | (G << 8) | (B << 16) | (A << 24));
			}
		}

		D3D11_TEXTURE2D_DESC td = {};
		td.Width = texW;
		td.Height = texH;
		td.MipLevels = 1u;
		td.ArraySize = 1u;
		td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		td.SampleDesc.Count = 1u;
		td.Usage = D3D11_USAGE_IMMUTABLE;
		td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

		D3D11_SUBRESOURCE_DATA tsd = {};
		tsd.pSysMem = pixels.data();
		tsd.SysMemPitch = texW * sizeof(std::uint32_t);

		wrl::ComPtr<ID3D11Texture2D> tex;
		hr = pDevice->CreateTexture2D(&td, &tsd, &tex);
		if (FAILED(hr)) { throw GraphicsHrException(__LINE__, __FILE__, hr); }

		D3D11_SHADER_RESOURCE_VIEW_DESC srvd = {};
		srvd.Format = td.Format;
		srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvd.Texture2D.MostDetailedMip = 0u;
		srvd.Texture2D.MipLevels = 1u;
		hr = pDevice->CreateShaderResourceView(tex.Get(), &srvd, &pSphereTex);
		if (FAILED(hr)) { throw GraphicsHrException(__LINE__, __FILE__, hr); }

		// --- 采样器 ---
		D3D11_SAMPLER_DESC samp = {};
		samp.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samp.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		samp.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		samp.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		samp.MaxLOD = D3D11_FLOAT32_MAX;
		hr = pDevice->CreateSamplerState(&samp, &pSphereSampler);
		if (FAILED(hr)) { throw GraphicsHrException(__LINE__, __FILE__, hr); }
	}
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
// 绘制：带旋转纹理的球体
// ============================================================================
void Graphics::DrawTexturedSphere(float dt)
{
	using namespace DirectX;

	// 更新角度
	sphereWorldAngle += dt * 0.8f;
	sphereTexAngle += dt * XM_PI * 0.8f;
	if (sphereWorldAngle > XM_2PI) sphereWorldAngle -= XM_2PI;
	if (sphereTexAngle > XM_2PI) sphereTexAngle -= XM_2PI;

	// 计算矩阵
	const XMMATRIX world =
		XMMatrixRotationY(sphereWorldAngle) *
		XMMatrixRotationX(0.35f * sphereWorldAngle);

	const XMMATRIX view = XMMatrixLookAtLH(
		XMVectorSet(0.0f, 0.0f, -4.0f, 0.0f),
		XMVectorZero(),
		XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)
	);

	const float aspect = (float)width / (float)height;
	const XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV2, aspect, 0.1f, 50.0f);

	const float c = cosf(sphereTexAngle);
	const float s = sinf(sphereTexAngle);
	const XMMATRIX texRot = XMMATRIX(
		c,  s, 0, 0,
		-s, c, 0, 0,
		0,  0, 1, 0,
		0,  0, 0, 1
	);

	SphereCBufData cb = {};
	cb.world = XMMatrixTranspose(world);
	cb.view = XMMatrixTranspose(view);
	cb.proj = XMMatrixTranspose(proj);
	cb.texRot = XMMatrixTranspose(texRot);

	// 更新常量缓冲
	D3D11_MAPPED_SUBRESOURCE msr;
	HRESULT hr = pContext->Map(pSphereCBuf.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0u, &msr);
	if (FAILED(hr)) { throw GraphicsHrException(__LINE__, __FILE__, hr); }
	memcpy(msr.pData, &cb, sizeof(cb));
	pContext->Unmap(pSphereCBuf.Get(), 0u);

	// 绑定渲染目标（防止 ImGui 改写 OM 状态后遗漏）
	pContext->OMSetRenderTargets(1, pTarget.GetAddressOf(), pDSV.Get());

	// IA
	const UINT stride = sizeof(VertexPosNormalTex);
	const UINT offset = 0u;
	pContext->IASetVertexBuffers(0u, 1u, pSphereVB.GetAddressOf(), &stride, &offset);
	pContext->IASetIndexBuffer(pSphereIB.Get(), DXGI_FORMAT_R32_UINT, 0u);
	pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pContext->IASetInputLayout(pSphereLayout.Get());

	// VS/PS
	pContext->VSSetShader(pSphereVS.Get(), nullptr, 0u);
	pContext->VSSetConstantBuffers(0u, 1u, pSphereCBuf.GetAddressOf());

	pContext->PSSetShader(pSpherePS.Get(), nullptr, 0u);
	pContext->PSSetShaderResources(0u, 1u, pSphereTex.GetAddressOf());
	pContext->PSSetSamplers(0u, 1u, pSphereSampler.GetAddressOf());

	// 视口
	D3D11_VIEWPORT vp;
	vp.Width = (float)width;
	vp.Height = (float)height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0.0f;
	vp.TopLeftY = 0.0f;
	pContext->RSSetViewports(1u, &vp);

	// Rasterizer: 保持和你原先 Frustum 一样不剔除，避免因为 winding 导致看不到
	D3D11_RASTERIZER_DESC rasterDesc = {};
	rasterDesc.FillMode = D3D11_FILL_SOLID;
	rasterDesc.CullMode = D3D11_CULL_NONE;
	rasterDesc.FrontCounterClockwise = FALSE;
	rasterDesc.DepthClipEnable = TRUE;

	wrl::ComPtr<ID3D11RasterizerState> rs;
	hr = pDevice->CreateRasterizerState(&rasterDesc, &rs);
	if (FAILED(hr)) { throw GraphicsHrException(__LINE__, __FILE__, hr); }
	pContext->RSSetState(rs.Get());

	// Draw
	pContext->DrawIndexed(sphereIndexCount, 0, 0);
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
		throw GraphicsHrException(__LINE__, __FILE__, hr);
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
		throw GraphicsHrException(__LINE__, __FILE__, hr);
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
		throw GraphicsHrException(__LINE__, __FILE__, hr);
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
		throw GraphicsHrException(__LINE__, __FILE__, hr);
	}

	hr = pDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &pVertexShader);
	if (FAILED(hr))
	{
		throw GraphicsHrException(__LINE__, __FILE__, hr);
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
		throw GraphicsHrException(__LINE__, __FILE__, hr);
	}
	pContext->IASetInputLayout(pInputLayout.Get());

	// 加载像素着色器
	wrl::ComPtr<ID3D11PixelShader> pPixelShader;
	wrl::ComPtr<ID3DBlob> pPSBlob;
	hr = D3DReadFileToBlob(L"PixelShader.cso", &pPSBlob);
	if (FAILED(hr))
	{
		throw GraphicsHrException(__LINE__, __FILE__, hr);
	}

	hr = pDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &pPixelShader);
	if (FAILED(hr))
	{
		throw GraphicsHrException(__LINE__, __FILE__, hr);
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
GraphicsException::GraphicsException(int line, const char* file, const std::string& note) noexcept
	: std::runtime_error(note), line(line), file(file)
{
}

const char* GraphicsException::what() const noexcept
{
	return std::runtime_error::what();
}

const char* GraphicsException::GetType() const noexcept
{
	return "Graphics Exception";
}

int GraphicsException::GetLine() const noexcept
{
	return line;
}

const std::string& GraphicsException::GetFile() const noexcept
{
	return file;
}

std::string GraphicsException::GetOriginString() const noexcept
{
	std::ostringstream oss;
	oss << "[File] " << file << "\n[Line] " << line;
	return oss.str();
}

// GraphicsHrException 实现
GraphicsHrException::GraphicsHrException(int line, const char* file, HRESULT hr) noexcept
	: GraphicsException(line, file, "HRESULT Error"), hr(hr)
{
}

GraphicsHrException::GraphicsHrException(int line, const char* file, HRESULT hr, const std::string& info) noexcept
	: GraphicsException(line, file, "HRESULT Error"), hr(hr), info(info)
{
}

const char* GraphicsHrException::what() const noexcept
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

const char* GraphicsHrException::GetType() const noexcept
{
	return "Graphics HRESULT Exception";
}

HRESULT GraphicsHrException::GetErrorCode() const noexcept
{
	return hr;
}

std::string GraphicsHrException::GetErrorString() const noexcept
{
	return std::system_category().message(hr);
}

std::string GraphicsHrException::GetErrorInfo() const noexcept
{
	return info;
}

// GraphicsDeviceRemovedException 实现
GraphicsDeviceRemovedException::GraphicsDeviceRemovedException(int line, const char* file, HRESULT hr, const std::string& info) noexcept
	: GraphicsHrException(line, file, hr, info)
{
}

const char* GraphicsDeviceRemovedException::GetType() const noexcept
{
	return "Graphics Device Removed Exception";
}

// GraphicsInfoException 实现
GraphicsInfoException::GraphicsInfoException(int line, const char* file, const std::string& info) noexcept
	: GraphicsException(line, file, "Info Error"), info(info)
{
}

const char* GraphicsInfoException::what() const noexcept
{
	std::string fullMsg = GetType();
	fullMsg += ": ";
	fullMsg += info;
	static std::string cached;
	cached = fullMsg;
	return cached.c_str();
}

const char* GraphicsInfoException::GetType() const noexcept
{
	return "Graphics Info Exception";
}

const std::string& GraphicsInfoException::GetInfo() const noexcept
{
	return info;
}