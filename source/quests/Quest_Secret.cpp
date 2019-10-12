#include "Pch.h"
#include "GameCore.h"
#include "Quest_Secret.h"
#include "BaseObject.h"
#include "GameFile.h"
#include "QuestManager.h"
#include "Team.h"
#include "World.h"
#include "Location.h"
#include "Language.h"
#include "Game.h"
#include "GameGui.h"
#include "GameMessages.h"
#include "Arena.h"
#include "AIController.h"
#include "GroundItem.h"
#include "OutsideLocation.h"

//=================================================================================================
void Quest_Secret::InitOnce()
{
	quest_mgr->RegisterSpecialHandler(this, "secret_attack");
	quest_mgr->RegisterSpecialHandler(this, "secret_reward");
	quest_mgr->RegisterSpecialIfHandler(this, "secret_first_dialog");
	quest_mgr->RegisterSpecialIfHandler(this, "secret_can_fight");
	quest_mgr->RegisterSpecialIfHandler(this, "secret_win");
	quest_mgr->RegisterSpecialIfHandler(this, "secret_can_get_reward");
}

//=================================================================================================
void Quest_Secret::LoadLanguage()
{
	txSecretAppear = Str("secretAppear");
}

//=================================================================================================
void Quest_Secret::Init()
{
	state = (BaseObject::Get("tomashu_dom")->mesh ? SECRET_NONE : SECRET_OFF);
	GetNote().desc.clear();
	where = -1;
	where2 = -1;
}

//=================================================================================================
void Quest_Secret::Save(GameWriter& f)
{
	f << state;
	f << GetNote().desc;
	f << where;
	f << where2;
}

//=================================================================================================
void Quest_Secret::Load(GameReader& f)
{
	f >> state;
	f >> GetNote().desc;
	f >> where;
	f >> where2;
	if(state > SECRET_NONE && !BaseObject::Get("tomashu_dom")->mesh)
		throw "Save uses 'data.pak' file which is missing!";
}

//=================================================================================================
bool Quest_Secret::Special(DialogContext& ctx, cstring msg)
{
	if(strcmp(msg, "secret_attack") == 0)
	{
		state = SECRET_FIGHT;
		game->arena->units.clear();

		ctx.talker->in_arena = 1;
		game->arena->units.push_back(ctx.talker);
		if(Net::IsOnline())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::CHANGE_ARENA_STATE;
			c.unit = ctx.talker;
		}

		for(Unit& unit : team->members)
		{
			unit.in_arena = 0;
			game->arena->units.push_back(&unit);
			if(Net::IsOnline())
			{
				NetChange& c = Add1(Net::changes);
				c.type = NetChange::CHANGE_ARENA_STATE;
				c.unit = &unit;
			}
		}
	}
	else if(strcmp(msg, "secret_reward") == 0)
	{
		state = SECRET_REWARD;
		const Item* item = Item::Get("sword_forbidden");
		ctx.pc->unit->AddItem2(item, 1u, 1u);
	}
	else
		assert(0);
	return false;
}

//=================================================================================================
bool Quest_Secret::SpecialIf(DialogContext& ctx, cstring msg)
{
	if(strcmp(msg, "secret_first_dialog") == 0)
	{
		if(state == SECRET_GENERATED2)
		{
			state = SECRET_TALKED;
			return true;
		}
		return false;
	}
	else if(strcmp(msg, "secret_can_fight") == 0)
		return state == SECRET_TALKED;
	else if(strcmp(msg, "secret_win") == 0)
		return Any(state, SECRET_WIN, SECRET_REWARD);
	else if(strcmp(msg, "secret_can_get_reward") == 0)
		return state == SECRET_WIN;
	assert(0);
	return false;
}

//=================================================================================================
bool Quest_Secret::CheckMoonStone(GroundItem* item, Unit& unit)
{
	assert(item);

	if(state == SECRET_NONE && world->GetCurrentLocation()->type == L_OUTSIDE && world->GetCurrentLocation()->target == MOONWELL && item->item->id == "krystal"
		&& Vec3::Distance2d(item->pos, Vec3(128.f, 0, 128.f)) < 1.2f)
	{
		game_gui->messages->AddGameMsg(txSecretAppear, 3.f);
		state = SECRET_DROPPED_STONE;
		Location& l = *world->CreateLocation(L_DUNGEON, Vec2(0, 0), -128.f, DWARF_FORT, UnitGroup::Get("challange"), false, 3);
		l.st = 18;
		l.active_quest = ACTIVE_QUEST_HOLDER;
		l.state = LS_UNKNOWN;
		where = l.index;
		Vec2& cpos = world->GetCurrentLocation()->pos;
		Item* note = &GetNote();
		note->desc = Format("\"%c %d km, %c %d km\"", cpos.y > l.pos.y ? 'S' : 'N', (int)abs((cpos.y - l.pos.y) / 3), cpos.x > l.pos.x ? 'W' : 'E', (int)abs((cpos.x - l.pos.x) / 3));
		unit.AddItem2(note, 1u, 1u, false);
		delete item;
		team->AddExp(5000);
		if(Net::IsOnline())
			Net::PushChange(NetChange::SECRET_TEXT);
		return true;
	}

	return false;
}

//=================================================================================================
void Quest_Secret::UpdateFight()
{
	if(state != SECRET_FIGHT)
		return;

	Arena* arena = game->arena;
	int count[2] = { 0 };

	for(vector<Unit*>::iterator it = arena->units.begin(), end = arena->units.end(); it != end; ++it)
	{
		if((*it)->IsStanding())
			count[(*it)->in_arena]++;
	}

	if(arena->units[0]->hp < 10.f)
		count[1] = 0;

	if(count[0] == 0 || count[1] == 0)
	{
		// o¿yw wszystkich
		for(Unit* unit : arena->units)
		{
			unit->in_arena = -1;
			if(unit->hp <= 0.f)
			{
				unit->HealPoison();
				unit->live_state = Unit::ALIVE;
				unit->mesh_inst->Play("wstaje2", PLAY_ONCE | PLAY_PRIO3, 0);
				unit->action = A_ANIMATION;
				unit->animation = ANI_PLAY;
				if(unit->IsAI())
					unit->ai->Reset();
				if(Net::IsOnline())
				{
					NetChange& c = Add1(Net::changes);
					c.type = NetChange::STAND_UP;
					c.unit = unit;
				}
			}

			if(Net::IsOnline())
			{
				NetChange& c = Add1(Net::changes);
				c.type = NetChange::CHANGE_ARENA_STATE;
				c.unit = unit;
			}
		}

		arena->units[0]->hp = arena->units[0]->hpmax;
		if(Net::IsOnline())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::UPDATE_HP;
			c.unit = arena->units[0];
		}

		if(count[0])
		{
			// gracz wygra³
			state = SECRET_WIN;
			arena->units[0]->OrderAutoTalk();
			team->AddLearningPoint();
			team->AddExp(25000);
		}
		else
		{
			// gracz przegra³
			state = SECRET_LOST;
		}

		arena->units.clear();
	}
}
