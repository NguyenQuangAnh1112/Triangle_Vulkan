struct VSInPut {
    float2 inPosition;
    float3 inColor;
};


struct VertexOutput {
    float4 sv_position : SV_Position;
    float3 color;
};


[shader("vertex")]
VertexOutput vertMain(VSInPut input) {
    VertexOutput output;
    output.sv_position = float4(input.inPosition, 0.0, 1.0);
    output.color = input.inColor;
    return output;
}


[shader("fragment")]
float4 fragMain(VertexOutput inVert) : SV_Target {
    return float4(inVert.color, 1.0);
};
