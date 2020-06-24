#include "CylinderApp.h"

CylinderApp::CylinderApp(HINSTANCE hInstance) : D3DApp(hInstance)
{
}

CylinderApp::~CylinderApp()
{
}

bool CylinderApp::Init()
{
    if (D3DApp::Init() == false)
        return false;

	LoadTextures();
	BuildSamplerStates();
	BuildCircleBuffers();
	BuildShadersAndInputLayout();
	BuildMaterials();
	BuildRenderItems();
	BuildConstantBuffers();
	BuildRasterizerStates();

	return true;
}

void CylinderApp::OnResize()
{
	D3DApp::OnResize();

	XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, proj);
}

void CylinderApp::UpdateScene(GameTimer gt)
{
	AnimateMaterials(gt);
	UpdateCamera(gt);
	UpdatePassCB(gt);
}

void CylinderApp::DrawScene()
{
	md3dImmediateContext->ClearRenderTargetView(CurrentBackBufferView().Get(), Colors::LightSteelBlue);
	md3dImmediateContext->ClearDepthStencilView(mDepthStencilView.Get(),
		D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// 交换链, 因此需要指定新的渲染目标.
	md3dImmediateContext->OMSetRenderTargets(1, CurrentBackBufferView().GetAddressOf(), mDepthStencilView.Get());

	// 设置顶点需要的常量缓冲区.
	md3dImmediateContext->VSSetConstantBuffers(2, 1, mConstantBuffers["pass"].GetAddressOf());
	md3dImmediateContext->GSSetConstantBuffers(2, 1, mConstantBuffers["pass"].GetAddressOf());
	md3dImmediateContext->PSSetConstantBuffers(2, 1, mConstantBuffers["pass"].GetAddressOf());

	// 绘制需要的输入布局描述和图元拓扑.
	md3dImmediateContext->IASetInputLayout(mInputLayout.Get());

	/// <summary>
	/// 绘制不透明物体.
	/// </summary>
	md3dImmediateContext->VSSetShader(mVertexShaders["standardVS"].Get(), nullptr, 0);
	md3dImmediateContext->GSSetShader(mGeometryShaders["cylinderGS"].Get(), nullptr, 0);
	md3dImmediateContext->PSSetShader(mPixelShaders["opaquePS"].Get(), nullptr, 0);

	DrawRenderItems(md3dImmediateContext.Get(), mRitemLayer[(int)RenderLayer::Opaque]);

	HR(mSwapChain->Present(0, 0));

	// 交换链.
	mCurrentBuffer = (mCurrentBuffer + 1) % FlipBufferCount;
}

void CylinderApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void CylinderApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void CylinderApp::OnMouseMove(WPARAM btnState, int x, int y)
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

void CylinderApp::UpdateCamera(GameTimer gt)
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

void CylinderApp::UpdatePassCB(GameTimer gt)
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

void CylinderApp::AnimateMaterials(GameTimer gt)
{
}

void CylinderApp::LoadTextures()
{
	mTextureSrvs.resize(1);
	HR(CreateDDSTextureFromFile(md3dDevice.Get(), L"../../Textures/white1x1.dds",
		nullptr, mTextureSrvs[0].GetAddressOf()));
}

void CylinderApp::BuildSamplerStates()
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

void CylinderApp::BuildCircleBuffers()
{
	// 细分次数和圆形半径.
	int subdivision = 20;
	int radius = 5;

	// 顶点数据和索引数据一起对应.
	std::vector<Vertex> vertices(subdivision + 1);
	std::vector<UINT> indices(subdivision + 1);

	for (int i = 0; i <= subdivision; ++i)
	{
		float x = 5 * cosf(i * XM_2PI / subdivision);
		float z = 5 * sinf(i * XM_2PI / subdivision);

		vertices[i].Position = { x, 0.0f, z };
		vertices[i].Normal = { 1.0f, 1.0f, 1.0f };
		vertices[i].TexC = { 0.0f, 0.0f };

		indices[i] = i;
	}

	/// <summary>
	/// 基本信息创建.
	/// </summary>
	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(UINT);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "circleGeo";

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	/// <summary>
	/// 顶点缓冲区创建.
	/// </summary>
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

	HR(md3dDevice->CreateBuffer(&vbd, &vInitData, mVertexBuffers["circle"].GetAddressOf()));

	/// <summary>
	/// 索引缓冲区构建.
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

	HR(md3dDevice->CreateBuffer(&ibd, &iInitData, mIndexBuffers["circle"].GetAddressOf()));

	geo->DrawArgs["circle"] = submesh;
	mGeos[geo->Name] = std::move(geo);
}

void CylinderApp::BuildConstantBuffers()
{
	/// <summary>
	/// 物体常量缓冲区.
	/// </summary>
	D3D11_BUFFER_DESC objCbd;
	objCbd.Usage = D3D11_USAGE_DYNAMIC;
	objCbd.ByteWidth = sizeof(ObjectConstants);
	objCbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	objCbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	objCbd.MiscFlags = 0;
	objCbd.StructureByteStride = 0;

	HR(md3dDevice->CreateBuffer(&objCbd, nullptr, mConstantBuffers["object"].GetAddressOf()));

	/// <summary>
	/// 材质常量缓冲区.
	/// </summary>
	D3D11_BUFFER_DESC matCbd;
	matCbd.Usage = D3D11_USAGE_DYNAMIC;
	matCbd.ByteWidth = sizeof(MaterialConstants);
	matCbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	matCbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	matCbd.MiscFlags = 0;
	matCbd.StructureByteStride = 0;

	HR(md3dDevice->CreateBuffer(&matCbd, nullptr, mConstantBuffers["material"].GetAddressOf()));

	/// <summary>
	/// 每一帧的过程常量缓冲区.
	/// </summary>
	D3D11_BUFFER_DESC passCbd;
	passCbd.Usage = D3D11_USAGE_DYNAMIC;
	passCbd.ByteWidth = sizeof(PassConstants);
	passCbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	passCbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	passCbd.MiscFlags = 0;
	passCbd.StructureByteStride = 0;

	HR(md3dDevice->CreateBuffer(&passCbd, nullptr, mConstantBuffers["pass"].GetAddressOf()));
}

void CylinderApp::BuildShadersAndInputLayout()
{
	ComPtr<ID3DBlob> vertexBlob = d3dUtil::CompileShader(L"Shaders/Default.hlsl", nullptr, "VS", "vs_5_0");
	ComPtr<ID3DBlob> geometryBlob = d3dUtil::CompileShader(L"Shaders/Default.hlsl", nullptr, "GS", "gs_5_0");
	ComPtr<ID3DBlob> opaquePixelBlob = d3dUtil::CompileShader(L"Shaders/Default.hlsl", nullptr, "PS", "ps_5_0");

	HR(md3dDevice->CreateVertexShader(vertexBlob->GetBufferPointer(), vertexBlob->GetBufferSize(),
		nullptr, mVertexShaders["standardVS"].GetAddressOf()));
	HR(md3dDevice->CreateGeometryShader(geometryBlob->GetBufferPointer(), geometryBlob->GetBufferSize(),
		nullptr, mGeometryShaders["cylinderGS"].GetAddressOf()));
	HR(md3dDevice->CreatePixelShader(opaquePixelBlob->GetBufferPointer(), opaquePixelBlob->GetBufferSize(),
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

void CylinderApp::BuildRasterizerStates()
{
	///
	/// 网格渲染.
	///
	D3D11_RASTERIZER_DESC wireframeRSDesc;
	ZeroMemory(&wireframeRSDesc, sizeof(D3D11_RASTERIZER_DESC));
	wireframeRSDesc.CullMode = D3D11_CULL_NONE;
	wireframeRSDesc.FillMode = D3D11_FILL_WIREFRAME;
	wireframeRSDesc.FrontCounterClockwise = FALSE;
	wireframeRSDesc.DepthClipEnable = TRUE;

	HR(md3dDevice->CreateRasterizerState(&wireframeRSDesc, mRasterizerStates["opaque_wireframe"].GetAddressOf()));
}

void CylinderApp::BuildMaterials()
{
	auto defaultMat = std::make_unique<Material>();
	defaultMat->Name = "default";
	defaultMat->MatCBIndex = 0;
	defaultMat->DiffuseSrvHeapIndex = 0;
	defaultMat->DiffuseAlbedo = { 1.0f, 0.0f, 0.0f, 1.0f };
	defaultMat->FresnelR0 = { 0.01f, 0.01f, 0.01f };
	defaultMat->Roughness = 0.125f;

	mMaterials[defaultMat->Name] = std::move(defaultMat);
}

void CylinderApp::BuildRenderItems()
{
	auto mCircleRitem = std::make_unique<RenderItem>();
	mCircleRitem->World = MathHelper::Identity4x4();
	mCircleRitem->TexTransform = MathHelper::Identity4x4();
	mCircleRitem->ObjectCBIndex = 0;
	mCircleRitem->Geo = mGeos["circleGeo"].get();
	mCircleRitem->Mat = mMaterials["default"].get();
	mCircleRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
	mCircleRitem->VertexBuffer = mVertexBuffers["circle"].Get();
	mCircleRitem->IndexBuffer = mIndexBuffers["circle"].Get();
	mCircleRitem->IndexCount = mCircleRitem->Geo->DrawArgs["circle"].IndexCount;
	mCircleRitem->StartIndexLocation = mCircleRitem->Geo->DrawArgs["circle"].StartIndexLocation;
	mCircleRitem->BaseVertexLocation = mCircleRitem->Geo->DrawArgs["circle"].BaseVertexLocation;

	mRitemLayer[(int)RenderLayer::Opaque].push_back(mCircleRitem.get());
	mAllRitems.push_back(std::move(mCircleRitem));
}

void CylinderApp::DrawRenderItems(ID3D11DeviceContext* context, const std::vector<RenderItem*>& ritems)
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
