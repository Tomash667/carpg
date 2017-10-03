#include "Pch.h"
#include "Core.h"
#include "MeshInstance.h"
#include "BitStreamFunc.h"

//---------------------------
const int BLEND_TO_BIND_POSE = -1;
void(*MeshInstance::Predraw)(void*, Matrix*, int) = nullptr;
Mesh::KeyframeBone blendb_zero;

//=================================================================================================
// Inicjalizacja obiektów u¿ywanych do animacji modeli
//=================================================================================================
void Mesh::MeshInit()
{
	blendb_zero.scale = 1.f;
	blendb_zero.rot = Quat::Identity;
	blendb_zero.pos = Vec3::Zero;
};

//=================================================================================================
// Konstruktor instancji Mesh
//=================================================================================================
MeshInstance::MeshInstance(Mesh* mesh, bool preload) : mesh(mesh), need_update(true), frame_end_info(false),
frame_end_info2(false), ptr(nullptr), preload(preload)
{
	if(!preload)
	{
		assert(mesh && mesh->IsLoaded());

		mat_bones.resize(mesh->head.n_bones);
		blendb.resize(mesh->head.n_bones);
		groups.resize(mesh->head.n_groups);
	}
}

//=================================================================================================
// Odtwarza podan¹ animacjê, szybsze bo nie musi szukaæ nazwy
//=================================================================================================
void MeshInstance::Play(Mesh::Animation* anim, int flags, int group)
{
	assert(anim && InRange(group, 0, mesh->head.n_groups - 1));

	Group& gr = groups[group];

	// ignoruj animacjê
	if(IS_SET(flags, PLAY_IGNORE) && gr.anim == anim)
		return;

	// resetuj szybkoœæ i blending
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

	// ustaw animacjê
	gr.anim = anim;
	gr.prio = ((flags & 0x60) >> 5);
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
		for(int g = 0; g < mesh->head.n_groups; ++g)
		{
			if(g != group && (!groups[g].IsActive() || groups[g].prio < gr.prio))
				CLEAR_BIT(groups[g].state, FLAG_BLENDING);
		}
	}
}

//=================================================================================================
// Wy³¹cz grupê
//=================================================================================================
void MeshInstance::Deactivate(int group, bool in_update)
{
	assert(InRange(group, 0, mesh->head.n_groups - 1));

	Group& gr = groups[group];

	if(IS_SET(gr.state, FLAG_GROUP_ACTIVE))
	{
		SetupBlending(group, true, in_update);

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
void MeshInstance::Update(float dt)
{
	frame_end_info = false;
	frame_end_info2 = false;

	for(word i = 0; i < mesh->head.n_groups; ++i)
	{
		Group& gr = groups[i];

		if(IS_SET(gr.state, FLAG_UPDATED))
		{
			CLEAR_BIT(gr.state, FLAG_UPDATED);
			continue;
		}

		// blending
		if(IS_SET(gr.state, FLAG_BLENDING))
		{
			need_update = true;
			gr.blend_time += dt;
			if(gr.blend_time >= gr.blend_max)
				CLEAR_BIT(gr.state, FLAG_BLENDING);
		}

		// odtwarzaj animacjê
		if(IS_SET(gr.state, FLAG_PLAYING))
		{
			need_update = true;

			if(IS_SET(gr.state, FLAG_BLEND_WAIT))
			{
				if(IS_SET(gr.state, FLAG_BLENDING))
					continue;
			}

			// odtwarzaj od ty³u
			if(IS_SET(gr.state, FLAG_BACK))
			{
				gr.time -= dt * gr.speed;
				if(gr.time < 0) // przekroczono czas animacji
				{
					// informacja o koñcu animacji (do wywalenia)
					if(i == 0)
						frame_end_info = true;
					else
						frame_end_info2 = true;
					if(IS_SET(gr.state, FLAG_ONCE))
					{
						gr.time = 0;
						if(IS_SET(gr.state, FLAG_STOP_AT_END))
							Stop(i);
						else
							Deactivate(i, true);
					}
					else
					{
						gr.time = fmod(gr.time, gr.anim->length) + gr.anim->length;
						if(gr.anim->n_frames == 1)
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
				if(gr.time >= gr.anim->length) // przekroczono czas animacji
				{
					if(i == 0)
						frame_end_info = true;
					else
						frame_end_info2 = true;
					if(IS_SET(gr.state, FLAG_ONCE))
					{
						gr.time = gr.anim->length;
						if(IS_SET(gr.state, FLAG_STOP_AT_END))
							Stop(i);
						else
							Deactivate(i, true);
					}
					else
					{
						gr.time = fmod(gr.time, gr.anim->length);
						if(gr.anim->n_frames == 1)
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

//====================================================================================================
// Ustawia koœci przed rysowaniem modelu
//====================================================================================================
void MeshInstance::SetupBones(Matrix* mat_scale)
{
	if(!need_update)
		return;
	need_update = false;

	Matrix BoneToParentPoseMat[32];
	BoneToParentPoseMat[0] = Matrix::IdentityMatrix;
	Mesh::KeyframeBone tmp_keyf;

	// oblicz przekszta³cenia dla ka¿dej grupy
	const word n_groups = mesh->head.n_groups;
	for(word bones_group = 0; bones_group < n_groups; ++bones_group)
	{
		const Group& gr_bones = groups[bones_group];
		const vector<byte>& bones = mesh->groups[bones_group].bones;
		int anim_group;

		// ustal z któr¹ animacj¹ ustalaæ blending
		anim_group = GetUsableGroup(bones_group);

		if(anim_group == BLEND_TO_BIND_POSE)
		{
			// nie ma ¿adnej animacji
			if(gr_bones.IsBlending())
			{
				// jest blending pomiêdzy B--->0
				float bt = gr_bones.blend_time / gr_bones.blend_max;

				for(BoneIter it = bones.begin(), end = bones.end(); it != end; ++it)
				{
					const word b = *it;
					Mesh::KeyframeBone::Interpolate(tmp_keyf, blendb[b], blendb_zero, bt);
					tmp_keyf.Mix(BoneToParentPoseMat[b], mesh->bones[b].mat);
				}
			}
			else
			{
				// brak blendingu, wszystko na zero
				for(BoneIter it = bones.begin(), end = bones.end(); it != end; ++it)
				{
					const word b = *it;
					BoneToParentPoseMat[b] = mesh->bones[b].mat;
				}
			}
		}
		else
		{
			const Group& gr_anim = groups[anim_group];
			bool hit;
			const int index = gr_anim.GetFrameIndex(hit);
			const vector<Mesh::Keyframe>& frames = gr_anim.anim->frames;

			if(gr_anim.IsBlending() || gr_bones.IsBlending())
			{
				// jest blending
				const float bt = (gr_bones.IsBlending() ? (gr_bones.blend_time / gr_bones.blend_max) :
					(gr_anim.blend_time / gr_anim.blend_max));

				if(hit)
				{
					// równe trafienie w klatkê
					const vector<Mesh::KeyframeBone>& keyf = frames[index].bones;
					for(BoneIter it = bones.begin(), end = bones.end(); it != end; ++it)
					{
						const word b = *it;
						Mesh::KeyframeBone::Interpolate(tmp_keyf, blendb[b], keyf[b - 1], bt);
						tmp_keyf.Mix(BoneToParentPoseMat[b], mesh->bones[b].mat);
					}
				}
				else
				{
					// trzeba interpolowaæ
					const float t = (gr_anim.time - frames[index].time) / (frames[index + 1].time - frames[index].time);
					const vector<Mesh::KeyframeBone>& keyf = frames[index].bones;
					const vector<Mesh::KeyframeBone>& keyf2 = frames[index + 1].bones;

					for(BoneIter it = bones.begin(), end = bones.end(); it != end; ++it)
					{
						const word b = *it;
						Mesh::KeyframeBone::Interpolate(tmp_keyf, keyf[b - 1], keyf2[b - 1], t);
						Mesh::KeyframeBone::Interpolate(tmp_keyf, blendb[b], tmp_keyf, bt);
						tmp_keyf.Mix(BoneToParentPoseMat[b], mesh->bones[b].mat);
					}
				}
			}
			else
			{
				// nie ma blendingu
				if(hit)
				{
					// równe trafienie w klatkê
					const vector<Mesh::KeyframeBone>& keyf = frames[index].bones;
					for(BoneIter it = bones.begin(), end = bones.end(); it != end; ++it)
					{
						const word b = *it;
						keyf[b - 1].Mix(BoneToParentPoseMat[b], mesh->bones[b].mat);
					}
				}
				else
				{
					// trzeba interpolowaæ
					const float t = (gr_anim.time - frames[index].time) / (frames[index + 1].time - frames[index].time);
					const vector<Mesh::KeyframeBone>& keyf = frames[index].bones;
					const vector<Mesh::KeyframeBone>& keyf2 = frames[index + 1].bones;

					for(BoneIter it = bones.begin(), end = bones.end(); it != end; ++it)
					{
						const word b = *it;
						Mesh::KeyframeBone::Interpolate(tmp_keyf, keyf[b - 1], keyf2[b - 1], t);
						tmp_keyf.Mix(BoneToParentPoseMat[b], mesh->bones[b].mat);
					}
				}
			}
		}
	}

	if(ptr)
		Predraw(ptr, BoneToParentPoseMat, 0);

	// Macierze przekszta³caj¹ce ze wsp. danej koœci do wsp. modelu w ustalonej pozycji
	// (To obliczenie nale¿a³oby po³¹czyæ z poprzednim)
	Matrix BoneToModelPoseMat[32];
	BoneToModelPoseMat[0] = Matrix::IdentityMatrix;
	for(word i = 1; i < mesh->head.n_bones; ++i)
	{
		const Mesh::Bone& bone = mesh->bones[i];

		// Jeœli to koœæ g³ówna, przekszta³cenie z danej koœci do nadrzêdnej = z danej koœci do modelu
		// Jeœli to nie koœæ g³ówna, przekszta³cenie z danej koœci do modelu = z danej koœci do nadrzêdnej * z nadrzêdnej do modelu
		if(bone.parent == 0)
			BoneToModelPoseMat[i] = BoneToParentPoseMat[i];
		else
			BoneToModelPoseMat[i] = BoneToParentPoseMat[i] * BoneToModelPoseMat[bone.parent];
	}
	
	// przeskaluj koœci
	if(mat_scale)
	{
		for(int i = 0; i < mesh->head.n_bones; ++i)
			BoneToModelPoseMat[i] = BoneToModelPoseMat[i] * mat_scale[i];
	}

	// Macierze zebrane koœci - przekszta³caj¹ce z modelu do koœci w pozycji spoczynkowej * z koœci do modelu w pozycji bie¿¹cej
	mat_bones[0] = Matrix::IdentityMatrix;
	for(word i = 1; i < mesh->head.n_bones; ++i)
		mat_bones[i] = mesh->model_to_bone[i] * BoneToModelPoseMat[i];
}

//=================================================================================================
// Ustawia blending (przejœcia pomiêdzy animacjami)
//=================================================================================================
void MeshInstance::SetupBlending(int bones_group, bool first, bool in_update)
{
	int anim_group;
	const Group& gr_bones = groups[bones_group];
	const vector<byte>& bones = mesh->groups[bones_group].bones;

	// nowe ustalanie z której grupy braæ animacjê!
	// teraz wybiera wed³ug priorytetu
	anim_group = GetUsableGroup(bones_group);

	if(anim_group == BLEND_TO_BIND_POSE)
	{
		// nie ma ¿adnej animacji
		if(gr_bones.IsBlending())
		{
			// jest blending pomiêdzy B--->0
			const float bt = gr_bones.blend_time / gr_bones.blend_max;

			for(BoneIter it = bones.begin(), end = bones.end(); it != end; ++it)
			{
				const word b = *it;
				Mesh::KeyframeBone::Interpolate(blendb[b], blendb[b], blendb_zero, bt);
			}
		}
		else
		{
			// brak blendingu, wszystko na zero
			for(BoneIter it = bones.begin(), end = bones.end(); it != end; ++it)
				memcpy(&blendb[*it], &blendb_zero, sizeof(blendb_zero));
		}
	}
	else
	{
		// jest jakaœ animacja
		const Group& gr_anim = groups[anim_group];
		bool hit;
		const int index = gr_anim.GetFrameIndex(hit);
		const vector<Mesh::Keyframe>& frames = gr_anim.anim->frames;

		if(gr_anim.IsBlending() || gr_bones.IsBlending())
		{
			// je¿eli gr_anim == gr_bones to mo¿na to zoptymalizowaæ

			// jest blending
			const float bt = (gr_bones.IsBlending() ? (gr_bones.blend_time / gr_bones.blend_max) :
				(gr_anim.blend_time / gr_anim.blend_max));

			if(hit)
			{
				// równe trafienie w klatkê
				const vector<Mesh::KeyframeBone>& keyf = frames[index].bones;
				for(BoneIter it = bones.begin(), end = bones.end(); it != end; ++it)
				{
					const word b = *it;
					Mesh::KeyframeBone::Interpolate(blendb[b], blendb[b], keyf[b - 1], bt);
				}
			}
			else
			{
				// trzeba interpolowaæ
				const float t = (gr_anim.time - frames[index].time) / (frames[index + 1].time - frames[index].time);
				const vector<Mesh::KeyframeBone>& keyf = frames[index].bones;
				const vector<Mesh::KeyframeBone>& keyf2 = frames[index + 1].bones;
				Mesh::KeyframeBone tmp_keyf;

				for(BoneIter it = bones.begin(), end = bones.end(); it != end; ++it)
				{
					const word b = *it;
					Mesh::KeyframeBone::Interpolate(tmp_keyf, keyf[b - 1], keyf2[b - 1], t);
					Mesh::KeyframeBone::Interpolate(blendb[b], blendb[b], tmp_keyf, bt);
				}
			}
		}
		else
		{
			// nie ma blendingu
			if(hit)
			{
				// równe trafienie w klatkê
				const vector<Mesh::KeyframeBone>& keyf = frames[index].bones;
				for(BoneIter it = bones.begin(), end = bones.end(); it != end; ++it)
				{
					const word b = *it;
					blendb[b] = keyf[b - 1];
				}
			}
			else
			{
				// trzeba interpolowaæ
				const float t = (gr_anim.time - frames[index].time) / (frames[index + 1].time - frames[index].time);
				const vector<Mesh::KeyframeBone>& keyf = frames[index].bones;
				const vector<Mesh::KeyframeBone>& keyf2 = frames[index + 1].bones;

				for(BoneIter it = bones.begin(), end = bones.end(); it != end; ++it)
				{
					const word b = *it;
					Mesh::KeyframeBone::Interpolate(blendb[b], keyf[b - 1], keyf2[b - 1], t);
				}
			}
		}
	}

	// znajdz podrzêdne grupy które nie s¹ aktywne i ustaw im blending
	if(first)
	{
		for(int group = 0; group < mesh->head.n_groups; ++group)
		{
			auto& gr = groups[group];
			if(group != bones_group && (!gr.IsActive() || gr.prio < gr_bones.prio))
			{
				SetupBlending(group, false);
				SET_BIT(gr.state, FLAG_BLENDING);
				if(in_update && group > bones_group)
					SET_BIT(gr.state, FLAG_UPDATED);
				gr.blend_time = 0;
			}
		}
	}
}

//=================================================================================================
// Czy jest jakiœ blending?
//=================================================================================================
bool MeshInstance::IsBlending() const
{
	for(int i = 0; i < mesh->head.n_groups; ++i)
	{
		if(IS_SET(groups[i].state, FLAG_BLENDING))
			return true;
	}
	return false;
}

//=================================================================================================
// Zwraca najwy¿szy priorytet animacji i jej grupê
//=================================================================================================
int MeshInstance::GetHighestPriority(uint& _group)
{
	int best = -1;

	for(uint i = 0; i < uint(mesh->head.n_groups); ++i)
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
// Zwraca grupê której ma u¿ywaæ dana grupa
//=================================================================================================
int MeshInstance::GetUsableGroup(uint group)
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
		// u¿yj animacji z aktualnej grupy, to mo¿e byæ równoczeœnie 'top_group'
		return group;
	}
	else
	{
		// u¿yj animacji z grupy z najwy¿szym priorytetem
		return top_group;
	}
}

//=================================================================================================
// Czyœci koœci tak ¿e jest bazowa poza, pozwala na renderowanie skrzyni bez animacji,
// Przyda³a by siê lepsza nazwa tej funkcji i u¿ywanie jej w czasie odtwarzania animacji
//=================================================================================================
void MeshInstance::ClearBones()
{
	for(int i = 0; i < mesh->head.n_bones; ++i)
		mat_bones[i] = Matrix::IdentityMatrix;
	need_update = false;
}

//=================================================================================================
// Ustawia podan¹ animacje na koniec
//=================================================================================================
void MeshInstance::SetToEnd(Mesh::Animation* a, Matrix* mat_scale)
{
	assert(a);

	groups[0].anim = a;
	groups[0].blend_time = 0.f;
	groups[0].state = FLAG_GROUP_ACTIVE;
	groups[0].time = a->length;
	groups[0].used_group = 0;
	groups[0].prio = 3;

	if(mesh->head.n_groups > 1)
	{
		for(int i = 1; i < mesh->head.n_groups; ++i)
		{
			groups[i].anim = nullptr;
			groups[i].state = 0;
			groups[i].used_group = 0;
			groups[i].time = groups[0].time;
			groups[i].blend_time = groups[0].blend_time;
		}
	}

	need_update = true;

	SetupBones(mat_scale);
}

//=================================================================================================
void MeshInstance::SetToEnd(Matrix* mat_scale)
{
	groups[0].blend_time = 0.f;
	groups[0].state = FLAG_GROUP_ACTIVE;
	groups[0].time = groups[0].anim->length;
	groups[0].used_group = 0;
	groups[0].prio = 1;

	for(uint i = 1; i < groups.size(); ++i)
	{
		groups[i].state = 0;
		groups[i].used_group = 0;
		groups[i].blend_time = 0;
	}

	need_update = true;

	SetupBones(mat_scale);
}

//=================================================================================================
// Resetuje animacjê z blendingiem
//=================================================================================================
void MeshInstance::ResetAnimation()
{
	SetupBlending(0);

	groups[0].time = 0.f;
	groups[0].blend_time = 0.f;
	SET_BIT(groups[0].state, FLAG_BLENDING | FLAG_PLAYING);
}

//=================================================================================================
// Zwraca czas blendingu w przedziale 0..1
//=================================================================================================
float MeshInstance::Group::GetBlendT() const
{
	if(IsBlending())
		return blend_time / blend_max;
	else
		return 1.f;
}

//=================================================================================================
void MeshInstance::Save(HANDLE file)
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
void MeshInstance::Load(HANDLE file)
{
	ReadFile(file, &frame_end_info, sizeof(frame_end_info), &tmp, nullptr);
	ReadFile(file, &frame_end_info2, sizeof(frame_end_info2), &tmp, nullptr);

	for(vector<Group>::iterator it = groups.begin(), end = groups.end(); it != end; ++it)
	{
		ReadFile(file, &it->time, sizeof(it->time), &tmp, nullptr);
		ReadFile(file, &it->speed, sizeof(it->speed), &tmp, nullptr);
		it->blend_time = 0.f;
		ReadFile(file, &it->state, sizeof(it->state), &tmp, nullptr);
		ReadFile(file, &it->prio, sizeof(it->prio), &tmp, nullptr);
		ReadFile(file, &it->used_group, sizeof(it->used_group), &tmp, nullptr);
		byte len;
		ReadFile(file, &len, sizeof(len), &tmp, nullptr);
		if(len)
		{
			BUF[len] = 0;
			ReadFile(file, BUF, len, &tmp, nullptr);
			it->anim = mesh->GetAnimation(BUF);
		}
		else
			it->anim = nullptr;
	}

	need_update = true;
}

//=================================================================================================
void MeshInstance::Write(BitStream& stream) const
{
	int fai = 0;
	if(frame_end_info)
		fai |= 0x01;
	if(frame_end_info2)
		fai |= 0x02;
	stream.WriteCasted<byte>(fai);
	stream.WriteCasted<byte>(groups.size());

	for(const Group& group : groups)
	{
		stream.Write(group.time);
		stream.Write(group.speed);
		stream.Write(group.state);
		stream.WriteCasted<byte>(group.prio);
		stream.WriteCasted<byte>(group.used_group);
		if(group.anim)
			WriteString1(stream, group.anim->name);
		else
			stream.WriteCasted<byte>(0);
	}
}

//=================================================================================================
bool MeshInstance::Read(BitStream& stream)
{
	int fai;
	byte groups_count;

	if(!stream.ReadCasted<byte>(fai)
		|| !stream.Read(groups_count))
		return false;

	frame_end_info = IS_SET(fai, 0x01);
	frame_end_info2 = IS_SET(fai, 0x02);

	if(preload)
		groups.resize(groups_count);

	for(Group& group : groups)
	{
		if(stream.Read(group.time) &&
			stream.Read(group.speed) &&
			stream.Read(group.state) &&
			stream.ReadCasted<byte>(group.prio) &&
			stream.ReadCasted<byte>(group.used_group) &&
			ReadString1(stream))
		{
			if(BUF[0])
			{
				if(preload)
				{
					string* str = StringPool.Get();
					*str = BUF;
					group.anim = (Mesh::Animation*)str;
				}
				else
				{
					group.anim = mesh->GetAnimation(BUF);
					if(!group.anim)
					{
						Error("Missing animation '%s'.", BUF);
						return false;
					}
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

//=================================================================================================
bool MeshInstance::ApplyPreload(Mesh* mesh)
{
	assert(mesh
		&& preload
		&& !this->mesh
		&& groups.size() == mesh->head.n_groups);

	preload = false;
	this->mesh = mesh;
	mat_bones.resize(mesh->head.n_bones);
	blendb.resize(mesh->head.n_bones);

	for(auto& group : groups)
	{
		string* str = (string*)group.anim;
		if(str)
		{
			group.anim = mesh->GetAnimation(str->c_str());
			if(!group.anim)
			{
				Error("Invalid animation '%s' for mesh '%s'.", str->c_str(), mesh->path.c_str());
				StringPool.Free(str);
				return false;
			}
			StringPool.Free(str);
		}
	}

	return true;
}
