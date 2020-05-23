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
	Light gLights[MaxLights];
};

/// 主纹理和副纹理.
Texture2D gDiffuseMap      : register(t0);
Texture2D gDiffuseMapAlpha : register(t1);

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

/// 顶点着色器输出, 像素着色器输入.
struct VertexOut
{
	float4 PosH	   : SV_POSITION;
	float4 PosW    : POSITION;
	float3 NormalW : NORMAL;
	float2 TexC    : TEXCOORD0;
};


/// 顶点着色器.
VertexOut VS (VertexIn vin)
{
	VertexOut vout;

	vout.PosW = mul(float4(vin.PosL, 1.0), gWorld);
	vout.PosH = mul(vout.PosW, gViewProj);

	// 这里法线变换需要注意, 这里是在等比变换的基础上, 因此不需要使用逆转置矩阵.
	vout.NormalW = mul(vin.NormalL, (float3x3)gWorld);

	// 纹理旋转动画.
	float2x2 rotation = float2x2(
		float2(cos(gTotalTime), sin(gTotalTime)),
		float2(-sin(gTotalTime), cos(gTotalTime)));

	// 纹理旋转的中心点是(0.5, 0.5), 因此需要偏移UV坐标.
	// 注意：这里一定要是减法, 因为原始旋旋转中心是(0, 0), 
	// 我们想要的效果是绕(0.5, 0.5)旋转, 因此将纹理坐标-0.5,
	// 也就将(0.5, 0.5)偏移到原始旋转中心(0, 0),
	// 旋转操作完成后, 再将纹理坐标偏移回去.
	vin.TexC -= float2(0.5, 0.5);
	vin.TexC = mul(vin.TexC, rotation);
	vin.TexC += float2(0.5, 0.5);

	float4 texC = mul(float4(vin.TexC, 0.0, 1.0), gTexTransform);
	vout.TexC = mul(texC, gMatTransform).xy;

	return vout;
}

/// 像素着色器.
float4 PS (VertexOut pin) : SV_Target
{
	// 纹理采样.
	float4 diffuseAlbedo = gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC) 
		* gDiffuseMapAlpha.Sample(gsamAnisotropicWrap, pin.TexC)
		* gDiffuseAlbedo;

	// 获取环境光.
	float4 ambient = gAmbientLight * diffuseAlbedo;

	// 关照计算必需.
	pin.NormalW = normalize(pin.NormalW);
	float3 toEyeW = normalize(gEyePosW - pin.PosW.xyz);

	const float gShininess = 1 - gRoughness;
	Material mat = { diffuseAlbedo, gFresnelR0, gShininess };
	float4 dirLight = ComputeLighting(gLights, mat, pin.PosW.xyz, pin.NormalW, toEyeW, 1.0);

	float4 finalColor = ambient + dirLight;
	finalColor.a = gDiffuseAlbedo.a;

	return finalColor;
}
