#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <string>

namespace wrl = Microsoft::WRL;

class Skybox
{
public:
    struct SkyboxCBufData
    {
        DirectX::XMMATRIX view;
        DirectX::XMMATRIX proj;
    };

    Skybox();
    ~Skybox();

    void Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext,
        const std::wstring& vsPath = L"Skybox_VS.cso",
        const std::wstring& psPath = L"Skybox_PS.cso");

    void LoadTexture(ID3D11Device* pDevice, const std::wstring& texturePath);

    // 绘制天空盒（需要传入 yaw/pitch 相机角度）
    void Draw(ID3D11DeviceContext* pContext, float yaw = 0.0f, float pitch = 0.0f);

private:
    void CreateCubeMesh(ID3D11Device* pDevice);

    wrl::ComPtr<ID3D11VertexShader> pVS;
    wrl::ComPtr<ID3D11PixelShader> pPS;
    wrl::ComPtr<ID3D11InputLayout> pInputLayout;
    wrl::ComPtr<ID3D11Buffer> pConstBuffer;
    wrl::ComPtr<ID3D11ShaderResourceView> pSkyboxSRV;
    wrl::ComPtr<ID3D11SamplerState> pSamplerState;
    wrl::ComPtr<ID3D11Buffer> pVertexBuffer;
    wrl::ComPtr<ID3D11Buffer> pIndexBuffer;
    UINT indexCount = 0;
};