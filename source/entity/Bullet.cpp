#include "Pch.h"
#include "Bullet.h"

#include "Ability.h"
#include "AIController.h"
#include "BitStreamFunc.h"
#include "CombatHelper.h"
#include "Game.h"
#include "GameResources.h"
#include "Level.h"
#include "LevelArea.h"
#include "Net.h"
#include "Object.h"
#include "PhysicCallbacks.h"
#include "QuestManager.h"
#include "Quest_Tutorial.h"
#include "Unit.h"

#include <Mesh.h>
#include <ParticleSystem.h>
#include <ResourceManager.h>
#include <Scene.h>
#include <SoundManager.h>

EntityType<Bullet>::Impl EntityType<Bullet>::impl;

//=================================================================================================
bool Bullet::Update(float dt, LevelArea& area)
{
	// update position
	Vec3 prev_pos = pos;
	pos += Vec3(sin(rot.y) * speed, yspeed, cos(rot.y) * speed) * dt;
	if(ability && ability->type == Ability::Ball)
		yspeed -= 10.f * dt;

	// update node & particles
	if(node)
	{
		node->center = pos;
		node->mat = Matrix::Rotation(rot) * Matrix::Translation(pos);
	}
	if(pe)
		pe->pos = pos;
	if(trail)
		trail->AddPoint(pos);

	if((timer -= dt) <= 0.f)
	{
		// timeout, delete bullet
		if(node)
		{
			area.tmp->scene->Remove(node);
			node->Free();
		}
		if(trail)
			trail->destroy = true;
		if(pe)
			pe->destroy = true;
		delete this;
		return true;
	}

	// do contact test
	btCollisionShape* shape;
	if(!ability)
		shape = game_level->shape_arrow;
	else
		shape = ability->shape;
	assert(shape->isConvex());

	btTransform tr_from, tr_to;
	tr_from.setOrigin(ToVector3(prev_pos));
	tr_from.setRotation(btQuaternion(rot.y, rot.x, rot.z));
	tr_to.setOrigin(ToVector3(pos));
	tr_to.setRotation(tr_from.getRotation());

	BulletCallback callback(owner ? owner->cobj : nullptr);
	phy_world->convexSweepTest((btConvexShape*)shape, tr_from, tr_to, callback);
	if(!callback.hasHit())
		return false;

	Vec3 hitpoint = ToVec3(callback.hitpoint);
	Unit* hitted = nullptr;
	if(IsSet(callback.target->getCollisionFlags(), CG_UNIT))
		hitted = reinterpret_cast<Unit*>(callback.target->getUserPointer());

	// something was hit, remove bullet
	if(node)
	{
		area.tmp->scene->Remove(node);
		node->Free();
	}
	if(trail)
		trail->destroy = true;
	if(pe)
		pe->destroy = true;

	OnHit(area, hitted, hitpoint, callback);

	if(Net::IsServer())
	{
		NetChange& c = Add1(net->changes);
		c.type = NetChange::REMOVE_BULLET;
		c.id = id;
	}

	delete this;
	return true;
}

//=================================================================================================
void Bullet::OnHit(LevelArea& area, Unit* hitted, const Vec3& hitpoint, BulletCallback& callback)
{
	if(hitted)
	{
		if(Net::IsClient())
			return;

		if(!ability)
		{
			if(owner && owner->IsFriend(*hitted, true) || attack < -50.f)
			{
				// friendly fire
				if(hitted->IsBlocking() && AngleDiff(Clip(rot.y + PI), hitted->rot) < PI * 2 / 5)
				{
					MATERIAL_TYPE mat = hitted->GetShield().material;
					sound_mgr->PlaySound3d(game_res->GetMaterialSound(MAT_IRON, mat), hitpoint, ARROW_HIT_SOUND_DIST);
					if(Net::IsOnline())
					{
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::HIT_SOUND;
						c.id = MAT_IRON;
						c.count = mat;
						c.pos = hitpoint;
					}
				}
				else
					game->PlayHitSound(MAT_IRON, hitted->GetBodyMaterial(), hitpoint, ARROW_HIT_SOUND_DIST, false);
				return;
			}

			// hit enemy unit
			if(owner && owner->IsAlive() && hitted->IsAI())
				hitted->ai->HitReaction(start_pos);

			// calculate modifiers
			int mod = CombatHelper::CalculateModifier(DMG_PIERCE, hitted->data->flags);
			float m = 1.f;
			if(mod == -1)
				m += 0.25f;
			else if(mod == 1)
				m -= 0.25f;
			if(hitted->IsNotFighting())
				m += 0.1f;
			if(hitted->action == A_PAIN)
				m += 0.1f;

			// backstab bonus damage
			float angle_dif = AngleDiff(rot.y, hitted->rot);
			float backstab_mod = backstab;
			if(IsSet(hitted->data->flags2, F2_BACKSTAB_RES))
				backstab_mod /= 2;
			m += angle_dif / PI * backstab_mod;

			// apply modifiers
			float attack = this->attack * m;

			if(hitted->IsBlocking() && angle_dif < PI * 2 / 5)
			{
				// play sound
				MATERIAL_TYPE mat = hitted->GetShield().material;
				sound_mgr->PlaySound3d(game_res->GetMaterialSound(MAT_IRON, mat), hitpoint, ARROW_HIT_SOUND_DIST);
				if(Net::IsOnline())
				{
					NetChange& c = Add1(Net::changes);
					c.type = NetChange::HIT_SOUND;
					c.id = MAT_IRON;
					c.count = mat;
					c.pos = hitpoint;
				}

				// train blocking
				if(hitted->IsPlayer())
					hitted->player->Train(TrainWhat::BlockBullet, attack / hitted->hpmax, level);

				// reduce damage
				float block = hitted->CalculateBlock() * hitted->GetBlockMod() * (1.f - angle_dif / (PI * 2 / 5));
				float stamina = min(block, attack);
				attack -= block;
				hitted->RemoveStaminaBlock(stamina);

				// pain animation & break blocking
				if(hitted->stamina < 0)
				{
					hitted->BreakAction();

					if(!IsSet(hitted->data->flags, F_DONT_SUFFER))
					{
						if(hitted->action != A_POSITION)
							hitted->action = A_PAIN;
						else
							hitted->animation_state = AS_POSITION_HURT;

						MeshInstance* meshInst = hitted->node->mesh_inst;
						if(meshInst->mesh->head.n_groups == 2)
							meshInst->Play(NAMES::ani_hurt, PLAY_PRIO1 | PLAY_ONCE, 1);
						else
						{
							meshInst->Play(NAMES::ani_hurt, PLAY_PRIO3 | PLAY_ONCE, 0);
							hitted->animation = ANI_PLAY;
						}
					}
				}

				if(attack < 0)
				{
					// shot blocked by shield
					if(owner && owner->IsPlayer())
					{
						// train player in bow
						owner->player->Train(TrainWhat::BowNoDamage, 0.f, hitted->level);
						// aggravate
						hitted->AttackReaction(*owner);
					}
					return;
				}
			}

			// calculate defense
			float def = hitted->CalculateDefense();

			// calculate damage
			float dmg = CombatHelper::CalculateDamage(attack, def);

			// hit sound
			game->PlayHitSound(MAT_IRON, hitted->GetBodyMaterial(), hitpoint, HIT_SOUND_DIST, dmg > 0.f);

			// train player armor skill
			if(hitted->IsPlayer())
				hitted->player->Train(TrainWhat::TakeDamageArmor, attack / hitted->hpmax, level);

			if(dmg < 0)
			{
				if(owner && owner->IsPlayer())
				{
					// train player in bow
					owner->player->Train(TrainWhat::BowNoDamage, 0.f, hitted->level);
					// aggravate
					hitted->AttackReaction(*owner);
				}
				return;
			}

			// train player in bow
			if(owner && owner->IsPlayer())
			{
				float v = dmg / hitted->hpmax;
				if(hitted->hp - dmg < 0.f && !hitted->IsImmortal())
					v = max(TRAIN_KILL_RATIO, v);
				if(v > 1.f)
					v = 1.f;
				owner->player->Train(TrainWhat::BowAttack, v, hitted->level);
			}

			hitted->GiveDmg(dmg, owner, &hitpoint);

			// apply poison
			if(poison_attack > 0.f)
			{
				float poison_res = hitted->GetPoisonResistance();
				if(poison_res > 0.f)
				{
					Effect e;
					e.effect = EffectId::Poison;
					e.source = EffectSource::Temporary;
					e.source_id = -1;
					e.value = -1;
					e.power = poison_attack / 5 * poison_res;
					e.time = 5.f;
					hitted->AddEffect(e);
				}
			}
		}
		else
		{
			// hit unit with spell
			if(owner && owner->IsFriend(*hitted, true))
			{
				// frendly fire
				area.SpellHitEffect(*this, hitpoint, hitted);

				// hit sound
				if(hitted->IsBlocking() && AngleDiff(Clip(rot.y + PI), hitted->rot) < PI * 2 / 5)
				{
					MATERIAL_TYPE mat = hitted->GetShield().material;
					sound_mgr->PlaySound3d(game_res->GetMaterialSound(MAT_IRON, mat), hitpoint, ARROW_HIT_SOUND_DIST);
					if(Net::IsOnline())
					{
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::HIT_SOUND;
						c.id = MAT_IRON;
						c.count = mat;
						c.pos = hitpoint;
					}
				}
				else
					game->PlayHitSound(MAT_IRON, hitted->GetBodyMaterial(), hitpoint, HIT_SOUND_DIST, false);
				return;
			}

			if(hitted->IsAI() && owner && owner->IsAlive())
				hitted->ai->HitReaction(start_pos);

			float dmg = attack;
			if(owner)
				dmg += owner->level * ability->dmg_bonus;
			float angle_dif = AngleDiff(rot.y, hitted->rot);
			float base_dmg = dmg;

			if(hitted->IsBlocking() && angle_dif < PI * 2 / 5)
			{
				float block = hitted->CalculateBlock() * hitted->GetBlockMod() * (1.f - angle_dif / (PI * 2 / 5));
				float stamina = min(dmg, block);
				dmg -= block / 2;
				hitted->RemoveStaminaBlock(stamina);

				if(hitted->IsPlayer())
				{
					// player blocked spell, train him
					hitted->player->Train(TrainWhat::BlockBullet, base_dmg / hitted->hpmax, level);
				}

				if(dmg < 0)
				{
					// blocked by shield
					area.SpellHitEffect(*this, hitpoint, hitted);
					return;
				}
			}

			hitted->GiveDmg(dmg, owner, &hitpoint, !IsSet(ability->flags, Ability::Poison) ? Unit::DMG_MAGICAL : 0);

			// apply poison
			if(IsSet(ability->flags, Ability::Poison))
			{
				float poison_res = hitted->GetPoisonResistance();
				if(poison_res > 0.f)
				{
					Effect e;
					e.effect = EffectId::Poison;
					e.source = EffectSource::Temporary;
					e.source_id = -1;
					e.value = -1;
					e.power = dmg / 5 * poison_res;
					e.time = 5.f;
					hitted->AddEffect(e);
				}
			}

			// apply spell effect
			area.SpellHitEffect(*this, hitpoint, hitted);
		}
	}
	else
	{
		// hit object
		if(!ability)
		{
			sound_mgr->PlaySound3d(game_res->GetMaterialSound(MAT_IRON, MAT_ROCK), hitpoint, ARROW_HIT_SOUND_DIST);

			ParticleEmitter* pe = new ParticleEmitter;
			pe->tex = game_res->tSpark;
			pe->emission_interval = 0.01f;
			pe->life = 5.f;
			pe->particle_life = 0.5f;
			pe->emissions = 1;
			pe->spawn_min = 10;
			pe->spawn_max = 15;
			pe->max_particles = 15;
			pe->pos = hitpoint;
			pe->speed_min = Vec3(-1, 0, -1);
			pe->speed_max = Vec3(1, 1, 1);
			pe->pos_min = Vec3(-0.1f, -0.1f, -0.1f);
			pe->pos_max = Vec3(0.1f, 0.1f, 0.1f);
			pe->size = 0.3f;
			pe->op_size = ParticleEmitter::POP_LINEAR_SHRINK;
			pe->alpha = 0.9f;
			pe->op_alpha = ParticleEmitter::POP_LINEAR_SHRINK;
			pe->mode = 0;
			pe->Init();
			area.tmp->pes.push_back(pe);

			if(owner && owner->IsPlayer() && Net::IsLocal() && callback.target && IsSet(callback.target->getCollisionFlags(), CG_OBJECT))
			{
				Object* obj = static_cast<Object*>(callback.target->getUserPointer());
				if(obj && obj->base && obj->base->id == "bow_target")
				{
					if(quest_mgr->quest_tutorial->in_tutorial)
						quest_mgr->quest_tutorial->HandleBulletCollision();
					owner->player->Train(TrainWhat::BowNoDamage, 0.f, 1);
				}
			}
		}
		else
		{
			// hit object with spell
			area.SpellHitEffect(*this, hitpoint, nullptr);
		}
	}
}

//=================================================================================================
void Bullet::Save(GameWriter& f) const
{
	f << id;
	f << pos;
	f << rot;
	if(mesh)
		f << mesh->filename;
	else
		f.Write0();
	f << speed;
	f << timer;
	f << attack;
	f << tex_size;
	f << yspeed;
	f << poison_attack;
	f << (owner ? owner->id : -1);
	f << (ability ? ability->hash : 0);
	if(tex)
		f << tex->filename;
	else
		f.Write0();
	f << (trail ? trail->id : -1);
	f << (pe ? pe->id : -1);
	f << backstab;
	f << level;
	f << start_pos;
}

//=================================================================================================
void Bullet::Load(GameReader& f)
{
	if(LOAD_VERSION >= V_0_16)
		f >> id;
	Register();
	f >> pos;
	f >> rot;
	const string& mesh_id = f.ReadString1();
	if(!mesh_id.empty())
		mesh = res_mgr->Load<Mesh>(mesh_id);
	else
		mesh = nullptr;
	f >> speed;
	f >> timer;
	f >> attack;
	f >> tex_size;
	f >> yspeed;
	f >> poison_attack;
	owner = Unit::GetById(f.Read<int>());
	if(LOAD_VERSION >= V_0_13)
	{
		int ability_hash = f.Read<int>();
		if(ability_hash != 0)
		{
			ability = Ability::Get(ability_hash);
			if(!ability)
				throw Format("Missing ability %u for bullet.", ability_hash);
		}
		else
			ability = nullptr;
	}
	else
	{
		const string& ability_id = f.ReadString1();
		if(!ability_id.empty())
		{
			ability = Ability::Get(ability_id);
			if(!ability)
				throw Format("Missing ability '%s' for bullet.", ability_id.c_str());
		}
		else
			ability = nullptr;
	}
	const string& tex_name = f.ReadString1();
	if(!tex_name.empty())
		tex = res_mgr->Load<Texture>(tex_name);
	else
		tex = nullptr;
	trail = TrailParticleEmitter::GetById(f.Read<int>());
	if(LOAD_VERSION < V_0_13)
	{
		TrailParticleEmitter* old_trail = TrailParticleEmitter::GetById(f.Read<int>());
		if(old_trail)
			old_trail->destroy = true;
	}
	pe = ParticleEmitter::GetById(f.Read<int>());
	if(LOAD_VERSION < V_0_16)
		f.Skip<bool>();
	f >> level;
	if(LOAD_VERSION >= V_0_10)
		f >> backstab;
	else
	{
		int backstab_value;
		f >> backstab_value;
		backstab = 0.25f * (backstab_value + 1);
	}
	f >> start_pos;
}

//=================================================================================================
void Bullet::Write(BitStreamWriter& f) const
{
	f << id;
	f << pos;
	f << rot;
	f << speed;
	f << yspeed;
	f << timer;
	f << (owner ? owner->id : -1);
	f << (ability ? ability->hash : 0);
}

//=================================================================================================
bool Bullet::Read(BitStreamReader& f, TmpLevelArea& tmp_area)
{
	f >> id;
	f >> pos;
	f >> rot;
	f >> speed;
	f >> yspeed;
	f >> timer;
	int unit_id = f.Read<int>();
	int ability_hash = f.Read<int>();
	if(!f)
		return false;

	if(ability_hash == 0)
	{
		ability = nullptr;
		mesh = game_res->aArrow;
		pe = nullptr;
		tex = nullptr;
		tex_size = 0.f;

		TrailParticleEmitter* tpe = new TrailParticleEmitter;
		tpe->fade = 0.3f;
		tpe->color1 = Vec4(1, 1, 1, 0.5f);
		tpe->color2 = Vec4(1, 1, 1, 0);
		tpe->Init(50);
		tmp_area.tpes.push_back(tpe);
		trail = tpe;
	}
	else
	{
		ability = Ability::Get(ability_hash);
		if(!ability)
		{
			Error("Missing ability '%u'.", ability_hash);
			return false;
		}

		mesh = ability->mesh;
		tex = ability->tex;
		tex_size = ability->size;
		trail = nullptr;
		pe = nullptr;

		if(ability->tex_particle)
		{
			pe = new ParticleEmitter;
			pe->tex = ability->tex_particle;
			pe->emission_interval = 0.1f;
			pe->life = -1;
			pe->particle_life = 0.5f;
			pe->emissions = -1;
			pe->spawn_min = 3;
			pe->spawn_max = 4;
			pe->max_particles = 50;
			pe->pos = pos;
			pe->speed_min = Vec3(-1, -1, -1);
			pe->speed_max = Vec3(1, 1, 1);
			pe->pos_min = Vec3(-ability->size, -ability->size, -ability->size);
			pe->pos_max = Vec3(ability->size, ability->size, ability->size);
			pe->size = ability->size_particle;
			pe->op_size = ParticleEmitter::POP_LINEAR_SHRINK;
			pe->alpha = 1.f;
			pe->op_alpha = ParticleEmitter::POP_LINEAR_SHRINK;
			pe->mode = 1;
			pe->Init();
			tmp_area.pes.push_back(pe);
		}
	}

	if(unit_id != -1)
	{
		owner = game_level->FindUnit(unit_id);
		if(!owner)
		{
			Error("Missing bullet owner %d.", unit_id);
			return false;
		}
	}
	else
		owner = nullptr;

	Register();
	return true;
}
