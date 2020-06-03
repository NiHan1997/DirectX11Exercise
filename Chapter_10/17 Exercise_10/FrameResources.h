#pragma once

#include "..\..\Common\d3dUtil.h"
#include "..\..\Common\MathHelper.h"

using namespace DirectX;

/// 顶点数据.
struct Vertex
{
	Vertex() = default;
	Vertex(float x, float y, float z, float nx, float ny, float nz, float u, float v) :
		Position(x, y, z), Normal(nx, ny, nz), TexC(u, v)
	{
	}

	XMFLOAT3 Position;
	XMFLOAT3 Normal;
	XMFLOAT2 TexC;
};

/// 每个物体的常量缓冲区.
struct ObjectConstants
{
	XMFLOAT4X4 World = MathHelper::Identity4x4();
	XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();
};

/// 过程常量缓冲区.
struct PassConstants
{
	XMFLOAT4X4 View = MathHelper::Identity4x4();
	XMFLOAT4X4 InvView = MathHelper::Identity4x4();
	XMFLOAT4X4 Proj = MathHelper::Identity4x4();
	XMFLOAT4X4 InvProj = MathHelper::Identity4x4();
	XMFLOAT4X4 ViewProj = MathHelper::Identity4x4();
	XMFLOAT4X4 InvViewProj = MathHelper::Identity4x4();

	XMFLOAT3 EyePosW = { 0.0f, 0.0f, 0.0f };
	float cbPerObjectPad1 = 0.0f;
	XMFLOAT2 RenderTargetSize = { 0.0f, 0.0f };
	XMFLOAT2 InvRenderTargetSize = { 0.0f, 0.0f };

	float NearZ = 0.0f;
	float FarZ = 0.0f;
	float TotalTime = 0.0f;
	float DeltaTime = 0.0f;

	// 环境光和其他光源.
	XMFLOAT4 AmbientLight = { 0.0f, 0.0f, 0.0f, 1.0f };
	Light Lights[MaxLights];
};
