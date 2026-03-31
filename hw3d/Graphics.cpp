#include "stdafx.h"
#include "Graphics.h"
#include <stdexcept>
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#include <wrl/client.h> 
#include <DirectXMath.h>
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "Vertex.h"
#include "Geometry.h"
#include <vector>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "D3DCompiler.lib")

namespace dx = DirectX;

// ============================================================================
// 构造函数
// ============================================================================
Graphics::Graphics(HWND hWnd, int width, int height)
    : width(width), height(height)
{
    // 初始化光照默认值
    cbData.ambientLight = dx::XMFLOAT3(0.2f, 0.2f, 0.2f);
    cbData.lightDir = dx::XMFLOAT3(1.0f, -1.0f, 0.5f);
    cbData.lightColor = dx::XMFLOAT3(1.0f, 1.0f, 1.0f);
    cbData.materialAmbient = dx::XMFLOAT3(0.2f, 0.1f, 0.1f);
    cbData.materialDiffuse = dx::XMFLOAT3(0.8f, 0.3f, 0.3f);
    cbData.materialSpecular = dx::XMFLOAT3(1.0f, 1.0f, 1.0f);
    cbData.materialShininess = 32.0f;

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

    // =========================================================================
    // 创建球体网格
    // =========================================================================
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
    if (FAILED(hr)) throw GraphicsHrException(__LINE__, __FILE__, hr);

    D3D11_BUFFER_DESC ibd = {};
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.Usage = D3D11_USAGE_IMMUTABLE;
    ibd.ByteWidth = (UINT)mesh.indexVec.size() * sizeof(DWORD);
    ibd.StructureByteStride = sizeof(DWORD);
    D3D11_SUBRESOURCE_DATA isd = {};
    isd.pSysMem = mesh.indexVec.data();
    hr = pDevice->CreateBuffer(&ibd, &isd, &pSphereIB);
    if (FAILED(hr)) throw GraphicsHrException(__LINE__, __FILE__, hr);

    // =========================================================================
    // 创建纯色球体着色器
    // =========================================================================
    // 顶点着色器
    wrl::ComPtr<ID3DBlob> solidVSBlob;
    hr = D3DReadFileToBlob(L"SolidSphere_VS.cso", &solidVSBlob);
    if (FAILED(hr)) throw GraphicsHrException(__LINE__, __FILE__, hr);
    hr = pDevice->CreateVertexShader(solidVSBlob->GetBufferPointer(),
        solidVSBlob->GetBufferSize(), nullptr, &pSolidVS);
    if (FAILED(hr)) throw GraphicsHrException(__LINE__, __FILE__, hr);

    // 输入布局（位置 + 法线）
    const D3D11_INPUT_ELEMENT_DESC solidLayout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };
    hr = pDevice->CreateInputLayout(solidLayout, (UINT)std::size(solidLayout),
        solidVSBlob->GetBufferPointer(), solidVSBlob->GetBufferSize(), &pSolidLayout);
    if (FAILED(hr)) throw GraphicsHrException(__LINE__, __FILE__, hr);

    // 像素着色器
    wrl::ComPtr<ID3DBlob> solidPSBlob;
    hr = D3DReadFileToBlob(L"SolidSphere_PS.cso", &solidPSBlob);
    if (FAILED(hr)) throw GraphicsHrException(__LINE__, __FILE__, hr);
    hr = pDevice->CreatePixelShader(solidPSBlob->GetBufferPointer(),
        solidPSBlob->GetBufferSize(), nullptr, &pSolidPS);
    if (FAILED(hr)) throw GraphicsHrException(__LINE__, __FILE__, hr);

    // 常量缓冲区
    D3D11_BUFFER_DESC cbd = {};
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbd.Usage = D3D11_USAGE_DYNAMIC;
    cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    cbd.ByteWidth = sizeof(SolidCBufData);
    hr = pDevice->CreateBuffer(&cbd, nullptr, &pSolidCBuf);
    if (FAILED(hr)) throw GraphicsHrException(__LINE__, __FILE__, hr);

    // =========================================================================
    // ImGui 初始化
    // =========================================================================
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui_ImplWin32_Init(hWnd);
    ImGui_ImplDX11_Init(pDevice.Get(), pContext.Get());
    ImGui::StyleColorsDark();
}

// ============================================================================
// 析构函数
// ============================================================================
Graphics::~Graphics()
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
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
// BeginImGuiRender
// ============================================================================
void Graphics::BeginImGuiRender()
{
    pContext->OMSetDepthStencilState(nullptr, 0);
}

// ============================================================================
// 光源/材质控制函数
// ============================================================================
void Graphics::SetLightDir(float x, float y, float z)
{
    cbData.lightDir = dx::XMFLOAT3(x, y, z);
}

void Graphics::SetMaterialDiffuse(float r, float g, float b)
{
    cbData.materialDiffuse = dx::XMFLOAT3(r, g, b);
}

void Graphics::SetMaterialSpecular(float r, float g, float b)
{
    cbData.materialSpecular = dx::XMFLOAT3(r, g, b);
}

void Graphics::SetMaterialShininess(float shininess)
{
    cbData.materialShininess = shininess;
}

// ============================================================================
// DrawSolidSphere
// ============================================================================
void Graphics::DrawSolidSphere(float dt, float rotationAngle, float posX, float posY, float posZ)
{
    using namespace DirectX;

    // 更新自动旋转角度
    sphereWorldAngle += dt * 0.8f;
    if (sphereWorldAngle > XM_2PI) sphereWorldAngle -= XM_2PI;

    float finalRotation = sphereWorldAngle + rotationAngle;

    // 矩阵计算
    const XMMATRIX world = XMMatrixTranslation(posX, posY, posZ) *
        XMMatrixRotationY(finalRotation) *
        XMMatrixRotationX(0.35f * finalRotation);

    const XMMATRIX view = XMMatrixLookAtLH(
        XMVectorSet(0.0f, 0.0f, -4.0f, 0.0f),
        XMVectorZero(),
        XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)
    );

    const float aspect = (float)width / (float)height;
    const XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV2, aspect, 0.1f, 50.0f);

    // 填充常量缓冲区
    SolidCBufData cb = cbData;  // 复制当前光照参数
    cb.world = XMMatrixTranspose(world);
    cb.view = XMMatrixTranspose(view);
    cb.proj = XMMatrixTranspose(proj);
    cb.cameraPos = XMFLOAT3(0.0f, 0.0f, -4.0f);

    // 更新 GPU 常量缓冲区
    D3D11_MAPPED_SUBRESOURCE msr;
    HRESULT hr = pContext->Map(pSolidCBuf.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0u, &msr);
    if (FAILED(hr)) throw GraphicsHrException(__LINE__, __FILE__, hr);
    memcpy(msr.pData, &cb, sizeof(cb));
    pContext->Unmap(pSolidCBuf.Get(), 0u);

    // 绑定渲染目标
    pContext->OMSetRenderTargets(1, pTarget.GetAddressOf(), pDSV.Get());

    // IA 阶段
    const UINT stride = sizeof(VertexPosNormalTex);
    const UINT offset = 0u;
    pContext->IASetVertexBuffers(0u, 1u, pSphereVB.GetAddressOf(), &stride, &offset);
    pContext->IASetIndexBuffer(pSphereIB.Get(), DXGI_FORMAT_R32_UINT, 0u);
    pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    pContext->IASetInputLayout(pSolidLayout.Get());

    // VS/PS
    pContext->VSSetShader(pSolidVS.Get(), nullptr, 0u);
    pContext->VSSetConstantBuffers(0u, 1u, pSolidCBuf.GetAddressOf());
    pContext->PSSetShader(pSolidPS.Get(), nullptr, 0u);
    pContext->PSSetConstantBuffers(0u, 1u, pSolidCBuf.GetAddressOf());

    // 视口
    D3D11_VIEWPORT vp;
    vp.Width = (float)width;
    vp.Height = (float)height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0f;
    pContext->RSSetViewports(1u, &vp);

    // 光栅化器（背面剔除）
    D3D11_RASTERIZER_DESC rasterDesc = {};
    rasterDesc.FillMode = D3D11_FILL_SOLID;
    rasterDesc.CullMode = D3D11_CULL_BACK;
    rasterDesc.FrontCounterClockwise = FALSE;
    rasterDesc.DepthClipEnable = TRUE;

    wrl::ComPtr<ID3D11RasterizerState> rs;
    hr = pDevice->CreateRasterizerState(&rasterDesc, &rs);
    if (FAILED(hr)) throw GraphicsHrException(__LINE__, __FILE__, hr);
    pContext->RSSetState(rs.Get());

    // 绘制
    pContext->DrawIndexed(sphereIndexCount, 0, 0);
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
// 异常类实现（保持原有不变）
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