#include "Pch.h"
#include "Trap.h"

#include "Ability.h"
#include "BitStreamFunc.h"
#include "CombatHelper.h"
#include "Explo.h"
#include "GameCommon.h"
#include "GameResources.h"
#include "LevelArea.h"
#include "Net.h"
#include "Unit.h"

#include <ParticleSystem.h>
#include <SoundManager.h>

EntityType<Trap>::Impl EntityType<Trap>::impl;

//=================================================================================================
bool Trap::Update(float dt, LevelArea& area)
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
				for(Unit* unit : area.units)
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
				sound_mgr->PlaySound3d(base->sound, pos, base->sound_dist);
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
				time = 0.f;

				sound_mgr->PlaySound3d(base->sound2, pos, base->sound_dist2);

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
			bool end = false;
			time += dt;
			if(time >= 0.27f)
			{
				time = 0.27f;
				end = true;
			}

			obj2.pos.y = obj.pos.y - 2.f + 2.f * (time / 0.27f);

			if(Net::IsLocal())
			{
				for(Unit* unit : area.units)
				{
					if(!unit->IsAlive())
						continue;
					if(CircleToCircle(obj2.pos.x, obj2.pos.z, base->rw, unit->pos.x, unit->pos.z, unit->GetUnitRadius()))
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

			if(end)
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
				time = 1.5f;
				sound_mgr->PlaySound3d(base->sound3, pos, base->sound_dist3);
			}
		}
		else if(state == 4)
		{
			// hiding spears
			time -= dt;
			if(time <= 0.f)
			{
				time = 0.f;
				state = 5;
			}

			obj2.pos.y = obj.pos.y - 2.f + time / 1.5f * 2.f;
		}
		else if(state == 5)
		{
			// spears are hidden, wait until unit moves away to reactivate
			bool reactivate;
			if(Net::IsLocal())
			{
				reactivate = true;
				for(Unit* unit : area.units)
				{
					if(!IsSet(unit->data->flags, F_SLIGHT)
						&& CircleToCircle(obj2.pos.x, obj2.pos.z, base->rw, unit->pos.x, unit->pos.z, unit->GetUnitRadius()))
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
				for(Unit* unit : area.units)
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
				sound_mgr->PlaySound3d(base->sound, pos, base->sound_dist);

				if(Net::IsLocal())
				{
					Bullet* bullet = new Bullet;
					area.tmp->bullets.push_back(bullet);

					bullet->Register();
					bullet->isArrow = true;
					bullet->level = 4;
					bullet->backstab = 0.25f;
					bullet->attack = GetAttack();
					bullet->mesh = game_res->aArrow;
					bullet->pos = Vec3(2.f * tile.x + pos.x - float(int(pos.x / 2) * 2) + Random(-base->rw, base->rw) - 1.2f * DirToPos(dir).x,
						Random(0.5f, 1.5f),
						2.f * tile.y + pos.z - float(int(pos.z / 2) * 2) + Random(-base->h, base->h) - 1.2f * DirToPos(dir).y);
					bullet->start_pos = bullet->pos;
					bullet->rot = Vec3(0, DirToRot(dir), 0);
					bullet->owner = nullptr;
					bullet->pe = nullptr;
					bullet->speed = TRAP_ARROW_SPEED;
					bullet->ability = nullptr;
					bullet->tex = nullptr;
					bullet->tex_size = 0.f;
					bullet->timer = ARROW_TIMER;
					bullet->yspeed = 0.f;
					bullet->poison_attack = (base->type == TRAP_POISON ? bullet->attack : 0.f);

					TrailParticleEmitter* tpe = new TrailParticleEmitter;
					tpe->fade = 0.3f;
					tpe->color1 = Vec4(1, 1, 1, 0.5f);
					tpe->color2 = Vec4(1, 1, 1, 0);
					tpe->Init(50);
					area.tmp->tpes.push_back(tpe);
					bullet->trail = tpe;

					sound_mgr->PlaySound3d(game_res->sBow[Rand() % 2], bullet->pos, SHOOT_SOUND_DIST);

					if(Net::IsServer())
					{
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::SHOOT_ARROW;
						c << bullet->id
							<< -1 // owner
							<< bullet->start_pos
							<< bullet->rot.x
							<< bullet->rot.y
							<< bullet->speed
							<< bullet->yspeed;

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
				for(Unit* unit : area.units)
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
			for(Unit* unit : area.units)
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
				Explo* explo = area.CreateExplo(fireball, exploPos);
				explo->dmg = GetAttack();
				explo->owner = owner;

				if(fireball->sound_hit)
				{
					sound_mgr->PlaySound3d(fireball->sound_hit, pos, fireball->sound_hit_dist);
					if(Net::IsServer())
					{
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::SPELL_SOUND;
						c.e_id = 1;
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
		f << obj.rot.y;

	if(f.isLocal)
	{
		if(base->type != TRAP_FIREBALL)
		{
			f << state;
			f << time;

			if(base->type == TRAP_SPEAR)
			{
				f << obj2.pos.y;
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
	hitted = nullptr;
	obj.pos = pos;
	obj.rot = Vec3(0, 0, 0);
	obj.scale = 1.f;
	obj.base = nullptr;
	obj.mesh = base->mesh;

	if(type == TRAP_ARROW || type == TRAP_POISON)
	{
		f >> tile;
		f >> dir;
	}
	else
		f >> obj.rot.y;

	if(type == TRAP_SPEAR)
	{
		obj2.pos = pos;
		obj2.rot = obj.rot;
		obj2.scale = 1.f;
		obj2.mesh = base->mesh2;
		obj2.base = nullptr;
	}

	if(f.isLocal)
	{
		if(base->type != TRAP_FIREBALL)
		{
			f >> state;
			f >> time;

			if(base->type == TRAP_SPEAR)
			{
				f >> obj2.pos.y;
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

		if(LOAD_VERSION >= V_DEV)
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
	f << obj.rot.y;

	if(net->mp_load)
	{
		f.WriteCasted<byte>(state);
		f << time;
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
	f >> obj.rot.y;
	if(!f)
		return false;
	base = &BaseTrap::traps[type];

	state = 0;
	obj.base = nullptr;
	obj.mesh = base->mesh;
	obj.pos = pos;
	obj.scale = 1.f;
	obj.rot.x = obj.rot.z = 0;
	mpTrigger = false;
	hitted = nullptr;

	if(net->mp_load)
	{
		f.ReadCasted<byte>(state);
		f >> time;
		if(!f)
			return false;
	}

	if(type == TRAP_ARROW || type == TRAP_POISON)
		obj.rot = Vec3(0, 0, 0);
	else if(type == TRAP_SPEAR)
	{
		obj2.base = nullptr;
		obj2.mesh = base->mesh2;
		obj2.pos = obj.pos;
		obj2.rot = obj.rot;
		obj2.scale = 1.f;
		obj2.pos.y -= 2.f;
		hitted = nullptr;
	}

	Register();
	return true;
}
