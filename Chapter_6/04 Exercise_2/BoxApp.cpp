#include "BoxApp.h"

BoxApp::BoxApp(HINSTANCE hInstance) : D3DApp(hInstance)
{
	mMainWndCaption = L"Box App";
}

BoxApp::~BoxApp()
{
}

bool BoxApp::Init()
{
	if (!D3DApp::Init())
		return false;

	BuildBuffers();
	BuildShadersAndInputLayout();

	return true;
}

void BoxApp::OnResize()
{
	D3DApp::OnResize();

	XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, proj);
}

void BoxApp::UpdateScene(float dt)
{
	float x = mRadius * sinf(mPhi) * cosf(mTheta);
	float z = mRadius * sinf(mPhi) * sinf(mTheta);
	float y = mRadius * cosf(mPhi);

	XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&mView, view);

	XMMATRIX world = XMLoadFloat4x4(&mWorld);
	XMMATRIX proj = XMLoadFloat4x4(&mProj);
	XMMATRIX worldViewProj = world * view * proj;
	XMStoreFloat4x4(&mObjectCB.WorldViewProj, XMMatrixTranspose(worldViewProj));

	// ��CPU�ĳ��������ϴ���GPU.
	D3D11_MAPPED_SUBRESOURCE mappedData;
	HR(md3dImmediateContext->Map(mConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));
	memcpy_s(mappedData.pData, sizeof(ConstantBuffer), &mObjectCB, sizeof(ConstantBuffer));
	md3dImmediateContext->Unmap(mConstantBuffer.Get(), 0);
}

void BoxApp::DrawScene()
{
	md3dImmediateContext->ClearRenderTargetView(CurrentBackBufferView().Get(), Colors::LightSteelBlue);
	md3dImmediateContext->ClearDepthStencilView(mDepthStencilView.Get(),
		D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// ������, �����Ҫָ���µ���ȾĿ��.
	md3dImmediateContext->OMSetRenderTargets(1, CurrentBackBufferView().GetAddressOf(), mDepthStencilView.Get());

	// ������Ҫ�����벼��������ͼԪ����.
	md3dImmediateContext->IASetInputLayout(mInputLayout.Get());
	md3dImmediateContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// �󶨶��㻺����������������.
	UINT posStride = sizeof(VertexPosition);
	UINT colorStride = sizeof(VertexColor);
	UINT offset = 0;
	md3dImmediateContext->IASetVertexBuffers(0, 1, mVertexPositionBuffer.GetAddressOf(), &posStride, &offset);
	md3dImmediateContext->IASetVertexBuffers(1, 1, mVertexColorBuffer.GetAddressOf(), &colorStride, &offset);
	md3dImmediateContext->IASetIndexBuffer(mIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

	// ���ö�����ɫ����������ɫ��, �Լ�������Ҫ�ĳ���������.
	md3dImmediateContext->VSSetShader(mVertexShader.Get(), nullptr, 0);
	md3dImmediateContext->VSSetConstantBuffers(0, 1, mConstantBuffer.GetAddressOf());
	md3dImmediateContext->PSSetShader(mPixelShader.Get(), nullptr, 0);

	// ���¾���.
	XMMATRIX world = XMLoadFloat4x4(&mWorld);
	XMMATRIX view = XMLoadFloat4x4(&mView);
	XMMATRIX proj = XMLoadFloat4x4(&mProj);
	XMMATRIX worldViewProj = world * view * proj;
	XMStoreFloat4x4(&mObjectCB.WorldViewProj, worldViewProj);

	// ����.
	md3dImmediateContext->DrawIndexed(36, 0, 0);

	// ������.
	mCurrentBuffer = (mCurrentBuffer + 1) % FlipBufferCount;

	HR(mSwapChain->Present(0, 0));
}

void BoxApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void BoxApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void BoxApp::OnMouseMove(WPARAM btnState, int x, int y)
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
		float dx = 0.005f * static_cast<float>(x - mLastMousePos.x);
		float dy = 0.005f * static_cast<float>(y - mLastMousePos.y);

		mRadius += dx - dy;

		mRadius = MathHelper::Clamp(mRadius, 3.0f, 15.0f);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void BoxApp::BuildBuffers()
{
	///
	/// ����λ�ö��㻺����.
	///
	std::vector<VertexPosition> posVertices =
	{
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f) },
		{ XMFLOAT3(-1.0f, +1.0f, -1.0f) },
		{ XMFLOAT3(+1.0f, +1.0f, -1.0f) },
		{ XMFLOAT3(+1.0f, -1.0f, -1.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, +1.0f) },
		{ XMFLOAT3(-1.0f, +1.0f, +1.0f) },
		{ XMFLOAT3(+1.0f, +1.0f, +1.0f) },
		{ XMFLOAT3(+1.0f, -1.0f, +1.0f) }
	};

	D3D11_BUFFER_DESC posVbd;
	posVbd.Usage = D3D11_USAGE_IMMUTABLE;
	posVbd.ByteWidth = sizeof(VertexPosition) * posVertices.size();
	posVbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	posVbd.CPUAccessFlags = 0;
	posVbd.StructureByteStride = 0;
	posVbd.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA vpInitData;
	vpInitData.pSysMem = posVertices.data();
	HR(md3dDevice->CreateBuffer(&posVbd, &vpInitData, mVertexPositionBuffer.GetAddressOf()));

	///
	/// ������ɫ���㻺����.
	///
	std::vector<VertexColor> colorVetices =
	{
		{ XMFLOAT4(Colors::White) },
		{ XMFLOAT4(Colors::Black) },
		{ XMFLOAT4(Colors::Red) },
		{ XMFLOAT4(Colors::Green) },
		{ XMFLOAT4(Colors::Blue) },
		{ XMFLOAT4(Colors::Yellow) },
		{ XMFLOAT4(Colors::Cyan) },
		{ XMFLOAT4(Colors::Magenta) }
	};

	D3D11_BUFFER_DESC colorVbd;
	colorVbd.Usage = D3D11_USAGE_IMMUTABLE;
	colorVbd.ByteWidth = sizeof(VertexColor) * colorVetices.size();
	colorVbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	colorVbd.CPUAccessFlags = 0;
	colorVbd.StructureByteStride = 0;
	colorVbd.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA vcInitData;
	vcInitData.pSysMem = colorVetices.data();
	vcInitData.SysMemPitch = 0;
	vcInitData.SysMemSlicePitch = 0;
	HR(md3dDevice->CreateBuffer(&colorVbd, &vcInitData, mVertexColorBuffer.GetAddressOf()));

	///
	/// ��������������.
	///
	std::vector<UINT> indices =
	{
		// front face
		0, 1, 2,
		0, 2, 3,

		// back face
		4, 6, 5,
		4, 7, 6,

		// left face
		4, 5, 1,
		4, 1, 0,

		// right face
		3, 2, 6,
		3, 6, 7,

		// top face
		1, 5, 6,
		1, 6, 2,

		// bottom face
		4, 0, 3,
		4, 3, 7
	};

	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(UINT) * indices.size();
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.StructureByteStride = 0;
	ibd.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA iInitData;
	iInitData.pSysMem = indices.data();
	HR(md3dDevice->CreateBuffer(&ibd, &iInitData, mIndexBuffer.GetAddressOf()));

	///
	/// ��������������.
	///
	D3D11_BUFFER_DESC cbd;
	ZeroMemory(&cbd, sizeof(cbd));
	cbd.Usage = D3D11_USAGE_DYNAMIC;
	cbd.ByteWidth = sizeof(ConstantBuffer);
	cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbd.StructureByteStride = 0;
	cbd.MiscFlags = 0;
	HR(md3dDevice->CreateBuffer(&cbd, nullptr, mConstantBuffer.GetAddressOf()));
}

void BoxApp::BuildShadersAndInputLayout()
{
	///
	/// ������ɫ��, Ȼ�󴴽�������ɫ����������ɫ��.
	///
	ComPtr<ID3DBlob> vertexBlob = d3dUtil::CompileShader(L"Shaders/color.hlsl", nullptr, "VS", "vs_5_0");
	HR(md3dDevice->CreateVertexShader(vertexBlob->GetBufferPointer(), vertexBlob->GetBufferSize(), nullptr, mVertexShader.GetAddressOf()));

	ComPtr<ID3DBlob> pixelBlob = d3dUtil::CompileShader(L"Shaders/color.hlsl", nullptr, "PS", "ps_5_0");
	HR(md3dDevice->CreatePixelShader(pixelBlob->GetBufferPointer(), pixelBlob->GetBufferSize(), nullptr, mPixelShader.GetAddressOf()));

	///
	/// ������������벼������.
	///
	D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
		D3D11_INPUT_PER_VERTEX_DATA, 0},
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0,
		D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	HR(md3dDevice->CreateInputLayout(vertexDesc, ARRAYSIZE(vertexDesc),
		vertexBlob->GetBufferPointer(), vertexBlob->GetBufferSize(),
		mInputLayout.GetAddressOf()));
}
