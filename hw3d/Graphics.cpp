#include "stdafx.h"
#include "Graphics.h"
#include <stdexcept>
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#include <wrl/client.h> 
#include <DirectXMath.h>
#include <DirectXTex.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "D3DCompiler.lib")
#pragma comment(lib, "DirectXTex.lib")

namespace dx = DirectX;

// ============================================================================
// 构造函数
// ============================================================================
Graphics::Graphics(HWND hWnd, int width, int height)
    : width(width), height(height)
{
    // 1. 配置交换链
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

    // 2. 创建设备
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        nullptr, 0, D3D11_SDK_VERSION, &sd,
        &pSwap, &pDevice, nullptr, &pContext
    );
    if (FAILED(hr)) throw GraphicsHrException(__LINE__, __FILE__, hr);

    // 3. 创建渲染目标视图
    wrl::ComPtr<ID3D11Texture2D> pBackBuffer;
    hr = pSwap->GetBuffer(0, __uuidof(ID3D11Texture2D), &pBackBuffer);
    if (FAILED(hr)) throw GraphicsHrException(__LINE__, __FILE__, hr);
    hr = pDevice->CreateRenderTargetView(pBackBuffer.Get(), nullptr, &pTarget);
    if (FAILED(hr)) throw GraphicsHrException(__LINE__, __FILE__, hr);

    // 4. 深度模板状态
    D3D11_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthEnable = TRUE;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
    wrl::ComPtr<ID3D11DepthStencilState> pDSState;
    hr = pDevice->CreateDepthStencilState(&dsDesc, &pDSState);
    if (FAILED(hr)) throw GraphicsHrException(__LINE__, __FILE__, hr);
    pContext->OMSetDepthStencilState(pDSState.Get(), 1u);

    // 5. 创建深度缓冲区
    wrl::ComPtr<ID3D11Texture2D> pDepthStencil;
    D3D11_TEXTURE2D_DESC depthDesc = {};
    depthDesc.Width = width;
    depthDesc.Height = height;
    depthDesc.MipLevels = 1;
    depthDesc.ArraySize = 1;
    depthDesc.Format = DXGI_FORMAT_D32_FLOAT;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.Usage = D3D11_USAGE_DEFAULT;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    hr = pDevice->CreateTexture2D(&depthDesc, nullptr, &pDepthStencil);
    if (FAILED(hr)) throw GraphicsHrException(__LINE__, __FILE__, hr);

    // 6. 创建深度模板视图
    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = depthDesc.Format;
    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    hr = pDevice->CreateDepthStencilView(pDepthStencil.Get(), &dsvDesc, &pDSV);
    if (FAILED(hr)) throw GraphicsHrException(__LINE__, __FILE__, hr);

    pContext->OMSetRenderTargets(1, pTarget.GetAddressOf(), pDSV.Get());
}

// ============================================================================
// 析构函数
// ============================================================================
Graphics::~Graphics()
{
}

// ============================================================================
// BeginFrame
// ============================================================================
void Graphics::BeginFrame(float red, float green, float blue)
{
    const float color[] = { red, green, blue, 1.0f };
    pContext->ClearRenderTargetView(pTarget.Get(), color);
    pContext->ClearDepthStencilView(pDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0u);
    pContext->OMSetRenderTargets(1, pTarget.GetAddressOf(), pDSV.Get());
}

// ============================================================================
// EndFrame
// ============================================================================
void Graphics::EndFrame()
{
    pSwap->Present(1, 0);
}

// ============================================================================
// ClearBuffer
// ============================================================================
void Graphics::ClearBuffer(float red, float green, float blue) noexcept
{
    const float color[] = { red, green, blue, 1.0f };
    pContext->ClearRenderTargetView(pTarget.Get(), color);
}

// ============================================================================
// 异常类实现
// ============================================================================
GraphicsException::GraphicsException(int line, const char* file, const std::string& note) noexcept
    : std::runtime_error(note), line(line), file(file) {}

const char* GraphicsException::what() const noexcept { return std::runtime_error::what(); }
const char* GraphicsException::GetType() const noexcept { return "Graphics Exception"; }
int GraphicsException::GetLine() const noexcept { return line; }
const std::string& GraphicsException::GetFile() const noexcept { return file; }
std::string GraphicsException::GetOriginString() const noexcept {
    std::ostringstream oss; oss << "[File] " << file << "\n[Line] " << line; return oss.str();
}

GraphicsHrException::GraphicsHrException(int line, const char* file, HRESULT hr) noexcept
    : GraphicsException(line, file, "HRESULT Error"), hr(hr) {
}
GraphicsHrException::GraphicsHrException(int line, const char* file, HRESULT hr, const std::string& info) noexcept
    : GraphicsException(line, file, "HRESULT Error"), hr(hr), info(info) {
}
const char* GraphicsHrException::what() const noexcept {
    std::string fullMsg = GetType(); fullMsg += ": "; fullMsg += GetErrorString();
    if (!info.empty()) { fullMsg += "\nInfo: "; fullMsg += info; }
    static std::string cached; cached = fullMsg; return cached.c_str();
}
const char* GraphicsHrException::GetType() const noexcept { return "Graphics HRESULT Exception"; }
HRESULT GraphicsHrException::GetErrorCode() const noexcept { return hr; }
std::string GraphicsHrException::GetErrorString() const noexcept { return std::system_category().message(hr); }
std::string GraphicsHrException::GetErrorInfo() const noexcept { return info; }

GraphicsDeviceRemovedException::GraphicsDeviceRemovedException(int line, const char* file, HRESULT hr, const std::string& info) noexcept
    : GraphicsHrException(line, file, hr, info) {
}
const char* GraphicsDeviceRemovedException::GetType() const noexcept { return "Graphics Device Removed Exception"; }

GraphicsInfoException::GraphicsInfoException(int line, const char* file, const std::string& info) noexcept
    : GraphicsException(line, file, "Info Error"), info(info) {
}
const char* GraphicsInfoException::what() const noexcept {
    std::string fullMsg = GetType(); fullMsg += ": "; fullMsg += info;
    static std::string cached; cached = fullMsg; return cached.c_str();
}
const char* GraphicsInfoException::GetType() const noexcept { return "Graphics Info Exception"; }
const std::string& GraphicsInfoException::GetInfo() const noexcept { return info; }

// ============================================================================
// 天空盒相关方法
// ============================================================================
void Graphics::InitializeSkybox()
{
    skybox.Initialize(pDevice.Get(), pContext.Get(), L"Skybox_VS.cso", L"Skybox_PS.cso");
}

void Graphics::LoadSkyboxTexture(const std::wstring& texturePath)
{
    skybox.LoadTexture(pDevice.Get(), texturePath);
}

void Graphics::DrawSkybox()
{
    using namespace DirectX;

    const XMMATRIX view = XMMatrixLookAtLH(
        XMVectorSet(0.0f, 0.0f, -4.0f, 0.0f),
        XMVectorZero(),
        XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)
    );

    const float aspect = (float)width / (float)height;
    const XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV2, aspect, 0.1f, 50.0f);

    skybox.SetViewMatrix(view);
    skybox.SetProjMatrix(proj);

    // 保存当前状态
    ID3D11DepthStencilState* pOldDSState = nullptr;
    UINT oldStencilRef = 0;
    pContext->OMGetDepthStencilState(&pOldDSState, &oldStencilRef);

    ID3D11RasterizerState* pOldRSState = nullptr;
    pContext->RSGetState(&pOldRSState);

    // 设置天空盒专用状态
    D3D11_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthEnable = TRUE;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
    wrl::ComPtr<ID3D11DepthStencilState> pSkyboxDSState;
    HRESULT hr = pDevice->CreateDepthStencilState(&dsDesc, &pSkyboxDSState);
    if (FAILED(hr)) throw GraphicsHrException(__LINE__, __FILE__, hr);
    pContext->OMSetDepthStencilState(pSkyboxDSState.Get(), 0);

    D3D11_RASTERIZER_DESC rasterDesc = {};
    rasterDesc.FillMode = D3D11_FILL_SOLID;
    rasterDesc.CullMode = D3D11_CULL_NONE;
    rasterDesc.FrontCounterClockwise = TRUE;
    rasterDesc.DepthClipEnable = TRUE;
    wrl::ComPtr<ID3D11RasterizerState> pSkyboxRS;
    hr = pDevice->CreateRasterizerState(&rasterDesc, &pSkyboxRS);
    if (FAILED(hr)) throw GraphicsHrException(__LINE__, __FILE__, hr);
    pContext->RSSetState(pSkyboxRS.Get());

    skybox.Draw(pContext.Get());

    // 恢复原来的状态
    pContext->OMSetDepthStencilState(pOldDSState, oldStencilRef);
    pContext->RSSetState(pOldRSState);
}