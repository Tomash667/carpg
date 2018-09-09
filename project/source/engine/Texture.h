#pragma once

struct TextureLock
{
	TextureLock(TEX tex);
	~TextureLock();
	uint* operator [] (uint row) { return (uint*)(data + pitch * row); }

private:
	TEX tex;
	byte* data;
	int pitch;
};
