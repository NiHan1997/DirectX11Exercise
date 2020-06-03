#include "..\..\Common\d3dApp.h"
#include "..\..\Common\d3dUtil.h"
#include "..\..\Common\GeometryGenerator.h"
#include "..\..\Common\MathHelper.h"
#include "FrameResources.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

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

	// 物体材质信息.
	Material* Mat = nullptr;

	// 纹理矩阵, 用于纹理缩放.
	XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();

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
	Mirrors,
	Reflect,
	Shadow,
	ReflectShadow,
	Transparent,
	Count
};

class StencilApp : public D3DApp
{
public:
	StencilApp(HINSTANCE hInstance);
	StencilApp(const StencilApp& rhs) = delete;
	StencilApp operator=(const StencilApp& rhs) = delete;
	~StencilApp();

	/// 父类的相关虚方法.
	bool Init() override;
	void OnResize() override;
	void UpdateScene(GameTimer gt) override;
	void DrawScene() override;

	void OnMouseDown(WPARAM btnState, int x, int y) override;
	void OnMouseUp(WPARAM btnState, int x, int y) override;
	void OnMouseMove(WPARAM btnState, int x, int y) override;
	void OnKeyboardInput(const GameTimer& gt);

	/// 更新摄像机数据.
	void UpdateCamera(GameTimer gt);

	/// 更新过程常量缓冲区.
	void UpdateMainPassCB(GameTimer gt);

	// 新增, 反射物体是镜像观察.
	void UpdateReflectedPassCB(const GameTimer& gt);

	/// 纹理运动.
	void AnimateMaterials(GameTimer gt);

private:
	/// 加载纹理资源.
	void LoadTextures();

	/// 创建采样器.
	void BuildSamplerStates();

	/// 顶点缓冲区, 索引缓冲区, 常量缓冲区的构建.
	void BuildRoomBuffers();
	void BuildSkullBuffers();
	void BuildConstantBuffers();

	/// 着色器编译, 输入布局描述的构建.
	void BuildShadersAndInputLayout();

	/// 渲染状态(方式)构建.
	void BuildRasterizerStates();

	/// 材质创建.
	void BuildMaterials();

	/// 渲染项数据存储.
	void BuildRenderItems();

	/// 绘制物体.
	void DrawRenderItems(ID3D11DeviceContext* context, const std::vector<RenderItem*>& ritems);

private:
	// 纹理资源视图.
	std::vector<ComPtr<ID3D11ShaderResourceView>> mTextureSrvs;
	std::vector<ComPtr<ID3D11SamplerState>> mSamplers;

	// 顶点缓冲区, 索引缓冲区, 常量缓冲区.
	std::unordered_map<std::string, ComPtr<ID3D11Buffer>> mVertexBuffers;
	std::unordered_map<std::string, ComPtr<ID3D11Buffer>> mIndexBuffers;
	std::unordered_map<std::string, ComPtr<ID3D11Buffer>> mConstantBuffers;

	// 渲染过程数据.
	PassConstants mMainPassCB;
	PassConstants mReflectPassCB;

	// 骷髅、地面、阴影位置信息存储.
	XMFLOAT3 mSkullTranslation = { 0.0f, 1.0f, -5.0f };
	XMFLOAT3 mShadowTranslation = { 0.0f, 0.0f, 0.0f };

	// 输入布局描述.
	ComPtr<ID3D11InputLayout> mInputLayout = nullptr;

	// 顶点着色器, 像素着色器.
	std::unordered_map<std::string, ComPtr<ID3D11VertexShader>> mVertexShaders;
	std::unordered_map<std::string, ComPtr<ID3D11PixelShader>> mPixelShaders;

	// 几何体数据信息.
	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeos;

	// 材质数据信息.
	std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;

	// 渲染项.
	std::vector<std::unique_ptr<RenderItem>> mAllRitems;
	std::vector<RenderItem*> mRitemLayer[(int)RenderLayer::Count];
	RenderItem* mSkullRitem = nullptr;
	RenderItem* mReflectedSkullRitem = nullptr;
	RenderItem* mReflectedFloorRitem = nullptr;
	RenderItem* mShadowedSkullRitem = nullptr;
	RenderItem* mReflectedShadowedSkullRitem = nullptr;

	// 渲染状态.
	std::unordered_map<std::string, ComPtr<ID3D11RasterizerState>> mRasterizerStates;
	std::unordered_map<std::string, ComPtr<ID3D11BlendState>> mBlendStates;
	std::unordered_map<std::string, ComPtr<ID3D11DepthStencilState>> mDepthStecnilStates;

	// 世界、观察、投影矩阵.
	XMFLOAT3 mEyePos = { 0.0f, 0.0f, 0.0f };
	XMFLOAT4X4 mWorld = MathHelper::Identity4x4();
	XMFLOAT4X4 mView = MathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();

	// 球面坐标系的数据.
	float mTheta = 1.5f * XM_PI;
	float mPhi = XM_PIDIV2 - 0.1f;
	float mRadius = 15.0f;

	// 鼠标数据.
	POINT mLastMousePos;
};

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	StencilApp theApp(hInstance);

	if (!theApp.Init())
		return 0;

	return theApp.Run();
}
