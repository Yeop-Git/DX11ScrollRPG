#pragma once

#include <unordered_map>

#include <d3d11.h>
#include <wrl/client.h>

#include "SpriteId.h"

using namespace Microsoft::WRL;

class ResourceManager
{
public:
	bool Initialize(ID3D11Device* device);
	bool LoadTextrue(ID3D11Device* device, SpriteId id, const char* filePath);

	ID3D11ShaderResourceView* GetTexture(SpriteId id) const;

	void Clear();

private:
	std::unordered_map<SpriteId, ComPtr<ID3D11ShaderResourceView>> textures_;
};