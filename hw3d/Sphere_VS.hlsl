cbuffer CBuf : register(b0)
{
    matrix g_World;
    matrix g_View;
    matrix g_Proj;
    matrix g_TexRot;
};

struct VSInput
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float2 tex : TEXCOORD;
};

struct VSOutput
{
    float4 pos : SV_POSITION;
    float2 tex : TEXCOORD;
};

VSOutput main(VSInput input)
{
    VSOutput output;

    float4 posW = mul(float4(input.pos, 1.0f), g_World);
    float4 posV = mul(posW, g_View);
    output.pos = mul(posV, g_Proj);

    float2 centered = input.tex - float2(0.5f, 0.5f);
    float4 t = float4(centered, 0.0f, 1.0f);
    float4 tRot = mul(t, g_TexRot);
    output.tex = tRot.xy + float2(0.5f, 0.5f);

    return output;
}

