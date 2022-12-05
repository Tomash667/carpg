#include "Pch.h"
#include "Trap.h"

#include "Ability.h"
#include "BitStreamFunc.h"
#include "CombatHelper.h"
#include "Explo.h"
#include "GameCommon.h"
#include "GameResources.h"
#include "LevelPart.h"
#include "LocationPart.h"
#include "Net.h"
#include "Unit.h"

#include <ParticleSystem.h>
#include <SoundManager.h>

EntityType<Trap>::Impl EntityType<Trap>::impl;

//=================================================================================================
Trap::~Trap()
{
	delete hitted;
	delete meshInst;
}

//=================================================================================================
bool Trap::Update(float dt, LocationPart& locPart)
{
	Unit* owner = this->owner;

	switch(base->type)
	{
	case TRAP_SPEAR:
		if(state == 0)
		{
			// check if someone is step on it
			bool trigger = false;
			if(Net::IsLocal())
			{
				for(Unit* unit : locPart.units)
				{
					if(unit->IsStanding() && !IsSet(unit->data->flags, F_SLIGHT)
						&& (!owner || owner->IsEnemy(*unit))
						&& CircleToCircle(pos.x, pos.z, base->rw, unit->pos.x, unit->pos.z, unit->GetUnitRadius()))
					{
						trigger = true;
						break;
					}
				}
			}
			else if(mpTrigger)
			{
				trigger = true;
				mpTrigger = false;
			}

			if(trigger)
			{
				soundMgr->PlaySound3d(base->sound, pos, base->sound_dist);
				state = 1;
				time = Random(0.5f, 0.75f);

				if(Net::IsServer())
				{
					NetChange& c = Add1(Net::changes);
					c.type = NetChange::TRIGGER_TRAP;
					c.id = id;
				}
			}
		}
		else if(state == 1)
		{
			// count timer to spears come out
			bool trigger = false;
			if(Net::IsLocal())
			{
				time -= dt;
				if(time <= 0.f)
					trigger = true;
			}
			else if(mpTrigger)
			{
				trigger = true;
				mpTrigger = false;
			}

			if(trigger)
			{
				state = 2;
				meshInst->Play("takeOut", PLAY_ONCE | PLAY_STOP_AT_END);
				time = 0.f;

				soundMgr->PlaySound3d(base->sound2, pos, base->sound_dist2);

				if(Net::IsServer())
				{
					NetChange& c = Add1(Net::changes);
					c.type = NetChange::TRIGGER_TRAP;
					c.id = id;
				}
			}
		}
		else if(state == 2)
		{
			// move spears
			meshInst->Update(dt);

			if(Net::IsLocal())
			{
				for(Unit* unit : locPart.units)
				{
					if(!unit->IsAlive())
						continue;
					if(CircleToCircle(pos.x, pos.z, base->rw, unit->pos.x, unit->pos.z, unit->GetUnitRadius()))
					{
						bool found = false;
						for(Unit* unit2 : *hitted)
						{
							if(unit == unit2)
							{
								found = true;
								break;
							}
						}

						if(!found)
						{
							// hit unit with spears
							int mod = CombatHelper::CalculateModifier(DMG_PIERCE, unit->data->flags);
							float m = 1.f;
							if(mod == -1)
								m += 0.25f;
							else if(mod == 1)
								m -= 0.25f;
							if(unit->action == A_PAIN)
								m += 0.1f;

							// calculate attack & defense
							float attack = GetAttack() * m;
							float def = unit->CalculateDefense();
							float dmg = CombatHelper::CalculateDamage(attack, def);

							// hit sound
							unit->PlayHitSound(MAT_IRON, MAT_SPECIAL_UNIT, unit->pos + Vec3(0, 1.f, 0), dmg > 0);

							// train player armor skill
							if(unit->IsPlayer())
								unit->player->Train(TrainWhat::TakeDamageArmor, attack / unit->hpmax, 4);

							// damage
							if(dmg > 0)
								unit->GiveDmg(dmg);

							hitted->push_back(unit);
						}
					}
				}
			}

			if(meshInst->IsEnded())
			{
				state = 3;
				if(Net::IsLocal())
					hitted->clear();
				time = 1.f;
			}
		}
		else if(state == 3)
		{
			// count timer to hide spears
			time -= dt;
			if(time <= 0.f)
			{
				state = 4;
				meshInst->Play("hide", PLAY_ONCE);
				soundMgr->PlaySound3d(base->sound3, pos, base->sound_dist3);
			}
		}
		else if(state == 4)
		{
			// hiding spears
			meshInst->Update(dt);
			if(meshInst->IsEnded())
				state = 5;
		}
		else if(state == 5)
		{
			// spears are hidden, wait until unit moves away to reactivate
			bool reactivate;
			if(Net::IsLocal())
			{
				reactivate = true;
				for(Unit* unit : locPart.units)
				{
					if(!IsSet(unit->data->flags, F_SLIGHT)
						&& CircleToCircle(pos.x, pos.z, base->rw, unit->pos.x, unit->pos.z, unit->GetUnitRadius()))
					{
						reactivate = false;
						break;
					}
				}
			}
			else
			{
				if(mpTrigger)
				{
					reactivate = true;
					mpTrigger = false;
				}
				else
					reactivate = false;
			}

			if(reactivate)
			{
				state = 0;
				if(Net::IsServer())
				{
					NetChange& c = Add1(Net::changes);
					c.type = NetChange::TRIGGER_TRAP;
					c.id = id;
				}
			}
		}
		break;
	case TRAP_ARROW:
	case TRAP_POISON:
		if(state == 0)
		{
			// check if someone is step on it
			bool trigger = false;
			if(Net::IsLocal())
			{
				for(Unit* unit : locPart.units)
				{
					if(unit->IsStanding() && !IsSet(unit->data->flags, F_SLIGHT)
						&& (!owner || owner->IsEnemy(*unit))
						&& CircleToRectangle(unit->pos.x, unit->pos.z, unit->GetUnitRadius(), pos.x, pos.z, base->rw, base->h))
					{
						trigger = true;
						break;
					}
				}
			}
			else if(mpTrigger)
			{
				trigger = true;
				mpTrigger = false;
			}

			if(trigger)
			{
				// someone step on trap, shoot arrow
				state = Net::IsLocal() ? 1 : 2;
				time = Random(5.f, 7.5f);
				soundMgr->PlaySound3d(base->sound, pos, base->sound_dist);

				if(Net::IsLocal())
				{
					Bullet* bullet = new Bullet;
					locPart.lvlPart->bullets.push_back(bullet);

					bullet->Register();
					bullet->isArrow = true;
					bullet->level = 4;
					bullet->backstab = 0.25f;
					bullet->attack = GetAttack();
					bullet->mesh = gameRes->aArrow;
					bullet->pos = Vec3(2.f * tile.x + pos.x - float(int(pos.x / 2) * 2) + Random(-base->rw, base->rw) - 1.2f * DirToPos(dir).x,
						Random(0.5f, 1.5f),
						2.f * tile.y + pos.z - float(int(pos.z / 2) * 2) + Random(-base->h, base->h) - 1.2f * DirToPos(dir).y);
					bullet->startPos = bullet->pos;
					bullet->rot = Vec3(0, DirToRot(dir), 0);
					bullet->owner = nullptr;
					bullet->pe = nullptr;
					bullet->speed = TRAP_ARROW_SPEED;
					bullet->ability = nullptr;
					bullet->tex = nullptr;
					bullet->texSize = 0.f;
					bullet->timer = ARROW_TIMER;
					bullet->yspeed = 0.f;
					bullet->poisonAttack = (base->type == TRAP_POISON ? bullet->attack : 0.f);

					TrailParticleEmitter* tpe = new TrailParticleEmitter;
					tpe->fade = 0.3f;
					tpe->color1 = Vec4(1, 1, 1, 0.5f);
					tpe->color2 = Vec4(1, 1, 1, 0);
					tpe->Init(50);
					locPart.lvlPart->tpes.push_back(tpe);
					bullet->trail = tpe;

					soundMgr->PlaySound3d(gameRes->sBow[Rand() % 2], bullet->pos, SHOOT_SOUND_DIST);

					if(Net::IsServer())
					{
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::SHOOT_ARROW;
						c << bullet->id
							<< -1 // owner
							<< bullet->startPos
							<< bullet->rot.x
							<< bullet->rot.y
							<< bullet->speed
							<< bullet->yspeed
							<< 0; // ability

						NetChange& c2 = Add1(Net::changes);
						c2.type = NetChange::TRIGGER_TRAP;
						c2.id = id;
					}
				}
			}
		}
		else if(state == 1)
		{
			time -= dt;
			if(time <= 0.f)
				state = 2;
		}
		else
		{
			// check if units leave trap
			bool empty;
			if(Net::IsLocal())
			{
				empty = true;
				for(Unit* unit : locPart.units)
				{
					if(!IsSet(unit->data->flags, F_SLIGHT)
						&& CircleToRectangle(unit->pos.x, unit->pos.z, unit->GetUnitRadius(), pos.x, pos.z, base->rw, base->h))
					{
						empty = false;
						break;
					}
				}
			}
			else
			{
				if(mpTrigger)
				{
					empty = true;
					mpTrigger = false;
				}
				else
					empty = false;
			}

			if(empty)
			{
				state = 0;
				if(Net::IsServer())
				{
					NetChange& c = Add1(Net::changes);
					c.type = NetChange::TRIGGER_TRAP;
					c.id = id;
				}
			}
		}
		break;
	case TRAP_FIREBALL:
		{
			if(Net::IsClient())
				break;

			bool trigger = false;
			for(Unit* unit : locPart.units)
			{
				if(unit->IsStanding()
					&& (!owner || owner->IsEnemy(*unit))
					&& CircleToRectangle(unit->pos.x, unit->pos.z, unit->GetUnitRadius(), pos.x, pos.z, base->rw, base->h))
				{
					trigger = true;
					break;
				}
			}

			if(trigger)
			{
				Ability* fireball = Ability::Get("fireball");
				Vec3 exploPos = pos + Vec3(0, 0.2f, 0);
				Explo* explo = locPart.CreateExplo(fireball, exploPos);
				explo->dmg = GetAttack();
				explo->owner = owner;

				if(fireball->sound_hit)
				{
					soundMgr->PlaySound3d(fireball->sound_hit, pos, fireball->sound_hit_dist);
					if(Net::IsServer())
					{
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::SPELL_SOUND;
						c.extraId = 1;
						c.ability = fireball;
						c.pos = pos;
					}
				}

				if(Net::IsServer())
				{
					NetChange& c = Add1(Net::changes);
					c.type = NetChange::REMOVE_TRAP;
					c.id = id;
				}

				delete this;
				return true;
			}
		}
		break;
	case TRAP_BEAR:
		if(state == 0)
		{
			// check if someone is step on it
			bool trigger = false;
			if(Net::IsLocal())
			{
				for(Unit* unit : locPart.units)
				{
					if(unit->IsStanding() && !IsSet(unit->data->flags, F_SLIGHT)
						&& (!owner || owner->IsEnemy(*unit))
						&& CircleToCircle(pos.x, pos.z, base->rw, unit->pos.x, unit->pos.z, unit->GetUnitRadius()))
					{
						trigger = true;
						break;
					}
				}
			}
			else if(mpTrigger)
			{
				trigger = true;
				mpTrigger = false;
			}

			if(trigger)
			{
				soundMgr->PlaySound3d(base->sound, pos, base->sound_dist);
				state = 1;
				meshInst->Play("trigger", PLAY_ONCE | PLAY_STOP_AT_END | PLAY_NO_BLEND);

				if(Net::IsServer())
				{
					NetChange& c = Add1(Net::changes);
					c.type = NetChange::TRIGGER_TRAP;
					c.id = id;
				}
			}
		}
		else if(state == 1)
		{
			meshInst->Update(dt);
			if(meshInst->IsEnded())
			{
				if(Net::IsLocal())
				{
					Unit* target = nullptr;
					float bestDist = 10.f;
					for(Unit* unit : locPart.units)
					{
						if(unit->IsStanding()
							&& (!owner || owner->IsEnemy(*unit))
							&& CircleToCircle(pos.x, pos.z, base->rw * 1.5f, unit->pos.x, unit->pos.z, unit->GetUnitRadius()))
						{
							const float dist = Vec3::Distance(pos, unit->pos);
							if(dist < bestDist)
							{
								target = unit;
								bestDist = dist;
							}
						}
					}

					if(target)
					{
						// hit unit
						int mod = CombatHelper::CalculateModifier(DMG_SLASH, target->data->flags);
						float m = 1.f;
						if(mod == -1)
							m += 0.25f;
						else if(mod == 1)
							m -= 0.25f;
						if(target->action == A_PAIN)
							m += 0.1f;

						// calculate attack & defense
						float attack = GetAttack() * m;
						float def = target->CalculateDefense();
						float dmg = CombatHelper::CalculateDamage(attack, def);

						// hit sound
						target->PlayHitSound(MAT_IRON, MAT_SPECIAL_UNIT, target->pos + Vec3(0, 1.f, 0), dmg > 0);

						// train player armor skill
						if(target->IsPlayer())
							target->player->Train(TrainWhat::TakeDamageArmor, attack / target->hpmax, 4);

						// damage
						if(dmg > 0)
							target->GiveDmg(dmg);

						// effect
						Effect e;
						e.effect = EffectId::Rooted;
						e.source = EffectSource::Temporary;
						e.source_id = -1;
						e.power = 0;
						e.time = 5.f;
						e.value = EffectValue_Generic;
						target->AddEffect(e);
					}
				}
				state = 2;
				time = 5.f;
			}
		}
		else if(state == 2)
		{
			time -= dt;
			if(time <= 0.f)
			{
				delete this;
				return true;
			}
		}
		break;
	default:
		assert(0);
		break;
	}

	return false;
}

//=================================================================================================
void Trap::Save(GameWriter& f)
{
	f << id;
	f << base->type;
	f << pos;

	if(base->type == TRAP_ARROW || base->type == TRAP_POISON)
	{
		f << tile;
		f << dir;
	}
	else
		f << rot;

	if(f.isLocal)
	{
		if(base->type != TRAP_FIREBALL)
		{
			f << state;
			f << time;

			if(Any(base->type, TRAP_SPEAR, TRAP_BEAR))
				MeshInstance::SaveOptional(f, meshInst);

			if(base->type == TRAP_SPEAR)
			{
				f << hitted->size();
				for(Unit* unit : *hitted)
					f << unit->id;
			}
		}
		f << owner;
		f << attack;
	}
}

//=================================================================================================
void Trap::Load(GameReader& f)
{
	TRAP_TYPE type;

	if(LOAD_VERSION >= V_0_12)
		f >> id;
	Register();
	f >> type;
	f >> pos;
	if(LOAD_VERSION < V_0_12)
		f.Skip<int>(); // old netid

	base = &BaseTrap::traps[type];
	meshInst = nullptr;
	hitted = nullptr;

	if(type == TRAP_ARROW || type == TRAP_POISON)
	{
		f >> tile;
		f >> dir;
		rot = 0;
	}
	else
		f >> rot;

	if(f.isLocal)
	{
		if(base->type != TRAP_FIREBALL)
		{
			f >> state;
			f >> time;

			if(Any(base->type, TRAP_SPEAR, TRAP_BEAR))
			{
				if(LOAD_VERSION >= V_0_18)
					MeshInstance::LoadOptional(f, meshInst);
				else
				{
					f.Skip<float>(); // old obj2.pos.y
					state = 0; // don't restore trap animation, not worth doing
				}
			}

			if(base->type == TRAP_SPEAR)
			{
				uint count = f.Read<uint>();
				hitted = new vector<Unit*>;
				if(count)
				{
					hitted->resize(count);
					for(Unit*& unit : *hitted)
						unit = Unit::GetById(f.Read<int>());
				}
			}
		}
		else
			state = 0;

		if(LOAD_VERSION >= V_0_18)
		{
			f >> owner;
			f >> attack;
		}
		else
		{
			owner = nullptr;
			attack = 0;
		}
	}
}

//=================================================================================================
void Trap::Write(BitStreamWriter& f)
{
	f << id;
	f.WriteCasted<byte>(base->type);
	f.WriteCasted<byte>(dir);
	f << tile;
	f << pos;
	f << rot;

	if(net->mpLoad)
	{
		f.WriteCasted<byte>(state);
		f << time;
		if(Any(base->type, TRAP_SPEAR, TRAP_BEAR))
			MeshInstance::SaveOptional(f, meshInst);
	}
}

//=================================================================================================
bool Trap::Read(BitStreamReader& f)
{
	TRAP_TYPE type;
	f >> id;
	f.ReadCasted<byte>(type);
	f.ReadCasted<byte>(dir);
	f >> tile;
	f >> pos;
	f >> rot;
	if(!f)
		return false;
	base = &BaseTrap::traps[type];

	state = 0;
	meshInst = nullptr;
	mpTrigger = false;
	hitted = nullptr;

	if(net->mpLoad)
	{
		f.ReadCasted<byte>(state);
		f >> time;
		if(Any(base->type, TRAP_SPEAR, TRAP_BEAR))
			MeshInstance::LoadOptional(f, meshInst);
		if(!f)
			return false;
	}

	Register();
	return true;
}
