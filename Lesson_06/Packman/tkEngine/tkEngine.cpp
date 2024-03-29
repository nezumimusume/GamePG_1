/*!
 * @brief	tkEngine
 */
#include "tkEngine/tkEnginePreCompile.h"
#include "tkEngine/tkEngine.h"
#include "tkEngine/gameObject/tkGameObjectManager.h"
#include "tkEngine/shape/tkShapeVertex.h"
#include "tkEngine/graphics/tkEffect.h"

namespace tkEngine{
	LRESULT CALLBACK CEngine::MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg)
		{
		case WM_DESTROY:
			Instance().Final();
			PostQuitMessage(0);
			return 0;
		}

		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	bool CEngine::InitWindow(const SInitParam& initParam)
	{
		TK_ASSERT(initParam.screenHeight != 0, "screenHeight is zero");
		TK_ASSERT(initParam.screenWidth != 0, "screenWidth is zero");
		TK_ASSERT(initParam.gameObjectPrioMax != 0, "gameObjectPrioMax is zero");
		TK_ASSERT(initParam.numRenderContext != 0, "numRenderContext is zero");
		TK_ASSERT(initParam.commandBufferSizeTbl != nullptr, "commandBufferSizeTbl is null");

		m_screenHeight = initParam.screenHeight;
		m_screenWidth = initParam.screenWidth;
		WNDCLASSEX wc =
		{
			sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L,
			GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr,
			TEXT("D3D Tutorial"), nullptr
		};
		RegisterClassEx(&wc);
		// Create the application's window
		m_hWnd = CreateWindow(TEXT("D3D Tutorial"), TEXT("D3D Tutorial 06: Meshes"),
			WS_OVERLAPPEDWINDOW, 0, 0, m_screenWidth, m_screenHeight,
			nullptr, nullptr, wc.hInstance, nullptr);

		return m_hWnd != nullptr;
	}
	bool CEngine::InitDirectX(const SInitParam& initParam)
	{
		if( nullptr == ( m_pD3D = Direct3DCreate9( D3D_SDK_VERSION ) ) ){
			//D3Dオブジェクトを作成できなかった。
	        return false;
	    }
	    D3DPRESENT_PARAMETERS d3dpp;
	    ZeroMemory( &d3dpp, sizeof( d3dpp ) );
    	d3dpp.Windowed = TRUE;
	    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	    d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
	    d3dpp.EnableAutoDepthStencil = TRUE;
    	d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
		d3dpp.BackBufferWidth = initParam.frameBufferWidth;
		d3dpp.BackBufferHeight = initParam.frameBufferHeight;

		m_frameBufferWidth = initParam.frameBufferWidth;
		m_frameBufferHeight = initParam.frameBufferHeight;

    	// Create the D3DDevice
	    if( FAILED( m_pD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, m_hWnd,
	                                      D3DCREATE_SOFTWARE_VERTEXPROCESSING,
	                                      &d3dpp, &m_pD3DDevice ) ) )
	    {
	        return false;
	    }
		//バックバッファのレンダリングターゲットと深度ステンシルバッファを取得しておいて覚えておく。
		LPDIRECT3DSURFACE9 rt, depth;
		m_pD3DDevice->GetRenderTarget(0, &rt);
		m_pD3DDevice->GetDepthStencilSurface(&depth);
		m_backBufferRT.SetSurfaceDX(rt);
		m_backBufferRT.SetDepthSurfaceDX(depth);
	    return true;
	    
	}
	bool CEngine::Init(const SInitParam& initParam)
	{
		//ウィンドウ初期化。
		if (!InitWindow(initParam)) {
			return false;
		}
		//DirectX初期化。
		if (!InitDirectX(initParam)) {
			return false;
		}
		//メインレンダリングターゲットを作成。
		for (int i = 0; i < 2; i++) {
			m_mainRenderTarget[0].Create(
				m_frameBufferWidth,
				m_frameBufferHeight,
				1,
				FMT_A8R8G8B8,
				FMT_D16,
				MULTISAMPLE_NONE,
				0
			);
		}

		CGameObjectManager::Instance().Init( initParam.gameObjectPrioMax );
		InitCopyBackBufferPrimitive();
		//レンダリングコンテキストの初期化。
		{
			m_renderContextArray.reset(new CRenderContext[initParam.numRenderContext]);
			for (int i = 0; i < initParam.numRenderContext; i++) {
				m_renderContextArray[i].Init(m_pD3DDevice, initParam.commandBufferSizeTbl[i]);
			}
			m_numRenderContext = initParam.numRenderContext;
			if (m_numRenderContext > 1) {
				TK_ASSERT(initParam.renderContextMap != nullptr, "renderContextMap is nullptr!!!");
				m_renderContextMap.reset( new SRenderContextMap[m_numRenderContext]);
				memcpy(m_renderContextMap.get(), initParam.renderContextMap, sizeof(SRenderContextMap) * m_numRenderContext);
			}
		}
		//トランスフォーム済みプリミティブを描画するシェーダーをロード。
		m_pTransformedPrimEffect = m_effectManager.LoadEffect("Assets/presetShader/TransformedPrim.fx");
		//プリレンダリングを作成。
		m_preRender.Create( initParam.graphicsConfig );
		//ポストエフェクトをレンダリング。
		m_postEffect.Create( initParam.graphicsConfig );
		
		ShowWindow(m_hWnd, SW_SHOWDEFAULT);
		UpdateWindow(m_hWnd);
		return true;
	}
	void CEngine::InitCopyBackBufferPrimitive()
	{
		static SShapeVertex_PT vertex[]{
			{
				-1.0f, 1.0f, 0.0f, 1.0f,
				0.0f, 0.0f
			},
			{
				1.0f, 1.0f, 0.0f, 1.0f,
				1.0f, 0.0f
			},
			{
				-1.0f, -1.0f, 0.0f, 1.0f,
				0.0f, 1.0f
			},
			{
				1.0f, -1.0f, 0.0f, 1.0f,
				1.0f, 1.0f
			},
		};
		static unsigned short index[] = {
			0,1,2,3
		};
		m_copyBackBufferPrim.Create(
			CPrimitive::eTriangleStrip,
			4,
			sizeof(SShapeVertex_PT),
			scShapeVertex_PT_Element,
			vertex,
			4,
			eIndexFormat16,
			index
			);
	}
	void CEngine::CopyMainRenderTargetToBackBuffer(CRenderContext& renderContext)
	{
		CRenderTarget& rt = m_mainRenderTarget[m_currentMainRenderTarget];
		m_pTransformedPrimEffect->SetTechnique(renderContext, "ColorNormalPrim");
		m_pTransformedPrimEffect->Begin(renderContext);
		m_pTransformedPrimEffect->BeginPass(renderContext, 0);
		m_pTransformedPrimEffect->SetTexture(renderContext, "g_tex", rt.GetTexture());
		renderContext.SetVertexDeclaration(m_copyBackBufferPrim.GetVertexDecl());
		renderContext.SetStreamSource(0, m_copyBackBufferPrim.GetVertexBuffer());
		renderContext.SetIndices(m_copyBackBufferPrim.GetIndexBuffer());
		renderContext.DrawIndexedPrimitive(&m_copyBackBufferPrim);
		
		m_pTransformedPrimEffect->EndPass(renderContext);
		m_pTransformedPrimEffect->End(renderContext);
	}
	void CEngine::RunGameLoop()
	{
		// Enter the message loop
		MSG msg;
		ZeroMemory(&msg, sizeof(msg));
		while (msg.message != WM_QUIT)
		{
			if (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else {
				//キー入力を更新。
				m_keyInput.Update();

				CRenderContext& topRenderContext = m_renderContextArray[0];
				CRenderContext& lastRenderContext = m_renderContextArray[m_numRenderContext - 1];
				topRenderContext.SetRenderTarget(0, &m_mainRenderTarget[m_currentMainRenderTarget]);
				
				CGameObjectManager& goMgr = CGameObjectManager::Instance();
				goMgr.Execute(
					m_renderContextArray.get(), 
					m_numRenderContext, 
					m_renderContextMap.get(),
					m_preRender,
					m_postEffect
				);
				lastRenderContext.SetRenderTarget(0, &m_backBufferRT);
				CopyMainRenderTargetToBackBuffer(lastRenderContext);

				m_pD3DDevice->BeginScene();
				//レンダリングコマンドのサブミット
				for( int i = 0; i < m_numRenderContext; i++ ){
					m_renderContextArray[i].SubmitCommandBuffer();
				}
				//
				m_pD3DDevice->EndScene();
				m_pD3DDevice->Present(nullptr, nullptr, nullptr, nullptr);
			}
		}
	}
	void CEngine::Final()
	{
		for (int i = 0; i < 2; i++) {
			m_mainRenderTarget[i].Release();
		}
		m_effectManager.Release();
		if (m_pD3DDevice != nullptr)
			m_pD3DDevice->Release();

		if (m_pD3D != nullptr)
			m_pD3D->Release();
	}
}// namespace tkEngine
