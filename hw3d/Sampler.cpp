#include "Sampler.h"
#include "Graphics.h"
#include "GraphicsThrowMacros.h"
#include "BindableCodex.h"

namespace Bind
{
    Sampler::Sampler(Graphics& gfx, Type type, bool reflect, UINT slot)
        : type(type), reflect(reflect), slot(slot)
    {
        D3D11_SAMPLER_DESC samplerDesc = {};

        switch (type)
        {
        case Type::Anisotropic:
            samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
            samplerDesc.MaxAnisotropy = D3D11_REQ_MAXANISOTROPY;
            break;
        case Type::Bilinear:
            samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
            samplerDesc.MaxAnisotropy = 1;
            break;
        case Type::Point:
            samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
            samplerDesc.MaxAnisotropy = 1;
            break;
        }

        samplerDesc.AddressU = reflect ? D3D11_TEXTURE_ADDRESS_MIRROR : D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressV = reflect ? D3D11_TEXTURE_ADDRESS_MIRROR : D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressW = reflect ? D3D11_TEXTURE_ADDRESS_MIRROR : D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

        // 显式处理HRESULT
        HRESULT hr = gfx.GetDevice()->CreateSamplerState(&samplerDesc, &pSampler);
        if (FAILED(hr))
        {
            throw GraphicsHrException(__LINE__, __FILE__, hr);
        }
    }

	void Sampler::Bind(Graphics& gfx) noexcept
	{
		gfx.GetContext()->PSSetSamplers(slot, 1, pSampler.GetAddressOf());
	}

	std::shared_ptr<Sampler> Sampler::Resolve(Graphics& gfx, Type type, bool reflect, UINT slot)
	{
		return Codex::Resolve<Sampler>(gfx, type, reflect, slot);
	}

	std::string Sampler::GenerateUID(Type type, bool reflect, UINT slot)
	{
		using namespace std::string_literals;
		return typeid(Sampler).name() + "#"s + std::to_string((int)type) + "#" + std::to_string(reflect) + "#" + std::to_string(slot);
	}

	std::string Sampler::GetUID() const noexcept
	{
		return GenerateUID(type, reflect, slot);
	}
}
	