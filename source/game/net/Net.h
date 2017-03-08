#pragma once

//-----------------------------------------------------------------------------
enum GamePacket : byte
{
	/* komunikat powitalny do serwera, size:7-21 
	byte - id
	int - wersja gry
	string1 - nick (16)
	*/
	ID_HELLO = ID_USER_PACKET_ENUM+1, 

	/* aktualizacja w lobby, size:4+
	byte - id
	byte - liczba zmian
	[tablica]
	{
		byte - rodzaj (0-aktualizuj gracza,1-dodaj gracza,2-usuñ gracza,3-liczba graczy,4-przywódca)
		byte - id gracza/liczba graczy
		[jeœli 0]
		{
			byte - czy gotowy
			byte - klasa
		}
		[jeœli 1]
		{
			string1 - nick
		}
	}
	*/
	ID_LOBBY_UPDATE,

	/* gracz nie mo¿e do³¹czyæ do serwera, size:2-6
	byte - id
	byte - powód (0-brak miejsca,1-z³a wersja,2-zajêty nick,3-z³y nick przy wczytywaniu,4-z³e ID_HELLO,5-nieznany b³¹d)
	[jeœli powód=1]
	int - wersja
	*/
	ID_CANT_JOIN,

	/* do³¹cza gracza do lobby, size:4+
	byte - id
	byte - id gracza
	byte - liczba graczy
	byte - liczba graczy w lobby
	byte - id przywódcy
	[dla ka¿dego gracza]
	{
		byte - id
		byte - gotowy
		byte - klasa
		string1 - nick
	}
	*/
	ID_JOIN,
	
	/* player changed readyness
	byte - id
	bool - ready
	*/
	ID_CHANGE_READY,

	/* wiadomoœæ do wszystkich gracz, size:4+
	byte - id
	byte - id gracza
	string1 - tekst
	*/
	ID_SAY,

	/* prywatna wiadomoœæ, size:4+
	byte - id
	byte - id gracza
	string1-tekst
	*/
	ID_WHISPER,

	/* wiadomoœæ od serwera do wszystkich, size:3+
	byte - id
	string1 - tekst
	*/
	ID_SERVER_SAY,

	/* gracz opuszcza serwer, size:1
	byte - id
	*/
	ID_LEAVE,

	/* zamykanie serwera, size:2
	byte - id
	byte - powód
	*/
	ID_SERVER_CLOSE,

	/* trwa uruchamianie serwera, size:2
	byte - id
	byte - pozosta³y czas
	*/
	ID_STARTUP,

	/* przerwano odliczanie startu, size:1 */
	ID_END_STARTUP,

	/* send to server when changed class
	byte - id
	CharacterData
	bool - ready
	------------------------------------
	Response:
	byte - id
	bool - ok
	*/
	ID_PICK_CHARACTER,

	/* info about current server task
	byte - id
	byte - state (0-generating world, 1-sending world, 2-waiting for players)
	*/
	ID_STATE,

	/* dane œwiata gry
	byte - id
	byte - liczba lokacji
	{
		byte - typ
		[jeœli insidelocation]
		byte - poziomy
		[jeœli miasto/wioska]
		{
			byte - citizens
			word - citizens_world
		}
		[jeœli wioska]
		{
			byte - budynek1
			byte - budynek2
		}
		byte - stan
		VEC2 - pozycja
		string1 - nazwa
	}
	byte - startowa lokacja
	*/
	ID_WORLD_DATA,

	ID_READY,

	/* trwa zmiana poziomu na inny, klient odsy³a ID_
	byte - id
	[SERWER]
	{
		byte - id lokacji
		byte - poziom podziemi
	}
	[KLIENT]
	byte - czy 1 wizyta [0-nie, 1-tak]
	*/
	ID_CHANGE_LEVEL,

	/*
	byte - id
	*/
	ID_LEVEL_DATA,

	ID_PLAYER_DATA2,
	ID_START,
	ID_CONTROL,
	ID_CHANGES,
	ID_PLAYER_UPDATE,

	// dane gracza wysy³ane na pocz¹tku gry [WritePlayerStartData/ReadPlayerStartData]
	ID_PLAYER_START_DATA,

	// trwa zapisywanie gry
	//ID_SAVING,

	// przejdŸ do gry na mapie œwiata
	ID_START_AT_WORLDMAP
};

//-----------------------------------------------------------------------------
struct NetChange
{
	enum TYPE
	{
		UNIT_POS, // unit position/rotation/animation [int(netid)-unit, auto from Unit: VEC3-pos, float-rot, float-animation speed, byte-animation]
		CHANGE_EQUIPMENT, // unit changed equipped item SERVER[int(netid)-unit, byte(id)-item slot, Item-item(automaticaly set)] / CLIENT[int(id)-i_index (if slot remove, else equip)]
		TAKE_WEAPON, // unit take/hide weapon SERVER[int(netid)-unit, bool(id)-hide, byte-type of weapon(auto)] / CLIENT[bool(id) hide, byte-type of weapon(auto)]
		ATTACK, // unit attack SERVER[int(netid)-unit, byte(id)-type/flags, float(f[1])-speed] / CLIENT[byte(id)-type/flags, float(f[1])-speed, [(float(f[2]) - bow yspeed]]
		CHANGE_FLAGS, // change of game flags [byte-flags (auto 0x01-bandit, 0x02-crazies attack, 0x04-anyone talking)
		FALL, // unit falls on ground [int(netid)-unit]
		DIE, // unit dies [int(netid)-unit]
		SPAWN_BLOOD, // spawn blood [byte(id)-type, VEC3(pos)]
		UPDATE_HP, // update unit hp [int(netid)-unit, auto: float-hp, float-hpmax]
		HURT_SOUND, // play unit hurt sound [int(netid)-unit]
		DROP_ITEM, // unit drops item SERVER[int(netid)-unit] / CLIENT[int(id)-i_index, int(ile)-count]
		SPAWN_ITEM, // spawn item on ground [GroundItem(item)]
		PICKUP_ITEM, // unit picks up item SERVER[int(netid)-unit, bool(ile)-up animation] / CLIENT[int(id)-item netid]
		REMOVE_ITEM, // remove ground item (picked by someone) [int(id)-item netid]
		CONSUME_ITEM, // unit consume item SERVER[int(netid)-unit, string1(Item id)-consumed item, bool(ile)-force] / CLIENT[int(id)-item index]
		HIT_SOUND, // play hit sound [VEC3(pos), byte(id)-material, byte(ile)-material2]
		STUNNED, // unit get stunned [int(netid)-unit]
		SHOOT_ARROW, // create shooted arrow [int(netid)-unit, VEC3(pos), float(f[0])-rotY, float(f[1])-speedY, float(f[2])-rotX, float(extra_f)-speed]
		LOOT_UNIT, // player wants to loot unit [int(netid)-unit]
		GET_ITEM, // player gets item from unit or container [int(id)-i_index, int(ile)-count]
		GET_ALL_ITEMS, // player picks up all items from corpse/chest []
		PUT_ITEM, // player puts item into unit or container [int(id)-i_index, int(ile)-count]
		STOP_TRADE, // player ends trade []
		UPDATE_CREDIT, // update team credit [auto: vector<size byte, int-unit netid, int-credit>]
		TAKE_ITEM_CREDIT, // player takes item for credit [int(id)-index]
		IDLE, // unit playes idle animation SERVER[int(netid)-unit, byte(id)-animation index] / CLIENT[byte(id)-animation index]
		HELLO, // play unit hello sound [int(netid)-unit]
		ALL_QUESTS_COMPLETED, // info about completing all unique quests []
		TALK, // unit talks or player start dialog SERVER[int(netid)-unit, byte(id)-animation, int(ile)-skip id, string1(str)-text] / CLIENT[int(netid)-unit]
		LOOT_CHEST, // player wants to loot chest [int(id)-chest netid]
		CHEST_OPEN, // chest opening animation [int(id)-chest netid]
		CHEST_CLOSE, // chest closing animation [int(id)-chest netid]
		CHOICE, // player selected dialog choice [byte(id)-choice]
		SKIP_DIALOG, // skip dialog by player [int(id)-skip id]
		CHANGE_LOCATION_STATE, // change location state [byte(id)-location index, byte(ile)-state(0-known,1-visited)]
		ADD_RUMOR, // add rumor to journal [string1(rumors[id])-text]
		TELL_NAME, // hero tells his name [int(netid)-unit]
		HAIR_COLOR, // change unit hair color [int(netid)-unit, auto:VEC4-color]
		ENTER_BUILDING, // player wants to enter building [byte(id)-building index]
		EXIT_BUILDING, // player wants to exit building []
		WARP, // warp unit or notify server about warping SERVER[int(netid)-unit, auto:char-in building, VEC3-pos, float-rot] / CLIENT[]
		ADD_NOTE, // player added note to journal [string1-text (automaticaly set)]
		REGISTER_ITEM, // register new item [auto:string1(base_item.id)-item id, string1(item2.id)-id, string1(item2.name)-name, string1(item2.desc)-description, int(item2.refid)-item refid]
		ADD_QUEST, // added quest [id = quest, auto: int(quest.refid), string1(quest.name), string2(quest.msgs[0]), string2(quest.msgs[1]))
		ADD_QUEST_MAIN, // added main quest [like ADD_QUEST]
		UPDATE_QUEST, // update quest [id = quest, auto: int(quest.refid), byte(quest.state), string2(quest.msgs.back())]
		RENAME_ITEM, // item rename [auto: int(base_item.refid), string1(base_item.id), string1(base_item.name)]
		UPDATE_QUEST_MULTI, // update quest with multiple texts [id = quest, int(quest.refid)-(auto), byte(quest.state)-(auto), vector<size:byte(ile), string2>]
		REMOVE_PLAYER, // remove player from game [byte(id)-player id, byte(ile)-reason]
		CHANGE_LEADER, // player wants to change leader or notification [byte(id)-player id]
		RANDOM_NUMBER, // player get random number SERVER[byte(unit.player.id), byte(id)-number] / CLIENT[byte(id)-number]
		CHEAT_WARP, // player used cheat 'warp' [byte(id)-inside building index]
		CHEAT_SKIP_DAYS, // player used cheat 'skip_days' [int(id)-days]
		CHEAT_KILLALL, // player used cheat 'killall' [int(netid)-ignored unit, byte(id)-type]
		CHEAT_NOCLIP, // player used cheat 'noclip' [bool(id)-state]
		CHEAT_GODMODE, // player used cheat 'godmode' [bool(id)-state]
		CHEAT_INVISIBLE, // player used cheat 'invisible' [bool(id)-state]
		CHEAT_SCARE, // player used cheat 'scare' []
		CHEAT_SUICIDE, // player used cheat 'suicide' []
		CHEAT_HEALUNIT, // player used cheat 'healunit' [int(netid)-unit]
		CHEAT_KILL, // player used cheat 'kill' [int(netid)-unit]
		CHEAT_HEAL, // player used cheat 'heal' []
		CHEAT_SPAWN_UNIT, // player used cheat 'spawn_unit' [string1(base_unit)-unit id, byte(ile)-count, char(id)-level, char(i)-in_arena]
		CHEAT_ADDITEM, // player used cheat 'additem' or 'addteam' [string1(base_item)-item id, byte(ile)-count, bool(id)-is team]
		CHEAT_ADDGOLD, // player used cheat 'addgold' [int(ile)-count]
		CHEAT_ADDGOLD_TEAM, // player used cheat 'addgold_team' [int(ile)-count]
		CHEAT_SETSTAT, // player used cheat setstat [byte(id)-stat id, bool(ile)-is skill, char(i)-value]
		CHEAT_MODSTAT, // player used cheat modstat [byte(id)-stat id, bool(ile)-is skill, char(i)-value]
		CHEAT_REVEAL, // player used cheat 'reveal' []
		CHEAT_GOTO_MAP, // player used cheat 'goto_map' []
		USE_USEABLE, // unit uses useable object SERVER[int(netid)-unit, int(id)-useable netid, byte(ile)-state(0-stop,1-start,2-start special)] / CLIENT[int(id)-useable netid, byte(ile)-state(0-stop,1-start)]
		STAND_UP, // unit stands up SERVER[int(netid)-unit] / CLIENT[]
		GAME_OVER, // game over []
		RECRUIT_NPC, // recruit npc to team [int(netid)-unit, auto:bool-is free]
		KICK_NPC, // kick npc out of team [int(netid)-unit]
		REMOVE_UNIT, // remove unit from game [int(netid)-unit]
		SPAWN_UNIT, // spawn new unit [int(netid)-unit]
		IS_BETTER_ITEM, // client checks if item is better for npc SERVER[bool(id)-is better] / CLINT[int(id)-i_index]
		PVP, // response to pvp request [bool(id)-is accepted]
		CHEAT_CITIZEN, // player used cheat 'citizen' []
		CHEAT_SHOW_MINIMAP, // player used cheat 'show_minimap'
		CHANGE_ARENA_STATE, // change unit arena state [int(netid)-unit, auto:char-state]
		ARENA_SOUND, // plays arena sound [byte(id)-type]
		SHOUT, // unit shout after seeing enemy [int(netid)-unit]
		LEAVE_LOCATION, // leader wants to leave location or leaving notification SERVER[] / CLIENT[char(id)-type]
		EXIT_TO_MAP, // exit to map []
		ENTER_LOCATION, // enter current location []
		TRAVEL, // leader wants to travel to location [byte(id)-location index]
		WORLD_TIME, // change world time [auto: int-worldtime, byte-day, byte-month, byte-year]
		USE_DOOR, // someone open/close door [int(id)-door netid, bool(ile)-is closing]
		CREATE_EXPLOSION, // create explosion effect [string1(spell->id), VEC3(pos)]
		REMOVE_TRAP, // remove trap [int(id)-trap netid]
		TRIGGER_TRAP, // trigger trap [int(id)-trap netid]
		TRAIN_MOVE, // player is training dexterity by moving []
		EVIL_SOUND, // play evil sound []
		ENCOUNTER, // start encounter on world map [string1(str)-encounter text]
		CLOSE_ENCOUNTER, // close encounter message box []
		CLOSE_PORTAL, // close portal in location []
		CLEAN_ALTAR, // clean altar in evil quest []
		ADD_LOCATION, // add new location [byte(id)-location index, auto: [byte(loc.type), if dungeon byte(loc.levels)], byte(loc.state), INT2(loc.pos), string1(loc.name)]
		REMOVE_CAMP, // remove camp [byte(id)-camp index]
		CHANGE_AI_MODE, // change unit ai mode [int(netid)-unit, byte-mode (0x1-dont attack, 0x02-assist, 0x04-not idle, 0x08-attack team)]
		CHANGE_UNIT_BASE, // change unit base type [int(netid)-unit, string1(unit.data.id)-base unit id]
		CHEAT_CHANGE_LEVEL, // player used cheat to change level (<>+shift+ctrl) [bool(id)-is down]
		CHEAT_WARP_TO_STAIRS, // player used cheat to warp to stairs (<>+shift) [bool(id)-is down]
		CAST_SPELL, // unit casts spell [int(netid)-unit, byte(id)-attack id]
		CREATE_SPELL_BALL, // create ball - spell effect [string1(spell->id), VEC3(pos), float(f[0])-rotY, float(f[1])-speedY), int(extra_netid)-owner]
		SPELL_SOUND, // play spell sound [string1(spell->id), VEC3(pos)]
		CREATE_DRAIN, // drain blood effect [int(netid)-unit that sucks blood]
		CREATE_ELECTRO, // create electro effect [int(e_id)-electro netid), VEC3(pos), VEC3(f)-pos2]
		UPDATE_ELECTRO, // update electro effect [int(e_id)-electro netid, VEC3(pos)]
		ELECTRO_HIT, // electro hit effect [VEC3(pos)]
		RAISE_EFFECT, // raise spell effect [VEC3(pos)]
		REVEAL_MINIMAP, // revealing minimap [auto:vector<size:word, byte-x, byte-y>]
		CHEAT_NOAI, // player used cheat 'noai' or notification to players [bool(id)-state]
		END_OF_GAME, // end of game, time run out []
		REST, // player rest in inn [byte(id)-days]
		TRAIN, // player trains [byte(id)-type (0-attribute, 1-skill, 2-tournament), byte(ile)-stat type]
		UPDATE_FREE_DAYS, // update players free days [vector<size:byte, int-unit netid, int-days>]
		CHANGE_MP_VARS, // multiplayer vars changed [auto: bool-mp_use_interp, float-mp_interp]
		GAME_SAVED, // game saved notification []
		PAY_CREDIT, // player pays credit [int(id-count)]
		GIVE_GOLD, // player give gold to unit [int(id)-netid, int(ile)-count]
		DROP_GOLD, // player drops gold on group [int(id)-count]
		HERO_LEAVE, // ai left team due too many team members [int(netid)-unit]
		PAUSED, // game paused/resumed [bool(id)-is paused]
		HEAL_EFFECT, // heal spell effect [VEC3(pos)]
		SECRET_TEXT, // secret letter text update [auto:string1-text]
		PUT_GOLD, // player puts gold into container [int(ile)-count]
		UPDATE_MAP_POS, // update position on world map [auto:VEC2-pos]
		CHEAT_TRAVEL, // player used cheat for fast travel on map [byte(id)-location index]
		CHEAT_HURT, // player used cheat 'hurt' [int(netid) - unit]
		CHEAT_BREAK_ACTION, // player used cheat 'break_action' [int(netid)-unit]
		CHEAT_FALL, // player used cheat 'fall' [int(netid)-unit]
		REMOVE_USED_ITEM, // remove used item from unit hand [int(netid)-unit]
		GAME_STATS, // game stats showed at end of game [auto:int-kills]
		USEABLE_SOUND, // play useable object sound for unit [int(netid)-unit]
		YELL, // player yell to move ai []
		ACADEMY_TEXT, // show when trying to enter academy []
		BREAK_ACTION, // break unit action [int(netid)-unit]
	} type;
	union
	{
		Unit* unit;
		GroundItem* item;
		const Item* base_item;
		UnitData* base_unit;
		int e_id;
		Spell* spell;
	};
	union
	{
		struct
		{
			int id, ile;
			union
			{
				int i;
				string* str;
			};
		};
		float f[3];
		VEC3P vec3;
		const Item* item2;
	};
	VEC3 pos;
	union
	{
		float extra_f;
		int extra_netid;
	};
};

//-----------------------------------------------------------------------------
struct NetChangePlayer
{
	enum TYPE
	{
		PICKUP, // item picked up [int(id)-count, int(ile)-team count]
		LOOT, // response to looting [bool(id)-can loot, if can: ItemListTeam]
		GOLD_MSG, // message about gained gold [bool(id)-default msg, int(ile)-count]
		START_DIALOG, // start dialog with unit or is busy [int(netid)-unit]
		END_DIALOG, // end of dialog []
		SHOW_DIALOG_CHOICES, // show dialog choices [auto:byte-count, char-escape choice, vector<string1>-choices]
		START_TRADE, // start trade [int(id)-unit, auto:ItemList]
		START_SHARE, // start sharing items [auto:int-weight, int-weight max, int-gold, stats, ItemListTeam]
		START_GIVE, // start giving items [auto:int-weight, int-weight max, int-gold, stats, ItemListTeam]
		SET_FROZEN, // change player frozen state [byte(id)-state]
		REMOVE_QUEST_ITEM, // remove quest item from inventory [int(id)-quest refid]
		DEVMODE, // change devmode for player [bool(id)-allowed]
		USE_USEABLE, // someone else is using useable []
		IS_BETTER_ITEM, // response to is IS_BETTER_ITEM (bool(id)-is better)
		PVP, // question about pvp [byte(id)-player id]
		ADD_ITEMS, // add multiple same items to inventory [int(id)-team count, int(ile)-count, Item(item)]
		ADD_ITEMS_TRADER, // add items to trader which is trading with player [int(id)-unit, int(ile)-count, int(a)-team count, Item(item)]
		ADD_ITEMS_CHEST, // add items to chest which is open by player [int(id)-chest netid, int(ile)-count, int(a)-team count, Item(item)]
		REMOVE_ITEMS, // remove items from inventory [int(id)-i_index, int(ile)-count]
		REMOVE_ITEMS_TRADER, // remove items from traded inventory which is trading with player [int(id)-unit, int(ile)-count, int(a)-i_index]
		PREPARE_WARP, // preparing to warp []
		ENTER_ARENA, // entering arena []
		START_ARENA_COMBAT, // start of arena combat []
		EXIT_ARENA, // exit from arena []
		NO_PVP, // player refused to pvp [byte(id)-player id]
		CANT_LEAVE_LOCATION, // can't leave location message [byte(id)-reason (0-not in combat, 1-geather team)]
		LOOK_AT, // force player to look at unit [int(netid)-unit or null]
		END_FALLBACK, // end of fallback []
		REST, // response to rest in inn [byte(id)-days]
		TRAIN, // response to training [byte(id)-type (0-attribute, 1-skill, 2-tournament), byte(ile)-stat type]
		UNSTUCK, // warped player to not stuck position [VEC3(pos)]
		GOLD_RECEIVED, // message about receiving gold from another player [byte(id)-player id, int(ile)-count]
		GAIN_STAT, // player gained attribute/skill [bool(id)-is skill; byte(ile)-count; byte(a)-what]
		UPDATE_TRADER_GOLD, // update trader gold [int(id)-unit gold, int(ile)-count]
		UPDATE_TRADER_INVENTORY, // update trader inventory after getting item [int(netid)-unit, ItemListTeam]
		PLAYER_STATS, // update player statistics [byte(id)-flags, vector<int>-values]
		ADDED_ITEM_MSG, // message about gaining item []
		ADDED_ITEMS_MSG, // message about gaining multiple items [byte(ile)-count]
		STAT_CHANGED, // player stat changed [byte(id)-ChangedStatType, byte(a)-stat id, int(ile)-value]
		ADD_PERK, // add perk to player [byte(id)-perk, int(ile)-value]
	} type;
	PlayerController* pc;
	int id, ile;
	union
	{
		int a;
		Unit* unit;
	};
	const Item* item;
	VEC3 pos;
};

//-----------------------------------------------------------------------------
enum AttackId
{
	AID_Attack,
	AID_PowerAttack,
	AID_Shoot,
	AID_StartShoot,
	AID_Block,
	AID_Bash,
	AID_RunningAttack,
	AID_StopBlock
};

//-----------------------------------------------------------------------------
enum class ChangedStatType
{
	ATTRIBUTE,
	SKILL,
	BASE_ATTRIBUTE,
	BASE_SKILL
};

//-----------------------------------------------------------------------------
enum class JoinResult
{
	Ok = -1,
	FullServer = 0,
	InvalidVersion,
	TakenNick,
	InvalidItemsCrc,
	BrokenPacket,
	OtherError,
	InvalidNick,
	InvalidSpellsCrc,
	InvalidUnitsCrc,
	InvalidDialogsCrc,
	InvalidGameTypeCrc
};
