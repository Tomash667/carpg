#include "Pch.h"
#include "CommandParser.h"

#include "AIController.h"
#include "Arena.h"
#include "BitStreamFunc.h"
#include "BuildingGroup.h"
#include "City.h"
#include "Console.h"
#include "ConsoleCommands.h"
#include "Game.h"
#include "GameGui.h"
#include "InsideLocation.h"
#include "Level.h"
#include "LevelGui.h"
#include "MpBox.h"
#include "NetChangePlayer.h"
#include "Pathfinding.h"
#include "PlayerInfo.h"
#include "QuestManager.h"
#include "ScriptManager.h"
#include "ServerPanel.h"
#include "Team.h"
#include "Utility.h"
#include "Version.h"
#include "World.h"
#include "WorldMapGui.h"

#include <CrashHandler.h>
#include <Engine.h>
#include <Render.h>
#include <SceneManager.h>
#include <Terrain.h>
#include <Tokenizer.h>

//-----------------------------------------------------------------------------
CommandParser* cmdp;

//=================================================================================================
void CommandParser::AddCommands()
{
	cmds.push_back(ConsoleCommand(&sceneMgr->useFog, "useFog", "draw fog (useFog 0/1)", F_ANYWHERE | F_CHEAT | F_WORLD_MAP));
	cmds.push_back(ConsoleCommand(&sceneMgr->useLighting, "useLighting", "use lighting (useLighting 0/1)", F_ANYWHERE | F_CHEAT | F_WORLD_MAP));
	cmds.push_back(ConsoleCommand(&sceneMgr->useNormalmap, "useNormalmap", "use normal mapping (useNormalmap 0/1)", F_ANYWHERE | F_WORLD_MAP));
	cmds.push_back(ConsoleCommand(&sceneMgr->useSpecularmap, "useSpecularmap", "use specular mapping (useSpecularmap 0/1)", F_ANYWHERE | F_WORLD_MAP));

	cmds.push_back(ConsoleCommand(&game->drawParticleSphere, "drawParticleSphere", "draw particle extents sphere (drawParticleSphere 0/1)", F_ANYWHERE | F_CHEAT | F_WORLD_MAP));
	cmds.push_back(ConsoleCommand(&game->drawUnitRadius, "drawUnitRadius", "draw units radius (drawUnitRadius 0/1)", F_ANYWHERE | F_CHEAT | F_WORLD_MAP));
	cmds.push_back(ConsoleCommand(&game->drawHitbox, "drawHitbox", "draw weapons hitbox (drawHitbox 0/1)", F_ANYWHERE | F_CHEAT | F_WORLD_MAP));
	cmds.push_back(ConsoleCommand(&game->drawPhy, "drawPhy", "draw physical colliders (drawPhy 0/1)", F_ANYWHERE | F_CHEAT | F_WORLD_MAP));
	cmds.push_back(ConsoleCommand(&game->drawCol, "drawCol", "draw colliders (drawCol 0/1)", F_ANYWHERE | F_CHEAT | F_WORLD_MAP));
	cmds.push_back(ConsoleCommand(&game->gameSpeed, "speed", "game speed (speed 0-10)", F_CHEAT | F_GAME | F_WORLD_MAP | F_MP_VAR, 0.01f, 10.f));
	cmds.push_back(ConsoleCommand(&game->nextSeed, "nextSeed", "random seed used in next map generation", F_ANYWHERE | F_CHEAT | F_WORLD_MAP));
	cmds.push_back(ConsoleCommand(&game->dontWander, "dontWander", "citizens don't wander around city (dontWander 0/1)", F_ANYWHERE | F_CHEAT | F_WORLD_MAP));
	cmds.push_back(ConsoleCommand(&net->mpInterp, "mpInterp", "interpolation interval (mpInterp 0.f-1.f)", F_MULTIPLAYER | F_WORLD_MAP | F_MP_VAR, 0.f, 1.f));
	cmds.push_back(ConsoleCommand(&net->mpUseInterp, "mpUseInterp", "set use of interpolation (mpUseInterp 0/1)", F_MULTIPLAYER | F_WORLD_MAP | F_MP_VAR));
	cmds.push_back(ConsoleCommand(&game->usePostfx, "usePostfx", "use post effects (usePostfx 0/1)", F_ANYWHERE | F_WORLD_MAP));
	cmds.push_back(ConsoleCommand(&game->uvMod, "uvMod", "terrain uv mod (uvMod 1-256)", F_ANYWHERE, 1, 256, VoidF(this, &Game::UvModChanged)));
	cmds.push_back(ConsoleCommand(&game->settings.grassRange, "grassRange", "grass draw range", F_ANYWHERE | F_WORLD_MAP, 0.f));
	cmds.push_back(ConsoleCommand(&game->devmode, "devmode", "developer mode (devmode 0/1)", F_GAME | F_SERVER | F_WORLD_MAP | F_MENU));

	cmds.push_back(ConsoleCommand(CMD_WHISPER, "whisper", "send private message to player (whisper nick msg)", F_LOBBY | F_MULTIPLAYER | F_WORLD_MAP | F_NO_ECHO));
	cmds.push_back(ConsoleCommand(CMD_WHISPER, "w", "send private message to player, short from whisper (w nick msg)", F_LOBBY | F_MULTIPLAYER | F_WORLD_MAP | F_NO_ECHO));
	cmds.push_back(ConsoleCommand(CMD_EXIT, "exit", "exit to menu", F_NOT_MENU | F_WORLD_MAP));
	cmds.push_back(ConsoleCommand(CMD_QUIT, "quit", "quit from game", F_ANYWHERE));
	cmds.push_back(ConsoleCommand(CMD_RANDOM, "random", "roll random number 1-100 or pick random character (random, random [name], use ? to get list)", F_ANYWHERE));
	cmds.push_back(ConsoleCommand(CMD_CMDS, "cmds", "show commands and write them to file commands.txt, with all show unavailable commands too (cmds [all])", F_ANYWHERE));
	cmds.push_back(ConsoleCommand(CMD_WARP, "warp", "move player into building (warp building/group [front])", F_CHEAT | F_GAME));
	cmds.push_back(ConsoleCommand(CMD_KILLALL, "killall", "kills all enemy units in current level, with 1 it kills allies too, ignore unit in front of player (killall [0/1])", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_SAVE, "save", "save game (save 1-11 [text] or filename)", F_GAME | F_WORLD_MAP | F_SERVER));
	cmds.push_back(ConsoleCommand(CMD_LOAD, "load", "load game (load 1-11 or filename)", F_GAME | F_WORLD_MAP | F_MENU | F_SERVER));
	cmds.push_back(ConsoleCommand(CMD_REVEAL_MINIMAP, "revealMinimap", "reveal dungeon minimap", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_SKIP_DAYS, "skipDays", "skip days [skipDays [count])", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_LIST, "list", "display list of types, don't enter type to list possible choices (list type [filter])", F_ANYWHERE));
	cmds.push_back(ConsoleCommand(CMD_HEAL_UNIT, "healUnit", "heal unit in front of player", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_SUICIDE, "suicide", "kill player", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_CITIZEN, "citizen", "citizens/crazies don't attack player or his team", F_GAME | F_CHEAT | F_WORLD_MAP));
	cmds.push_back(ConsoleCommand(CMD_SCARE, "scare", "enemies escape", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_INVISIBLE, "invisible", "ai can't see player (invisible 0/1)", F_GAME | F_CHEAT | F_NO_ECHO));
	cmds.push_back(ConsoleCommand(CMD_GODMODE, "godmode", "player can't be killed (godmode 0/1)", F_ANYWHERE | F_CHEAT | F_NO_ECHO));
	cmds.push_back(ConsoleCommand(CMD_NOCLIP, "noclip", "turn off player collisions (noclip 0/1)", F_GAME | F_CHEAT | F_NO_ECHO));
	cmds.push_back(ConsoleCommand(CMD_GOTO_MAP, "gotoMap", "transport player to world map", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_VERSION, "version", "displays game version", F_ANYWHERE));
	cmds.push_back(ConsoleCommand(CMD_REVEAL, "reveal", "reveal all locations on world map", F_GAME | F_CHEAT | F_WORLD_MAP));
	cmds.push_back(ConsoleCommand(CMD_MAP2CONSOLE, "map2console", "draw dungeon map in console", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_ADD_ITEM, "addItem", "add item to player inventory (addItem id [count])", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_ADD_TEAM_ITEM, "addTeamItem", "add team item to player inventory (addTeamItem id [count])", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_ADD_GOLD, "addGold", "give gold to player (addGold count)", F_GAME | F_CHEAT | F_WORLD_MAP));
	cmds.push_back(ConsoleCommand(CMD_ADD_TEAM_GOLD, "addTeamGold", "give gold to team (addTeamGold count)", F_GAME | F_CHEAT | F_WORLD_MAP));
	cmds.push_back(ConsoleCommand(CMD_SET_STAT, "setStat", "set player statistics, use ? to get list (setStat stat value)", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_MOD_STAT, "modStat", "modify player statistics, use ? to get list (modStat stat value)", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_HELP, "help", "display information about command (help [command])", F_ANYWHERE));
	cmds.push_back(ConsoleCommand(CMD_SPAWN_UNIT, "spawnUnit", "create unit in front of player (spawnUnit id [level count arena])", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_HEAL, "heal", "heal player", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_KILL, "kill", "kill unit in front of player", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_PLAYER_DEVMODE, "playerDevmode", "get/set player developer mode in multiplayer (playerDevmode nick/all [0/1])", F_MULTIPLAYER | F_WORLD_MAP | F_CHEAT | F_SERVER));
	cmds.push_back(ConsoleCommand(CMD_NOAI, "noai", "disable ai (noai 0/1)", F_CHEAT | F_GAME | F_WORLD_MAP | F_NO_ECHO));
	cmds.push_back(ConsoleCommand(CMD_PAUSE, "pause", "pause/unpause", F_GAME | F_SERVER));
	cmds.push_back(ConsoleCommand(CMD_MULTISAMPLING, "multisampling", "sets multisampling (multisampling type [quality])", F_ANYWHERE | F_WORLD_MAP | F_NO_ECHO));
	cmds.push_back(ConsoleCommand(CMD_RESOLUTION, "resolution", "show or change display resolution (resolution [w h])", F_ANYWHERE | F_WORLD_MAP));
	cmds.push_back(ConsoleCommand(CMD_QUICKSTART, "quickstart", "pick random character, get ready and start game", F_MENU | F_LOBBY));
	cmds.push_back(ConsoleCommand(CMD_CLEAR, "clear", "clear text", F_ANYWHERE | F_WORLD_MAP));
	cmds.push_back(ConsoleCommand(CMD_HURT, "hurt", "deal 100 damage to unit ('hurt 1' targets self)", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_BREAK_ACTION, "breakAction", "break unit current action ('break 1' targets self)", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_FALL, "fall", "unit fall on ground for some time ('fall 1' targets self)", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_RELOAD_SHADERS, "reloadShaders", "reload shaders", F_ANYWHERE | F_WORLD_MAP));
	cmds.push_back(ConsoleCommand(CMD_TILE_INFO, "tileInfo", "display info about map tile", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_CRASH, "crash", "crash game to death!", F_SINGLEPLAYER | F_LOBBY | F_MENU | F_WORLD_MAP | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_FORCE_QUEST, "forceQuest", "force next random quest to select (use list quest or none/reset)", F_SERVER | F_GAME | F_WORLD_MAP | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_STUN, "stun", "stun unit for time (stun [length=1] [1 = self])", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_REFRESH_COOLDOWN, "refreshCooldown", "refresh action cooldown/charges", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_DRAW_PATH, "drawPath", "draw debug pathfinding, look at target", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_VERIFY, "verify", "verify game state integrity", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_ADD_EFFECT, "addEffect", "add effect to selected unit (addEffect effect <valueType> power [source [perk/time]])", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_REMOVE_EFFECT, "removeEffect", "remove effect from selected unit (removeEffect effect/source [perk] [valueType])", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_LIST_EFFECTS, "listEffects", "display selected unit effects", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_ADD_PERK, "addPerk", "add perk to selected unit (addPerk perk)", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_REMOVE_PERK, "removePerk", "remove perk from selected unit (removePerk perk)", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_LIST_PERKS, "listPerks", "display selected unit perks", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_SELECT, "select", "select and display currently selected target (select [me/show/target] - use target or show by default)", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_LIST_STATS, "listStats", "display selected unit stats", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_ADD_LEARNING_POINTS, "addLearningPoints", "add learning point to selected unit [count - default 1]", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_CLEAN_LEVEL, "cleanLevel", "remove all corpses and blood from level (cleanLevel [buildingId])", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_ARENA, "arena", "spawns enemies on arena (example arena 3 rat vs 2 wolf)", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_REMOVE_UNIT, "removeUnit", "remove selected unit", F_GAME | F_CHEAT | F_SERVER));
	cmds.push_back(ConsoleCommand(CMD_ADD_EXP, "addExp", "add experience to team (addExp value)", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_NOCD, "nocd", "player abilities have no cooldown & use no mana/stamina (nocd 0/1)", F_ANYWHERE | F_CHEAT | F_NO_ECHO));
	cmds.push_back(ConsoleCommand(CMD_FIND, "find", "find nearest entity (find type id)", F_GAME | F_CHEAT));

	// verify all commands are added
	if(IsDebug())
	{
		for(uint i = 0; i < CMD_MAX; ++i)
		{
			bool ok = false;
			for(ConsoleCommand& cmd : cmds)
			{
				if(cmd.cmd == i)
				{
					ok = true;
					break;
				}
			}
			if(!ok)
				Error("Missing command %u.", i);
		}
	}
}

//=================================================================================================
void CommandParser::ParseCommand(const string& commandStr, PrintMsgFunc printMsgFunc, PARSE_SOURCE source)
{
	this->printMsgFunc = printMsgFunc;

	t.FromString(commandStr);

	try
	{
		if(!t.Next())
			return;

		if(t.IsSymbol('#'))
		{
			ParseScript();
			return;
		}

		const string& token = t.MustGetItem();

		for(vector<ConsoleCommand>::iterator it = cmds.begin(), end = cmds.end(); it != end; ++it)
		{
			if(token != it->name)
				continue;

			if(IsSet(it->flags, F_CHEAT) && !game->devmode)
			{
				Msg("You can't use command '%s' without devmode.", token.c_str());
				return;
			}

			if(!IsAllSet(it->flags, F_ANYWHERE))
			{
				if(gameGui->server->visible)
				{
					if(!IsSet(it->flags, F_LOBBY))
					{
						Msg("You can't use command '%s' in server lobby.", token.c_str());
						return;
					}
					else if(IsSet(it->flags, F_SERVER) && !Net::IsServer())
					{
						Msg("Only server can use command '%s'.", token.c_str());
						return;
					}
				}
				else if(game->gameState == GS_MAIN_MENU)
				{
					if(!IsSet(it->flags, F_MENU))
					{
						Msg("You can't use command '%s' in menu.", token.c_str());
						return;
					}
				}
				else if(Net::IsOnline())
				{
					if(!IsSet(it->flags, F_MULTIPLAYER))
					{
						if(IsSet(it->flags, F_SINGLEPLAYER))
							Msg("You can't use command '%s' in multiplayer.", token.c_str());
						else
							Msg("You can't use command '%s' in game.", token.c_str());
						return;
					}
					else if(IsSet(it->flags, F_SERVER) && !Net::IsServer())
					{
						Msg("Only server can use command '%s'.", token.c_str());
						return;
					}
				}
				else if(game->gameState == GS_WORLDMAP)
				{
					if(!IsSet(it->flags, F_WORLD_MAP))
					{
						Msg("You can't use command '%s' on world map.", token.c_str());
						return;
					}
				}
				else
				{
					if(!IsSet(it->flags, F_SINGLEPLAYER))
					{
						if(IsSet(it->flags, F_MULTIPLAYER))
							Msg("You can't use command '%s' in singleplayer.", token.c_str());
						else
							Msg("You can't use command '%s' in game.", token.c_str());
						return;
					}
				}
			}

			switch(it->type)
			{
			case ConsoleCommand::VAR_BOOL:
				if(t.Next())
				{
					if(IsSet(it->flags, F_MP_VAR) && Net::IsClient())
					{
						Msg("Only server can change this variable.");
						return;
					}

					bool value = t.MustGetBool();
					bool& var = it->Get<bool>();

					if(value != var)
					{
						var = value;
						if(IsSet(it->flags, F_MP_VAR) && Net::IsServer())
							Net::PushChange(NetChange::CHANGE_MP_VARS);
						if(it->changed)
							it->changed();
					}
				}
				Msg("%s = %d", it->name, *(bool*)it->var ? 1 : 0);
				break;
			case ConsoleCommand::VAR_FLOAT:
				{
					float& f = it->Get<float>();

					if(t.Next())
					{
						if(IsSet(it->flags, F_MP_VAR) && Net::IsClient())
						{
							Msg("Only server can change this variable.");
							return;
						}

						float f2 = Clamp(t.MustGetFloat(), it->_float.x, it->_float.y);
						if(!Equal(f, f2))
						{
							f = f2;
							if(IsSet(it->flags, F_MP_VAR) && Net::IsServer())
								Net::PushChange(NetChange::CHANGE_MP_VARS);
						}
					}
					Msg("%s = %g", it->name, f);
				}
				break;
			case ConsoleCommand::VAR_INT:
				{
					int& i = it->Get<int>();

					if(t.Next())
					{
						if(IsSet(it->flags, F_MP_VAR) && !Net::IsServer())
						{
							Msg("Only server can change this variable.");
							return;
						}

						int i2 = Clamp(t.MustGetInt(), it->_int.x, it->_int.y);
						if(i != i2)
						{
							i = i2;
							if(IsSet(it->flags, F_MP_VAR) && Net::IsServer())
								Net::PushChange(NetChange::CHANGE_MP_VARS);
							if(it->changed)
								it->changed();
						}
					}
					Msg("%s = %d", it->name, i);
				}
				break;
			case ConsoleCommand::VAR_UINT:
				{
					uint& u = it->Get<uint>();

					if(t.Next())
					{
						if(IsSet(it->flags, F_MP_VAR) && !Net::IsServer())
						{
							Msg("Only server can change this variable.");
							return;
						}

						uint u2 = Clamp(t.MustGetUint(), it->_uint.x, it->_uint.y);
						if(u != u2)
						{
							u = u2;
							if(IsSet(it->flags, F_MP_VAR) && Net::IsServer())
								Net::PushChange(NetChange::CHANGE_MP_VARS);
						}
					}
					Msg("%s = %u", it->name, u);
				}
				break;
			case ConsoleCommand::VAR_NONE:
				RunCommand(*it, source);
				break;
			}
			return;
		}

		Msg("Unknown command '%s'!", token.c_str());
	}
	catch(const Tokenizer::Exception& e)
	{
		Msg("Failed to parse command: %s", e.str->c_str());
	}
}

void CommandParser::ParseScript()
{
	Msg(t.GetInnerString().c_str());
	if(!game->devmode)
	{
		Msg("You can't use script command without devmode.");
		return;
	}
	if(game->gameState != GS_LEVEL)
	{
		Msg("Script commands can only be used inside level.");
		return;
	}

	cstring code = t.GetTextRest();
	Unit* targetUnit = game->pc->data.selectedUnit;
	if(Net::IsLocal())
	{
		string& output = scriptMgr->OpenOutput();
		ScriptContext& ctx = scriptMgr->GetContext();
		ctx.pc = game->pc;
		ctx.target = targetUnit;
		scriptMgr->RunScript(code);
		if(!output.empty())
			Msg(output.c_str());
		ctx.pc = nullptr;
		ctx.target = nullptr;
		scriptMgr->CloseOutput();
	}
	else
	{
		NetChange& c = Net::PushChange(NetChange::RUN_SCRIPT);
		c.str = StringPool.Get();
		*c.str = code;
		c.id = (targetUnit ? targetUnit->id : -1);
	}
}

void CommandParser::RunCommand(ConsoleCommand& cmd, PARSE_SOURCE source)
{
	if(!IsSet(cmd.flags, F_NO_ECHO))
		Msg(t.GetInnerString().c_str());

	switch(cmd.cmd)
	{
	case CMD_GOTO_MAP:
		if(Net::IsLocal())
			game->ExitToMap();
		else
			Net::PushChange(NetChange::CHEAT_GOTO_MAP);
		break;
	case CMD_VERSION:
		Msg("CaRpg version " VERSION_STR ", built %s.", utility::GetCompileTime().c_str());
		break;
	case CMD_QUIT:
		if(t.Next() && t.IsInt(1))
			exit(0);
		else
			game->Quit();
		break;
	case CMD_REVEAL:
		if(Net::IsLocal())
			world->Reveal();
		else
			PushGenericCmd(CMD_REVEAL);
		break;
	case CMD_MAP2CONSOLE:
		if(game->gameState == GS_LEVEL && !gameLevel->location->outside)
		{
			InsideLocationLevel& lvl = static_cast<InsideLocation*>(gameLevel->location)->GetLevelData();
			Tile::DebugDraw(lvl.map, Int2(lvl.w, lvl.h));
		}
		else
			Msg("You need to be inside dungeon!");
		break;
	case CMD_ADD_ITEM:
	case CMD_ADD_TEAM_ITEM:
		if(t.Next())
		{
			bool isTeam = (cmd.cmd == CMD_ADD_TEAM_ITEM);
			const string& itemName = t.MustGetItem();
			const Item* item = Item::TryGet(itemName);
			if(!item)
				Msg("Can't find item with id '%s'!", itemName.c_str());
			else
			{
				int count;
				if(t.Next())
				{
					count = t.MustGetInt();
					if(count < 1)
						count = 1;
				}
				else
					count = 1;

				if(Net::IsLocal())
					game->pc->unit->AddItem2(item, count, isTeam ? count : 0, false);
				else
				{
					NetChange& c = Net::PushChange(NetChange::CHEAT_ADD_ITEM);
					c.baseItem = item;
					c.count = count;
					c.id = isTeam ? 1 : 0;
				}
			}
		}
		else
			Msg("You need to enter item id!");
		break;
	case CMD_ADD_GOLD:
	case CMD_ADD_TEAM_GOLD:
		if(t.Next())
		{
			bool isTeam = (cmd.cmd == CMD_ADD_TEAM_GOLD);
			int count = t.MustGetInt();
			if(isTeam && count <= 0)
				Msg("Gold count must by positive!");
			if(Net::IsLocal())
			{
				if(isTeam)
					team->AddGold(count);
				else
					game->pc->unit->gold = max(game->pc->unit->gold + count, 0);
			}
			else
			{
				PushGenericCmd(CMD_ADD_GOLD)
					<< isTeam
					<< count;
			}
		}
		else
			Msg("You need to enter gold amount!");
		break;
	case CMD_SET_STAT:
	case CMD_MOD_STAT:
		if(!t.Next())
			Msg("Enter name of attribute/skill and value. Use ? to get list of attributes/skills.");
		else if(t.IsSymbol('?'))
		{
			LocalVector<AttributeId> attribs;
			for(int i = 0; i < (int)AttributeId::MAX; ++i)
				attribs.push_back((AttributeId)i);
			std::sort(attribs.begin(), attribs.end(),
				[](AttributeId a1, AttributeId a2) -> bool
			{
				return strcmp(Attribute::attributes[(int)a1].id, Attribute::attributes[(int)a2].id) < 0;
			});
			LocalVector<SkillId> skills;
			for(int i = 0; i < (int)SkillId::MAX; ++i)
				skills.push_back((SkillId)i);
			std::sort(skills.begin(), skills.end(),
				[](SkillId s1, SkillId s2) -> bool
			{
				return strcmp(Skill::skills[(int)s1].id, Skill::skills[(int)s2].id) < 0;
			});
			LocalString str = "List of attributes: ";
			Join(attribs.Get(), str.get_ref(), ", ",
				[](AttributeId a)
			{
				return Attribute::attributes[(int)a].id;
			});
			str += ".";
			Msg(str.c_str());
			str = "List of skills: ";
			Join(skills.Get(), str.get_ref(), ", ",
				[](SkillId s)
			{
				return Skill::skills[(int)s].id;
			});
			str += ".";
			Msg(str.c_str());
		}
		else
		{
			int value;
			bool skill;
			const string& s = t.MustGetItem();
			Attribute* ai = Attribute::Find(s);
			if(ai)
			{
				value = (int)ai->attribId;
				skill = false;
			}
			else
			{
				const Skill* si = Skill::Find(s);
				if(si)
				{
					value = (int)si->skillId;
					skill = true;
				}
				else
				{
					Msg("Invalid attribute/skill '%s'.", s.c_str());
					break;
				}
			}

			if(!t.Next())
				Msg("You need to enter number!");
			else
			{
				int num = t.MustGetInt();

				if(Net::IsLocal())
				{
					if(skill)
					{
						if(cmd.cmd == CMD_MOD_STAT)
							num += game->pc->unit->stats->skill[value];
						int v = Clamp(num, Skill::MIN, Skill::MAX);
						game->pc->unit->Set((SkillId)value, v);
					}
					else
					{
						if(cmd.cmd == CMD_MOD_STAT)
							num += game->pc->unit->stats->attrib[value];
						int v = Clamp(num, Attribute::MIN, Attribute::MAX);
						game->pc->unit->Set((AttributeId)value, v);
					}
				}
				else
				{
					NetChange& c = Net::PushChange(cmd.cmd == CMD_SET_STAT ? NetChange::CHEAT_SET_STAT : NetChange::CHEAT_MOD_STAT);
					c.id = value;
					c.count = (skill ? 1 : 0);
					c.i = num;
				}
			}
		}
		break;
	case CMD_CMDS:
		{
			bool all = (t.Next() && t.GetItem() == "all");
			vector<const ConsoleCommand*> cmds2;

			if(all)
			{
				for(const ConsoleCommand& cmd : cmds)
					cmds2.push_back(&cmd);
			}
			else
			{
				for(const ConsoleCommand& cmd : cmds)
				{
					bool ok = true;
					if(IsSet(cmd.flags, F_CHEAT) && !game->devmode)
						ok = false;

					if(!IsAllSet(cmd.flags, F_ANYWHERE))
					{
						if(gameGui->server->visible)
						{
							if(!IsSet(cmd.flags, F_LOBBY))
								ok = false;
							else if(IsSet(cmd.flags, F_SERVER) && !Net::IsServer())
								ok = false;
						}
						else if(game->gameState == GS_MAIN_MENU)
						{
							if(!IsSet(cmd.flags, F_MENU))
								ok = false;
						}
						else if(Net::IsOnline())
						{
							if(!IsSet(cmd.flags, F_MULTIPLAYER))
								ok = false;
							else if(IsSet(cmd.flags, F_SERVER) && !Net::IsServer())
								ok = false;
						}
						else if(game->gameState == GS_WORLDMAP)
						{
							if(!IsSet(cmd.flags, F_WORLD_MAP))
								ok = false;
						}
						else
						{
							if(!IsSet(cmd.flags, F_SINGLEPLAYER))
								ok = false;
						}
					}

					if(ok)
						cmds2.push_back(&cmd);
				}
			}

			std::sort(cmds2.begin(), cmds2.end(), [](const ConsoleCommand* cmd1, const ConsoleCommand* cmd2)
			{
				return strcmp(cmd1->name, cmd2->name) < 0;
			});

			string s = "Available commands:\n";
			Msg("Available commands:");

			for(const ConsoleCommand* cmd : cmds2)
			{
				cstring str = Format("%s - %s.", cmd->name, cmd->desc);
				Msg(str);
				s += str;
				s += "\n";
			}

			TextWriter f("commands.txt");
			f << s;
		}
		break;
	case CMD_HELP:
		if(t.Next())
		{
			const string& cmd = t.MustGetItem();
			for(vector<ConsoleCommand>::iterator it2 = cmds.begin(), end2 = cmds.end(); it2 != end2; ++it2)
			{
				if(cmd == it2->name)
				{
					Msg("%s - %s.", cmd.c_str(), it2->desc);
					return;
				}
			}

			Msg("Unknown command '%s'!", cmd.c_str());
		}
		else
			Msg("Enter 'cmds' or 'cmds all' to show list of commands. To get information about single command enter \"help CMD\". To get ID of item/unit use command \"list\".");
		break;
	case CMD_SPAWN_UNIT:
		if(t.Next())
		{
			auto& id = t.MustGetItem();
			UnitData* data = UnitData::TryGet(id);
			if(!data || IsSet(data->flags, F_SECRET))
				Msg("Missing base unit '%s'!", id.c_str());
			else if(data->group == G_PLAYER)
				Msg("Can't spawn player unit.");
			else
			{
				int level = -1, count = 1, inArena = -1;
				if(t.Next())
				{
					level = t.MustGetInt();
					if(t.Next())
					{
						count = t.MustGetInt();
						if(t.Next())
							inArena = Clamp(t.MustGetInt(), -1, 1);
					}
				}

				if(Net::IsLocal())
				{
					LocationPart& locPart = *game->pc->unit->locPart;
					for(int i = 0; i < count; ++i)
					{
						Unit* u = gameLevel->SpawnUnitNearLocation(locPart, game->pc->unit->GetFrontPos(), *data, &game->pc->unit->pos, level);
						if(!u)
						{
							Msg("No free space for unit '%s'!", data->id.c_str());
							break;
						}
						else if(inArena != -1)
						{
							u->inArena = inArena;
							game->arena->units.push_back(u);
						}
					}
				}
				else
				{
					NetChange& c = Net::PushChange(NetChange::CHEAT_SPAWN_UNIT);
					c.baseUnit = data;
					c.count = count;
					c.id = level;
					c.i = inArena;
				}
			}
		}
		else
			Msg("You need to enter unit id!");
		break;
	case CMD_HEAL:
		if(Net::IsLocal())
			HealUnit(*game->pc->unit);
		else
			PushGenericCmd(CMD_HEAL);
		break;
	case CMD_KILL:
		if(Unit* target = game->pc->data.GetTargetUnit(); target && target->IsAlive())
		{
			if(Net::IsLocal())
				target->GiveDmg(target->hpmax);
			else
				PushGenericCmd(CMD_KILL) << target->id;
		}
		else
			Msg("No unit in front of player.");
		break;
	case CMD_LIST:
		CmdList();
		break;
	case CMD_HEAL_UNIT:
		if(Unit* target = game->pc->data.GetTargetUnit())
		{
			if(Net::IsLocal())
				HealUnit(*target);
			else
				PushGenericCmd(CMD_HEAL_UNIT) << target->id;
		}
		else
			Msg("No unit in front of player.");
		break;
	case CMD_SUICIDE:
		if(Net::IsLocal())
			game->pc->unit->GiveDmg(game->pc->unit->hpmax);
		else
			PushGenericCmd(CMD_SUICIDE);
		break;
	case CMD_CITIZEN:
		if(team->isBandit || team->craziesAttack)
		{
			if(Net::IsLocal())
			{
				team->isBandit = false;
				team->craziesAttack = false;
				if(Net::IsOnline())
					Net::PushChange(NetChange::CHANGE_FLAGS);
			}
			else
				PushGenericCmd(CMD_CITIZEN);
		}
		break;
	case CMD_SCARE:
		if(Net::IsLocal())
			Scare(game->pc);
		else
			PushGenericCmd(CMD_SCARE);
		break;
	case CMD_INVISIBLE:
		if(t.Next())
		{
			game->pc->invisible = t.MustGetBool();
			if(Net::IsClient())
				PushGenericCmd(CMD_INVISIBLE) << game->pc->invisible;
		}
		Msg("invisible = %d", game->pc->invisible ? 1 : 0);
		break;
	case CMD_GODMODE:
		if(t.Next())
		{
			game->pc->godmode = t.MustGetBool();
			if(Net::IsClient())
				PushGenericCmd(CMD_GODMODE) << game->pc->godmode;
		}
		Msg("godmode = %d", game->pc->godmode ? 1 : 0);
		break;
	case CMD_NOCLIP:
		if(t.Next())
		{
			game->pc->noclip = t.MustGetBool();
			if(Net::IsClient())
				PushGenericCmd(CMD_NOCLIP) << game->pc->noclip;
		}
		Msg("noclip = %d", game->pc->noclip ? 1 : 0);
		break;
	case CMD_KILLALL:
		{
			bool friendly = false;
			if(t.Next())
				friendly = t.MustGetBool();
			Unit* ignore = nullptr;
			if(game->pc->data.beforePlayer == BP_UNIT)
				ignore = game->pc->data.beforePlayerPtr.unit;
			if(Net::IsLocal())
				gameLevel->KillAll(friendly, *game->pc->unit, ignore);
			else
			{
				PushGenericCmd(CMD_KILLALL)
					<< friendly
					<< (ignore ? ignore->id : -1);
			}
		}
		break;
	case CMD_SAVE:
		if(game->CanSaveGame() == ActionResult::Yes)
		{
			int slot = 1;
			LocalString text;
			if(t.Next())
			{
				if(t.IsString())
				{
					slot = -1;
					text = t.GetString();
				}
				else if(t.IsInt())
				{
					slot = t.GetInt();
					if(slot < 1 || slot > 11)
					{
						Msg("Invalid slot %d.", slot);
						break;
					}
					if(t.Next())
						text = t.MustGetString();
				}
				else
					t.StartUnexpected().Add(tokenizer::T_INT).Add(tokenizer::T_STRING).Throw();
			}
			if(slot != -1)
				game->SaveGameSlot(slot, text->c_str());
			else
				game->SaveGameFilename(text.get_ref());
		}
		else
			Msg(Net::IsClient() ? "Only server can save game." : "You can't save game in this moment.");
		break;
	case CMD_LOAD:
		if(game->CanLoadGame() == ActionResult::Yes)
		{
			LocalString name;
			int slot = 1;
			if(t.Next())
			{
				if(t.IsString())
				{
					name = t.GetString();
					slot = -1;
				}
				else if(t.IsInt())
				{
					slot = t.GetInt();
					if(slot < 1 || slot > 11)
					{
						Msg("Invalid slot %d.", slot);
						break;
					}
				}
				else
					t.StartUnexpected().Add(tokenizer::T_INT).Add(tokenizer::T_STRING).Throw();
			}

			try
			{
				if(slot != -1)
					game->LoadGameSlot(slot);
				else
					game->LoadGameFilename(name.get_ref());
				gui->CloseDialog(gameGui->console);
			}
			catch(const SaveException& ex)
			{
				cstring error = Format("Failed to load game: %s", ex.msg);
				Error(error);
				Msg(error);
				if(!gui->HaveDialog(gameGui->console))
					gui->ShowDialog(gameGui->console);
			}
		}
		else
			Msg(Net::IsClient() ? "Only server can load game." : "You can't load game in this moment.");
		break;
	case CMD_REVEAL_MINIMAP:
		if(Net::IsLocal())
			gameLevel->RevealMinimap();
		else
			Net::PushChange(NetChange::CHEAT_REVEAL_MINIMAP);
		break;
	case CMD_SKIP_DAYS:
		{
			int count = 1;
			if(t.Next())
				count = t.MustGetInt();
			if(count > 0)
			{
				if(Net::IsLocal())
					world->Update(count, UM_SKIP);
				else
					PushGenericCmd(CMD_SKIP_DAYS) << count;
			}
		}
		break;
	case CMD_WARP:
		if(t.Next())
		{
			const string& type = t.MustGetItem();
			CityBuilding* cityBuilding = nullptr;
			int index;
			if(BuildingGroup* group = BuildingGroup::TryGet(type))
			{
				if(gameLevel->cityCtx)
					cityBuilding = gameLevel->cityCtx->FindBuilding(group, &index);
				if(!cityBuilding)
				{
					Msg("Missing building group '%s'.", type.c_str());
					break;
				}
			}
			else if(Building* building = Building::TryGet(type))
			{
				if(gameLevel->cityCtx)
					cityBuilding = gameLevel->cityCtx->FindBuilding(building, &index);
				if(!cityBuilding)
				{
					Msg("Missing building '%s'.", type.c_str());
					break;
				}
			}
			else
			{
				Msg("Invalid building or group '%s'.", type.c_str());
				break;
			}

			InsideBuilding* inside;
			if((t.Next() && t.IsItem("front")) || !cityBuilding->HaveInside())
				inside = nullptr;
			else
				inside = gameLevel->cityCtx->FindInsideBuilding(cityBuilding->building, &index);

			if(Net::IsLocal())
			{
				Unit& unit = *game->pc->unit;
				if(inside)
				{
					if(unit.locPart->partId != index)
					{
						// warp to building
						game->fallbackType = FALLBACK::ENTER;
						game->fallbackTimer = -1.f;
						game->fallbackValue = index;
						game->fallbackValue2 = -1;
						unit.frozen = (unit.usable ? FROZEN::YES_NO_ANIM : FROZEN::YES);
					}
					else
					{
						// already in that building, warp to entrance
						unit.rot = PI;
						gameLevel->WarpUnit(unit, inside->insideSpawn);
					}
				}
				else if(unit.locPart->partType != LocationPart::Type::Outside)
				{
					// warp from building to front of building
					game->fallbackType = FALLBACK::ENTER;
					game->fallbackTimer = -1.f;
					game->fallbackValue = -1;
					game->fallbackValue2 = index;
					unit.frozen = (unit.usable ? FROZEN::YES_NO_ANIM : FROZEN::YES);
				}
				else
				{
					// warp from outside to front of building
					gameLevel->WarpUnit(*game->pc->unit, cityBuilding->walkPt);
					unit.RotateTo(PtToPos(cityBuilding->pt));
				}
			}
			else
			{
				PushGenericCmd(CMD_WARP)
					<< (byte)index
					<< (inside != nullptr);
			}
		}
		else
			Msg("You need to enter where.");
		break;
	case CMD_WHISPER:
		if(t.Next())
		{
			const string& playerNick = t.MustGetText();
			PlayerInfo* info = net->FindPlayer(playerNick);
			if(!info)
				Msg("No player with nick '%s'.", playerNick.c_str());
			else if(t.NextLine())
			{
				const string& text = t.MustGetItem();
				if(info->id == team->myId)
					Msg("Whispers in your head: %s", text.c_str());
				else
				{
					BitStreamWriter f;
					f << ID_WHISPER;
					f.WriteCasted<byte>(Net::IsServer() ? team->myId : info->id);
					f << text;
					if(Net::IsServer())
						net->SendServer(f, MEDIUM_PRIORITY, RELIABLE, info->adr);
					else
						net->SendClient(f, MEDIUM_PRIORITY, RELIABLE);
					cstring s = Format("@%s: %s", info->name.c_str(), text.c_str());
					gameGui->AddMsg(s);
					Info(s);
				}
			}
			else
				Msg("You need to enter message.");
		}
		else
			Msg("You need to enter player nick.");
		break;
	case CMD_EXIT:
		game->ExitToMenu();
		break;
	case CMD_RANDOM:
		if(gameGui->server->visible)
		{
			if(t.Next())
			{
				if(t.IsSymbol('?'))
				{
					LocalVector<Class*> classes;
					for(Class* clas : Class::classes)
					{
						if(clas->IsPickable())
							classes.push_back(clas);
					}
					std::sort(classes.begin(), classes.end(),
						[](Class* c1, Class* c2) -> bool
					{
						return strcmp(c1->id.c_str(), c2->id.c_str()) < 0;
					});
					LocalString str = "List of classes: ";
					Join(classes.Get(), str.get_ref(), ", ",
						[](Class* c)
					{
						return c->id;
					});
					str += ".";
					Msg(str.c_str());
				}
				else
				{
					const string& classId = t.MustGetItem();
					Class* clas = Class::TryGet(classId);
					if(clas)
					{
						if(clas->IsPickable())
						{
							gameGui->server->PickClass(clas, false);
							Msg("You picked random character.");
						}
						else
							Msg("Class '%s' is not pickable by players.", classId.c_str());
					}
					else
						Msg("Invalid class id '%s'. Use 'random ?' for list of classes.", classId.c_str());
				}
			}
			else
			{
				gameGui->server->PickClass(nullptr, false);
				Msg("You picked random character.");
			}
		}
		else if(Net::IsOnline())
		{
			int n = Random(1, 100);
			NetChange& c = Net::PushChange(NetChange::RANDOM_NUMBER);
			c.id = n;
			c.unit = game->pc->unit;
			gameGui->AddMsg(Format("You rolled %d.", n));
		}
		else
			Msg("You rolled %d.", Random(1, 100));
		break;
	case CMD_PLAYER_DEVMODE:
		if(t.Next())
		{
			string nick = t.MustGetText();
			if(t.Next())
			{
				bool b = t.MustGetBool();
				if(nick == "all")
				{
					for(PlayerInfo& info : net->players)
					{
						if(info.left == PlayerInfo::LEFT_NO && info.devmode != b && info.id != 0)
						{
							info.devmode = b;
							NetChangePlayer& c = info.pc->playerInfo->PushChange(NetChangePlayer::DEVMODE);
							c.id = (b ? 1 : 0);
						}
					}
				}
				else
				{
					PlayerInfo* info = net->FindPlayer(nick);
					if(!info)
						Msg("No player with nick '%s'.", nick.c_str());
					else if(info->devmode != b)
					{
						info->devmode = b;
						NetChangePlayer& c = info->pc->playerInfo->PushChange(NetChangePlayer::DEVMODE);
						c.id = (b ? 1 : 0);
					}
				}
			}
			else
			{
				if(nick == "all")
				{
					LocalString s = "Players devmode: ";
					bool any = false;
					for(PlayerInfo& info : net->players)
					{
						if(info.left == PlayerInfo::LEFT_NO && info.id != 0)
						{
							s += Format("%s(%d), ", info.name.c_str(), info.devmode ? 1 : 0);
							any = true;
						}
					}
					if(any)
					{
						s.pop(2);
						s += ".";
						Msg(s->c_str());
					}
					else
						Msg("No players.");
				}
				else
				{
					PlayerInfo* info = net->FindPlayer(nick);
					if(!info)
						Msg("No player with nick '%s'.", nick.c_str());
					else
						Msg("Player devmode: %s(%d).", nick.c_str(), info->devmode ? 1 : 0);
				}
			}
		}
		else
			Msg("You need to enter player nick/all.");
		break;
	case CMD_NOAI:
		if(t.Next())
		{
			game->noai = t.MustGetBool();
			if(Net::IsOnline())
			{
				NetChange& c = Net::PushChange(NetChange::CHEAT_NOAI);
				c.id = (game->noai ? 1 : 0);
			}
		}
		Msg("noai = %d", game->noai ? 1 : 0);
		break;
	case CMD_PAUSE:
		game->PauseGame();
		break;
	case CMD_MULTISAMPLING:
		if(t.Next())
		{
			int type = t.MustGetInt();
			int level = -1;
			if(t.Next())
				level = t.MustGetInt();
			int result = render->SetMultisampling(type, level);
			if(result == 2)
			{
				int ms, msq;
				render->GetMultisampling(ms, msq);
				Msg("Changed multisampling to %d, %d.", ms, msq);
			}
			else
				Msg(result == 1 ? "This mode is already set." : "This mode is unavailable.");
		}
		else
		{
			int ms, msq;
			render->GetMultisampling(ms, msq);
			Msg("multisampling = %d, %d", ms, msq);
		}
		break;
	case CMD_RESOLUTION:
		if(t.Next())
		{
			int w = t.MustGetInt(), h = -1;
			bool pickH = true, valid = false;
			if(t.Next())
			{
				h = t.MustGetInt();
				pickH = false;
			}
			const vector<Resolution>& resolutions = render->GetResolutions();
			for(const Resolution& res : resolutions)
			{
				if(w == res.size.x)
				{
					if(pickH)
					{
						if((int)res.size.y >= h)
						{
							h = res.size.y;
							valid = true;
						}
					}
					else if(h == res.size.y)
					{
						valid = true;
						break;
					}
				}
			}
			if(valid)
				engine->SetWindowSize(Int2(w, h));
			else
				Msg("Can't change resolution to %dx%d.", w, h);
		}
		else
		{
			LocalString s = Format("Current resolution %dx%d. Available: ",
				engine->GetWindowSize().x, engine->GetWindowSize().y);
			const vector<Resolution>& resolutions = render->GetResolutions();
			for(const Resolution& res : resolutions)
				s += Format("%dx%d, ", res.size.x, res.size.y);
			s.pop(2u);
			s += ".";
			Msg(s);
		}
		break;
	case CMD_QUICKSTART:
		if(gameGui->server->visible)
		{
			if(!gameGui->server->Quickstart())
				Msg("Not everyone is ready.");
		}
		else
			game->StartQuickGame();
		break;
	case CMD_CLEAR:
		switch(source)
		{
		case PS_CONSOLE:
			gameGui->console->Reset();
			break;
		case PS_CHAT:
			gameGui->mpBox->itb.Reset();
			break;
		case PS_LOBBY:
			gameGui->server->itb.Reset();
			break;
		case PS_UNKNOWN:
		default:
			Msg("Unknown parse source.");
			break;
		}
		break;
	case CMD_HURT:
	case CMD_BREAK_ACTION:
	case CMD_FALL:
		{
			Unit* u;
			if(t.Next() && t.IsInt(1))
				u = game->pc->unit;
			else if(Unit* target = game->pc->data.GetTargetUnit())
				u = target;
			else
			{
				Msg("No unit in front of player.");
				break;
			}
			if(Net::IsLocal())
			{
				if(cmd.cmd == CMD_HURT)
					u->GiveDmg(100.f);
				else if(cmd.cmd == CMD_BREAK_ACTION)
					u->BreakAction(Unit::BREAK_ACTION_MODE::NORMAL, true);
				else
					u->Fall();
			}
			else
				PushGenericCmd(cmd.cmd) << u->id;
		}
		break;
	case CMD_RELOAD_SHADERS:
		render->ReloadShaders();
		if(engine->IsShutdown())
			return;
		break;
	case CMD_TILE_INFO:
		if(gameLevel->location->outside && game->pc->unit->locPart->partType == LocationPart::Type::Outside && gameLevel->terrain->IsInside(game->pc->unit->pos))
		{
			OutsideLocation* outside = static_cast<OutsideLocation*>(gameLevel->location);
			const TerrainTile& t = outside->tiles[PosToPt(game->pc->unit->pos)(outside->size)];
			Msg(t.GetInfo());
		}
		else
			Msg("You must stand on terrain tile.");
		break;
	case CMD_CRASH:
		CrashHandler::DoCrash();
		break;
	case CMD_FORCE_QUEST:
		{
			if(t.Next())
			{
				const string& id = t.MustGetItem();
				if(!questMgr->SetForcedQuest(id))
					Msg("Invalid quest id '%s'.", id.c_str());
			}
			int force = questMgr->GetForcedQuest();
			cstring name;
			if(force == Q_FORCE_DISABLED)
				name = "disabled";
			else if(force == Q_FORCE_NONE)
				name = "none";
			else
				name = questMgr->GetQuestInfos()[force].name;
			Msg("Forced quest: %s", name);
		}
		break;
	case CMD_STUN:
		{
			float length = 1.f;
			if(t.Next() && t.IsFloat())
			{
				length = t.GetFloat();
				if(length <= 0.f)
					break;
			}

			Unit* u;
			if(t.Next() && t.IsInt(1))
				u = game->pc->unit;
			else if(Unit* target = game->pc->data.GetTargetUnit())
				u = target;
			else
			{
				Msg("No unit in front of player.");
				break;
			}

			if(Net::IsLocal())
			{
				Effect e;
				e.effect = EffectId::Stun;
				e.source = EffectSource::Temporary;
				e.sourceId = -1;
				e.value = -1;
				e.power = 0;
				e.time = length;
				u->AddEffect(e);
			}
			else
			{
				PushGenericCmd(CMD_STUN)
					<< u->id
					<< length;
			}
		}
		break;
	case CMD_REFRESH_COOLDOWN:
		game->pc->RefreshCooldown();
		if(Net::IsClient())
			PushGenericCmd(CMD_REFRESH_COOLDOWN);
		break;
	case CMD_DRAW_PATH:
		if(game->pc->data.beforePlayer == BP_UNIT)
		{
			pathfinding->SetTarget(game->pc->data.beforePlayerPtr.unit);
			Msg("Set draw path target.");
		}
		else
		{
			pathfinding->SetTarget(nullptr);
			Msg("Removed draw path target.");
		}
		break;
	case CMD_VERIFY:
		world->VerifyObjects();
		break;
	case CMD_ADD_EFFECT:
		if(!t.Next())
		{
			Msg("Use 'list effect' to get list of existing effects. Some examples:");
			Msg("addEffect regeneration 5 - add permanent regeneration 5 hp/sec");
			Msg("addEffect meleeAttack 30 perk strongBack - add 30 melee attack assigned to perk");
			Msg("addEffect magicResistance 0.5 temporary 30 - add 50% magic resistance for 30 seconds");
			Msg("addEffect attribute str 5 item weapon - add +5 strength assigned to weapon");
		}
		else
		{
			Effect e;

			const string& effectId = t.MustGetItem();
			e.effect = EffectInfo::TryGet(effectId);
			if(e.effect == EffectId::None)
			{
				Msg("Invalid effect '%s'.", effectId.c_str());
				break;
			}
			t.Next();

			EffectInfo& info = EffectInfo::effects[(int)e.effect];
			if(info.valueType == EffectInfo::None)
				e.value = -1;
			else
			{
				const string& value = t.MustGetItem();
				if(info.valueType == EffectInfo::Attribute)
				{
					Attribute* attrib = Attribute::Find(value);
					if(!attrib)
					{
						Msg("Invalid attribute '%s' for effect '%s'.", value.c_str(), info.id);
						break;
					}
					e.value = (int)attrib->attribId;
				}
				else
				{
					const Skill* skill = Skill::Find(value);
					if(!skill)
					{
						Msg("Invalid skill '%s' for effect '%s'.", value.c_str(), info.id);
						break;
					}
					e.value = (int)skill->skillId;
				}
				t.Next();
			}

			e.power = t.MustGetFloat();
			if(t.Next())
			{
				const string& sourceName = t.MustGetItem();
				if(sourceName == "permanent")
				{
					e.source = EffectSource::Permanent;
					e.sourceId = -1;
					e.time = 0;
				}
				else if(sourceName == "perk")
				{
					e.source = EffectSource::Perk;
					t.Next();
					const string& perkId = t.MustGetItem();
					Perk* perk = Perk::Get(perkId);
					if(!perk)
					{
						Msg("Invalid perk source '%s'.", perkId.c_str());
						break;
					}
					e.sourceId = perk->hash;
					e.time = 0;
				}
				else if(sourceName == "temporary")
				{
					e.source = EffectSource::Temporary;
					e.sourceId = -1;
					t.Next();
					e.time = t.MustGetFloat();
				}
				else if(sourceName == "item")
				{
					e.source = EffectSource::Item;
					t.Next();
					const string slotId = t.MustGetItem();
					e.sourceId = ItemSlotInfo::Find(slotId);
					if(e.sourceId == SLOT_INVALID)
					{
						Msg("Invalid item source '%s'.", slotId.c_str());
						break;
					}
					e.time = 0;
				}
				else
				{
					Msg("Invalid effect source '%s'.", sourceName.c_str());
					break;
				}
			}
			else
			{
				e.source = EffectSource::Permanent;
				e.sourceId = -1;
				e.time = 0;
			}

			Unit* unit = game->pc->data.selectedUnit;
			if(!unit)
				Msg("No unit selected.");
			else if(Net::IsLocal())
				unit->AddEffect(e);
			else
			{
				PushGenericCmd(CMD_ADD_EFFECT)
					<< unit->id
					<< (char)e.effect
					<< (char)e.source
					<< e.sourceId
					<< (char)e.value
					<< e.power
					<< e.time;
			}
		}
		break;
	case CMD_REMOVE_EFFECT:
		if(!t.Next())
			Msg("Enter source or effect name.");
		else
		{
			EffectSource source = EffectSource::None;
			EffectId effect = EffectId::None;
			int sourceId = -1;
			int value = -1;

			const string& str = t.MustGetItem();
			if(str == "permanent")
			{
				source = EffectSource::Permanent;
				t.Next();
			}
			else if(str == "temporary")
			{
				source = EffectSource::Temporary;
				t.Next();
			}
			else if(str == "perk")
			{
				source = EffectSource::Perk;
				if(t.Next())
				{
					const string& perkId = t.MustGetItem();
					Perk* perk = Perk::Get(perkId);
					if(perk)
					{
						sourceId = perk->hash;
						t.Next();
					}
				}
			}
			else if(str == "item")
			{
				source = EffectSource::Item;
				if(t.Next())
				{
					const string slotId = t.MustGetItem();
					ITEM_SLOT slot = ItemSlotInfo::Find(slotId);
					if(slot != SLOT_INVALID)
					{
						sourceId = slot;
						t.Next();
					}
				}
			}

			if(t.IsItem())
			{
				const string& effectName = t.MustGetItem();
				effect = EffectInfo::TryGet(effectName);
				if(effect == EffectId::None)
				{
					Msg("Invalid effect or source '%s'.", effectName.c_str());
					break;
				}

				EffectInfo& info = EffectInfo::effects[(int)effect];
				if(info.valueType != EffectInfo::None && t.Next())
				{
					const string& valueStr = t.MustGetItem();
					if(info.valueType == EffectInfo::Attribute)
					{
						Attribute* attrib = Attribute::Find(valueStr);
						if(!attrib)
						{
							Msg("Invalid effect attribute '%s'.", valueStr.c_str());
							break;
						}
						value = (int)attrib->attribId;
					}
					else
					{
						const Skill* skill = Skill::Find(valueStr);
						if(!skill)
						{
							Msg("Invalid effect skill '%s'.", valueStr.c_str());
							break;
						}
						value = (int)skill->skillId;
					}
				}
			}

			Unit* unit = game->pc->data.selectedUnit;
			if(!unit)
				Msg("No unit selected.");
			else if(Net::IsLocal())
				RemoveEffect(unit, effect, source, sourceId, value);
			else
			{
				PushGenericCmd(CMD_REMOVE_EFFECT)
					<< unit->id
					<< (char)effect
					<< (char)source
					<< (char)sourceId
					<< (char)value;
			}
		}
		break;
	case CMD_LIST_EFFECTS:
		{
			Unit* unit = game->pc->data.selectedUnit;
			if(!unit)
				Msg("No unit selected.");
			else if(Net::IsLocal() || unit->IsLocalPlayer())
				ListEffects(unit);
			else
				PushGenericCmd(CMD_LIST_EFFECTS) << unit->id;
		}
		break;
	case CMD_ADD_PERK:
		if(!t.Next())
			Msg("Perk id required. Use 'list perks' to display list.");
		else
		{
			const string& perkId = t.MustGetItem();
			Perk* perk = Perk::Get(perkId);
			if(!perk)
			{
				Msg("Invalid perk '%s'.", perkId.c_str());
				break;
			}

			int value = -1;
			if(perk->valueType == Perk::Attribute)
			{
				if(!t.Next())
				{
					Msg("Perk '%s' require 'attribute' value.", perk->id.c_str());
					break;
				}
				const string& attribName = t.MustGetItem();
				Attribute* attrib = Attribute::Find(attribName);
				if(!attrib)
				{
					Msg("Invalid attribute '%s' for perk '%s'.", attribName.c_str(), perk->id.c_str());
					break;
				}
				value = (int)attrib->attribId;
			}
			else if(perk->valueType == Perk::Skill)
			{
				if(!t.Next())
				{
					Msg("Perk '%s' require 'skill' value.", perk->id.c_str());
					break;
				}
				const string& skillName = t.MustGetItem();
				const Skill* skill = Skill::Find(skillName);
				if(!skill)
				{
					Msg("Invalid skill '%s' for perk '%s'.", skillName.c_str(), perk->id.c_str());
					break;
				}
				value = (int)skill->skillId;
			}

			Unit* unit = game->pc->data.selectedUnit;
			if(!unit)
				Msg("No unit selected.");
			else if(!unit->IsPlayer())
				Msg("Only players have perks.");
			else if(Net::IsLocal())
				AddPerk(unit->player, perk, value);
			else
			{
				PushGenericCmd(CMD_ADD_PERK)
					<< unit->id
					<< perk->hash
					<< (char)value;
			}
		}
		break;
	case CMD_REMOVE_PERK:
		if(!t.Next())
			Msg("Perk id required. Use 'list perks' to display list.");
		else
		{
			const string& perkId = t.MustGetItem();
			Perk* perk = Perk::Get(perkId);
			if(!perk)
			{
				Msg("Invalid perk '%s'.", perkId.c_str());
				break;
			}

			int value = -1;
			if(perk->valueType == Perk::Attribute)
			{
				if(!t.Next())
				{
					Msg("Perk '%s' require 'attribute' value.", perk->id.c_str());
					break;
				}
				const string& attribName = t.MustGetItem();
				Attribute* attrib = Attribute::Find(attribName);
				if(!attrib)
				{
					Msg("Invalid attribute '%s' for perk '%s'.", attribName.c_str(), perk->id.c_str());
					break;
				}
				value = (int)attrib->attribId;
			}
			else if(perk->valueType == Perk::Skill)
			{
				if(!t.Next())
				{
					Msg("Perk '%s' require 'skill' value.", perk->id.c_str());
					break;
				}
				const string& skillName = t.MustGetItem();
				const Skill* skill = Skill::Find(skillName);
				if(!skill)
				{
					Msg("Invalid skill '%s' for perk '%s'.", skillName.c_str(), perk->id.c_str());
					break;
				}
				value = (int)skill->skillId;
			}

			Unit* unit = game->pc->data.selectedUnit;
			if(!unit)
				Msg("No unit selected.");
			else if(!unit->IsPlayer())
				Msg("Only players have perks.");
			else if(Net::IsLocal())
				RemovePerk(unit->player, perk, value);
			else
			{
				PushGenericCmd(CMD_REMOVE_PERK)
					<< unit->id
					<< perk->hash
					<< (char)value;
			}
		}
		break;
	case CMD_LIST_PERKS:
		{
			Unit* unit = game->pc->data.selectedUnit;
			if(!unit)
				Msg("No unit selected.");
			else if(!unit->IsPlayer())
				Msg("Only players have perks.");
			else if(Net::IsLocal() || unit->IsLocalPlayer())
				ListPerks(unit->player);
			else
				PushGenericCmd(CMD_LIST_PERKS) << unit->id;
		}
		break;
	case CMD_SELECT:
		{
			enum SelectType
			{
				SELECT_ME,
				SELECT_SHOW,
				SELECT_TARGET
			} select;

			if(t.Next())
			{
				const string& type = t.MustGetItem();
				if(type == "me")
					select = SELECT_ME;
				else if(type == "show")
					select = SELECT_SHOW;
				else if(type == "target")
					select = SELECT_TARGET;
				else
				{
					Msg("Invalid select type '%s'.", type.c_str());
					break;
				}
			}
			else if(game->pc->data.beforePlayer == BP_UNIT)
				select = SELECT_TARGET;
			else
				select = SELECT_SHOW;

			if(select == SELECT_ME)
				game->pc->data.selectedUnit = game->pc->unit;
			else if(select == SELECT_TARGET)
			{
				if(Unit* target = game->pc->data.GetTargetUnit())
					game->pc->data.selectedUnit = target;
				else
				{
					Msg("No unit in front of player.");
					break;
				}
			}

			Unit* unit = game->pc->data.selectedUnit;
			if(unit)
				Msg("Currently selected: %s (%s, %d)", unit->GetName(), unit->data->id.c_str(), unit->id);
			else
				Msg("Currently selected: none");
		}
		break;
	case CMD_LIST_STATS:
		{
			Unit* unit = game->pc->data.selectedUnit;
			if(!unit)
				Msg("No unit selected.");
			else if(Net::IsLocal() || unit->IsLocalPlayer())
				ListStats(unit);
			else
				PushGenericCmd(CMD_LIST_STATS) << unit->id;
		}
		break;
	case CMD_ADD_LEARNING_POINTS:
		{
			int count = 1;
			if(t.Next())
				count = t.MustGetInt();
			if(count < 1)
				break;

			Unit* unit = game->pc->data.selectedUnit;
			if(!unit)
				Msg("No unit selected.");
			else if(!unit->IsPlayer())
				Msg("Only players have learning points.");
			else if(Net::IsLocal())
				unit->player->AddLearningPoint(count);
			else
			{
				PushGenericCmd(CMD_ADD_LEARNING_POINTS)
					<< unit->id
					<< count;
			}
		}
		break;
	case CMD_CLEAN_LEVEL:
		{
			int buildingId = -2;
			if(t.Next())
				buildingId = t.MustGetInt();
			if(Net::IsLocal())
				gameLevel->CleanLevel(buildingId);
			else
			{
				NetChange& c = Net::PushChange(NetChange::CLEAN_LEVEL);
				c.id = buildingId;
			}
		}
		break;
	case CMD_ARENA:
		if(!gameLevel->HaveArena())
			Msg("Arena required inside location.");
		else
		{
			cstring s = t.GetTextRest();
			if(Net::IsLocal())
				ArenaCombat(s);
			else
			{
				NetChange& c = Net::PushChange(NetChange::CHEAT_ARENA);
				c.str = StringPool.Get();
				*c.str = s;
			}
		}
		break;
	case CMD_REMOVE_UNIT:
		{
			Unit* unit = game->pc->data.selectedUnit;
			if(!unit)
				Msg("No unit selected.");
			else if(unit->IsPlayer())
				Msg("Can't remove player unit.");
			else
				gameLevel->RemoveUnit(unit);
		}
		break;
	case CMD_ADD_EXP:
		if(t.Next())
		{
			int exp = t.MustGetInt();
			if(exp <= 0)
				Msg("Exp must by positive!");
			if(Net::IsLocal())
				team->AddExp(exp);
			else
				PushGenericCmd(CMD_ADD_EXP) << exp;
		}
		else
			Msg("You need to enter exp amount!");
		break;
	case CMD_NOCD:
		if(t.Next())
		{
			game->pc->nocd = t.MustGetBool();
			if(Net::IsClient())
				PushGenericCmd(CMD_NOCD) << game->pc->nocd;
		}
		Msg("nocd = %d", game->pc->nocd ? 1 : 0);
		break;
	case CMD_FIND:
		if(!t.Next())
			Msg("Missing type!");
		else
		{
			const string& type = t.MustGetItem();
			if(type != "trap")
			{
				Msg("Invalid type!");
				break;
			}
			if(!t.Next())
			{
				Msg("Missing id!");
				break;
			}

			const string& id = t.MustGetItem();
			BaseTrap* base = BaseTrap::Get(id);
			if(!base)
			{
				Msg("Invalid trap id!");
				break;
			}

			Trap* trap = gameLevel->FindTrap(base, game->pc->unit->pos);
			if(trap)
				Msg("Closest trap '%s': %g; %g; %g", base->id, trap->pos.x, trap->pos.y, trap->pos.z);
			else
				Msg("Closest trap '%s': Not found", base->id);
		}
		break;
	default:
		assert(0);
		break;
	}
}

//=================================================================================================
bool CommandParser::ParseStream(BitStreamReader& f, PlayerInfo& info)
{
	LocalString str;
	PrintMsgFunc prevFunc = printMsgFunc;
	printMsgFunc = [&str](cstring s)
	{
		if(!str.empty())
			str += "\n";
		str += s;
	};

	bool result = ParseStreamInner(f, info.pc);

	printMsgFunc = prevFunc;

	if(result && !str.empty())
	{
		NetChangePlayer& c = info.PushChange(NetChangePlayer::GENERIC_CMD_RESPONSE);
		c.str = str.Pin();
	}

	return result;
}

//=================================================================================================
void CommandParser::ParseStringCommand(int cmd, const string& s, PlayerInfo& info)
{
	LocalString str;
	PrintMsgFunc prevFunc = printMsgFunc;
	printMsgFunc = [&str](cstring s)
	{
		if(!str.empty())
			str += "\n";
		str += s;
	};

	switch((CMD)cmd)
	{
	case CMD_ARENA:
		ArenaCombat(s.c_str());
		break;
	}

	printMsgFunc = prevFunc;

	if(!str.empty())
	{
		NetChangePlayer& c = info.PushChange(NetChangePlayer::GENERIC_CMD_RESPONSE);
		c.str = str.Pin();
	}
}

//=================================================================================================
bool CommandParser::ParseStreamInner(BitStreamReader& f, PlayerController* player)
{
	CMD cmd = (CMD)f.Read<byte>();
	switch(cmd)
	{
	case CMD_ADD_EFFECT:
		{
			int id;
			Effect e;

			f >> id;
			f.ReadCasted<char>(e.effect);
			f.ReadCasted<char>(e.source);
			f >> e.sourceId;
			f.ReadCasted<char>(e.value);
			f >> e.power;
			f >> e.time;

			Unit* unit = gameLevel->FindUnit(id);
			if(!unit)
			{
				Error("CommandParser CMD_ADD_EFFECT: Missing unit %d.", id);
				return false;
			}
			if(e.effect >= EffectId::Max)
			{
				Error("CommandParser CMD_ADD_EFFECT: Invalid effect %d.", e.effect);
				return false;
			}
			if(e.source >= EffectSource::Max)
			{
				Error("CommandParser CMD_ADD_EFFECT: Invalid effect source %d.", e.source);
				return false;
			}
			if(e.source == EffectSource::Perk)
			{
				if(!Perk::Get(e.sourceId))
				{
					Error("CommandParser CMD_ADD_EFFECT: Invalid source id %d for perk source.", e.sourceId);
					return false;
				}
			}
			else if(e.source == EffectSource::Item)
			{
				if(e.sourceId < 0 || e.sourceId >= SLOT_MAX)
				{
					Error("CommandParser CMD_ADD_EFFECT: Invalid source id %d for item source.", e.sourceId);
					return false;
				}
			}
			else if(e.sourceId != -1)
			{
				Error("CommandParser CMD_ADD_EFFECT: Invalid source id %d for source %d.", e.sourceId, e.source);
				return false;
			}
			if(e.time > 0 && e.source != EffectSource::Temporary)
			{
				Error("CommandParser CMD_ADD_EFFECT: Invalid time %g for source %d.", e.time, e.source);
				return false;
			}

			EffectInfo& info = EffectInfo::effects[(int)e.effect];
			bool okValue;
			if(info.valueType == EffectInfo::None)
				okValue = (e.value == -1);
			else if(info.valueType == EffectInfo::Attribute)
				okValue = (e.value >= 0 && e.value < (int)AttributeId::MAX);
			else
				okValue = (e.value >= 0 && e.value < (int)SkillId::MAX);
			if(!okValue)
			{
				Error("CommandParser CMD_ADD_EFFECT: Invalid value %d for effect %s.", e.value, info.id);
				return false;
			}

			unit->AddEffect(e);
		}
		break;
	case CMD_REMOVE_EFFECT:
		{
			int id;
			EffectId effect;
			EffectSource source;
			int sourceId;
			int value;

			f >> id;
			f.ReadCasted<char>(effect);
			f.ReadCasted<char>(source);
			f >> sourceId;
			f.ReadCasted<char>(value);

			Unit* unit = gameLevel->FindUnit(id);
			if(!unit)
			{
				Error("CommandParser CMD_REMOVE_EFFECT: Missing unit %d.", id);
				return false;
			}
			if(effect >= EffectId::Max)
			{
				Error("CommandParser CMD_REMOVE_EFFECT: Invalid effect %d.", effect);
				return false;
			}
			if(source >= EffectSource::Max)
			{
				Error("CommandParser CMD_REMOVE_EFFECT: Invalid effect source %d.", source);
				return false;
			}
			if(source == EffectSource::Perk)
			{
				if(!Perk::Get(sourceId))
				{
					Error("CommandParser CMD_REMOVE_EFFECT: Invalid source id %d for perk source.", sourceId);
					return false;
				}
			}
			else if(source == EffectSource::Item)
			{
				if(sourceId < 0 || sourceId >= SLOT_MAX)
				{
					Error("CommandParser CMD_REMOVE_EFFECT: Invalid source id %d for item source.", sourceId);
					return false;
				}
			}
			else if(sourceId != -1)
			{
				Error("CommandParser CMD_REMOVE_EFFECT: Invalid source id %d for source %d.", sourceId, source);
				return false;
			}

			if((int)effect >= 0)
			{
				EffectInfo& info = EffectInfo::effects[(int)effect];
				bool okValue;
				if(info.valueType == EffectInfo::None)
					okValue = (value == -1);
				else if(info.valueType == EffectInfo::Attribute)
					okValue = (value >= 0 && value < (int)AttributeId::MAX);
				else
					okValue = (value >= 0 && value < (int)SkillId::MAX);
				if(!okValue)
				{
					Error("CommandParser CMD_REMOVE_EFFECT: Invalid value %d for effect %s.", value, info.id);
					return false;
				}
			}

			RemoveEffect(unit, effect, source, sourceId, value);
		}
		break;
	case CMD_LIST_EFFECTS:
		{
			int id;
			f >> id;
			Unit* unit = gameLevel->FindUnit(id);
			if(!unit)
			{
				Error("CommandParser CMD_LIST_EFFECTS: Missing unit %d.", id);
				return false;
			}
			ListEffects(unit);
		}
		break;
	case CMD_ADD_PERK:
		{
			int id, value;
			int perkHash;

			f >> id;
			f >> perkHash;
			f.ReadCasted<char>(value);

			Unit* unit = gameLevel->FindUnit(id);
			if(!unit)
			{
				Error("CommandParser CMD_ADD_PERK: Missing unit %d.", id);
				return false;
			}
			if(!unit->player)
			{
				Error("CommandParser CMD_ADD_PERK: Unit %d is not player.", id);
				return false;
			}
			Perk* perk = Perk::Get(perkHash);
			if(!perk)
			{
				Error("CommandParser CMD_ADD_PERK: Invalid perk %u.", perkHash);
				return false;
			}
			if(perk->valueType == Perk::None)
			{
				if(value != -1)
				{
					Error("CommandParser CMD_ADD_PERK: Invalid value %d for perk '%s'.", value, perk->id.c_str());
					return false;
				}
			}
			else if(perk->valueType == Perk::Attribute)
			{
				if(value <= (int)AttributeId::MAX)
				{
					Error("CommandParser CMD_ADD_PERK: Invalid value %d for perk '%s'.", value, perk->id.c_str());
					return false;
				}
			}
			else if(perk->valueType == Perk::Skill)
			{
				if(value <= (int)SkillId::MAX)
				{
					Error("CommandParser CMD_ADD_PERK: Invalid value %d for perk '%s'.", value, perk->id.c_str());
					return false;
				}
			}

			AddPerk(unit->player, perk, value);
		}
		break;
	case CMD_REMOVE_PERK:
		{
			int id, value;
			int perkHash;

			f >> id;
			f >> perkHash;
			f.ReadCasted<char>(value);

			Unit* unit = gameLevel->FindUnit(id);
			if(!unit)
			{
				Error("CommandParser CMD_REMOVE_PERK: Missing unit %d.", id);
				return false;
			}
			if(!unit->player)
			{
				Error("CommandParser CMD_REMOVE_PERK: Unit %d is not player.", id);
				return false;
			}
			Perk* perk = Perk::Get(perkHash);
			if(!perk)
			{
				Error("CommandParser CMD_REMOVE_PERK: Invalid perk %u.", perkHash);
				return false;
			}
			if(perk->valueType == Perk::None)
			{
				if(value != -1)
				{
					Error("CommandParser CMD_REMOVE_PERK: Invalid value %d for perk '%s'.", value, perk->id.c_str());
					return false;
				}
			}
			else if(perk->valueType == Perk::Attribute)
			{
				if(value <= (int)AttributeId::MAX)
				{
					Error("CommandParser CMD_REMOVE_PERK: Invalid value %d for perk '%s'.", value, perk->id.c_str());
					return false;
				}
			}
			else if(perk->valueType == Perk::Skill)
			{
				if(value <= (int)SkillId::MAX)
				{
					Error("CommandParser CMD_REMOVE_PERK: Invalid value %d for perk '%s'.", value, perk->id.c_str());
					return false;
				}
			}

			RemovePerk(unit->player, perk, value);
		}
		break;
	case CMD_LIST_PERKS:
		{
			int id;
			f >> id;
			Unit* unit = gameLevel->FindUnit(id);
			if(!unit)
			{
				Error("CommandParser CMD_LIST_PERKS: Missing unit %d.", id);
				return false;
			}
			if(!unit->player)
			{
				Error("CommandParser CMD_LIST_PERKS: Unit %d is not player.", id);
				return false;
			}
			ListPerks(unit->player);
		}
		break;
	case CMD_LIST_STATS:
		{
			int id;
			f >> id;
			Unit* unit = gameLevel->FindUnit(id);
			if(!unit)
			{
				Error("CommandParser CMD_LIST_STAT: Missing unit %d.", id);
				return false;
			}
			ListStats(unit);
		}
		break;
	case CMD_ADD_LEARNING_POINTS:
		{
			int id, count;

			f >> id;
			f >> count;

			Unit* unit = gameLevel->FindUnit(id);
			if(!unit)
			{
				Error("CommandParser CMD_ADD_LEARNING_POINTS: Missing unit %d.", id);
				return false;
			}
			if(!unit->player)
			{
				Error("CommandParser CMD_ADD_LEARNING_POINTS: Unit %d is not player.", id);
				return false;
			}
			if(count < 1)
			{
				Error("CommandParser CMD_ADD_LEARNING_POINTS: Invalid count %d.", count);
				return false;
			}

			unit->player->AddLearningPoint(count);
		}
		break;
	case CMD_ADD_EXP:
		{
			int exp;
			f >> exp;
			if(exp <= 0)
			{
				Error("CommandParser CMD_ADD_EXP: Value must be positive.");
				return false;
			}
			team->AddExp(exp);
		}
		break;
	case CMD_NOCD:
		f >> player->nocd;
		break;
	case CMD_NOCLIP:
		f >> player->noclip;
		break;
	case CMD_GODMODE:
		f >> player->godmode;
		break;
	case CMD_INVISIBLE:
		f >> player->invisible;
		break;
	case CMD_REVEAL:
		world->Reveal();
		break;
	case CMD_HEAL:
		if(game->gameState == GS_LEVEL)
			HealUnit(*player->unit);
		break;
	case CMD_KILL:
		{
			int id;
			f >> id;
			if(game->gameState == GS_LEVEL)
			{
				if(Unit* unit = gameLevel->FindUnit(id))
				{
					if(unit->IsAlive())
						unit->GiveDmg(unit->hpmax);
				}
				else
				{
					Error("CommandParser CMD_KILL: Missing unit %d.", id);
					return false;
				}
			}
		}
		break;
	case CMD_HEAL_UNIT:
		{
			int id;
			f >> id;
			if(game->gameState == GS_LEVEL)
			{
				if(Unit* unit = gameLevel->FindUnit(id))
					HealUnit(*unit);
				else
				{
					Error("CommandParser CMD_HEAL_UNIT: Missing unit %d.", id);
					return false;
				}
			}
		}
		break;
	case CMD_SUICIDE:
		if(game->gameState == GS_LEVEL)
			player->unit->GiveDmg(player->unit->hpmax);
		break;
	case CMD_SCARE:
		Scare(player);
		break;
	case CMD_CITIZEN:
		if(team->isBandit || team->craziesAttack)
		{
			team->isBandit = false;
			team->craziesAttack = false;
			Net::PushChange(NetChange::CHANGE_FLAGS);
		}
		break;
	case CMD_SKIP_DAYS:
		{
			int count;
			f >> count;
			world->Update(count, UM_SKIP);
		}
		break;
	case CMD_WARP:
		{
			byte buildingIndex;
			bool inside;
			f >> buildingIndex;
			f >> inside;
			if(game->gameState != GS_LEVEL)
				break;
			Unit& unit = *player->unit;
			if(unit.frozen != FROZEN::NO)
			{
				Error("CommandParser CMD_WARP: Unit is frozen.");
				break;
			}
			if(inside)
			{
				if(!gameLevel->cityCtx || buildingIndex >= gameLevel->cityCtx->insideBuildings.size())
				{
					Error("CommandParser CMD_WARP: Invalid inside building index %u.", buildingIndex);
					break;
				}

				if(unit.locPart->partId != buildingIndex)
				{
					Net::WarpData& warp = Add1(net->warps);
					warp.u = &unit;
					warp.where = buildingIndex;
					warp.building = -1;
					warp.timer = 1.f;
					unit.frozen = (unit.usable ? FROZEN::YES_NO_ANIM : FROZEN::YES);
					player->playerInfo->PushChange(NetChangePlayer::PREPARE_WARP);
				}
				else
				{
					unit.rot = PI;
					gameLevel->WarpUnit(unit, gameLevel->cityCtx->insideBuildings[buildingIndex]->insideSpawn);
				}
			}
			else
			{
				if(!gameLevel->cityCtx || buildingIndex >= gameLevel->cityCtx->buildings.size())
				{
					Error("CommandParser CMD_WARP: Invalid building index %u.", buildingIndex);
					return false;
				}

				if(unit.locPart->partType != LocationPart::Type::Outside)
				{
					Net::WarpData& warp = Add1(net->warps);
					warp.u = &unit;
					warp.where = -1;
					warp.building = buildingIndex;
					warp.timer = 1.f;
					unit.frozen = (unit.usable ? FROZEN::YES_NO_ANIM : FROZEN::YES);
					player->playerInfo->PushChange(NetChangePlayer::PREPARE_WARP);
				}
				else
				{
					CityBuilding& cityBuilding = gameLevel->cityCtx->buildings[buildingIndex];
					gameLevel->WarpUnit(unit, cityBuilding.walkPt);
					unit.RotateTo(PtToPos(cityBuilding.pt));
				}
			}
		}
		break;
	case CMD_HURT:
	case CMD_BREAK_ACTION:
	case CMD_FALL:
		{
			int id;
			f >> id;
			if(game->gameState == GS_LEVEL)
			{
				if(Unit* unit = gameLevel->FindUnit(id))
				{
					switch(cmd)
					{
					case CMD_HURT:
						unit->GiveDmg(100.f);
						break;
					case CMD_BREAK_ACTION:
						unit->BreakAction(Unit::BREAK_ACTION_MODE::NORMAL, true);
						break;
					case CMD_FALL:
						unit->Fall();
						break;
					}
				}
				else
				{
					cstring name;
					switch(cmd)
					{
					case CMD_HURT:
						name = "CMD_HURT";
						break;
					case CMD_BREAK_ACTION:
						name = "CMD_BREAK_ACTION";
						break;
					case CMD_FALL:
						name = "CMD_FALL";
						break;
					}
					Error("CommandParser %s: Missing unit %d.", name, id);
					return false;
				}
			}
		}
		break;
	case CMD_STUN:
		{
			int id;
			float length;
			f >> id;
			f >> length;
			if(game->gameState == GS_LEVEL)
			{
				if(Unit* unit = gameLevel->FindUnit(id))
				{
					Effect e;
					e.effect = EffectId::Stun;
					e.source = EffectSource::Temporary;
					e.sourceId = -1;
					e.value = -1;
					e.power = 0;
					e.time = length;
					unit->AddEffect(e);
				}
				else
				{
					Error("CommandParser CMD_HEAL_UNIT: Missing unit %d.", id);
					return false;
				}
			}
		}
		break;
	case CMD_REFRESH_COOLDOWN:
		if(game->gameState == GS_LEVEL)
			player->RefreshCooldown();
		break;
	case CMD_KILLALL:
		{
			bool friendly;
			int id;
			f >> friendly;
			f >> id;
			if(game->gameState == GS_LEVEL)
			{
				Unit* ignored;
				if(id == -1)
					ignored = nullptr;
				else
				{
					ignored = gameLevel->FindUnit(id);
					if(!ignored)
					{
						Error("CommandParsed CMD_KILLALL: Missing unit %d.", id);
						return false;
					}
				}

				gameLevel->KillAll(friendly, *player->unit, ignored);
			}
		}
		break;
	case CMD_ADD_GOLD:
		{
			bool isTeam;
			int count;
			f >> isTeam;
			f >> count;
			if(isTeam)
			{
				if(count <= 0)
				{
					Error("CommandParsed CMD_ADD_GOLD: Invalid count %d.", count);
				}
				else
					team->AddGold(count);
			}
			else
			{
				player->unit->gold = max(player->unit->gold + count, 0);
				player->playerInfo->UpdateGold();
			}
		}
		break;
	default:
		Error("Unknown generic command %u.", cmd);
		return false;
	}
	return true;
}

//=================================================================================================
void CommandParser::HealUnit(Unit& unit)
{
	if(unit.hp != unit.hpmax)
	{
		unit.hp = unit.hpmax;
		if(Net::IsServer())
		{
			NetChange& c = Net::PushChange(NetChange::UPDATE_HP);
			c.unit = &unit;
		}
	}
	if(unit.mp != unit.mpmax)
	{
		unit.mp = unit.mpmax;
		if(Net::IsServer() && unit.IsTeamMember())
		{
			NetChange& c = Net::PushChange(NetChange::UPDATE_MP);
			c.unit = &unit;
		}
	}
	if(unit.stamina != unit.staminaMax)
	{
		unit.stamina = unit.staminaMax;
		if(Net::IsServer() && unit.IsTeamMember())
		{
			NetChange& c = Net::PushChange(NetChange::UPDATE_STAMINA);
			c.unit = &unit;
		}
	}

	// remove negative effect
	unit.RemoveEffect(EffectId::Poison);
	unit.RemoveEffect(EffectId::Stun);
	unit.RemoveEffect(EffectId::Rooted);
	unit.RemoveEffect(EffectId::SlowMove);
}

//=================================================================================================
void CommandParser::RemoveEffect(Unit* u, EffectId effect, EffectSource source, int sourceId, int value)
{
	uint removed = u->RemoveEffects(effect, source, sourceId, value);
	Msg("%u effects removed.", removed);
}

//=================================================================================================
void CommandParser::ListEffects(Unit* u)
{
	if(u->effects.empty())
	{
		Msg("Unit have no effects.");
		return;
	}

	LocalString s;
	s = Format("Unit effects (%u):", u->effects.size());
	for(Effect& e : u->effects)
	{
		EffectInfo& info = EffectInfo::effects[(int)e.effect];
		s += '\n';
		s += info.id;
		if(info.valueType == EffectInfo::Attribute)
			s += Format("(%s)", Attribute::attributes[e.value].id);
		else if(info.valueType == EffectInfo::Skill)
			s += Format("(%s)", Skill::skills[e.value].id);
		s += Format(", power %g, source ", e.power);
		switch(e.source)
		{
		case EffectSource::Temporary:
			s += Format("temporary, time %g", e.time);
			break;
		case EffectSource::Perk:
			s += Format("perk (%s)", Perk::Get(e.sourceId)->id.c_str());
			break;
		case EffectSource::Permanent:
			s += "permanent";
			break;
		case EffectSource::Item:
			s += "item (";
			if(e.sourceId < 0 || e.sourceId >= SLOT_MAX)
				s += Format("invalid %d", e.sourceId);
			else
				s += ItemSlotInfo::slots[e.sourceId].id;
			s += ')';
			break;
		}
	}
	Msg(s.c_str());
}

//=================================================================================================
void CommandParser::AddPerk(PlayerController* pc, Perk* perk, int value)
{
	if(!pc->AddPerk(perk, value))
		Msg("Unit already have this perk.");
}

//=================================================================================================
void CommandParser::RemovePerk(PlayerController* pc, Perk* perk, int value)
{
	if(!pc->RemovePerk(perk, value))
		Msg("Unit don't have this perk.");
}

//=================================================================================================
void CommandParser::ListPerks(PlayerController* pc)
{
	if(pc->perks.empty())
	{
		Msg("Unit have no perks.");
		return;
	}

	LocalString s;
	s = Format("Unit perks (%u):", pc->perks.size());
	for(TakenPerk& tp : pc->perks)
	{
		s += Format("\n%s", tp.perk->id.c_str());
		if(tp.perk->valueType == Perk::Attribute)
			s += Format(" (%s)", Attribute::attributes[tp.value].id);
		else if(tp.perk->valueType == Perk::Skill)
			s += Format(" (%s)", Skill::skills[tp.value].id);
	}
	Msg(s.c_str());
}

int ConvertResistance(float value)
{
	return (int)round((1.f - value) * 100);
}

//=================================================================================================
void CommandParser::ListStats(Unit* u)
{
	int hp = int(u->hp);
	if(hp == 0 && u->hp > 0)
		hp = 1;
	if(u->IsPlayer())
		u->player->RecalculateLevel();
	Msg("--- %s (%s) level %d ---", u->GetName(), u->data->id.c_str(), u->level);
	if(u->data->statProfile && !u->data->statProfile->subprofiles.empty() && !u->IsPlayer())
	{
		Msg("Profile %s.%s (weapon:%s armor:%s)",
			u->data->statProfile->id.c_str(),
			u->data->statProfile->subprofiles[u->stats->subprofile.index]->id.c_str(),
			Skill::skills[(int)WeaponTypeInfo::info[u->stats->subprofile.weapon].skill].id,
			Skill::skills[(int)GetArmorTypeSkill((ARMOR_TYPE)u->stats->subprofile.armor)].id);
	}
	Msg("Health: %d/%d (bonus: %+g, regeneration: %+g/sec, natural: x%g)", hp, (int)u->hpmax, u->GetEffectSum(EffectId::Health),
		u->GetEffectSum(EffectId::Regeneration), u->GetEffectMul(EffectId::NaturalHealingMod));
	if(u->IsUsingMp())
	{
		Msg("Mana: %d/%d (bonus: %+g, regeneration: %+g/sec, mod: x%g)", (int)u->mp, (int)u->mpmax, u->GetEffectSum(EffectId::Mana), u->GetMpRegen(),
			1.f + u->GetEffectSum(EffectId::ManaRegeneration));
	}
	Msg("Stamina: %d/%d (bonus: %+g, regeneration: %+g/sec, mod: x%g)", (int)u->stamina, (int)u->staminaMax, u->GetEffectSum(EffectId::Stamina),
		u->GetEffectMax(EffectId::StaminaRegeneration), u->GetEffectMul(EffectId::StaminaRegenerationMod));
	Msg("Melee attack: %s (bonus: %+g, backstab: x%g), ranged: %s (bonus: %+g, backstab: x%g)",
		(u->HaveWeapon() || u->data->type == UNIT_TYPE::ANIMAL) ? Format("%d", (int)u->CalculateAttack()) : "-",
		u->GetEffectSum(EffectId::MeleeAttack),
		1.f + u->GetBackstabMod(u->GetEquippedItem(SLOT_WEAPON)),
		u->HaveBow() ? Format("%d", (int)u->CalculateAttack(&u->GetBow())) : "-",
		u->GetEffectSum(EffectId::RangedAttack),
		1.f + u->GetBackstabMod(u->GetEquippedItem(SLOT_BOW)));
	Msg("Defense %d (bonus: %+g), block: %s", (int)u->CalculateDefense(), u->GetEffectSum(EffectId::Defense),
		u->HaveShield() ? Format("%d", (int)u->CalculateBlock()) : "-");
	Msg("Mobility: %d (bonus %+g)", (int)u->CalculateMobility(), u->GetEffectSum(EffectId::Mobility));
	Msg("Carry: %g/%g (mod: x%g)", u->GetWeight(), u->GetWeightMax(), u->GetEffectMul(EffectId::Carry));
	Msg("Magic resistance: %d%%, magic power: %+d", ConvertResistance(u->CalculateMagicResistance()), u->CalculateMagicPower());
	Msg("Poison resistance: %d%%", ConvertResistance(u->GetPoisonResistance()));
	LocalString s = "Attributes: ";
	for(int i = 0; i < (int)AttributeId::MAX; ++i)
	{
		int value = u->Get((AttributeId)i);
		float bonus = 0;
		for(Effect& e : u->effects)
		{
			if(e.effect == EffectId::Attribute && e.value == i)
				bonus += e.power;
		}
		if(bonus == 0)
			s += Format("%s:%d ", Attribute::attributes[i].id, value);
		else
			s += Format("%s:%d(%+g) ", Attribute::attributes[i].id, value, bonus);
	}
	Msg(s.c_str());
	s = "Skills: ";
	for(int i = 0; i < (int)SkillId::MAX; ++i)
	{
		int value = u->Get((SkillId)i, nullptr, false);
		if(value > 0)
		{
			float bonus = 0;
			for(Effect& e : u->effects)
			{
				if(e.effect == EffectId::Skill && e.value == i)
					bonus += e.power;
			}
			if(bonus == 0)
				s += Format("%s:%d ", Skill::skills[i].id, value);
			else
				s += Format("%s:%d(%+g)", Skill::skills[i].id, value, bonus);
		}
	}
	Msg(s.c_str());
}

//=================================================================================================
void CommandParser::ArenaCombat(cstring str)
{
	vector<Arena::Enemy> units;
	Tokenizer t;
	t.FromString(str);
	bool any[2] = { false,false };
	try
	{
		bool side = false;
		t.Next();
		while(!t.IsEof())
		{
			if(t.IsItem("vs"))
			{
				side = true;
				t.Next();
			}
			uint count = 1;
			if(t.IsInt())
			{
				count = t.GetInt();
				if(count < 1)
					t.Throw("Invalid count %d.", count);
				t.Next();
			}
			const string& id = t.MustGetItem();
			UnitData* ud = UnitData::TryGet(id);
			if(!ud || IsSet(ud->flags, F_SECRET))
				t.Throw("Missing unit '%s'.", id.c_str());
			else if(ud->group == G_PLAYER)
				t.Throw("Unit '%s' can't be spawned.", id.c_str());
			t.Next();
			int level = -2;
			if(t.IsItem("lvl"))
			{
				t.Next();
				level = t.MustGetInt();
				t.Next();
			}
			units.push_back({ ud, count, level, side });
			any[side ? 1 : 0] = true;
		}
	}
	catch(const Tokenizer::Exception& e)
	{
		Msg("Broken units list: %s", e.ToString());
		return;
	}

	if(units.size() == 0)
		Msg("Empty units list.");
	else if(!any[0] || !any[1])
		Msg("Missing other units.");
	else
		game->arena->SpawnUnit(units);
}

//=================================================================================================
void CommandParser::CmdList()
{
	if(!t.Next())
	{
		Msg("Display list of items/units/quests (item/items, itemn/itemNames, unit/units, unitn/unitNames, quest(s), effect(s), perk(s)). Examples:");
		Msg("'list item' - list of items ordered by id");
		Msg("'list itemn' - list of items ordered by name");
		Msg("'list unit t' - list of units ordered by id starting from t");
		return;
	}

	enum LIST_TYPE
	{
		LIST_ITEM,
		LIST_ITEM_NAME,
		LIST_UNIT,
		LIST_UNIT_NAME,
		LIST_QUEST,
		LIST_EFFECT,
		LIST_PERK
	};

	LIST_TYPE listType;
	const string& lis = t.MustGetItem();
	if(lis == "item" || lis == "items")
		listType = LIST_ITEM;
	else if(lis == "itemn" || lis == "itemNames")
		listType = LIST_ITEM_NAME;
	else if(lis == "unit" || lis == "units")
		listType = LIST_UNIT;
	else if(lis == "unitn" || lis == "unitNames")
		listType = LIST_UNIT_NAME;
	else if(lis == "quest" || lis == "quests")
		listType = LIST_QUEST;
	else if(lis == "effect" || lis == "effects")
		listType = LIST_EFFECT;
	else if(lis == "perk" || lis == "perks")
		listType = LIST_PERK;
	else
	{
		Msg("Unknown list type '%s'!", lis.c_str());
		return;
	}

	LocalString match;
	if(t.Next())
		match = t.MustGetText();

	switch(listType)
	{
	case LIST_ITEM:
		{
			LocalVector<const Item*> items;
			for(auto it : Item::items)
			{
				auto item = it.second;
				if(match.empty() || _strnicmp(match.c_str(), item->id.c_str(), match.length()) == 0)
					items.push_back(item);
			}

			if(items.empty())
			{
				Msg("No items found starting with '%s'.", match.c_str());
				return;
			}

			std::sort(items.begin(), items.end(), [](const Item* item1, const Item* item2)
			{
				return item1->id < item2->id;
			});

			LocalString s = Format("Items list (%d):\n", items.size());
			Msg("Items list (%d):", items.size());

			for(auto item : items)
			{
				cstring s2 = Format("%s (%s)", item->id.c_str(), item->name.c_str());
				Msg(s2);
				s += s2;
				s += "\n";
			}

			Info(s.c_str());
		}
		break;
	case LIST_ITEM_NAME:
		{
			LocalVector<const Item*> items;
			for(auto it : Item::items)
			{
				auto item = it.second;
				if(match.empty() || _strnicmp(match.c_str(), item->name.c_str(), match.length()) == 0)
					items.push_back(item);
			}

			if(items.empty())
			{
				Msg("No items found starting with name '%s'.", match.c_str());
				return;
			}

			std::sort(items.begin(), items.end(), [](const Item* item1, const Item* item2)
			{
				return strcoll(item1->name.c_str(), item2->name.c_str()) < 0;
			});

			LocalString s = Format("Items list (%d):\n", items.size());
			Msg("Items list (%d):", items.size());

			for(auto item : items)
			{
				cstring s2 = Format("%s (%s)", item->name.c_str(), item->id.c_str());
				Msg(s2);
				s += s2;
				s += "\n";
			}

			Info(s.c_str());
		}
		break;
	case LIST_UNIT:
		{
			LocalVector<UnitData*> units;
			for(auto unit : UnitData::units)
			{
				if(!IsSet(unit->flags, F_SECRET) && (match.empty() || _strnicmp(match.c_str(), unit->id.c_str(), match.length()) == 0))
					units.push_back(unit);
			}

			if(units.empty())
			{
				Msg("No units found starting with '%s'.", match.c_str());
				return;
			}

			std::sort(units.begin(), units.end(), [](const UnitData* unit1, const UnitData* unit2)
			{
				return unit1->id < unit2->id;
			});

			LocalString s = Format("Units list (%d):\n", units.size());
			Msg("Units list (%d):", units.size());

			for(auto unit : units)
			{
				cstring s2 = Format("%s (%s)", unit->id.c_str(), unit->name.c_str());
				Msg(s2);
				s += s2;
				s += "\n";
			}

			Info(s.c_str());
		}
		break;
	case LIST_UNIT_NAME:
		{
			LocalVector<UnitData*> units;
			for(auto unit : UnitData::units)
			{
				if(!IsSet(unit->flags, F_SECRET) && (match.empty() || _strnicmp(match.c_str(), unit->name.c_str(), match.length()) == 0))
					units.push_back(unit);
			}

			if(units.empty())
			{
				Msg("No units found starting with name '%s'.", match.c_str());
				return;
			}

			std::sort(units.begin(), units.end(), [](const UnitData* unit1, const UnitData* unit2)
			{
				return strcoll(unit1->name.c_str(), unit2->name.c_str()) < 0;
			});

			LocalString s = Format("Units list (%d):\n", units.size());
			Msg("Units list (%d):", units.size());

			for(auto unit : units)
			{
				cstring s2 = Format("%s (%s)", unit->name.c_str(), unit->id.c_str());
				Msg(s2);
				s += s2;
				s += "\n";
			}

			Info(s.c_str());
		}
		break;
	case LIST_QUEST:
		{
			LocalVector<const QuestInfo*> quests;
			for(auto& info : questMgr->GetQuestInfos())
			{
				if(match.empty() || _strnicmp(match.c_str(), info.name, match.length()) == 0)
					quests.push_back(&info);
			}

			if(quests.empty())
			{
				Msg("No quests found starting with '%s'.", match.c_str());
				return;
			}

			std::sort(quests.begin(), quests.end(), [](const QuestInfo* quest1, const QuestInfo* quest2)
			{
				return strcoll(quest1->name, quest2->name) < 0;
			});

			LocalString s = Format("Quests list (%d):\n", quests.size());
			Msg("Quests list (%d):", quests.size());

			cstring questType[] = {
				"mayor",
				"captain",
				"random",
				"unique"
			};

			for(auto quest : quests)
			{
				cstring s2 = Format("%s (%s)", quest->name, questType[(int)quest->type]);
				Msg(s2);
				s += s2;
				s += "\n";
			}

			Info(s.c_str());
		}
		break;
	case LIST_EFFECT:
		{
			LocalVector<const EffectInfo*> effects;
			for(EffectInfo& info : EffectInfo::effects)
			{
				if(match.empty() || _strnicmp(match.c_str(), info.id, match.length()) == 0)
					effects.push_back(&info);
			}

			if(effects.empty())
			{
				Msg("No effects found starting with '%s'.", match.c_str());
				return;
			}

			std::sort(effects.begin(), effects.end(), [](const EffectInfo* effect1, const EffectInfo* effect2)
			{
				return strcoll(effect1->id, effect2->id) < 0;
			});

			LocalString s = Format("Effects list (%d):\n", effects.size());
			Msg("Effects list (%d):", effects.size());

			for(const EffectInfo* effect : effects)
			{
				cstring s2 = Format("%s (%s)", effect->id, effect->desc);
				Msg(s2);
				s += s2;
				s += "\n";
			}

			Info(s.c_str());
		}
		break;
	case LIST_PERK:
		{
			LocalVector<const Perk*> perks;
			for(Perk* perk : Perk::perks)
			{
				if(match.empty() || _strnicmp(match.c_str(), perk->id.c_str(), match.length()) == 0)
					perks.push_back(perk);
			}

			if(perks.empty())
			{
				Msg("No perks found starting with '%s'.", match.c_str());
				return;
			}

			std::sort(perks.begin(), perks.end(), [](const Perk* perk1, const Perk* perk2)
			{
				return strcoll(perk1->id.c_str(), perk2->id.c_str()) < 0;
			});

			LocalString s = Format("Perks list (%d):\n", perks.size());
			Msg("Perks list (%d):", perks.size());

			for(const Perk* perk : perks)
			{
				cstring s2 = Format("%s (%s, %s)", perk->id.c_str(), perk->name.c_str(), perk->desc.c_str());
				Msg(s2);
				s += s2;
				s += "\n";
			}

			Info(s.c_str());
		}
		break;
	}
}

//=================================================================================================
NetChangeWriter CommandParser::PushGenericCmd(CMD cmd)
{
	NetChange& c = Net::PushChange(NetChange::GENERIC_CMD);
	return (c << (byte)cmd);
}

//=================================================================================================
void CommandParser::Scare(PlayerController* player)
{
	for(AIController* ai : game->ais)
	{
		if(ai->unit->IsEnemy(*player->unit) && Vec3::Distance(ai->unit->pos, player->unit->pos) < ALERT_RANGE && gameLevel->CanSee(*ai->unit, *player->unit))
		{
			ai->morale = -10;
			ai->targetLastPos = player->unit->pos;
		}
	}
}
