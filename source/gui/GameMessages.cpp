#include "Pch.h"
#include "GameMessages.h"
#include "Game.h"
#include "Language.h"
#include "PlayerInfo.h"
#include "SaveState.h"
#include "SoundManager.h"
#include "ResourceManager.h"
#include "GameGui.h"

//=================================================================================================
void GameMessages::LoadLanguage()
{
	txGamePausedBig = Str("gamePausedBig");
	txINeedWeapon = Str("iNeedWeapon");
	txNoHealthPotion = Str("noHealthPotion");
	txNoManaPotion = Str("noManaPotion");
	txCantDo = Str("cantDo");
	txDontLootFollower = Str("dontLootFollower");
	txDontLootArena = Str("dontLootArena");
	txUnlockedDoor = Str("unlockedDoor");
	txNeedKey = Str("needKey");
	txGmsLooted = Str("gmsLooted");
	txGmsRumor = Str("gmsRumor");
	txGmsJournalUpdated = Str("gmsJournalUpdated");
	txGmsUsed = Str("gmsUsed");
	txGmsUnitBusy = Str("gmsUnitBusy");
	txGmsGatherTeam = Str("gmsGatherTeam");
	txGmsNotLeader = Str("gmsNotLeader");
	txGmsNotInCombat = Str("gmsNotInCombat");
	txGmsAddedItem = Str("gmsAddedItem");
	txGmsGettingOutOfRange = Str("gmsGettingOutOfRange");
	txGmsLeftEvent = Str("gmsLeftEvent");
	txGameSaved = Str("gameSaved");
	txGainTextAttrib = Str("gainTextAttrib");
	txGainTextSkill = Str("gainTextSkill");
	txGainLearningPoints = Str("gainLearningPoints");
	txLearnedPerk = Str("learnedPerk");
	txTooComplicated = Str("tooComplicated");
	txAddedCursedStone = Str("addedCursedStone");
	txGameLoaded = Str("gameLoaded");
	txGoldPlus = Str("goldPlus");
	txQuestCompletedGold = Str("questCompletedGold");
	txGmsAddedItems = Str("gmsAddedItems");
	txNeedWand = Str("needWand");
	txLearnedAbility = Str("learnedAbility");
}

//=================================================================================================
void GameMessages::LoadData()
{
	snd_scribble = res_mgr->Load<Sound>("scribble.mp3");
}

//=================================================================================================
void GameMessages::Reset()
{
	msgs.clear();
	msgs_h = 0;
}

//=================================================================================================
void GameMessages::Draw(ControlDrawData*)
{
	for(list<GameMsg>::iterator it = msgs.begin(), end = msgs.end(); it != end; ++it)
	{
		int a = 255;
		if(it->fade < 0)
			a = 255 + int(it->fade * 2550);
		else if(it->fade > 0 && it->time < 0.f)
			a = 255 - int(it->fade * 2550);
		Rect rect = { 0, int(it->pos.y) - it->size.y / 2, gui->wnd_size.x, int(it->pos.y) + it->size.y / 2 };
		gui->DrawText(GameGui::font, it->msg, DTF_CENTER | DTF_OUTLINE, Color::Alpha(a), rect);
	}

	if(game->paused)
	{
		Rect r = { 0, 0, gui->wnd_size.x, gui->wnd_size.y };
		gui->DrawText(GameGui::font_big, txGamePausedBig, DTF_CENTER | DTF_VCENTER, Color::Black, r);
	}
}

//=================================================================================================
void GameMessages::Update(float dt)
{
	int h = 0, total_h = msgs_h;

	for(list<GameMsg>::iterator it = msgs.begin(), end = msgs.end(); it != end; ++it)
	{
		GameMsg& m = *it;
		m.time -= dt;
		h += m.size.y;

		if(m.time >= 0.f)
		{
			m.fade += dt;
			if(m.fade > 0.f)
				m.fade = 0.f;
		}
		else
		{
			m.fade -= dt;
			if(m.fade < -0.1f)
			{
				msgs_h -= m.size.y;
				it = msgs.erase(it);
				end = msgs.end();
				if(it == end)
					break;
				else
					continue;
			}
		}

		float target_h = float(gui->wnd_size.y) / 2 - float(total_h) / 2 + h;
		m.pos.y += (target_h - m.pos.y)*dt * 2;
	}
}

//=================================================================================================
void GameMessages::Save(FileWriter& f) const
{
	f << msgs.size();
	for(const GameMsg& msg : msgs)
	{
		f.WriteString2(msg.msg);
		f << msg.time;
		f << msg.fade;
		f << msg.pos;
		f << msg.size;
		f << msg.type;
		f << msg.subtype;
		f << msg.value;
	}
	f << msgs_h;
}

//=================================================================================================
void GameMessages::Load(FileReader& f)
{
	msgs.resize(f.Read<uint>());
	for(GameMsg& msg : msgs)
	{
		f.ReadString2(msg.msg);
		f >> msg.time;
		f >> msg.fade;
		f >> msg.pos;
		f >> msg.size;
		f >> msg.type;
		if(LOAD_VERSION >= V_0_8)
		{
			f >> msg.subtype;
			f >> msg.value;
		}
		else
		{
			msg.subtype = -1;
			msg.value = 0;
		}
	}
	f >> msgs_h;
}

//=================================================================================================
void GameMessages::AddMessage(cstring text, float time, int type, int subtype, int value)
{
	assert(text && time > 0.f);

	GameMsg& m = Add1(msgs);
	m.msg = text;
	m.type = type;
	m.subtype = subtype;
	m.time = time;
	m.fade = -0.1f;
	m.size = GameGui::font->CalculateSize(text, gui->wnd_size.x - 64);
	m.size.y += 6;
	m.value = value;

	if(msgs.size() == 1u)
		m.pos = Vec2(float(gui->wnd_size.x) / 2, float(gui->wnd_size.y) / 2);
	else
	{
		list<GameMsg>::reverse_iterator it = msgs.rbegin();
		++it;
		GameMsg& prev = *it;
		m.pos = Vec2(float(gui->wnd_size.x) / 2, prev.pos.y + prev.size.y);
	}

	msgs_h += m.size.y;
}

//=================================================================================================
void GameMessages::AddMessageIfNotExists(cstring text, float time, int type)
{
	if(type >= 0)
	{
		for(GameMsg& msg : msgs)
		{
			if(msg.type == type)
			{
				msg.time = time;
				return;
			}
		}
	}
	else
	{
		for(GameMsg& msg : msgs)
		{
			if(msg.msg == text)
			{
				msg.time = time;
				return;
			}
		}
	}

	AddMessage(text, time, type);
}

//=================================================================================================
void GameMessages::AddGameMsg3(GMS id)
{
	cstring text;
	float time = 3.f;
	bool repeat = false;

	switch(id)
	{
	case GMS_IS_LOOTED:
		text = txGmsLooted;
		break;
	case GMS_ADDED_RUMOR:
		repeat = true;
		text = txGmsRumor;
		sound_mgr->PlaySound2d(snd_scribble);
		break;
	case GMS_JOURNAL_UPDATED:
		repeat = true;
		text = txGmsJournalUpdated;
		sound_mgr->PlaySound2d(snd_scribble);
		break;
	case GMS_USED:
		text = txGmsUsed;
		time = 2.f;
		break;
	case GMS_UNIT_BUSY:
		text = txGmsUnitBusy;
		break;
	case GMS_GATHER_TEAM:
		text = txGmsGatherTeam;
		break;
	case GMS_NOT_LEADER:
		text = txGmsNotLeader;
		break;
	case GMS_NOT_IN_COMBAT:
		text = txGmsNotInCombat;
		break;
	case GMS_ADDED_ITEM:
		text = txGmsAddedItem;
		repeat = true;
		break;
	case GMS_GETTING_OUT_OF_RANGE:
		text = txGmsGettingOutOfRange;
		break;
	case GMS_LEFT_EVENT:
		text = txGmsLeftEvent;
		break;
	case GMS_GAME_SAVED:
		text = txGameSaved;
		time = 1.f;
		break;
	case GMS_NEED_WEAPON:
		text = txINeedWeapon;
		time = 2.f;
		break;
	case GMS_NO_HEALTH_POTION:
		text = txNoHealthPotion;
		time = 2.f;
		break;
	case GMS_NO_MANA_POTION:
		text = txNoManaPotion;
		time = 2.f;
		break;
	case GMS_CANT_DO:
		text = txCantDo;
		break;
	case GMS_DONT_LOOT_FOLLOWER:
		text = txDontLootFollower;
		break;
	case GMS_DONT_LOOT_ARENA:
		text = txDontLootArena;
		break;
	case GMS_UNLOCK_DOOR:
		text = txUnlockedDoor;
		break;
	case GMS_NEED_KEY:
		text = txNeedKey;
		break;
	case GMS_LEARNED_PERK:
		text = txLearnedPerk;
		break;
	case GMS_TOO_COMPLICATED:
		text = txTooComplicated;
		time = 2.5f;
		break;
	case GMS_ADDED_CURSED_STONE:
		text = txAddedCursedStone;
		break;
	case GMS_GAME_LOADED:
		text = txGameLoaded;
		time = 1.f;
		break;
	case GMS_NEED_WAND:
		text = txNeedWand;
		time = 2.f;
		break;
	case GMS_LEARNED_ABILITY:
		text = txLearnedAbility;
		break;
	default:
		assert(0);
		return;
	}

	if(repeat)
		AddGameMsg(text, time);
	else
		AddGameMsg2(text, time, id);
}

//=================================================================================================
void GameMessages::AddGameMsg3(PlayerController* player, GMS id)
{
	assert(player);
	if(player->is_local)
		AddGameMsg3(id);
	else
	{
		NetChangePlayer& c = Add1(player->player_info->changes);
		c.type = NetChangePlayer::GAME_MESSAGE;
		c.id = id;
	}
}

//=================================================================================================
void GameMessages::AddFormattedMessage(PlayerController* player, GMS id, int subtype, int value)
{
	assert(player);
	if(player->is_local)
	{
		GameMsg* existing = nullptr;
		for(GameMsg& msg : msgs)
		{
			if(msg.type == id && msg.subtype == subtype)
			{
				existing = &msg;
				value += msg.value;
				break;
			}
		}

		cstring text;
		float time = 3.f;
		switch(id)
		{
		case GMS_GAIN_LEARNING_POINTS:
			text = Format(txGainLearningPoints, value);
			break;
		case GMS_GAIN_ATTRIBUTE:
			text = Format(txGainTextAttrib, Attribute::attributes[subtype].name.c_str(), value);
			break;
		case GMS_GAIN_SKILL:
			text = Format(txGainTextSkill, Skill::skills[subtype].name.c_str(), value);
			break;
		case GMS_GOLD_ADDED:
			text = Format(txGoldPlus, value);
			break;
		case GMS_QUEST_COMPLETED_GOLD:
			text = Format(txQuestCompletedGold, value);
			time = 4.f;
			break;
		case GMS_ADDED_ITEMS:
			text = Format(txGmsAddedItems, value);
			break;
		default:
			assert(0);
			return;
		}

		if(existing)
		{
			GameMsg& msg = *existing;
			msg.msg = text;
			msg.time = time;
			msg.size = GameGui::font->CalculateSize(text, gui->wnd_size.x - 64);
			msg.size.y += 6;
			msg.value = value;
		}
		else
			AddMessage(text, time, id, subtype, value);
	}
	else
	{
		NetChangePlayer& c = Add1(player->player_info->changes);
		c.type = NetChangePlayer::GAME_MESSAGE_FORMATTED;
		c.id = id;
		c.count = value;
		c.a = subtype;
	}
}

//=================================================================================================
void GameMessages::AddItemMessage(PlayerController* player, uint count)
{
	if(count == 1u)
		AddGameMsg3(player, GMS_ADDED_ITEM);
	else
		AddFormattedMessage(player, GMS_ADDED_ITEMS, 0, count);
}
