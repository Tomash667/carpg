#include "Pch.h"
#include "ExploSystem.h"

#include "Ability.h"
#include "Explo.h"
#include "GameFile.h"
#include "GameResources.h"
#include "LevelArea.h"
#include "Net.h"
#include "SaveState.h"
#include "Unit.h"

#include <Scene.h>
#include <SceneNode.h>
#include <SoundManager.h>

Explo* ExploSystem::Add(LevelArea* area, Ability* ability, const Vec3& pos)
{
	// create explo
	Explo* explo = new Explo;
	explo->area = area;
	explo->ability = ability;
	explo->pos = pos;
	explo->size = 0;
	explo->sizemax = ability->explode_range;
	explos.push_back(explo);

	// create scene node
	SceneNode* node = SceneNode::Get();
	node->SetMesh(game_res->aSpellball);
	node->flags |= SceneNode::F_NO_LIGHTING | SceneNode::F_ALPHA_BLEND | SceneNode::F_NO_ZWRITE;
	node->center = explo->pos;
	node->radius = 0;
	node->mat = Matrix::Scale(0) * Matrix::Translation(explo->pos);
	node->tex_override = &ability->tex_explode;
	node->tint = Vec4(1, 1, 1, 1);
	area->scene->Add(node);

	// play sound
	sound_mgr->PlaySound3d(ability->sound_hit, explo->pos, ability->sound_hit_dist);

	// notify
	if(Net::IsOnline())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::CREATE_EXPLOSION;
		c.ability = ability;
		c.pos = explo->pos;
	}

	return explo;
}

void ExploSystem::Update(float dt)
{
	LoopAndRemove(explos, [=](Explo* explo)
	{
		// increase size
		bool delete_me = false;
		explo->size += explo->sizemax * dt;
		if(explo->size >= explo->sizemax)
		{
			delete_me = true;
			explo->size = explo->sizemax;
		}
		else
		{
			SceneNode* node = explo->node;
			node->radius = node->mesh->head.radius * explo->size;
			node->mat = Matrix::Scale(explo->size) * Matrix::Translation(explo->pos);
			node->tint.w = 1.f - explo->size / explo->sizemax;
		}

		if(Net::IsLocal())
		{
			// deal damage
			Unit* owner = explo->owner;
			const float dmg = explo->dmg * Lerp(1.f, 0.1f, explo->size / explo->sizemax);
			for(Unit* unit : explo->area->units)
			{
				if(!unit->IsAlive() || (owner && owner->IsFriend(*unit, true)))
					continue;

				if(!IsInside(explo->hitted, unit))
				{
					Box box;
					unit->GetBox(box);

					if(SphereToBox(explo->pos, explo->size, box))
					{
						unit->GiveDmg(dmg, owner, nullptr, Unit::DMG_NO_BLOOD | Unit::DMG_MAGICAL);
						explo->hitted.push_back(unit);
					}
				}
			}
		}

		if(delete_me)
			delete explo;
		return delete_me;
	});
}

void ExploSystem::RecreateScene()
{
	for(Explo* explo : explos)
	{
		SceneNode* node = SceneNode::Get();
		node->SetMesh(game_res->aSpellball);
		node->flags |= SceneNode::F_NO_LIGHTING | SceneNode::F_ALPHA_BLEND | SceneNode::F_NO_ZWRITE;
		node->center = explo->pos;
		node->radius *= explo->size;
		node->mat = Matrix::Scale(explo->size) * Matrix::Translation(explo->pos);
		node->tex_override = &explo->ability->tex_explode;
		node->tint = Vec4(1, 1, 1, 1.f - explo->size / explo->sizemax);
		explo->area->scene->Add(node);
	}
}

void ExploSystem::Reset()
{
	DeleteElements(explos);
}

void ExploSystem::Save(GameWriter& f)
{
	f << explos.size();
	for(Explo* explo : explos)
	{
		f << explo->pos;
		f << explo->size;
		f << explo->sizemax;
		f << explo->dmg;
		f << explo->hitted;
		f << explo->owner;
		f << explo->ability;
	}
}

void ExploSystem::Load(GameReader& f)
{
	explos.resize(f.Read<uint>());
	for(Explo*& explo : explos)
	{
		explo = new Explo;
		f >> explo->pos;
		f >> explo->size;
		f >> explo->sizemax;
		f >> explo->dmg;
		f >> explo->hitted;
		f >> explo->owner;
		f >> explo->ability;
	}
}

void ExploSystem::LoadOld(GameReader& f)
{
	uint count = f.Read<uint>();
	if(!count)
		return;
	uint offset = explos.size();
	explos.resize(offset + count);
	for(uint i = 0; i < count; ++i)
	{
		Explo*& explo = explos[offset + i];
		explo = new Explo;
		f >> explo->pos;
		f >> explo->size;
		f >> explo->sizemax;
		f >> explo->dmg;
		f >> explo->hitted;
		f >> explo->owner;
		if(LOAD_VERSION >= V_0_13)
			f >> explo->ability;
		else
		{
			const string& tex = f.ReadString1();
			for(Ability* ability : Ability::abilities)
			{
				if(ability->tex_explode.diffuse && ability->tex_explode.diffuse->filename == tex)
				{
					explo->ability = ability;
					break;
				}
			}
		}
	}
}
