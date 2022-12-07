#include "Pch.h"
#include "Quest_Secret.h"

#include "AIController.h"
#include "Arena.h"
#include "BaseObject.h"
#include "Game.h"
#include "GameGui.h"
#include "GameMessages.h"
#include "GroundItem.h"
#include "Language.h"
#include "Location.h"
#include "OutsideLocation.h"
#include "QuestManager.h"
#include "Team.h"
#include "Unit.h"
#include "World.h"

//=================================================================================================
void Quest_Secret::InitOnce()
{
	questMgr->RegisterSpecialHandler(this, "secret_attack");
	questMgr->RegisterSpecialHandler(this, "secret_reward");
	questMgr->RegisterSpecialIfHandler(this, "secret_first_dialog");
	questMgr->RegisterSpecialIfHandler(this, "secret_can_fight");
	questMgr->RegisterSpecialIfHandler(this, "secret_win");
	questMgr->RegisterSpecialIfHandler(this, "secret_can_get_reward");
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

		ctx.talker->inArena = 1;
		game->arena->units.push_back(ctx.talker);
		if(Net::IsOnline())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::CHANGE_ARENA_STATE;
			c.unit = ctx.talker;
		}

		for(Unit& unit : team->members)
		{
			unit.inArena = 0;
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
		gameGui->messages->AddGameMsg(txSecretAppear, 3.f);
		state = SECRET_DROPPED_STONE;
		Location& l = *world->CreateLocation(L_DUNGEON, world->GetRandomPlace(), DWARF_FORT, 3);
		l.group = UnitGroup::Get("challange");
		l.st = 18;
		l.activeQuest = ACTIVE_QUEST_HOLDER;
		l.state = LS_UNKNOWN;
		where = l.index;
		Vec2& cpos = world->GetCurrentLocation()->pos;
		Item* note = &GetNote();
		note->desc = Format("\"%c %d km, %c %d km\"", cpos.y > l.pos.y ? 'S' : 'N', (int)abs((cpos.y - l.pos.y) / 3), cpos.x > l.pos.x ? 'W' : 'E', (int)abs((cpos.x - l.pos.x) / 3));
		unit.AddItem2(note, 1u, 1u, false);
		team->AddExp(5000);
		if(Net::IsOnline())
			Net::PushChange(NetChange::SECRET_TEXT);
		if(item->count > 1)
		{
			--item->count;
			return false;
		}
		else
		{
			delete item;
			return true;
		}
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
			count[(*it)->inArena]++;
	}

	if(arena->units[0]->hp < 10.f)
		count[1] = 0;

	if(count[0] == 0 || count[1] == 0)
	{
		// revive all
		for(Unit* unit : arena->units)
		{
			unit->inArena = -1;
			if(unit->hp <= 0.f)
				unit->Standup();

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
			// player won
			state = SECRET_WIN;
			arena->units[0]->OrderAutoTalk();
			team->AddLearningPoint();
			team->AddExp(25000);
		}
		else
		{
			// player lost
			state = SECRET_LOST;
		}

		arena->units.clear();
	}
}
