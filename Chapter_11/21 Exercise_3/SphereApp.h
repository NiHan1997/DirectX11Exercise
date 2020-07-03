#pragma once

#include "..\..\Common\d3dApp.h"
#include "..\..\Common\d3dUtil.h"
#include "..\..\Common\GeometryGenerator.h"
#include "..\..\Common\MathHelper.h"
#include "FrameResources.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

/// ��Ⱦ������, ÿһ����Ⱦ�������Ⱦһ��������Ҫ������.
struct RenderItem
{
	RenderItem() = default;
	RenderItem(const RenderItem& rhs) = delete;
	~RenderItem() = default;

	// ��Ⱦ����.
	int ObjectCBIndex = -1;

	// ������������.
	XMFLOAT4X4 World = MathHelper::Identity4x4();

	// ��������һ��������.
	MeshGeometry* Geo = nullptr;

	// ���������Ϣ.
	Material* Mat = nullptr;

	// �������, ������������.
	XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();

	// �������ͼԪ���ˡ����㻺����������������.
	D3D11_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	ID3D11Buffer* VertexBuffer = nullptr;
	ID3D11Buffer* IndexBuffer = nullptr;

	// ������Ķ��㻺�������������������ܺ��ж�����������, ��Ҫƫ������Ե�ǰ������.
	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	UINT BaseVertexLocation = 0;
};

/// ��Ⱦ�ֲ�, �����ʹ�ø���.
enum class RenderLayer : int
{
	Opaque = 0,
	Count
};

class SphereApp : public D3DApp
{
public:
	SphereApp(HINSTANCE hInstance);
	SphereApp(const SphereApp& rhs) = delete;
	SphereApp operator=(const SphereApp& rhs) = delete;
	~SphereApp();

	/// ���������鷽��.
	bool Init() override;
	void OnResize() override;
	void UpdateScene(GameTimer gt) override;
	void DrawScene() override;

	void OnMouseDown(WPARAM btnState, int x, int y) override;
	void OnMouseUp(WPARAM btnState, int x, int y) override;
	void OnMouseMove(WPARAM btnState, int x, int y) override;

	/// �������������.
	void UpdateCamera(GameTimer gt);

	/// ���¹��̳���������.
	void UpdatePassCB(GameTimer gt);

	/// ���������˶�.
	void AnimateMaterials(GameTimer gt);

private:
	/// ����������Դ.
	void LoadTextures();

	/// ����������.
	void BuildSamplerStates();

	/// ���㻺����, ����������, �����������Ĺ���.
	void BuildSphereBuffers();
	void BuildConstantBuffers();

	/// ��ɫ������, ���벼�������Ĺ���.
	void BuildShadersAndInputLayout();

	/// ��Ⱦ״̬(��ʽ)����.
	void BuildRasterizerStates();

	/// ���ʴ���.
	void BuildMaterials();

	/// ��Ⱦ�����ݴ洢.
	void BuildRenderItems();

	/// ��������.
	void DrawRenderItems(ID3D11DeviceContext* context, const std::vector<RenderItem*>& ritems);

private:
	// ������Դ��ͼ.
	std::vector<ComPtr<ID3D11ShaderResourceView>> mTextureSrvs;
	std::vector<ComPtr<ID3D11SamplerState>> mSamplers;

	// ���㻺����, ����������, ����������.
	std::unordered_map<std::string, ComPtr<ID3D11Buffer>> mVertexBuffers;
	std::unordered_map<std::string, ComPtr<ID3D11Buffer>> mIndexBuffers;
	std::unordered_map<std::string, ComPtr<ID3D11Buffer>> mConstantBuffers;

	// ��Ⱦ��������.
	PassConstants mMainPassCB;

	// ���벼������.
	ComPtr<ID3D11InputLayout> mInputLayout = nullptr;
	ComPtr<ID3D11InputLayout> mTreeSpriteInputLayout = nullptr;

	// ������ɫ��, ������ɫ��.
	std::unordered_map<std::string, ComPtr<ID3D11VertexShader>> mVertexShaders;
	std::unordered_map<std::string, ComPtr<ID3D11PixelShader>> mPixelShaders;
	std::unordered_map<std::string, ComPtr<ID3D11GeometryShader>> mGeometryShaders;

	// ������������Ϣ.
	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeos;

	// ����������Ϣ.
	std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;

	// ��Ⱦ��.
	std::vector<std::unique_ptr<RenderItem>> mAllRitems;
	std::vector<RenderItem*> mRitemLayer[(int)RenderLayer::Count];

	// ��Ⱦ״̬.
	std::unordered_map<std::string, ComPtr<ID3D11RasterizerState>> mRasterizerStates;

	// ���硢�۲졢ͶӰ����.
	XMFLOAT3 mEyePos = { 0.0f, 0.0f, 0.0f };
	XMFLOAT4X4 mWorld = MathHelper::Identity4x4();
	XMFLOAT4X4 mView = MathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();

	// ��������ϵ������.
	float mTheta = 1.5f * XM_PI;
	float mPhi = XM_PIDIV2 - 0.1f;
	float mRadius = 20.0f;

	// �������.
	POINT mLastMousePos;
};

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	SphereApp theApp(hInstance);

	if (!theApp.Init())
		return 0;

	return theApp.Run();
}