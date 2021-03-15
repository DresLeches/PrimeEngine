#ifndef __PYENGINE_2_0_D3D11Renderer_H___
#define __PYENGINE_2_0_D3D11Renderer_H___

#define NOMINMAX
// API Abstraction
#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"
#if APIABSTRACTION_D3D11

#include "PrimeEngine/Application/WinApplication.h"

//#define D3D10_IGNORE_SDK_LAYERS

#include <D3DCommon.h>
#include <D3DCompiler.h>
#include <d3d11.h>

// Inter-Engine includes
#include "PrimeEngine/MainFunction/MainFunctionArgs.h"
#include "PrimeEngine/MemoryManagement/Handle.h"
#include "PrimeEngine/Events/EventQueueManager.h"
#include "PrimeEngine/Events/StandardEvents.h"
#include "PrimeEngine/Render/IRenderer.h"
// Sibling/Children includes

// DirectXTex includes and linking
#include "../Utils/DirectXTex/Include/DirectXTex.h"
#pragma comment(lib, "DirectXTex.lib")

// This class is implementation of IRenderer for D3D11

// forward declaration of texture classs

namespace PE {

struct TextureGPU;

#define VOXEL_RESOLUTION 64
#define GRID_RESOLUTION 4
#define ITERATIONS_BEFORE_BAKING 10000

class D3D11Renderer : public IRenderer
{
public:

	virtual void setRenderTargetsAndViewportWithDepth(TextureGPU *pDestColorTex = 0, TextureGPU *pDestDepthTex = 0, bool clearRenderTargte = false, bool clearDepth = false);

	virtual void setRenderTargetsAndViewportWithDepthOnly(TextureGPU *pDestColorTex = 0, TextureGPU *pDestDepthTex = 0, bool clearRenderTargte = false, bool clearDepth = false);

	virtual void setDepthStencilOnlyRenderTargetAndViewport(TextureGPU *pDestDepthTex, bool clear = false);

	virtual void setRenderTargetsAndViewportWithNoDepth(TextureGPU *pDestColorTex = 0, bool clear = false);

	D3D11Renderer(PE::GameContext &context, unsigned int width, unsigned int height);

	virtual void swap(bool vsync = 0) {m_pSwapChain->Present(vsync ? 1 : 0, 0);}

	virtual void ClearVoxelGrid(SamplerState *);
	virtual void SetupVoxelization(SamplerState *);
	virtual void Voxelize(SamplerState *);
	virtual void ConeTrace(SamplerState *);
	virtual void SSAO(SamplerState *);
	virtual void IndirectDiffuseLighting(SamplerState *);
	virtual void IndirectSpecularLighting(SamplerState *);
	virtual void VolumetricLighting(SamplerState *);
	virtual void FXAA(SamplerState *);
	virtual void ToneMap(SamplerState *);
	virtual void ColorGrade(SamplerState *);
	virtual void SkyBoxRender(SamplerState *);
	virtual void MotionBlur(SamplerState *);
	virtual void HUDRender(SamplerState *);

	virtual void BakeIrradianceProbes();
	virtual void LoadIrradianceProbes();

	virtual PrimitiveTypes::UInt32 getWidth(){return m_width;}
	virtual PrimitiveTypes::UInt32 getHeight(){return m_height;}
	virtual void setClearColor(Vector4 color){m_clearColor = color;}
	virtual void setVSync(bool useVsync){m_wantVSync = useVsync;}
	virtual void clear()
	{
		m_pD3DContext->ClearRenderTargetView(m_pRenderTargetView, m_clearColor.m_values);
		clearDepthAndStencil();
	}

	virtual void endRenderTarget(TextureGPU *pTex);
	virtual void endFrame();

	void clearDepthAndStencil()
	{
		m_pD3DContext->ClearDepthStencilView(m_pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	}

public:
	ID3D11ShaderResourceView* m_nullSRV[11] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	ID3D11UnorderedAccessView* m_nullUAV[6] = { 0, 0, 0, 0, 0, 0 };

public:
	IDXGISwapChain *m_pSwapChain;
	
	ID3D11Texture2D *m_pDepthStencilBuffer;
	ID3D11Texture2D *m_pSSAO;
	ID3D11Texture2D *m_pIndirectDiffuseLit;
	ID3D11Texture2D *m_pIndirectSpecularLit;
	ID3D11Texture2D *m_pVolumetricLit;
	ID3D11Texture2D *m_pFXAA;
	ID3D11Texture2D *m_pToneMap;
	ID3D11Texture2D *m_pColorGrade;
	ID3D11Texture2D *m_pSkyBoxRender;
	ID3D11Texture2D *m_pMotionBlur;
	ID3D11Texture2D *m_pHUDRender;
	ID3D11Texture2D *m_gBuffers[4];
	ID3D11Texture2D *m_injectionFlag[6];
	
	ID3D11Texture3D *m_voxelGrids[6];
	ID3D11Texture3D *m_shGrids[3];
	
	ID3D11DepthStencilView *m_pDepthStencilView;
	
	ID3D11RenderTargetView *m_pRenderTargetView;
	ID3D11RenderTargetView *m_pRenderTargetViewGBuffers[4];
	
	ID3D11UnorderedAccessView *m_pUnorderedAccessViewVoxelGrids[6];
	ID3D11UnorderedAccessView *m_pUnorderedAccessViewinjectionFlag[6];
	ID3D11UnorderedAccessView *m_pUnorderedAccessViewSHGrids[3];
	ID3D11UnorderedAccessView *m_pUnorderedAccessViewSSAO;
	ID3D11UnorderedAccessView *m_pUnorderedAccessViewIndirectDiffuseLit;
	ID3D11UnorderedAccessView *m_pUnorderedAccessViewIndirectSpecularLit;
	ID3D11UnorderedAccessView *m_pUnorderedAccessViewVolumetricLit;
	ID3D11UnorderedAccessView *m_pUnorderedAccessViewFXAA;
	ID3D11UnorderedAccessView *m_pUnorderedAccessViewToneMap;
	ID3D11UnorderedAccessView *m_pUnorderedAccessViewColorGrade;
	ID3D11UnorderedAccessView *m_pUnorderedAccessViewSkyBoxRender;
	ID3D11UnorderedAccessView *m_pUnorderedAccessViewMotionBlur;
	ID3D11UnorderedAccessView *m_pUnorderedAccessViewHUDRender;

public:
	ID3D11ShaderResourceView *m_pSRV;
	ID3D11ShaderResourceView *m_pSRVGBuffers[4];
	ID3D11ShaderResourceView *m_pSRVVoxelGrids[6];
	ID3D11ShaderResourceView *m_pSRVInjectionFlag[6];
	ID3D11ShaderResourceView *m_pSRVSHGrids[3];
	ID3D11ShaderResourceView *m_pSRVSSAO;
	ID3D11ShaderResourceView *m_pSRVIndirectDiffuseLit;
	ID3D11ShaderResourceView *m_pSRVIndirectSpecularLit;
	ID3D11ShaderResourceView *m_pSRVVolumetricLit;
	ID3D11ShaderResourceView *m_pSRVFXAA;
	ID3D11ShaderResourceView *m_pSRVToneMap;
	ID3D11ShaderResourceView *m_pSRVColorGrade;
	ID3D11ShaderResourceView *m_pSRVSkyBoxRender;
	ID3D11ShaderResourceView *m_pSRVMotionBlur;
	ID3D11ShaderResourceView *m_pSRVHUDRender;

	ID3D11Device *m_pD3DDevice;
	ID3D11DeviceContext *m_pD3DContext;
	PrimitiveTypes::UInt32 m_width, m_height;
	PrimitiveTypes::Bool m_nv3DvisionOn;
	Vector4 m_clearColor;
	bool m_wantVSync;

friend class IRenderer;
};

}; // namepsace PE

#endif // APIABstraction
#endif // File guard
