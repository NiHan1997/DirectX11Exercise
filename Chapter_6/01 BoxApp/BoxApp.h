#pragma once

#include "..\..\Common\d3dApp.h"
#include "..\..\Common\d3dUtil.h"

using namespace DirectX;
using namespace DirectX::PackedVector;
using Microsoft::WRL::ComPtr;

/// 顶点数据.
struct Vertex
{
	XMFLOAT3 Pos;
	XMFLOAT4 Color;
};

/// 常量缓冲区需要的数据.
struct ConstantBuffer
{
	XMFLOAT4X4 WorldViewProj;
};

class BoxApp : public D3DApp
{
public:
	BoxApp(HINSTANCE hInstance);
	BoxApp(const BoxApp& rhs) = delete;
	BoxApp operator=(const BoxApp& rhs) = delete;
	~BoxApp();

	/// 父类的相关虚方法.
	bool Init() override;
	void OnResize() override;
	void UpdateScene(float dt) override;
	void DrawScene() override;

	void OnMouseDown(WPARAM btnState, int x, int y) override;
	void OnMouseUp(WPARAM btnState, int x, int y) override;
	void OnMouseMove(WPARAM btnState, int x, int y) override;

private:
	/// 顶点缓冲区, 索引缓冲区, 常量缓冲区的构建.
	void BuildBuffers();

	/// 着色器编译, 输入布局描述的构建.
	void BuildShadersAndInputLayout();

private:
	// 顶点缓冲区, 索引缓冲区, 常量缓冲区.
	ComPtr<ID3D11Buffer> mVertexBuffer = nullptr;
	ComPtr<ID3D11Buffer> mIndexBuffer = nullptr;
	ComPtr<ID3D11Buffer> mConstantBuffer = nullptr;

	// 常量缓冲区需要的数据.
	ConstantBuffer mObjectCB;

	// 输入布局描述.
	ComPtr<ID3D11InputLayout> mInputLayout = nullptr;

	// 顶点着色器, 像素着色器.
	ComPtr<ID3D11VertexShader> mVertexShader = nullptr;
	ComPtr<ID3D11PixelShader> mPixelShader = nullptr;

	// 世界、观察、投影矩阵.
	XMFLOAT4X4 mWorld = MathHelper::Identity4x4();
	XMFLOAT4X4 mView = MathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();

	// 球面坐标系的数据.
	float mTheta = 1.5f * XM_PI;
	float mPhi = XM_PIDIV4;
	float mRadius = 5.0f;

	// 鼠标数据.
	POINT mLastMousePos;
};

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	BoxApp theApp(hInstance);

	if (!theApp.Init())
		return 0;

	return theApp.Run();
}
