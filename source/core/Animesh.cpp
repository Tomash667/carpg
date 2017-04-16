#include "Pch.h"
#include "Base.h"
#include "Animesh.h"
#include "SaveState.h"
#include "BitStreamFunc.h"
#include "ResourceManager.h"

//---------------------------
Animesh::KeyframeBone blendb_zero;
MATRIX mat_zero;
void (*AnimeshInstance::Predraw)(void*,MATRIX*,int) = nullptr;

struct AVertex
{
	VEC3 pos;
	VEC3 normal;
	VEC2 tex;
	VEC3 tangent;
	VEC3 binormal;
};

const VEC3 DefaultSpecularColor(1,1,1);
const float DefaultSpecularIntensity = 0.2f;
const int DefaultSpecularHardness = 10;

//=================================================================================================
// Inicjalizacja obiekt�w u�ywanych do animacji modeli
//=================================================================================================
void Animesh::MeshInit()
{
	blendb_zero.scale = 1.f;
	D3DXQuaternionIdentity(&blendb_zero.rot);

	MATRIX tmp_matrix;
	D3DXMatrixScaling( &mat_zero, blendb_zero.scale );
	D3DXMatrixRotationQuaternion( &tmp_matrix, &blendb_zero.rot );
	mat_zero *= tmp_matrix;
	D3DXMatrixTranslation( &tmp_matrix, blendb_zero.pos );
	mat_zero *= tmp_matrix;
};

//=================================================================================================
// Konstruktor Animesh
//=================================================================================================
Animesh::Animesh() : vb(nullptr), ib(nullptr)
{
}

//=================================================================================================
// Destruktor Animesh
//=================================================================================================
Animesh::~Animesh()
{
	SafeRelease(vb);
	SafeRelease(ib);
}

//=================================================================================================
// Wczytywanie modelu z pliku
//=================================================================================================
void Animesh::Load(StreamReader& stream, IDirect3DDevice9* device)
{
	assert(device);

	// header
	if(!stream.Read(head))
		throw "Failed to read file header.";
	if(memcmp(head.format, "QMSH", 4) != 0)
		throw Format("Invalid file signature '%.4s'.", head.format);
	if(head.version < 12 || head.version > 19)
		throw Format("Invalid file version '%d'.", head.version);
	if(head.n_bones >= 32)
		throw Format("Too many bones (%d).", head.n_bones);
	if(head.n_subs == 0)
		throw "Missing model mesh!";
	if(IS_SET(head.flags, ANIMESH_ANIMATED) && !IS_SET(head.flags, ANIMESH_STATIC))
	{
		if(head.n_bones == 0)
			throw "No bones.";
		if(head.version >= 13 && head.n_groups == 0)
			throw "No bone groups.";
	}

	// camera
	if(head.version >= 13)
	{
		stream.Read(cam_pos);
		stream.Read(cam_target);
		if(head.version >= 15)
			stream.Read(cam_up);
		else
			cam_up = VEC3(0, 1, 0);
		if(!stream)
			throw "Missing camera data.";
	}
	else
	{
		cam_pos = VEC3(1, 1, 1);
		cam_target = VEC3(0, 0, 0);
		cam_up = VEC3(0, 1, 0);
	}

	// ------ vertices
	// set vertex size & fvf
	if(IS_SET(head.flags, ANIMESH_PHYSICS))
	{
		vertex_decl = VDI_POS;
		vertex_size = sizeof(VPos);
	}
	else
	{
		vertex_size = sizeof(VEC3);
		if(IS_SET(head.flags, ANIMESH_ANIMATED))
		{
			if(IS_SET(head.flags, ANIMESH_TANGENTS))
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
			if(IS_SET(head.flags, ANIMESH_TANGENTS))
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

	for(word i = 0; i<head.n_subs; ++i)
	{
		Submesh& sub = subs[i];

		stream.Read(sub.first);
		stream.Read(sub.tris);
		stream.Read(sub.min_ind);
		stream.Read(sub.n_ind);
		stream.Read(sub.name);
		stream.ReadString1();
			
		if(BUF[0])
			sub.tex = ResourceManager::Get().GetLoadedTexture(BUF);
		else
			sub.tex = nullptr;

		if(head.version >= 16)
		{
			// specular value
			if(head.version >= 18)
			{
				stream.Read(sub.specular_color);
				stream.Read(sub.specular_intensity);
				stream.Read(sub.specular_hardness);
			}
			else
			{
				sub.specular_color = DefaultSpecularColor;
				sub.specular_intensity = DefaultSpecularIntensity;
				sub.specular_hardness = DefaultSpecularHardness;
			}

			// normalmap
			if(IS_SET(head.flags, ANIMESH_TANGENTS))
			{
				stream.ReadString1();
				if(BUF[0])
				{
					sub.tex_normal = ResourceManager::Get().GetLoadedTexture(BUF);
					stream.Read(sub.normal_factor);
				}
				else
					sub.tex_normal = nullptr;
			}
			else
				sub.tex_normal = nullptr;

			// specular map
			stream.ReadString1();
			if(BUF[0])
			{
				sub.tex_specular = ResourceManager::Get().GetLoadedTexture(BUF);
				stream.Read(sub.specular_factor);
				stream.Read(sub.specular_color_factor);
			}
			else
				sub.tex_specular = nullptr;
		}
		else
		{
			sub.tex_specular = nullptr;
			sub.tex_normal = nullptr;
			sub.specular_color = DefaultSpecularColor;
			sub.specular_intensity = DefaultSpecularIntensity;
			sub.specular_hardness = DefaultSpecularHardness;
		}

		if(!stream)
			throw Format("Failed to read submesh %u.", i);
	}

	// animation data
	if(IS_SET(head.flags, ANIMESH_ANIMATED) && !IS_SET(head.flags, ANIMESH_STATIC))
	{
		// bones
		size = Bone::MIN_SIZE * head.n_bones;
		if(!stream.Ensure(size))
			throw "Failed to read bones.";
		bones.resize(head.n_bones+1);

		// zero bone
		Bone& zero_bone = bones[0];
		zero_bone.parent = 0;
		zero_bone.name = "zero";
		zero_bone.id = 0;
		D3DXMatrixIdentity(&zero_bone.mat);

		for(byte i = 1; i<=head.n_bones; ++i)
		{
			Bone& bone = bones[i];

			bone.id = i;
			stream.Read(bone.parent);

			stream.Read(bone.mat._11);
			stream.Read(bone.mat._12);
			stream.Read(bone.mat._13);
			bone.mat._14 = 0;
			stream.Read(bone.mat._21);
			stream.Read(bone.mat._22);
			stream.Read(bone.mat._23);
			bone.mat._24 = 0;
			stream.Read(bone.mat._31);
			stream.Read(bone.mat._32);
			stream.Read(bone.mat._33);
			bone.mat._34 = 0;
			stream.Read(bone.mat._41);
			stream.Read(bone.mat._42);
			stream.Read(bone.mat._43);
			bone.mat._44 = 1;
			
			stream.Read(bone.name);

			bones[bone.parent].childs.push_back(i);
		}

		if(!stream)
			throw "Failed to read bones data.";

		// animations
		size = Animation::MIN_SIZE * head.n_anims;
		if(!stream.Ensure(size))
			throw "Failed to read animations.";
		anims.resize(head.n_anims);

		for(byte i = 0; i<head.n_anims; ++i)
		{
			Animation& anim = anims[i];

			stream.Read(anim.name);
			stream.Read(anim.length);
			stream.Read(anim.n_frames);

			size = anim.n_frames * (4 + sizeof(KeyframeBone) * head.n_bones);
			if(!stream.Ensure(size))
				throw Format("Failed to read animation %u data.", i);

			anim.frames.resize(anim.n_frames);

			for(word j = 0; j<anim.n_frames; ++j)
			{
				stream.Read(anim.frames[j].time);
				anim.frames[j].bones.resize(head.n_bones);
				stream.Read(anim.frames[j].bones.data(), sizeof(KeyframeBone) * head.n_bones);
			}
		}

		// add zero bone to count
		++head.n_bones;
	}

	// points
	size = Point::MIN_SIZE * head.n_points;
	if(!stream.Ensure(size))
		throw "Failed to read points.";
	attach_points.resize(head.n_points);
	for(word i = 0; i<head.n_points; ++i)
	{
		Point& p = attach_points[i];

		stream.Read(p.name);
		stream.Read(p.mat);
		stream.Read(p.bone);
		if(head.version == 12)
			++p.bone; // in that version there was different counting
		stream.Read(p.type);
		if(head.version >= 14)
			stream.Read(p.size);
		else
		{
			stream.Read(p.size.x);
			p.size.y = p.size.z = p.size.x;
		}
		if(head.version >= 19)
		{
			stream.Read(p.rot);
			p.rot.y = clip(-p.rot.y);
		}
		else
		{
			// fallback, it was often wrong but thats the way it was (works good for PI/2 and PI*3/2, inverted for 0 and PI, bad for other)
			p.rot = VEC3(0, MatrixGetYaw(p.mat), 0);
		}
	}

	if(IS_SET(head.flags, ANIMESH_ANIMATED) && !IS_SET(head.flags, ANIMESH_STATIC))
	{
		// groups
		if(head.version == 12 && head.n_groups < 2)
		{
			head.n_groups = 1;
			groups.resize(1);

			BoneGroup& gr = groups[0];
			gr.name = "default";
			gr.parent = 0;
			gr.bones.reserve(head.n_bones-1);

			for(word i = 1; i<head.n_bones; ++i)
				gr.bones.push_back((byte)i);
		}
		else
		{
			if(!stream.Ensure(BoneGroup::MIN_SIZE * head.n_groups))
				throw "Failed to read bone groups.";
			groups.resize(head.n_groups);
			for(word i = 0; i<head.n_groups; ++i)
			{
				BoneGroup& gr = groups[i];

				stream.Read(gr.name);

				// parent group
				stream.Read(gr.parent);
				assert(gr.parent < head.n_groups);
				assert(gr.parent != i || i == 0);

				// bone indexes
				byte count;
				stream.Read(count);
				gr.bones.resize(count);
				stream.Read(gr.bones.data(), gr.bones.size());
				if(head.version == 12)
				{
					for(byte& b : gr.bones)
						++b;
				}
			}
		}

		if(!stream)
			throw "Failed to read bone groups data.";

		SetupBoneMatrices();
	}

	// splits
	if(IS_SET(head.flags, ANIMESH_SPLIT))
	{
		size = sizeof(Split) * head.n_subs;
		if(!stream.Ensure(size))
			throw "Failed to read mesh splits.";
		splits.resize(head.n_subs);
		stream.Read(splits.data(), size);
	}
}

//=================================================================================================
// Ustawienie macierzy ko�ci
//=================================================================================================
void Animesh::SetupBoneMatrices()
{
	model_to_bone.resize(head.n_bones);
	D3DXMatrixIdentity(&model_to_bone[0]);

	for(word i=1; i<head.n_bones; ++i)
	{
		const Animesh::Bone& bone = bones[i];

		D3DXMatrixInverse(&model_to_bone[i], nullptr, &bone.mat);

		if(bone.parent > 0)
			D3DXMatrixMultiply(&model_to_bone[i], &model_to_bone[bone.parent], &model_to_bone[i]);
	}
}

//=================================================================================================
// Zwraca ko�� o podanej nazwie
//=================================================================================================
Animesh::Bone* Animesh::GetBone(cstring name)
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
// Zwraca animacj� o podanej nazwie
//=================================================================================================
Animesh::Animation* Animesh::GetAnimation(cstring name)
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
// Zwraca indeks ramki i czy dok�adne trafienie
//=================================================================================================
int Animesh::Animation::GetFrameIndex( float time, bool& hit )
{
	assert(time >= 0 && time <= length);

	for( word i=0; i<n_frames; ++i )
	{
		if( equal( time, frames[i].time ) )
		{
			// r�wne trafienie w klatk�
			hit = true;
			return i;
		}
		else if( time < frames[i].time )
		{
			// b�dzie potrzebna interpolacja mi�dzy dwoma klatkami
			assert( i != 0 && "Czas przed pierwsz� klatk�!" );
			hit = false;
			return i - 1;
		}
	}

	// b��d, chyba nie mo�e tu doj�� bo by wywali�o si� na assercie
	// chyba �e w trybie release
	return 0;
}

//=================================================================================================
// Interpolacja skali, pozycji i obrotu
//=================================================================================================
void Animesh::KeyframeBone::Interpolate( Animesh::KeyframeBone& out, const Animesh::KeyframeBone& k,
									  const Animesh::KeyframeBone& k2, float t )
{
	D3DXQuaternionSlerp( &out.rot, &k.rot, &k2.rot, t );
	D3DXVec3Lerp( &out.pos, &k.pos, &k2.pos, t );
	out.scale = lerp( k.scale, k2.scale, t );
}

//=================================================================================================
// Mno�enie macierzy w przekszta�ceniu dla danej ko�ci
//=================================================================================================
void Animesh::KeyframeBone::Mix( MATRIX& out, const MATRIX& mul ) const
{
	MATRIX tmp_matrix;

	D3DXMatrixScaling( &out, scale );
	D3DXMatrixRotationQuaternion( &tmp_matrix, &rot );
	out *= tmp_matrix;
	D3DXMatrixTranslation( &tmp_matrix, pos );
	out *= tmp_matrix;
	out *= mul;
}

//=================================================================================================
// Zwraca punkt o podanej nazwie
//=================================================================================================
Animesh::Point* Animesh::GetPoint(cstring name)
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
// Zwraca dane ramki
//=================================================================================================
void Animesh::GetKeyframeData(KeyframeBone& keyframe, Animation* anim, uint bone, float time)
{
	assert(anim);

	bool hit;
	int index = anim->GetFrameIndex(time, hit);

	if(hit)
	{
		// exact hit in frame
		keyframe = anim->frames[index].bones[bone-1];
	}
	else
	{
		// interpolate beetween two key frames
		const vector<Animesh::KeyframeBone>& keyf = anim->frames[index].bones;
		const vector<Animesh::KeyframeBone>& keyf2 = anim->frames[index+1].bones;
		const float t = (time - anim->frames[index].time) /
			(anim->frames[index+1].time - anim->frames[index].time);

		KeyframeBone::Interpolate(keyframe, keyf[bone-1], keyf2[bone-1], t);
	}
}

//=================================================================================================
// Konstruktor instancji Animesh
//=================================================================================================
AnimeshInstance::AnimeshInstance(Animesh* ani) : ani(ani), need_update(true), frame_end_info(false),
frame_end_info2(false), /*sub_override(nullptr),*/ ptr(nullptr)
{
	assert(ani);

	mat_bones.resize(ani->head.n_bones);
	blendb.resize(ani->head.n_bones);
	groups.resize(ani->head.n_groups);
}

//=================================================================================================
// Destruktor instancji Animesh
//=================================================================================================
AnimeshInstance::~AnimeshInstance()
{
	/*if(sub_override)
	{
		for(word i=0; i<ani->head.n_subs; ++i)
		{
			if(sub_override[i].tex)
				sub_override[i].tex->Release();
		}
		delete[] sub_override;
	}*/
}

//=================================================================================================
// Odtwarza podan� animacj�, szybsze bo nie musi szuka� nazwy
//=================================================================================================
void AnimeshInstance::Play(Animesh::Animation* anim, int flags, int group)
{
	assert(anim && in_range(group, 0, ani->head.n_groups-1));

	Group& gr = groups[group];

	// ignoruj animacj�
	if(IS_SET(flags, PLAY_IGNORE) && gr.anim == anim)
		return;

	// resetuj szybko�� i blending
	if(IS_SET(gr.state, PLAY_RESTORE))
	{
		gr.speed = 1.f;
		gr.blend_max = 0.33f;
	}

	int new_state = 0;

	// blending
	if(!IS_SET(flags, PLAY_NO_BLEND))
	{
		SetupBlending(group);
		SET_BIT(new_state, FLAG_BLENDING);
		if(IS_SET(flags, PLAY_BLEND_WAIT))
			SET_BIT(new_state, FLAG_BLEND_WAIT);
		gr.blend_time = 0.f;
	}

	// ustaw animacj�
	gr.anim = anim;
	gr.prio = ((flags & 0x60)>>5);
	gr.state = new_state | FLAG_PLAYING | FLAG_GROUP_ACTIVE;
	if(IS_SET(flags, PLAY_ONCE))
		SET_BIT(gr.state, FLAG_ONCE);
	if(IS_SET(flags, PLAY_BACK))
	{
		SET_BIT(gr.state, FLAG_BACK);
		gr.time = anim->length;
	}
	else
		gr.time = 0.f;
	if(IS_SET(flags, PLAY_STOP_AT_END))
		SET_BIT(gr.state, FLAG_STOP_AT_END);
	if(IS_SET(flags, PLAY_RESTORE))
		SET_BIT(gr.state, FLAG_RESTORE);

	// anuluj blending w innych grupach
	if(IS_SET(flags, PLAY_NO_BLEND))
	{
		for(int g=0; g<ani->head.n_groups; ++g)
		{
			if(g != group && (!groups[g].IsActive() || groups[g].prio < gr.prio))
				CLEAR_BIT(groups[g].state, FLAG_BLENDING);
		}
	}
}

//=================================================================================================
// Wy��cz grup�
//=================================================================================================
void AnimeshInstance::Deactivate(int group)
{
	assert(in_range(group, 0, ani->head.n_groups-1));

	Group& gr = groups[group];

	if(IS_SET(gr.state, FLAG_GROUP_ACTIVE))
	{
		SetupBlending(group);

		if(IS_SET(gr.state, FLAG_RESTORE))
		{
			gr.speed = 1.f;
			gr.blend_max = 0.33f;
		}

		gr.state = FLAG_BLENDING;
		gr.blend_time = 0.f;
	}
}
//=================================================================================================
// Aktualizuje animacje
//=================================================================================================
void AnimeshInstance::Update(float dt)
{
	frame_end_info = false;
	frame_end_info2 = false;

	for( word i=0; i<ani->head.n_groups; ++i )
	{
		Group& gr = groups[i];

		// blending
		if( IS_SET(gr.state,FLAG_BLENDING) )
		{
			need_update = true;
			gr.blend_time += dt;
			if(gr.blend_time >= gr.blend_max)
				CLEAR_BIT(gr.state,FLAG_BLENDING);
		}

		// odtwarzaj animacj�
		if( IS_SET(gr.state,FLAG_PLAYING))
		{
			need_update = true;

			if(IS_SET(gr.state, FLAG_BLEND_WAIT))
			{
				if(IS_SET(gr.state, FLAG_BLENDING))
					continue;
			}

			// odtwarzaj od ty�u
			if( IS_SET(gr.state,FLAG_BACK) )
			{
				gr.time -= dt * gr.speed;
				if( gr.time < 0 ) // przekroczono czas animacji
				{
					// informacja o ko�cu animacji (do wywalenia)
					if( i == 0 )
						frame_end_info = true;
					else
						frame_end_info2 = true;
					if( IS_SET(gr.state,FLAG_ONCE) )
					{
						gr.time = 0;
						if( IS_SET(gr.state,FLAG_STOP_AT_END) )
							Stop(i);
						else
							Deactivate(i);
					}
					else
					{
						gr.time = fmod(gr.time, gr.anim->length) + gr.anim->length;
						if( gr.anim->n_frames == 1 )
						{
							gr.time = 0;
							Stop(i);
						}
					}
				}
			}
			else // odtwarzaj normalnie
			{
				gr.time += dt * gr.speed;
				if( gr.time >= gr.anim->length ) // przekroczono czas animacji
				{
					if( i == 0 )
						frame_end_info = true;
					else
						frame_end_info2 = true;
					if( IS_SET(gr.state,FLAG_ONCE) )
					{
						gr.time = gr.anim->length;
						if( IS_SET(gr.state,FLAG_STOP_AT_END) )
							Stop(i);
						else
							Deactivate(i);
					}
					else
					{
						gr.time = fmod(gr.time, gr.anim->length);
						if( gr.anim->n_frames == 1 )
						{
							gr.time = 0;
							Stop(i);
						}
					}
				}
			}
		}
	}
}

#define BLEND_TO_BIND_POSE -1

//====================================================================================================
// Ustawia ko�ci przed rysowaniem modelu
//====================================================================================================
void AnimeshInstance::SetupBones(MATRIX* mat_scale)
{
	if(!need_update)
		return;
	need_update = false;

	MATRIX BoneToParentPoseMat[32];
	D3DXMatrixIdentity( &BoneToParentPoseMat[0] );
	Animesh::KeyframeBone tmp_keyf;

	// oblicz przekszta�cenia dla ka�dej grupy
	const word n_groups = ani->head.n_groups;
	for(word bones_group=0; bones_group<n_groups; ++bones_group )
	{
		const Group& gr_bones = groups[bones_group];
		const vector<byte>& bones = ani->groups[bones_group].bones;
		int anim_group;

		// ustal z kt�r� animacj� ustala� blending
		anim_group = GetUseableGroup(bones_group);

		if( anim_group == BLEND_TO_BIND_POSE )
		{
			// nie ma �adnej animacji
			if( gr_bones.IsBlending() )
			{
				// jest blending pomi�dzy B--->0
				float bt = gr_bones.blend_time / gr_bones.blend_max;

				for( BoneIter it = bones.begin(), end = bones.end(); it != end; ++it )
				{
					const word b = *it;
					Animesh::KeyframeBone::Interpolate( tmp_keyf, blendb[b], blendb_zero, bt );
					tmp_keyf.Mix( BoneToParentPoseMat[b], ani->bones[b].mat );
				}
			}
			else
			{
				// brak blendingu, wszystko na zero
				for( BoneIter it = bones.begin(), end = bones.end(); it != end; ++it )
				{
					const word b = *it;
					BoneToParentPoseMat[b] = mat_zero * ani->bones[b].mat;
				}
			}
		}
		else
		{
			const Group& gr_anim = groups[anim_group];
			bool hit;
			const int index = gr_anim.GetFrameIndex(hit);
			const vector<Animesh::Keyframe>& frames = gr_anim.anim->frames;

			if( gr_anim.IsBlending() || gr_bones.IsBlending() )
			{
				// jest blending
				const float bt = (gr_bones.IsBlending() ? (gr_bones.blend_time / gr_bones.blend_max) :
					(gr_anim.blend_time / gr_anim.blend_max));

				if(hit)
				{
					// r�wne trafienie w klatk�
					const vector<Animesh::KeyframeBone>& keyf = frames[index].bones;
					for( BoneIter it = bones.begin(), end = bones.end(); it != end; ++it )
					{
						const word b = *it;
						Animesh::KeyframeBone::Interpolate( tmp_keyf, blendb[b], keyf[b-1], bt );
						tmp_keyf.Mix( BoneToParentPoseMat[b], ani->bones[b].mat );
					}
				}
				else
				{
					// trzeba interpolowa�
					const float t = (gr_anim.time - frames[index].time) / (frames[index+1].time - frames[index].time);
					const vector<Animesh::KeyframeBone>& keyf = frames[index].bones;
					const vector<Animesh::KeyframeBone>& keyf2 = frames[index+1].bones;

					for( BoneIter it = bones.begin(), end = bones.end(); it != end; ++it )
					{
						const word b = *it;
						Animesh::KeyframeBone::Interpolate( tmp_keyf, keyf[b-1], keyf2[b-1], t );
						Animesh::KeyframeBone::Interpolate( tmp_keyf, blendb[b], tmp_keyf, bt );
						tmp_keyf.Mix( BoneToParentPoseMat[b], ani->bones[b].mat );
					}
				}
			}
			else
			{
				// nie ma blendingu
				if(hit)
				{
					// r�wne trafienie w klatk�
					const vector<Animesh::KeyframeBone>& keyf = frames[index].bones;
					for( BoneIter it = bones.begin(), end = bones.end(); it != end; ++it )
					{
						const word b = *it;
						keyf[b-1].Mix( BoneToParentPoseMat[b], ani->bones[b].mat );
					}
				}
				else
				{
					// trzeba interpolowa�
					const float t = (gr_anim.time - frames[index].time) / (frames[index+1].time - frames[index].time);
					const vector<Animesh::KeyframeBone>& keyf = frames[index].bones;
					const vector<Animesh::KeyframeBone>& keyf2 = frames[index+1].bones;

					for( BoneIter it = bones.begin(), end = bones.end(); it != end; ++it )
					{
						const word b = *it;
						Animesh::KeyframeBone::Interpolate( tmp_keyf, keyf[b-1], keyf2[b-1], t );
						tmp_keyf.Mix( BoneToParentPoseMat[b], ani->bones[b].mat );
					}
				}
			}
		}
	}

	if(ptr)
		Predraw(ptr, BoneToParentPoseMat, 0);

	// Macierze przekszta�caj�ce ze wsp. danej ko�ci do wsp. modelu w ustalonej pozycji
	// (To obliczenie nale�a�oby po��czy� z poprzednim)
	MATRIX BoneToModelPoseMat[32];
	D3DXMatrixIdentity( &BoneToModelPoseMat[0] );
	for( word i=1; i<ani->head.n_bones; ++i )
	{
		const Animesh::Bone& bone = ani->bones[i];

		// Je�li to ko�� g��wna, przekszta�cenie z danej ko�ci do nadrz�dnej = z danej ko�ci do modelu
		// Je�li to nie ko�� g��wna, przekszta�cenie z danej ko�ci do modelu = z danej ko�ci do nadrz�dnej * z nadrz�dnej do modelu
		if( bone.parent == 0 )
			BoneToModelPoseMat[i] = BoneToParentPoseMat[i];
		else
			BoneToModelPoseMat[i] = BoneToParentPoseMat[i] * BoneToModelPoseMat[bone.parent];
	}

	//if(ptr)
	//	Predraw(ptr, BoneToModelPoseMat, 1);

	// przeskaluj ko�ci
	if(mat_scale)
	{
		for(int i=0; i<ani->head.n_bones; ++i)
			D3DXMatrixMultiply(&BoneToModelPoseMat[i], &BoneToModelPoseMat[i], &mat_scale[i]);
	}

	// Macierze zebrane ko�ci - przekszta�caj�ce z modelu do ko�ci w pozycji spoczynkowej * z ko�ci do modelu w pozycji bie��cej
	D3DXMatrixIdentity( &mat_bones[0] );
	for( word i=1; i<ani->head.n_bones; ++i )
		mat_bones[i] = ani->model_to_bone[i] * BoneToModelPoseMat[i];

 	//if(ptr)
 	//	Predraw(ptr, &mat_bones[0], 2);
}

//=================================================================================================
// Ustawia blending (przej�cia pomi�dzy animacjami)
//=================================================================================================
void AnimeshInstance::SetupBlending(int bones_group, bool first)
{
	int anim_group;
	const Group& gr_bones = groups[bones_group];
	const vector<byte>& bones = ani->groups[bones_group].bones;

	// nowe ustalanie z kt�rej grupy bra� animacj�!
	// teraz wybiera wed�ug priorytetu
	anim_group = GetUseableGroup(bones_group);

	if( anim_group == BLEND_TO_BIND_POSE )
	{
		// nie ma �adnej animacji
		if( gr_bones.IsBlending() )
		{
			// jest blending pomi�dzy B--->0
			const float bt = gr_bones.blend_time / gr_bones.blend_max;

			for( BoneIter it = bones.begin(), end = bones.end(); it != end; ++it )
			{
				const word b = *it;
				Animesh::KeyframeBone::Interpolate( blendb[b], blendb[b], blendb_zero, bt );
			}
		}
		else
		{
			// brak blendingu, wszystko na zero
			for( BoneIter it = bones.begin(), end = bones.end(); it != end; ++it )
				memcpy( &blendb[*it], &blendb_zero, sizeof(blendb_zero) );
		}
	}
	else
	{
		// jest jaka� animacja
		const Group& gr_anim = groups[anim_group];
		bool hit;
		const int index = gr_anim.GetFrameIndex(hit);
		const vector<Animesh::Keyframe>& frames = gr_anim.anim->frames;

		if( gr_anim.IsBlending() || gr_bones.IsBlending() )
		{
			// je�eli gr_anim == gr_bones to mo�na to zoptymalizowa�

			// jest blending
			const float bt = (gr_bones.IsBlending() ? (gr_bones.blend_time / gr_bones.blend_max) :
				(gr_anim.blend_time / gr_anim.blend_max));

			if(hit)
			{
				// r�wne trafienie w klatk�
				const vector<Animesh::KeyframeBone>& keyf = frames[index].bones;
				for( BoneIter it = bones.begin(), end = bones.end(); it != end; ++it )
				{
					const word b = *it;
					Animesh::KeyframeBone::Interpolate( blendb[b], blendb[b], keyf[b-1], bt );
				}
			}
			else
			{
				// trzeba interpolowa�
				const float t = (gr_anim.time - frames[index].time) / (frames[index+1].time - frames[index].time);
				const vector<Animesh::KeyframeBone>& keyf = frames[index].bones;
				const vector<Animesh::KeyframeBone>& keyf2 = frames[index+1].bones;
				Animesh::KeyframeBone tmp_keyf;

				for( BoneIter it = bones.begin(), end = bones.end(); it != end; ++it )
				{
					const word b = *it;
					Animesh::KeyframeBone::Interpolate( tmp_keyf, keyf[b-1], keyf2[b-1], t );
					Animesh::KeyframeBone::Interpolate( blendb[b], blendb[b], tmp_keyf, bt );
				}
			}
		}
		else
		{
			// nie ma blendingu
			if(hit)
			{
				// r�wne trafienie w klatk�
				const vector<Animesh::KeyframeBone>& keyf = frames[index].bones;
				for( BoneIter it = bones.begin(), end = bones.end(); it != end; ++it )
				{
					const word b = *it;
					blendb[b] = keyf[b-1];
				}
			}
			else
			{
				// trzeba interpolowa�
				const float t = (gr_anim.time - frames[index].time) / (frames[index+1].time - frames[index].time);
				const vector<Animesh::KeyframeBone>& keyf = frames[index].bones;
				const vector<Animesh::KeyframeBone>& keyf2 = frames[index+1].bones;

				for( BoneIter it = bones.begin(), end = bones.end(); it != end; ++it )
				{
					const word b = *it;
					Animesh::KeyframeBone::Interpolate( blendb[b], keyf[b-1], keyf2[b-1], t );
				}
			}
		}
	}

	// znajdz podrz�dne grupy kt�re nie s� aktywne i ustaw im blending
	if(first)
	{
		for( int group=0; group<ani->head.n_groups; ++group )
		{
			if( group != bones_group && (!groups[group].IsActive() ||
				groups[group].prio < gr_bones.prio))
			{
				SetupBlending(group, false);
				SET_BIT( groups[group].state, FLAG_BLENDING );
				groups[group].blend_time = 0;
			}
		}
	}
}

//=================================================================================================
// Czy jest jaki� blending?
//=================================================================================================
bool AnimeshInstance::IsBlending() const
{
	for(int i=0; i<ani->head.n_groups; ++i)
	{
		if(IS_SET(groups[i].state, FLAG_BLENDING))
			return true;
	}
	return false;
}

//=================================================================================================
// Zwraca najwy�szy priorytet animacji i jej grup�
//=================================================================================================
int AnimeshInstance::GetHighestPriority(uint& _group)
{
	int best = -1;

	for(uint i=0; i<uint(ani->head.n_groups); ++i)
	{
		if(groups[i].IsActive() && groups[i].prio > best)
		{
			best = groups[i].prio;
			_group = i;
		}
	}

	return best;
}

//=================================================================================================
// Zwraca grup� kt�rej ma u�ywa� dana grupa
//=================================================================================================
int AnimeshInstance::GetUseableGroup(uint group)
{
	uint top_group;
	int highest_prio = GetHighestPriority(top_group);
	if(highest_prio == -1)
	{
		// brak jakiejkolwiek animacji
		return BLEND_TO_BIND_POSE;
	}
	else if(groups[group].IsActive() && groups[group].prio == highest_prio)
	{
		// u�yj animacji z aktualnej grupy, to mo�e by� r�wnocze�nie 'top_group'
		return group;
	}
	else
	{
		// u�yj animacji z grupy z najwy�szym priorytetem
		return top_group;
	}
}

//=================================================================================================
// Czy�ci ko�ci tak �e jest bazowa poza, pozwala na renderowanie skrzyni bez animacji,
// Przyda�a by si� lepsza nazwa tej funkcji i u�ywanie jej w czasie odtwarzania animacji
//=================================================================================================
void AnimeshInstance::ClearBones()
{
	for(int i=0; i<ani->head.n_bones; ++i)
		D3DXMatrixIdentity(&mat_bones[i]);
	need_update = false;
}

//=================================================================================================
// Ustawia podan� animacje na koniec
//=================================================================================================
void AnimeshInstance::SetToEnd(cstring anim, MATRIX* mat_scale)
{
	assert(anim);

	Animesh::Animation* a = ani->GetAnimation(anim);
	assert(a);

	groups[0].anim = a;
	groups[0].blend_time = 0.f;
	groups[0].state = FLAG_GROUP_ACTIVE;
	groups[0].time = a->length;
	groups[0].used_group = 0;
	groups[0].prio = 3;

	if(ani->head.n_groups > 1)
	{
		for(int i=1; i<ani->head.n_groups; ++i)
		{
			groups[i].anim = nullptr;
			groups[i].state = 0;
			groups[i].used_group = 0;
		}
	}
	
	need_update = true;

	SetupBones(mat_scale);

	groups[0].state = 0;
}

//=================================================================================================
// Ustawia podan� animacje na koniec
//=================================================================================================
void AnimeshInstance::SetToEnd(Animesh::Animation* a, MATRIX* mat_scale)
{
	assert(a);

	groups[0].anim = a;
	groups[0].blend_time = 0.f;
	groups[0].state = FLAG_GROUP_ACTIVE;
	groups[0].time = a->length;
	groups[0].used_group = 0;
	groups[0].prio = 3;

	if(ani->head.n_groups > 1)
	{
		for(int i=1; i<ani->head.n_groups; ++i)
		{
			groups[i].anim = nullptr;
			groups[i].state = 0;
			groups[i].used_group = 0;
		}
	}

	need_update = true;

	SetupBones(mat_scale);

	groups[0].state = 0;
}

void AnimeshInstance::SetToEnd(MATRIX* mat_scale)
{
	groups[0].blend_time = 0.f;
	groups[0].state = FLAG_GROUP_ACTIVE;
	groups[0].time = groups[0].anim->length;
	groups[0].used_group = 0;
	groups[0].prio = 1;

	for(uint i=1; i<groups.size(); ++i)
	{
		groups[i].state = 0;
		groups[i].used_group = 0;
		groups[i].blend_time = 0;
	}

	need_update = true;

	SetupBones(mat_scale);
}

//=================================================================================================
// Resetuje animacj� z blendingiem
//=================================================================================================
void AnimeshInstance::ResetAnimation()
{
	SetupBlending(0);

	groups[0].time = 0.f;
	groups[0].blend_time = 0.f;
	SET_BIT(groups[0].state, FLAG_BLENDING | FLAG_PLAYING);
}

//=================================================================================================
// Zwraca czas blendingu w przedziale 0..1
//=================================================================================================
float AnimeshInstance::Group::GetBlendT() const
{
	if(IsBlending())
		return blend_time / blend_max;
	else
		return 1.f;
}

//=================================================================================================
// Wczytuje dane wierzcho�k�w z modelu (na razie dzia�a tylko dla VEC3)
//=================================================================================================
VertexData* Animesh::LoadVertexData(StreamReader& stream)
{
	// read and check header
	Header head;
	if(!stream.Read(head))
		throw "Failed to read file header.";
	if(memcmp(head.format,"QMSH",4) != 0)
		throw Format("Invalid file signature '%.4s'.", head.format);
	if(head.version < 13)
		throw Format("Invalid file version '%d'.", head.version);
	if(head.flags != ANIMESH_PHYSICS)
		throw Format("Invalid mesh flags '%d'.", head.flags);

	// skip camera data
	stream.Skip(sizeof(VEC3)*2);	

	VertexData* vd = new VertexData;
	vd->radius = head.radius;

	// read vertices
	uint size = sizeof(VEC3) * head.n_verts;
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

	return vd;
}

//=================================================================================================
Animesh::Point* Animesh::FindPoint(cstring name)
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
Animesh::Point* Animesh::FindNextPoint(cstring name, Point* point)
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

extern DWORD tmp;
extern char BUF[256];

//=================================================================================================
void AnimeshInstance::Save(HANDLE file)
{
	WriteFile(file, &frame_end_info, sizeof(frame_end_info), &tmp, nullptr);
	WriteFile(file, &frame_end_info2, sizeof(frame_end_info2), &tmp, nullptr);

	for(vector<Group>::iterator it = groups.begin(), end = groups.end(); it != end; ++it)
	{
		WriteFile(file, &it->time, sizeof(it->time), &tmp, nullptr);
		WriteFile(file, &it->speed, sizeof(it->speed), &tmp, nullptr);
		// nie zapisuj blendingu
		int state = it->state;
		state &= ~FLAG_BLENDING;
		WriteFile(file, &state, sizeof(state), &tmp, nullptr);
		WriteFile(file, &it->prio, sizeof(it->prio), &tmp, nullptr);
		WriteFile(file, &it->used_group, sizeof(it->used_group), &tmp, nullptr);
		if(it->anim)
		{
			byte len = (byte)it->anim->name.length();
			WriteFile(file, &len, sizeof(len), &tmp, nullptr);
			WriteFile(file, it->anim->name.c_str(), len, &tmp, nullptr);
		}
		else
		{
			byte len = 0;
			WriteFile(file, &len, sizeof(len), &tmp, nullptr);
		}
	}
}

//=================================================================================================
void AnimeshInstance::Load(HANDLE file)
{
	ReadFile(file, &frame_end_info, sizeof(frame_end_info), &tmp, nullptr);
	ReadFile(file, &frame_end_info2, sizeof(frame_end_info2), &tmp, nullptr);

	for(vector<Group>::iterator it = groups.begin(), end = groups.end(); it != end; ++it)
	{
		ReadFile(file, &it->time, sizeof(it->time), &tmp, nullptr);
		ReadFile(file, &it->speed, sizeof(it->speed), &tmp, nullptr);
		it->blend_time = 0.f;
		ReadFile(file, &it->state, sizeof(it->state), &tmp, nullptr);
		if(LOAD_VERSION < V_0_2_10)
		{
			// unused now
			int last_frame;
			ReadFile(file, &last_frame, sizeof(last_frame), &tmp, nullptr);
		}
		ReadFile(file, &it->prio, sizeof(it->prio), &tmp, nullptr);
		ReadFile(file, &it->used_group, sizeof(it->used_group), &tmp, nullptr);
		byte len;
		ReadFile(file, &len, sizeof(len), &tmp, nullptr);
		if(len)
		{
			BUF[len] = 0;
			ReadFile(file, BUF, len, &tmp, nullptr);
			it->anim = ani->GetAnimation(BUF);
		}
		else
			it->anim = nullptr;
	}

	need_update = true;
}

//=================================================================================================
void AnimeshInstance::Write(BitStream& stream) const
{
	int fai = 0;
	if(frame_end_info)
		fai |= 0x01;
	if(frame_end_info2)
		fai |= 0x02;
	stream.WriteCasted<byte>(fai);

	for(const Group& group : groups)
	{
		stream.Write(group.time);
		stream.Write(group.speed);
		stream.WriteCasted<byte>(group.state);
		stream.WriteCasted<byte>(group.prio);
		stream.WriteCasted<byte>(group.used_group);
		if(group.anim)
			WriteString1(stream, group.anim->name);
		else
			stream.WriteCasted<byte>(0);
	}
}

//=================================================================================================
bool AnimeshInstance::Read(BitStream& stream)
{
	int fai;

	if(!stream.ReadCasted<byte>(fai))
		return false;

	frame_end_info = IS_SET(fai, 0x01);
	frame_end_info2 = IS_SET(fai, 0x02);

	for(Group& group : groups)
	{
		if(stream.Read(group.time) &&
			stream.Read(group.speed) &&
			stream.ReadCasted<byte>(group.state) &&
			stream.ReadCasted<byte>(group.prio) &&
			stream.ReadCasted<byte>(group.used_group) &&
			ReadString1(stream))
		{
			if(BUF[0])
			{
				group.anim = ani->GetAnimation(BUF);
				if(!group.anim)
				{
					ERROR(Format("Missing animation '%s'.", BUF));
					return false;
				}
			}
			else
				group.anim = nullptr;
		}
		else
			return false;
	}

	need_update = true;
	return true;
}
