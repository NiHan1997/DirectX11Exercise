#include "CreteApp.h"

CrateApp::CrateApp(HINSTANCE hInstance) : D3DApp(hInstance)
{
	mMainWndCaption = L"Shapes App";
}

CrateApp::~CrateApp()
{
}

bool CrateApp::Init()
{
	if (D3DApp::Init() == false)
		return false;

	LoadTextures();
	BuildSamplerStates();
	BuildShapesBuffers();
	BuildShadersAndInputLayout();
	BuildMaterials();
	BuildRenderItems();
	BuildConstantBuffers();
	BuildRasterizerStates();

	return true;
}

void CrateApp::OnResize()
{
	D3DApp::OnResize();

	XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, proj);
}

void CrateApp::UpdateScene(GameTimer gt)
{
	UpdateCamera(gt);
	UpdatePassCB(gt);
}

void CrateApp::DrawScene()
{
	md3dImmediateContext->ClearRenderTargetView(CurrentBackBufferView().Get(), Colors::LightSteelBlue);
	md3dImmediateContext->ClearDepthStencilView(mDepthStencilView.Get(),
		D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// ������, �����Ҫָ���µ���ȾĿ��.
	md3dImmediateContext->OMSetRenderTargets(1, CurrentBackBufferView().GetAddressOf(), mDepthStencilView.Get());

	// ������Ҫ�����벼��������ͼԪ����.
	md3dImmediateContext->IASetInputLayout(mInputLayout.Get());

	// ���ö�����ɫ����������ɫ��, �Լ�������Ҫ�ĳ���������.
	md3dImmediateContext->VSSetShader(mVertexShader.Get(), nullptr, 0);
	md3dImmediateContext->PSSetShader(mPixelShader.Get(), nullptr, 0);

	// ���ù��̳���������.
	md3dImmediateContext->VSSetConstantBuffers(2, 1, mConstantBuffers["pass"].GetAddressOf());
	md3dImmediateContext->PSSetConstantBuffers(2, 1, mConstantBuffers["pass"].GetAddressOf());

	// ���ò�����������.
	md3dImmediateContext->PSSetSamplers(0, mSamplers.size(), mSamplers.data()->GetAddressOf());

	// ����������֡����60֡, ÿһ֡����һ�λ���.
	static int index = 0;
	md3dImmediateContext->PSSetShaderResources(0, 1, mTextureSrvs[index].GetAddressOf());
	index = (index + 1) % mTextureSrvs.size();

	md3dImmediateContext->RSSetState(nullptr);

	// ����.
	DrawRenderItems(md3dImmediateContext.Get(), mOpaqueRitems);

	// ������.
	mCurrentBuffer = (mCurrentBuffer + 1) % FlipBufferCount;

	HR(mSwapChain->Present(0, 0));
}

void CrateApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void CrateApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void CrateApp::OnMouseMove(WPARAM btnState, int x, int y)
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
		float dx = 0.01f * static_cast<float>(x - mLastMousePos.x);
		float dy = 0.01f * static_cast<float>(y - mLastMousePos.y);

		mRadius += dx - dy;

		mRadius = MathHelper::Clamp(mRadius, 5.0f, 50.0f);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void CrateApp::UpdateCamera(GameTimer gt)
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

void CrateApp::UpdatePassCB(GameTimer gt)
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

void CrateApp::LoadTextures()
{
	mTextureSrvs.resize(120);

	for (int i = 0; i < (int)mTextureSrvs.capacity(); ++i)
	{
		std::wstring texPath;
		if (i < 9)
			texPath = L"Textures/Fire00" + std::to_wstring(i + 1) + L".DDS";
		else if (i < 99)
			texPath = L"Textures/Fire0" + std::to_wstring(i + 1) + L".DDS";
		else
			texPath = L"Textures/Fire" + std::to_wstring(i + 1) + L".DDS";

		HR(CreateDDSTextureFromFile(md3dDevice.Get(), texPath.c_str(),
			nullptr, mTextureSrvs[i].GetAddressOf()));
	}
}

void CrateApp::BuildSamplerStates()
{
	mSamplers.resize(6);

	///
	/// �����ﴴ��6�ֳ��õĲ�����.
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

void CrateApp::BuildShapesBuffers()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData box;

	geoGen.CreateBox(2.0f, 2.0f, 2.0f, box);

	///
	/// �����帨���ṹ��.
	///
	SubmeshGeometry boxSubmesh;
	boxSubmesh.IndexCount = (UINT)box.Indices.size();
	boxSubmesh.StartIndexLocation = 0;
	boxSubmesh.BaseVertexLocation = 0;

	///
	/// ���㻺����.
	///
	std::vector<Vertex> vertices(box.Vertices.size());
	for (UINT i = 0; i < (UINT)box.Vertices.size(); ++i)
	{
		vertices[i].Position = box.Vertices[i].Position;
		vertices[i].Normal = box.Vertices[i].Normal;
		vertices[i].TexC = box.Vertices[i].TexC;
	}

	///
	/// ����������.
	///
	std::vector<UINT> indices = box.Indices;

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(UINT);

	///
	/// �������㻺����.
	///
	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = vbByteSize;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.StructureByteStride = 0;
	vbd.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA vInitData;
	vInitData.pSysMem = vertices.data();
	vInitData.SysMemPitch = 0;
	vInitData.SysMemSlicePitch = 0;
	HR(md3dDevice->CreateBuffer(&vbd, &vInitData, mVertexBuffers["shapes"].GetAddressOf()));

	///
	/// ��������������.
	///
	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = ibByteSize;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.StructureByteStride = 0;
	ibd.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA iInitData;
	iInitData.pSysMem = indices.data();
	iInitData.SysMemPitch = 0;
	iInitData.SysMemSlicePitch = 0;
	HR(md3dDevice->CreateBuffer(&ibd, &iInitData, mIndexBuffers["shapes"].GetAddressOf()));

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "shapesGeo";

	geo->DrawArgs["box"] = boxSubmesh;

	mGeos[geo->Name] = std::move(geo);
}

void CrateApp::BuildConstantBuffers()
{
	/// ���峣��������.
	D3D11_BUFFER_DESC objCbd;
	objCbd.Usage = D3D11_USAGE_DYNAMIC;
	objCbd.ByteWidth = sizeof(ObjectConstants);
	objCbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	objCbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	objCbd.StructureByteStride = 0;
	objCbd.MiscFlags = 0;
	HR(md3dDevice->CreateBuffer(&objCbd, nullptr, mConstantBuffers["object"].GetAddressOf()));

	/// ���ʳ���������.
	D3D11_BUFFER_DESC matCbd;
	matCbd.Usage = D3D11_USAGE_DYNAMIC;
	matCbd.ByteWidth = sizeof(MaterialConstants);
	matCbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	matCbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	matCbd.StructureByteStride = 0;
	matCbd.MiscFlags = 0;
	HR(md3dDevice->CreateBuffer(&matCbd, nullptr, mConstantBuffers["material"].GetAddressOf()));

	/// ���̳���������.
	D3D11_BUFFER_DESC passCbd;
	passCbd.Usage = D3D11_USAGE_DYNAMIC;
	passCbd.ByteWidth = sizeof(PassConstants);
	passCbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	passCbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	passCbd.StructureByteStride = 0;
	passCbd.MiscFlags = 0;
	HR(md3dDevice->CreateBuffer(&passCbd, nullptr, mConstantBuffers["pass"].GetAddressOf()));
}

void CrateApp::BuildShadersAndInputLayout()
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
		D3D11_INPUT_PER_VERTEX_DATA, 0},
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
		D3D11_INPUT_PER_VERTEX_DATA, 0},
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24,
		D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

	HR(md3dDevice->CreateInputLayout(vertexDesc, ARRAYSIZE(vertexDesc),
		vertexBlob->GetBufferPointer(), vertexBlob->GetBufferSize(),
		mInputLayout.GetAddressOf()));
}

void CrateApp::BuildRasterizerStates()
{
	D3D11_RASTERIZER_DESC rsDesc;
	ZeroMemory(&rsDesc, sizeof(D3D11_RASTERIZER_DESC));
	rsDesc.FillMode = D3D11_FILL_WIREFRAME;
	rsDesc.CullMode = D3D11_CULL_BACK;
	rsDesc.FrontCounterClockwise = FALSE;
	rsDesc.DepthClipEnable = TRUE;

	HR(md3dDevice->CreateRasterizerState(&rsDesc, mRasterizerStates["opaque_wireframe"].GetAddressOf()));
}

void CrateApp::BuildMaterials()
{
	auto boxMat = std::make_unique<Material>();
	boxMat->Name = "boxMat";
	boxMat->MatCBIndex = 0;
	boxMat->DiffuseSrvHeapIndex = 0;
	boxMat->DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	boxMat->FresnelR0 = { 0.05f, 0.05f, 0.05f };
	boxMat->Roughness = 0.2f;

	mMaterials["boxMat"] = std::move(boxMat);
}

void CrateApp::BuildRenderItems()
{
	// ��������Ⱦ��.
	auto boxRitem = std::make_unique<RenderItem>();
	boxRitem->ObjectCBIndex = 0;
	XMStoreFloat4x4(&boxRitem->World, XMMatrixScaling(4.0f, 4.0f, 4.0f));
	boxRitem->Geo = mGeos["shapesGeo"].get();
	boxRitem->Mat = mMaterials["boxMat"].get();
	boxRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	boxRitem->VertexBuffer = mVertexBuffers["shapes"].Get();
	boxRitem->IndexBuffer = mIndexBuffers["shapes"].Get();
	boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
	boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
	boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
	mAllRitems.push_back(std::move(boxRitem));

	for (auto& e : mAllRitems)
		mOpaqueRitems.push_back(e.get());
}

void CrateApp::DrawRenderItems(ID3D11DeviceContext* context, const std::vector<RenderItem*>& ritems)
{
	for (size_t i = 0; i < ritems.size(); i++)
	{
		auto ri = ritems[i];

		UINT stride = sizeof(Vertex);
		UINT offset = 0;
		context->IASetVertexBuffers(0, 1, &ri->VertexBuffer, &stride, &offset);
		context->IASetIndexBuffer(ri->IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
		context->IASetPrimitiveTopology(ri->PrimitiveType);

		// ��������������.
		XMMATRIX world = XMLoadFloat4x4(&ri->World);
		XMMATRIX texTransform = XMLoadFloat4x4(&ri->TexTransform);

		ObjectConstants objConstants;
		XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
		XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));

		d3dUtil::CopyDataToGpu(context, &objConstants,
			sizeof(ObjectConstants), mConstantBuffers["object"].Get());

		context->VSSetConstantBuffers(0, 1, mConstantBuffers["object"].GetAddressOf());

		// ���ʳ���������.
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

		context->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation,
			ri->BaseVertexLocation, 0);
	}
}
