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

/// 给定三角形顶点, 对三角形进行细分.
void SubdivideTriangle(VertexOut v0, VertexOut v1, VertexOut v2,
	inout TriangleStream<GeoOut> triangleStream)
{
	float len = length(v0.PosW);

	float3 m0 = normalize((v0.PosW + v1.PosW) * 0.5) * len;
	float3 m1 = normalize((v1.PosW + v2.PosW) * 0.5) * len;
	float3 m2 = normalize((v0.PosW + v2.PosW) * 0.5) * len;

	float4 v[6];
	v[0] = float4(v0.PosW, 1.0);
	v[1] = float4(m0, 1.0);
	v[2] = float4(m2, 1.0);
	v[3] = float4(m1, 1.0);
	v[4] = float4(v2.PosW, 1.0);
	v[5] = float4(v1.PosW, 1.0);

	GeoOut geoOut[6];
	[unroll]
	for (int i = 0; i < 6; ++i)
	{
		geoOut[i].PosH = mul(v[i], gViewProj);
		geoOut[i].PosW = v[i].xyz;
	}

	[unroll]
	for (int j = 0; j < 5; ++j)
		triangleStream.Append(geoOut[j]);

	triangleStream.RestartStrip();

	triangleStream.Append(geoOut[1]);
	triangleStream.Append(geoOut[5]);
	triangleStream.Append(geoOut[3]);

	triangleStream.RestartStrip();
}

/// 几何着色器.
[maxvertexcount(40)]
void GS(triangle VertexOut gin[3], inout TriangleStream<GeoOut> triangleStream)
{
	float distance = length(gEyePosW);

	// 不细分.
	if (distance >= 30)
	{
		float4 v[3];
		v[0] = float4(gin[0].PosW, 1.0);
		v[1] = float4(gin[1].PosW, 1.0);
		v[2] = float4(gin[2].PosW, 1.0);

		GeoOut geoOut[3];
		[unroll]
		for (int i = 0; i < 3; ++i)
		{
			geoOut[i].PosH = mul(v[i], gViewProj);
			geoOut[i].PosW = v[i].xyz;

			triangleStream.Append(geoOut[i]);
		}
	}
	// 一次细分.
	else if (distance >= 15)
	{
		SubdivideTriangle(gin[0], gin[1], gin[2], triangleStream);
	}
	// 两次细分.
	else
	{
		// 这里计算出一次细分后的顶点位置, 然后通过三角形细分方法进行对应细分.
		float len = length(gin[0].PosW);

		float3 m0 = normalize((gin[0].PosW + gin[1].PosW) * 0.5) * len;
		float3 m1 = normalize((gin[1].PosW + gin[2].PosW) * 0.5) * len;
		float3 m2 = normalize((gin[0].PosW + gin[2].PosW) * 0.5) * len;

		VertexOut v[6];
		v[0].PosW = gin[0].PosW;
		v[1].PosW = m0;
		v[2].PosW = m2;
		v[3].PosW = m1;
		v[4].PosW = gin[2].PosW;
		v[5].PosW = gin[1].PosW;

		// 三角形二次细分.
		SubdivideTriangle(v[0], v[1], v[2], triangleStream);
		SubdivideTriangle(v[1], v[3], v[2], triangleStream);
		SubdivideTriangle(v[2], v[3], v[4], triangleStream);
		SubdivideTriangle(v[3], v[1], v[5], triangleStream);
	}
}


/// 像素着色器.
float4 PS (GeoOut pin) : SV_Target
{
	return float4(1.0, 0.0, 0.0, 1.0);
}
