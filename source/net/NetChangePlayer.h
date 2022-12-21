#pragma once

//-----------------------------------------------------------------------------
struct NetChangePlayer
{
	enum TYPE
	{
		LOOT, // response to looting [bool(id)-can loot, if can: items]
		START_DIALOG, // start dialog with unit or is busy [int(id)-unit]
		END_DIALOG, // end of dialog []
		SHOW_DIALOG_CHOICES, // show dialog choices [auto:byte-count, char-escape choice, vector<string1>-choices]
		START_TRADE, // start trade [int(id)-unit, auto:ItemList]
		START_SHARE, // start sharing items [auto:int-weight, int-weight max, int-gold, stats, items]
		START_GIVE, // start giving items [auto:int-weight, int-weight max, int-gold, stats, items]
		SET_FROZEN, // change player frozen state [byte(id)-state]
		REMOVE_QUEST_ITEM, // remove quest item from inventory [int(id)-quest id]
		DEVMODE, // change devmode for player [bool(id)-allowed]
		USE_USABLE, // someone else is using usable []
		IS_BETTER_ITEM, // response to is IS_BETTER_ITEM (bool(id)-is better)
		PVP, // question about pvp [byte(id)-player id]
		ADD_ITEMS, // add multiple same items to inventory [int(id)-team count, int(count)-count, Item(item)]
		ADD_ITEMS_TRADER, // add items to trader which is trading with player [int(id)-unit, int(count)-count, int(a)-team count, Item(item)]
		ADD_ITEMS_CHEST, // add items to chest which is open by player [int(id)-chest, int(count)-count, int(a)-team count, Item(item)]
		REMOVE_ITEMS, // remove items from inventory [int(id)-iIndex, int(count)-count]
		REMOVE_ITEMS_TRADER, // remove items from traded inventory which is trading with player [int(id)-unit, int(count)-count, int(a)-iIndex]
		PREPARE_WARP, // preparing to warp []
		ENTER_ARENA, // entering arena []
		START_ARENA_COMBAT, // start of arena combat []
		EXIT_ARENA, // exit from arena []
		NO_PVP, // player refused to pvp [byte(id)-player id]
		CANT_LEAVE_LOCATION, // can't leave location message [byte(id)-reason (CanLeaveLocationResult)]
		LOOK_AT, // force player to look at unit [int(id)-unit or null]
		END_FALLBACK, // end of fallback []
		REST, // response to rest in inn [byte(id)-days]
		TRAIN, // response to training [byte(id)-type (0-attribute, 1-skill, 2-tournament, 3-perk, 4-ability), int(count)-value]
		UNSTUCK, // warped player to not stuck position [Vec3(pos)]
		GOLD_RECEIVED, // message about receiving gold from another player [byte(id)-player id, int(count)-count]
		UPDATE_TRADER_GOLD, // update trader gold [int(id)-unit gold, int(count)-count]
		UPDATE_TRADER_INVENTORY, // update trader inventory after getting item [int(id)-unit, ItemListTeam]
		PLAYER_STATS, // update player statistics [byte(id)-flags, vector<int>-values]
		STAT_CHANGED, // player stat changed [byte(id)-ChangedStatType, byte(a)-stat id, int(count)-value]
		ADD_PERK, // add perk to player [int(id)-perk hash, char(count)-value]
		REMOVE_PERK, // remvoe perk from player [int(id)-perk hash, char(count)-value]
		GAME_MESSAGE, // show game message [int(id)-game message id]
		RUN_SCRIPT_RESULT, // run script result [string(str)-output]
		GENERIC_CMD_RESPONSE, // response to GENERIC_CMD [string(str)-result]
		ADD_EFFECT, // add effect to player [char(id)-effect, char(count)-source, char(a1)-sourceId, char(a2)-value, pos.x-power, pos.y-time]
		REMOVE_EFFECT, // remove effect from player [char(id)-effect, char(count)-source, char(a1)-sourceId, char(a2)-value]
		ON_REST, // player is resting [byte(count)-days]
		GAME_MESSAGE_FORMATTED, // add formatted message [int(id)-game message id, int(a)-subtype, int(count)-value]
		SOUND, // play sound [int(id)-sound id (0-gold)]
		ADD_ABILITY, // add ability to player [int(ability->hash)]
		REMOVE_ABILITY, // remove ability from player [int(ability->hash)]
		AFTER_CRAFT, // after crafting - update ingredients, play sound
		ADD_RECIPE, // add recipe to player [int(recipe->hash)]
		END_PREPARE, // end prepare action []

		MAX
	} type;
	int id, count;
	union
	{
		int a;
		Unit* unit;
		string* str;
		Ability* ability;
		Recipe* recipe;
		struct
		{
			short a1, a2;
		};
	};
	const Item* item;
	Vec3 pos;
};
static_assert((int)NetChangePlayer::MAX <= 256, "too many NetChangePlayer");
