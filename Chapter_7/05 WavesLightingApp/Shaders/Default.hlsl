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
};

/// MaterialCB的常量缓冲区结构体.
cbuffer MaterialConstants : register(b1)
{
	float4 gDiffsueAlbedo;
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
	Light gLights[MaxLights];
};

/// 顶点着色器输入.
struct VertexIn
{
	float3 PosL	   : POSITION;
	float3 NormalL : NORMAL;
	float4 Color   : COLOR;
};

/// 顶点着色器输出, 像素着色器输入.
struct VertexOut
{
	float4 PosH	   : SV_POSITION;
	float4 PosW    : POSITION;
	float3 NormalW : NORMAL;
	float4 Color   : COLOR;
};


/// 顶点着色器.
VertexOut VS (VertexIn vin)
{
	VertexOut vout;

	vout.PosW = mul(float4(vin.PosL, 1.0), gWorld);
	vout.PosH = mul(vout.PosW, gViewProj);

	// 这里法线变换需要注意, 这里是在等比变换的基础上, 因此不需要使用逆转置矩阵.
	vout.NormalW = mul(vin.NormalL, (float3x3)gWorld);

	vout.Color = vin.Color;

	return vout;
}

/// 像素着色器.
float4 PS (VertexOut pin) : SV_Target
{
	// 获取环境光.
	float4 ambient = gAmbientLight * gDiffsueAlbedo;

	// 关照计算必需.
	pin.NormalW = normalize(pin.NormalW);
	float3 toEyeW = normalize(gEyePosW - pin.PosW.xyz);

	const float gShininess = 1 - gRoughness;
	Material mat = { gDiffsueAlbedo, gFresnelR0, gShininess };
	float4 dirLight = ComputeLighting(gLights, mat, pin.PosW.xyz, pin.NormalW, toEyeW, 1.0);

	float4 finalColor = ambient + dirLight * pin.Color;
	finalColor.a = gDiffsueAlbedo.a;

	return finalColor;
}
