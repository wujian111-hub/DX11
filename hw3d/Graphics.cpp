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
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "D3DCompiler.lib")

namespace dx = DirectX;

#ifndef GFX_THROW_INFO
#define GFX_THROW_INFO(hr) \
	if (FAILED(hr)) \
	{ \
		throw std::runtime_error("Graphics error"); \
	}
#endif

Graphics::Graphics(HWND hWnd, int width, int height)
	:
     width(width),
     height(height)
{
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

	GFX_THROW_INFO(D3D11CreateDeviceAndSwapChain(
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
	));

	wrl::ComPtr<ID3D11Texture2D> pBackBuffer;
	GFX_THROW_INFO(pSwap->GetBuffer(0, __uuidof(ID3D11Texture2D), &pBackBuffer));
	GFX_THROW_INFO(pDevice->CreateRenderTargetView(pBackBuffer.Get(), nullptr, &pTarget));
}

void Graphics::ClearBuffer(float red, float green, float blue) noexcept
{
	const float color[] = { red, green, blue, 1.0f };
	pContext->ClearRenderTargetView(pTarget.Get(), color);
}

void Graphics::BeginFrame(float red, float green, float blue)
{
	assert(pTarget != nullptr);
	assert(pContext != nullptr);

	const float color[] = { red, green, blue, 1.0f };
	pContext->ClearRenderTargetView(pTarget.Get(), color);

	char buf[100];
	sprintf_s(buf, "Clear with color: %.1f, %.1f, %.1f\n", red, green, blue);
	OutputDebugStringA(buf);
}

void Graphics::EndFrame()
{
	pSwap->Present(1, 0);
}

void Graphics::Frustum(float angle,float x,float y)
{
	struct Vertex
	{
		struct
		{
			float x;
			float y;
			float z;
		} pos;
		struct
		{
			unsigned char r;
			unsigned char g;
			unsigned char b;
			unsigned char a;
		} color;
	};
	const Vertex vertices[] =
	{
		// 下底面 (z = -1.0, 较大)
   { -1.0f, -1.0f, -1.0f, 255, 0,   0,   255 },  // 0 左下前
   {  1.0f, -1.0f, -1.0f, 0,   255, 0,   255 },  // 1 右下前
   { -1.0f,  1.0f, -1.0f, 0,   0,   255, 255 },  // 2 左上前
   {  1.0f,  1.0f, -1.0f, 255, 255, 0,   255 },  // 3 右上前

   // 上底面 (z = 1.0, 较小)
   { -0.5f, -0.5f,  1.0f, 255, 0,   255, 255 },  // 4 左下后
   {  0.5f, -0.5f,  1.0f, 0,   255, 255, 255 },  // 5 右下后
   { -0.5f,  0.5f,  1.0f, 0,   0,   0,   255 },  // 6 左上后
   {  0.5f,  0.5f,  1.0f, 255, 255, 255, 255 },  // 7 右上后
	};
	wrl::ComPtr<ID3D11Buffer> pVertexBuffer;
	D3D11_BUFFER_DESC bd = {};

	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.CPUAccessFlags = 0u;
	bd.MiscFlags = 0u;
	bd.ByteWidth = sizeof(vertices);
	bd.StructureByteStride = sizeof(Vertex);
	D3D11_SUBRESOURCE_DATA sd = {};
	sd.pSysMem = vertices;
	GFX_THROW_INFO(pDevice->CreateBuffer(&bd, &sd, &pVertexBuffer));

	const unsigned short indices[] =
	{
		// 下底面 (前面) - 从外部看逆时针
		0, 2, 1,  1, 2, 3,

		// 上底面 (后面) - 从外部看逆时针（注意顺序相反）
		4, 5, 6,  5, 7, 6,

		// 侧面
		// 左侧面
		0, 4, 2,  2, 4, 6,
		// 右侧面
		1, 3, 5,  3, 7, 5,
		// 底面
		0, 1, 4,  1, 5, 4,
		// 顶面
		2, 6, 3,  3, 6, 7,
	};

	wrl::ComPtr<ID3D11Buffer> pIndexBuffer;
	D3D11_BUFFER_DESC ibd = {};
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.Usage = D3D11_USAGE_DEFAULT;
	ibd.CPUAccessFlags = 0u;
	ibd.MiscFlags = 0u;
	ibd.ByteWidth = sizeof(indices);
	ibd.StructureByteStride = sizeof(unsigned short);
	D3D11_SUBRESOURCE_DATA isd = {};
	isd.pSysMem = indices;
	GFX_THROW_INFO(pDevice->CreateBuffer(&ibd, &isd, &pIndexBuffer));

	struct ConstantBuffer
	{
		dx::XMMATRIX transform;
	};

	ConstantBuffer cb = 
	{
		{
			dx::XMMatrixTranspose(
				dx::XMMatrixRotationZ(angle)*
				dx::XMMatrixRotationX(angle)*
				dx::XMMatrixScaling(3.0f/4.0f,1.0f,1.0f)*
				dx::XMMatrixTranslation(x,y,4.0f)*
				dx::XMMatrixPerspectiveLH(1.0f,3.0f/4.0f,0.5f,10.0f)
				)
		}
	};


	wrl::ComPtr<ID3D11Buffer> pConstantBuffer;
	D3D11_BUFFER_DESC cbd = {};
	cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbd.Usage = D3D11_USAGE_DEFAULT;
	cbd.CPUAccessFlags =0;
	cbd.MiscFlags = 0u;
	cbd.ByteWidth = sizeof(ConstantBuffer);
	cbd.StructureByteStride = 0;
	D3D11_SUBRESOURCE_DATA csd = {};
	csd.pSysMem = &cb;
	GFX_THROW_INFO(pDevice->CreateBuffer(&cbd, &csd, &pConstantBuffer));

	pContext->VSSetConstantBuffers(0, 1, pConstantBuffer.GetAddressOf());
	const UINT stride = sizeof(Vertex);
	const UINT offset = 0u;
	pContext->IASetVertexBuffers(0, 1, pVertexBuffer.GetAddressOf(), &stride, &offset);
	pContext->IASetIndexBuffer(pIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);  // 修正：改用 IASetIndexBuffer

	wrl::ComPtr<ID3D11PixelShader> pPixelShader;
	wrl::ComPtr<ID3DBlob> pBlob;
	GFX_THROW_INFO(D3DReadFileToBlob(L"PixelShader.cso", &pBlob));
	GFX_THROW_INFO(pDevice->CreatePixelShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, &pPixelShader));
	pContext->PSSetShader(pPixelShader.Get(), nullptr, 0);

	wrl::ComPtr<ID3D11VertexShader> pVertexShader;
	GFX_THROW_INFO(D3DReadFileToBlob(L"VertexShader.cso", &pBlob));
	GFX_THROW_INFO(pDevice->CreateVertexShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, &pVertexShader));
	pContext->VSSetShader(pVertexShader.Get(), nullptr, 0);

	wrl::ComPtr<ID3D11InputLayout> pInputLayout;
	const D3D11_INPUT_ELEMENT_DESC ied[] =
	{
		{ "Position", 0, DXGI_FORMAT_R32G32B32_FLOAT,0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "Color", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 12u, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	GFX_THROW_INFO(pDevice->CreateInputLayout(ied, (UINT)std::size(ied), pBlob->GetBufferPointer(), pBlob->GetBufferSize(), &pInputLayout));
	pContext->IASetInputLayout(pInputLayout.Get());

	pContext->OMSetRenderTargets(1, pTarget.GetAddressOf(), nullptr);
	pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	D3D11_VIEWPORT vp;
	vp.Width = (float)width;
	vp.Height = (float)height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0.0f;
	vp.TopLeftY = 0.0f;
	pContext->RSSetViewports(1, &vp);

	pContext->DrawIndexed((UINT)std::size(indices), 0, 0);
}