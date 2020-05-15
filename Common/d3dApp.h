#pragma once

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "dxgi.lib")

#include "d3dUtil.h"
#include "GameTimer.h"
#include <string>
#include <wrl.h>
#include <unordered_map>

class D3DApp
{
public:
	D3DApp(HINSTANCE hInstance);
	D3DApp(const D3DApp& rhs) = delete;
	D3DApp operator=(const D3DApp& rhs) = delete;
	virtual ~D3DApp();

	HINSTANCE AppInst() const;
	HWND MainWnd() const;

	// 这个方法返回摄像机的宽高比(重要).
	float AspectRatio() const;

	// 运行程序.
	int Run();

	// 绘制场景必需的一些方法, 需要在App中实现这些虚方法.(如果需要的话)
	virtual bool Init();
	virtual void OnResize();
	virtual void UpdateScene(GameTimer gt) = 0;
	virtual void DrawScene() = 0;
	virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	// 处理鼠标.
	virtual void OnMouseDown(WPARAM btnState, int x, int y) { }
	virtual void OnMouseUp(WPARAM btnState, int x, int y) { }
	virtual void OnMouseMove(WPARAM btnState, int x, int y) { }

protected:
	// 初始化环境.
	bool InitMainWindow();
	bool InitDirect3D();

	// 计算FPS, 1/FPS, 帧数等相关信息.
	void CalculateFrameStats();

	// 返回当前使用的后台缓冲区视图.
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> CurrentBackBufferView() const;

protected:
	// 环境配置必需变量.
	HINSTANCE mhAppInst;
	HWND      mhMainWnd;
	bool      mAppPaused;
	bool      mMinimized;
	bool      mMaximized;
	bool      mResizing;
	UINT      m4xMsaaQuality;

	GameTimer mTimer;

	// 使用新式的Flip交换链.
	static const int FlipBufferCount = 2;
	int mCurrentBuffer = 0;

	Microsoft::WRL::ComPtr<ID3D11Device> md3dDevice = nullptr;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> md3dImmediateContext = nullptr;
	Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain = nullptr;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> mRenderTargetView[FlipBufferCount];
	Microsoft::WRL::ComPtr<ID3D11Texture2D> mDepthStencilBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> mDepthStencilView = nullptr;
	D3D11_VIEWPORT mScreenViewport;

	// 其他信息.
	std::wstring mMainWndCaption;
	D3D_DRIVER_TYPE md3dDriverType;
	int mClientWidth;
	int mClientHeight;
	bool mEnable4xMsaa;
};
