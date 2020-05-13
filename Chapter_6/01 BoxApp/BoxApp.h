#pragma once

#include "..\..\Common\d3dApp.h"
#include "..\..\Common\d3dUtil.h"

using namespace DirectX;
using namespace DirectX::PackedVector;
using Microsoft::WRL::ComPtr;

/// ��������.
struct Vertex
{
	XMFLOAT3 Pos;
	XMFLOAT4 Color;
};

/// ������������Ҫ������.
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

	/// ���������鷽��.
	bool Init() override;
	void OnResize() override;
	void UpdateScene(float dt) override;
	void DrawScene() override;

	void OnMouseDown(WPARAM btnState, int x, int y) override;
	void OnMouseUp(WPARAM btnState, int x, int y) override;
	void OnMouseMove(WPARAM btnState, int x, int y) override;

private:
	/// ���㻺����, ����������, �����������Ĺ���.
	void BuildBuffers();

	/// ��ɫ������, ���벼�������Ĺ���.
	void BuildShadersAndInputLayout();

private:
	// ���㻺����, ����������, ����������.
	ComPtr<ID3D11Buffer> mVertexBuffer = nullptr;
	ComPtr<ID3D11Buffer> mIndexBuffer = nullptr;
	ComPtr<ID3D11Buffer> mConstantBuffer = nullptr;

	// ������������Ҫ������.
	ConstantBuffer mObjectCB;

	// ���벼������.
	ComPtr<ID3D11InputLayout> mInputLayout = nullptr;

	// ������ɫ��, ������ɫ��.
	ComPtr<ID3D11VertexShader> mVertexShader = nullptr;
	ComPtr<ID3D11PixelShader> mPixelShader = nullptr;

	// ���硢�۲졢ͶӰ����.
	XMFLOAT4X4 mWorld = MathHelper::Identity4x4();
	XMFLOAT4X4 mView = MathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();

	// ��������ϵ������.
	float mTheta = 1.5f * XM_PI;
	float mPhi = XM_PIDIV4;
	float mRadius = 5.0f;

	// �������.
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
