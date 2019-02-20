#pragma once

//-----------------------------------------------------------------------------
#include "Control.h"

//-----------------------------------------------------------------------------
enum GMS
{
	GMS_NEED_WEAPON = 1,
	GMS_NEED_KEY,
	GMS_DONT_LOOT_FOLLOWER,
	GMS_JOURNAL_UPDATED,
	GMS_GATHER_TEAM,
	GMS_ADDED_RUMOR,
	GMS_UNLOCK_DOOR,
	GMS_CANT_DO,
	GMS_UNIT_BUSY,
	GMS_NOT_NOW,
	GMS_NOT_IN_COMBAT,
	GMS_IS_LOOTED,
	GMS_USED,
	GMS_DONT_LOOT_ARENA,
	GMS_NOT_LEADER,
	GMS_ONLY_LEADER_CAN_TRAVEL,
	GMS_NO_POTION,
	GMS_GAME_SAVED,
	GMS_PICK_CHARACTER,
	GMS_ADDED_ITEM,
	GMS_GETTING_OUT_OF_RANGE,
	GMS_LEFT_EVENT,
	GMS_GAIN_LEARNING_POINTS,
	GMS_LEARNED_PERK,
	GMS_TOO_COMPLICATED,
	GMS_GAIN_ATTRIBUTE,
	GMS_GAIN_SKILL,
	GMS_ADDED_CURSED_STONE
};

//-----------------------------------------------------------------------------
struct GameMsg
{
	string msg;
	float time, fade;
	Vec2 pos;
	Int2 size;
	int type, subtype, value;
};

//-----------------------------------------------------------------------------
class GameMessages : public Control
{
public:
	void LoadLanguage();
	void Draw(ControlDrawData* cdd = nullptr) override;
	void Update(float dt) override;
	void Reset();
	void Save(FileWriter& f) const;
	void Load(FileReader& f);
	void AddMessage(cstring text, float time, int type, int subtype = -1, int value = 0);
	void AddMessageIfNotExists(cstring text, float time, int type);
	void AddGameMsg(cstring msg, float time) { AddMessage(msg, time, 0); }
	void AddGameMsg2(cstring msg, float time, int id = -1) { AddMessageIfNotExists(msg, time, id); }
	void AddGameMsg3(GMS id);
	void AddGameMsg3(PlayerController* player, GMS id);
	void AddFormattedMessage(PlayerController* player, GMS id, int subtype, int value);

private:
	list<GameMsg> msgs;
	int msgs_h;
	cstring txGamePausedBig, txINeedWeapon, txNoHpp, txCantDo, txDontLootFollower, txDontLootArena, txUnlockedDoor, txNeedKey, txGmsLooted, txGmsRumor,
		txGmsJournalUpdated, txGmsUsed, txGmsUnitBusy, txGmsGatherTeam, txGmsNotLeader, txGmsNotInCombat, txGmsAddedItem, txGmsGettingOutOfRange,
		txGmsLeftEvent, txGameSaved, txGainTextAttrib, txGainTextSkill, txGainLearningPoints, txLearnedPerk, txTooComplicated, txAddedCursedStone;
};
