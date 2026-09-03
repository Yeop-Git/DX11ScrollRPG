#include "ResourceManager.h"

#include "../ThirdParty/stb/stb_image.h"

// Application.cpp의 함수를 매개변수만 조금 바꿔서 사용
bool ResourceManager::LoadTextrue(ID3D11Device* device, SpriteId id, const char* filePath)
{
	int width = 0;
	int height = 0;
	int channels = 0;

	unsigned char* pixels = stbi_load(
		filePath,
		&width,
		&height,
		&channels,
		4
	);

	if (!pixels) return false;

	D3D11_TEXTURE2D_DESC textureDesc{};

	textureDesc.Width = static_cast<UINT>(width);
	textureDesc.Height = static_cast<UINT>(height);

	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;

	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	textureDesc.SampleDesc.Count = 1;

	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA initialData{};
	initialData.pSysMem = pixels;
	initialData.SysMemPitch = static_cast<UINT>(width * 4);

	ComPtr<ID3D11Texture2D> texture;

	HRESULT hr = device->CreateTexture2D(
		&textureDesc,
		&initialData,
		texture.GetAddressOf()
	);

	stbi_image_free(pixels);

	if (FAILED(hr)) return false;

	ComPtr<ID3D11ShaderResourceView> srv;

	hr = device->CreateShaderResourceView(
		texture.Get(),
		nullptr,
		&srv
	);

	if (FAILED(hr)) return false;

	textures_[id] = std::move(srv);

	return true;
}

ID3D11ShaderResourceView* ResourceManager::GetTexture(SpriteId id) const
{
	auto it = textures_.find(id);

	if (it == textures_.end()) return nullptr;

	return it->second.Get();
}

void ResourceManager::Clear()
{
	textures_.clear();
}