#pragma once

#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
 
#include "d3dx11Effect.h"
#include "DDSTextureLoader.h"
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>
#include <d3dcompiler.h>
#include "dxerr.h"
#include <cassert>
#include <ctime>
#include <algorithm>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include "MathHelper.h"
#include <wrl.h>
#include <unordered_map>

///**************************************************************
/// 用于输出错误信息.
///**************************************************************
#if defined(DEBUG) | defined(_DEBUG)
	#ifndef HR
	#define HR(x)                                               \
	{                                                           \
		HRESULT hr = (x);                                       \
		if(FAILED(hr))                                          \
		{                                                       \
			DXTrace(__FILEW__, (DWORD)__LINE__, hr, L#x, true); \
		}                                                       \
	}
	#endif

#else
	#ifndef HR
	#define HR(x) (x)
	#endif
#endif 

///**************************************************************
/// 一些工具类.
///**************************************************************

class d3dHelper
{
public:
	static Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> CreateRandomTexture1DSRV(ID3D11Device* device);
};

class TextHelper
{
public:

	template<typename T>
	static std::wstring ToString(const T& s)
	{
		std::wostringstream oss;
		oss << s;

		return oss.str();
	}

	template<typename T>
	static T FromString(const std::wstring& s)
	{
		T x;
		std::wistringstream iss(s);
		iss >> x;

		return x;
	}
};

// Order: left, right, bottom, top, near, far.
void ExtractFrustumPlanes(DirectX::XMFLOAT4 planes[6], DirectX::CXMMATRIX T);

///**************************************************************
/// 常用的一些方法.
///**************************************************************
class d3dUtil
{
public:
	// HLSL的编译.
	static Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(
		const std::wstring& filename,
		const D3D_SHADER_MACRO* defines,
		const std::string& entrypoint,
		const std::string& target);

	// 加载HLSL的编译后的CSO文件.
	static Microsoft::WRL::ComPtr<ID3DBlob> LoadBinary(const std::wstring& filename);

	static void CopyDataToGpu(
		ID3D11DeviceContext* context,
		const void* srcCpuData, 
		rsize_t dataSize, 
		ID3D11Resource* dstGpuData);
};

/// 几何体辅助结构, 存储数据方便渲染项使用.
struct SubmeshGeometry
{
	UINT BaseVertexLocation;			// 顶点缓冲区偏移量.
	UINT StartIndexLocation;			// 索引缓冲区偏移量.
	UINT IndexCount;					// 索引的数量.
};

/// 一类几何体(顶点缓冲区、索引缓冲区合并了的物体)都放在这里, 通过无序图在渲染项中确定数据.
struct MeshGeometry
{
	std::string Name;			// 这个(类)几何体的名称.

	// 几何体(们)的顶点缓冲区、索引缓冲区.
	Microsoft::WRL::ComPtr<ID3D11Buffer> VertexBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D11Buffer> IndexBuffer = nullptr;

	// 几何体(们)的偏移量数据..
	std::unordered_map<std::string, SubmeshGeometry> DrawArgs;
};

/// 光源定义结构体.
struct Light
{
	DirectX::XMFLOAT3 Strength = { 0.5f, 0.5f, 0.5f };		// 光照强度.
	float FalloffStart = 1.0f;								// 点光源/聚光灯衰减开始.
	DirectX::XMFLOAT3 Direction = { 0.0f, -1.0f, 0.0f };	// 平行光/聚光灯方向.
	float FalloffEnd = 10.0f;								// 点光源/聚光灯衰减结束.
	DirectX::XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f };		// 点光源/聚光灯位置.
	float SpotPower = 64.0f;								// 点光源强度.
};

/// 光源的最大数量.
#define MaxLights 16

/// 材质常量缓冲区.
struct MaterialConstants
{
	DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
	float Roughness = 0.25f;

	// 纹理映射矩阵.
	DirectX::XMFLOAT4X4 MatTransform = MathHelper::Identity4x4();
};

/// 简单的材质球的定义.
struct Material
{
	std::string Name;

	int MatCBIndex = -1;

	int DiffuseSrvHeapIndex = -1;
	int NormalSrvHeapIndex = -1;

	DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
	float Roughness = 0.25f;
	DirectX::XMFLOAT4X4 MatTransform = MathHelper::Identity4x4();
};