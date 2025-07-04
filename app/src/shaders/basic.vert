struct VSInput
{
    float3 pos : POSITION;
    float3 col : COLOR;
};

struct PSInput
{
    float4 pos : SV_POSITION;
    float3 col : COLOR;
};

cbuffer UniformBufferObject : register(b0)
{
    matrix model;
    matrix view;
    matrix proj;
};

PSInput main(VSInput input)
{
    PSInput o;

    float4 worldPos = mul(model, float4(input.pos, 1.0));
    float4 viewPos = mul(view, worldPos);
    o.pos = mul(proj, viewPos);

    o.col = input.col;
    return o;
}
