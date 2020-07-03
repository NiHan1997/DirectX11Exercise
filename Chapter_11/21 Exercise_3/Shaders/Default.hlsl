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
	float3 PosW : POSITION;
};

/// 顶点着色器输出, 几何着色器输入.
struct GeoOut
{
	float3 PosW : POSITION;
	float4 PosH	: SV_POSITION;
};

/// 顶点着色器.
VertexOut VS (VertexIn vin)
{
	VertexOut vout;
	vout.PosW = mul(float4(vin.PosL, 1.0), gWorld).xyz;

	return vout;
}

/// 几何着色器.
[maxvertexcount(3)]
void GS(triangle VertexOut gin[3], uint primID : SV_PrimitiveID,
	inout TriangleStream<GeoOut> triangleStream)
{
	// 根据图元的不同ID产生不同的速度.
	float speed = (primID % 3 + 1) * 0.5;

	// 计算爆炸方向.
	float3 v1 = float3(gin[1].PosW - gin[0].PosW);
	float3 v2 = float3(gin[2].PosW - gin[0].PosW);
	float3 normal = cross(v1, v2);

	GeoOut geoOut;
	[unroll]
	for (int i = 0; i < 3; ++i)
	{
		// 延法线方向扩张.
		geoOut.PosW = gin[i].PosW + speed * gTotalTime * normal;
		geoOut.PosH = mul(float4(geoOut.PosW, 1.0), gViewProj);

		triangleStream.Append(geoOut);
	}
	triangleStream.RestartStrip();
}

/// 像素着色器.
float4 PS (GeoOut pin) : SV_Target
{
	return float4(1.0, 0.0, 0.0, 1.0);
}
