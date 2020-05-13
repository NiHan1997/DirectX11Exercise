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
#include "LightHelper.h"
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
