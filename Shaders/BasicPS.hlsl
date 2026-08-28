struct PSInput
{
    float4 position : SV_Position;
    float4 color : COLOR;
};

float4 main(PSInput input) : SV_Target
{
    return input.color; // input color를 그대로 출력
    // Vertex Shadder : 정점 위치 + 색
    // -> Rasterizer : 정점 사이 색상 보간
    // -> Pixel Shader : 보간된 색상 출력
}