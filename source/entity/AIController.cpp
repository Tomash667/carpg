#include "Pch.h"
#include "AIController.h"

#include "Ability.h"
#include "Game.h"
#include "GameGui.h"
#include "GameMessages.h"
#include "InsideLocation.h"
#include "Level.h"
#include "QuestManager.h"
#include "Quest_Tournament.h"
#include "Unit.h"

//=================================================================================================
void AIController::Init(Unit* unit)
{
	assert(unit);

	this->unit = unit;
	unit->ai = this;
	state = Idle;
	nextAttack = 0.f;
	ignore = 0.f;
	morale = unit->GetMaxMorale();
	cooldown[0] = 0.f;
	cooldown[1] = 0.f;
	cooldown[2] = 0.f;
	havePotion = HavePotion::Check;
	haveMpPotion = HavePotion::Check;
	potion = -1;
	inCombat = false;
	st.idle.action = Idle_None;
	startPos = unit->pos;
	startRot = unit->rot;
	locTimer = 0.f;
	timer = 0.f;
	scanTimer = Random(0.f, 0.2f);
	changeAiMode = false;
	pfState = PFS_NOT_USING;
}

//=================================================================================================
void AIController::Save(GameWriter& f)
{
	f << unit->id;
	f << target;
	f << alertTarget;
	f << state;
	f << targetLastPos;
	f << alertTargetPos;
	f << startPos;
	f << inCombat;
	f << pfState;
	if(pfState != PFS_NOT_USING)
	{
		f << pfTimer;
		if(pfState == PFS_WALKING || pfState == PFS_LOCAL_TRY_WALK)
		{
			f.WriteVector2(pfPath);
			f << pfTargetTile;
			if(pfState == PFS_LOCAL_TRY_WALK)
				f << pfLocalTry;
		}
		if(pfState == PFS_WALKING || pfState == PFS_WALKING_LOCAL)
		{
			f.WriteVector1(pfLocalPath);
			f << pfLocalTargetTile;
		}
	}
	f << nextAttack;
	f << timer;
	f << ignore;
	f << morale;
	f << startRot;
	if(unit->data->abilities)
		f << cooldown;
	switch(state)
	{
	case Cast:
		f << st.cast.ability->hash;
		break;
	case Escape:
		f << (st.escape.room ? st.escape.room->index : -1);
		break;
	case Idle:
		f << st.idle.action;
		switch(st.idle.action)
		{
		case Idle_None:
		case Idle_Animation:
			break;
		case Idle_Rot:
			f << st.idle.rot;
			break;
		case Idle_Move:
		case Idle_TrainCombat:
		case Idle_Pray:
			f << st.idle.pos;
			break;
		case Idle_Look:
		case Idle_WalkTo:
		case Idle_Chat:
		case Idle_WalkNearUnit:
		case Idle_MoveAway:
			f << st.idle.unit;
			break;
		case Idle_Use:
		case Idle_WalkUse:
		case Idle_WalkUseEat:
			f << st.idle.usable;
			break;
		case Idle_TrainBow:
			f << st.idle.obj.pos;
			f << st.idle.obj.rot;
			break;
		case Idle_MoveRegion:
		case Idle_RunRegion:
			f << st.idle.region.locPart->partId;
			f << st.idle.region.pos;
			f << st.idle.region.exit;
			break;
		default:
			assert(0);
			break;
		}
		break;
	case SearchEnemy:
		f << (st.search.room ? st.search.room->index : -1);
		break;
	}
	f << havePotion;
	f << haveMpPotion;
	f << potion;
	f << cityWander;
	f << locTimer;
	f << scanTimer;
}

//=================================================================================================
void AIController::Load(GameReader& f)
{
	f >> unit;
	unit->ai = this;
	f >> target;
	f >> alertTarget;
	f >> state;
	f >> targetLastPos;
	f >> alertTargetPos;
	f >> startPos;
	f >> inCombat;
	f >> pfState;
	if(pfState != PFS_NOT_USING)
	{
		f >> pfTimer;
		if(pfState == PFS_WALKING || pfState == PFS_LOCAL_TRY_WALK)
		{
			f.ReadVector2(pfPath);
			f >> pfTargetTile;
			if(pfState == PFS_LOCAL_TRY_WALK)
				f >> pfLocalTry;
		}
		if(pfState == PFS_WALKING || pfState == PFS_WALKING_LOCAL)
		{
			f.ReadVector1(pfLocalPath);
			f >> pfLocalTargetTile;
		}
	}
	f >> nextAttack;
	f >> timer;
	f >> ignore;
	f >> morale;
	if(LOAD_VERSION < V_0_12)
		f.Skip<float>(); // old last_scan
	f >> startRot;
	if(unit->data->abilities)
		f >> cooldown;
	switch(state)
	{
	case Cast:
		if(LOAD_VERSION >= V_0_13)
			st.cast.ability = Ability::Get(f.Read<int>());
		else
		{
			st.cast.ability = unit->data->abilities->ability[unit->aiMode];
			if(LOAD_VERSION < V_0_12)
				f.Skip<int>(); // old cast_target
		}
		break;
	case Escape:
		{
			int room_id = f.Read<int>();
			if(room_id != -1)
				st.escape.room = reinterpret_cast<InsideLocation*>(gameLevel->location)->GetLevelData().rooms[room_id];
			else
				st.escape.room = nullptr;
		}
		break;
	case Idle:
		if(LOAD_VERSION >= V_0_13)
			LoadIdleAction(f, st.idle, true);
		break;
	case SearchEnemy:
		{
			int room_id = f.Read<int>();
			st.search.room = reinterpret_cast<InsideLocation*>(gameLevel->location)->GetLevelData().rooms[room_id];
		}
		break;
	}
	f >> havePotion;
	if(LOAD_VERSION >= V_0_12)
		f >> haveMpPotion;
	else
		haveMpPotion = HavePotion::Check;
	f >> potion;
	if(LOAD_VERSION < V_0_13)
	{
		if(state == Idle)
			LoadIdleAction(f, st.idle, true);
		else
		{
			StateData::IdleState idle;
			LoadIdleAction(f, idle, false);
		}
	}
	f >> cityWander;
	f >> locTimer;
	if(LOAD_VERSION < V_0_13)
		f.Skip<float>(); // old shoot_yspeed
	if(LOAD_VERSION < V_0_12)
	{
		bool goto_inn;
		f >> goto_inn;
		if(goto_inn)
			unit->OrderGoToInn();
	}
	if(LOAD_VERSION >= V_0_19)
		f >> scanTimer;
	else
		scanTimer = Random(0.f, 0.2f);
	changeAiMode = false;
}

//=================================================================================================
void AIController::LoadIdleAction(GameReader& f, StateData::IdleState& idle, bool apply)
{
	f >> idle.action;
	switch(idle.action)
	{
	case Idle_None:
	case Idle_Animation:
		break;
	case Idle_Rot:
		f >> idle.rot;
		break;
	case Idle_Move:
	case Idle_TrainCombat:
	case Idle_Pray:
		f >> idle.pos;
		break;
	case Idle_Look:
	case Idle_WalkTo:
	case Idle_Chat:
	case Idle_WalkNearUnit:
	case Idle_MoveAway:
		f >> idle.unit;
		break;
	case Idle_Use:
	case Idle_WalkUse:
	case Idle_WalkUseEat:
		f >> idle.usable;
		break;
	case Idle_TrainBow:
		f >> idle.obj.pos;
		f >> idle.obj.rot;
		if(apply)
			game->aiBowTargets.push_back(this);
		break;
	case Idle_MoveRegion:
	case Idle_RunRegion:
		{
			int partId;
			f >> partId;
			f >> idle.region.pos;
			if(LOAD_VERSION >= V_0_11)
			{
				f >> idle.region.exit;
				idle.region.locPart = gameLevel->GetLocationPartById(partId);
			}
			else
			{
				if(partId == LocationPart::OLD_EXIT_ID)
				{
					idle.region.exit = true;
					idle.region.locPart = gameLevel->GetLocationPartById(LocationPart::OUTSIDE_ID);
				}
				else
				{
					idle.region.exit = false;
					idle.region.locPart = gameLevel->GetLocationPartById(partId);
					if(!idle.region.locPart)
						idle.region.locPart = gameLevel->localPart;
				}
			}
		}
		break;
	default:
		assert(0);
		break;
	}
}

//=================================================================================================
bool AIController::CheckPotion(bool inCombat)
{
	if(unit->action != A_NONE)
		return false;

	if(havePotion != HavePotion::No && !IsSet(unit->data->flags, F_UNDEAD))
	{
		float hpp = unit->GetHpp();
		if(hpp < 0.5f || (hpp < 0.75f && !inCombat) || (!Equal(hpp, 1.f) && unit->busy == Unit::Busy_Tournament))
		{
			int index = unit->FindHealingPotion();
			if(index == -1)
			{
				if(unit->busy == Unit::Busy_No && unit->IsFollower() && !unit->summoner)
					unit->Talk(RandomString(game->txAiNoHpPot));
				havePotion = HavePotion::No;
				return false;
			}

			if(unit->ConsumeItem(index) != 3 && this->inCombat)
				state = Dodge;
			timer = Random(1.f, 1.5f);

			return true;
		}
	}

	if(haveMpPotion != HavePotion::No)
	{
		Class* clas = unit->GetClass();
		if(!clas || !IsSet(clas->flags, Class::F_MP_BAR))
			haveMpPotion = HavePotion::No;
		else
		{
			float mpp = unit->GetMpp();
			if(mpp < 0.33f || (mpp < 0.66f && !inCombat) || (mpp < 0.80f && unit->busy == Unit::Busy_Tournament))
			{
				int index = unit->FindManaPotion();
				if(index == -1)
				{
					if(unit->busy == Unit::Busy_No && unit->IsFollower() && !unit->summoner)
						unit->Talk(RandomString(game->txAiNoMpPot));
					haveMpPotion = HavePotion::No;
					return false;
				}

				if(unit->ConsumeItem(index) != 3 && this->inCombat)
					state = Dodge;
				timer = Random(1.f, 1.5f);

				return true;
			}
		}
	}

	return false;
}

//=================================================================================================
void AIController::Reset()
{
	if(state != AIController::Idle)
	{
		state = Idle;
		changeAiMode = true;
	}
	target = nullptr;
	alertTarget = nullptr;
	pfPath.clear();
	pfLocalPath.clear();
	inCombat = false;
	nextAttack = 0.f;
	timer = Random(1.f, 3.f);
	scanTimer = Random(0.f, 0.2f);
	ignore = 0.f;
	morale = unit->GetMaxMorale();
	cooldown[0] = 0.f;
	cooldown[1] = 0.f;
	cooldown[2] = 0.f;
	havePotion = HavePotion::Check;
	haveMpPotion = HavePotion::Check;
	st.idle.action = Idle_None;
	pfState = PFS_NOT_USING;
}

//=================================================================================================
float AIController::GetMorale() const
{
	float m = morale;
	float hpp = unit->GetHpp();

	if(hpp < 0.1f)
		m -= 3.f;
	else if(hpp < 0.25f)
		m -= 2.f;
	else if(hpp < 0.5f)
		m -= 1.f;

	return m;
}

//=================================================================================================
bool AIController::CanWander() const
{
	if(gameLevel->cityCtx && locTimer <= 0.f && !game->dontWander && IsSet(unit->data->flags, F_AI_WANDERS))
	{
		if(unit->busy != Unit::Busy_No)
			return false;
		if(unit->IsHero())
		{
			if(unit->hero->teamMember && unit->GetOrder() != ORDER_WANDER)
				return false;
			else if(questMgr->questTournament->IsGenerated())
				return false;
			else
				return true;
		}
		else if(unit->locPart->partType == LocationPart::Type::Outside)
			return true;
		else
			return false;
	}
	else
		return false;
}

//=================================================================================================
Vec3 AIController::PredictTargetPos(const Unit& target, float bulletSpeed) const
{
	if(bulletSpeed == 0.f)
		return target.GetCenter();

	Vec3 vel = target.pos - target.prevPos;
	vel *= 60;

	float a = bulletSpeed * bulletSpeed - vel.Dot2d();
	float b = -2 * vel.Dot2d(target.pos - unit->pos);
	float c = -(target.pos - unit->pos).Dot2d();

	float delta = b * b - 4 * a * c;
	// no solution, can't hit target, just aim at center
	if(delta < 0)
		return target.GetCenter();

	Vec3 pos = target.pos + ((b + std::sqrt(delta)) / (2 * a)) * Vec3(vel.x, 0, vel.z);
	pos.y += target.GetUnitHeight() / 2;
	return pos;
}

//=================================================================================================
void AIController::Shout()
{
	if(!unit->data->sounds->Have(SOUND_SEE_ENEMY))
		return;

	game->PlayAttachedSound(*unit, unit->data->sounds->Random(SOUND_SEE_ENEMY), Unit::ALERT_SOUND_DIST);
	if(Net::IsOnline())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::UNIT_SOUND;
		c.unit = unit;
		c.id = SOUND_SEE_ENEMY;
	}

	// alarm near allies
	Unit* target_unit = target;
	for(Unit* u : unit->locPart->units)
	{
		if(u->toRemove || unit == u || !u->IsStanding() || u->IsPlayer() || !unit->IsFriend(*u) || u->ai->state == Fighting
			|| u->ai->alertTarget || u->dontAttack)
			continue;

		if(Vec3::Distance(unit->pos, u->pos) <= ALERT_RANGE && gameLevel->CanSee(*unit, *u))
		{
			u->ai->alertTarget = target_unit;
			u->ai->alertTargetPos = targetLastPos;
		}
	}
}

//=================================================================================================
void AIController::HitReaction(const Vec3& pos)
{
	if(unit->dontAttack || !Any(state, Idle, SearchEnemy))
		return;

	target = nullptr;
	targetLastPos = pos;
	if(state == Idle)
		changeAiMode = true;
	state = SeenEnemy;
	timer = Random(10.f, 15.f);
	cityWander = false;
	if(!unit->data->sounds->Have(SOUND_SEE_ENEMY))
		return;

	game->PlayAttachedSound(*unit, unit->data->sounds->Random(SOUND_SEE_ENEMY), Unit::ALERT_SOUND_DIST);

	if(Net::IsOnline())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::UNIT_SOUND;
		c.unit = unit;
		c.id = SOUND_SEE_ENEMY;
	}

	// alarm near allies
	for(Unit* u : unit->locPart->units)
	{
		if(u->toRemove || unit == u || !u->IsStanding() || u->IsPlayer() || !unit->IsFriend(*u) || u->dontAttack)
			continue;

		if((u->ai->state == Idle || u->ai->state == SearchEnemy)
			&& Vec3::Distance(unit->pos, u->pos) <= ALERT_RANGE && gameLevel->CanSee(*unit, *u))
		{
			AIController* ai2 = u->ai;
			ai2->targetLastPos = pos;
			ai2->state = SeenEnemy;
			ai2->timer = Random(5.f, 10.f);
		}
	}
}

//=================================================================================================
// when target is nullptr, it deals no damage (dummy training)
void AIController::DoAttack(Unit* target, bool running)
{
	if(!(unit->action == A_NONE && (unit->meshInst->mesh->head.n_groups == 1 || unit->weaponState == WeaponState::Taken) && nextAttack <= 0.f))
		return;

	if(unit->data->sounds->Have(SOUND_ATTACK) && Rand() % 4 == 0)
		game->PlayAttachedSound(*unit, unit->data->sounds->Random(SOUND_ATTACK), Unit::ATTACK_SOUND_DIST);
	unit->action = A_ATTACK;
	unit->act.attack.index = unit->GetRandomAttack();

	bool doPowerAttack;
	if(!IsSet(unit->data->flags, F_NO_POWER_ATTACK))
	{
		if(target && target->action == A_BLOCK)
			doPowerAttack = (Rand() % 2 == 0);
		else
			doPowerAttack = (Rand() % 5 == 0);
	}
	else
		doPowerAttack = false;

	if(running)
	{
		unit->act.attack.power = 1.5f;
		unit->act.attack.run = true;
		doPowerAttack = false;
	}
	else
	{
		unit->act.attack.power = 1.f;
		unit->act.attack.run = false;
	}

	float staminaCost = (running || doPowerAttack) ? 1.5f : 1.f;
	if(unit->HaveWeapon())
	{
		const Weapon& weapon = unit->GetWeapon();
		staminaCost *= weapon.GetInfo().stamina * unit->GetStaminaMod(weapon);
	}
	else
		staminaCost *= Unit::STAMINA_UNARMED_ATTACK;
	unit->RemoveStamina(staminaCost);

	float speed = (doPowerAttack ? unit->GetPowerAttackSpeed() : unit->GetAttackSpeed()) * unit->GetStaminaAttackSpeedMod();

	if(unit->meshInst->mesh->head.n_groups > 1)
	{
		unit->meshInst->Play(NAMES::aniAttacks[unit->act.attack.index], PLAY_PRIO1 | PLAY_ONCE, 1);
		unit->meshInst->groups[1].speed = speed;
	}
	else
	{
		unit->meshInst->Play(NAMES::aniAttacks[unit->act.attack.index], PLAY_PRIO1 | PLAY_ONCE, 0);
		unit->meshInst->groups[0].speed = speed;
		unit->animation = ANI_PLAY;
	}
	unit->animationState = (doPowerAttack ? AS_ATTACK_PREPARE : AS_ATTACK_CAN_HIT);
	unit->act.attack.hitted = !target;

	if(Net::IsOnline())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::ATTACK;
		c.unit = unit;
		c.id = (doPowerAttack ? AID_PrepareAttack : (running ? AID_RunningAttack : AID_Attack));
		c.f[1] = speed;
	}
}
