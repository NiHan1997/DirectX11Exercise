/// 模板参考值.
cbuffer StencilConstants : register(b0)
{
	uint gStencil;
	uint pad0;
	uint pad1;
	uint pad2;
};

/// 顶点着色器输出, 像素着色器输入.
struct VertexOut
{
	float4 PosH	: SV_POSITION;
	float2 TexC : TEXCOORD0;
};

/// 屏幕四个顶点的纹理坐标, 屏幕后处理特效的标配.
static const float2 gTexCoords[6] =
{
	float2(0.0f, 1.0f),
	float2(0.0f, 0.0f),
	float2(1.0f, 0.0f),
	float2(0.0f, 1.0f),
	float2(1.0f, 0.0f),
	float2(1.0f, 1.0f)
};

/// 顶点着色器.
VertexOut VS(uint vid : SV_VertexID)
{
	VertexOut vout;

	// 将顶点纹理坐标转化到NDC空间, 这里的效果就是覆盖整个窗口.
	vout.TexC = gTexCoords[vid];
	vout.PosH = float4(2.0f * vout.TexC.x - 1.0f, 1.0f - 2.0f * vout.TexC.y, 0.0f, 1.0f);

	return vout;
}

/// 像素着色器.
float4 PS(VertexOut pin) : SV_Target
{
	float4 green = float4(0.48, 0.94, 0.41, 1.0);			// 绘制 1 次.
	float4 blue = float4(0.38, 0.84, 1.0, 1.0);				// 绘制 2 次.
	float4 orange = float4(0.96, 0.60, 0.05, 1.0);			// 绘制 3 次.
	float4 red = float4(0.96, 0.25, 0.38, 1.0);				// 绘制 4 次.
	float4 purple = float4(0.73, 0.53, 0.95, 1.0);			// 绘制 5 次.

	if (gStencil == 1)
	   return green;
	else if (gStencil == 2)
		return blue;
	else if (gStencil == 3)
		return orange;
	else if (gStencil == 4)
		return red;
	else
		return purple;
}
