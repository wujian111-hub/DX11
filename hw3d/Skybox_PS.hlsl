TextureCube gTex : register(t0);
SamplerState gSam : register(s0);

struct PSInput
{
    float4 posH : SV_POSITION;
    float3 texCoord : TEXCOORD0;
};

float4 main(PSInput input) : SV_TARGET
{
    return gTex.Sample(gSam, input.texCoord);
}