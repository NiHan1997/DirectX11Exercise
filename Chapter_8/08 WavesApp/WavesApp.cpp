#include "WavesApp.h"

WavesApp::WavesApp(HINSTANCE hInstance) : D3DApp(hInstance)
{
}

WavesApp::~WavesApp()
{
}

bool WavesApp::Init()
{
	if (D3DApp::Init() == false)
		return false;

	mWaves = std::make_unique<Waves>(128, 128, 1.0f, 0.03f, 4.0f, 0.2f);

	LoadTextures();
	BuildSamplerStates();
	BuildHillBuffers();
	BuildWavesBuffers();
	BuildShadersAndInputLayout();
	BuildMaterials();
	BuildRenderItems();
	BuildConstantBuffers();
	BuildRasterizerStates();

	return true;
}

void WavesApp::OnResize()
{
	D3DApp::OnResize();

	XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, proj);
}

void WavesApp::UpdateScene(GameTimer gt)
{
	AnimateMaterials(gt);
	UpdateCamera(gt);
	UpdatePassCB(gt);
	UpdateWaves(gt);
}

void WavesApp::DrawScene()
{
	md3dImmediateContext->ClearRenderTargetView(CurrentBackBufferView().Get(), Colors::LightSteelBlue);
	md3dImmediateContext->ClearDepthStencilView(mDepthStencilView.Get(),
		D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// 交换链, 因此需要指定新的渲染目标.
	md3dImmediateContext->OMSetRenderTargets(1, CurrentBackBufferView().GetAddressOf(), mDepthStencilView.Get());

	// 绘制需要的输入布局描述和图元拓扑.
	md3dImmediateContext->IASetInputLayout(mInputLayout.Get());

	// 设置顶点着色器和像素着色器, 以及顶点需要的常量缓冲区.
	md3dImmediateContext->VSSetShader(mVertexShader.Get(), nullptr, 0);
	md3dImmediateContext->PSSetShader(mPixelShader.Get(), nullptr, 0);

	md3dImmediateContext->VSSetConstantBuffers(2, 1, mConstantBuffers["pass"].GetAddressOf());
	md3dImmediateContext->PSSetConstantBuffers(2, 1, mConstantBuffers["pass"].GetAddressOf());

	md3dImmediateContext->PSSetSamplers(0, mSamplers.size(), mSamplers.data()->GetAddressOf());

	md3dImmediateContext->RSSetState(nullptr);

	// 绘制.
	DrawRenderItems(md3dImmediateContext.Get(), mRitemLayer[(int)RenderLayer::Opaque]);

	// 交换链.
	mCurrentBuffer = (mCurrentBuffer + 1) % FlipBufferCount;

	HR(mSwapChain->Present(0, 0));
}

void WavesApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void WavesApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void WavesApp::OnMouseMove(WPARAM btnState, int x, int y)
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

		mRadius = MathHelper::Clamp(mRadius, 15.0f, 50.0f);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void WavesApp::UpdateCamera(GameTimer gt)
{
	float x = mRadius * sinf(mPhi) * cosf(mTheta);
	float z = mRadius * sinf(mPhi) * sinf(mTheta);
	float y = mRadius * cosf(mPhi);

	XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);

	XMStoreFloat4x4(&mView, view);
}

void WavesApp::UpdatePassCB(GameTimer gt)
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
		sizeof(PassConstants), mConstantBuffers["pass"].Get());
}

void WavesApp::UpdateWaves(GameTimer gt)
{
	static float t_base = 0.0f;
	if (mTimer.TotalTime() - t_base >= 0.25f)
	{
		t_base += 0.25f;

		int i = MathHelper::Rand(4, mWaves->RowCount() - 5);
		int j = MathHelper::Rand(4, mWaves->ColumnCount() - 5);
		float r = MathHelper::RandF(0.2f, 0.5f);

		mWaves->Disturb(i, j, r);
	}

	mWaves->Update(gt.DeltaTime());

	auto currentWavesVB = mVertexBuffers["waves"].Get();
	std::vector<Vertex> vertices(mWaves->VertexCount());
	for (int i = 0; i < mWaves->VertexCount(); ++i)
	{
		Vertex v;
		v.Position = mWaves->Position(i);
		v.Normal = mWaves->Normal(i);

		v.TexC.x = 0.5f + v.Position.x / mWaves->Width();
		v.TexC.y = 0.5f - v.Position.z / mWaves->Depth();

		vertices[i] = v;
	}

	// 将CPU的数据拷贝到GPU.
	d3dUtil::CopyDataToGpu(md3dImmediateContext.Get(), vertices.data(),
		vertices.size() * sizeof(Vertex), mVertexBuffers["waves"].Get());
}

void WavesApp::AnimateMaterials(GameTimer gt)
{
	auto waterMat = mMaterials["water"].get();

	float& tu = waterMat->MatTransform(3, 0);
	float& tv = waterMat->MatTransform(3, 1);

	tu += 0.1f * gt.DeltaTime();
	tv += 0.02f * gt.DeltaTime();

	if (tu > 1.0f) tu -= 1.0f;
	if (tv > 1.0f) tv -= 1.0f;

	waterMat->MatTransform(3, 0) = tu;
	waterMat->MatTransform(3, 1) = tv;

	mWaveRitem->Mat = waterMat;
}

void WavesApp::LoadTextures()
{
	std::vector<std::wstring> texturePath =
	{
		L"../../Textures/grass.dds",
		L"../../Textures/water1.dds"
	};

	mTextureSrvs.resize(texturePath.size());

	for (int i = 0; i < (int)texturePath.size(); ++i)
	{
		HR(CreateDDSTextureFromFile(md3dDevice.Get(), texturePath[i].c_str(),
			nullptr, mTextureSrvs[i].GetAddressOf()));
	}
}

void WavesApp::BuildSamplerStates()
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
}

void WavesApp::BuildHillBuffers()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData grid;
	geoGen.CreateGrid(160.0f, 160.0f, 50, 50, grid);

	std::vector<Vertex> vertices(grid.Vertices.size());
	for (UINT i = 0; i < (UINT)vertices.size(); ++i)
	{
		auto& p = grid.Vertices[i].Position;
		vertices[i].Position = p;
		vertices[i].Position.y = GetHillsHeight(p.x, p.z);
		vertices[i].Normal = GetHillsNormal(p.x, p.z);
		vertices[i].TexC = grid.Vertices[i].TexC;
	}

	std::vector<UINT> indices = grid.Indices;

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(UINT);

	///
	/// 创建顶点缓冲区.
	///
	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = vbByteSize;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	vbd.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA vInitData;
	vInitData.pSysMem = vertices.data();
	vInitData.SysMemPitch = 0;
	vInitData.SysMemSlicePitch = 0;

	HR(md3dDevice->CreateBuffer(&vbd, &vInitData, mVertexBuffers["hill"].GetAddressOf()));

	///
	/// 创建索引缓冲区.
	///
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

	HR(md3dDevice->CreateBuffer(&ibd, &iInitData, mIndexBuffers["hill"].GetAddressOf()));

	///
	/// 几何体信息.
	///
	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "hillGeo";

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["hill"] = submesh;
	geo->VertexBuffer = mVertexBuffers["hill"].Get();
	geo->IndexBuffer = mIndexBuffers["hill"].Get();

	mGeos[geo->Name] = std::move(geo);
}

void WavesApp::BuildWavesBuffers()
{
	std::vector<UINT> indices(mWaves->TriangleCount() * 3);
	assert(mWaves->VertexCount() < 0x0000ffff);

	int m = mWaves->RowCount();
	int n = mWaves->ColumnCount();
	int k = 0;

	for (int i = 0; i < m - 1; ++i)
	{
		for (int j = 0; j < n - 1; ++j)
		{
			indices[k++] = i * n + j;
			indices[k++] = i * n + j + 1;
			indices[k++] = (i + 1) * n + j;

			indices[k++] = (i + 1) * n + j;
			indices[k++] = i * n + j + 1;
			indices[k++] = (i + 1) * n + j + 1;
		}
	}

	const UINT vbByteSize = (UINT)mWaves->VertexCount() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(UINT);

	///
	/// 创建动态顶点缓冲区.
	///
	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_DYNAMIC;
	vbd.ByteWidth = vbByteSize;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	vbd.MiscFlags = 0;
	vbd.StructureByteStride = 0;

	HR(md3dDevice->CreateBuffer(&vbd, nullptr, mVertexBuffers["waves"].GetAddressOf()));

	///
	/// 创建索引缓冲区.
	///
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

	HR(md3dDevice->CreateBuffer(&ibd, &iInitData, mIndexBuffers["waves"].GetAddressOf()));

	///
	/// 几何体信息.
	///
	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "wavesGeo";

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["waves"] = submesh;
	geo->VertexBuffer = mVertexBuffers["waves"].Get();
	geo->IndexBuffer = mIndexBuffers["waves"].Get();

	mGeos[geo->Name] = std::move(geo);
}

void WavesApp::BuildConstantBuffers()
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

	HR(md3dDevice->CreateBuffer(&passCbd, nullptr, mConstantBuffers["pass"].GetAddressOf()));
}

void WavesApp::BuildShadersAndInputLayout()
{
	ComPtr<ID3DBlob> vertexBlob = d3dUtil::CompileShader(L"Shaders/Default.hlsl", nullptr, "VS", "vs_5_0");
	ComPtr<ID3DBlob> pixelBlob = d3dUtil::CompileShader(L"Shaders/Default.hlsl", nullptr, "PS", "ps_5_0");

	HR(md3dDevice->CreateVertexShader(vertexBlob->GetBufferPointer(), vertexBlob->GetBufferSize(),
		nullptr, mVertexShader.GetAddressOf()));
	HR(md3dDevice->CreatePixelShader(pixelBlob->GetBufferPointer(), pixelBlob->GetBufferSize(),
		nullptr, mPixelShader.GetAddressOf()));

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

void WavesApp::BuildRasterizerStates()
{
	///
	/// 网格渲染.
	///
	D3D11_RASTERIZER_DESC rsDesc;
	ZeroMemory(&rsDesc, sizeof(D3D11_RASTERIZER_DESC));
	rsDesc.CullMode = D3D11_CULL_BACK;
	rsDesc.FillMode = D3D11_FILL_WIREFRAME;
	rsDesc.FrontCounterClockwise = FALSE;
	rsDesc.DepthClipEnable = TRUE;

	HR(md3dDevice->CreateRasterizerState(&rsDesc, mRasterizerStates["opaque_wireframe"].GetAddressOf()));
}

void WavesApp::BuildMaterials()
{
	auto grass = std::make_unique<Material>();
	grass->Name = "grass";
	grass->MatCBIndex = 0;
	grass->DiffuseSrvHeapIndex = 0;
	grass->DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	grass->FresnelR0 = { 0.01f, 0.01f, 0.01f };
	grass->Roughness = 0.125f;

	auto water = std::make_unique<Material>();
	water->Name = "water";
	water->MatCBIndex = 1;
	water->DiffuseSrvHeapIndex = 1;
	water->DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	water->FresnelR0 = { 0.2f, 0.2f, 0.2f };
	water->Roughness = 0.0f;

	mMaterials["grass"] = std::move(grass);
	mMaterials["water"] = std::move(water);
}

void WavesApp::BuildRenderItems()
{
	auto wavesRitem = std::make_unique<RenderItem>();
	wavesRitem->World = MathHelper::Identity4x4();
	wavesRitem->ObjectCBIndex = 0;
	wavesRitem->Geo = mGeos["wavesGeo"].get();
	wavesRitem->Mat = mMaterials["water"].get();
	XMStoreFloat4x4(&wavesRitem->TexTransform, XMMatrixScaling(5.0f, 5.0f, 1.0f));
	wavesRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	wavesRitem->IndexCount = wavesRitem->Geo->DrawArgs["waves"].IndexCount;
	wavesRitem->StartIndexLocation = wavesRitem->Geo->DrawArgs["waves"].StartIndexLocation;
	wavesRitem->BaseVertexLocation = wavesRitem->Geo->DrawArgs["waves"].BaseVertexLocation;
	wavesRitem->VertexBuffer = wavesRitem->Geo->VertexBuffer.Get();
	wavesRitem->IndexBuffer = wavesRitem->Geo->IndexBuffer.Get();
	mWaveRitem = wavesRitem.get();
	mRitemLayer[(int)RenderLayer::Opaque].push_back(wavesRitem.get());

	auto hillRitem = std::make_unique<RenderItem>();
	hillRitem->World = MathHelper::Identity4x4();
	hillRitem->ObjectCBIndex = 1;
	hillRitem->Geo = mGeos["hillGeo"].get();
	hillRitem->Mat = mMaterials["grass"].get();
	XMStoreFloat4x4(&hillRitem->TexTransform, XMMatrixScaling(5.0f, 5.0f, 1.0f));
	hillRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	hillRitem->IndexCount = hillRitem->Geo->DrawArgs["hill"].IndexCount;
	hillRitem->StartIndexLocation = hillRitem->Geo->DrawArgs["hill"].StartIndexLocation;
	hillRitem->BaseVertexLocation = hillRitem->Geo->DrawArgs["hill"].BaseVertexLocation;
	hillRitem->VertexBuffer = hillRitem->Geo->VertexBuffer.Get();
	hillRitem->IndexBuffer = hillRitem->Geo->IndexBuffer.Get();
	mRitemLayer[(int)RenderLayer::Opaque].push_back(hillRitem.get());

	mAllRitems.push_back(std::move(wavesRitem));
	mAllRitems.push_back(std::move(hillRitem));
}

void WavesApp::DrawRenderItems(ID3D11DeviceContext* context, const std::vector<RenderItem*>& ritems)
{
	for (size_t i = 0; i < ritems.size(); i++)
	{
		auto ri = ritems[i];

		UINT stride = sizeof(Vertex);
		UINT offset = 0;
		context->IASetVertexBuffers(0, 1, &ri->VertexBuffer, &stride, &offset);
		context->IASetIndexBuffer(ri->IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
		context->IASetPrimitiveTopology(ri->PrimitiveType);

		// 常量缓冲区数据.
		XMMATRIX world = XMLoadFloat4x4(&ri->World);
		XMMATRIX texTransform = XMLoadFloat4x4(&ri->TexTransform);

		ObjectConstants objConstants;
		XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
		XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));
		d3dUtil::CopyDataToGpu(context, &objConstants, 
			sizeof(ObjectConstants), mConstantBuffers["object"].Get());

		context->VSSetConstantBuffers(0, 1, mConstantBuffers["object"].GetAddressOf());

		// 材质常量缓冲区数据.
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

		// 纹理贴图数据.
		context->PSSetShaderResources(0, 1, mTextureSrvs[ri->Mat->DiffuseSrvHeapIndex].GetAddressOf());

		context->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation,
			ri->BaseVertexLocation, 0);
	}
}

float WavesApp::GetHillsHeight(float x, float z) const
{
	return 0.3f * (z * sinf(0.1f * x) + x * cosf(0.1f * z));
}

XMFLOAT3 WavesApp::GetHillsNormal(float x, float z) const
{
	XMFLOAT3 n(
		-0.03f * z * cosf(0.1f * x) - 0.3f * cosf(0.1f * z),
		1.0f,
		-0.3f * sinf(0.1f * x) + 0.03f * x * sinf(0.1f * z));

	XMVECTOR unitNormal = XMVector3Normalize(XMLoadFloat3(&n));
	XMStoreFloat3(&n, unitNormal);

	return n;
}
