// GraphicsResource.h
#pragma once

class Graphics;  // 前向声明
struct ID3D11Device;
struct ID3D11DeviceContext;
class DxgiInfoManager;

class GraphicsResource
{
public:
    static ID3D11DeviceContext* GetContext(Graphics& gfx) noexcept;
    static ID3D11Device* GetDevice(Graphics& gfx) noexcept;
    static DxgiInfoManager& GetInfoManager(Graphics& gfx);
};
