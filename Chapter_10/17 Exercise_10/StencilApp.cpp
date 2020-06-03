#include "StencilApp.h"

StencilApp::StencilApp(HINSTANCE hInstance) : D3DApp(hInstance)
{
}

StencilApp::~StencilApp()
{
}

bool StencilApp::Init()
{
	if (D3DApp::Init() == false)
		return false;

	LoadTextures();
	BuildSamplerStates();
	BuildRoomBuffers();
	BuildSkullBuffers();
	BuildShadersAndInputLayout();
	BuildMaterials();
	BuildRenderItems();
	BuildConstantBuffers();
	BuildRasterizerStates();

	return true;
}

void StencilApp::OnResize()
{
	D3DApp::OnResize();

	XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, proj);
}

void StencilApp::UpdateScene(GameTimer gt)
{
	OnKeyboardInput(gt);
	AnimateMaterials(gt);
	UpdateCamera(gt);
	UpdateMainPassCB(gt);
	UpdateReflectedPassCB(gt);
}

void StencilApp::DrawScene()
{
	md3dImmediateContext->ClearRenderTargetView(CurrentBackBufferView().Get(), Colors::LightSteelBlue);
	md3dImmediateContext->ClearDepthStencilView(mDepthStencilView.Get(),
		D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// 交换链, 因此需要指定新的渲染目标.
	md3dImmediateContext->OMSetRenderTargets(1, CurrentBackBufferView().GetAddressOf(), mDepthStencilView.Get());

	// 绘制需要的输入布局描述和图元拓扑.
	md3dImmediateContext->IASetInputLayout(mInputLayout.Get());

	/// <summary>
	/// 绘制不透明物体.
	/// </summary>
	md3dImmediateContext->RSSetState(nullptr);

	md3dImmediateContext->VSSetShader(mVertexShaders["standardVS"].Get(), nullptr, 0);
	md3dImmediateContext->PSSetShader(mPixelShaders["opaquePS"].Get(), nullptr, 0);

	md3dImmediateContext->VSSetConstantBuffers(2, 1, mConstantBuffers["mainPass"].GetAddressOf());
	md3dImmediateContext->PSSetConstantBuffers(2, 1, mConstantBuffers["mainPass"].GetAddressOf());

	DrawRenderItems(md3dImmediateContext.Get(), mRitemLayer[(int)RenderLayer::Opaque]);

	/// <summary>
	/// 标记镜面区域为模板区域.
	/// </summary>
	md3dImmediateContext->OMSetDepthStencilState(mDepthStecnilStates["markMirror"].Get(), 1);
	DrawRenderItems(md3dImmediateContext.Get(), mRitemLayer[(int)RenderLayer::Mirrors]);

	/// <summary>
	/// 绘制反射骷髅和地面.
	/// </summary>
	md3dImmediateContext->RSSetState(mRasterizerStates["reflection"].Get());
	md3dImmediateContext->OMSetDepthStencilState(mDepthStecnilStates["reflection"].Get(), 1);

	md3dImmediateContext->VSSetConstantBuffers(2, 1, mConstantBuffers["reflectPass"].GetAddressOf());
	md3dImmediateContext->PSSetConstantBuffers(2, 1, mConstantBuffers["reflectPass"].GetAddressOf());

	DrawRenderItems(md3dImmediateContext.Get(), mRitemLayer[(int)RenderLayer::Reflect]);

	/// <summary>
	/// 绘制反射阴影.
	/// </summary>
	md3dImmediateContext->OMSetDepthStencilState(mDepthStecnilStates["shadow"].Get(), 1);
	DrawRenderItems(md3dImmediateContext.Get(), mRitemLayer[(int)RenderLayer::ReflectShadow]);

	/// <summary>
	/// 绘制镜面.
	/// </summary>
	md3dImmediateContext->VSSetConstantBuffers(2, 1, mConstantBuffers["mainPass"].GetAddressOf());
	md3dImmediateContext->PSSetConstantBuffers(2, 1, mConstantBuffers["mainPass"].GetAddressOf());

	md3dImmediateContext->RSSetState(nullptr);
	md3dImmediateContext->OMSetDepthStencilState(nullptr, 0);
	const float blendFactor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	md3dImmediateContext->OMSetBlendState(mBlendStates["transparent"].Get(), blendFactor, 0xffffffff);
	DrawRenderItems(md3dImmediateContext.Get(), mRitemLayer[(int)RenderLayer::Transparent]);

	/// <summary>
	/// 绘制阴影.
	/// </summary>
	md3dImmediateContext->OMSetDepthStencilState(mDepthStecnilStates["shadow"].Get(), 0);
	DrawRenderItems(md3dImmediateContext.Get(), mRitemLayer[(int)RenderLayer::Shadow]);

	md3dImmediateContext->OMSetDepthStencilState(nullptr, 0);

	HR(mSwapChain->Present(0, 0));

	// 交换链.
	mCurrentBuffer = (mCurrentBuffer + 1) % FlipBufferCount;
}

void StencilApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void StencilApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void StencilApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0)
	{
		float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

		mTheta -= dx;
		mPhi -= dy;

		mPhi = MathHelper::Clamp(mPhi, 0.1f, XM_PI - 0.1f);
	}
	else if ((btnState & MK_RBUTTON) != 0)
	{
		float dx = 0.05f * static_cast<float>(x - mLastMousePos.x);
		float dy = 0.05f * static_cast<float>(y - mLastMousePos.y);

		mRadius += dx - dy;

		mRadius = MathHelper::Clamp(mRadius, 15.0f, 150.0f);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void StencilApp::OnKeyboardInput(const GameTimer& gt)
{
	const float dt = gt.DeltaTime();

	if (GetAsyncKeyState('A') & 0x8000)
		mSkullTranslation.x -= dt;
	if (GetAsyncKeyState('D') & 0x8000)
		mSkullTranslation.x += dt;
	if (GetAsyncKeyState('W') & 0x8000)
		mSkullTranslation.y += dt;
	if (GetAsyncKeyState('S') & 0x8000)
		mSkullTranslation.y -= dt;

	mSkullTranslation.y = MathHelper::Max(mSkullTranslation.y, 0.0f);

	// 更新骷髅的世界矩阵.
	XMMATRIX skullScale = XMMatrixScaling(0.45f, 0.45f, 0.45f);
	XMMATRIX skullRotation = XMMatrixRotationY(XM_PIDIV2);
	XMMATRIX skullOffset = XMMatrixTranslation(mSkullTranslation.x, mSkullTranslation.y, mSkullTranslation.z);
	XMMATRIX skullWorld = skullScale * skullRotation * skullOffset;
	XMStoreFloat4x4(&mSkullRitem->World, skullWorld);

	// 更新镜像骷髅的世界矩阵.
	XMVECTOR mirrorPlane = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);			// 墙面.
	XMMATRIX reflection = XMMatrixReflect(mirrorPlane);
	XMStoreFloat4x4(&mReflectedSkullRitem->World, skullWorld * reflection);

	// 更新骷髅阴影的世界矩阵.
	XMVECTOR shadowPlane = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);			// 地面.
	XMVECTOR toMainLight = -XMLoadFloat3(&mMainPassCB.Lights[0].Direction);
	XMMATRIX shadow = XMMatrixShadow(shadowPlane, toMainLight);
	XMMATRIX shadowOffset = XMMatrixTranslation(0.0f, 0.01f, 0.0f);
	XMStoreFloat4x4(&mShadowedSkullRitem->World, skullWorld * shadow * shadowOffset);

	// 更新反射阴影世界矩阵.
	XMStoreFloat4x4(&mReflectedShadowedSkullRitem->World, skullWorld * shadow * shadowOffset * reflection);
}

void StencilApp::UpdateCamera(GameTimer gt)
{
	float x = mRadius * sinf(mPhi) * cosf(mTheta);
	float z = mRadius * sinf(mPhi) * sinf(mTheta);
	float y = mRadius * cosf(mPhi);

	XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);

	XMStoreFloat3(&mEyePos, pos);
	XMStoreFloat4x4(&mView, view);
}

void StencilApp::UpdateMainPassCB(GameTimer gt)
{
	XMMATRIX view = XMLoadFloat4x4(&mView);
	XMMATRIX proj = XMLoadFloat4x4(&mProj);
	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));

	mMainPassCB.EyePosW = mEyePos;
	mMainPassCB.RenderTargetSize = XMFLOAT2((float)mClientWidth, (float)mClientHeight);
	mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
	mMainPassCB.NearZ = 1.0f;
	mMainPassCB.FarZ = 1000.0f;
	mMainPassCB.TotalTime = gt.TotalTime();
	mMainPassCB.DeltaTime = gt.DeltaTime();

	mMainPassCB.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };
	mMainPassCB.Lights[0].Direction = { 0.57735f, -0.57735f, 0.57735f };
	mMainPassCB.Lights[0].Strength = { 0.9f, 0.9f, 0.9f };
	mMainPassCB.Lights[1].Direction = { -0.57735f, -0.57735f, 0.57735f };
	mMainPassCB.Lights[1].Strength = { 0.5f, 0.5f, 0.5f };
	mMainPassCB.Lights[2].Direction = { 0.0f, -0.707f, -0.707f };
	mMainPassCB.Lights[2].Strength = { 0.2f, 0.2f, 0.2f };

	d3dUtil::CopyDataToGpu(md3dImmediateContext.Get(), &mMainPassCB,
		sizeof(PassConstants), mConstantBuffers["mainPass"].Get());
}

void StencilApp::UpdateReflectedPassCB(const GameTimer& gt)
{
	mReflectPassCB = mMainPassCB;

	// 镜像中光照方向需要反转.
	XMVECTOR mirrorPlane = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	XMMATRIX reflection = XMMatrixReflect(mirrorPlane);

	for (int i = 0; i < 3; ++i)
	{
		XMVECTOR lightDir = XMLoadFloat3(&mMainPassCB.Lights[i].Direction);
		lightDir = XMVector3TransformNormal(lightDir, reflection);
		XMStoreFloat3(&mReflectPassCB.Lights[i].Direction, lightDir);
	}

	d3dUtil::CopyDataToGpu(md3dImmediateContext.Get(), &mReflectPassCB,
		sizeof(PassConstants), mConstantBuffers["reflectPass"].Get());
}

void StencilApp::AnimateMaterials(GameTimer gt)
{
}

void StencilApp::LoadTextures()
{
	const std::vector<std::wstring> texPathList =
	{
		L"../../Textures/bricks3.dds",
		L"../../Textures/checkboard.dds",
		L"../../Textures/ice.dds",
		L"../../Textures/white1x1.dds"
	};

	mTextureSrvs.resize(texPathList.size());

	for (UINT i = 0; i < (UINT)texPathList.size(); ++i)
	{
		HR(CreateDDSTextureFromFile(md3dDevice.Get(), texPathList[i].c_str(),
			nullptr, mTextureSrvs[i].GetAddressOf()));
	}
}

void StencilApp::BuildSamplerStates()
{
	mSamplers.resize(6);

	///
	/// 在这里创建6种常用的采样器.
	///

	D3D11_SAMPLER_DESC pointWrap;
	pointWrap.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	pointWrap.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	pointWrap.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	pointWrap.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	pointWrap.ComparisonFunc = D3D11_COMPARISON_NEVER;
	pointWrap.MinLOD = 0;
	pointWrap.MaxLOD = D3D11_FLOAT32_MAX;
	pointWrap.MipLODBias = 0;
	pointWrap.MaxAnisotropy = 0;
	HR(md3dDevice->CreateSamplerState(&pointWrap, mSamplers[0].GetAddressOf()));

	D3D11_SAMPLER_DESC pointClamp;
	pointClamp.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	pointClamp.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	pointClamp.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	pointClamp.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	pointClamp.ComparisonFunc = D3D11_COMPARISON_NEVER;
	pointClamp.MinLOD = 0;
	pointClamp.MaxLOD = D3D11_FLOAT32_MAX;
	pointClamp.MipLODBias = 0;
	pointClamp.MaxAnisotropy = 0;
	HR(md3dDevice->CreateSamplerState(&pointClamp, mSamplers[1].GetAddressOf()));

	D3D11_SAMPLER_DESC linearWrap;
	linearWrap.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	linearWrap.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	linearWrap.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	linearWrap.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	linearWrap.ComparisonFunc = D3D11_COMPARISON_NEVER;
	linearWrap.MinLOD = 0;
	linearWrap.MaxLOD = D3D11_FLOAT32_MAX;
	linearWrap.MipLODBias = 0;
	linearWrap.MaxAnisotropy = 0;
	HR(md3dDevice->CreateSamplerState(&linearWrap, mSamplers[2].GetAddressOf()));

	D3D11_SAMPLER_DESC linearClamp;
	linearClamp.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	linearClamp.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	linearClamp.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	linearClamp.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	linearClamp.ComparisonFunc = D3D11_COMPARISON_NEVER;
	linearClamp.MinLOD = 0;
	linearClamp.MaxLOD = D3D11_FLOAT32_MAX;
	linearClamp.MipLODBias = 0;
	linearClamp.MaxAnisotropy = 0;
	HR(md3dDevice->CreateSamplerState(&linearClamp, mSamplers[3].GetAddressOf()));

	D3D11_SAMPLER_DESC anistropicWrap;
	anistropicWrap.Filter = D3D11_FILTER_ANISOTROPIC;
	anistropicWrap.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	anistropicWrap.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	anistropicWrap.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	anistropicWrap.MipLODBias = 0;
	anistropicWrap.MaxAnisotropy = 8;
	HR(md3dDevice->CreateSamplerState(&anistropicWrap, mSamplers[4].GetAddressOf()));

	D3D11_SAMPLER_DESC anistropicClamp;
	anistropicClamp.Filter = D3D11_FILTER_ANISOTROPIC;
	anistropicClamp.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	anistropicClamp.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	anistropicClamp.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	anistropicClamp.MipLODBias = 0;
	anistropicClamp.MaxAnisotropy = 8;
	HR(md3dDevice->CreateSamplerState(&anistropicClamp, mSamplers[5].GetAddressOf()));

	// 设置采样器.
	md3dImmediateContext->PSSetSamplers(0, (UINT)mSamplers.size(), mSamplers.data()->GetAddressOf());
}

void StencilApp::BuildRoomBuffers()
{
	std::vector<Vertex> vertices =
	{
		// 地面顶点数据, 这里注意纹理已经缩放了.
		Vertex(-3.5f, 0.0f, -10.0f, 0.0f, 1.0f, 0.0f, 0.0f, 4.0f), // 0 
		Vertex(-3.5f, 0.0f,   0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f),
		Vertex(7.5f, 0.0f,   0.0f, 0.0f, 1.0f, 0.0f, 4.0f, 0.0f),
		Vertex(7.5f, 0.0f, -10.0f, 0.0f, 1.0f, 0.0f, 4.0f, 4.0f),

		// 墙壁顶点数据.
		Vertex(-3.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f), // 4
		Vertex(-3.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
		Vertex(-2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.5f, 0.0f),
		Vertex(-2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.5f, 2.0f),

		Vertex(2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f), // 8 
		Vertex(2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
		Vertex(7.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 2.0f, 0.0f),
		Vertex(7.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 2.0f, 2.0f),

		Vertex(-3.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f), // 12
		Vertex(-3.5f, 6.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
		Vertex(7.5f, 6.0f, 0.0f, 0.0f, 0.0f, -1.0f, 6.0f, 0.0f),
		Vertex(7.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 6.0f, 1.0f),

		// 镜面顶点数据.
		Vertex(-2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f), // 16
		Vertex(-2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
		Vertex(2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f),
		Vertex(2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f)
	};

	std::vector<UINT> indices =
	{
		// 地面.
		0, 1, 2,
		0, 2, 3,

		// 墙壁.
		4, 5, 6,
		4, 6, 7,

		8, 9, 10,
		8, 10, 11,

		12, 13, 14,
		12, 14, 15,

		// 镜子.
		16, 17, 18,
		16, 18, 19
	};

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(UINT);

	/// <summary>
	/// 创建顶点缓冲区.
	/// </summary>
	D3D11_BUFFER_DESC cbd;
	cbd.Usage = D3D11_USAGE_IMMUTABLE;
	cbd.ByteWidth = vbByteSize;
	cbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	cbd.CPUAccessFlags = 0;
	cbd.MiscFlags = 0;
	cbd.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA vInitData;
	vInitData.pSysMem = vertices.data();
	vInitData.SysMemPitch = 0;
	vInitData.SysMemSlicePitch = 0;

	HR(md3dDevice->CreateBuffer(&cbd, &vInitData, mVertexBuffers["room"].GetAddressOf()));

	/// <summary>
	/// 创建索引缓冲区.
	/// </summary>
	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = ibByteSize;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	ibd.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA iInitData;
	iInitData.pSysMem = indices.data();
	iInitData.SysMemPitch = 0;
	iInitData.SysMemSlicePitch = 0;

	HR(md3dDevice->CreateBuffer(&ibd, &iInitData, mIndexBuffers["room"].GetAddressOf()));

	/// <summary>
	/// 几何体信息.
	/// </summary>
	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "roomGeo";

	SubmeshGeometry floorSubmesh;
	floorSubmesh.IndexCount = 6;
	floorSubmesh.StartIndexLocation = 0;
	floorSubmesh.BaseVertexLocation = 0;

	SubmeshGeometry wallSubmesh;
	wallSubmesh.IndexCount = 18;
	wallSubmesh.StartIndexLocation = 6;
	wallSubmesh.BaseVertexLocation = 0;

	SubmeshGeometry mirrorSubmesh;
	mirrorSubmesh.IndexCount = 6;
	mirrorSubmesh.StartIndexLocation = 24;
	mirrorSubmesh.BaseVertexLocation = 0;

	geo->DrawArgs["floor"] = floorSubmesh;
	geo->DrawArgs["wall"] = wallSubmesh;
	geo->DrawArgs["mirror"] = mirrorSubmesh;

	mGeos[geo->Name] = std::move(geo);
}

void StencilApp::BuildSkullBuffers()
{
	std::fstream fin("Models/skull.txt");
	if (!fin)
	{
		MessageBox(0, L"Models/skull.txt not found.", 0, 0);
		return;
	}

	UINT vcount = 0;
	UINT tcount = 0;
	std::string ignore;

	fin >> ignore >> vcount;
	fin >> ignore >> tcount;
	fin >> ignore >> ignore >> ignore >> ignore;

	std::vector<Vertex> vertices(vcount);
	for (UINT i = 0; i < vcount; ++i)
	{
		fin >> vertices[i].Position.x >> vertices[i].Position.y >> vertices[i].Position.z;
		fin >> vertices[i].Normal.x >> vertices[i].Normal.y >> vertices[i].Normal.z;

		// 骷髅不考虑纹理坐标.
		vertices[i].TexC = { 0.0f, 0.0f };
	}

	fin >> ignore;
	fin >> ignore;
	fin >> ignore;

	std::vector<UINT> indices(3 * tcount);
	for (UINT i = 0; i < tcount; ++i)
	{
		fin >> indices[i * 3 + 0] >> indices[i * 3 + 1] >> indices[i * 3 + 2];
	}

	fin.close();

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(UINT);

	/// <summary>
	/// 创建顶点缓冲区.
	/// </summary>
	D3D11_BUFFER_DESC cbd;
	cbd.Usage = D3D11_USAGE_IMMUTABLE;
	cbd.ByteWidth = vbByteSize;
	cbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	cbd.CPUAccessFlags = 0;
	cbd.MiscFlags = 0;
	cbd.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA vInitData;
	vInitData.pSysMem = vertices.data();
	vInitData.SysMemPitch = 0;
	vInitData.SysMemSlicePitch = 0;

	HR(md3dDevice->CreateBuffer(&cbd, &vInitData, mVertexBuffers["skull"].GetAddressOf()));

	/// <summary>
	/// 创建索引缓冲区.
	/// </summary>
	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = ibByteSize;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	ibd.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA iInitData;
	iInitData.pSysMem = indices.data();
	iInitData.SysMemPitch = 0;
	iInitData.SysMemSlicePitch = 0;

	HR(md3dDevice->CreateBuffer(&ibd, &iInitData, mIndexBuffers["skull"].GetAddressOf()));

	/// <summary>
	/// 几何体信息.
	/// </summary>
	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "skullGeo";

	SubmeshGeometry skullSubmesh;
	skullSubmesh.IndexCount = (UINT)indices.size();
	skullSubmesh.StartIndexLocation = 0;
	skullSubmesh.BaseVertexLocation = 0;

	geo->DrawArgs["skull"] = skullSubmesh;

	mGeos[geo->Name] = std::move(geo);
}

void StencilApp::BuildConstantBuffers()
{
	/// 物体常量缓冲区.
	D3D11_BUFFER_DESC objCbd;
	objCbd.Usage = D3D11_USAGE_DYNAMIC;
	objCbd.ByteWidth = sizeof(ObjectConstants);
	objCbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	objCbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	objCbd.MiscFlags = 0;
	objCbd.StructureByteStride = 0;

	HR(md3dDevice->CreateBuffer(&objCbd, nullptr, mConstantBuffers["object"].GetAddressOf()));

	/// 材质常量缓冲区.
	D3D11_BUFFER_DESC matCbd;
	matCbd.Usage = D3D11_USAGE_DYNAMIC;
	matCbd.ByteWidth = sizeof(MaterialConstants);
	matCbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	matCbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	matCbd.MiscFlags = 0;
	matCbd.StructureByteStride = 0;

	HR(md3dDevice->CreateBuffer(&matCbd, nullptr, mConstantBuffers["material"].GetAddressOf()));

	/// 每一帧的常量数据.
	D3D11_BUFFER_DESC passCbd;
	passCbd.Usage = D3D11_USAGE_DYNAMIC;
	passCbd.ByteWidth = sizeof(PassConstants);
	passCbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	passCbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	passCbd.MiscFlags = 0;
	passCbd.StructureByteStride = 0;

	HR(md3dDevice->CreateBuffer(&passCbd, nullptr, mConstantBuffers["mainPass"].GetAddressOf()));
	HR(md3dDevice->CreateBuffer(&passCbd, nullptr, mConstantBuffers["reflectPass"].GetAddressOf()));
}

void StencilApp::BuildShadersAndInputLayout()
{
	ComPtr<ID3DBlob> vertexBlob = d3dUtil::CompileShader(L"Shaders/Default.hlsl", nullptr, "VS", "vs_5_0");
	ComPtr<ID3DBlob> pixelBlob = d3dUtil::CompileShader(L"Shaders/Default.hlsl", nullptr, "PS", "ps_5_0");

	HR(md3dDevice->CreateVertexShader(vertexBlob->GetBufferPointer(), vertexBlob->GetBufferSize(),
		nullptr, mVertexShaders["standardVS"].GetAddressOf()));
	HR(md3dDevice->CreatePixelShader(pixelBlob->GetBufferPointer(), pixelBlob->GetBufferSize(),
		nullptr, mPixelShaders["opaquePS"].GetAddressOf()));

	D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
		D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
		D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24,
		D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	HR(md3dDevice->CreateInputLayout(vertexDesc, ARRAYSIZE(vertexDesc),
		vertexBlob->GetBufferPointer(), vertexBlob->GetBufferSize(),
		mInputLayout.GetAddressOf()));
}

void StencilApp::BuildRasterizerStates()
{
	/// <summary>
	/// 透明渲染.
	/// </summary>
	D3D11_BLEND_DESC transparentDesc;
	transparentDesc.AlphaToCoverageEnable = FALSE;
	transparentDesc.IndependentBlendEnable = FALSE;
	transparentDesc.RenderTarget[0].BlendEnable = TRUE;
	transparentDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	transparentDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	transparentDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	transparentDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	transparentDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	transparentDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	transparentDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	HR(md3dDevice->CreateBlendState(&transparentDesc, mBlendStates["transparent"].GetAddressOf()));


	/// <summary>
	/// 标记镜面模板区域的渲染.
	/// </summary>

	// 开启深度测试, 关闭深度写入, 开启模板测试.
	D3D11_DEPTH_STENCIL_DESC mirrorDDS;
	mirrorDDS.DepthEnable = TRUE;
	mirrorDDS.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	mirrorDDS.DepthFunc = D3D11_COMPARISON_LESS;
	mirrorDDS.StencilEnable = TRUE;
	mirrorDDS.StencilReadMask = 0xff;
	mirrorDDS.StencilWriteMask = 0xff;

	// 模板测试正面细节.
	mirrorDDS.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	mirrorDDS.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
	mirrorDDS.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	mirrorDDS.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;

	// 背面不关心.
	mirrorDDS.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	mirrorDDS.BackFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
	mirrorDDS.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	mirrorDDS.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;

	HR(md3dDevice->CreateDepthStencilState(&mirrorDDS, mDepthStecnilStates["markMirror"].GetAddressOf()));


	/// <summary>
	/// 反射物体的渲染.
	/// </summary>
	D3D11_DEPTH_STENCIL_DESC reflectionDDS;
	reflectionDDS.DepthEnable = TRUE;
	reflectionDDS.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	reflectionDDS.DepthFunc = D3D11_COMPARISON_LESS;
	reflectionDDS.StencilEnable = TRUE;
	reflectionDDS.StencilReadMask = 0xff;
	reflectionDDS.StencilWriteMask = 0xff;

	// 模板测试正面细节.
	reflectionDDS.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
	reflectionDDS.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	reflectionDDS.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	reflectionDDS.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;

	// 背面不关心.
	reflectionDDS.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	reflectionDDS.BackFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
	reflectionDDS.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	reflectionDDS.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;

	HR(md3dDevice->CreateDepthStencilState(&reflectionDDS, mDepthStecnilStates["reflection"].GetAddressOf()));


	/// <summary>
	/// 反射物体渲染需要绕序反转.
	/// </summary>
	D3D11_RASTERIZER_DESC reflectionRSDesc;
	ZeroMemory(&reflectionRSDesc, sizeof(D3D11_RASTERIZER_DESC));
	reflectionRSDesc.CullMode = D3D11_CULL_BACK;
	reflectionRSDesc.FillMode = D3D11_FILL_SOLID;
	reflectionRSDesc.FrontCounterClockwise = TRUE;
	reflectionRSDesc.DepthClipEnable = TRUE;

	HR(md3dDevice->CreateRasterizerState(&reflectionRSDesc, mRasterizerStates["reflection"].GetAddressOf()));


	/// <summary>
	/// 阴影渲染.
	/// </summary>
	D3D11_DEPTH_STENCIL_DESC shadowDDS;
	shadowDDS.DepthEnable = TRUE;
	shadowDDS.DepthFunc = D3D11_COMPARISON_LESS;
	shadowDDS.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	shadowDDS.StencilEnable = TRUE;
	shadowDDS.StencilReadMask = 0xff;
	shadowDDS.StencilWriteMask = 0xff;

	shadowDDS.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
	shadowDDS.FrontFace.StencilPassOp = D3D11_STENCIL_OP_INCR_SAT;
	shadowDDS.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	shadowDDS.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;

	shadowDDS.BackFace.StencilFunc = D3D11_COMPARISON_EQUAL;
	shadowDDS.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	shadowDDS.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	shadowDDS.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;

	HR(md3dDevice->CreateDepthStencilState(&shadowDDS, mDepthStecnilStates["shadow"].GetAddressOf()));
}

void StencilApp::BuildMaterials()
{
	// 墙面材质.
	auto bricks = std::make_unique<Material>();
	bricks->Name = "bricks";
	bricks->MatCBIndex = 0;
	bricks->DiffuseSrvHeapIndex = 0;
	bricks->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	bricks->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	bricks->Roughness = 0.25f;

	// 地面材质.
	auto checkertile = std::make_unique<Material>();
	checkertile->Name = "checkertile";
	checkertile->MatCBIndex = 1;
	checkertile->DiffuseSrvHeapIndex = 1;
	checkertile->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	checkertile->FresnelR0 = XMFLOAT3(0.07f, 0.07f, 0.07f);
	checkertile->Roughness = 0.3f;

	// 镜面纹理材质.
	auto icemirror = std::make_unique<Material>();
	icemirror->Name = "icemirror";
	icemirror->MatCBIndex = 2;
	icemirror->DiffuseSrvHeapIndex = 2;
	icemirror->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.3f);
	icemirror->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	icemirror->Roughness = 0.5f;

	// 骷髅材质.
	auto skullMat = std::make_unique<Material>();
	skullMat->Name = "skullMat";
	skullMat->MatCBIndex = 3;
	skullMat->DiffuseSrvHeapIndex = 3;
	skullMat->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	skullMat->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	skullMat->Roughness = 0.3f;

	// 阴影材质.
	auto shadowMat = std::make_unique<Material>();
	shadowMat->Name = "shadowMat";
	shadowMat->MatCBIndex = 4;
	shadowMat->DiffuseSrvHeapIndex = 3;
	shadowMat->DiffuseAlbedo = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.5f);
	shadowMat->FresnelR0 = XMFLOAT3(0.001f, 0.001f, 0.001f);
	shadowMat->Roughness = 0.0f;

	mMaterials["bricks"] = std::move(bricks);
	mMaterials["checkertile"] = std::move(checkertile);
	mMaterials["icemirror"] = std::move(icemirror);
	mMaterials["skullMat"] = std::move(skullMat);
	mMaterials["shadowMat"] = std::move(shadowMat);
}

void StencilApp::BuildRenderItems()
{
	// 地面渲染项.
	auto floorRitem = std::make_unique<RenderItem>();
	floorRitem->World = MathHelper::Identity4x4();
	floorRitem->ObjectCBIndex = 0;
	floorRitem->Mat = mMaterials["checkertile"].get();
	floorRitem->Geo = mGeos["roomGeo"].get();
	floorRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	floorRitem->IndexCount = floorRitem->Geo->DrawArgs["floor"].IndexCount;
	floorRitem->StartIndexLocation = floorRitem->Geo->DrawArgs["floor"].StartIndexLocation;
	floorRitem->BaseVertexLocation = floorRitem->Geo->DrawArgs["floor"].BaseVertexLocation;
	floorRitem->VertexBuffer = mVertexBuffers["room"].Get();
	floorRitem->IndexBuffer = mIndexBuffers["room"].Get();
	mRitemLayer[(int)RenderLayer::Opaque].push_back(floorRitem.get());

	// 墙壁渲染项.
	auto wallRitem = std::make_unique<RenderItem>();
	wallRitem->World = MathHelper::Identity4x4();
	wallRitem->ObjectCBIndex = 1;
	wallRitem->Mat = mMaterials["bricks"].get();
	wallRitem->Geo = mGeos["roomGeo"].get();
	wallRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	wallRitem->IndexCount = wallRitem->Geo->DrawArgs["wall"].IndexCount;
	wallRitem->StartIndexLocation = wallRitem->Geo->DrawArgs["wall"].StartIndexLocation;
	wallRitem->BaseVertexLocation = wallRitem->Geo->DrawArgs["wall"].BaseVertexLocation;
	wallRitem->VertexBuffer = mVertexBuffers["room"].Get();
	wallRitem->IndexBuffer = mIndexBuffers["room"].Get();
	mRitemLayer[(int)RenderLayer::Opaque].push_back(wallRitem.get());

	// 骷髅渲染项.
	auto skullRitem = std::make_unique<RenderItem>();
	skullRitem->World = MathHelper::Identity4x4();
	skullRitem->TexTransform = MathHelper::Identity4x4();
	skullRitem->ObjectCBIndex = 2;
	skullRitem->Mat = mMaterials["skullMat"].get();
	skullRitem->Geo = mGeos["skullGeo"].get();
	skullRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	skullRitem->IndexCount = skullRitem->Geo->DrawArgs["skull"].IndexCount;
	skullRitem->StartIndexLocation = skullRitem->Geo->DrawArgs["skull"].StartIndexLocation;
	skullRitem->BaseVertexLocation = skullRitem->Geo->DrawArgs["skull"].BaseVertexLocation;
	skullRitem->VertexBuffer = mVertexBuffers["skull"].Get();
	skullRitem->IndexBuffer = mIndexBuffers["skull"].Get();
	mSkullRitem = skullRitem.get();
	mRitemLayer[(int)RenderLayer::Opaque].push_back(skullRitem.get());

	// 反射地面渲染项.
	auto reflectedFloorRitem = std::make_unique<RenderItem>();
	*reflectedFloorRitem = *floorRitem;
	XMVECTOR mirrorPlane = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0);
	XMStoreFloat4x4(&reflectedFloorRitem->World, XMLoadFloat4x4(&floorRitem->World) * XMMatrixReflect(mirrorPlane));;
	reflectedFloorRitem->ObjectCBIndex = 3;
	mReflectedFloorRitem = reflectedFloorRitem.get();
	mRitemLayer[(int)RenderLayer::Reflect].push_back(reflectedFloorRitem.get());

	// 反射的骷髅渲染项.
	auto reflectedSkullRitem = std::make_unique<RenderItem>();
	*reflectedSkullRitem = *skullRitem;
	reflectedSkullRitem->ObjectCBIndex = 4;
	mReflectedSkullRitem = reflectedSkullRitem.get();
	mRitemLayer[(int)RenderLayer::Reflect].push_back(reflectedSkullRitem.get());

	// 阴影渲染项.
	auto shadowedSkullRitem = std::make_unique<RenderItem>();
	*shadowedSkullRitem = *skullRitem;
	shadowedSkullRitem->ObjectCBIndex = 5;
	shadowedSkullRitem->Mat = mMaterials["shadowMat"].get();
	mShadowedSkullRitem = shadowedSkullRitem.get();
	mRitemLayer[(int)RenderLayer::Shadow].push_back(shadowedSkullRitem.get());

	// 反射阴影渲染项.
	auto reflectedShadowedRitem = std::make_unique<RenderItem>();
	*reflectedShadowedRitem = *shadowedSkullRitem;
	reflectedShadowedRitem->ObjectCBIndex = 6;
	mReflectedShadowedSkullRitem = reflectedShadowedRitem.get();
	mRitemLayer[(int)RenderLayer::ReflectShadow].push_back(reflectedShadowedRitem.get());

	// 镜面渲染项.
	auto mirrorRitem = std::make_unique<RenderItem>();
	mirrorRitem->World = MathHelper::Identity4x4();
	mirrorRitem->TexTransform = MathHelper::Identity4x4();
	mirrorRitem->ObjectCBIndex = 7;
	mirrorRitem->Mat = mMaterials["icemirror"].get();
	mirrorRitem->Geo = mGeos["roomGeo"].get();
	mirrorRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	mirrorRitem->IndexCount = mirrorRitem->Geo->DrawArgs["mirror"].IndexCount;
	mirrorRitem->StartIndexLocation = mirrorRitem->Geo->DrawArgs["mirror"].StartIndexLocation;
	mirrorRitem->BaseVertexLocation = mirrorRitem->Geo->DrawArgs["mirror"].BaseVertexLocation;
	mirrorRitem->VertexBuffer = mVertexBuffers["room"].Get();
	mirrorRitem->IndexBuffer = mIndexBuffers["room"].Get();
	mRitemLayer[(int)RenderLayer::Mirrors].push_back(mirrorRitem.get());
	mRitemLayer[(int)RenderLayer::Transparent].push_back(mirrorRitem.get());

	mAllRitems.push_back(std::move(floorRitem));
	mAllRitems.push_back(std::move(wallRitem));
	mAllRitems.push_back(std::move(skullRitem));
	mAllRitems.push_back(std::move(reflectedSkullRitem));
	mAllRitems.push_back(std::move(reflectedFloorRitem));
	mAllRitems.push_back(std::move(shadowedSkullRitem));
	mAllRitems.push_back(std::move(reflectedShadowedRitem));
	mAllRitems.push_back(std::move(mirrorRitem));
}

void StencilApp::DrawRenderItems(ID3D11DeviceContext* context, const std::vector<RenderItem*>& ritems)
{
	for (size_t i = 0; i < ritems.size(); i++)
	{
		auto ri = ritems[i];

		UINT stride = sizeof(Vertex);
		UINT offset = 0;
		context->IASetVertexBuffers(0, 1, &ri->VertexBuffer, &stride, &offset);
		context->IASetIndexBuffer(ri->IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
		context->IASetPrimitiveTopology(ri->PrimitiveType);

		/// 常量缓冲区数据.
		XMMATRIX world = XMLoadFloat4x4(&ri->World);
		XMMATRIX texTransform = XMLoadFloat4x4(&ri->TexTransform);
		
		ObjectConstants objConstants;
		XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
		XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));

		d3dUtil::CopyDataToGpu(context, &objConstants,
			sizeof(ObjectConstants), mConstantBuffers["object"].Get());

		context->VSSetConstantBuffers(0, 1, mConstantBuffers["object"].GetAddressOf());

		// 材质常量缓冲区.
		MaterialConstants matConstants;
		matConstants.DiffuseAlbedo = ri->Mat->DiffuseAlbedo;
		matConstants.FresnelR0 = ri->Mat->FresnelR0;
		matConstants.Roughness = ri->Mat->Roughness;

		XMMATRIX matTransform = XMLoadFloat4x4(&ri->Mat->MatTransform);
		XMStoreFloat4x4(&matConstants.MatTransform, XMMatrixTranspose(matTransform));

		d3dUtil::CopyDataToGpu(context, &matConstants,
			sizeof(MaterialConstants), mConstantBuffers["material"].Get());

		context->VSSetConstantBuffers(1, 1, mConstantBuffers["material"].GetAddressOf());
		context->PSSetConstantBuffers(1, 1, mConstantBuffers["material"].GetAddressOf());

		// 纹理数据.
		context->PSSetShaderResources(0, 1, mTextureSrvs[ri->Mat->DiffuseSrvHeapIndex].GetAddressOf());

		// 绘制.
		context->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation,
			ri->BaseVertexLocation, 0);
	}
}
