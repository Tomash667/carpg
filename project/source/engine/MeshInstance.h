#pragma once

//-----------------------------------------------------------------------------
#include "Mesh.h"

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
// obiekt wykorzystuj¹cy Mesh
// zwalnia tekstury override przy niszczeniu
//-----------------------------------------------------------------------------
struct MeshInstance
{
	enum FLAGS
	{
		FLAG_PLAYING = 1 << 0,
		FLAG_ONCE = 1 << 1,
		FLAG_BACK = 1 << 2,
		FLAG_GROUP_ACTIVE = 1 << 3,
		FLAG_BLENDING = 1 << 4,
		FLAG_STOP_AT_END = 1 << 5,
		FLAG_BLEND_WAIT = 1 << 6,
		FLAG_RESTORE = 1 << 7,
		FLAG_UPDATED = 1 << 8
		// jeœli bêdzie wiêcej flagi potrzeba zmian w Read/Write
	};

	struct Group
	{
		Group() : anim(nullptr), state(0), speed(1.f), prio(0), blend_max(0.33f)
		{
		}

		float time, speed, blend_time, blend_max;
		int state, prio, used_group;
		Mesh::Animation* anim;

		int GetFrameIndex(bool& hit) const
		{
			return anim->GetFrameIndex(time, hit);
		}
		float GetBlendT() const;
		bool IsActive() const
		{
			return IS_SET(state, FLAG_GROUP_ACTIVE);
		}
		bool IsBlending() const
		{
			return IS_SET(state, FLAG_BLENDING);
		}
		bool IsPlaying() const
		{
			return IS_SET(state, FLAG_PLAYING);
		}
		float GetProgress() const
		{
			return time / anim->length;
		}
	};
	typedef vector<byte>::const_iterator BoneIter;

	explicit MeshInstance(Mesh* mesh, bool preload = false);

	// kontynuuj odtwarzanie animacji
	void Play(int group = 0)
	{
		SET_BIT(groups[group].state, FLAG_PLAYING);
	}
	// odtwarzaj animacjê
	void Play(Mesh::Animation* anim, int flags, int group);
	// odtwarzaj animacjê o podanej nazwie
	void Play(cstring name, int flags, int group = 0)
	{
		Mesh::Animation* anim = mesh->GetAnimation(name);
		assert(anim);
		Play(anim, flags, group);
	}
	// zatrzymaj animacjê
	void Stop(int group = 0)
	{
		CLEAR_BIT(groups[group].state, FLAG_PLAYING);
	}
	// deazktywój grupê
	void Deactivate(int group = 0, bool in_update = false);
	// aktualizacja animacji
	void Update(float dt);
	// ustawianie blendingu
	void SetupBlending(int grupa, bool first = true, bool in_update = false);
	// ustawianie koœci
	void SetupBones(Matrix* mat_scale = nullptr);
	float GetProgress() const
	{
		return groups[0].GetProgress();
	}
	float GetProgress2() const
	{
		assert(mesh->head.n_groups > 1);
		return groups[1].GetProgress();
	}
	float GetProgress(int group) const
	{
		assert(InRange(group, 0, mesh->head.n_groups - 1));
		return groups[group].GetProgress();
	}
	void ClearBones();
	void SetToEnd(cstring anim, Matrix* mat_scale = nullptr)
	{
		Mesh::Animation* a = mesh->GetAnimation(anim);
		SetToEnd(a, mat_scale);
	}
	void SetToEnd(Mesh::Animation* anim, Matrix* mat_scale = nullptr);
	void SetToEnd(Matrix* mat_scale = nullptr);
	void ResetAnimation();
	void Save(HANDLE file);
	void Load(HANDLE file);
	void Write(BitStream& stream) const;
	bool Read(BitStream& stream);
	bool ApplyPreload(Mesh* mesh);

	int GetHighestPriority(uint& group);
	int GetUsableGroup(uint group);
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

	Mesh* mesh;
	bool frame_end_info, frame_end_info2, need_update, preload;
	vector<Matrix> mat_bones;
	vector<Mesh::KeyframeBone> blendb;
	vector<Group> groups;
	void* ptr;
	static void(*Predraw)(void*, Matrix*, int);
};
