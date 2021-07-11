#include "Pch.h"
#include "MeshBuilder.h"

#include "Level.h"
#include "Room.h"

#include <Camera.h>
#include <DirectX.h>
#include <ResourceManager.h>
#include <SuperShader.h>

MeshBuilder::MeshBuilder() : vb(nullptr), ib(nullptr)
{
	shader = render->GetShader<SuperShader>();
	texFloor = res_mgr->Load<Texture>("floor_tile.jpg");
}

MeshBuilder::~MeshBuilder()
{
	Clear();
}

void MeshBuilder::Clear()
{
	SafeRelease(vb);
	SafeRelease(ib);
}

void MeshBuilder::Build(Level* level)
{
	Clear();

	vertices.clear();
	indices.clear();
	floorParts.clear();

	for(Room* room : level->rooms)
	{
		const Box& box = room->box;

		// --- floor
		floorParts.push_back(Int2(indices.size(), 6));

		word index = (word)vertices.size();
		indices.push_back(index + 0);
		indices.push_back(index + 2);
		indices.push_back(index + 3);
		indices.push_back(index + 0);
		indices.push_back(index + 3);
		indices.push_back(index + 1);

		Vec3 pos = Vec3(box.v1.x, box.v1.y, box.v1.z);
		vertices.push_back({ pos, Vec3::Up, Vec2(pos.x / 2, pos.z / 2) });
		pos = Vec3(box.v2.x, box.v1.y, box.v1.z);
		vertices.push_back({ pos, Vec3::Up, Vec2(pos.x / 2, pos.z / 2) });
		pos = Vec3(box.v1.x, box.v1.y, box.v2.z);
		vertices.push_back({ pos, Vec3::Up, Vec2(pos.x / 2, pos.z / 2) });
		pos = Vec3(box.v2.x, box.v1.y, box.v2.z);
		vertices.push_back({ pos, Vec3::Up, Vec2(pos.x / 2, pos.z / 2) });
	}

	// create vertex buffer
	D3D11_BUFFER_DESC desc = {};
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.ByteWidth = sizeof(VDefault) * vertices.size();
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA data = {};
	data.pSysMem = vertices.data();

	render->GetDevice()->CreateBuffer(&desc, &data, &vb);

	// create index buffer
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.ByteWidth = sizeof(word) * indices.size();
	desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	desc.StructureByteStride = 0;

	data.pSysMem = indices.data();

	render->GetDevice()->CreateBuffer(&desc, &data, &ib);
}


void MeshBuilder::Draw(Camera& camera)
{
	if(!vb)
		return;

	const std::array<Light*, 3> lights{ nullptr, nullptr, nullptr };

	app::render->SetBlendState(Render::BLEND_NO);
	app::render->SetDepthState(Render::DEPTH_YES);
	app::render->SetRasterState(Render::RASTER_NORMAL);

	shader->Prepare();
	shader->SetShader(shader->GetShaderId(false, false, false, false, false, false, false, false));
	shader->SetCustomMesh(vb, ib, sizeof(VDefault));

	// floor
	shader->SetTexture(texFloor);
	for(Int2& part : floorParts)
		shader->DrawCustom(Matrix::IdentityMatrix, camera.mat_view_proj, lights, part.x, part.y);

	// wall
	//shader->SetTexture(texWall);
	//for(Int2& part : wallParts)
	//	shader->DrawCustom(Matrix::IdentityMatrix, camera.mat_view_proj, lights, part.x, part.y);

	// ceiling
	//shader->SetTexture(texCeiling);
	//for(Int2& part : ceilingParts)
	//	shader->DrawCustom(Matrix::IdentityMatrix, camera.mat_view_proj, lights, part.x, part.y);
}
