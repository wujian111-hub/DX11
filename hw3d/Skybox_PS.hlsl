TextureCube skyboxTex : register(t0);
SamplerState samLinear : register(s0);

struct PSInput
{
    float4 posH : SV_POSITION;
    float3 texCoord : TEXCOORD0;
};

float4 main(PSInput input) : SV_target
{
    return skyboxTex.Sample(samLinear, input.texCoord);
}