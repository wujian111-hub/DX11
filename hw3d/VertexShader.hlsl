cbuffer CBuf : register(b0)
{
    matrix transform;
};

struct VSInput
{
    float3 pos : Position;
    float3 color : Color;
};

struct VSOutput
{
    float4 pos : SV_POSITION;
    float3 color : COLOR;
};

VSOutput main(VSInput input)
{
    VSOutput output;
    output.pos = mul(float4(input.pos, 1.0f), transform);
    output.color = input.color;
    return output;
}