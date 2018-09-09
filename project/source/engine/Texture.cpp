#include "Pch.h"
#include "EngineCore.h"
#include "Texture.h"
#include "DirectX.h"

TextureLock::TextureLock(TEX tex) : tex(tex)
{
	assert(tex);
	D3DLOCKED_RECT lock;
	tex->LockRect(0, &lock, nullptr, 0);
	data = (byte*)lock.pBits;
	pitch = lock.Pitch;
}

TextureLock::~TextureLock()
{
	tex->UnlockRect(0);
}
