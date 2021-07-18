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
	texCeiling = res_mgr->Load<Texture>("sufit.jpg");
	texWall = res_mgr->Load<Texture>("256-01a.jpg");
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
	if(level->rooms.empty())
		return;

	vertices.clear();
	indices.clear();
	floorParts.clear();
	ceilingParts.clear();
	wallParts.clear();
	BuildLinks(level);

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

		// --- ceiling
		ceilingParts.push_back(Int2(indices.size(), 6));

		index = (word)vertices.size();
		indices.push_back(index + 0);
		indices.push_back(index + 1);
		indices.push_back(index + 2);
		indices.push_back(index + 2);
		indices.push_back(index + 1);
		indices.push_back(index + 3);

		pos = Vec3(box.v1.x, box.v2.y, box.v1.z);
		vertices.push_back({ pos, Vec3::Down, Vec2(pos.x / 2, pos.z / 2) });
		pos = Vec3(box.v2.x, box.v2.y, box.v1.z);
		vertices.push_back({ pos, Vec3::Down, Vec2(pos.x / 2, pos.z / 2) });
		pos = Vec3(box.v1.x, box.v2.y, box.v2.z);
		vertices.push_back({ pos, Vec3::Down, Vec2(pos.x / 2, pos.z / 2) });
		pos = Vec3(box.v2.x, box.v2.y, box.v2.z);
		vertices.push_back({ pos, Vec3::Down, Vec2(pos.x / 2, pos.z / 2) });

		// --- walls
		uint offset = indices.size();
		uint count = 0;

		// left wall
		if(PrepareOverlappingRooms(room, DIR_LEFT))
		{
			std::sort(activeLinks.begin(), activeLinks.end(), [](const RoomLink* ra, const RoomLink* rb)
			{
				return ra->box.v1.z < rb->box.v1.z;
			});

			vector<Vec2> parts;
			float start = room->box.v1.z, end;
			for(RoomLink* link : activeLinks)
			{
				end = link->box.v1.z;
				if(end - start > 0)
					parts.push_back(Vec2(start, end));
				start = link->box.v2.z;
			}
			end = room->box.v2.z;
			if(end - start > 0)
				parts.push_back(Vec2(start, end));

			for(const Vec2& part : parts)
			{
				index = (word)vertices.size();
				indices.push_back(index + 0);
				indices.push_back(index + 2);
				indices.push_back(index + 3);
				indices.push_back(index + 0);
				indices.push_back(index + 3);
				indices.push_back(index + 1);

				pos = Vec3(box.v1.x, box.v1.y, part.x);
				vertices.push_back({ pos, Vec3::Right, Vec2(pos.z / 2, pos.y / 2) });
				pos = Vec3(box.v1.x, box.v1.y, part.y);
				vertices.push_back({ pos, Vec3::Right, Vec2(pos.z / 2, pos.y / 2) });
				pos = Vec3(box.v1.x, box.v2.y, part.x);
				vertices.push_back({ pos, Vec3::Right, Vec2(pos.z / 2, pos.y / 2) });
				pos = Vec3(box.v1.x, box.v2.y, part.y);
				vertices.push_back({ pos, Vec3::Right, Vec2(pos.z / 2, pos.y / 2) });

				count += 6;
			}
		}
		else
		{
			index = (word)vertices.size();
			indices.push_back(index + 0);
			indices.push_back(index + 2);
			indices.push_back(index + 3);
			indices.push_back(index + 0);
			indices.push_back(index + 3);
			indices.push_back(index + 1);

			pos = Vec3(box.v1.x, box.v1.y, box.v1.z);
			vertices.push_back({ pos, Vec3::Right, Vec2(pos.z / 2, pos.y / 2) });
			pos = Vec3(box.v1.x, box.v1.y, box.v2.z);
			vertices.push_back({ pos, Vec3::Right, Vec2(pos.z / 2, pos.y / 2) });
			pos = Vec3(box.v1.x, box.v2.y, box.v1.z);
			vertices.push_back({ pos, Vec3::Right, Vec2(pos.z / 2, pos.y / 2) });
			pos = Vec3(box.v1.x, box.v2.y, box.v2.z);
			vertices.push_back({ pos, Vec3::Right, Vec2(pos.z / 2, pos.y / 2) });

			count += 6;
		}

		// right wall
		if(PrepareOverlappingRooms(room, DIR_RIGHT))
		{
			std::sort(activeLinks.begin(), activeLinks.end(), [](const RoomLink* ra, const RoomLink* rb)
			{
				return ra->box.v1.z < rb->box.v1.z;
			});

			vector<Vec2> parts;
			float start = room->box.v1.z, end;
			for(RoomLink* link : activeLinks)
			{
				end = link->box.v1.z;
				if(end - start > 0)
					parts.push_back(Vec2(start, end));
				start = link->box.v2.z;
			}
			end = room->box.v2.z;
			if(end - start > 0)
				parts.push_back(Vec2(start, end));

			for(const Vec2& part : parts)
			{
				index = (word)vertices.size();
				indices.push_back(index + 0);
				indices.push_back(index + 1);
				indices.push_back(index + 2);
				indices.push_back(index + 1);
				indices.push_back(index + 3);
				indices.push_back(index + 2);

				pos = Vec3(box.v2.x, box.v1.y, part.x);
				vertices.push_back({ pos, Vec3::Left, Vec2(pos.z / 2, pos.y / 2) });
				pos = Vec3(box.v2.x, box.v1.y, part.y);
				vertices.push_back({ pos, Vec3::Left, Vec2(pos.z / 2, pos.y / 2) });
				pos = Vec3(box.v2.x, box.v2.y, part.x);
				vertices.push_back({ pos, Vec3::Left, Vec2(pos.z / 2, pos.y / 2) });
				pos = Vec3(box.v2.x, box.v2.y, part.y);
				vertices.push_back({ pos, Vec3::Left, Vec2(pos.z / 2, pos.y / 2) });

				count += 6;
			}
		}
		else
		{
			index = (word)vertices.size();
			indices.push_back(index + 0);
			indices.push_back(index + 1);
			indices.push_back(index + 2);
			indices.push_back(index + 1);
			indices.push_back(index + 3);
			indices.push_back(index + 2);

			pos = Vec3(box.v2.x, box.v1.y, box.v1.z);
			vertices.push_back({ pos, Vec3::Left, Vec2(pos.z / 2, pos.y / 2) });
			pos = Vec3(box.v2.x, box.v1.y, box.v2.z);
			vertices.push_back({ pos, Vec3::Left, Vec2(pos.z / 2, pos.y / 2) });
			pos = Vec3(box.v2.x, box.v2.y, box.v1.z);
			vertices.push_back({ pos, Vec3::Left, Vec2(pos.z / 2, pos.y / 2) });
			pos = Vec3(box.v2.x, box.v2.y, box.v2.z);
			vertices.push_back({ pos, Vec3::Left, Vec2(pos.z / 2, pos.y / 2) });

			count += 6;
		}

		// front wall
		if(PrepareOverlappingRooms(room, DIR_FRONT))
		{
			std::sort(activeLinks.begin(), activeLinks.end(), [](const RoomLink* ra, const RoomLink* rb)
			{
				return ra->box.v1.x < rb->box.v1.x;
			});

			vector<Vec2> parts;
			float start = room->box.v1.x, end;
			for(RoomLink* link : activeLinks)
			{
				end = link->box.v1.x;
				if(end - start > 0)
					parts.push_back(Vec2(start, end));
				start = link->box.v2.x;
			}
			end = room->box.v2.x;
			if(end - start > 0)
				parts.push_back(Vec2(start, end));

			for(const Vec2& part : parts)
			{
				index = (word)vertices.size();
				indices.push_back(index + 0);
				indices.push_back(index + 2);
				indices.push_back(index + 3);
				indices.push_back(index + 0);
				indices.push_back(index + 3);
				indices.push_back(index + 1);

				pos = Vec3(part.x, box.v1.y, box.v2.z);
				vertices.push_back({ pos, Vec3::Backward, Vec2(pos.x / 2, pos.y / 2) });
				pos = Vec3(part.y, box.v1.y, box.v2.z);
				vertices.push_back({ pos, Vec3::Backward, Vec2(pos.x / 2, pos.y / 2) });
				pos = Vec3(part.x, box.v2.y, box.v2.z);
				vertices.push_back({ pos, Vec3::Backward, Vec2(pos.x / 2, pos.y / 2) });
				pos = Vec3(part.y, box.v2.y, box.v2.z);
				vertices.push_back({ pos, Vec3::Backward, Vec2(pos.x / 2, pos.y / 2) });

				count += 6;
			}
		}
		else
		{
			index = (word)vertices.size();
			indices.push_back(index + 0);
			indices.push_back(index + 2);
			indices.push_back(index + 3);
			indices.push_back(index + 0);
			indices.push_back(index + 3);
			indices.push_back(index + 1);

			pos = Vec3(box.v1.x, box.v1.y, box.v2.z);
			vertices.push_back({ pos, Vec3::Backward, Vec2(pos.x / 2, pos.y / 2) });
			pos = Vec3(box.v2.x, box.v1.y, box.v2.z);
			vertices.push_back({ pos, Vec3::Backward, Vec2(pos.x / 2, pos.y / 2) });
			pos = Vec3(box.v1.x, box.v2.y, box.v2.z);
			vertices.push_back({ pos, Vec3::Backward, Vec2(pos.x / 2, pos.y / 2) });
			pos = Vec3(box.v2.x, box.v2.y, box.v2.z);
			vertices.push_back({ pos, Vec3::Backward, Vec2(pos.x / 2, pos.y / 2) });

			count += 6;
		}

		// back wall
		if(PrepareOverlappingRooms(room, DIR_BACK))
		{
			std::sort(activeLinks.begin(), activeLinks.end(), [](const RoomLink* ra, const RoomLink* rb)
			{
				return ra->box.v1.x < rb->box.v1.x;
			});

			vector<Vec2> parts;
			float start = room->box.v1.x, end;
			for(RoomLink* link : activeLinks)
			{
				end = link->box.v1.x;
				if(end - start > 0)
					parts.push_back(Vec2(start, end));
				start = link->box.v2.x;
			}
			end = room->box.v2.x;
			if(end - start > 0)
				parts.push_back(Vec2(start, end));

			for(const Vec2& part : parts)
			{
				index = (word)vertices.size();
				indices.push_back(index + 0);
				indices.push_back(index + 1);
				indices.push_back(index + 2);
				indices.push_back(index + 1);
				indices.push_back(index + 3);
				indices.push_back(index + 2);

				pos = Vec3(part.x, box.v1.y, box.v1.z);
				vertices.push_back({ pos, Vec3::Forward, Vec2(pos.x / 2, pos.y / 2) });
				pos = Vec3(part.y, box.v1.y, box.v1.z);
				vertices.push_back({ pos, Vec3::Forward, Vec2(pos.x / 2, pos.y / 2) });
				pos = Vec3(part.x, box.v2.y, box.v1.z);
				vertices.push_back({ pos, Vec3::Forward, Vec2(pos.x / 2, pos.y / 2) });
				pos = Vec3(part.y, box.v2.y, box.v1.z);
				vertices.push_back({ pos, Vec3::Forward, Vec2(pos.x / 2, pos.y / 2) });

				count += 6;
			}
		}
		else
		{
			index = (word)vertices.size();
			indices.push_back(index + 0);
			indices.push_back(index + 1);
			indices.push_back(index + 2);
			indices.push_back(index + 1);
			indices.push_back(index + 3);
			indices.push_back(index + 2);

			pos = Vec3(box.v1.x, box.v1.y, box.v1.z);
			vertices.push_back({ pos, Vec3::Forward, Vec2(pos.x / 2, pos.y / 2) });
			pos = Vec3(box.v2.x, box.v1.y, box.v1.z);
			vertices.push_back({ pos, Vec3::Forward, Vec2(pos.x / 2, pos.y / 2) });
			pos = Vec3(box.v1.x, box.v2.y, box.v1.z);
			vertices.push_back({ pos, Vec3::Forward, Vec2(pos.x / 2, pos.y / 2) });
			pos = Vec3(box.v2.x, box.v2.y, box.v1.z);
			vertices.push_back({ pos, Vec3::Forward, Vec2(pos.x / 2, pos.y / 2) });

			count += 6;
		}

		if(count != 0)
			wallParts.push_back(Int2(offset, count));
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

void MeshBuilder::BuildLinks(Level* level)
{
	for(Room* room : level->rooms)
		room->links.clear();

	uint count = level->rooms.size();
	for(uint i = 0; i < count; ++i)
	{
		Room* a = level->rooms[i];
		Box aBox = a->box;
		for(uint j = i + 1; j < count; ++j)
		{
			Room* b = level->rooms[j];
			Box bBox = b->box;
			if(BoxToBox(aBox, bBox))
			{
				Box ab(max(aBox.v1.x, bBox.v1.x), max(aBox.v1.y, bBox.v1.y), max(aBox.v1.z, bBox.v1.z),
					min(aBox.v2.x, bBox.v2.x), min(aBox.v2.y, bBox.v2.y), min(aBox.v2.z, bBox.v2.z));
				const Dir dir = GetDirection(a, b);

				RoomLink& la = Add1(a->links);
				la.dir = dir;
				la.box = ab;
				la.room = b;
				la.first = true;

				RoomLink& lb = Add1(b->links);
				lb.dir = Reversed(dir);
				lb.box = ab;
				lb.room = a;
				lb.first = false;
			}
		}
	}
}

int GetMaxDimension(const Vec3& dif)
{
	const Vec3 max(abs(dif.x), abs(dif.y), abs(dif.z));
	if(max.x > max.y)
	{
		if(max.x > max.z)
			return 0; // x
		else
			return 2; // z
	}
	else
	{
		if(max.y > max.z)
			return 1; // y
		else
			return 2; // z
	}
}

Dir MeshBuilder::GetDirection(Room* a, Room* b)
{
	const Vec3 aPos = a->box.Midpoint();
	const Vec3 bPos = b->box.Midpoint();
	const Vec3 dif = bPos - aPos;
	switch(GetMaxDimension(dif))
	{
	default:
	case 0: // x
		return dif.x > 0 ? DIR_RIGHT : DIR_LEFT;
	case 1: // y
		return dif.y > 0 ? DIR_UP : DIR_DOWN;
	case 2: // z
		return dif.z > 0 ? DIR_FRONT : DIR_BACK;
	}
}

bool MeshBuilder::PrepareOverlappingRooms(Room* room, Dir dir)
{
	activeLinks.clear();
	for(RoomLink& link : room->links)
	{
		if(link.dir == dir)
			activeLinks.push_back(&link);
	}
	return !activeLinks.empty();
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
	shader->SetTexture(texWall);
	for(Int2& part : wallParts)
		shader->DrawCustom(Matrix::IdentityMatrix, camera.mat_view_proj, lights, part.x, part.y);

	// ceiling
	shader->SetTexture(texCeiling);
	for(Int2& part : ceilingParts)
		shader->DrawCustom(Matrix::IdentityMatrix, camera.mat_view_proj, lights, part.x, part.y);
}
