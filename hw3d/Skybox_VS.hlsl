struct VSInput
{
    float3 posL : POSITION;
};

struct VSOutput
{
    float4 posH : SV_POSITION;
    float3 texCoord : TEXCOORD0;
};

VSOutput main(VSInput input)
{
    VSOutput output;
    
    // 直接输出位置，不做任何变换
    output.posH = float4(input.posL, 1.0f);
    output.texCoord = input.posL;
    
    return output;
}