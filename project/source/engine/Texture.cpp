#include "Pch.h"
#include "EngineCore.h"
#include "Texture.h"
#include "DirectX.h"

//=================================================================================================
Int2 Texture::GetSize(TEX tex)
{
	D3DSURFACE_DESC desc;
	V(tex->GetLevelDesc(0, &desc));
	return Int2(desc.Width, desc.Height);
}

//=================================================================================================
TextureLock::TextureLock(TEX tex) : tex(tex)
{
	assert(tex);
	D3DLOCKED_RECT lock;
	V(tex->LockRect(0, &lock, nullptr, 0));
	data = static_cast<byte*>(lock.pBits);
	pitch = lock.Pitch;
}

//=================================================================================================
TextureLock::~TextureLock()
{
	if(tex)
		V(tex->UnlockRect(0));
}

//=================================================================================================
void TextureLock::GenerateMipSubLevels()
{
	assert(tex);
	V(tex->UnlockRect(0));
	tex->GenerateMipSubLevels();
	tex = nullptr;
}
