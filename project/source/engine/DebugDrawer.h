#pragma once

//-----------------------------------------------------------------------------
#include "ShaderHandler.h"
#include "VertexDeclaration.h"

//-----------------------------------------------------------------------------
class DebugDrawer : public ShaderHandler
{
public:
	typedef delegate<void(DebugDrawer*)> Handler;

	DebugDrawer();
	~DebugDrawer();
	void InitOnce();
	void Cleanup();
	void OnInit() override;
	void OnReset() override;
	void OnReload() override;
	void OnRelease() override;
	void Draw();
	void BeginBatch();
	void AddQuad(const Vec3(&pts)[4], const Vec4& color);
	void EndBatch();

	Handler GetHandler() const { return handler; }

	void SetHandler(Handler handler) { this->handler = handler; }

private:
	Handler handler;
	ID3DXEffect* effect;
	D3DXHANDLE hTechSimple, hMatCombined;
	IDirect3DVertexDeclaration9* vertex_decl;
	VB vb;
	uint vb_size;
	vector<VColor> verts;
	bool batch;
};
