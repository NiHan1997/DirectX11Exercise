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

	// 将CPU的常量数据上传到GPU.
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

	// 交换链, 因此需要指定新的渲染目标.
	md3dImmediateContext->OMSetRenderTargets(1, CurrentBackBufferView().GetAddressOf(), mDepthStencilView.Get());

	// 绘制需要的输入布局描述和图元拓扑.
	md3dImmediateContext->IASetInputLayout(mInputLayout.Get());
	md3dImmediateContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 绑定顶点缓冲区和索引缓冲区.
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	md3dImmediateContext->IASetVertexBuffers(0, 1, mVertexBuffer.GetAddressOf(), &stride, &offset);
	md3dImmediateContext->IASetIndexBuffer(mIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

	// 设置顶点着色器和像素着色器, 以及顶点需要的常量缓冲区.
	md3dImmediateContext->VSSetShader(mVertexShader.Get(), nullptr, 0);
	md3dImmediateContext->VSSetConstantBuffers(0, 1, mConstantBuffer.GetAddressOf());
	md3dImmediateContext->PSSetShader(mPixelShader.Get(), nullptr, 0);

	// 更新矩阵.
	XMMATRIX world = XMLoadFloat4x4(&mWorld);
	XMMATRIX view = XMLoadFloat4x4(&mView);
	XMMATRIX proj = XMLoadFloat4x4(&mProj);
	XMMATRIX worldViewProj = world * view * proj;
	XMStoreFloat4x4(&mObjectCB.WorldViewProj, worldViewProj);

	// 绘制.
	md3dImmediateContext->DrawIndexed(36, 0, 0);

	// 交换链.
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
	/// 创建顶点缓冲区.
	///
	std::vector<Vertex> vertices =
	{
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::White)  },
		{ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Black)   },
		{ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Red)     },
		{ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green)   },
		{ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Blue)    },
		{ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Yellow)  },
		{ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Cyan)    },
		{ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Magenta) }
	};

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex) * vertices.size();
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.StructureByteStride = 0;
	vbd.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA vInitData;
	vInitData.pSysMem = vertices.data();
	HR(md3dDevice->CreateBuffer(&vbd, &vInitData, mVertexBuffer.GetAddressOf()));

	///
	/// 创建索引缓冲区.
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
	/// 创建常量缓冲区.
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
	/// 编译着色器, 然后创建顶点着色器和像素着色器.
	///
	ComPtr<ID3DBlob> vertexBlob = d3dUtil::CompileShader(L"Shaders/color.hlsl", nullptr, "VS", "vs_5_0");
	HR(md3dDevice->CreateVertexShader(vertexBlob->GetBufferPointer(), vertexBlob->GetBufferSize(), nullptr, mVertexShader.GetAddressOf()));

	ComPtr<ID3DBlob> pixelBlob = d3dUtil::CompileShader(L"Shaders/color.hlsl", nullptr, "PS", "ps_5_0");
	HR(md3dDevice->CreatePixelShader(pixelBlob->GetBufferPointer(), pixelBlob->GetBufferSize(), nullptr, mPixelShader.GetAddressOf()));

	///
	/// 创建顶点的输入布局描述.
	///
	D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
		D3D11_INPUT_PER_VERTEX_DATA, 0},
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12,
		D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	HR(md3dDevice->CreateInputLayout(vertexDesc, ARRAYSIZE(vertexDesc),
		vertexBlob->GetBufferPointer(), vertexBlob->GetBufferSize(),
		mInputLayout.GetAddressOf()));
}
