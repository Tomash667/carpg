#pragma once

class RenderTarget
{
	friend class Render;

	RenderTarget() {}
	~RenderTarget() {}
public:
	TEX GetTexture() const { return tex; }
	const Int2& GetSize() const { return size; }

private:
	TEX tex;
	SURFACE surf;
	Int2 size;
};
