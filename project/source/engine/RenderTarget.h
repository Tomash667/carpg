#pragma once

class RenderTarget
{
	friend class Render;

	RenderTarget() : tmp_surf(false) {}
	~RenderTarget() {}
public:
	void SaveToFile(cstring filename);
	void FreeSurface();
	TEX GetTexture() const { return tex; }
	SURFACE GetSurface();
	const Int2& GetSize() const { return size; }

private:
	TEX tex;
	SURFACE surf;
	Int2 size;
	bool tmp_surf;
};
