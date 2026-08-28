struct VSInput
{
    float3 position : POSITION;
    float4 color : COLOR;
};

struct VSOutput
{
    float4 position : SV_POSITION; // 최종 출력 위치
    float4 color : COLOR;
};

VSOutput main(VSInput input)
{
    VSOutput output;
    
    // input vertex를 받아서 그대로 output vertex로 전달
    output.position = float4(input.position, 1.0f); // w=1의 의미는 다음에 학습
    output.color = input.color;
    
    return output;
}