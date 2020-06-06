///***********************************************************************************
/// 树木物体的着色器.
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
Texture2DArray gDiffuseMap : register(t0);

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
	float3 PosW	 : POSITION;
	float2 SizeW : SIZE;
};

/// 几何着色器输入, 顶点着色器输出.
struct VertexOut
{
	float3 CenterW : POSITION;
	float2 SizeW   : SIZE;
};

/// 几何着色器输出, 像素着色器输入.
struct GeoOut
{
	float4 PosH    : SV_POSITION;
	float3 PosW    : POSITION;
	float3 NormalW : NORMAL;
	float2 TexC    : TEXCOORD0;
	uint PrimID    : SV_PrimitiveID;
};

/// 顶点着色器.
VertexOut VS(VertexIn vin)
{
	VertexOut vout;
	vout.CenterW = vin.PosW;
	vout.SizeW = vin.SizeW;

	return vout;
}

/// 几何着色器.
[maxvertexcount(4)]
void GS (point VertexOut gin[1], uint primID : SV_PrimitiveID, inout TriangleStream<GeoOut> triStrem)
{
	// 获取基准向量.
	float3 up = float3(0.0, 1.0, 0.0);
	float3 look = gEyePosW - gin[0].CenterW;
	look.y = 0.0;
	look = normalize(look);
	float3 right = cross(up, look);

	// 计算拓展顶点位置.
	float halfWidth = 0.5 * gin[0].SizeW.x;
	float halfHeight = 0.5 * gin[0].SizeW.y;

	float4 v[4];
	v[0] = float4(gin[0].CenterW + halfWidth * right + halfHeight * up, 1.0); 
	v[1] = float4(gin[0].CenterW - halfWidth * right + halfHeight * up, 1.0); 
	v[2] = float4(gin[0].CenterW + halfWidth * right - halfHeight * up, 1.0); 
	v[3] = float4(gin[0].CenterW - halfWidth * right - halfHeight * up, 1.0);

	// 索引对应.
	float2 texC[4] = 
	{
		float2(0.0, 0.0),
		float2(1.0, 0.0),
		float2(0.0, 1.0),
		float2(1.0, 1.0)
	};

	// 输出.
	GeoOut geoOut;
	[unroll]
	for (int i = 0; i < 4; ++i)
	{
		geoOut.PosH = mul(v[i], gViewProj);
		geoOut.PosW = v[i].xyz;
		geoOut.NormalW = look;
		geoOut.TexC = texC[i];
		geoOut.PrimID = primID;

		triStrem.Append(geoOut);		
	}
}

/// 像素着色器.
float4 PS(GeoOut pin) : SV_Target
{
	// 纹理采样.
	float3 uvw = float3(pin.TexC, pin.PrimID % 3);
	float4 diffuseAlbedo = gDiffuseMap.Sample(gsamAnisotropicWrap, uvw) * gDiffuseAlbedo;

#ifdef ALPHA_TEST
	clip(diffuseAlbedo.a - 0.1);
#endif	

	// 获取环境光.
	float4 ambient = gAmbientLight * diffuseAlbedo;

	// 光照计算必需.
	pin.NormalW = normalize(pin.NormalW);
	float3 toEyeW = gEyePosW - pin.PosW.xyz;
	float distToEye = length(toEyeW);
	toEyeW /= distToEye;

	// 光照计算.
	const float gShininess = 1 - gRoughness;
	Material mat = { diffuseAlbedo, gFresnelR0, gShininess };
	float4 dirLight = ComputeLighting(gLights, mat, pin.PosW.xyz, pin.NormalW, toEyeW, 1.0);

	float4 finalColor = ambient + dirLight;

#ifdef FOG
	float fogAmount = saturate((distToEye - gFogStart) / gFogRange);
	finalColor = lerp(finalColor, gFogColor, fogAmount);
#endif

	finalColor.a = diffuseAlbedo.a;

	return finalColor;
}