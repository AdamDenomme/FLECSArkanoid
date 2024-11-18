struct debug_vertex {
	float4 position : SV_POSITION;
	float4 color : COLOR;
};

float4 main(debug_vertex fromVS) : SV_TARGET {
	return fromVS.color;
}