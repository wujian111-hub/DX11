struct VSOut
{
    float4 pos : SV_POSITION;
    float3 color : Color;
};

cbuffer CBuf
{
    matrix transform;
};

VSOut main(float3 pos : Position, float3 color : Color)
{
    VSOut vso;
    vso.pos = mul(float4(pos, 1.0f),transform);
    vso.color = color;
    return vso;
}