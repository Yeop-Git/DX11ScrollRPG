#include "ResourceManager.h"

#include "../ThirdParty/stb/stb_image.h"

// 모든 텍스쳐 로드하기
bool ResourceManager::Initialize(ID3D11Device* device)
{
	// Player
	if (!LoadTextrue(device, SpriteId::PlayerIdle, "Assets/Textures/Player/Idle.png")) return false;
	if (!LoadTextrue(device, SpriteId::PlayerRun, "Assets/Textures/Player/Run.png")) return false;
	if (!LoadTextrue(device, SpriteId::PlayerJumpStart, "Assets/Textures/Player/JumpStart.png")) return false;
	if (!LoadTextrue(device, SpriteId::PlayerJumpEnd, "Assets/Textures/Player/JumpEnd.png")) return false;
	if (!LoadTextrue(device, SpriteId::PlayerAttack, "Assets/Textures/Player/Attack.png")) return false;
	if (!LoadTextrue(device, SpriteId::PlayerDead, "Assets/Textures/Player/Dead.png")) return false;

	// Monster
	if (!LoadTextrue(device, SpriteId::MonsterIdle, "Assets/Textures/Monster/Idle.png")) return false;
	if (!LoadTextrue(device, SpriteId::MonsterChase, "Assets/Textures/Monster/Chase.png")) return false;
	if (!LoadTextrue(device, SpriteId::MonsterHurt, "Assets/Textures/Monster/Hurt.png")) return false;
	if (!LoadTextrue(device, SpriteId::MonsterDead, "Assets/Textures/Monster/Dead.png")) return false;

	// World
	if (!LoadTextrue(device, SpriteId::Ground, "Assets/Textures/World/Ground.png")) return false;
	if (!LoadTextrue(device, SpriteId::Tree, "Assets/Textures/World/Tree.png")) return false;
	if (!LoadTextrue(device, SpriteId::Background, "Assets/Textures/World/Background.png")) return false;

	//World Item
	if (!LoadTextrue(device, SpriteId::Coin,"Assets/Textures/Item/Coin.png")) return false;
	if (!LoadTextrue(device, SpriteId::Potion,"Assets/Textures/Item/Potion.png")) return false;
}

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