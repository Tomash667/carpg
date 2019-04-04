#pragma once

//-----------------------------------------------------------------------------
struct Item;
struct PlayerController;
struct Unit;

//-----------------------------------------------------------------------------
struct NetChangePlayer
{
	enum TYPE
	{
		PICKUP, // item picked up [int(id)-count, int(count)-team count]
		LOOT, // response to looting [bool(id)-can loot, if can: ItemListTeam]
		GOLD_MSG, // message about gained gold [bool(id)-default msg, int(count)-count]
		START_DIALOG, // start dialog with unit or is busy [int(netid)-unit]
		END_DIALOG, // end of dialog []
		SHOW_DIALOG_CHOICES, // show dialog choices [auto:byte-count, char-escape choice, vector<string1>-choices]
		START_TRADE, // start trade [int(id)-unit, auto:ItemList]
		START_SHARE, // start sharing items [auto:int-weight, int-weight max, int-gold, stats, ItemListTeam]
		START_GIVE, // start giving items [auto:int-weight, int-weight max, int-gold, stats, ItemListTeam]
		SET_FROZEN, // change player frozen state [byte(id)-state]
		REMOVE_QUEST_ITEM, // remove quest item from inventory [int(id)-quest refid]
		DEVMODE, // change devmode for player [bool(id)-allowed]
		USE_USABLE, // someone else is using usable []
		IS_BETTER_ITEM, // response to is IS_BETTER_ITEM (bool(id)-is better)
		PVP, // question about pvp [byte(id)-player id]
		ADD_ITEMS, // add multiple same items to inventory [int(id)-team count, int(count)-count, Item(item)]
		ADD_ITEMS_TRADER, // add items to trader which is trading with player [int(id)-unit, int(count)-count, int(a)-team count, Item(item)]
		ADD_ITEMS_CHEST, // add items to chest which is open by player [int(id)-chest netid, int(count)-count, int(a)-team count, Item(item)]
		REMOVE_ITEMS, // remove items from inventory [int(id)-i_index, int(count)-count]
		REMOVE_ITEMS_TRADER, // remove items from traded inventory which is trading with player [int(id)-unit, int(count)-count, int(a)-i_index]
		PREPARE_WARP, // preparing to warp []
		ENTER_ARENA, // entering arena []
		START_ARENA_COMBAT, // start of arena combat []
		EXIT_ARENA, // exit from arena []
		NO_PVP, // player refused to pvp [byte(id)-player id]
		CANT_LEAVE_LOCATION, // can't leave location message [byte(id)-reason (CanLeaveLocationResult)]
		LOOK_AT, // force player to look at unit [int(netid)-unit or null]
		END_FALLBACK, // end of fallback []
		REST, // response to rest in inn [byte(id)-days]
		TRAIN, // response to training [byte(id)-type (0-attribute, 1-skill, 2-tournament, 3-perk), byte(count)-stat type]
		UNSTUCK, // warped player to not stuck position [Vec3(pos)]
		GOLD_RECEIVED, // message about receiving gold from another player [byte(id)-player id, int(count)-count]
		UPDATE_TRADER_GOLD, // update trader gold [int(id)-unit gold, int(count)-count]
		UPDATE_TRADER_INVENTORY, // update trader inventory after getting item [int(netid)-unit, ItemListTeam]
		PLAYER_STATS, // update player statistics [byte(id)-flags, vector<int>-values]
		ADDED_ITEMS_MSG, // message about gaining multiple items [byte(count)-count]
		STAT_CHANGED, // player stat changed [byte(id)-ChangedStatType, byte(a)-stat id, int(count)-value]
		ADD_PERK, // add perk to player [char(id)-perk, char(count)-value]
		REMOVE_PERK, // remvoe perk from player [char(id)-perk, char(count)-value]
		GAME_MESSAGE, // show game message [int(id)-game message id]
		RUN_SCRIPT_RESULT, // run script result [string(str)-output]
		GENERIC_CMD_RESPONSE, // response to GENERIC_CMD [string(str)-result]
		ADD_EFFECT, // add effect to player [char(id)-effect, char(count)-source, char(a1)-source_id, char(a2)-value, pos.x-power, pos.y-time]
		REMOVE_EFFECT, // remove effect from player [char(id)-effect, char(count)-source, char(a1)-source_id, char(a2)-value]
		ON_REST, // player is resting [byte(count)-days]
		GAME_MESSAGE_FORMATTED, // add formatted message [int(id)-game message id, int(a)-subtype, int(count)-value]

		MAX
	} type;
	int id, count;
	union
	{
		int a;
		Unit* unit;
		string* str;
		struct
		{
			short a1, a2;
		};
	};
	const Item* item;
	Vec3 pos;
};
static_assert((int)NetChangePlayer::MAX <= 256, "too many NetChangePlayer");
