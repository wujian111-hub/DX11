#include "Sampler.h"
#include "GraphicsThrowMacros.h"

Sampler::Sampler(Graphics& gfx)
{
	INFOMAN(gfx);
	D3D11_SAMPLER_DESC samplerDesc ={};
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU =D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV =D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MaxAnisotropy = D3D11_REQ_MAXANISOTROPY;
	//samplerDesc.BorderColor[0] = 0.0f;
	//samplerDesc.BorderColor[1] = 1.0f;
	//samplerDesc.BorderColor[2] = 0.0f;
	//samplerDesc.BorderColor[3] = 0.0f;

	GFX_THROW_INFO(GetDevice(gfx)->CreateSamplerState(&samplerDesc, &pSampler));
}

void Sampler::Bind(Graphics& gfx) noexcept
{
	GetContext(gfx)->PSSetSamplers(0, 1, pSampler.GetAddressOf());
}
	