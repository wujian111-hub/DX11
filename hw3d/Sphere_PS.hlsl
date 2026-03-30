Texture2D g_Tex : register(t0);
SamplerState g_SamLinear : register(s0);

struct PSInput
{
    float4 pos : SV_POSITION;
    float2 tex : TEXCOORD;
};

// 使用 ps_5_0 编译这个函数
float4 main(PSInput input) : SV_TARGET
{
    return g_Tex.Sample(g_SamLinear, input.tex);
}

