#include "Pch.h"
#include "Bullet.h"

#include "Ability.h"
#include "AIController.h"
#include "BitStreamFunc.h"
#include "CombatHelper.h"
#include "Game.h"
#include "GameResources.h"
#include "Level.h"
#include "LevelPart.h"
#include "LocationPart.h"
#include "Net.h"
#include "Object.h"
#include "ParticleEffect.h"
#include "PhysicCallbacks.h"
#include "QuestManager.h"
#include "Quest_Tutorial.h"
#include "Unit.h"

#include <Mesh.h>
#include <ParticleSystem.h>
#include <ResourceManager.h>
#include <SoundManager.h>

EntityType<Bullet>::Impl EntityType<Bullet>::impl;

//=================================================================================================
bool Bullet::Update(float dt, LocationPart& locPart)
{
	// update position
	Vec3 prevPos = pos;
	pos += Vec3(sin(rot.y) * speed, yspeed, cos(rot.y) * speed) * dt;
	if(ability && ability->type == Ability::Ball)
		yspeed -= 10.f * dt;

	// update particles
	if(pe)
		pe->pos = pos;
	if(trail)
		trail->AddPoint(pos);

	// remove bullet on timeout
	if((timer -= dt) <= 0.f)
	{
		// timeout, delete bullet
		if(trail)
			trail->destroy = true;
		if(pe)
			pe->destroy = true;
		delete this;
		return true;
	}

	// do contact test
	btCollisionShape* shape;
	if(isArrow)
		shape = gameLevel->shapeArrow;
	else
		shape = ability->shape;
	assert(shape->isConvex());

	btTransform trFrom, trTo;
	trFrom.setOrigin(ToVector3(prevPos));
	trFrom.setRotation(btQuaternion(rot.y, rot.x, rot.z));
	trTo.setOrigin(ToVector3(pos));
	trTo.setRotation(trFrom.getRotation());

	BulletCallback callback(owner ? owner->cobj : nullptr);
	phyWorld->convexSweepTest((btConvexShape*)shape, trFrom, trTo, callback);
	if(!callback.hasHit())
		return false;

	Vec3 hitpoint = ToVec3(callback.hitpoint);
	Unit* hitted = nullptr;
	if(IsSet(callback.target->getCollisionFlags(), CG_UNIT))
		hitted = reinterpret_cast<Unit*>(callback.target->getUserPointer());

	// something was hit, remove bullet
	if(trail)
		trail->destroy = true;
	if(pe)
		pe->destroy = true;

	OnHit(locPart, hitted, hitpoint, callback);

	if(Net::IsServer())
	{
		NetChange& c = Net::PushChange(NetChange::REMOVE_BULLET);
		c.id = id;
		c.extraId = (hitted != nullptr ? 1 : 0);
	}

	delete this;
	return true;
}

//=================================================================================================
void Bullet::OnHit(LocationPart& locPart, Unit* hitted, const Vec3& hitpoint, BulletCallback& callback)
{
	if(hitted)
	{
		if(Net::IsClient())
			return;

		if(isArrow)
		{
			if(owner && owner->IsFriend(*hitted, true) || attack < -50.f)
			{
				// friendly fire
				MATERIAL_TYPE material;
				if(hitted->IsBlocking() && AngleDiff(Clip(rot.y + PI), hitted->rot) < PI * 2 / 5)
					material = hitted->GetShield().material;
				else
					material = MAT_SPECIAL_UNIT;
				hitted->PlayHitSound(MAT_IRON, material, hitpoint, false);
				return;
			}

			// hit enemy unit
			if(owner && owner->IsAlive() && hitted->IsAI())
				hitted->ai->HitReaction(startPos);

			// special effects
			bool preventTooManySounds = false;
			if(ability)
			{
				assert(ability->effect == Ability::Rooted); // TODO

				Effect e;
				e.effect = EffectId::Rooted;
				e.source = EffectSource::Temporary;
				e.sourceId = -1;
				e.value = EffectValue_Rooted_Vines;
				e.power = 0.f;
				e.time = ability->time;
				hitted->AddEffect(e);

				if(ability->soundHit)
				{
					preventTooManySounds = true;
					soundMgr->PlaySound3d(ability->soundHit, hitpoint, ability->soundHitDist);
					if(Net::IsServer())
					{
						NetChange& c = Net::PushChange(NetChange::SPELL_SOUND);
						c.extraId = 1;
						c.ability = ability;
						c.pos = hitpoint;
					}
				}
			}

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
			float angleDif = AngleDiff(rot.y, hitted->rot);
			float backstabMod = backstab;
			if(IsSet(hitted->data->flags2, F2_BACKSTAB_RES))
				backstabMod /= 2;
			m += angleDif / PI * backstabMod;

			// apply modifiers
			float attack = this->attack * m;

			if(hitted->IsBlocking() && angleDif < PI * 2 / 5)
			{
				// play sound
				const MATERIAL_TYPE material = hitted->GetShield().material;
				hitted->PlayHitSound(MAT_IRON, material, hitpoint, false);

				// train blocking
				if(hitted->IsPlayer())
					hitted->player->Train(TrainWhat::BlockBullet, attack / hitted->hpmax, level);

				// reduce damage
				float block = hitted->CalculateBlock() * hitted->GetBlockMod() * (1.f - angleDif / (PI * 2 / 5));
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
							hitted->animationState = AS_POSITION_HURT;

						if(hitted->meshInst->mesh->head.nGroups == 2)
							hitted->meshInst->Play(NAMES::aniHurt, PLAY_PRIO1 | PLAY_ONCE, 1);
						else
						{
							hitted->meshInst->Play(NAMES::aniHurt, PLAY_PRIO3 | PLAY_ONCE, 0);
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
			hitted->PlayHitSound(MAT_IRON, MAT_SPECIAL_UNIT, hitpoint, dmg > 0.f && !preventTooManySounds);

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
			if(poisonAttack > 0.f)
			{
				float poisonRes = hitted->GetPoisonResistance();
				if(poisonRes > 0.f)
				{
					Effect e;
					e.effect = EffectId::Poison;
					e.source = EffectSource::Temporary;
					e.sourceId = -1;
					e.value = -1;
					e.power = poisonAttack / 5 * poisonRes;
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
				locPart.SpellHitEffect(*this, hitpoint, hitted);

				// hit sound
				MATERIAL_TYPE material;
				if(hitted->IsBlocking() && AngleDiff(Clip(rot.y + PI), hitted->rot) < PI * 2 / 5)
					material = hitted->GetShield().material;
				else
					material = MAT_SPECIAL_UNIT;
				hitted->PlayHitSound(MAT_IRON, material, hitpoint, false);
				return;
			}

			if(hitted->IsAI() && owner && owner->IsAlive())
				hitted->ai->HitReaction(startPos);

			float dmg = attack;
			if(owner)
				dmg += owner->level * ability->dmgBonus;
			float angleDif = AngleDiff(rot.y, hitted->rot);
			float baseDmg = dmg;

			if(hitted->IsBlocking() && angleDif < PI * 2 / 5)
			{
				float block = hitted->CalculateBlock() * hitted->GetBlockMod() * (1.f - angleDif / (PI * 2 / 5));
				float stamina = min(dmg, block);
				dmg -= block / 2;
				hitted->RemoveStaminaBlock(stamina);

				if(hitted->IsPlayer())
				{
					// player blocked spell, train him
					hitted->player->Train(TrainWhat::BlockBullet, baseDmg / hitted->hpmax, level);
				}

				if(dmg < 0)
				{
					// blocked by shield
					locPart.SpellHitEffect(*this, hitpoint, hitted);
					return;
				}
			}

			hitted->GiveDmg(dmg, owner, &hitpoint, !IsSet(ability->flags, Ability::Poison) ? Unit::DMG_MAGICAL : 0);

			// apply poison
			if(IsSet(ability->flags, Ability::Poison))
			{
				float poisonRes = hitted->GetPoisonResistance();
				if(poisonRes > 0.f)
				{
					Effect e;
					e.effect = EffectId::Poison;
					e.source = EffectSource::Temporary;
					e.sourceId = -1;
					e.value = -1;
					e.power = dmg / 5 * poisonRes;
					e.time = 5.f;
					hitted->AddEffect(e);
				}
			}

			// apply spell effect
			locPart.SpellHitEffect(*this, hitpoint, hitted);
		}
	}
	else
	{
		// hit object
		if(!ability)
		{
			soundMgr->PlaySound3d(gameRes->GetMaterialSound(MAT_IRON, MAT_ROCK), hitpoint, HIT_SOUND_DIST);

			ParticleEmitter* pe = new ParticleEmitter;
			gameRes->peHit->Apply(pe);
			pe->pos = hitpoint;
			pe->Init();
			locPart.lvlPart->pes.push_back(pe);

			if(owner && owner->IsPlayer() && Net::IsLocal() && callback.target && IsSet(callback.target->getCollisionFlags(), CG_OBJECT))
			{
				Object* obj = static_cast<Object*>(callback.target->getUserPointer());
				if(obj && obj->base && obj->base->id == "bowTarget")
				{
					if(questMgr->questTutorial->inTutorial)
						questMgr->questTutorial->HandleBulletCollision();
					owner->player->Train(TrainWhat::BowNoDamage, 0.f, 1);
				}
			}
		}
		else if(Net::IsLocal())
		{
			// hit object with spell
			locPart.SpellHitEffect(*this, hitpoint, nullptr);
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
	f << texSize;
	f << yspeed;
	f << poisonAttack;
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
	f << startPos;
	f << isArrow;
}

//=================================================================================================
void Bullet::Load(GameReader& f)
{
	if(LOAD_VERSION >= V_0_16)
		f >> id;
	Register();
	f >> pos;
	f >> rot;
	const string& meshId = f.ReadString1();
	if(!meshId.empty())
		mesh = resMgr->Load<Mesh>(meshId);
	else
		mesh = nullptr;
	f >> speed;
	f >> timer;
	f >> attack;
	f >> texSize;
	f >> yspeed;
	f >> poisonAttack;
	owner = Unit::GetById(f.Read<int>());
	if(LOAD_VERSION >= V_0_13)
	{
		int abilityHash = f.Read<int>();
		if(abilityHash != 0)
		{
			ability = Ability::Get(abilityHash);
			if(!ability)
				throw Format("Missing ability %u for bullet.", abilityHash);
		}
		else
			ability = nullptr;
	}
	else
	{
		const string& abilityId = f.ReadString1();
		if(!abilityId.empty())
		{
			ability = Ability::Get(abilityId);
			if(!ability)
				throw Format("Missing ability '%s' for bullet.", abilityId.c_str());
		}
		else
			ability = nullptr;
	}
	const string& texName = f.ReadString1();
	if(!texName.empty())
		tex = resMgr->Load<Texture>(texName);
	else
		tex = nullptr;
	trail = TrailParticleEmitter::GetById(f.Read<int>());
	if(LOAD_VERSION < V_0_13)
	{
		TrailParticleEmitter* oldTrail = TrailParticleEmitter::GetById(f.Read<int>());
		if(oldTrail)
			oldTrail->destroy = true;
	}
	pe = ParticleEmitter::GetById(f.Read<int>());
	if(LOAD_VERSION < V_0_16)
		f.Skip<bool>();
	f >> level;
	f >> backstab;
	f >> startPos;
	if(LOAD_VERSION >= V_0_18)
		f >> isArrow;
	else
		isArrow = (ability == nullptr);
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
	f << isArrow;
}

//=================================================================================================
bool Bullet::Read(BitStreamReader& f, LevelPart& lvlPart)
{
	f >> id;
	f >> pos;
	f >> rot;
	f >> speed;
	f >> yspeed;
	f >> timer;
	int unitId = f.Read<int>();
	int abilityHash = f.Read<int>();
	f >> isArrow;
	if(!f)
		return false;

	if(abilityHash != 0)
	{
		ability = Ability::Get(abilityHash);
		if(!ability)
		{
			Error("Missing ability '%u'.", abilityHash);
			return false;
		}
	}
	else
		ability = nullptr;

	if(isArrow)
	{
		mesh = (ability && ability->mesh ? ability->mesh : gameRes->aArrow);
		pe = nullptr;
		tex = nullptr;
		texSize = 0.f;

		TrailParticleEmitter* tpe = new TrailParticleEmitter;
		tpe->fade = 0.3f;
		if(ability)
		{
			tpe->color1 = ability->color;
			tpe->color2 = tpe->color1;
			tpe->color2.w = 0;
		}
		else
		{
			tpe->color1 = Vec4(1, 1, 1, 0.5f);
			tpe->color2 = Vec4(1, 1, 1, 0);
		}
		tpe->Init(50);
		lvlPart.tpes.push_back(tpe);
		trail = tpe;
	}
	else
	{
		ability = Ability::Get(abilityHash);
		if(!ability)
		{
			Error("Missing ability '%u'.", abilityHash);
			return false;
		}

		mesh = ability->mesh;
		tex = ability->tex;
		texSize = ability->size;
		trail = nullptr;
		pe = nullptr;

		if(ability->texParticle)
		{
			pe = new ParticleEmitter;
			gameRes->peSpellHit->Apply(pe);
			pe->tex = ability->texParticle;
			pe->pos = pos;
			pe->posMin = Vec3(-ability->size, -ability->size, -ability->size);
			pe->posMax = Vec3(ability->size, ability->size, ability->size);
			pe->size = Vec2(ability->sizeParticle, 0.f);
			pe->Init();
			lvlPart.pes.push_back(pe);
		}
	}

	if(unitId != -1)
	{
		owner = gameLevel->FindUnit(unitId);
		if(!owner)
		{
			Error("Missing bullet owner %d.", unitId);
			return false;
		}
	}
	else
		owner = nullptr;

	Register();
	return true;
}
