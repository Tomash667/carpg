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

//=================================================================================================
void Quest_Secret::InitOnce()
{
	QM.RegisterSpecialHandler(this, "secret_attack");
	QM.RegisterSpecialHandler(this, "secret_reward");
	QM.RegisterSpecialIfHandler(this, "secret_first_dialog");
	QM.RegisterSpecialIfHandler(this, "secret_can_fight");
	QM.RegisterSpecialIfHandler(this, "secret_win");
	QM.RegisterSpecialIfHandler(this, "secret_can_get_reward");

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
void Quest_Secret::Special(DialogContext& ctx, cstring msg)
{
	Game& game = Game::Get();
	if(strcmp(msg, "secret_attack") == 0)
	{
		state = SECRET_FIGHT;
		game.at_arena.clear();

		ctx.talker->in_arena = 1;
		game.at_arena.push_back(ctx.talker);
		if(Net::IsOnline())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::CHANGE_ARENA_STATE;
			c.unit = ctx.talker;
		}

		for(Unit* unit : Team.members)
		{
			unit->in_arena = 0;
			game.at_arena.push_back(unit);
			if(Net::IsOnline())
			{
				NetChange& c = Add1(Net::changes);
				c.type = NetChange::CHANGE_ARENA_STATE;
				c.unit = unit;
			}
		}
	}
	else if(strcmp(msg, "secret_reward") == 0)
	{
		state = SECRET_REWARD;
		const Item* item = Item::Get("sword_forbidden");
		ctx.pc->unit->AddItem2(item, 1u, 1u);
	}
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
	}
	else if(strcmp(msg, "secret_can_fight") == 0)
		return state == SECRET_TALKED;
	else if(strcmp(msg, "secret_win") == 0)
		return Any(state, SECRET_WIN, SECRET_REWARD);
	else if(strcmp(msg, "secret_can_get_reward") == 0)
		return state == SECRET_WIN;
	return false;
}

//=================================================================================================
bool Quest_Secret::CheckMoonStone(GroundItem* item, Unit& unit)
{
	assert(item);

	if(state == SECRET_NONE && W.GetCurrentLocation()->type == L_MOONWELL && item->item->id == "krystal"
		&& Vec3::Distance2d(item->pos, Vec3(128.f, 0, 128.f)) < 1.2f)
	{
		Game::Get().AddGameMsg(txSecretAppear, 3.f);
		state = SECRET_DROPPED_STONE;
		Location& l = *W.CreateLocation(L_DUNGEON, Vec2(0, 0), -128.f, DWARF_FORT, SG_CHALLANGE, false, 3);
		l.st = 18;
		l.active_quest = (Quest_Dungeon*)ACTIVE_QUEST_HOLDER;
		l.state = LS_UNKNOWN;
		where = l.index;
		Vec2& cpos = W.GetCurrentLocation()->pos;
		Item* note = &GetNote();
		note->desc = Format("\"%c %d km, %c %d km\"", cpos.y > l.pos.y ? 'S' : 'N', (int)abs((cpos.y - l.pos.y) / 3), cpos.x > l.pos.x ? 'W' : 'E', (int)abs((cpos.x - l.pos.x) / 3));
		unit.AddItem2(note, 1u, 1u, false);
		delete item;
		if(Net::IsOnline())
			Net::PushChange(NetChange::SECRET_TEXT);
		return true;
	}

	return false;
}
