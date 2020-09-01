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
#include "SceneNodeHelper.h"
#include "Unit.h"

#include <ParticleSystem.h>
#include <Scene.h>
#include <SceneNode.h>
#include <SoundManager.h>

EntityType<Trap>::Impl EntityType<Trap>::impl;

//=================================================================================================
bool Trap::Update(float dt, LevelArea& area)
{
	if(state == -1)
	{
		time -= dt;
		if(time <= 0.f)
			state = 0;
		return false;
	}

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
				node->mesh_inst->Play("takeOut", PLAY_ONCE | PLAY_STOP_AT_END | PLAY_NO_BLEND);
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
			node->mesh_inst->Update(dt);
			if(Net::IsLocal())
			{
				for(Unit* unit : area.units)
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
							float attack = float(base->attack) * m;
							float def = unit->CalculateDefense();
							float dmg = CombatHelper::CalculateDamage(attack, def);

							// hit sound
							sound_mgr->PlaySound3d(game_res->GetMaterialSound(MAT_IRON, unit->GetBodyMaterial()), unit->pos + Vec3(0, 1.f, 0), HIT_SOUND_DIST);

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

			if(node->mesh_inst->IsEnded())
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
				node->mesh_inst->Play("hide", PLAY_ONCE | PLAY_NO_BLEND);
				sound_mgr->PlaySound3d(base->sound3, pos, base->sound_dist3);
			}
		}
		else if(state == 4)
		{
			// hiding spears
			node->mesh_inst->Update(dt);
			if(node->mesh_inst->IsEnded())
				state = 5;
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
				for(Unit* unit : area.units)
				{
					if(unit->IsStanding() && !IsSet(unit->data->flags, F_SLIGHT)
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
				node->visible = false;
				sound_mgr->PlaySound3d(base->sound, pos, base->sound_dist);

				if(Net::IsLocal())
				{
					Bullet* bullet = new Bullet;
					bullet->Register();
					bullet->level = 4;
					bullet->backstab = 0.25f;
					bullet->attack = float(base->attack);
					bullet->pos = Vec3(2.f * tile.x + pos.x - float(int(pos.x / 2) * 2) + Random(-base->rw, base->rw) - 1.2f * DirToPos(dir).x,
						Random(0.5f, 1.5f),
						2.f * tile.y + pos.z - float(int(pos.z / 2) * 2) + Random(-base->h, base->h) - 1.2f * DirToPos(dir).y);
					bullet->rot = Vec3(0, DirToRot(dir), 0);
					bullet->owner = nullptr;
					bullet->speed = TRAP_ARROW_SPEED;
					bullet->yspeed = 0.f;
					bullet->poison_attack = (base->type == TRAP_POISON ? float(base->attack) : 0.f);
					area.CreateArrow(bullet);

					if(Net::IsServer())
					{
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::TRIGGER_TRAP;
						c.id = id;
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
				node->visible = true;
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
			if(!Net::IsLocal())
				break;

			bool trigger = false;
			for(Unit* unit : area.units)
			{
				if(unit->IsStanding()
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
				explo->dmg = float(base->attack);

				area.tmp->scene->Remove(node);
				node->Free();

				if(Net::IsOnline())
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
	f << rot;

	if(base->type == TRAP_ARROW || base->type == TRAP_POISON)
	{
		f << tile;
		f << dir;
	}

	if(f.isLocal && base->type != TRAP_FIREBALL)
	{
		f << state;
		f << time;

		if(base->type == TRAP_SPEAR)
		{
			SceneNodeHelper::Save(node, f);
			f << hitted->size();
			for(Unit* unit : *hitted)
				f << unit->id;
		}
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
	if(LOAD_VERSION >= V_DEV)
		f >> rot;

	base = &BaseTrap::traps[type];
	hitted = nullptr;

	if(type == TRAP_ARROW || type == TRAP_POISON)
	{
		f >> tile;
		f >> dir;
	}

	if(LOAD_VERSION < V_DEV)
	{
		if(type == TRAP_ARROW || type == TRAP_POISON)
			rot = 0;
		else
			f >> rot;
	}

	if(f.isLocal && base->type != TRAP_FIREBALL)
	{
		f >> state;
		f >> time;

		if(base->type == TRAP_SPEAR)
		{
			if(LOAD_VERSION >= V_DEV)
				node = SceneNodeHelper::Load(f);
			else
			{
				f.Skip<float>(); // old obj2.pos.y
				state = 0;
			}
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

	if(net->mp_load)
	{
		f.WriteCasted<byte>(state);
		f << time;
		if(base->type == TRAP_SPEAR)
			SceneNodeHelper::Save(node, f);
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
	mpTrigger = false;
	hitted = nullptr;

	if(net->mp_load)
	{
		f.ReadCasted<byte>(state);
		f >> time;
		if(base->type == TRAP_SPEAR)
			node = SceneNodeHelper::Load(f);
		if(!f)
			return false;
	}

	Register();
	return true;
}
