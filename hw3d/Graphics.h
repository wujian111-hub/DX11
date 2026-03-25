#pragma once
#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>
#include <d3dcompiler.h>
#include <stdexcept> 
#include <string>
#include "DxgiInfoManager.h"

#ifndef GFX_THROW_INFO
#define GFX_THROW_INFO(hrcall) \
{ \
    HRESULT hr = (hrcall); \
    if (FAILED(hr)) \
    { \
        throw Graphics::HrException(__LINE__, __FILE__, hr); \
    } \
}
#endif

#ifndef INFOMAN
#define INFOMAN(gfx) HRESULT hr; DxgiInfoManager& infoManager = GetInfoManager((gfx))
#endif

namespace wrl = Microsoft::WRL;

namespace Bind
{
	class Bindable;
	class RenderTarget;
}

class Graphics
{
	friend class GraphicsResource;

public:
	// 投影类型枚举
	enum class ProjectionType
	{
		Perspective,   // 透视投影
		Orthographic   // 正交投影
	};

	// 异常类定义
	class Exception : public std::runtime_error
	{
	public:
		Exception(int line, const char* file, const std::string& note = "") noexcept;
		const char* what() const noexcept override;
		virtual const char* GetType() const noexcept;
		int GetLine() const noexcept;
		const std::string& GetFile() const noexcept;
		std::string GetOriginString() const noexcept;
	private:
		int line;
		std::string file;
	};

	class HrException : public Exception
	{
	public:
		HrException(int line, const char* file, HRESULT hr) noexcept;
		HrException(int line, const char* file, HRESULT hr, const std::string& info) noexcept;
		const char* what() const noexcept override;
		const char* GetType() const noexcept override;
		HRESULT GetErrorCode() const noexcept;
		std::string GetErrorString() const noexcept;
		std::string GetErrorInfo() const noexcept;
	private:
		HRESULT hr;
		std::string info;
	};

	class DeviceRemovedException : public HrException
	{
	public:
		DeviceRemovedException(int line, const char* file, HRESULT hr, const std::string& info = "") noexcept;
		const char* GetType() const noexcept override;
	};

	class InfoException : public Exception
	{
	public:
		InfoException(int line, const char* file, const std::string& info) noexcept;
		const char* what() const noexcept override;
		const char* GetType() const noexcept override;
		const std::string& GetInfo() const noexcept;
	private:
		std::string info;
	};

public:
	Graphics(HWND hWnd, int width, int height);
	Graphics(const Graphics&) = delete;
	~Graphics() = default;

	void BeginFrame(float red, float green, float blue);
	void EndFrame();
	void ClearBuffer(float red, float green, float blue) noexcept;

	// 新的 Frustum 函数声明（带所有控制参数）
	void Frustum(float angle, float x, float y, float z,
		float scaleTop, float scaleBottom, float scaleHeight,
		float topR, float topG, float topB,
		float bottomR, float bottomG, float bottomB,
		float sideR, float sideG, float sideB);

	// 投影类型控制函数
	void SetProjectionType(ProjectionType type) { m_projectionType = type; }
	ProjectionType GetProjectionType() const { return m_projectionType; }

	// 正交投影视野大小控制函数
	void SetOrthoViewSize(float size) { m_orthoViewSize = size; }
	float GetOrthoViewSize() const { return m_orthoViewSize; }

	// 这两个函数放在这里，public 区域
	ID3D11Device* GetDevice() const noexcept { return pDevice.Get(); }
	ID3D11DeviceContext* GetContext() const noexcept { return pContext.Get(); }

private:
	int width;
	int height;

	Microsoft::WRL::ComPtr<ID3D11Device> pDevice;
	Microsoft::WRL::ComPtr<IDXGISwapChain> pSwap;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> pContext;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> pTarget;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> pDSV;
	DxgiInfoManager infoManager;

	// 投影相关成员变量
	ProjectionType m_projectionType = ProjectionType::Perspective;
	float m_orthoViewSize = 5.0f;  // 正交投影视野大小，默认5.0
};