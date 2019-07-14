#pragma once

//-----------------------------------------------------------------------------
#include "ShaderHandler.h"
#include "VertexDeclaration.h"

//-----------------------------------------------------------------------------
class DebugDrawer : public ShaderHandler
{
public:
	typedef delegate<void(DebugDrawer*)> Handler;

	DebugDrawer(Render* render);
	~DebugDrawer();
	void OnInit() override;
	void OnReset() override;
	void OnReload() override;
	void OnRelease() override;
	void SetCamera(const CameraBase& camera);
	void Draw();
	void BeginBatch();
	void AddQuad(const Vec3(&pts)[4], const Vec4& color);
	void EndBatch();

	Handler GetHandler() const { return handler; }

	void SetHandler(Handler handler) { this->handler = handler; }

private:
	Render* render;
	IDirect3DDevice9* device;
	Handler handler;
	ID3DXEffect* effect;
	D3DXHANDLE hTechSimple, hMatCombined;
	IDirect3DVertexDeclaration9* vertex_decl;
	VB vb;
	uint vb_size;
	vector<VColor> verts;
	Matrix mat_view_proj;
	bool batch;
};
