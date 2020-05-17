#pragma once
#include "..\..\Common\d3dApp.h"
#include "..\..\Common\GeometryGenerator.h"
#include "FrameResources.h"

using namespace DirectX;
using namespace DirectX::PackedVector;
using Microsoft::WRL::ComPtr;

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

	// �������, ���������˶�.
	XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();

	// ��������һ��������.
	MeshGeometry* Geo = nullptr;

	// ����.
	Material* Mat = nullptr;

	// �������ͼԪ���ˡ����㻺����������������.
	D3D11_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	ID3D11Buffer* VertexBuffer = nullptr;
	ID3D11Buffer* IndexBuffer = nullptr;

	// ������Ķ��㻺�������������������ܺ��ж�����������, ��Ҫƫ������Ե�ǰ������.
	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	UINT BaseVertexLocation = 0;
};

class CrateApp : public D3DApp
{
public:
	CrateApp(HINSTANCE hInstance);
	CrateApp(const CrateApp& rhs) = delete;
	CrateApp operator=(const CrateApp& rhs) = delete;
	~CrateApp();

	/// ���������鷽��.
	bool Init() override;
	void OnResize() override;
	void UpdateScene(GameTimer gt) override;
	void DrawScene() override;

	void OnMouseDown(WPARAM btnState, int x, int y) override;
	void OnMouseUp(WPARAM btnState, int x, int y) override;
	void OnMouseMove(WPARAM btnState, int x, int y) override;

	void UpdateCamera(GameTimer gt);
	void UpdatePassCB(GameTimer gt);

private:
	/// ����������Դ.
	void LoadTextures();

	/// ����������.
	void BuildSamplerStates();

	/// ���㻺����, ����������, �����������Ĺ���.
	void BuildShapesBuffers();
	void BuildConstantBuffers();

	/// ��ɫ������, ���벼�������Ĺ���.
	void BuildShadersAndInputLayout();

	/// ��Ⱦ״̬(��ʽ)����.
	void BuildRasterizerStates();

	// ���ʹ���.
	void BuildMaterials();

	/// ��Ⱦ�����ݴ洢.
	void BuildRenderItems();

	/// ��������.
	void DrawRenderItems(ID3D11DeviceContext* context, const std::vector<RenderItem*>& ritems);

private:
	// ������Դ��ͼ.
	//std::unordered_map<std::string, ComPtr<ID3D11Resource>> mTextures;
	std::unordered_map<std::string, ComPtr<ID3D11ShaderResourceView>> mTextureSrvs;
	std::vector<ComPtr<ID3D11SamplerState>> mSamplers;

	// ���㻺����, ����������, ����������.
	std::unordered_map<std::string, ComPtr<ID3D11Buffer>> mVertexBuffers;
	std::unordered_map<std::string, ComPtr<ID3D11Buffer>> mIndexBuffers;
	std::unordered_map<std::string, ComPtr<ID3D11Buffer>> mConstantBuffers;

	// ���̳�������������.
	PassConstants mMainPassCB;

	// ���벼������.
	ComPtr<ID3D11InputLayout> mInputLayout = nullptr;

	// ������ɫ��, ������ɫ��.
	ComPtr<ID3D11VertexShader> mVertexShader = nullptr;
	ComPtr<ID3D11PixelShader> mPixelShader = nullptr;

	// ������������Ϣ.
	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeos;

	// ��Ⱦ��.
	std::vector<std::unique_ptr<RenderItem>> mAllRitems;
	std::vector<RenderItem*> mOpaqueRitems;

	// ��Ⱦ״̬.
	std::unordered_map<std::string, ComPtr<ID3D11RasterizerState>> mRasterizerStates;

	// ������Ϣ.
	std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;

	// ���硢�۲졢ͶӰ����.
	XMFLOAT4X4 mWorld = MathHelper::Identity4x4();
	XMFLOAT4X4 mView = MathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();

	// ��������ϵ������.
	float mTheta = 1.5f * XM_PI;
	float mPhi = XM_PIDIV2 - 0.1f;
	float mRadius = 5.0f;

	// ���ջ�����Ϣ.
	XMFLOAT3 mEyePos = { 0.0f, 0.0f, 0.0f };
	float mSunTheta = 1.25f * XM_PI;
	float mSunPhi = XM_PIDIV4;

	// �������.
	POINT mLastMousePos;
};

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	CrateApp theApp(hInstance);

	if (!theApp.Init())
		return 0;

	return theApp.Run();
}