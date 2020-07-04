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
	float3 PosL	   : POSITION;
	float3 NormalL : NORMAL;
	float2 TexC    : TEXCOORD0;
};

/// 顶点着色器输出, 几何着色器输入.
struct VertexOut
{
	float4 PosW    : POSITION;
	float3 NormalW : NORMAL;
	float2 TexC    : TEXCOORD0;
};

/// 几何着色器输出, 像素着色器输入.
struct GeoOut
{
	float4 PosH	   : SV_POSITION;
	float4 PosW    : POSITION;
	float3 NormalW : NORMAL;
	float2 TexC    : TEXCOORD0;
};

/// 顶点着色器.
VertexOut VS(VertexIn vin)
{
	VertexOut vout;

	vout.PosW = mul(float4(vin.PosL, 1.0), gWorld);

	// 这里法线变换需要注意, 这里是在等比变换的基础上, 因此不需要使用逆转置矩阵.
	vout.NormalW = mul(vin.NormalL, (float3x3)gWorld);

	float4 texC = mul(float4(vin.TexC, 0.0, 1.0), gTexTransform);
	vout.TexC = mul(texC, gMatTransform).xy;

	return vout;
}

[maxvertexcount(6)]
void GS(triangle VertexOut gin[3], inout TriangleStream<GeoOut> triangleStream)
{
	// 原始的3个顶点.
	[unroll]
	for (int i = 0; i < 3; ++i)
	{
		GeoOut geoOut;
		geoOut.PosW = gin[i].PosW;
		geoOut.PosH = mul(gin[i].PosW, gViewProj);
		geoOut.NormalW = gin[i].NormalW;
		geoOut.TexC = gin[i].TexC;

		triangleStream.Append(geoOut);
	}
	triangleStream.RestartStrip();

	// 求出重心坐标.
	float4 pivot = (gin[0].PosW + gin[1].PosW + gin[2].PosW) / 3;
	float2 texC = (gin[0].TexC + gin[1].TexC + gin[2].TexC) / 3;

	// 求出表面法线.
	float3 v1 = (gin[1].PosW - gin[0].PosW).xyz;
	float3 v2 = (gin[2].PosW - gin[0].PosW).xyz;
	float3 normal = normalize(cross(v1, v2));

	GeoOut geoOut[3];
	geoOut[0].PosW = pivot;
	geoOut[0].PosH = mul(pivot, gViewProj);
	geoOut[0].NormalW = normal;
	geoOut[0].TexC = texC;

	geoOut[1].PosW = pivot + float4(normal, 0.0) * 1.5;
	geoOut[1].PosH = mul(geoOut[1].PosW, gViewProj);
	geoOut[1].NormalW = normal;
	geoOut[1].TexC = texC;

	geoOut[2] = geoOut[1];

	triangleStream.Append(geoOut[0]);
	triangleStream.Append(geoOut[1]);
	triangleStream.Append(geoOut[2]);

	triangleStream.RestartStrip();
}

/// 像素着色器.
float4 PS(GeoOut pin) : SV_Target
{
	// 纹理采样.
	float4 diffuseAlbedo = gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC) * gDiffuseAlbedo;

	/// 透明度测试逻辑.
#ifdef ALPHA_TEST
	clip(diffuseAlbedo.a - 0.1);
#endif

	// 获取环境光.
	float4 ambient = gAmbientLight * diffuseAlbedo;

	// 关照计算必需.
	pin.NormalW = normalize(pin.NormalW);
	float3 toEyeW = gEyePosW - pin.PosW.xyz;
	float distToEye = length(toEyeW);
	toEyeW /= distToEye;

	const float gShininess = 1 - gRoughness;
	Material mat = { diffuseAlbedo, gFresnelR0, gShininess };
	float4 dirLight = ComputeLighting(gLights, mat, pin.PosW.xyz, pin.NormalW, toEyeW, 1.0);

	float4 finalColor = ambient + dirLight;

#ifdef FOG
	float fogAmount = saturate((distToEye - gFogStart) / gFogRange);
	finalColor = lerp(finalColor, gFogColor, fogAmount);
#endif

	finalColor.a = gDiffuseAlbedo.a;

	return finalColor;
}
