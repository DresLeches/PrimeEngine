
// API Abstraction
#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

// Outer-Engine includes
// Sibling includes
#include "Texture.h"

#include "Texture_DDS_Loader_GL.h"

// Inter-Engine includes
#include "../../Utils/ErrorHandling.h"
#include "PrimeEngine/Utils/PEString.h"
#include "PrimeEngine/Math/CameraOps.h"
#include "PrimeEngine/Utils/StringOps.h"
#include "PrimeEngine/FileSystem/FileReader.h"
namespace PE {
#if APIABSTRACTION_OGL

//-----------------------------------------------------------------------------
#endif

void TextureGPU::createTextureNoFamily(const char *textureFilename, const char *package)
{
    PEString::generatePathname(*m_pContext, textureFilename, package, "Textures", PEString::s_buf, PEString::BUF_SIZE);

	StringOps::writeToString(textureFilename, m_name, 256);
	// Path is now a full path to the file with the filename itself

#if APIABSTRACTION_D3D9
	
	m_pTexture = gfxLoadDDSTexture(PEString::s_buf);

#elif APIABSTRACTION_D3D11
	// retrieve device from IRenderer
	m_pTexture = gfxLoadDDSTexture(PEString::s_buf);

	D3D11Renderer *pD3D11Renderer = static_cast<D3D11Renderer *>(m_pContext->getGPUScreen());
	ID3D11Device *pDevice = pD3D11Renderer->m_pD3DDevice;
	ID3D11DeviceContext *pDeviceContext = pD3D11Renderer->m_pD3DContext;

	/*
	D3D11_SHADER_RESOURCE_VIEW_DESC sbSRVDesc;
		sbSRVDesc.Buffer.ElementOffset = 0;
		sbSRVDesc.Buffer.ElementWidth = structSize;
		sbSRVDesc.Buffer.FirstElement = 0;
		sbSRVDesc.Buffer.NumElements = numStructs;
		sbSRVDesc.Format = DXGI_FORMAT_UNKNOWN;
		sbSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	*/
	// since we are accessing the whole resource in the format that it was created, no need to fill in the D3D11_SHADER_RESOURCE_VIEW_DESC struct
	HRESULT hr = pDevice->CreateShaderResourceView(m_pTexture, NULL, &m_pShaderResourceView);
	PEASSERT(SUCCEEDED(hr), "Creating shader resource view");
#elif APIABSTRACTION_OGL
	m_texture = gfxLoadDDSTexture(PEString::s_buf);
#elif PE_PLAT_IS_PSVITA
	m_texture = gfxLoadDDSTexture(PEString::s_buf);
#	endif
}

void TextureGPU::createColorTextureGPU(const char* textureFilename, const char *package, ESamplerState samplerState /*= = SamplerState_Count*/)
{
	//default
	m_samplerState = SamplerState_MipLerp_MinTexelLerp_MagTexelLerp_Wrap;

	// in case custom was provided
	if (samplerState != SamplerState_Count)
		m_samplerState = samplerState;

    createTextureNoFamily(textureFilename, package);
    m_family = TextureFamily::COLOR_MAP;

	PEASSERT(API_CHOOSE_DX11_DX9_OGL_PSVITA(m_pShaderResourceView, m_pTexture, m_texture, true), "texture shader resource not set");
}

void TextureGPU::createCubeMapTextureGPU(const char * textureFilenameXN, const char * textureFilenameXP,
	const char * textureFilenameYN, const char * textureFilenameYP,
	const char * textureFilenameZN, const char * textureFilenameZP,
	const char *package, ESamplerState samplerState /*= = SamplerState_Count*/)
{
	//default
	m_samplerState = SamplerState_MipLerp_MinTexelLerp_MagTexelLerp_Wrap;

	// in case custom was provided
	if (samplerState != SamplerState_Count)
		m_samplerState = samplerState;

	TextureGPU xn(*m_pContext, m_arena);
	xn.createColorTextureGPU(textureFilenameXN, package);
	TextureGPU xp(*m_pContext, m_arena);
	xp.createColorTextureGPU(textureFilenameXP, package);

	TextureGPU yn(*m_pContext, m_arena);
	yn.createColorTextureGPU(textureFilenameYN, package);
	TextureGPU yp(*m_pContext, m_arena);
	yp.createColorTextureGPU(textureFilenameYP, package);

	TextureGPU zn(*m_pContext, m_arena);
	zn.createColorTextureGPU(textureFilenameZN, package);
	TextureGPU zp(*m_pContext, m_arena);
	zp.createColorTextureGPU(textureFilenameZP, package);

	// Each element in the texture array has the same format/dimensions.
	D3D11_TEXTURE2D_DESC texElementDesc;
	xn.m_pTexture->GetDesc(&texElementDesc);

	D3D11_TEXTURE2D_DESC texArrayDesc;
	texArrayDesc.Width = texElementDesc.Width;
	texArrayDesc.Height = texElementDesc.Height;
	texArrayDesc.MipLevels = texElementDesc.MipLevels;
	texArrayDesc.ArraySize = 6;
	texArrayDesc.Format = texElementDesc.Format;
	texArrayDesc.SampleDesc.Count = 1;
	texArrayDesc.SampleDesc.Quality = 0;
	texArrayDesc.Usage = D3D11_USAGE_DEFAULT;
	texArrayDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texArrayDesc.CPUAccessFlags = 0;
	texArrayDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

	D3D11Renderer *pD3D11Renderer = static_cast<D3D11Renderer *>(m_pContext->getGPUScreen());
	ID3D11Device *pd3dDevice = pD3D11Renderer->m_pD3DDevice;

	HRESULT hr = pd3dDevice->CreateTexture2D(&texArrayDesc, 0, &m_pTexture);
		
	// Copy individual texture elements into texture array.
	ID3D11DeviceContext* pd3dContext;
	pd3dDevice->GetImmediateContext(&pd3dContext);
	D3D11_BOX sourceRegion;

	// For XN
	for (UINT mipLevel = 0; mipLevel < texArrayDesc.MipLevels; mipLevel++)
	{
		sourceRegion.left = 0;
		sourceRegion.right = (texArrayDesc.Width >> mipLevel);
		sourceRegion.top = 0;
		sourceRegion.bottom = (texArrayDesc.Height >> mipLevel);
		sourceRegion.front = 0;
		sourceRegion.back = 1;

		//test for overflow
		if (sourceRegion.bottom == 0 || sourceRegion.right == 0)
			break;

		pd3dContext->CopySubresourceRegion(m_pTexture, D3D11CalcSubresource(mipLevel, 0, texArrayDesc.MipLevels), 0, 0, 0, xn.m_pTexture, mipLevel, &sourceRegion);
	}

	// For XP
	for (UINT mipLevel = 0; mipLevel < texArrayDesc.MipLevels; mipLevel++)
	{
		sourceRegion.left = 0;
		sourceRegion.right = (texArrayDesc.Width >> mipLevel);
		sourceRegion.top = 0;
		sourceRegion.bottom = (texArrayDesc.Height >> mipLevel);
		sourceRegion.front = 0;
		sourceRegion.back = 1;

		//test for overflow
		if (sourceRegion.bottom == 0 || sourceRegion.right == 0)
			break;

		pd3dContext->CopySubresourceRegion(m_pTexture, D3D11CalcSubresource(mipLevel, 1, texArrayDesc.MipLevels), 0, 0, 0, xp.m_pTexture, mipLevel, &sourceRegion);
	}

	// For YP
	for (UINT mipLevel = 0; mipLevel < texArrayDesc.MipLevels; mipLevel++)
	{
		sourceRegion.left = 0;
		sourceRegion.right = (texArrayDesc.Width >> mipLevel);
		sourceRegion.top = 0;
		sourceRegion.bottom = (texArrayDesc.Height >> mipLevel);
		sourceRegion.front = 0;
		sourceRegion.back = 1;

		//test for overflow
		if (sourceRegion.bottom == 0 || sourceRegion.right == 0)
			break;

		pd3dContext->CopySubresourceRegion(m_pTexture, D3D11CalcSubresource(mipLevel, 2, texArrayDesc.MipLevels), 0, 0, 0, yp.m_pTexture, mipLevel, &sourceRegion);
	}

	// For YN
	for (UINT mipLevel = 0; mipLevel < texArrayDesc.MipLevels; mipLevel++)
	{
		sourceRegion.left = 0;
		sourceRegion.right = (texArrayDesc.Width >> mipLevel);
		sourceRegion.top = 0;
		sourceRegion.bottom = (texArrayDesc.Height >> mipLevel);
		sourceRegion.front = 0;
		sourceRegion.back = 1;

		//test for overflow
		if (sourceRegion.bottom == 0 || sourceRegion.right == 0)
			break;

		pd3dContext->CopySubresourceRegion(m_pTexture, D3D11CalcSubresource(mipLevel, 3, texArrayDesc.MipLevels), 0, 0, 0, yn.m_pTexture, mipLevel, &sourceRegion);
	}

	// For ZN
	for (UINT mipLevel = 0; mipLevel < texArrayDesc.MipLevels; mipLevel++)
	{
		sourceRegion.left = 0;
		sourceRegion.right = (texArrayDesc.Width >> mipLevel);
		sourceRegion.top = 0;
		sourceRegion.bottom = (texArrayDesc.Height >> mipLevel);
		sourceRegion.front = 0;
		sourceRegion.back = 1;

		//test for overflow
		if (sourceRegion.bottom == 0 || sourceRegion.right == 0)
			break;

		pd3dContext->CopySubresourceRegion(m_pTexture, D3D11CalcSubresource(mipLevel, 4, texArrayDesc.MipLevels), 0, 0, 0, zn.m_pTexture, mipLevel, &sourceRegion);
	}

	// For ZP
	for (UINT mipLevel = 0; mipLevel < texArrayDesc.MipLevels; mipLevel++)
	{
		sourceRegion.left = 0;
		sourceRegion.right = (texArrayDesc.Width >> mipLevel);
		sourceRegion.top = 0;
		sourceRegion.bottom = (texArrayDesc.Height >> mipLevel);
		sourceRegion.front = 0;
		sourceRegion.back = 1;

		//test for overflow
		if (sourceRegion.bottom == 0 || sourceRegion.right == 0)
			break;

		pd3dContext->CopySubresourceRegion(m_pTexture, D3D11CalcSubresource(mipLevel, 5, texArrayDesc.MipLevels), 0, 0, 0, zp.m_pTexture, mipLevel, &sourceRegion);
	}

	// Create a resource view to the texture array.
	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
	viewDesc.Format = texArrayDesc.Format;
	viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	viewDesc.TextureCube.MostDetailedMip = 0;
	viewDesc.TextureCube.MipLevels = texArrayDesc.MipLevels;

	hr = pd3dDevice->CreateShaderResourceView(m_pTexture, &viewDesc, &m_pShaderResourceView);

	m_family = TextureFamily::COLOR_CUBE;
}

void TextureGPU::createBumpTextureGPU(const PrimitiveTypes::String textureFilename, const char *package)
{
	m_samplerState = SamplerState_MipLerp_MinTexelLerp_MagTexelLerp_Wrap;

	createTextureNoFamily(textureFilename, package);

	m_family = TextureFamily::NORMAL_MAP;
}

void TextureGPU::createSpecularTextureGPU(const PrimitiveTypes::String textureFilename, const char *package)
{
	m_samplerState = SamplerState_MipLerp_MinTexelLerp_MagTexelLerp_Wrap;

	createTextureNoFamily(textureFilename, package);

	m_family = TextureFamily::SPECULAR_MAP;
}

void TextureGPU::createGlowTextureGPU(const PrimitiveTypes::String textureFilename, const char *package)
{
	m_samplerState = SamplerState_MipLerp_MinTexelLerp_MagTexelLerp_Wrap;

	createTextureNoFamily(textureFilename, package);

	m_family = TextureFamily::GLOW_MAP;
}
#if 0
void TextureGPU::createReadableColorTexture(PrimitiveTypes::UInt32 w, PrimitiveTypes::UInt32 h)
{
	m_viewport.TopLeftX = 0;
	m_viewport.TopLeftY = 0;
	m_viewport.Width = w;
	m_viewport.Height = h;
	m_viewport.MinDepth = 0.0f;
	m_viewport.MaxDepth = 1.0f;

#	if APIABSTRACTION_D3D9
	// retrieve device from IRenderer
	ID3D10Device *pD3DDevice = IRenderer::Instance()->m_device;

	ID3D10Texture2D *pColorMap = 0;

	D3D10_TEXTURE2D_DESC texDesc;
		texDesc.Width = w;
		texDesc.Height = h;
		texDesc.MipLevels = 0;
		texDesc.ArraySize = 1;
		texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Usage = D3D10_USAGE_DEFAULT;
		texDesc.BindFlags = D3D10_BIND_RENDER_TARGET |
		                    D3D10_BIND_SHADER_RESOURCE;
		texDesc.CPUAccessFlags = 0;
		texDesc.MiscFlags = D3D10_RESOURCE_MISC_GENERATE_MIPS;
#elif APIABSTRACTION_D3D11
	// retrieve device from IRenderer
	ID3D11Device *pD3DDevice = IRenderer::Instance()->m_device;

	//ID3D11Texture2D *pColorMap = 0;

	D3D11_TEXTURE2D_DESC texDesc;
		texDesc.Width = w;
		texDesc.Height = h;
		texDesc.MipLevels = 0;
		texDesc.ArraySize = 1;
		texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Usage = D3D11_USAGE_STAGING ;
		texDesc.BindFlags = 0;
		texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		texDesc.MiscFlags = 0;
#endif
	HRESULT hr = pD3DDevice->CreateTexture2D(&texDesc, 0, &m_pTexure);
	assert(SUCCEEDED(hr));

	// Null description means to create a view to all mipmap levels
	// using the format the texture was created with
	//hr = pD3DDevice->CreateRenderTargetView(m_pTexure, 0, &m_pRenderTargetView);
	//assert(SUCCEEDED(hr));
	//hr = pD3DDevice->CreateShaderResourceView(m_pTexure, 0, &m_pShaderResourceView);
	//assert(SUCCEEDED(hr));

	//m_pTexure->Release();
}
#endif

void TextureGPU::createRandom2DTexture()
{
	createColorTextureGPU("Imrod_Diffuse.dds", "Default");
}

void TextureGPU::createColorCubeTextureGPU(const char * textureFilename, const char *package)
{
	PEString::generatePathname(*m_pContext, textureFilename, package, "Textures", PEString::s_buf, PEString::BUF_SIZE);

	// Path is now a full path to the file with the filename itself

#if APIABSTRACTION_D3D11
	assert(!"Not implememnted!");
#if 0
	// retrieve device from IRenderer
	ID3D11Device *pD3DDevice = IRenderer::Instance()->m_device;

	ID3D11Texture2D *tex = 0;
	D3DX11_IMAGE_LOAD_INFO loadInfo;
	loadInfo.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
	HRESULT hr = D3DX11CreateTextureFromFileA(pD3DDevice, path, &loadInfo, 0, (ID3D11Resource **)(&tex), 0);
	assert(SUCCEEDED(hr));

	D3D11_TEXTURE2D_DESC texDesc;
	tex->GetDesc(&texDesc);

	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
	viewDesc.Format = texDesc.Format;
	viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	viewDesc.TextureCube.MipLevels = texDesc.MipLevels;
	viewDesc.TextureCube.MostDetailedMip = 0;

	hr = pD3DDevice->CreateShaderResourceView(tex, &viewDesc, &m_pShaderResourceView);
	assert(SUCCEEDED(hr));
	tex->Release();
#endif
#	else
	assert(!"Not implemented!");
#	endif
	m_family = TextureFamily::COLOR_CUBE;
}

}; // namespace PE
