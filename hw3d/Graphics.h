#pragma once

#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>
#include <d3dcompiler.h>
#include <stdexcept>
#include <string>
#include "DxgiInfoManager.h"
#include <DirectXMath.h>
#include "Skybox.h"

// 宏定义
#ifndef GFX_THROW_INFO
#define GFX_THROW_INFO(hrcall) \
{ \
    HRESULT hr = (hrcall); \
    if (FAILED(hr)) \
    { \
        throw GraphicsHrException(__LINE__, __FILE__, hr); \
    } \
}
#endif

#ifndef GFX_THROW_INFO_MSG
#define GFX_THROW_INFO_MSG(hrcall, msg) \
{ \
    HRESULT hr = (hrcall); \
    if (FAILED(hr)) \
    { \
        throw GraphicsHrException(__LINE__, __FILE__, hr, msg); \
    } \
}
#endif

#ifndef INFOMAN
#define INFOMAN(gfx) \
    HRESULT hr; \
    DxgiInfoManager& infoManager = (gfx).GetInfoManager()
#endif

namespace wrl = Microsoft::WRL;

// ==================== 异常类定义 ====================

class GraphicsException : public std::runtime_error
{
public:
    GraphicsException(int line, const char* file, const std::string& note = "") noexcept;
    const char* what() const noexcept override;
    virtual const char* GetType() const noexcept;
    int GetLine() const noexcept;
    const std::string& GetFile() const noexcept;
    std::string GetOriginString() const noexcept;

private:
    int line;
    std::string file;
};

class GraphicsHrException : public GraphicsException
{
public:
    GraphicsHrException(int line, const char* file, HRESULT hr) noexcept;
    GraphicsHrException(int line, const char* file, HRESULT hr, const std::string& info) noexcept;
    const char* what() const noexcept override;
    const char* GetType() const noexcept override;
    HRESULT GetErrorCode() const noexcept;
    std::string GetErrorString() const noexcept;
    std::string GetErrorInfo() const noexcept;

private:
    HRESULT hr;
    std::string info;
};

class GraphicsDeviceRemovedException : public GraphicsHrException
{
public:
    GraphicsDeviceRemovedException(int line, const char* file, HRESULT hr, const std::string& info = "") noexcept;
    const char* GetType() const noexcept override;
};

class GraphicsInfoException : public GraphicsException
{
public:
    GraphicsInfoException(int line, const char* file, const std::string& info) noexcept;
    const char* what() const noexcept override;
    const char* GetType() const noexcept override;
    const std::string& GetInfo() const noexcept;

private:
    std::string info;
};

// ==================== Graphics 类 ====================

class Graphics
{
    friend class GraphicsResource;

public:

    // 相机控制
    void SetCameraRotation(float yaw, float pitch);
    void GetCameraRotation(float& yaw, float& pitch) const;
    void RotateCamera(float deltaYaw, float deltaPitch);


    Graphics(HWND hWnd, int width, int height);
    Graphics(const Graphics&) = delete;
    ~Graphics();

    void BeginFrame(float red, float green, float blue);
    void EndFrame();
    void ClearBuffer(float red, float green, float blue) noexcept;

    // 设备访问
    ID3D11Device* GetDevice() const noexcept { return pDevice.Get(); }
    ID3D11DeviceContext* GetContext() const noexcept { return pContext.Get(); }

    // 天空盒控制
    void InitializeSkybox();
    void LoadSkyboxTexture(const std::wstring& texturePath);
    void DrawSkybox();

    DxgiInfoManager& GetInfoManager() noexcept { return infoManager; }



private:
    int width;
    int height;

    float m_cameraYaw = 0.0f;
    float m_cameraPitch = 0.0f;

    Microsoft::WRL::ComPtr<ID3D11Device> pDevice;
    Microsoft::WRL::ComPtr<IDXGISwapChain> pSwap;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> pContext;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> pTarget;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> pDSV;
    DxgiInfoManager infoManager;

    // 天空盒
    Skybox skybox;
};