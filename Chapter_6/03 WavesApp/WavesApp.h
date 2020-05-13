#pragma once

#include "..\..\Common\d3dApp.h"
#include "..\..\Common\d3dUtil.h"
#include "..\..\Common\GeometryGenerator.h"
#include "..\..\Common\MathHelper.h"
#include "Waves.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

/// 顶点数据.
struct Vertex
{
	XMFLOAT3 Position;
	XMFLOAT4 Color;
};

/// 常量缓冲区数据.
struct ObjectConstants
{
	XMFLOAT4X4 WorldViewProj;
};

/// 渲染项数据, 每一个渲染项代表渲染一个物体需要的数据.
struct RenderItem
{
	RenderItem() = default;
	RenderItem(const RenderItem& rhs) = delete;
	~RenderItem() = default;

	// 渲染项编号.
	int ObjectCBIndex = -1;

	// 物体的世界矩阵.
	XMFLOAT4X4 World = MathHelper::Identity4x4();

	// 物体是哪一个几何体.
	MeshGeometry* Geo = nullptr;

	// 几何体的图元拓扑、顶点缓冲区、索引缓冲区.
	D3D11_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	ID3D11Buffer* VertexBuffer = nullptr;
	ID3D11Buffer* IndexBuffer = nullptr;

	// 几何体的顶点缓冲区、索引缓冲区可能含有多个物体的数据, 需要偏移量针对当前几何体.
	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	UINT BaseVertexLocation = 0;
};

/// 渲染分层, 后序会使用更多.
enum class RenderLayer : int
{
	Opaque = 0,
	Count
};

class WavesApp : public D3DApp
{
public:
	WavesApp(HINSTANCE hInstance);
	WavesApp(const WavesApp& rhs) = delete;
	WavesApp operator=(const WavesApp& rhs) = delete;
	~WavesApp();

	/// 父类的相关虚方法.
	bool Init() override;
	void OnResize() override;
	void UpdateScene(float dt) override;
	void DrawScene() override;

	void OnMouseDown(WPARAM btnState, int x, int y) override;
	void OnMouseUp(WPARAM btnState, int x, int y) override;
	void OnMouseMove(WPARAM btnState, int x, int y) override;

	/// 更新摄像机数据.
	void UpdateCamera(float dt);

	// 更新波浪顶点.
	void UpdateWaves(float dt);

private:
	/// 顶点缓冲区, 索引缓冲区, 常量缓冲区的构建.
	void BuildHillBuffers();
	void BuildWavesBuffers();
	void BuildConstantBuffers();

	/// 着色器编译, 输入布局描述的构建.
	void BuildShadersAndInputLayout();

	/// 渲染状态(方式)构建.
	void BuildRasterizerStates();

	/// 渲染项数据存储.
	void BuildRenderItems();

	/// 绘制物体.
	void DrawRenderItems(ID3D11DeviceContext* context, const std::vector<RenderItem*>& ritems);

	/// 计算山峰高度.
	float GetHillsHeight(float x, float z) const;

	/// 计算山峰某处的法线.
	XMFLOAT3 GetHillsNormal(float x, float z) const;

private:
	// 顶点缓冲区, 索引缓冲区, 常量缓冲区.
	std::unordered_map<std::string, ComPtr<ID3D11Buffer>> mVertexBuffers;
	std::unordered_map<std::string, ComPtr<ID3D11Buffer>> mIndexBuffers;
	ComPtr<ID3D11Buffer> mConstantBuffer = nullptr;

	// 输入布局描述.
	ComPtr<ID3D11InputLayout> mInputLayout = nullptr;

	// 顶点着色器, 像素着色器.
	ComPtr<ID3D11VertexShader> mVertexShader = nullptr;
	ComPtr<ID3D11PixelShader> mPixelShader = nullptr;

	// 波浪数据.
	std::unique_ptr<Waves> mWaves = nullptr;

	// 几何体数据信息.
	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeos;

	// 渲染项.
	std::vector<std::unique_ptr<RenderItem>> mAllRitems;
	std::vector<RenderItem*> mRitemLayer[(int)RenderLayer::Count];
	RenderItem* mWaveRitem = nullptr;

	// 渲染状态.
	std::unordered_map<std::string, ComPtr<ID3D11RasterizerState>> mRasterizerStates;

	// 世界、观察、投影矩阵.
	XMFLOAT4X4 mWorld = MathHelper::Identity4x4();
	XMFLOAT4X4 mView = MathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();

	// 球面坐标系的数据.
	float mTheta = 1.5f * XM_PI;
	float mPhi = XM_PIDIV4;
	float mRadius = 15.0f;

	// 鼠标数据.
	POINT mLastMousePos;
};

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	WavesApp theApp(hInstance);

	if (!theApp.Init())
		return 0;

	return theApp.Run();
}