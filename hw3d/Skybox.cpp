#include "stdafx.h"
#include "Skybox.h"
#include "Graphics.h"
#include <d3dcompiler.h>
#include <DDSTextureLoader.h>
#pragma comment(lib, "d3dcompiler.lib")

Skybox::Skybox()
{
}

Skybox::~Skybox()
{
}

void Skybox::Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext,
    const std::wstring& vsPath, const std::wstring& psPath)
{
    HRESULT hr;

    wrl::ComPtr<ID3DBlob> pVSBlob;
    hr = D3DReadFileToBlob(vsPath.c_str(), &pVSBlob);
    if (FAILED(hr)) throw GraphicsHrException(__LINE__, __FILE__, hr);

    hr = pDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &pVS);
    if (FAILED(hr)) throw GraphicsHrException(__LINE__, __FILE__, hr);

    wrl::ComPtr<ID3DBlob> pPSBlob;
    hr = D3DReadFileToBlob(psPath.c_str(), &pPSBlob);
    if (FAILED(hr)) throw GraphicsHrException(__LINE__, __FILE__, hr);

    hr = pDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &pPS);
    if (FAILED(hr)) throw GraphicsHrException(__LINE__, __FILE__, hr);

    D3D11_INPUT_ELEMENT_DESC ied[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };

    hr = pDevice->CreateInputLayout(ied, ARRAYSIZE(ied),
        pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &pInputLayout);
    if (FAILED(hr)) throw GraphicsHrException(__LINE__, __FILE__, hr);

    D3D11_BUFFER_DESC cbd = {};
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbd.Usage = D3D11_USAGE_DYNAMIC;
    cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    cbd.ByteWidth = sizeof(Skybox::SkyboxCBufData);

    hr = pDevice->CreateBuffer(&cbd, nullptr, &pConstBuffer);
    if (FAILED(hr)) throw GraphicsHrException(__LINE__, __FILE__, hr);

    D3D11_SAMPLER_DESC sd = {};
    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sd.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sd.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;

    hr = pDevice->CreateSamplerState(&sd, &pSamplerState);
    if (FAILED(hr)) throw GraphicsHrException(__LINE__, __FILE__, hr);

    CreateCubeMesh(pDevice);
}

void Skybox::LoadTexture(ID3D11Device* pDevice, const std::wstring& texturePath)
{
    HRESULT hr;
    hr = DirectX::CreateDDSTextureFromFile(pDevice, texturePath.c_str(), nullptr, &pSkyboxSRV);
    if (FAILED(hr)) throw GraphicsHrException(__LINE__, __FILE__, hr);
}

void Skybox::SetViewMatrix(DirectX::FXMMATRIX view)
{
    DirectX::XMStoreFloat4x4(&viewMatrix, view);
}

void Skybox::SetProjMatrix(DirectX::FXMMATRIX proj)
{
    DirectX::XMStoreFloat4x4(&projMatrix, proj);
}


void Skybox::Draw(ID3D11DeviceContext* pContext)
{
    OutputDebugStringA("Skybox::Draw - START\n");

    ID3D11Device* pDevice = nullptr;
    pContext->GetDevice(&pDevice);

    // 顶点着色器：带简单旋转
    const char* vsSource =
        "cbuffer CBuf : register(b0) { matrix world; }"
        "struct VSInput { float4 pos : POSITION; };"
        "struct VSOutput { float4 pos : SV_POSITION; float3 texCoord : TEXCOORD0; };"
        "VSOutput main(VSInput input) {"
        "    VSOutput output;"
        "    float4 worldPos = mul(input.pos, world);"
        "    output.pos = worldPos;"
        "    output.texCoord = -input.pos.xyz;"
        "    return output;"
        "}";

    wrl::ComPtr<ID3DBlob> pVSBlob;
    wrl::ComPtr<ID3DBlob> pErrorBlob;
    HRESULT hr = D3DCompile(vsSource, strlen(vsSource), nullptr, nullptr, nullptr,
        "main", "vs_5_0", 0, 0, &pVSBlob, &pErrorBlob);

    if (FAILED(hr))
    {
        OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
        return;
    }

    wrl::ComPtr<ID3D11VertexShader> pTempVS;
    pDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &pTempVS);
    pContext->VSSetShader(pTempVS.Get(), nullptr, 0);

    D3D11_INPUT_ELEMENT_DESC layoutDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };
    wrl::ComPtr<ID3D11InputLayout> pTempLayout;
    pDevice->CreateInputLayout(layoutDesc, 1, pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &pTempLayout);
    pContext->IASetInputLayout(pTempLayout.Get());

    const char* psSource =
        "TextureCube gTex : register(t0);"
        "SamplerState gSam : register(s0);"
        "struct PSInput { float4 posH : SV_POSITION; float3 texCoord : TEXCOORD0; };"
        "float4 main(PSInput input) : SV_TARGET {"
        "    return gTex.Sample(gSam, input.texCoord);"
        "}";

    wrl::ComPtr<ID3DBlob> pPSBlob;
    D3DCompile(psSource, strlen(psSource), nullptr, nullptr, nullptr,
        "main", "ps_5_0", 0, 0, &pPSBlob, &pErrorBlob);

    wrl::ComPtr<ID3D11PixelShader> pTempPS;
    pDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &pTempPS);
    pContext->PSSetShader(pTempPS.Get(), nullptr, 0);

    // ========== 设置旋转矩阵 ==========
    struct CBData { DirectX::XMMATRIX world; };
    CBData cb;

    static float angle = 0.0f;
    angle += 0.01f;
    cb.world = DirectX::XMMatrixTranspose(DirectX::XMMatrixRotationY(angle));

    D3D11_BUFFER_DESC cbd = {};
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbd.Usage = D3D11_USAGE_DEFAULT;
    cbd.ByteWidth = sizeof(CBData);
    D3D11_SUBRESOURCE_DATA csd = { &cb };

    wrl::ComPtr<ID3D11Buffer> pTempCB;
    pDevice->CreateBuffer(&cbd, &csd, &pTempCB);
    pContext->VSSetConstantBuffers(0, 1, pTempCB.GetAddressOf());

    pContext->PSSetShaderResources(0, 1, pSkyboxSRV.GetAddressOf());
    pContext->PSSetSamplers(0, 1, pSamplerState.GetAddressOf());
    pContext->OMSetDepthStencilState(nullptr, 0);

    // 双面渲染
    D3D11_RASTERIZER_DESC rasterDesc = {};
    rasterDesc.FillMode = D3D11_FILL_SOLID;
    rasterDesc.CullMode = D3D11_CULL_NONE;
    wrl::ComPtr<ID3D11RasterizerState> pRS;
    pDevice->CreateRasterizerState(&rasterDesc, &pRS);
    pContext->RSSetState(pRS.Get());

    UINT stride = sizeof(DirectX::XMFLOAT3);
    UINT offset = 0;
    pContext->IASetVertexBuffers(0, 1, pVertexBuffer.GetAddressOf(), &stride, &offset);
    pContext->IASetIndexBuffer(pIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
    pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    pContext->DrawIndexed(indexCount, 0, 0);

    pDevice->Release();

    OutputDebugStringA("Skybox::Draw - END\n");
}

void Skybox::CreateCubeMesh(ID3D11Device* pDevice)
{
    // 改成 -1 到 1
    DirectX::XMFLOAT3 vertices[] = {
        {-1.0f,  1.0f, -1.0f},
        { 1.0f,  1.0f, -1.0f},
        { 1.0f,  1.0f,  1.0f},
        {-1.0f,  1.0f,  1.0f},
        {-1.0f, -1.0f, -1.0f},
        { 1.0f, -1.0f, -1.0f},
        { 1.0f, -1.0f,  1.0f},
        {-1.0f, -1.0f,  1.0f}
    };

    UINT indices[] = {
        0,1,2, 0,2,3,
        4,6,5, 4,7,6,
        0,3,7, 0,7,4,
        1,5,6, 1,6,2,
        3,2,6, 3,6,7,
        0,4,5, 0,5,1
    };

    indexCount = ARRAYSIZE(indices);

    D3D11_BUFFER_DESC vbd = {};
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.Usage = D3D11_USAGE_IMMUTABLE;
    vbd.ByteWidth = sizeof(vertices);
    D3D11_SUBRESOURCE_DATA vsd = { vertices };

    HRESULT hr = pDevice->CreateBuffer(&vbd, &vsd, &pVertexBuffer);
    if (FAILED(hr)) throw GraphicsHrException(__LINE__, __FILE__, hr);

    D3D11_BUFFER_DESC ibd = {};
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.Usage = D3D11_USAGE_IMMUTABLE;
    ibd.ByteWidth = sizeof(indices);
    D3D11_SUBRESOURCE_DATA isd = { indices };

    hr = pDevice->CreateBuffer(&ibd, &isd, &pIndexBuffer);
    if (FAILED(hr)) throw GraphicsHrException(__LINE__, __FILE__, hr);
}