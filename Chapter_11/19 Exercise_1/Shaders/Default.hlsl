///***********************************************************************************
/// 通用的不透明物体的着色器.
///***********************************************************************************

// 基础光源的定义.
#ifndef NUM_DIR_LIGHTS
	#define NUM_DIR_LIGHTS 1
#endif

#ifndef NUM_DIR_LIGHTS
	#define NUM_DIR_LIGHTS 0
#endif

#ifndef NUM_DIR_LIGHTS
	#define NUM_DIR_LIGHTS 0
#endif

#include "LightingUtil.hlsl"

/// ObjectCB的常量缓冲区结构体.
cbuffer ObjectConstants : register(b0)
{
	float4x4 gWorld;
	float4x4 gTexTransform;
};

/// MaterialCB的常量缓冲区结构体.
cbuffer MaterialConstants : register(b1)
{
	float4 gDiffuseAlbedo;
	float3 gFresnelR0;
	float gRoughness;
	float4x4 gMatTransform;
};

/// PassCB的常量缓冲区结构体.
cbuffer PassConstants : register(b2)
{
	float4x4 gView;
	float4x4 gInvView;
	float4x4 gProj;
	float4x4 gInvProj;
	float4x4 gViewProj;
	float4x4 gInvViewProj;

	float3 gEyePosW;
	float cbPad0;
	float2 gRenderTargetSize;
	float2 gInvRenderTargetSize;

	float gNearZ;
	float gFarZ;
	float gTotalTime;
	float gDeltaTime;

	float4 gAmbientLight;
	float4 gFogColor;
	float gFogStart;
	float gFogRange;
	float2 cbPad1;
	Light gLights[MaxLights];
};

/// 主纹理.
Texture2D gDiffuseMap : register(t0);

/// 采样器.
SamplerState gsamPointWrap        : register(s0);
SamplerState gsamPointClamp       : register(s1);
SamplerState gsamLinearWrap       : register(s2);
SamplerState gsamLinearClamp      : register(s3);
SamplerState gsamAnisotropicWrap  : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

/// 顶点着色器输入.
struct VertexIn
{
	float3 PosL : POSITION;
};

/// 顶点着色器输出, 几何着色器输入.
struct VertexOut
{
	float3 PosL : POSITION;
};

/// 顶点着色器输出, 几何着色器输入.
struct GeoOut
{
	float4 PosH	: SV_POSITION;
};

/// 顶点着色器.
VertexOut VS (VertexIn vin)
{
	VertexOut vout;
	vout.PosL = vin.PosL;

	return vout;
}

/// 几何着色器.
[maxvertexcount(4)]
void GS(line VertexOut gin[2], inout LineStream<GeoOut> lineStream)
{
	float3 v[4];
	v[0] = gin[0].PosL;
	v[1] = gin[1].PosL;
	v[2] = gin[1].PosL + float3(0.0, 5.0, 0.0);
	v[3] = gin[0].PosL + float3(0.0, 5.0, 0.0);

	GeoOut geoOut;
	[unroll]
	for (int i = 0; i < 4; ++i)
	{
		geoOut.PosH = mul(float4(v[i], 1.0), gViewProj);

		lineStream.Append(geoOut);
	}
}

/// 像素着色器.
float4 PS (GeoOut pin) : SV_Target
{
	return float4(1.0, 0.0, 0.0, 1.0);
}
