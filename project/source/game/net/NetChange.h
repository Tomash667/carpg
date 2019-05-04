#pragma once

//-----------------------------------------------------------------------------
enum USE_USABLE_STATE
{
	USE_USABLE_STOP,
	USE_USABLE_END,
	USE_USABLE_START,
	USE_USABLE_START_SPECIAL
};

//-----------------------------------------------------------------------------
class NetChangeWriter
{
public:
	NetChangeWriter(NetChange& c);
	void Write(const void* ptr, uint size);
	template<typename T>
	NetChangeWriter& operator << (const T& e)
	{
		Write(&e, sizeof(T));
		return *this;
	}

private:
	byte* data;
	byte& len;
};

//-----------------------------------------------------------------------------
struct NetChange
{
	enum TYPE
	{
		UNIT_POS, // unit position/rotation/animation [int(netid)-unit, auto from Unit: Vec3-pos, float-rot, float-animation speed, byte-animation]
		CHANGE_EQUIPMENT, // unit changed equipped item SERVER[int(netid)-unit, byte(id)-item slot, Item-item(automaticaly set)] / CLIENT[int(id)-i_index (if slot remove, else equip)]
		TAKE_WEAPON, // unit take/hide weapon SERVER[int(netid)-unit, bool(id)-hide, byte-type of weapon(auto)] / CLIENT[bool(id) hide, byte-type of weapon(auto)]
		ATTACK, // unit attack SERVER[int(netid)-unit, byte(id)-type/flags, float(f[1])-speed] / CLIENT[byte(id)-type/flags, float(f[1])-speed, [(float(f[2]) - bow yspeed]]
		CHANGE_FLAGS, // change of game flags [byte-flags (auto 0x01-bandit, 0x02-crazies attack, 0x04-anyone talking)
		FALL, // unit falls on ground [int(netid)-unit]
		DIE, // unit dies [int(netid)-unit]
		SPAWN_BLOOD, // spawn blood [byte(id)-type, Vec3(pos)]
		UPDATE_HP, // update unit hp [int(netid)-unit, auto: float-hp, float-hpmax]
		HURT_SOUND, // play unit hurt sound [int(netid)-unit]
		DROP_ITEM, // unit drops item SERVER[int(netid)-unit] / CLIENT[int(id)-i_index, int(count)-count]
		SPAWN_ITEM, // spawn item on ground [GroundItem(item)]
		PICKUP_ITEM, // unit picks up item SERVER[int(netid)-unit, bool(count)-up animation] / CLIENT[int(id)-item netid]
		REMOVE_ITEM, // remove ground item (picked by someone) [int(id)-item netid]
		CONSUME_ITEM, // unit consume item SERVER[int(netid)-unit, string1(Item id)-consumed item, bool(count)-force] / CLIENT[int(id)-item index]
		HIT_SOUND, // play hit sound [Vec3(pos), byte(id)-material, byte(count)-material2]
		STUNNED, // unit get stunned [int(netid)-unit]
		SHOOT_ARROW, // create shooted arrow [int(netid)-unit, Vec3(pos), float(f[0])-rotY, float(f[1])-speedY, float(f[2])-rotX, float(extra_f)-speed]
		LOOT_UNIT, // player wants to loot unit [int(netid)-unit]
		GET_ITEM, // player gets item from unit or container [int(id)-i_index, int(count)-count]
		GET_ALL_ITEMS, // player picks up all items from corpse/chest []
		PUT_ITEM, // player puts item into unit or container [int(id)-i_index, int(count)-count]
		STOP_TRADE, // player ends trade []
		UPDATE_CREDIT, // update team credit [auto: vector<size byte, int-unit netid, int-credit>]
		TAKE_ITEM_CREDIT, // player takes item for credit [int(id)-index]
		IDLE, // unit playes idle animation SERVER[int(netid)-unit, byte(id)-animation index] / CLIENT[byte(id)-animation index]
		HELLO, // play unit hello sound [int(netid)-unit]
		ALL_QUESTS_COMPLETED, // info about completing all unique quests []
		TALK, // unit talks or player start dialog SERVER[int(netid)-unit, byte(id)-animation, int(count)-skip id, string1(str)-text] / CLIENT[int(netid)-unit]
		TALK_POS, // show talk text at position [Vec3(pos), string1(str)-text]
		CHOICE, // player selected dialog choice [byte(id)-choice]
		SKIP_DIALOG, // skip dialog by player [int(id)-skip id]
		CHANGE_LOCATION_STATE, // change location state to known [byte(id)-location index]
		ADD_RUMOR, // add rumor to journal [string1(rumors[id])-text]
		TELL_NAME, // hero tells his name [int(netid)-unit, bool(id)-set_name, string1-name if set]
		HAIR_COLOR, // change unit hair color [int(netid)-unit, auto: Vec4-color]
		ENTER_BUILDING, // player wants to enter building [byte(id)-building index]
		EXIT_BUILDING, // player wants to exit building []
		WARP, // warp unit or notify server about warping SERVER[int(netid)-unit, auto: char-in building, Vec3-pos, float-rot] / CLIENT[]
		ADD_NOTE, // player added note to journal [string1-text (automaticaly set)]
		REGISTER_ITEM, // register new item [auto: string1(base_item.id)-item id, string1(item2.id)-id, string1(item2.name)-name, string1(item2.desc)-description, int(item2.refid)-item refid]
		ADD_QUEST, // added quest [int(id)-index, auto: string1-quest.name, string2-quest.msgs[0], string2-quest.msgs[1])
		UPDATE_QUEST, // update quest [int(id)-index, auto: byte-quest.state, vector1<string2>-msgs (byte(count)-msgs count)]
		RENAME_ITEM, // item rename [auto: int(base_item.refid), string1(base_item.id), string1(base_item.name)]
		REMOVE_PLAYER, // remove player from game [byte(id)-player id, byte(count)-PlayerInfo.LeftReason]
		CHANGE_LEADER, // player wants to change leader or notification [byte(id)-player id]
		RANDOM_NUMBER, // player get random number SERVER[byte(unit.player.id), byte(id)-number] / CLIENT[byte(id)-number]
		CHEAT_WARP, // player used cheat 'warp' [byte(id)-inside building index]
		CHEAT_SKIP_DAYS, // player used cheat 'skip_days' [int(id)-days]
		CHEAT_KILLALL, // player used cheat 'killall' [int(netid)-ignored unit, byte(id)-mode]
		CHEAT_NOCLIP, // player used cheat 'noclip' [bool(id)-state]
		CHEAT_GODMODE, // player used cheat 'godmode' [bool(id)-state]
		CHEAT_INVISIBLE, // player used cheat 'invisible' [bool(id)-state]
		CHEAT_SCARE, // player used cheat 'scare' []
		CHEAT_SUICIDE, // player used cheat 'suicide' []
		CHEAT_HEAL_UNIT, // player used cheat 'heal_unit' [int(netid)-unit]
		CHEAT_KILL, // player used cheat 'kill' [int(netid)-unit]
		CHEAT_HEAL, // player used cheat 'heal' []
		CHEAT_SPAWN_UNIT, // player used cheat 'spawn_unit' [string1(base_unit)-unit id, byte(count)-count, char(id)-level, char(i)-in_arena]
		CHEAT_ADD_ITEM, // player used cheat 'add_item' or 'add_team_item' [string1(base_item)-item id, byte(count)-count, bool(id)-is team]
		CHEAT_ADD_GOLD, // player used cheat 'add_gold' or 'add_team_gold' [int(count)-count, bool(id)-is team]
		CHEAT_SET_STAT, // player used cheat set_stat [byte(id)-stat id, bool(count)-is skill, char(i)-value]
		CHEAT_MOD_STAT, // player used cheat mod_stat [byte(id)-stat id, bool(count)-is skill, char(i)-value]
		CHEAT_REVEAL, // player used cheat 'reveal' []
		CHEAT_GOTO_MAP, // player used cheat 'goto_map' []
		USE_USABLE, // unit uses usable object SERVER[int(netid)-unit, int(id)-usable netid, byte(count)-state(USE_USABLE_STATE)] / CLIENT[int(id)-usable netid, byte(count)-state(USE_USABLE_STATE)]
		STAND_UP, // unit stands up SERVER[int(netid)-unit] / CLIENT[]
		GAME_OVER, // game over []
		RECRUIT_NPC, // recruit npc to team [int(netid)-unit, auto:bool-is free]
		KICK_NPC, // kick npc out of team [int(netid)-unit]
		REMOVE_UNIT, // remove unit from game [int(netid)-unit]
		SPAWN_UNIT, // spawn new unit [int(netid)-unit]
		IS_BETTER_ITEM, // client checks if item is better for npc SERVER[bool(id)-is better] / CLINT[int(id)-i_index]
		PVP, // response to pvp request [bool(id)-is accepted]
		CHEAT_CITIZEN, // player used cheat 'citizen' []
		CHEAT_REVEAL_MINIMAP, // player used cheat 'reveal_minimap'
		CHANGE_ARENA_STATE, // change unit arena state, reset cooldown when entering arena [int(netid)-unit, auto:char-state]
		ARENA_SOUND, // plays arena sound [byte(id)-type (0-start,1-win,2-lose)]
		SHOUT, // unit shout after seeing enemy [int(netid)-unit]
		LEAVE_LOCATION, // leader wants to leave location or leaving notification SERVER[] / CLIENT[char(id)-type]
		EXIT_TO_MAP, // exit to map []
		ENTER_LOCATION, // enter current location []
		TRAVEL, // leader wants to travel to location [byte(id)-location index]
		TRAVEL_POS, // leader wants to travel to pos [Vec2(xy)-pos]
		STOP_TRAVEL, // leader stopped travel [Vec2(xy)-pos]
		END_TRAVEL, // leader finished travel []
		WORLD_TIME, // change world time [auto: int-worldtime, byte-day, byte-month, int-year]
		USE_DOOR, // someone open/close door [int(id)-door netid, bool(count)-is closing]
		CREATE_EXPLOSION, // create explosion effect [string1(spell->id), Vec3(pos)]
		REMOVE_TRAP, // remove trap [int(id)-trap netid]
		TRIGGER_TRAP, // trigger trap [int(id)-trap netid]
		EVIL_SOUND, // play evil sound []
		ENCOUNTER, // start encounter on world map [string1(str)-encounter text]
		CLOSE_ENCOUNTER, // close encounter message box []
		CLOSE_PORTAL, // close portal in location []
		CLEAN_ALTAR, // clean altar in evil quest []
		ADD_LOCATION, // add new location [byte(id)-location index, auto: [byte(loc.type), if dungeon byte(loc.levels)], byte(loc.state), Int2(loc.pos), string1(loc.name)]
		REMOVE_CAMP, // remove camp [byte(id)-camp index]
		CHANGE_AI_MODE, // change unit ai mode [int(netid)-unit, byte-mode (0x1-dont attack, 0x02-assist, 0x04-not idle, 0x08-attack team)]
		CHANGE_UNIT_BASE, // change unit base type [int(netid)-unit, string1(unit.data.id)-base unit id]
		CHEAT_CHANGE_LEVEL, // player used cheat to change level (<>+shift+ctrl) [bool(id)-is down]
		CHEAT_WARP_TO_STAIRS, // player used cheat to warp to stairs (<>+shift) [bool(id)-is down]
		CAST_SPELL, // unit casts spell [int(netid)-unit, byte(id)-attack id]
		CREATE_SPELL_BALL, // create ball - spell effect [string1(spell->id), Vec3(pos), float(f[0])-rotY, float(f[1])-speedY), int(extra_netid)-owner]
		SPELL_SOUND, // play spell sound [string1(spell->id), Vec3(pos)]
		CREATE_DRAIN, // drain blood effect [int(netid)-unit that sucks blood]
		CREATE_ELECTRO, // create electro effect [int(e_id)-electro netid), Vec3(pos), Vec3(f)-pos2]
		UPDATE_ELECTRO, // update electro effect [int(e_id)-electro netid, Vec3(pos)]
		ELECTRO_HIT, // electro hit effect [Vec3(pos)]
		RAISE_EFFECT, // raise spell effect [Vec3(pos)]
		REVEAL_MINIMAP, // revealing minimap [auto:vector<size:word, byte-x, byte-y>]
		CHEAT_NOAI, // player used cheat 'noai' or notification to players [bool(id)-state]
		END_OF_GAME, // end of game, time run out []
		REST, // player rest in inn [byte(id)-days]
		TRAIN, // player trains [byte(id)-type (0-attribute, 1-skill, 2-tournament), byte(count)-stat type]
		UPDATE_FREE_DAYS, // update players free days [vector<size:byte, int-unit netid, int-days>]
		CHANGE_MP_VARS, // multiplayer vars changed [auto: bool-mp_use_interp, float-mp_interp]
		GAME_SAVED, // game saved notification []
		PAY_CREDIT, // player pays credit [int(id-count)]
		GIVE_GOLD, // player give gold to unit [int(id)-netid, int(count)-count]
		DROP_GOLD, // player drops gold on group [int(id)-count]
		HERO_LEAVE, // ai left team due too many team members [int(netid)-unit]
		PAUSED, // game paused/resumed [bool(id)-is paused]
		HEAL_EFFECT, // heal spell effect [Vec3(pos)]
		SECRET_TEXT, // secret letter text update [auto:string1-text]
		PUT_GOLD, // player puts gold into container [int(count)-count]
		UPDATE_MAP_POS, // update position on world map [auto:Vec2-pos]
		CHEAT_TRAVEL, // player used cheat for fast travel on map [byte(id)-location index]
		CHEAT_TRAVEL_POS, // player used cheat for fast travel to pos on map [Vec2(xy)-pos]
		CHEAT_HURT, // player used cheat 'hurt' [int(netid) - unit]
		CHEAT_BREAK_ACTION, // player used cheat 'break_action' [int(netid)-unit]
		CHEAT_FALL, // player used cheat 'fall' [int(netid)-unit]
		REMOVE_USED_ITEM, // remove used item from unit hand [int(netid)-unit]
		GAME_STATS, // game stats showed at end of game [auto:int-kills]
		USABLE_SOUND, // play usable object sound for unit [int(netid)-unit]
		YELL, // player yell to move ai []
		BREAK_ACTION, // break unit action [int(netid)-unit]
		STUN, // unit stun - not shield bash [int(netid)-unit, f[0]-length]
		CHEAT_STUN, // player used cheat 'stun' [int(netid)-unit, f[0]-length]
		PLAYER_ACTION, // player unit is using action, client[Vec3-pos/data] / server[int(netid)-unit]
		CHEAT_REFRESH_COOLDOWN, // player used cheat 'refresh_cooldown'
		END_FALLBACK, // client fallback ended []
		RUN_SCRIPT, // run script [string(str)-code, int(id)-target netid]
		SET_NEXT_ACTION, // player set next action [auto: byte-next_action, ...]
		CHANGE_ALWAYS_RUN, // player toggle always run - notify to save it [bool(id)]
		GENERIC_CMD, // player used generic cheat [byte(size), byte[size]-content (first byte is CMD)]
		USE_ITEM, // player used item SERVER[int(id)-unit] CLIENT[int(id)-item index]
		MARK_UNIT, // mark unit [int(netid)-unit, bool(id)-is marked]
		CHEAT_ARENA, // player used cheat 'arena' [string(str)-list of enemies]
		CLEAN_LEVEL, // clean level from blood and corpses [int(id)-building id (-1 outside, -2 all)]
		CHANGE_LOCATION_IMAGE, // change location image [byte(id)-index, auto:byte-image]
		CHANGE_LOCATION_NAME, // change location name [byte(id)-index, auto:string1-name]
		SET_SHORTCUT, // player set shortcut [byte(id)-index, auto:byte-type, byte/string1-value]
		USE_CHEST, // unit uses chest SERVER[int(id)-chest id, int(count)-unit id] / CLIENT[int(id)-chest id]

		MAX
	} type;
	union
	{
		Unit* unit;
		GroundItem* item;
		const Item* base_item;
		UnitData* base_unit;
		int e_id;
		Spell* spell;
		CMD cmd;
		Usable* usable;
	};
	union
	{
		struct
		{
			int id, count;
			union
			{
				int i;
				string* str;
			};
		};
		float f[3];
		POD::Vec3 vec3;
		const Item* item2;
	};
	Vec3 pos;
	union
	{
		float extra_f;
		int extra_netid;
	};

	template<typename T>
	NetChangeWriter operator << (const T& e)
	{
		NetChangeWriter w(*this);
		w.Write(&e, sizeof(T));
		return w;
	}
};
static_assert((int)NetChange::MAX <= 256, "too many NetChanges");
