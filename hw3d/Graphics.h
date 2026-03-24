#pragma once
#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>
#include <d3dcompiler.h>
#include <stdexcept> 

#ifndef GFX_THROW_INFO
#define GFX_THROW_INFO(hr) \
	if (FAILED(hr)) \
	{ \
		throw std::runtime_error("Graphics error"); \
	}
#endif

namespace wrl = Microsoft::WRL;

namespace Bind
{
	class Bindable;
	class RenderTarget;
}

class Graphics
{
public:
	Graphics(HWND hWnd, int width, int height);
	Graphics(const Graphics&) = delete;
	~Graphics() = default;

	void BeginFrame(float red, float green, float blue);
	void EndFrame();
	void ClearBuffer(float red, float green, float blue) noexcept;
	void Frustum(float angle, float x, float y);

private:
	int width;
	int height;

	Microsoft::WRL::ComPtr<ID3D11Device> pDevice;
	Microsoft::WRL::ComPtr<IDXGISwapChain> pSwap;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> pContext;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> pTarget;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> pDSV;
};