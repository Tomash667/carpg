// format modeli
#pragma once

//-----------------------------------------------------------------------------
#include "VertexDeclaration.h"
#include "Resource.h"
#include "Stream.h"

//-----------------------------------------------------------------------------
enum ANIMESH_FLAGS
{
	ANIMESH_TANGENTS = 1<<0,
	ANIMESH_ANIMATED = 1<<1,
	ANIMESH_STATIC = 1<<2,
	ANIMESH_PHYSICS = 1<<3,
	ANIMESH_SPLIT = 1<<4
};

//-----------------------------------------------------------------------------
struct Face
{
	word idx[3];

	word operator [] (int n) const
	{
		return idx[n];
	}
};

//-----------------------------------------------------------------------------
struct VertexData
{
	vector<VEC3> verts;
	vector<Face> faces;
	float radius;
};

/*---------------------------
TODO:
- eksporter do V13 jest zbugowany jeœli s¹ specjalne koœci, póki nie ma 31 koœci nie ma co siê tym martwiæ

NASTÊPNA WERSJA:
- kolor zamiast tekstury
- FVF i vertex size w nag³ówku
- wczytywanie ze strumienia
- brak model_to_bone, odrazu policzone
- lepsza organizacja pliku, wczytywanie wszystkiego za jednym zamachem i ustawianie wskaŸników
- brak zerowej koœci bo i po co
- materia³y
- mo¿liwe 32 bit indices jako flaga
- zmiana nazwy na Mesh, MeshInstance
- lepsze Predraw
*/
struct Animesh
{
	struct Header
	{
		char format[4];
		byte version, flags;
		word n_verts, n_tris, n_subs, n_bones, n_anims, n_points, n_groups;
		float radius;
		BOX bbox;
	};

	struct Submesh
	{
		word first; // pierwszy strójk¹t do narysowania
		word tris; // ile trójk¹tów narysowaæ
		word min_ind; // odpowiednik parametru DrawIndexedPrimitive - MinIndex (tylko wyra¿ony w trójk¹tach)
		word n_ind; // odpowiednik parametru DrawIndexedPrimitive - NumVertices (tylko wyra¿ony w trójk¹tach)
		string name;//, normal_name, specular_name;
		TextureResource* tex, *tex_normal, *tex_specular;
		VEC3 specular_color;
		float specular_intensity;
		int specular_hardness;
		float normal_factor, specular_factor, specular_color_factor;

		static const uint MIN_SIZE = 10;
	};

	struct BoneGroup
	{
		word parent;
		string name;
		vector<byte> bones;

		static const uint MIN_SIZE = 4;
	};

	struct Bone
	{
		word id;
		word parent;
		MATRIX mat;
		string name;
		vector<word> childs;

		static const uint MIN_SIZE = 51;
	};

	struct KeyframeBone
	{
		VEC3 pos;
		QUAT rot;
		float scale;

		void Mix( MATRIX& out, const MATRIX& mul ) const;
		static void Interpolate( KeyframeBone& out, const KeyframeBone& k, const KeyframeBone& k2, float t );
	};

	struct Keyframe
	{
		float time;
		vector<KeyframeBone> bones;
	};

	struct Animation
	{
		string name;
		float length;
		word n_frames;
		vector<Keyframe> frames;

		static const uint MIN_SIZE = 7;

		int GetFrameIndex(float time, bool& hit);
	};

	struct Point
	{
		enum Type : word
		{
			OTHER,
			SPHERE,
			BOX
		};

		string name;
		MATRIX mat;
		VEC3 rot;
		word bone;
		Type type;
		VEC3 size;

		static const uint MIN_SIZE = 73;

		bool IsSphere() const { return type == SPHERE; }
		bool IsBox() const { return type == BOX; }
	};

	struct Split
	{
		VEC3 pos;
		float radius;
		BOX box;
	};

	Animesh();
	~Animesh();

	static void MeshInit();

	void SetupBoneMatrices();
	void Load(StreamReader& stream, IDirect3DDevice9* device);
	static VertexData* LoadVertexData(StreamReader& stream);
	Animation* GetAnimation(cstring name);
	Bone* GetBone(cstring name);
	Point* GetPoint(cstring name);
	TEX GetTexture(uint idx) const
	{
		assert(idx < head.n_subs && subs[idx].tex && subs[idx].tex->data);
		return (TEX)subs[idx].tex->data;
	}

	bool IsAnimated() const { return IS_SET(head.flags,ANIMESH_ANIMATED); }
	bool IsStatic() const { return IS_SET(head.flags,ANIMESH_STATIC); }

	void GetKeyframeData(KeyframeBone& keyframe, Animation* ani, uint bone, float time);
	// jeœli szuka hit to zwróci te¿ dla hit1, hit____ itp (u¿ywane dla boxów broni które siê powtarzaj¹)
	Point* FindPoint(cstring name);
	Point* FindNextPoint(cstring name, Point* point);

	void DrawSubmesh(IDirect3DDevice9* device, uint i)
	{
		assert(device && i < head.n_subs);
		Submesh& sub = subs[i];
		V( device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, sub.min_ind, sub.n_ind, sub.first*3, sub.tris) );
	}

	Header head;
	VB vb;
	IB ib;
	VertexDeclarationId vertex_decl;
	uint vertex_size;
	//int n_real_bones;
	vector<Submesh> subs;
	vector<Bone> bones;
	vector<Animation> anims;
	vector<MATRIX> model_to_bone;
	vector<Point> attach_points;
	vector<BoneGroup> groups;
	vector<Split> splits;
	VEC3 cam_pos, cam_target, cam_up;
	MeshResource* res;
};

//-----------------------------------------------------------------------------
// flagi u¿ywane przy odtwarzaniu animacji
enum PLAY_FLAGS
{
	// odtwarzaj raz, w przeciwnym razie jest zapêtlone
	// po odtworzeniu animacji na grupie wywo³ywana jest funkcja Deactive(), jeœli jest to grupa
	// podrzêdna to nadrzêdna jest odgrywana
	PLAY_ONCE = 0x01,
	// odtwarzaj od ty³u
	PLAY_BACK = 0x02,
	// wy³¹cza blending dla tej animacji
	PLAY_NO_BLEND = 0x04,
	// ignoruje wywo³anie Play() je¿eli jest ju¿ ta animacja
	PLAY_IGNORE = 0x08,
	PLAY_STOP_AT_END = 0x10,
	// priorytet animacji
	PLAY_PRIO0 = 0,
	PLAY_PRIO1 = 0x20,
	PLAY_PRIO2 = 0x40,
	PLAY_PRIO3 = 0x60,
	// odtwarza animacjê gdy skoñczy siê blending
	PLAY_BLEND_WAIT = 0x100,
	// resetuje speed i blend_max przy zmianie animacji
	PLAY_RESTORE = 0x200
};

//-----------------------------------------------------------------------------
struct SubOverride
{
	SubOverride() : tex(nullptr), draw(true)
	{
	}

	TEX tex;
	bool draw;
};

//-----------------------------------------------------------------------------
// obiekt wykorzystuj¹cy Animesh
// zwalnia tekstury override przy niszczeniu
//-----------------------------------------------------------------------------
struct AnimeshInstance
{
	enum FLAGS
	{
		FLAG_PLAYING = 1<<0,
		FLAG_ONCE = 1<<1,
		FLAG_BACK = 1<<2,
		FLAG_GROUP_ACTIVE = 1<<3,
		FLAG_BLENDING = 1<<4,
		FLAG_STOP_AT_END = 1<<5,
		FLAG_BLEND_WAIT = 1<<6,
		FLAG_RESTORE = 1<<7
		// jeœli bêdzie wiêcej flagi potrzeba zmian w Read/Write
	};

	struct Group
	{
		Group() : anim(nullptr), state(0), speed(1.f), prio(0), blend_max(0.33f)
		{
		}

		float time, speed, blend_time, blend_max;
		int state, prio, used_group;
		Animesh::Animation* anim;

		int GetFrameIndex(bool& hit) const { return anim->GetFrameIndex(time,hit); }
		float GetBlendT() const;
		bool IsActive() const { return IS_SET(state,FLAG_GROUP_ACTIVE); }
		bool IsBlending() const { return IS_SET(state,FLAG_BLENDING); }
		bool IsPlaying() const { return IS_SET(state,FLAG_PLAYING); }
		float GetProgress() const { return time / anim->length; }
	};
	typedef vector<byte>::const_iterator BoneIter;

	explicit AnimeshInstance(Animesh* animesh);
	~AnimeshInstance();

	// kontynuuj odtwarzanie animacji
	void Play(int group=0)
	{
		SET_BIT(groups[group].state, FLAG_PLAYING);
	}
	// odtwarzaj animacjê
	void Play(Animesh::Animation* anim, int flags, int group);
	// odtwarzaj animacjê o podanej nazwie
	void Play(cstring name, int flags, int group=0)
	{
		Animesh::Animation* anim = ani->GetAnimation(name);
		assert(anim);
		Play(anim, flags, group);
	}
	// zatrzymaj animacjê
	void Stop(int group=0)
	{
		CLEAR_BIT(groups[group].state,FLAG_PLAYING);
	}
	// deazktywój grupê
	void Deactivate(int group=0);
	// aktualizacja animacji
	void Update(float dt);
	// ustawianie blendingu
	void SetupBlending(int grupa, bool first=true);
	// ustawianie koœci	
	void SetupBones(MATRIX* mat_scale=nullptr);
	float GetProgress() const
	{
		return groups[0].GetProgress();
	}
	float GetProgress2() const
	{
		assert(ani->head.n_groups > 1);
		return groups[1].GetProgress();
	}
	float GetProgress(int group) const
	{
		assert(in_range(group, 0, ani->head.n_groups-1));
		return groups[group].GetProgress();
	}
	void ClearBones();
	void SetToEnd(cstring anim, MATRIX* mat_scale = nullptr);
	void SetToEnd(Animesh::Animation* anim, MATRIX* mat_scale = nullptr);
	void SetToEnd(MATRIX* mat_scale = nullptr);
	void ResetAnimation();
	void Save(HANDLE file);
	void Load(HANDLE file);
	void Write(BitStream& stream) const;
	bool Read(BitStream& stream);

	int GetHighestPriority(uint& group);
	int GetUseableGroup(uint group);
	bool GetEndResultClear(uint g)
	{
		bool r;
		if(g == 0)
		{
			r = frame_end_info;
			frame_end_info = false;
		}
		else
		{
			r = frame_end_info2;
			frame_end_info2 = false;
		}
		return r;
	}
	bool GetEndResult(uint g) const
	{
		if(g == 0)
			return frame_end_info;
		else
			return frame_end_info2;
	}

	bool IsBlending() const;

	Animesh* ani;
	bool frame_end_info, frame_end_info2, need_update;
	vector<MATRIX> mat_bones;
	vector<Animesh::KeyframeBone> blendb;
	vector<Group> groups;
	void* ptr;
	static void (*Predraw)(void*,MATRIX*,int);
};

typedef Animesh Mesh;
typedef AnimeshInstance MeshInstance;
