#pragma pack_matrix(row_major)

struct debug_vertex {
	float4 position : SV_POSITION; 
	float4 color : COLOR;
};

cbuffer shader_vars {
	matrix viewProjection;
};

debug_vertex main(float2 pos : POS, float4 clr : CLR) {
	debug_vertex output;
	output.position = mul(float4(pos,0,1), viewProjection);
	output.color = clr;
	return output;
}