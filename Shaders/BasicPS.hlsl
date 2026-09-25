Texture2D spriteTexture : register(t0);
SamplerState spriteSampler : register(s0);

struct PSInput
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD;
    float4 color : COLOR;
};

float4 main(PSInput input) : SV_Target
{
    return spriteTexture.Sample(spriteSampler, input.uv) * input.color;
}

float4 mainCutout(PSInput input) : SV_Target
{
    float4 color = spriteTexture.Sample(spriteSampler, input.uv) * input.color;
    clip(color.a - 0.5f);
    return color;
}
