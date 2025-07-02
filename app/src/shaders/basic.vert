struct VSInput {
    float3 pos : POSITION;
    float3 col : COLOR;
};
struct PSInput {
    float4 pos : SV_POSITION;
    float4 col : COLOR;
};

PSInput main(VSInput input) {
    PSInput o;
    o.pos = float4(input.pos, 1.0);
    o.col = float4(input.col, 1.0);
    return o;
}