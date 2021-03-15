
#define NOMINMAX

// API Abstraction
#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"
#if APIABSTRACTION_D3D11

// Outer-Engine includes

// Inter-Engine includes
#include "PrimeEngine/APIAbstraction/Texture/Texture.h"

// Sibling/Children includes
#include "D3D11Renderer.h"

namespace PE {

D3D11Renderer::D3D11Renderer(PE::GameContext &context, unsigned int width, unsigned int height)
	:IRenderer(context, width, height)
{
	WinApplication *pWinApp = static_cast<WinApplication*>(context.getApplication());
	m_width = width;
	m_height = height;

	// get main function args
	char buf[255];
	char name[] = "PYENGINE_3DVISION_ON";
	DWORD res = 0;
	res = GetEnvironmentVariableA(
		name,
		buf, 255);

	m_nv3DvisionOn = res > 0;

	// InitD3D
	HRESULT hr;
	
	IDXGIFactory *pFactory = 0;
	hr = CreateDXGIFactory( __uuidof(IDXGIFactory), (void**)&pFactory );
	assert( SUCCEEDED(hr));

	IDXGIAdapter *pAdapter = 0;
	hr = pFactory->EnumAdapters(0, &pAdapter);
	assert(hr != DXGI_ERROR_NOT_FOUND);

	IDXGIOutput *pOutput = 0;
	hr = pAdapter->EnumOutputs(0, &pOutput);
	assert(hr != DXGI_ERROR_NOT_FOUND);

	UINT numModes = 512;
	DXGI_MODE_DESC descs[512];
	hr = pOutput->GetDisplayModeList(
		DXGI_FORMAT_R8G8B8A8_UNORM, //  [in]       DXGI_FORMAT EnumFormat,
		0, //  [in]       UINT Flags,
		&numModes, //   [in, out]  UINT *pNumModes,
		descs //  [out]      DXGI_MODE_DESC *pDesc
	);

	DXGI_MODE_DESC best = descs[0];
	PrimitiveTypes::Float32 maxRate = 0;
	for (PrimitiveTypes::UInt32 i = 0; i < numModes; i++)
	{
		if (descs[i].Width == width && descs[i].Height == height)
		{
			PrimitiveTypes::Float32 rate = (PrimitiveTypes::Float32)(descs[i].RefreshRate.Numerator) / (PrimitiveTypes::Float32)(descs[i].RefreshRate.Denominator);
			if (rate > maxRate)
			{
				best = descs[i];
			}
		}
	}

	// Fill out a DXGI_SWAP_CHAIN_DESC to describe our swap chain.
	DXGI_SWAP_CHAIN_DESC sd;
	// Back buffer descritpion
	DXGI_MODE_DESC bufferDesc;
	bufferDesc.Width  = width;
	bufferDesc.Height = height;


	//RefreshRate	{Numerator=119974 Denominator=1000 }	DXGI_RATIONAL

	bufferDesc.RefreshRate.Numerator = 119974;
	bufferDesc.RefreshRate.Denominator = 1000;

	bufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	bufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE;
	//bufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	bufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	//bufferDesc.Scaling = DXGI_MODE_SCALING_CENTERED;
	sd.BufferDesc = bufferDesc;

	// Multi-sampling properties
	DXGI_SAMPLE_DESC sampleDesc;
	// No multisampling.
	sampleDesc.Count   = 1;
	sampleDesc.Quality = 0;
	sd.SampleDesc = sampleDesc;

	// This buufer is going to be used for output only (we render into it)
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;// | DXGI_USAGE_SHADER_INPUT;

	// how many buffers to use in swap chain; 1 == double buffering, 2 == triple buffering
	sd.BufferCount = 2;

	sd.OutputWindow = pWinApp->m_windowHandle;
	// go full screen if doing 3d vision
	sd.Windowed = !m_nv3DvisionOn; //true;

	// how to swap buffers; with DXGI_SWAP_EFFECT_DISCARD we let driver to select the ost efficient way
	sd.SwapEffect   = DXGI_SWAP_EFFECT_DISCARD;

	// if DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH is specified
	// when going to full screen, a ode will be selected to best fit the description above
	// if it is not selected, then the current mode will be used (i.e. leave same resolution for monitor -> bigger for application)
	sd.Flags        = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	// End DXGI_SWAP_CHAIN_DESC sd;

	bufferDesc.ScanlineOrdering = best.ScanlineOrdering;
	bufferDesc.Scaling = best.Scaling;
		// Create the device.---------------------------------------------------

	UINT createDeviceFlags = 0;//D3D11_CREATE_DEVICE_DEBUG;
#		if defined(DEBUG) || defined(_DEBUG)  
		createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG; // this will add D3D error messages to output
#		endif

	D3D_FEATURE_LEVEL level;
	D3D_FEATURE_LEVEL levelsWanted[] = 
	{ 
		D3D_FEATURE_LEVEL_11_0, 
		D3D_FEATURE_LEVEL_10_1, 
		D3D_FEATURE_LEVEL_10_0
	};
	UINT numLevelsWanted = sizeof( levelsWanted ) / sizeof( levelsWanted[0] );

	D3D_DRIVER_TYPE driverTypes[] =
	{
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_REFERENCE,
	};
	UINT numDriverTypes = sizeof( driverTypes ) / sizeof( driverTypes[0] );

	for (PrimitiveTypes::UInt32 iDrv = 0; iDrv < numDriverTypes; iDrv++)
	{

		HRESULT hr = D3D11CreateDeviceAndSwapChain(
			0,                 //default adapter
			driverTypes[iDrv], //D3D_DRIVER_TYPE_HARDWARE, D3D_DRIVER_TYPE_REFERENCE, D3D_DRIVER_TYPE_SOFTWARE, or D3D_DRIVER_TYPE_WARP.
			0,                 // no software device
			createDeviceFlags, 
			levelsWanted, // which levels we desire in order of best first, D3D_FEATURE_LEVEL_11_0
			numLevelsWanted, // need 1
			D3D11_SDK_VERSION,
			&sd,
			&m_pSwapChain,
			&m_pD3DDevice,
			&level, // TODO: pFeatureLevel
			&m_pD3DContext
		);

		if (SUCCEEDED(hr))
		{
			OutputDebugStringA("PyEgnine2.0: Created D3D Device:");
			if (level == D3D_FEATURE_LEVEL_11_0)
			{
				OutputDebugStringA("D3D_FEATURE_LEVEL_11_0\n");
			}
			else if (level == D3D_FEATURE_LEVEL_10_1)
			{
				OutputDebugStringA("D3D_FEATURE_LEVEL_10_1\n");
			}
			else if (level == D3D_FEATURE_LEVEL_10_0)
			{
				OutputDebugStringA("D3D_FEATURE_LEVEL_10_0\n");
			}
			break;
		}
		else
		{
			if (driverTypes[iDrv] == D3D_DRIVER_TYPE_HARDWARE)
			{
				OutputDebugStringA("PyEgnine2.0: WARNING: Could not created D3D device with HW driver\n");
			}
		}
	}

	// At this point the backbuffer is created, but nothing can be drawn in it
	// we need to link it to the output merger stage i.e. create a render target view

	ID3D11Texture2D *pBackBufferResource;
	m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **)(&pBackBufferResource));
	// get pointer to texture (back buffer) handle
	hr = m_pD3DDevice->CreateRenderTargetView(pBackBufferResource, 0, &m_pRenderTargetView);
	pBackBufferResource->Release(); // releasing handle

	// Make RTV and SRV for G-Buffers
	D3D11_TEXTURE2D_DESC texDescGBuffers;
	texDescGBuffers.Width = width;
	texDescGBuffers.Height = height;
	texDescGBuffers.MipLevels = 1;
	texDescGBuffers.ArraySize = 1;
	texDescGBuffers.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	texDescGBuffers.SampleDesc.Count = 1;
	texDescGBuffers.SampleDesc.Quality = 0;
	texDescGBuffers.Usage = D3D11_USAGE_DEFAULT;
	texDescGBuffers.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	texDescGBuffers.CPUAccessFlags = 0;
	texDescGBuffers.MiscFlags = 0;

	for (int i = 0; i < 4; ++i)
	{
		HRESULT hr = m_pD3DDevice->CreateTexture2D(&texDescGBuffers, 0, &m_gBuffers[i]);
		assert(SUCCEEDED(hr));

		// Null description means to create a view to all mipmap levels
		// using the format the texture was created with
		hr = m_pD3DDevice->CreateRenderTargetView(m_gBuffers[i], 0, &m_pRenderTargetViewGBuffers[i]);
		assert(SUCCEEDED(hr));
		hr = m_pD3DDevice->CreateShaderResourceView(m_gBuffers[i], 0, &m_pSRVGBuffers[i]);
		assert(SUCCEEDED(hr));
	}

	// Make UAV and SRV for intermediate buffers
	D3D11_TEXTURE2D_DESC texDescIntermediateBuffers;
	texDescIntermediateBuffers.Width = width;
	texDescIntermediateBuffers.Height = height;
	texDescIntermediateBuffers.MipLevels = 1;
	texDescIntermediateBuffers.ArraySize = 1;
	texDescIntermediateBuffers.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	texDescIntermediateBuffers.SampleDesc.Count = 1;
	texDescIntermediateBuffers.SampleDesc.Quality = 0;
	texDescIntermediateBuffers.Usage = D3D11_USAGE_DEFAULT;
	texDescIntermediateBuffers.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	texDescIntermediateBuffers.CPUAccessFlags = 0;
	texDescIntermediateBuffers.MiscFlags = 0;
	
	{
		// Create the SSAO texture, it's UAV and SRV
		HRESULT hr = m_pD3DDevice->CreateTexture2D(&texDescIntermediateBuffers, 0, &m_pSSAO);
		assert(SUCCEEDED(hr));

		hr = m_pD3DDevice->CreateUnorderedAccessView(m_pSSAO, 0, &m_pUnorderedAccessViewSSAO);
		assert(SUCCEEDED(hr));

		hr = m_pD3DDevice->CreateShaderResourceView(m_pSSAO, 0, &m_pSRVSSAO);
		assert(SUCCEEDED(hr));

		// Create the indirect diffuse texture, it's UAV and SRV
		hr = m_pD3DDevice->CreateTexture2D(&texDescIntermediateBuffers, 0, &m_pIndirectDiffuseLit);
		assert(SUCCEEDED(hr));

		hr = m_pD3DDevice->CreateUnorderedAccessView(m_pIndirectDiffuseLit, 0, &m_pUnorderedAccessViewIndirectDiffuseLit);
		assert(SUCCEEDED(hr));

		hr = m_pD3DDevice->CreateShaderResourceView(m_pIndirectDiffuseLit, 0, &m_pSRVIndirectDiffuseLit);
		assert(SUCCEEDED(hr));

		// Create the indirect specular texture, it's UAV and SRV
		hr = m_pD3DDevice->CreateTexture2D(&texDescIntermediateBuffers, 0, &m_pIndirectSpecularLit);
		assert(SUCCEEDED(hr));

		hr = m_pD3DDevice->CreateUnorderedAccessView(m_pIndirectSpecularLit, 0, &m_pUnorderedAccessViewIndirectSpecularLit);
		assert(SUCCEEDED(hr));

		hr = m_pD3DDevice->CreateShaderResourceView(m_pIndirectSpecularLit, 0, &m_pSRVIndirectSpecularLit);
		assert(SUCCEEDED(hr));

		// Create the volumetric lighting texture, it's UAV and SRV
		hr = m_pD3DDevice->CreateTexture2D(&texDescIntermediateBuffers, 0, &m_pVolumetricLit);
		assert(SUCCEEDED(hr));

		hr = m_pD3DDevice->CreateUnorderedAccessView(m_pVolumetricLit, 0, &m_pUnorderedAccessViewVolumetricLit);
		assert(SUCCEEDED(hr));

		hr = m_pD3DDevice->CreateShaderResourceView(m_pVolumetricLit, 0, &m_pSRVVolumetricLit);
		assert(SUCCEEDED(hr));

		// Create the fxaa texture, it's UAV and SRV
		hr = m_pD3DDevice->CreateTexture2D(&texDescIntermediateBuffers, 0, &m_pFXAA);
		assert(SUCCEEDED(hr));

		hr = m_pD3DDevice->CreateUnorderedAccessView(m_pFXAA, 0, &m_pUnorderedAccessViewFXAA);
		assert(SUCCEEDED(hr));

		hr = m_pD3DDevice->CreateShaderResourceView(m_pFXAA, 0, &m_pSRVFXAA);
		assert(SUCCEEDED(hr));

		// Create the tone mapped texture, it's UAV and SRV
		hr = m_pD3DDevice->CreateTexture2D(&texDescIntermediateBuffers, 0, &m_pToneMap);
		assert(SUCCEEDED(hr));

		hr = m_pD3DDevice->CreateUnorderedAccessView(m_pToneMap, 0, &m_pUnorderedAccessViewToneMap);
		assert(SUCCEEDED(hr));

		hr = m_pD3DDevice->CreateShaderResourceView(m_pToneMap, 0, &m_pSRVToneMap);
		assert(SUCCEEDED(hr));

		// Create the color graded texture, it's UAV and SRV
		hr = m_pD3DDevice->CreateTexture2D(&texDescIntermediateBuffers, 0, &m_pColorGrade);
		assert(SUCCEEDED(hr));

		hr = m_pD3DDevice->CreateUnorderedAccessView(m_pColorGrade, 0, &m_pUnorderedAccessViewColorGrade);
		assert(SUCCEEDED(hr));

		hr = m_pD3DDevice->CreateShaderResourceView(m_pColorGrade, 0, &m_pSRVColorGrade);
		assert(SUCCEEDED(hr));

		// Create the skybox render texture, it's UAV and SRV
		hr = m_pD3DDevice->CreateTexture2D(&texDescIntermediateBuffers, 0, &m_pSkyBoxRender);
		assert(SUCCEEDED(hr));

		hr = m_pD3DDevice->CreateUnorderedAccessView(m_pSkyBoxRender, 0, &m_pUnorderedAccessViewSkyBoxRender);
		assert(SUCCEEDED(hr));

		hr = m_pD3DDevice->CreateShaderResourceView(m_pSkyBoxRender, 0, &m_pSRVSkyBoxRender);
		assert(SUCCEEDED(hr));

		// Create the Motion Blur texture, it's UAV and SRV
		hr = m_pD3DDevice->CreateTexture2D(&texDescIntermediateBuffers, 0, &m_pMotionBlur);
		assert(SUCCEEDED(hr));

		hr = m_pD3DDevice->CreateUnorderedAccessView(m_pMotionBlur, 0, &m_pUnorderedAccessViewMotionBlur);
		assert(SUCCEEDED(hr));

		hr = m_pD3DDevice->CreateShaderResourceView(m_pMotionBlur, 0, &m_pSRVMotionBlur);
		assert(SUCCEEDED(hr));

		// Create the HUD render texture, it's UAV and SRV
		hr = m_pD3DDevice->CreateTexture2D(&texDescIntermediateBuffers, 0, &m_pHUDRender);
		assert(SUCCEEDED(hr));

		hr = m_pD3DDevice->CreateUnorderedAccessView(m_pHUDRender, 0, &m_pUnorderedAccessViewHUDRender);
		assert(SUCCEEDED(hr));

		hr = m_pD3DDevice->CreateShaderResourceView(m_pHUDRender, 0, &m_pSRVHUDRender);
		assert(SUCCEEDED(hr));
	}

	if (m_lightMapping)
	{
		// Make UAV and SRV for Injection Flag Texture
		D3D11_TEXTURE2D_DESC texDescInjectionFlag;
		texDescInjectionFlag.Width = width;
		texDescInjectionFlag.Height = height;
		texDescInjectionFlag.MipLevels = 1;
		texDescInjectionFlag.ArraySize = 1;
		texDescInjectionFlag.Format = DXGI_FORMAT_R8_UNORM;
		texDescInjectionFlag.SampleDesc.Count = 1;
		texDescInjectionFlag.SampleDesc.Quality = 0;
		texDescInjectionFlag.Usage = D3D11_USAGE_DEFAULT;
		texDescInjectionFlag.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
		texDescInjectionFlag.CPUAccessFlags = 0;
		texDescInjectionFlag.MiscFlags = 0;

		for (int i = 0; i < 6; ++i)
		{
			HRESULT hr = m_pD3DDevice->CreateTexture2D(&texDescInjectionFlag, 0, &m_injectionFlag[i]);
			assert(SUCCEEDED(hr));

			// Null description means to create a view to all mipmap levels
			// using the format the texture was created with
			hr = m_pD3DDevice->CreateUnorderedAccessView(m_injectionFlag[i], 0, &m_pUnorderedAccessViewinjectionFlag[i]);
			assert(SUCCEEDED(hr));
			hr = m_pD3DDevice->CreateShaderResourceView(m_injectionFlag[i], 0, &m_pSRVInjectionFlag[i]);
			assert(SUCCEEDED(hr));
		}

		// Make UAV and SRV for voxel grids
		D3D11_TEXTURE3D_DESC texDescVoxelGrids;

		texDescVoxelGrids.MipLevels = 1;
		texDescVoxelGrids.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		texDescVoxelGrids.Usage = D3D11_USAGE_DEFAULT;
		texDescVoxelGrids.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
		texDescVoxelGrids.CPUAccessFlags = 0;
		texDescVoxelGrids.MiscFlags = 0;

		for (int i = 0; i < 6; ++i)
		{
			texDescVoxelGrids.Width = (VOXEL_RESOLUTION >> i);
			texDescVoxelGrids.Height = (VOXEL_RESOLUTION >> i);
			texDescVoxelGrids.Depth = (VOXEL_RESOLUTION >> i);

			HRESULT hr = m_pD3DDevice->CreateTexture3D(&texDescVoxelGrids, 0, &m_voxelGrids[i]);
			assert(SUCCEEDED(hr));

			// Null description means to create a view to all mipmap levels
			// using the format the texture was created with
			hr = m_pD3DDevice->CreateUnorderedAccessView(m_voxelGrids[i], 0, &m_pUnorderedAccessViewVoxelGrids[i]);
			assert(SUCCEEDED(hr));
			hr = m_pD3DDevice->CreateShaderResourceView(m_voxelGrids[i], 0, &m_pSRVVoxelGrids[i]);
			assert(SUCCEEDED(hr));
		}

		// Make UAV and SRV for SH grids
		D3D11_TEXTURE3D_DESC texDescSHGrids;

		texDescSHGrids.MipLevels = 1;
		texDescSHGrids.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		texDescSHGrids.Usage = D3D11_USAGE_DEFAULT;
		texDescSHGrids.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
		texDescSHGrids.CPUAccessFlags = 0;
		texDescSHGrids.Width = GRID_RESOLUTION;
		texDescSHGrids.Height = GRID_RESOLUTION;
		texDescSHGrids.Depth = GRID_RESOLUTION;
		texDescSHGrids.MiscFlags = 0;

		for (int i = 0; i < 3; ++i)
		{
			HRESULT hr = m_pD3DDevice->CreateTexture3D(&texDescSHGrids, 0, &m_shGrids[i]);
			assert(SUCCEEDED(hr));

			// Null description means to create a view to all mipmap levels
			// using the format the texture was created with
			hr = m_pD3DDevice->CreateUnorderedAccessView(m_shGrids[i], 0, &m_pUnorderedAccessViewSHGrids[i]);
			assert(SUCCEEDED(hr));
			hr = m_pD3DDevice->CreateShaderResourceView(m_shGrids[i], 0, &m_pSRVSHGrids[i]);
			assert(SUCCEEDED(hr));
		}
	}
	else
	{
		LoadIrradianceProbes();
	}

	//hr = m_pD3DDevice->CreateShaderResourceView(pBackBufferResource, 0, &m_pSRV);
	//assert(SUCCEEDED(hr));


	D3D11_TEXTURE2D_DESC depthStencilDesc;
		depthStencilDesc.Width = width;
		depthStencilDesc.Height = height;
		
		depthStencilDesc.MipLevels = 1;
		depthStencilDesc.ArraySize = 1;
		depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;

		// Multi-sampling properties
		DXGI_SAMPLE_DESC sampleDesc2;
			// No multisampling.
			sampleDesc2.Count   = 1;
			sampleDesc2.Quality = 0;
		depthStencilDesc.SampleDesc = sampleDesc2;

		depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
		depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		depthStencilDesc.CPUAccessFlags = 0;
		depthStencilDesc.MiscFlags = 0;

	m_pD3DDevice->CreateTexture2D(
		&depthStencilDesc, 0, &m_pDepthStencilBuffer);

	m_pD3DDevice->CreateDepthStencilView(
		m_pDepthStencilBuffer, 0, &m_pDepthStencilView);

	setRenderTargetsAndViewportWithDepth();
}

void D3D11Renderer::BakeIrradianceProbes()
{
	DirectX::ScratchImage result0, result1, result2;

	DirectX::CaptureTexture(m_pD3DDevice, m_pD3DContext, (ID3D11Resource *)(m_shGrids[0]), result0);
	DirectX::SaveToDDSFile(result0.GetImages(), result0.GetImageCount(), result0.GetMetadata(), DirectX::DDS_FLAGS_NONE, L"AssetsOut/BakedLightingData/SHGridRed.dds");
	
	DirectX::CaptureTexture(m_pD3DDevice, m_pD3DContext, (ID3D11Resource *)(m_shGrids[1]), result1);
	DirectX::SaveToDDSFile(result1.GetImages(), result1.GetImageCount(), result1.GetMetadata(), DirectX::DDS_FLAGS_NONE, L"AssetsOut/BakedLightingData/SHGridGreen.dds");

	DirectX::CaptureTexture(m_pD3DDevice, m_pD3DContext, (ID3D11Resource *)(m_shGrids[2]), result2);
	DirectX::SaveToDDSFile(result2.GetImages(), result2.GetImageCount(), result2.GetMetadata(), DirectX::DDS_FLAGS_NONE, L"AssetsOut/BakedLightingData/SHGridBlue.dds");
}

void D3D11Renderer::LoadIrradianceProbes()
{
	DirectX::ScratchImage grid[3];

	DirectX::LoadFromDDSFile(L"AssetsOut/BakedLightingData/SHGridRed.dds", DirectX::DDS_FLAGS_NONE, nullptr, grid[0]);
	DirectX::CreateTexture(m_pD3DDevice, grid[0].GetImages(), grid[0].GetImageCount(), grid[0].GetMetadata(), (ID3D11Resource **)(&m_shGrids[0]));
	m_pD3DDevice->CreateShaderResourceView(m_shGrids[0], 0, &m_pSRVSHGrids[0]);

	DirectX::LoadFromDDSFile(L"AssetsOut/BakedLightingData/SHGridGreen.dds", DirectX::DDS_FLAGS_NONE, nullptr, grid[1]);
	DirectX::CreateTexture(m_pD3DDevice, grid[1].GetImages(), grid[1].GetImageCount(), grid[1].GetMetadata(), (ID3D11Resource **)(&m_shGrids[1]));
	m_pD3DDevice->CreateShaderResourceView(m_shGrids[1], 0, &m_pSRVSHGrids[1]);

	DirectX::LoadFromDDSFile(L"AssetsOut/BakedLightingData/SHGridBlue.dds", DirectX::DDS_FLAGS_NONE, nullptr, grid[2]);
	DirectX::CreateTexture(m_pD3DDevice, grid[2].GetImages(), grid[2].GetImageCount(), grid[2].GetMetadata(), (ID3D11Resource **)(&m_shGrids[2]));
	m_pD3DDevice->CreateShaderResourceView(m_shGrids[2], 0, &m_pSRVSHGrids[2]);
}

void D3D11Renderer::ClearVoxelGrid(SamplerState * ss)
{
	if (!m_lightMapping)
		return;

	UINT appendConsumeFlag = -1;

	// Set the G-Buffer SRVs
	m_pD3DContext->CSSetShaderResources(0, 1, &m_pSRVGBuffers[1]);
	
	// Set the Voxel grid flag UAVs
	m_pD3DContext->CSSetUnorderedAccessViews(0, 6, m_pUnorderedAccessViewVoxelGrids, &appendConsumeFlag);

	// Set the samplers
	m_pD3DContext->CSSetSamplers(0, 1, &(ss->m_pd3dSamplerState));

	// Compute injection flags for the voxel grids
	m_pD3DContext->Dispatch((m_width / 16), (m_height / 16), 1);

	// Perform house-keeping using null SRV and UAV
	m_pD3DContext->CSSetShaderResources(0, 1, m_nullSRV);
	m_pD3DContext->CSSetUnorderedAccessViews(0, 6, m_nullUAV, &appendConsumeFlag);
}

void D3D11Renderer::SetupVoxelization(SamplerState * ss)
{
	if (!m_lightMapping)
		return;

	UINT appendConsumeFlag = -1;

	// Set the G-Buffer SRVs
	m_pD3DContext->CSSetShaderResources(0, 2, m_pSRVGBuffers);
	// Set the Voxel grid SRVs
	m_pD3DContext->CSSetShaderResources(2, 6, m_pSRVVoxelGrids);
	
	// Set the Voxel grid flag UAVs
	m_pD3DContext->CSSetUnorderedAccessViews(0, 6, m_pUnorderedAccessViewinjectionFlag, &appendConsumeFlag);

	// Set the samplers
	m_pD3DContext->CSSetSamplers(0, 1, &(ss->m_pd3dSamplerState));

	// Compute injection flags for the voxel grids
	m_pD3DContext->Dispatch((m_width / 16), (m_height / 16), 1);

	// Perform house-keeping using null SRV and UAV
	m_pD3DContext->CSSetShaderResources(0, 8, m_nullSRV);
	m_pD3DContext->CSSetUnorderedAccessViews(0, 6, m_nullUAV, &appendConsumeFlag);
}

void D3D11Renderer::Voxelize(SamplerState * ss)
{
	if (!m_lightMapping)
		return;

	UINT appendConsumeFlag = -1;

	// Set the G-Buffer SRVs
	m_pD3DContext->CSSetShaderResources(0, 2, m_pSRVGBuffers);
	// Set the Voxel grid SRVs
	m_pD3DContext->CSSetShaderResources(2, 6, m_pSRVInjectionFlag);

	// Set the Voxel grid flag UAVs
	m_pD3DContext->CSSetUnorderedAccessViews(0, 6, m_pUnorderedAccessViewVoxelGrids, &appendConsumeFlag);

	// Set the samplers
	m_pD3DContext->CSSetSamplers(0, 1, &(ss->m_pd3dSamplerState));

	// Compute injection flags for the voxel grids
	m_pD3DContext->Dispatch((m_width / 16), (m_height / 16), 1);

	// Perform house-keeping using null SRV and UAV
	m_pD3DContext->CSSetShaderResources(0, 8, m_nullSRV);
	m_pD3DContext->CSSetUnorderedAccessViews(0, 6, m_nullUAV, &appendConsumeFlag);
}

void D3D11Renderer::ConeTrace(SamplerState * ss)
{
	if (!m_lightMapping)
		return;

	UINT appendConsumeFlag = -1;

	// Set the Voxel grid SRVs
	m_pD3DContext->CSSetShaderResources(0, 6, m_pSRVVoxelGrids);

	// Set the SH grid UAVs
	m_pD3DContext->CSSetUnorderedAccessViews(0, 3, m_pUnorderedAccessViewSHGrids, &appendConsumeFlag);

	// Set the samplers
	m_pD3DContext->CSSetSamplers(0, 1, &(ss->m_pd3dSamplerState));

	// Compute injection flags for the voxel grids
	m_pD3DContext->Dispatch((GRID_RESOLUTION / 4), (GRID_RESOLUTION / 4), (GRID_RESOLUTION / 4));

	// Perform house-keeping using null SRV and UAV
	m_pD3DContext->CSSetShaderResources(0, 6, m_nullSRV);
	m_pD3DContext->CSSetUnorderedAccessViews(0, 3, m_nullUAV, &appendConsumeFlag);

	static int iterations = 0;
	static bool baked = false;

	if (iterations > ITERATIONS_BEFORE_BAKING && !baked)
	{
		BakeIrradianceProbes();
		baked = true;
	}

	iterations++;
}

void D3D11Renderer::SSAO(SamplerState * ss)
{
	if (m_lowGraphicsMode)
		return;

	UINT appendConsumeFlag = -1;

	// Set the G-Buffer SRVs
	m_pD3DContext->CSSetShaderResources(0, 2, &m_pSRVGBuffers[1]);

	// Set the SSAO UAVs
	m_pD3DContext->CSSetUnorderedAccessViews(0, 1, &m_pUnorderedAccessViewSSAO, &appendConsumeFlag);

	// Set the samplers
	m_pD3DContext->CSSetSamplers(0, 1, &(ss->m_pd3dSamplerState));

	// Compute SSAO
	m_pD3DContext->Dispatch((m_width / 16), (m_height / 16), 1);

	// Perform house-keeping using null SRV and UAV
	m_pD3DContext->CSSetShaderResources(0, 3, m_nullSRV);
	m_pD3DContext->CSSetUnorderedAccessViews(0, 1, m_nullUAV, &appendConsumeFlag);
}

void D3D11Renderer::IndirectDiffuseLighting(SamplerState * ss)
{
	if (m_lowGraphicsMode)
		return;

	UINT appendConsumeFlag = -1;

	// Set the G-Buffer SRVs
	m_pD3DContext->CSSetShaderResources(0, 4, m_pSRVGBuffers);

	// Set the SH grids SRVs
	m_pD3DContext->CSSetShaderResources(4, 3, m_pSRVSHGrids);

	// Set the SSAO SRV
	m_pD3DContext->CSSetShaderResources(7, 1, &m_pSRVSSAO);

	// Set the indirect lit UAVs
	m_pD3DContext->CSSetUnorderedAccessViews(0, 1, &m_pUnorderedAccessViewIndirectDiffuseLit, &appendConsumeFlag);

	// Set the samplers
	m_pD3DContext->CSSetSamplers(0, 1, &(ss->m_pd3dSamplerState));

	// Compute indirect lighting
	m_pD3DContext->Dispatch((m_width / 16), (m_height / 16), 1);

	// Perform house-keeping using null SRV and UAV
	m_pD3DContext->CSSetShaderResources(0, 7, m_nullSRV);
	m_pD3DContext->CSSetUnorderedAccessViews(0, 1, m_nullUAV, &appendConsumeFlag);
}

void D3D11Renderer::IndirectSpecularLighting(SamplerState * ss)
{
	if (m_lowGraphicsMode)
		return;

	UINT appendConsumeFlag = -1;

	// Set the input texture
	m_pD3DContext->CSSetShaderResources(0, 1, &m_pSRVIndirectDiffuseLit);

	// Set the G-Buffer SRVs
	m_pD3DContext->CSSetShaderResources(1, 4, m_pSRVGBuffers);

	// Set the indirect lit UAVs
	m_pD3DContext->CSSetUnorderedAccessViews(0, 1, &m_pUnorderedAccessViewIndirectSpecularLit, &appendConsumeFlag);

	// Set the samplers
	m_pD3DContext->CSSetSamplers(0, 1, &(ss->m_pd3dSamplerState));

	// Compute indirect lighting
	m_pD3DContext->Dispatch((m_width / 16), (m_height / 16), 1);

	// Perform house-keeping using null SRV and UAV
	m_pD3DContext->CSSetShaderResources(0, 11, m_nullSRV);
	m_pD3DContext->CSSetUnorderedAccessViews(0, 1, m_nullUAV, &appendConsumeFlag);
}

void D3D11Renderer::VolumetricLighting(SamplerState * ss)
{
	if (m_lowGraphicsMode)
		return;

	UINT appendConsumeFlag = -1;

	// Set the input texture
	m_pD3DContext->CSSetShaderResources(0, 1, &m_pSRVIndirectSpecularLit);

	// Set the G-Buffer SRVs
	m_pD3DContext->CSSetShaderResources(1, 2, m_pSRVGBuffers);

	// Set the indirect lit UAVs
	m_pD3DContext->CSSetUnorderedAccessViews(0, 1, &m_pUnorderedAccessViewVolumetricLit, &appendConsumeFlag);

	// Set the samplers
	m_pD3DContext->CSSetSamplers(0, 1, &(ss->m_pd3dSamplerState));

	// Compute indirect lighting
	m_pD3DContext->Dispatch((m_width / 16), (m_height / 16), 1);

	// Perform house-keeping using null SRV and UAV
	m_pD3DContext->CSSetShaderResources(0, 11, m_nullSRV);
	m_pD3DContext->CSSetUnorderedAccessViews(0, 1, m_nullUAV, &appendConsumeFlag);
}

void D3D11Renderer::SkyBoxRender(SamplerState * ss)
{
	UINT appendConsumeFlag = -1;

	// Set the input texture
	if (m_lowGraphicsMode)
		m_pD3DContext->CSSetShaderResources(0, 1, &m_pSRVGBuffers[0]);
	else
		m_pD3DContext->CSSetShaderResources(0, 1, &m_pSRVVolumetricLit);

	m_pD3DContext->CSSetShaderResources(1, 1, &m_pSRVGBuffers[1]);

	// Set the skybox rendered UAV
	m_pD3DContext->CSSetUnorderedAccessViews(0, 1, &m_pUnorderedAccessViewSkyBoxRender, &appendConsumeFlag);

	// Set the samplers
	m_pD3DContext->CSSetSamplers(0, 1, &(ss->m_pd3dSamplerState));

	// Perform skybox render pass
	m_pD3DContext->Dispatch((m_width / 16), (m_height / 16), 1);

	// Perform house-keeping using null SRV and UAV
	m_pD3DContext->CSSetShaderResources(0, 8, m_nullSRV);
	m_pD3DContext->CSSetUnorderedAccessViews(0, 1, m_nullUAV, &appendConsumeFlag);
}

void D3D11Renderer::FXAA(SamplerState * ss)
{
	if (m_lowGraphicsMode)
		return;

	UINT appendConsumeFlag = -1;

	// Set the input texture
	m_pD3DContext->CSSetShaderResources(0, 1, &m_pSRVSkyBoxRender);

	// Set the tone mapped UAV
	m_pD3DContext->CSSetUnorderedAccessViews(0, 1, &m_pUnorderedAccessViewFXAA, &appendConsumeFlag);

	// Set the samplers
	m_pD3DContext->CSSetSamplers(0, 1, &(ss->m_pd3dSamplerState));

	// Perform tone mapping
	m_pD3DContext->Dispatch((m_width / 16), (m_height / 16), 1);

	// Perform house-keeping using null SRV and UAV
	m_pD3DContext->CSSetShaderResources(0, 1, m_nullSRV);
	m_pD3DContext->CSSetUnorderedAccessViews(0, 1, m_nullUAV, &appendConsumeFlag);
}

void D3D11Renderer::ToneMap(SamplerState * ss)
{
	UINT appendConsumeFlag = -1;

	// Set the input texture
	if (m_lowGraphicsMode)
		m_pD3DContext->CSSetShaderResources(0, 1, &m_pSRVSkyBoxRender);
	else
		m_pD3DContext->CSSetShaderResources(0, 1, &m_pSRVFXAA);

	// Set the tone mapped UAV
	m_pD3DContext->CSSetUnorderedAccessViews(0, 1, &m_pUnorderedAccessViewToneMap, &appendConsumeFlag);

	// Set the samplers
	m_pD3DContext->CSSetSamplers(0, 1, &(ss->m_pd3dSamplerState));

	// Perform tone mapping
	m_pD3DContext->Dispatch((m_width / 16), (m_height / 16), 1);

	// Perform house-keeping using null SRV and UAV
	m_pD3DContext->CSSetShaderResources(0, 1, m_nullSRV);
	m_pD3DContext->CSSetUnorderedAccessViews(0, 1, m_nullUAV, &appendConsumeFlag);
}

void D3D11Renderer::ColorGrade(SamplerState * ss)
{
	if (m_lowGraphicsMode)
		return;

	UINT appendConsumeFlag = -1;

	// Set the input texture
	m_pD3DContext->CSSetShaderResources(0, 1, &m_pSRVToneMap);

	// Set the color graded UAV
	m_pD3DContext->CSSetUnorderedAccessViews(0, 1, &m_pUnorderedAccessViewColorGrade, &appendConsumeFlag);

	// Set the samplers
	m_pD3DContext->CSSetSamplers(0, 1, &(ss->m_pd3dSamplerState));

	// Perform color grading
	m_pD3DContext->Dispatch((m_width / 16), (m_height / 16), 1);

	// Perform house-keeping using null SRV and UAV
	m_pD3DContext->CSSetShaderResources(0, 2, m_nullSRV);
	m_pD3DContext->CSSetUnorderedAccessViews(0, 1, m_nullUAV, &appendConsumeFlag);
}

void D3D11Renderer::MotionBlur(SamplerState * ss)
{
	if (m_lowGraphicsMode)
		return;

	UINT appendConsumeFlag = -1;

	// Set the input texture
	m_pD3DContext->CSSetShaderResources(0, 1, &m_pSRVColorGrade);

	// Set the positionDepth and normal textures
	m_pD3DContext->CSSetShaderResources(1, 2, &m_pSRVGBuffers[1]);

	// Set the HUD render UAV
	m_pD3DContext->CSSetUnorderedAccessViews(0, 1, &m_pUnorderedAccessViewMotionBlur, &appendConsumeFlag);

	// Set the samplers
	m_pD3DContext->CSSetSamplers(0, 1, &(ss->m_pd3dSamplerState));

	// Perform HUD rendering
	m_pD3DContext->Dispatch((m_width / 16), (m_height / 16), 1);

	// Perform house-keeping using null SRV and UAV
	m_pD3DContext->CSSetShaderResources(0, 2, m_nullSRV);
	m_pD3DContext->CSSetUnorderedAccessViews(0, 1, m_nullUAV, &appendConsumeFlag);
}

void D3D11Renderer::HUDRender(SamplerState * ss)
{
	UINT appendConsumeFlag = -1;

	// Set the input texture
	if (m_lowGraphicsMode)
		m_pD3DContext->CSSetShaderResources(0, 1, &m_pSRVToneMap);
	else
		m_pD3DContext->CSSetShaderResources(0, 1, &m_pSRVMotionBlur);

	// Set the HUD render UAV
	m_pD3DContext->CSSetUnorderedAccessViews(0, 1, &m_pUnorderedAccessViewHUDRender, &appendConsumeFlag);

	// Set the samplers
	m_pD3DContext->CSSetSamplers(0, 1, &(ss->m_pd3dSamplerState));

	// Perform HUD rendering
	m_pD3DContext->Dispatch((m_width / 16), (m_height / 16), 1);

	// Perform house-keeping using null SRV and UAV
	m_pD3DContext->CSSetShaderResources(0, 2, m_nullSRV);
	m_pD3DContext->CSSetUnorderedAccessViews(0, 1, m_nullUAV, &appendConsumeFlag);
}

void D3D11Renderer::setRenderTargetsAndViewportWithDepth(TextureGPU *pDestColorTex, TextureGPU *pDestDepthTex, bool clearRenderTarget, bool clearDepth)
{
	ID3D11RenderTargetView *renderTargets[5] = {0, 0, 0};
	if (pDestColorTex != 0)
	{
		renderTargets[0] = pDestColorTex->m_pRenderTargetView;
		for (int i = 0; i < 4; ++i)
			renderTargets[i + 1] = m_pRenderTargetViewGBuffers[i];
	}
	else
	{
		renderTargets[0] = m_pRenderTargetView;
		for (int i = 0; i < 4; ++i)
			renderTargets[i + 1] = m_pRenderTargetViewGBuffers[i];
	}

	m_pD3DContext->OMSetRenderTargets(5, renderTargets, pDestDepthTex ? pDestDepthTex->m_DepthStencilView : m_pDepthStencilView);
	static float x = 0;
#if 0
	x = fmodf(x + 0.01, 1.0f);
#endif
	if (pDestColorTex != 0)
	{
		m_pD3DContext->RSSetViewports(1, &pDestColorTex->m_viewport);
		
		float color[] = {0.1f+x, 0.1f, 0.1f, 1.0f};

		if (clearRenderTarget)
		{
			m_pD3DContext->ClearRenderTargetView(pDestColorTex->m_pRenderTargetView, color);

			for (int i = 0; i < 4; ++i)
			{
				m_pD3DContext->ClearRenderTargetView(m_pRenderTargetViewGBuffers[i], color);
			}
		}
		if (clearDepth)
			m_pD3DContext->ClearDepthStencilView(pDestDepthTex->m_DepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	}
		else
	{
		D3D11_VIEWPORT vp;
			vp.TopLeftX = 0;
			vp.TopLeftY = 0;
			vp.Width = (float)(m_width);
			vp.Height = (float)(m_height);
			vp.MinDepth = 0.0f;
			vp.MaxDepth = 1.0f;

		m_pD3DContext->RSSetViewports(1, &vp);
		float color[] = {0.0f+x, 0.0f, 0.0f, 1.0f};
		if (clearRenderTarget)
		{
			m_pD3DContext->ClearRenderTargetView(m_pRenderTargetView, color);

			for (int i = 0; i < 4; ++i)
			{
				m_pD3DContext->ClearRenderTargetView(m_pRenderTargetViewGBuffers[i], color);
			}
		}
		if (clearDepth)
			m_pD3DContext->ClearDepthStencilView(m_pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	}
}

void D3D11Renderer::setRenderTargetsAndViewportWithDepthOnly(TextureGPU *pDestColorTex, TextureGPU *pDestDepthTex, bool clearRenderTarget, bool clearDepth)
{
	m_pD3DContext->OMSetRenderTargets(0, NULL, pDestDepthTex ? pDestDepthTex->m_DepthStencilView : m_pDepthStencilView);
	
	if (pDestColorTex != 0)
	{
		m_pD3DContext->RSSetViewports(1, &pDestColorTex->m_viewport);

		if (clearDepth)
			m_pD3DContext->ClearDepthStencilView(pDestDepthTex->m_DepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	}
	else
	{
		D3D11_VIEWPORT vp;
		vp.TopLeftX = 0;
		vp.TopLeftY = 0;
		vp.Width = (float)(m_width);
		vp.Height = (float)(m_height);
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;

		m_pD3DContext->RSSetViewports(1, &vp);
		
		if (clearDepth)
			m_pD3DContext->ClearDepthStencilView(m_pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	}
}

void D3D11Renderer::setRenderTargetsAndViewportWithNoDepth(TextureGPU *pDestColorTex, bool clear)
{
	ID3D11RenderTargetView *renderTargets[1] = {0};
	if (pDestColorTex != 0)
	{
		renderTargets[0] = pDestColorTex->m_pRenderTargetView;
	}
	else
	{
		renderTargets[0] = m_pRenderTargetView;
	}

	m_pD3DContext->OMSetRenderTargets(1, renderTargets, 0);

	if (pDestColorTex != 0)
	{
		m_pD3DContext->RSSetViewports(1, &pDestColorTex->m_viewport);
		
		float color[] = {0.0f, 0.0f, 0.0f, 0.0f};
		if (clear)
		{
			m_pD3DContext->ClearRenderTargetView(pDestColorTex->m_pRenderTargetView, color);
		}
	}
	else
	{
		D3D11_VIEWPORT vp;
			vp.TopLeftX = 0;
			vp.TopLeftY = 0;
			vp.Width = (float)(m_width);
			vp.Height = (float)(m_height);
			vp.MinDepth = 0.0f;
			vp.MaxDepth = 1.0f;

		m_pD3DContext->RSSetViewports(1, &vp);
		float color[] = {1.0f * (rand() % 100) / 100.0f, 0.0f, 0.0f, 1.0f};
		if (clear)
		{
			m_pD3DContext->ClearRenderTargetView(m_pRenderTargetView, color);
		}
	}
}


void D3D11Renderer::setDepthStencilOnlyRenderTargetAndViewport(TextureGPU *pDestDepthTex, bool clear)
{
	m_pD3DContext->OMSetRenderTargets(0, NULL, pDestDepthTex->m_DepthStencilView);
	m_pD3DContext->RSSetViewports(1, &pDestDepthTex->m_viewport);
	
	if (clear)
	{
		m_pD3DContext->ClearDepthStencilView(pDestDepthTex->m_DepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	}
}

void D3D11Renderer::endRenderTarget(TextureGPU *pTex)
{
	m_pD3DContext->OMSetRenderTargets(0, NULL, NULL);
}


void IRenderer::Construct(PE::GameContext &context, unsigned int width, unsigned int height)
{
	PE::Handle h("D3D11GPUSCREEN", sizeof(D3D11Renderer));
	D3D11Renderer *pScreen = new(h) D3D11Renderer(context, width, height);
	context.m_pGPUScreen = pScreen;
}

void D3D11Renderer::endFrame()
{

}

bool IRenderer::checkForErrors(const char *situation)
{
	return true;
}


void IRenderer::AcquireRenderContextOwnership(int &threadOwnershipMask)
{
	bool needAssert = (threadOwnershipMask & Threading::RenderContext) > 0;

	if (needAssert)
	{
		assert(!needAssert);
	}

	m_renderLock.lock();

	threadOwnershipMask = threadOwnershipMask | Threading::RenderContext;

}
void IRenderer::ReleaseRenderContextOwnership(int &threadOwnershipMask)
{
	assert((threadOwnershipMask & Threading::RenderContext));

	m_renderLock.unlock();

	threadOwnershipMask = threadOwnershipMask & ~Threading::RenderContext;
}



}; // namespace PE

#endif // APIAbstraction
