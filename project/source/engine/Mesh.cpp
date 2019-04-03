#include "Pch.h"
#include "EngineCore.h"
#include "Mesh.h"
#include "ResourceManager.h"
#include "DirectX.h"

//---------------------------
Matrix mat_zero;

struct AVertex
{
	Vec3 pos;
	Vec3 normal;
	Vec2 tex;
	Vec3 tangent;
	Vec3 binormal;
};

const Vec3 DefaultSpecularColor(1, 1, 1);
const float DefaultSpecularIntensity = 0.2f;
const int DefaultSpecularHardness = 10;

//=================================================================================================
// Konstruktor Mesh
//=================================================================================================
Mesh::Mesh() : vb(nullptr), ib(nullptr), vertex_decl(VDI_DEFAULT)
{
}

//=================================================================================================
// Destruktor Mesh
//=================================================================================================
Mesh::~Mesh()
{
	SafeRelease(vb);
	SafeRelease(ib);
}

//=================================================================================================
// Wczytywanie modelu z pliku
//=================================================================================================
void Mesh::Load(StreamReader& stream, IDirect3DDevice9* device)
{
	assert(device);

	LoadHeader(stream);
	SetVertexSizeDecl();

	// ------ vertices
	// ensure size
	uint size = vertex_size * head.n_verts;
	if(!stream.Ensure(size))
		throw "Failed to read vertex buffer.";

	// create vertex buffer
	HRESULT hr = device->CreateVertexBuffer(size, 0, 0, D3DPOOL_MANAGED, &vb, nullptr);
	if(FAILED(hr))
		throw Format("Failed to create vertex buffer (%d).", hr);

	// read
	void* ptr;
	V(vb->Lock(0, size, &ptr, 0));
	stream.Read(ptr, size);
	V(vb->Unlock());

	// ----- triangles
	// ensure size
	size = sizeof(word) * head.n_tris * 3;
	if(!stream.Ensure(size))
		throw "Failed to read index buffer.";

	// create index buffer
	hr = device->CreateIndexBuffer(size, 0, D3DFMT_INDEX16, D3DPOOL_MANAGED, &ib, nullptr);
	if(FAILED(hr))
		throw Format("Failed to create index buffer (%d).", hr);

	// read
	V(ib->Lock(0, size, &ptr, 0));
	stream.Read(ptr, size);
	V(ib->Unlock());

	// ----- submeshes
	size = Submesh::MIN_SIZE * head.n_subs;
	if(!stream.Ensure(size))
		throw "Failed to read submesh data.";
	subs.resize(head.n_subs);

	for(word i = 0; i < head.n_subs; ++i)
	{
		Submesh& sub = subs[i];

		stream.Read(sub.first);
		stream.Read(sub.tris);
		stream.Read(sub.min_ind);
		stream.Read(sub.n_ind);
		stream.Read(sub.name);
		const string& tex_name = stream.ReadString1();
		if(!tex_name.empty())
			sub.tex = ResourceManager::Get<Texture>().GetLoaded(tex_name);
		else
			sub.tex = nullptr;

		// specular value
		stream.Read(sub.specular_color);
		stream.Read(sub.specular_intensity);
		stream.Read(sub.specular_hardness);

		// normalmap
		if(IS_SET(head.flags, F_TANGENTS))
		{
			const string& tex_name = stream.ReadString1();
			if(!tex_name.empty())
			{
				sub.tex_normal = ResourceManager::Get<Texture>().GetLoaded(tex_name);
				stream.Read(sub.normal_factor);
			}
			else
				sub.tex_normal = nullptr;
		}
		else
			sub.tex_normal = nullptr;

		// specular map
		const string& tex_name_specular = stream.ReadString1();
		if(!tex_name_specular.empty())
		{
			sub.tex_specular = ResourceManager::Get<Texture>().GetLoaded(tex_name_specular);
			stream.Read(sub.specular_factor);
			stream.Read(sub.specular_color_factor);
		}
		else
			sub.tex_specular = nullptr;

		if(!stream)
			throw Format("Failed to read submesh %u.", i);
	}

	// animation data
	if(IS_SET(head.flags, F_ANIMATED) && !IS_SET(head.flags, F_STATIC))
	{
		// bones
		size = Bone::MIN_SIZE * head.n_bones;
		if(!stream.Ensure(size))
			throw "Failed to read bones.";
		bones.resize(head.n_bones + 1);

		// zero bone
		Bone& zero_bone = bones[0];
		zero_bone.parent = 0;
		zero_bone.name = "zero";
		zero_bone.id = 0;
		zero_bone.mat = Matrix::IdentityMatrix;

		for(byte i = 1; i <= head.n_bones; ++i)
		{
			Bone& bone = bones[i];
			bone.id = i;

			if(head.version >= 21)
			{
				stream.Read(bone.name);
				stream.Read(bone.parent);
				LoadMatrix33(stream, bone.mat);
				stream.Skip(sizeof(Matrix) + sizeof(Vec4) * 2 + sizeof(bool));
			}
			else
			{
				stream.Read(bone.parent);
				LoadMatrix33(stream, bone.mat);
				stream.Read(bone.name);
			}

			bones[bone.parent].childs.push_back(i);
		}

		if(!stream)
			throw "Failed to read bones data.";

		// bone groups (version >= 21)
		if(head.version >= 21)
		{
			++head.n_bones; // remove when removed zero bone
			LoadBoneGroups(stream);
			--head.n_bones;
		}

		// animations
		size = Animation::MIN_SIZE * head.n_anims;
		if(!stream.Ensure(size))
			throw "Failed to read animations.";
		anims.resize(head.n_anims);

		for(byte i = 0; i < head.n_anims; ++i)
		{
			Animation& anim = anims[i];

			stream.Read(anim.name);
			stream.Read(anim.length);
			stream.Read(anim.n_frames);

			size = anim.n_frames * (4 + sizeof(KeyframeBone) * head.n_bones);
			if(!stream.Ensure(size))
				throw Format("Failed to read animation %u data.", i);

			anim.frames.resize(anim.n_frames);

			for(word j = 0; j < anim.n_frames; ++j)
			{
				stream.Read(anim.frames[j].time);
				anim.frames[j].bones.resize(head.n_bones);
				stream.Read(anim.frames[j].bones.data(), sizeof(KeyframeBone) * head.n_bones);
			}
		}

		// add zero bone to count
		++head.n_bones;
	}

	LoadPoints(stream);

	// bone groups (version < 21)
	if(head.version < 21 && IS_SET(head.flags, F_ANIMATED) && !IS_SET(head.flags, F_STATIC))
		LoadBoneGroups(stream);

	// splits
	if(IS_SET(head.flags, F_SPLIT))
	{
		size = sizeof(Split) * head.n_subs;
		if(!stream.Ensure(size))
			throw "Failed to read mesh splits.";
		splits.resize(head.n_subs);
		stream.Read(splits.data(), size);
	}
}

//=================================================================================================
// Load metadata only from mesh (points)
void Mesh::LoadMetadata(StreamReader& stream)
{
	if(vb)
		return;
	LoadHeader(stream);
	stream.SetPos(head.points_offset);
	LoadPoints(stream);
}

//=================================================================================================
void Mesh::LoadHeader(StreamReader& stream)
{
	// head
	stream.Read(head);
	if(!stream)
		throw "Failed to read file header.";
	if(memcmp(head.format, "QMSH", 4) != 0)
		throw Format("Invalid file signature '%.4s'.", head.format);
	if(head.version < 12 || head.version > 21)
		throw Format("Invalid file version '%u'.", head.version);
	if(head.version < 20)
		throw Format("Unsupported file version '%u'.", head.version);
	if(head.n_bones >= 32)
		throw Format("Too many bones (%u).", head.n_bones);
	if(head.n_subs == 0)
		throw "Missing model mesh!";
	if(IS_SET(head.flags, F_ANIMATED) && !IS_SET(head.flags, F_STATIC))
	{
		if(head.n_bones == 0)
			throw "No bones.";
		if(head.n_groups == 0)
			throw "No bone groups.";
	}
}

//=================================================================================================
void Mesh::SetVertexSizeDecl()
{
	if(IS_SET(head.flags, F_PHYSICS))
	{
		vertex_decl = VDI_POS;
		vertex_size = sizeof(VPos);
	}
	else
	{
		if(IS_SET(head.flags, F_ANIMATED))
		{
			if(IS_SET(head.flags, F_TANGENTS))
			{
				vertex_decl = VDI_ANIMATED_TANGENT;
				vertex_size = sizeof(VAnimatedTangent);
			}
			else
			{
				vertex_decl = VDI_ANIMATED;
				vertex_size = sizeof(VAnimated);
			}
		}
		else
		{
			if(IS_SET(head.flags, F_TANGENTS))
			{
				vertex_decl = VDI_TANGENT;
				vertex_size = sizeof(VTangent);
			}
			else
			{
				vertex_decl = VDI_DEFAULT;
				vertex_size = sizeof(VDefault);
			}
		}
	}
}

void Mesh::LoadPoints(StreamReader& stream)
{
	uint size = Point::MIN_SIZE * head.n_points;
	if(!stream.Ensure(size))
		throw "Failed to read points.";
	attach_points.clear();
	attach_points.resize(head.n_points);
	for(word i = 0; i < head.n_points; ++i)
	{
		Point& p = attach_points[i];

		stream.Read(p.name);
		stream.Read(p.mat);
		stream.Read(p.bone);
		stream.Read(p.type);
		stream.Read(p.size);
		stream.Read(p.rot);
		if(head.version < 21)
			p.rot.y = Clip(-p.rot.y);
		if(head.version < 22)
		{
			if(p.size.x < 0)
				p.size.x = -p.size.x;
			if(p.size.y < 0)
				p.size.y = -p.size.y;
			if(p.size.z < 0)
				p.size.z = -p.size.z;
		}
	}
}

void Mesh::LoadBoneGroups(StreamReader& stream)
{
	if(!stream.Ensure(BoneGroup::MIN_SIZE * head.n_groups))
		throw "Failed to read bone groups.";
	groups.resize(head.n_groups);
	for(word i = 0; i < head.n_groups; ++i)
	{
		BoneGroup& gr = groups[i];

		stream.Read(gr.name);

		// parent group
		stream.Read(gr.parent);
		assert(gr.parent < head.n_groups);
		assert(gr.parent != i || i == 0);

		// bone indices
		byte count;
		stream.Read(count);
		gr.bones.resize(count);
		stream.Read(gr.bones.data(), gr.bones.size());
	}

	if(!stream)
		throw "Failed to read bone groups data.";

	SetupBoneMatrices();
}

void Mesh::LoadMatrix33(StreamReader& stream, Matrix& m)
{
	stream.Read(m._11);
	stream.Read(m._12);
	stream.Read(m._13);
	m._14 = 0;
	stream.Read(m._21);
	stream.Read(m._22);
	stream.Read(m._23);
	m._24 = 0;
	stream.Read(m._31);
	stream.Read(m._32);
	stream.Read(m._33);
	m._34 = 0;
	stream.Read(m._41);
	stream.Read(m._42);
	stream.Read(m._43);
	m._44 = 1;
}

//=================================================================================================
// Ustawienie macierzy koœci
//=================================================================================================
void Mesh::SetupBoneMatrices()
{
	model_to_bone.resize(head.n_bones);
	model_to_bone[0] = Matrix::IdentityMatrix;

	for(word i = 1; i < head.n_bones; ++i)
	{
		const Mesh::Bone& bone = bones[i];
		bone.mat.Inverse(model_to_bone[i]);

		if(bone.parent > 0)
			model_to_bone[i] = model_to_bone[bone.parent] * model_to_bone[i];
	}
}

//=================================================================================================
// Zwraca koœæ o podanej nazwie
//=================================================================================================
Mesh::Bone* Mesh::GetBone(cstring name)
{
	assert(name);

	for(vector<Bone>::iterator it = bones.begin(), end = bones.end(); it != end; ++it)
	{
		if(it->name == name)
			return &*it;
	}

	return nullptr;
}

//=================================================================================================
// Zwraca animacjê o podanej nazwie
//=================================================================================================
Mesh::Animation* Mesh::GetAnimation(cstring name)
{
	assert(name);

	for(vector<Animation>::iterator it = anims.begin(), end = anims.end(); it != end; ++it)
	{
		if(it->name == name)
			return &*it;
	}

	return nullptr;
}

//=================================================================================================
// Zwraca indeks ramki i czy dok³adne trafienie
//=================================================================================================
int Mesh::Animation::GetFrameIndex(float time, bool& hit)
{
	assert(time >= 0 && time <= length);

	for(word i = 0; i < n_frames; ++i)
	{
		if(Equal(time, frames[i].time))
		{
			// równe trafienie w klatkê
			hit = true;
			return i;
		}
		else if(time < frames[i].time)
		{
			// bêdzie potrzebna interpolacja miêdzy dwoma klatkami
			assert(i != 0 && "Czas przed pierwsz¹ klatk¹!");
			hit = false;
			return i - 1;
		}
	}

	// b³¹d, chyba nie mo¿e tu dojœæ bo by wywali³o siê na assercie
	// chyba ¿e w trybie release
	return 0;
}

//=================================================================================================
// Interpolacja skali, pozycji i obrotu
//=================================================================================================
void Mesh::KeyframeBone::Interpolate(Mesh::KeyframeBone& out, const Mesh::KeyframeBone& k,
	const Mesh::KeyframeBone& k2, float t)
{
	out.rot = Quat::Slerp(k.rot, k2.rot, t);
	out.pos = Vec3::Lerp(k.pos, k2.pos, t);
	out.scale = Lerp(k.scale, k2.scale, t);
}

//=================================================================================================
// Mno¿enie macierzy w przekszta³ceniu dla danej koœci
//=================================================================================================
void Mesh::KeyframeBone::Mix(Matrix& out, const Matrix& mul) const
{
	out = Matrix::Scale(scale)
		* Matrix::Rotation(rot)
		* Matrix::Translation(pos)
		* mul;
}

//=================================================================================================
// Zwraca punkt o podanej nazwie
//=================================================================================================
Mesh::Point* Mesh::GetPoint(cstring name)
{
	assert(name);

	for(vector<Point>::iterator it = attach_points.begin(), end = attach_points.end(); it != end; ++it)
	{
		if(it->name == name)
			return &*it;
	}

	return nullptr;
}

//=================================================================================================
Mesh::Point* Mesh::GetPointIndex(cstring name)
{
	assert(name);
	int index = 0;
	for(vector<Point>::iterator it = attach_points.begin(), end = attach_points.end(); it != end; ++it, ++index)
	{
		if(it->name == name)
			return index;
	}
	return -1;
}

//=================================================================================================
// Zwraca dane ramki
//=================================================================================================
void Mesh::GetKeyframeData(KeyframeBone& keyframe, Animation* anim, uint bone, float time)
{
	assert(anim);

	bool hit;
	int index = anim->GetFrameIndex(time, hit);

	if(hit)
	{
		// exact hit in frame
		keyframe = anim->frames[index].bones[bone - 1];
	}
	else
	{
		// interpolate beetween two key frames
		const vector<Mesh::KeyframeBone>& keyf = anim->frames[index].bones;
		const vector<Mesh::KeyframeBone>& keyf2 = anim->frames[index + 1].bones;
		const float t = (time - anim->frames[index].time) /
			(anim->frames[index + 1].time - anim->frames[index].time);

		KeyframeBone::Interpolate(keyframe, keyf[bone - 1], keyf2[bone - 1], t);
	}
}

//=================================================================================================
// Wczytuje dane wierzcho³ków z modelu (na razie dzia³a tylko dla Vec3)
//=================================================================================================
void Mesh::LoadVertexData(VertexData* vd, StreamReader& stream)
{
	// read and check header
	Header head;
	stream >> head;
	if(!stream)
		throw "Failed to read file header.";
	if(memcmp(head.format, "QMSH", 4) != 0)
		throw Format("Invalid file signature '%.4s'.", head.format);
	if(head.version != 20)
		throw Format("Invalid file version '%d'.", head.version);
	if(head.flags != F_PHYSICS)
		throw Format("Invalid mesh flags '%d'.", head.flags);
	vd->radius = head.radius;

	// read vertices
	uint size = sizeof(Vec3) * head.n_verts;
	if(!stream.Ensure(size))
		throw "Failed to read vertex data.";
	vd->verts.resize(head.n_verts);
	stream.Read(vd->verts.data(), size);

	// read faces
	size = sizeof(Face) * head.n_tris;
	if(!stream.Ensure(size))
		throw "Failed to read triangle data.";
	vd->faces.resize(head.n_tris);
	stream.Read(vd->faces.data(), size);
}

//=================================================================================================
Mesh::Point* Mesh::FindPoint(cstring name)
{
	assert(name);

	int len = strlen(name);

	for(vector<Point>::iterator it = attach_points.begin(), end = attach_points.end(); it != end; ++it)
	{
		if(strncmp(name, (*it).name.c_str(), len) == 0)
			return &*it;
	}

	return nullptr;
}

//=================================================================================================
Mesh::Point* Mesh::FindNextPoint(cstring name, Point* point)
{
	assert(name && point);

	int len = strlen(name);

	for(vector<Point>::iterator it = attach_points.begin(), end = attach_points.end(); it != end; ++it)
	{
		if(&*it == point)
		{
			while(++it != end)
			{
				if(strncmp(name, (*it).name.c_str(), len) == 0)
					return &*it;
			}

			return nullptr;
		}
	}

	assert(0);
	return nullptr;
}
