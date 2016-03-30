// komendy w konsoli
#include "Pch.h"
#include "Base.h"
#include "ConsoleCommands.h"
#include "Game.h"
#include "Version.h"
#include "Terrain.h"

#define MSG(x) print_func(x)

extern string g_ctime;

//=================================================================================================
// Register standard console commands
void Game::AddCommands()
{
	AddCommand(ConsoleCommand(&cl_fog, "cl_fog", "draw fog (cl_fog 0/1)", F_ANYWHERE | F_CHEAT | F_WORLD_MAP));
	AddCommand(ConsoleCommand(&cl_glow, "cl_glow", "use glow (cl_glow 0/1)", F_ANYWHERE | F_WORLD_MAP));
	AddCommand(ConsoleCommand(&cl_lighting, "cl_lighting", "use lighting (cl_lighting 0/1)", F_ANYWHERE | F_CHEAT | F_WORLD_MAP));
	AddCommand(ConsoleCommand(&cl_normalmap, "cl_normalmap", "use normal mapping (cl_normalmap 0/1)", F_ANYWHERE | F_WORLD_MAP));
	AddCommand(ConsoleCommand(&cl_postfx, "cl_postfx", "use post effects (cl_postfx 0/1)", F_ANYWHERE | F_WORLD_MAP));
	AddCommand(ConsoleCommand(&cl_specularmap, "cl_specularmap", "use specular mapping (cl_specularmap 0/1)", F_ANYWHERE | F_WORLD_MAP));
	AddCommand(ConsoleCommand(&devmode, "devmode", "developer mode (devmode 0/1)", F_GAME | F_SERVER | F_WORLD_MAP | F_MENU));
	AddCommand(ConsoleCommand(&dont_wander, "dont_wander", "citizens don't wander around city (dont_wander 0/1)", F_ANYWHERE | F_WORLD_MAP));
	AddCommand(ConsoleCommand(&draw_col, "draw_col", "draw colliders (draw_col 0/1)", F_ANYWHERE | F_CHEAT | F_WORLD_MAP));
	AddCommand(ConsoleCommand(&draw_flags, "draw_flags", "set which elements of game draw (draw_flags int)", F_ANYWHERE | F_CHEAT | F_WORLD_MAP));
	AddCommand(ConsoleCommand(&draw_hitbox, "draw_hitbox", "draw weapons hitbox (draw_hitbox 0/1)", F_ANYWHERE | F_CHEAT | F_WORLD_MAP));
	AddCommand(ConsoleCommand(&draw_particle_sphere, "draw_particle_sphere", "draw particle extents sphere (draw_particle_sphere 0/1)", F_ANYWHERE | F_CHEAT | F_WORLD_MAP));
	AddCommand(ConsoleCommand(&draw_phy, "draw_phy", "draw physical colliders (draw_phy 0/1)", F_ANYWHERE | F_CHEAT | F_WORLD_MAP));
	AddCommand(ConsoleCommand(&draw_unit_radius, "draw_unit_radius", "draw units radius (draw_unit_radius 0/1)", F_ANYWHERE | F_CHEAT | F_WORLD_MAP));
	AddCommand(ConsoleCommand(&grass_range, "grass_range", "grass draw range", F_ANYWHERE | F_WORLD_MAP, 0.f));
	AddCommand(ConsoleCommand(&mp_interp, "mp_interp", "interpolation interval (mp_interp 0.f-1.f)", F_MULTIPLAYER | F_WORLD_MAP | F_MP_VAR, 0.f, 1.f));
	AddCommand(ConsoleCommand(&mp_use_interp, "mp_use_interp", "set use of interpolation (mp_use_interp 0/1)", F_MULTIPLAYER | F_WORLD_MAP | F_MP_VAR));
	AddCommand(ConsoleCommand(&next_seed, "next_seed", "random seed used in next map generation", F_ANYWHERE | F_CHEAT | F_WORLD_MAP));
	AddCommand(ConsoleCommand(&profiler_mode, "profiler", "profiler execution: 0-disabled, 1-update, 2-rendering", F_ANYWHERE | F_WORLD_MAP, 0, 2));
	AddCommand(ConsoleCommand(&shader_version, "shader_version", "force shader version (shader_version 2/3)", F_ANYWHERE | F_WORLD_MAP, 2, 3, VoidF(this, &Game::ShaderVersionChanged)));
	AddCommand(ConsoleCommand(&speed, "speed", "game speed (speed 0-10)", F_CHEAT | F_WORLD_MAP | F_SINGLEPLAYER, 0.f, 10.f));
	AddCommand(ConsoleCommand(&uv_mod, "uv_mod", "terrain uv mod (uv_mod 1-256)", F_ANYWHERE, 1, 256, VoidF(this, &Game::UvModChanged)));

	AddCommand(ConsoleCommand(&Game::CmdAddGold, "addgold", "give gold to player (addgold count)", F_GAME | F_CHEAT | F_WORLD_MAP));
	AddCommand(ConsoleCommand(&Game::CmdAddGoldTeam, "addgold_team", "give gold to team (addgold_team count)", F_GAME | F_CHEAT | F_WORLD_MAP));
	AddCommand(ConsoleCommand(&Game::CmdAddItem, "additem", "add item to player inventory (additem id [count])", F_GAME | F_CHEAT));
	AddCommand(ConsoleCommand(&Game::CmdAddTeam, "addteam", "add team item to player inventory (addteam id [count])", F_GAME | F_CHEAT));
	AddCommand(ConsoleCommand(&Game::CmdBreakAction, "break_action", "break unit current action ('break 1' targets self)", F_GAME | F_CHEAT));
	AddCommand(ConsoleCommand(&Game::CmdCitizen, "citizen", "citizens/crazies don't attack player or his team", F_GAME | F_CHEAT | F_WORLD_MAP));
	AddCommand(ConsoleCommand(&Game::CmdClear, "clear", "clear text", F_ANYWHERE | F_WORLD_MAP));
	AddCommand(ConsoleCommand(&Game::CmdCmds, "cmds", "show commands and write them to file commands.txt, with all show unavailable commands too (cmds [all])", F_ANYWHERE));
	AddCommand(ConsoleCommand(&Game::CmdCrash, "crash", "crash game to death!", F_ANYWHERE | F_WORLD_MAP | F_CHEAT));
	AddCommand(ConsoleCommand(&Game::CmdExit, "exit", "exit to menu", F_NOT_MENU | F_WORLD_MAP));
	AddCommand(ConsoleCommand(&Game::CmdFall, "fall", "unit fall on ground for some time ('fall 1' targets self)", F_GAME | F_CHEAT));
	AddCommand(ConsoleCommand(&Game::CmdGodmode, "godmode", "player can't be killed (godmode 0/1)", F_ANYWHERE | F_CHEAT | F_NO_ECHO));
	AddCommand(ConsoleCommand(&Game::CmdGotoMap, "goto_map", "transport player to world map", F_GAME | F_CHEAT));
	AddCommand(ConsoleCommand(&Game::CmdHeal, "heal", "heal player", F_GAME | F_CHEAT));
	AddCommand(ConsoleCommand(&Game::CmdHealUnit, "healunit", "heal unit in front of player", F_GAME | F_CHEAT));
	AddCommand(ConsoleCommand(&Game::CmdHelp, "help", "display information about command (help [command])", F_ANYWHERE));
	AddCommand(ConsoleCommand(&Game::CmdHurt, "hurt", "deal 100 damage to unit ('hurt 1' targets self)", F_GAME | F_CHEAT));
	AddCommand(ConsoleCommand(&Game::CmdInvisible, "invisible", "ai can't see player (invisible 0/1)", F_GAME | F_CHEAT | F_NO_ECHO));
	AddCommand(ConsoleCommand(&Game::CmdKick, "kick", "kick player from server (kick nick)", F_LOBBY | F_MULTIPLAYER | F_SERVER | F_WORLD_MAP));
	AddCommand(ConsoleCommand(&Game::CmdKill, "kill", "kill unit in front of player", F_GAME | F_CHEAT));
	AddCommand(ConsoleCommand(&Game::CmdKillAll, "killall", "kills all enemy units in current level, with 1 it kills allies too, ignore unit in front of player (killall [0/1])", F_GAME | F_CHEAT));
	AddCommand(ConsoleCommand(&Game::CmdLeader, "leader", "change team leader (leader nick)", F_LOBBY | F_MULTIPLAYER));
	AddCommand(ConsoleCommand(&Game::CmdList, "list", "display list of items/units sorted by id/name, unit item unitn itemn (list type [filter])", F_ANYWHERE));
	AddCommand(ConsoleCommand(&Game::CmdLoad, "load", "load game (load 1-10)", F_GAME | F_MENU | F_SERVER));
	AddCommand(ConsoleCommand(&Game::CmdMap2Console, "map2console", "draw dungeon map in console", F_GAME | F_CHEAT));
	AddCommand(ConsoleCommand(&Game::CmdModStat, "modstat", "modify player statistics, use ? to get list (modstat stat value)", F_GAME | F_CHEAT));
	AddCommand(ConsoleCommand(&Game::CmdMultisampling, "multisampling", "sets multisampling (multisampling type [quality])", F_ANYWHERE | F_WORLD_MAP | F_NO_ECHO));
	AddCommand(ConsoleCommand(&Game::CmdNoAi, "noai", "disable ai (noai 0/1)", F_CHEAT | F_GAME | F_WORLD_MAP | F_NO_ECHO));
	AddCommand(ConsoleCommand(&Game::CmdNoClip, "noclip", "turn off player collisions (noclip 0/1)", F_GAME | F_CHEAT | F_NO_ECHO));
	AddCommand(ConsoleCommand(&Game::CmdPause, "pause", "pause/unpause", F_GAME | F_SERVER));
	AddCommand(ConsoleCommand(&Game::CmdPlayerDevmode, "player_devmode", "get/set player developer mode in multiplayer (player_devmode nick/all [0/1])", F_MULTIPLAYER | F_WORLD_MAP | F_CHEAT | F_SERVER));
	AddCommand(ConsoleCommand(&Game::CmdQs, "qs", "pick random character, get ready and start game", F_LOBBY));
	AddCommand(ConsoleCommand(&Game::CmdQuickLoad, "quickload", "load game from last slot", F_SINGLEPLAYER | F_WORLD_MAP));
	AddCommand(ConsoleCommand(&Game::CmdQuickSave, "quicksave", "save game on last slot", F_GAME | F_WORLD_MAP));
	AddCommand(ConsoleCommand(&Game::CmdQuit, "quit", "quit from game", F_ANYWHERE));
	AddCommand(ConsoleCommand(&Game::CmdRandom, "random", "roll random number 1-100 or pick random character (random, random [name], use ? to get list)", F_ANYWHERE));
	AddCommand(ConsoleCommand(&Game::CmdReady, "ready", "set player as ready/unready", F_LOBBY));
	AddCommand(ConsoleCommand(&Game::CmdReloadShaders, "reload_shaders", "reload shaders", F_ANYWHERE | F_WORLD_MAP));
	AddCommand(ConsoleCommand(&Game::CmdResolution, "resolution", "show or change display resolution (resolution [w h hz])", F_ANYWHERE | F_WORLD_MAP));
	AddCommand(ConsoleCommand(&Game::CmdReveal, "reveal", "reveal all locations on world map", F_GAME | F_CHEAT | F_WORLD_MAP));
	AddCommand(ConsoleCommand(&Game::CmdSave, "save", "save game (save 1-10 [text])", F_GAME | F_SERVER));
	AddCommand(ConsoleCommand(&Game::CmdSay, "s", "send message to all players, short from say (say msg)", F_LOBBY | F_MULTIPLAYER | F_WORLD_MAP | F_NO_ECHO));
	AddCommand(ConsoleCommand(&Game::CmdSay, "say", "send message to all players (say msg)", F_LOBBY | F_MULTIPLAYER | F_WORLD_MAP | F_NO_ECHO));
	AddCommand(ConsoleCommand(&Game::CmdScare, "scare", "enemies escape", F_GAME | F_CHEAT));
	AddCommand(ConsoleCommand(&Game::CmdScreenshot, "screenshot", "save screenshot", F_ANYWHERE));
	AddCommand(ConsoleCommand(&Game::CmdServerSay, "server", "send message from server to all players (server msg)", F_LOBBY | F_MULTIPLAYER | F_SERVER | F_WORLD_MAP | F_NO_ECHO));
	AddCommand(ConsoleCommand(&Game::CmdSetSeed, "set_seed", "set randomness seed", F_ANYWHERE | F_WORLD_MAP | F_CHEAT));
	AddCommand(ConsoleCommand(&Game::CmdSetStat, "setstat", "set player statistics, use ? to get list (setstat stat value)", F_GAME | F_CHEAT));
	AddCommand(ConsoleCommand(&Game::CmdShowMinimap, "show_minimap", "reveal minimap", F_GAME | F_CHEAT));
	AddCommand(ConsoleCommand(&Game::CmdSkipDays, "skip_days", "skip days [skip_days [count])", F_GAME | F_CHEAT));
	AddCommand(ConsoleCommand(&Game::CmdSpawnUnit, "spawnunit", "create unit in front of player (spawnunit id [level count arena])", F_GAME | F_CHEAT));
	AddCommand(ConsoleCommand(&Game::CmdStart, "start", "start server", F_LOBBY));
	AddCommand(ConsoleCommand(&Game::CmdSuicide, "suicide", "kill player", F_GAME | F_CHEAT));
	AddCommand(ConsoleCommand(&Game::CmdTileInfo, "tile_info", "display info about map tile", F_GAME | F_CHEAT));
	AddCommand(ConsoleCommand(&Game::CmdVersion, "version", "displays game version", F_ANYWHERE));
	AddCommand(ConsoleCommand(&Game::CmdWarp, "warp", "move player into building (warp inn/arena/soltys/townhall)", F_CHEAT | F_GAME));
	AddCommand(ConsoleCommand(&Game::CmdWhisper, "w", "send private message to player, short from whisper (w nick msg)", F_LOBBY | F_MULTIPLAYER | F_WORLD_MAP | F_NO_ECHO));
	AddCommand(ConsoleCommand(&Game::CmdWhisper, "whisper", "send private message to player (whisper nick msg)", F_LOBBY | F_MULTIPLAYER | F_WORLD_MAP | F_NO_ECHO));
}

//=================================================================================================
void Game::ParseCommand(const string& _str, PrintMsgFunc _print_func, PARSE_SOURCE source)
{
	if(!_print_func)
		print_func = PrintMsgFunc(this, &Game::AddConsoleMsg);
	else
		print_func = _print_func;
	parse_source = source;
	cmd_t.FromString(_str);

	try
	{
		if(!cmd_t.Next())
			return;
		const string& token = cmd_t.MustGetItem();

		auto it = cmds.find(token.c_str());
		if(it == cmds.end())
		{
			MSG(Format("Unknown command '%s'!", token.c_str()));
			return;
		}

		ConsoleCommand& cmd = it->second;

		if(IS_SET(cmd.flags, F_CHEAT) && !devmode)
		{
			MSG(Format("You can't use command '%s' without devmode.", token.c_str()));
			return;
		}

		if(!IS_ALL_SET(cmd.flags, F_ANYWHERE))
		{
			if(server_panel->visible)
			{
				if(!IS_SET(cmd.flags, F_LOBBY))
				{
					MSG(Format("You can't use command '%s' in server lobby.", token.c_str()));
					return;
				}
				else if(IS_SET(cmd.flags, F_SERVER) && !sv_server)
				{
					MSG(Format("Only server can use command '%s'.", token.c_str()));
					return;
				}
			}
			else if(game_state == GS_MAIN_MENU)
			{
				if(!IS_SET(cmd.flags, F_MENU))
				{
					MSG(Format("You can't use command '%s' in menu.", token.c_str()));
					return;
				}
			}
			else if(sv_online)
			{
				if(!IS_SET(cmd.flags, F_MULTIPLAYER))
				{
					MSG(Format("You can't use command '%s' in multiplayer.", token.c_str()));
					return;
				}
				else if(IS_SET(cmd.flags, F_SERVER) && !sv_server)
				{
					MSG(Format("Only server can use command '%s'.", token.c_str()));
					return;
				}
			}
			else if(game_state == GS_WORLDMAP)
			{
				if(!IS_SET(cmd.flags, F_WORLD_MAP))
				{
					MSG(Format("You can't use command '%s' on world map.", token.c_str()));
					return;
				}
			}
			else
			{
				if(!IS_SET(cmd.flags, F_SINGLEPLAYER))
				{
					MSG(Format("You can't use command '%s' in singleplayer.", token.c_str()));
					return;
				}
			}
		}

		switch(cmd.type)
		{
		case ConsoleCommand::VAR_BOOL:
			if(cmd_t.Next())
			{
				if(IS_SET(cmd.flags, F_MP_VAR) && !IsServer())
				{
					MSG("Only server can change this variable.");
					return;
				}

				bool value = cmd_t.MustGetBool();
				bool& var = cmd.Get<bool>();

				if(value != var)
				{
					var = value;
					if(IS_SET(cmd.flags, F_MP_VAR))
						PushNetChange(NetChange::CHANGE_MP_VARS);
					if(cmd.changed)
						cmd.changed();
				}
			}
			MSG(Format("%s = %d", cmd.name, *(bool*)cmd.var ? 1 : 0));
			break;
		case ConsoleCommand::VAR_FLOAT:
			{
				float& f = cmd.Get<float>();

				if(cmd_t.Next())
				{
					if(IS_SET(cmd.flags, F_MP_VAR) && !IsServer())
					{
						MSG("Only server can change this variable.");
						return;
					}

					float f2 = clamp(cmd_t.MustGetNumberFloat(), cmd._float.x, cmd._float.y);
					if(!equal(f, f2))
					{
						f = f2;
						if(IS_SET(cmd.flags, F_MP_VAR))
							PushNetChange(NetChange::CHANGE_MP_VARS);
					}
				}
				MSG(Format("%s = %g", cmd.name, f));
			}
			break;
		case ConsoleCommand::VAR_INT:
			{
				int& i = cmd.Get<int>();

				if(cmd_t.Next())
				{
					if(IS_SET(cmd.flags, F_MP_VAR) && !IsServer())
					{
						MSG("Only server can change this variable.");
						return;
					}

					int i2 = clamp(cmd_t.MustGetInt(), cmd._int.x, cmd._int.y);
					if(i != i2)
					{
						i = i2;
						if(IS_SET(cmd.flags, F_MP_VAR))
							PushNetChange(NetChange::CHANGE_MP_VARS);
						if(cmd.changed)
							cmd.changed();
					}
				}
				MSG(Format("%s = %d", cmd.name, i));
			}
			break;
		case ConsoleCommand::VAR_UINT:
			{
				uint& u = cmd.Get<uint>();

				if(cmd_t.Next())
				{
					if(IS_SET(cmd.flags, F_MP_VAR) && !IsServer())
					{
						MSG("Only server can change this variable.");
						return;
					}

					uint u2 = clamp(cmd_t.MustGetUint(), cmd._uint.x, cmd._uint.y);
					if(u != u2)
					{
						u = u2;
						if(IS_SET(cmd.flags, F_MP_VAR))
							PushNetChange(NetChange::CHANGE_MP_VARS);
					}
				}
				MSG(Format("%s = %u", cmd.name, u));
			}
			break;
		case ConsoleCommand::VAR_NONE:
			if(!IS_SET(cmd.flags, F_NO_ECHO))
				MSG(_str.c_str());
			(this->*cmd.func_ptr)();
			break;
		default:
			assert(0);
			break;
		}
	}
	catch(const Tokenizer::Exception& e)
	{
		MSG(Format("Failed to parse command: %s", e.str->c_str()));
	}
}

//=================================================================================================
void Game::ShaderVersionChanged()
{
	ReloadShaders();
}

//=================================================================================================
void Game::CmdAddGold()
{
	if(cmd_t.Next())
	{
		int ile = cmd_t.MustGetInt();
		if(IsLocal())
			pc->unit->gold = max(pc->unit->gold + ile, 0);
		else
		{
			NetChange& c = Add1(net_changes);
			c.type = NetChange::CHEAT_ADDGOLD;
			c.id = ile;
		}
	}
	else
		MSG("You need to enter gold amount!");
}

//=================================================================================================
void Game::CmdAddGoldTeam()
{
	if(cmd_t.Next())
	{
		int ile = cmd_t.MustGetInt();
		if(ile <= 0)
			MSG("Gold count must by positive!");
		else
		{
			if(IsLocal())
				AddGold(ile);
			else
			{
				NetChange& c = Add1(net_changes);
				c.type = NetChange::CHEAT_ADDGOLD_TEAM;
				c.id = ile;
			}
		}
	}
	else
		MSG("You need to enter gold amount!");
}

//=================================================================================================
void Game::CmdAddItem()
{
	if(cmd_t.Next())
	{
		const string& item_name = cmd_t.MustGetItem();
		const Item* item = FindItem(item_name.c_str());
		if(!item || IS_SET(item->flags, ITEM_SECRET))
			MSG(Format("Can't find item with id '%s'!", item_name.c_str()));
		else
		{
			int ile;
			if(cmd_t.Next())
			{
				ile = cmd_t.MustGetInt();
				if(ile < 1)
					ile = 1;
			}
			else
				ile = 1;

			if(IsLocal())
				AddItem(*pc->unit, item, ile, false);
			else
			{
				NetChange& c = Add1(net_changes);
				c.type = NetChange::CHEAT_ADDITEM;
				c.base_item = item;
				c.ile = ile;
				c.id = 0;
			}
		}
	}
	else
		MSG("You need to enter item id!");
}

//=================================================================================================
void Game::CmdAddTeam()
{
	if(cmd_t.Next())
	{
		const string& item_name = cmd_t.MustGetItem();
		const Item* item = FindItem(item_name.c_str());
		if(!item || IS_SET(item->flags, ITEM_SECRET))
			MSG(Format("Can't find item with id '%s'!", item_name.c_str()));
		else
		{
			int ile;
			if(cmd_t.Next())
			{
				ile = cmd_t.MustGetInt();
				if(ile < 1)
					ile = 1;
			}
			else
				ile = 1;

			if(IsLocal())
				AddItem(*pc->unit, item, ile);
			else
			{
				NetChange& c = Add1(net_changes);
				c.type = NetChange::CHEAT_ADDITEM;
				c.base_item = item;
				c.ile = ile;
				c.id = 1;
			}
		}
	}
	else
		MSG("You need to enter item id!");
}

//=================================================================================================
void Game::CmdBreakAction()
{
	TargetCommand(TargetCmd::BreakAction);
}

//=================================================================================================
void Game::CmdCitizen()
{
	if(bandyta || atak_szalencow)
	{
		if(IsLocal())
		{
			bandyta = false;
			atak_szalencow = false;
			if(IsOnline())
				PushNetChange(NetChange::CHANGE_FLAGS);
		}
		else
			PushNetChange(NetChange::CHEAT_CITIZEN);
	}
}

//=================================================================================================
void Game::CmdClear()
{
	switch(parse_source)
	{
	case PS_CONSOLE:
		console->itb.Reset();
		break;
	case PS_CHAT:
		game_gui->mp_box->itb.Reset();
		break;
	case PS_LOBBY:
		server_panel->itb.Reset();
		break;
	case PS_UNKNOWN:
	default:
		MSG("Unknown parse source.");
		break;
	}
}

//=================================================================================================
void Game::CmdCmds()
{
	bool all = (cmd_t.Next() && cmd_t.GetItem() == "all");
	vector<const ConsoleCommand*> cmds2;

	if(all)
	{
		for(const auto& it : cmds)
			cmds2.push_back(&it.second);
	}
	else
	{
		for(const auto& it : cmds)
		{
			const ConsoleCommand& cmd = it.second;

			bool ok = true;
			if(IS_SET(cmd.flags, F_CHEAT) && !devmode)
				ok = false;

			if(!IS_ALL_SET(cmd.flags, F_ANYWHERE))
			{
				if(server_panel->visible)
				{
					if(!IS_SET(cmd.flags, F_LOBBY))
						ok = false;
					else if(IS_SET(cmd.flags, F_SERVER) && !sv_server)
						ok = false;
				}
				else if(game_state == GS_MAIN_MENU)
				{
					if(!IS_SET(cmd.flags, F_MENU))
						ok = false;
				}
				else if(sv_online)
				{
					if(!IS_SET(cmd.flags, F_MULTIPLAYER))
						ok = false;
					else if(IS_SET(cmd.flags, F_SERVER) && !sv_server)
						ok = false;
				}
				else if(game_state == GS_WORLDMAP)
				{
					if(!IS_SET(cmd.flags, F_WORLD_MAP))
						ok = false;
				}
				else
				{
					if(!IS_SET(cmd.flags, F_SINGLEPLAYER))
						ok = false;
				}
			}

			if(ok)
				cmds2.push_back(&cmd);
		}
	}

	std::sort(cmds2.begin(), cmds2.end(), ConsoleCommand::SortByName);

	string s = "Available commands:\n";
	MSG("Available commands:");

	for each(const ConsoleCommand* cmd in cmds2)
	{
		cstring str = Format("%s - %s.", cmd->name, cmd->desc);
		MSG(str);
		s += str;
		s += "\n";
	}

	HANDLE file = CreateFile("commands.txt", GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if(file)
	{
		WriteFile(file, s.c_str(), s.length(), &tmp, nullptr);
		CloseHandle(file);
	}
}

//=================================================================================================
void Game::CmdCrash()
{
	void DoCrash();
	DoCrash();
}

//=================================================================================================
void Game::CmdExit()
{
	ExitToMenu();
}

//=================================================================================================
void Game::CmdFall()
{
	TargetCommand(TargetCmd::Fall);
}

//=================================================================================================
void Game::CmdGodmode()
{
	if(cmd_t.Next())
	{
		if(!cmd_t.MustGetBool())
		{
			pc->godmode = false;
			if(!IsLocal())
			{
				godmode = false;
				NetChange& c = Add1(net_changes);
				c.type = NetChange::CHEAT_GODMODE;
				c.id = 0;
			}
		}
		else
		{
			pc->godmode = true;
			if(!IsLocal())
			{
				godmode = true;
				NetChange& c = Add1(net_changes);
				c.type = NetChange::CHEAT_GODMODE;
				c.id = 1;
			}
		}
	}
	MSG(Format("godmode = %d", pc->godmode ? 1 : 0));
}

//=================================================================================================
void Game::CmdGotoMap()
{
	if(IsLocal())
		ExitToMap();
	else
		PushNetChange(NetChange::CHEAT_GOTO_MAP);
}

//=================================================================================================
void Game::CmdHeal()
{
	if(IsLocal())
	{
		pc->unit->hp = pc->unit->hpmax;
		pc->unit->HealPoison();
		if(IsOnline())
		{
			NetChange& c = Add1(net_changes);
			c.type = NetChange::UPDATE_HP;
			c.unit = pc->unit;
		}
	}
	else
		PushNetChange(NetChange::CHEAT_HEAL);
}

//=================================================================================================
void Game::CmdHealUnit()
{
	if(selected_target)
	{
		if(IsLocal())
		{
			selected_target->hp = selected_target->hpmax;
			selected_target->HealPoison();
			if(IsOnline())
			{
				NetChange& c = Add1(net_changes);
				c.type = NetChange::UPDATE_HP;
				c.unit = selected_target;
			}
		}
		else
		{
			NetChange& c = Add1(net_changes);
			c.type = NetChange::CHEAT_HEALUNIT;
			c.unit = selected_target;
		}
	}
	else
		MSG("No unit selected.");
}

//=================================================================================================
void Game::CmdHelp()
{
	if(cmd_t.Next())
	{
		const string& name = cmd_t.MustGetItem();
		auto it = cmds.find(name.c_str());
		if(it == cmds.end())
			MSG(Format("Unknown command '%s'!", name.c_str()));
		else
			MSG(Format("%s - %s.", name.c_str(), it->second.desc));		
	}
	else
		MSG("Enter 'cmds' or 'cmds all' to show list of commands. To get information about single command enter \"help CMD\". To get ID of item/unit use command \"list\".");
}

//=================================================================================================
void Game::CmdHurt()
{
	TargetCommand(TargetCmd::Hurt);
}

//=================================================================================================
void Game::CmdInvisible()
{
	if(cmd_t.Next())
	{
		if(!cmd_t.MustGetBool())
		{
			pc->unit->invisible = false;
			if(!IsLocal())
			{
				invisible = false;
				NetChange& c = Add1(net_changes);
				c.type = NetChange::CHEAT_INVISIBLE;
				c.id = 0;
			}
		}
		else
		{
			pc->unit->invisible = true;
			if(!IsLocal())
			{
				invisible = true;
				NetChange& c = Add1(net_changes);
				c.type = NetChange::CHEAT_INVISIBLE;
				c.id = 1;
			}
		}
	}
	MSG(Format("invisible = %d", pc->unit->invisible ? 1 : 0));
}

//=================================================================================================
void Game::CmdKick()
{
	if(cmd_t.Next())
	{
		const string& player_name = cmd_t.MustGetItem();
		int index = FindPlayerIndex(player_name.c_str(), true);
		if(index == -1)
			MSG(Format("No player with nick '%s'.", player_name.c_str()));
		else
			KickPlayer(index);
	}
	else
		MSG("You need to enter player nick.");
}

//=================================================================================================
void Game::CmdKill()
{
	if(selected_target)
	{
		if(IsLocal())
			GiveDmg(GetContext(*pc->unit), nullptr, selected_target->hpmax, *selected_target);
		else
		{
			NetChange& c = Add1(net_changes);
			c.type = NetChange::CHEAT_KILL;
			c.unit = selected_target;
		}
	}
	else
		MSG("No unit selected.");
}

//=================================================================================================
void Game::CmdKillAll()
{
	int typ = 0;
	if(cmd_t.Next())
		typ = cmd_t.MustGetInt();
	Unit* ignore = nullptr;
	if(before_player == BP_UNIT)
		ignore = before_player_ptr.unit;
	if(!Cheat_KillAll(typ, *pc->unit, ignore))
		MSG(Format("Unknown parameter '%d'.", typ));
}

//=================================================================================================
void Game::CmdLeader()
{
	if(cmd_t.Next())
	{
		const string& player_name = cmd_t.MustGetItem();
		int index = FindPlayerIndex(player_name.c_str(), true);
		if(index == -1)
			MSG(Format("No player with nick '%s'.", player_name.c_str()));
		else
		{
			PlayerInfo& info = game_players[index];
			if(leader_id == info.id)
				MSG(Format("Player '%s' is already a leader.", player_name.c_str()));
			else if(sv_server || leader_id == my_id)
			{
				if(server_panel->visible)
				{
					if(sv_server)
					{
						leader_id = info.id;
						AddLobbyUpdate(INT2(Lobby_ChangeLeader, 0));
					}
					else
						MSG("You can't change a leader.");
				}
				else
				{
					NetChange& c = Add1(net_changes);
					c.type = NetChange::CHANGE_LEADER;
					c.id = info.id;

					if(sv_server)
					{
						leader_id = info.id;
						leader = info.u;

						if(dialog_enc)
							dialog_enc->bts[0].state = (IsLeader() ? Button::NONE : Button::DISABLED);
					}
				}

				AddMsg(Format("Leader changed to '%s'.", info.name.c_str()));
			}
			else
				MSG("You can't change a leader.");
		}
	}
	else
		MSG("You need to enter leader nick.");
}

//=================================================================================================
void Game::CmdList()
{
	static auto sortItemsById = [](const Item* item1, const Item* item2){ return item1->id < item2->id; };

	if(cmd_t.Next())
	{
		int co;

		{
			const string& lis = cmd_t.MustGetItem();
			if(lis == "item" || lis == "items")
				co = 0;
			else if(lis == "itemn" || lis == "item_names")
				co = 1;
			else if(lis == "unit" || lis == "units")
				co = 2;
			else if(lis == "unitn" || lis == "unit_names")
				co = 3;
			else
			{
				MSG(Format("Unknown list type '%s'!", lis.c_str()));
				return;
			}
		}

		if(cmd_t.Next())
		{
			const string& reg = cmd_t.MustGetItem();
			if(co == 0 || co == 1)
			{
				LocalVector2<const Item*> items;

				if(co == 0)
				{
					for(auto it : g_items)
					{
						const Item* item = it.second;
						if(_strnicmp(reg.c_str(), item->id.c_str(), reg.length()) == 0)
							items.push_back(item);
					}
				}
				else
				{
					for(auto it : g_items)
					{
						const Item* item = it.second;
						if(_strnicmp(reg.c_str(), item->name.c_str(), reg.length()) == 0)
							items.push_back(item);
					}
				}

				if(items.empty())
				{
					MSG(Format("No items found starting with '%s'.", reg.c_str()));
					return;
				}

				std::sort(items.begin(), items.end(), (co == 0 ? Item::SortById : Item::SortByName));

				string s = Format("Items list (%d):\n", items.size());
				MSG(Format("Items list (%d):", items.size()));

				if(co == 0)
				{
					for each(const Item* item in items)
					{
						if(IS_SET(item->flags, ITEM_SECRET))
							continue;
						cstring s2 = Format("%s (%s)", item->id.c_str(), item->name.c_str());
						MSG(s2);
						s += s2;
						s += "\n";
					}
				}
				else
				{
					for each(const Item* item in items)
					{
						if(IS_SET(item->flags, ITEM_SECRET))
							continue;
						cstring s2 = Format("%s (%s)", item->name.c_str(), item->id.c_str());
						MSG(s2);
						s += s2;
						s += "\n";
					}
				}

				LOG(s.c_str());
			}
			else
			{
				LocalVector2<const UnitData*> unitsd;

				if(co == 2)
				{
					for(UnitData* ud : unit_datas)
					{
						if(_strnicmp(reg.c_str(), ud->id.c_str(), reg.length()) == 0)
							unitsd.push_back(ud);
					}
				}
				else
				{
					for(UnitData* ud : unit_datas)
					{
						if(_strnicmp(reg.c_str(), ud->name.c_str(), reg.length()) == 0)
							unitsd.push_back(ud);
					}
				}

				if(unitsd.empty())
				{
					MSG(Format("No units found starting with '%s'.", reg.c_str()));
					return;
				}

				std::sort(unitsd.begin(), unitsd.end(), (co == 2 ? UnitData::SortById : UnitData::SortByName));

				string s = Format("Units list (%d):\n", unitsd.size());
				MSG(Format("Units list (%d):", unitsd.size()));

				if(co == 2)
				{
					for each(const UnitData* u in unitsd)
					{
						if(IS_SET(u->flags, F_SECRET))
							continue;
						cstring s2 = Format("%s (%s)", u->id.c_str(), u->name.c_str());
						MSG(s2);
						s += s2;
						s += "\n";
					}
				}
				else
				{
					for each(const UnitData* u in unitsd)
					{
						if(IS_SET(u->flags, F_SECRET))
							continue;
						cstring s2 = Format("%s (%s)", u->name.c_str(), u->id.c_str());
						MSG(s2);
						s += s2;
						s += "\n";
					}
				}

				LOG(s.c_str());
			}
		}
		else
		{
			if(co == 0 || co == 1)
			{
				LocalVector2<const Item*> items;

				for(auto it : g_items)
					items.push_back(it.second);

				std::sort(items.begin(), items.end(), (co == 0 ? Item::SortById : Item::SortByName));

				string s = Format("Items list (%d):\n", items.size());
				MSG(Format("Items list (%d):", items.size()));

				if(co == 0)
				{
					for each(const Item* item in items)
					{
						if(IS_SET(item->flags, ITEM_SECRET))
							continue;
						cstring s2 = Format("%s (%s)", item->id.c_str(), item->name.c_str());
						MSG(s2);
						s += s2;
						s += "\n";
					}
				}
				else
				{
					for each(const Item* item in items)
					{
						if(IS_SET(item->flags, ITEM_SECRET))
							continue;
						cstring s2 = Format("%s (%s)", item->name.c_str(), item->id.c_str());
						MSG(s2);
						s += s2;
						s += "\n";
					}
				}

				LOG(s.c_str());
			}
			else
			{
				LocalVector2<const UnitData*> unitsd;

				for(UnitData* ud : unit_datas)
					unitsd.push_back(ud);

				std::sort(unitsd.begin(), unitsd.end(), (co == 2 ? UnitData::SortById : UnitData::SortByName));

				string s = Format("Units list (%d):\n", unitsd.size());
				MSG(Format("Units list (%d):", unitsd.size()));

				if(co == 2)
				{
					for each(const UnitData* u in unitsd)
					{
						if(IS_SET(u->flags, F_SECRET))
							continue;
						cstring s2 = Format("%s (%s)", u->id.c_str(), u->name.c_str());
						MSG(s2);
						s += s2;
						s += "\n";
					}
				}
				else
				{
					for each(const UnitData* u in unitsd)
					{
						if(IS_SET(u->flags, F_SECRET))
							continue;
						cstring s2 = Format("%s (%s)", u->name.c_str(), u->id.c_str());
						MSG(s2);
						s += s2;
						s += "\n";
					}
				}

				LOG(s.c_str());
			}
		}
	}
	else
	{
		MSG("Display list of items/units (item/items, itemn/item_names, unit/units, unitn/unit_names). Examples:");
		MSG("'list item' - list of items ordered by id");
		MSG("'list itemn' - list of items ordered by name");
		MSG("'list unit cmd_t' - list of units ordered by id starting from cmd_t");
	}
}

//=================================================================================================
void Game::CmdLoad()
{
	if(CanLoadGame())
	{
		sv_online = false;
		sv_server = false;
		int slot = 1;
		if(cmd_t.Next())
			slot = clamp(cmd_t.MustGetInt(), 1, 10);
		try
		{
			LoadGameSlot(slot);
		}
		catch(cstring err)
		{
			ERROR(err);
			MSG(Format("Failed to load game: %s", err));
		}
	}
	else
		MSG("You can't load game in this moment.");
}

//=================================================================================================
void Game::CmdMap2Console()
{
	if(game_state == GS_LEVEL && !location->outside)
	{
		InsideLocationLevel& lvl = ((InsideLocation*)location)->GetLevelData();
		rysuj_mape_konsola(lvl.map, lvl.w, lvl.h);
	}
	else
		MSG("You need to be inside dungeon!");
}

//=================================================================================================
void Game::CmdModStat()
{
	SetOrModStat(false);
}

//=================================================================================================
void Game::CmdMultisampling()
{
	if(cmd_t.Next())
	{
		int type = cmd_t.MustGetInt();
		int level = -1;
		if(cmd_t.Next())
			level = cmd_t.MustGetInt();
		int wynik = ChangeMultisampling(type, level);
		if(wynik == 2)
		{
			int ms, msq;
			GetMultisampling(ms, msq);
			MSG(Format("Changed multisampling to %d, %d.", ms, msq));
		}
		else
			MSG(wynik == 1 ? "This mode is already set." : "This mode is unavailable.");
	}
	else
	{
		int ms, msq;
		GetMultisampling(ms, msq);
		MSG(Format("multisampling = %d, %d", ms, msq));
	}
}

//=================================================================================================
void Game::CmdNoAi()
{
	if(cmd_t.Next())
	{
		noai = cmd_t.MustGetBool();
		if(IsOnline())
		{
			NetChange& c = Add1(net_changes);
			c.type = NetChange::CHEAT_NOAI;
			c.id = (noai ? 1 : 0);
		}
	}
	MSG(Format("noai = %d", noai ? 1 : 0));
}

//=================================================================================================
void Game::CmdNoClip()
{
	if(cmd_t.Next())
	{
		if(!cmd_t.MustGetBool())
		{
			pc->noclip = false;
			if(!IsLocal())
			{
				noclip = false;
				NetChange& c = Add1(net_changes);
				c.type = NetChange::CHEAT_NOCLIP;
				c.id = 0;
			}
		}
		else
		{
			pc->noclip = true;
			if(!IsLocal())
			{
				noclip = true;
				NetChange& c = Add1(net_changes);
				c.type = NetChange::CHEAT_NOCLIP;
				c.id = 1;
			}
		}
	}
	MSG(Format("noclip = %d", pc->noclip ? 1 : 0));
}

//=================================================================================================
void Game::CmdPause()
{
	paused = !paused;
	if(IsOnline())
	{
		AddMultiMsg(paused ? txGamePaused : txGameResumed);
		NetChange& c = Add1(net_changes);
		c.type = NetChange::PAUSED;
		c.id = (paused ? 1 : 0);
		if(paused && game_state == GS_WORLDMAP && world_state == WS_TRAVEL)
			PushNetChange(NetChange::UPDATE_MAP_POS);
	}
}

//=================================================================================================
void Game::CmdPlayerDevmode()
{
	if(cmd_t.Next())
	{
		string player_name = cmd_t.MustGetItem();
		if(cmd_t.Next())
		{
			bool b = cmd_t.MustGetBool();
			if(player_name == "all")
			{
				for(PlayerInfo& info : game_players)
				{
					if(!info.left && info.devmode != b && info.id != 0)
					{
						info.devmode = b;
						NetChangePlayer& c = Add1(net_changes_player);
						c.type = NetChangePlayer::DEVMODE;
						c.id = (b ? 1 : 0);
						c.pc = info.u->player;
						info.NeedUpdate();
					}
				}
			}
			else
			{
				int index = FindPlayerIndex(player_name.c_str(), true);
				if(index == -1)
					MSG(Format("No player with nick '%s'.", player_name.c_str()));
				else
				{
					PlayerInfo& info = game_players[index];
					if(info.devmode != b)
					{
						info.devmode = b;
						NetChangePlayer& c = Add1(net_changes_player);
						c.type = NetChangePlayer::DEVMODE;
						c.id = (b ? 1 : 0);
						c.pc = info.u->player;
						info.NeedUpdate();
					}
				}
			}
		}
		else
		{
			if(player_name == "all")
			{
				LocalString s = "Players devmode: ";
				bool any = false;
				for(PlayerInfo& info : game_players)
				{
					if(!info.left && info.id != 0)
					{
						s += Format("%s(%d), ", info.name.c_str(), info.devmode ? 1 : 0);
						any = true;
					}
				}
				if(any)
				{
					s.pop(2);
					s += ".";
					MSG(s->c_str());
				}
				else
					MSG("No players.");
			}
			else
			{
				int index = FindPlayerIndex(player_name.c_str(), true);
				if(index == -1)
					MSG(Format("No player with nick '%s'.", player_name.c_str()));
				else
					MSG(Format("Player devmode: %s(%d).", player_name.c_str(), game_players[index].devmode ? 1 : 0));
			}
		}
	}
	else
		MSG("You need to enter player nick/all.");
}

//=================================================================================================
void Game::CmdQs()
{
	if(IsServer())
	{
		if(!sv_startup)
		{
			PlayerInfo& info = game_players[0];
			if(!info.ready)
			{
				if(info.clas == Class::INVALID)
					server_panel->PickClass(Class::RANDOM, true);
				else
				{
					info.ready = true;
					CheckReady();
				}
			}

			cstring error_text = nullptr;

			for(vector<PlayerInfo>::iterator it = game_players.begin(), end = game_players.end(); it != end; ++it)
			{
				if(!it->ready)
				{
					error_text = "Not everyone is ready.";
					break;
				}
			}

			if(!error_text)
			{
				// rozpocznij odliczanie do startu
				extern const int STARTUP_TIMER;
				sv_startup = true;
				last_startup_id = STARTUP_TIMER;
				startup_timer = float(STARTUP_TIMER);
				packet_data.resize(2);
				packet_data[0] = ID_STARTUP;
				packet_data[1] = (byte)STARTUP_TIMER;
				peer->Send((cstring)&packet_data[0], 2, IMMEDIATE_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
			}
			else
				MSG(error_text);
		}
	}
	else
	{
		PlayerInfo& info = game_players[0];
		if(!info.ready)
		{
			if(info.clas == Class::INVALID)
				server_panel->PickClass(Class::RANDOM, true);
			else
			{
				info.ready = true;
				ChangeReady();
			}
		}
	}
}

//=================================================================================================
void Game::CmdQuickLoad()
{
	Quickload(true);
}

//=================================================================================================
void Game::CmdQuickSave()
{
	Quicksave(true);
}

//=================================================================================================
void Game::CmdQuit()
{
	if(cmd_t.Next() && cmd_t.IsInt() && cmd_t.GetInt() == 1)
		exit(0);
	else
		Quit();
}

//=================================================================================================
void Game::CmdRandom()
{
	if(server_panel->visible)
	{
		if(cmd_t.Next())
		{
			if(cmd_t.IsSymbol('?'))
			{
				LocalVector2<Class> classes;
				for(ClassInfo& ci : g_classes)
				{
					if(ci.pickable)
						classes.push_back(ci.class_id);
				}
				std::sort(classes.begin(), classes.end(),
					[](Class c1, Class c2) -> bool
				{
					return strcmp(g_classes[(int)c1].id, g_classes[(int)c2].id) < 0;
				});
				LocalString str = "List of classes: ";
				Join(classes.Get(), str.get_ref(), ", ",
					[](Class c)
				{
					return g_classes[(int)c].id;
				});
				str += ".";
				MSG(str.c_str());
			}
			else
			{
				const string& clas = cmd_t.MustGetItem();
				ClassInfo* ci = ClassInfo::Find(clas);
				if(ci)
				{
					if(ClassInfo::IsPickable(ci->class_id))
					{
						server_panel->PickClass(ci->class_id, false);
						MSG("You picked random character.");
					}
					else
						MSG(Format("Class '%s' is not pickable by players.", clas.c_str()));
				}
				else
					MSG(Format("Invalid class name '%s'. Use 'random ?' for list of classes.", clas.c_str()));
			}
		}
		else
		{
			server_panel->PickClass(Class::RANDOM, false);
			MSG("You picked random character.");
		}
	}
	else if(sv_online)
	{
		int n = random(1, 100);
		NetChange& c = Add1(net_changes);
		c.type = NetChange::RANDOM_NUMBER;
		c.id = n;
		c.unit = pc->unit;
		AddMsg(Format("You rolled %d.", n));
	}
	else
		MSG(Format("You rolled %d.", random(1, 100)));
}

//=================================================================================================
void Game::CmdReady()
{
	PlayerInfo& info = game_players[0];
	info.ready = !info.ready;
	ChangeReady();
}

//=================================================================================================
void Game::CmdReloadShaders()
{
	ReloadShaders();
}

//=================================================================================================
void Game::CmdResolution()
{
	if(cmd_t.Next())
	{
		int w = cmd_t.MustGetInt(), h = -1, hz = -1;
		bool pick_h = true, pick_hz = true, valid = false;
		if(cmd_t.Next())
		{
			h = cmd_t.MustGetInt();
			pick_h = false;
			if(cmd_t.Next())
			{
				hz = cmd_t.MustGetInt();
				pick_hz = false;
			}
		}
		uint display_modes = d3d->GetAdapterModeCount(used_adapter, DISPLAY_FORMAT);
		for(uint i = 0; i<display_modes; ++i)
		{
			D3DDISPLAYMODE d_mode;
			V(d3d->EnumAdapterModes(used_adapter, DISPLAY_FORMAT, i, &d_mode));
			if(w == d_mode.Width)
			{
				if(pick_h)
				{
					if((int)d_mode.Height >= h)
					{
						h = d_mode.Height;
						if((int)d_mode.RefreshRate > hz)
							hz = d_mode.RefreshRate;
						valid = true;
					}
				}
				else if(h == d_mode.Height)
				{
					if(pick_hz)
					{
						if((int)d_mode.RefreshRate > hz)
							hz = d_mode.RefreshRate;
						valid = true;
					}
					else if(hz == d_mode.RefreshRate)
					{
						valid = true;
						break;
					}
				}
			}
		}
		if(valid)
			ChangeMode(w, h, fullscreen, hz);
		else
			MSG(Format("Can't change resolution to %dx%d (%d Hz).", w, h, hz));
	}
	else
	{
		// wypisz aktualn¹ rozdzielczoœæ i dostêpne
		LocalString s = Format("Current resolution %dx%d (%d Hz). Available: ", wnd_size.x, wnd_size.y, wnd_hz);
		uint display_modes = d3d->GetAdapterModeCount(used_adapter, DISPLAY_FORMAT);
		for(uint i = 0; i<display_modes; ++i)
		{
			D3DDISPLAYMODE d_mode;
			V(d3d->EnumAdapterModes(used_adapter, DISPLAY_FORMAT, i, &d_mode));
			s += Format("%dx%d(%d)", d_mode.Width, d_mode.Height, d_mode.RefreshRate);
			if(i + 1 != display_modes)
				s += ", ";
		}
		MSG(s);
	}
}

//=================================================================================================
void Game::CmdReveal()
{
	if(IsLocal())
		Cheat_Reveal();
	else
		PushNetChange(NetChange::CHEAT_REVEAL);
}

//=================================================================================================
void Game::CmdSave()
{
	if(CanSaveGame())
	{
		int slot = 1;
		LocalString text;
		if(cmd_t.Next())
		{
			slot = clamp(cmd_t.MustGetInt(), 1, 10);
			if(cmd_t.Next())
				text = cmd_t.MustGetString();
		}
		SaveGameSlot(slot, text->c_str());
	}
	else
		MSG("You can't save game in this moment.");
}

//=================================================================================================
void Game::CmdSay()
{
	if(cmd_t.NextLine())
	{
		const string& text = cmd_t.MustGetItem();
		net_stream.Reset();
		net_stream.Write(ID_SAY);
		net_stream.WriteCasted<byte>(my_id);
		WriteString1(net_stream, text);
		peer->Send(&net_stream, MEDIUM_PRIORITY, RELIABLE, 0, sv_server ? UNASSIGNED_SYSTEM_ADDRESS : server, sv_server);
		cstring s = Format("%s: %s", game_players[0].name.c_str(), text.c_str());
		AddMsg(s);
		LOG(s);
	}
	else
		MSG("You need to enter message.");
}

//=================================================================================================
void Game::CmdScare()
{
	if(IsLocal())
	{
		for(vector<AIController*>::iterator it = ais.begin(), end = ais.end(); it != end; ++it)
		{
			if(IsEnemy(*(*it)->unit, *pc->unit) && distance((*it)->unit->pos, pc->unit->pos) < ALERT_RANGE.x && CanSee(*(*it)->unit, *pc->unit))
				(*it)->morale = -10;
		}
	}
	else
		PushNetChange(NetChange::CHEAT_SCARE);
}

//=================================================================================================
void Game::CmdScreenshot()
{
	TakeScreenshot(true);
}

//=================================================================================================
void Game::CmdServerSay()
{
	if(cmd_t.NextLine())
	{
		const string& text = cmd_t.MustGetItem();
		if(players > 1)
		{
			net_stream.Reset();
			net_stream.Write(ID_SERVER_SAY);
			WriteString1(net_stream, text);
			peer->Send(&net_stream, MEDIUM_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
		}
		AddServerMsg(text.c_str());
		LOG(Format("SERWER: %s", text.c_str()));
		if(game_state == GS_LEVEL)
			game_gui->AddSpeechBubble(pc->unit, text.c_str());
	}
	else
		MSG("You need to enter message.");
}

//=================================================================================================
void Game::CmdSetSeed()
{
	cmd_t.Next();
	next_seed = cmd_t.MustGetUint();
	cmd_t.Next();
	next_seed_extra = true;
	next_seed_val[0] = cmd_t.MustGetInt();
	cmd_t.Next();
	next_seed_val[1] = cmd_t.MustGetInt();
	cmd_t.Next();
	next_seed_val[2] = cmd_t.MustGetInt();
}

//=================================================================================================
void Game::CmdSetStat()
{
	SetOrModStat(true);
}

//=================================================================================================
void Game::CmdShowMinimap()
{
	if(IsLocal())
		Cheat_ShowMinimap();
	else
		PushNetChange(NetChange::CHEAT_SHOW_MINIMAP);
}

//=================================================================================================
void Game::CmdSkipDays()
{
	int ile = 1;
	if(cmd_t.Next())
		ile = cmd_t.MustGetInt();
	if(ile > 0)
	{
		if(IsLocal())
			WorldProgress(ile, WPM_SKIP);
		else
		{
			NetChange& c = Add1(net_changes);
			c.type = NetChange::CHEAT_SKIP_DAYS;
			c.id = ile;
		}
	}
}

//=================================================================================================
void Game::CmdSpawnUnit()
{
	if(cmd_t.Next())
	{
		UnitData* data = FindUnitData(cmd_t.MustGetItem().c_str(), false);
		if(!data || IS_SET(data->flags, F_SECRET))
			MSG(Format("Missing base unit '%s'!", cmd_t.GetItem().c_str()));
		else
		{
			int level = -1, ile = 1, in_arena = -1;
			if(cmd_t.Next())
			{
				level = cmd_t.MustGetInt();
				if(cmd_t.Next())
				{
					ile = cmd_t.MustGetInt();
					if(cmd_t.Next())
						in_arena = clamp(cmd_t.MustGetInt(), -1, 1);
				}
			}

			if(IsLocal())
			{
				LevelContext& ctx = GetContext(*pc->unit);

				for(int i = 0; i<ile; ++i)
				{
					Unit* u = SpawnUnitNearLocation(ctx, pc->unit->GetFrontPos(), *data, &pc->unit->pos, level);
					if(!u)
					{
						MSG(Format("No free space for unit '%s'!", data->id.c_str()));
						break;
					}
					else if(in_arena != -1)
					{
						u->in_arena = in_arena;
						at_arena.push_back(u);
					}
					if(IsOnline())
						Net_SpawnUnit(u);
				}
			}
			else
			{
				NetChange& c = Add1(net_changes);
				c.type = NetChange::CHEAT_SPAWN_UNIT;
				c.base_unit = data;
				c.ile = ile;
				c.id = level;
				c.i = in_arena;
			}
		}
	}
	else
		MSG("You need to enter unit id!");
}

//=================================================================================================
void Game::CmdStart()
{
	if(sv_startup)
	{
		MSG("Server is already starting.");
		return;
	}

	cstring error_text = nullptr;

	for(vector<PlayerInfo>::iterator it = game_players.begin(), end = game_players.end(); it != end; ++it)
	{
		if(!it->ready)
		{
			error_text = "Not everyone is ready.";
			return;
		}
	}

	if(!error_text)
	{
		// rozpocznij odliczanie do startu
		extern const int STARTUP_TIMER;
		sv_startup = true;
		last_startup_id = STARTUP_TIMER;
		startup_timer = float(STARTUP_TIMER);
		packet_data.resize(2);
		packet_data[0] = ID_STARTUP;
		packet_data[1] = (byte)STARTUP_TIMER;
		peer->Send((cstring)&packet_data[0], 2, IMMEDIATE_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
		server_panel->bts[4].text = server_panel->txStop;
		cstring s = Format(server_panel->txStartingIn, STARTUP_TIMER);
		AddMsg(s);
		LOG(s);
	}
	else
		MSG(error_text);
}

//=================================================================================================
void Game::CmdSuicide()
{
	if(IsLocal())
		GiveDmg(GetContext(*pc->unit), nullptr, pc->unit->hpmax, *pc->unit);
	else
		PushNetChange(NetChange::CHEAT_SUICIDE);
}

//=================================================================================================
void Game::CmdTileInfo()
{
	if(location->outside && pc->unit->in_building == -1 && terrain->IsInside(pc->unit->pos))
	{
		OutsideLocation* outside = (OutsideLocation*)location;
		const TerrainTile& cmd_t = outside->tiles[pos_to_pt(pc->unit->pos)(outside->size)];
		MSG(cmd_t.GetInfo());
	}
	else
		MSG("You must stand on terrain tile.");
}

//=================================================================================================
void Game::CmdVersion()
{
	MSG(Format("CaRpg version " VERSION_STR ", built %s.", g_ctime.c_str()));
}

//=================================================================================================
void Game::CmdWarp()
{
	if(cmd_t.Next())
	{
		const string& type = cmd_t.MustGetItem();
		BUILDING b;
		if(type == "inn")
			b = B_INN;
		else if(type == "arena")
			b = B_ARENA;
		else if(type == "soltys")
			b = B_VILLAGE_HALL;
		else if(type == "townhall")
			b = B_CITY_HALL;
		else
			b = B_NONE;

		bool ok = false;

		if(city_ctx)
		{
			int id;
			InsideBuilding* building;
			if(b == B_INN)
				building = city_ctx->FindInn(id);
			else
				building = city_ctx->FindInsideBuilding(b, id);
			if(building)
			{
				// wejdŸ do budynku
				if(IsLocal())
				{
					fallback_co = FALLBACK_ENTER;
					fallback_t = -1.f;
					fallback_1 = id;
					pc->unit->frozen = 2;
				}
				else
				{
					NetChange& c = Add1(net_changes);
					c.type = NetChange::CHEAT_WARP;
					c.id = b;
				}
				ok = true;
			}
		}

		if(!ok)
			MSG(Format("Missing building type '%s'!", type.c_str()));
	}
	else
		MSG("You need to enter where.");
}

//=================================================================================================
void Game::CmdWhisper()
{
	if(cmd_t.Next())
	{
		const string& player_nick = cmd_t.MustGetItem();
		int index = FindPlayerIndex(player_nick.c_str(), true);
		if(index == -1)
			MSG(Format("No player with nick '%s'.", player_nick.c_str()));
		else if(cmd_t.NextLine())
		{
			const string& text = cmd_t.MustGetItem();
			PlayerInfo& info = game_players[index];
			if(info.id == my_id)
				MSG(Format("Whispers in your head: %s", text.c_str()));
			else
			{
				net_stream.Reset();
				net_stream.Write(ID_WHISPER);
				net_stream.WriteCasted<byte>(IsServer() ? my_id : info.id);
				WriteString1(net_stream, text);
				peer->Send(&net_stream, MEDIUM_PRIORITY, RELIABLE, 0, sv_server ? info.adr : server, false);
				cstring s = Format("@%s: %s", info.name.c_str(), text.c_str());
				AddMsg(s);
				LOG(s);
			}
		}
		else
			MSG("You need to enter message.");
	}
	else
		MSG("You need to enter player nick.");
}

//=================================================================================================
void Game::SetOrModStat(bool set)
{
	if(!cmd_t.Next())
		MSG("Enter name of attribute/skill and value. Use ? to get list of attributes/skills.");
	else if(cmd_t.IsSymbol('?'))
	{
		LocalVector2<Attribute> attribs;
		for(int i = 0; i<(int)Attribute::MAX; ++i)
			attribs.push_back((Attribute)i);
		std::sort(attribs.begin(), attribs.end(),
			[](Attribute a1, Attribute a2) -> bool
		{
			return strcmp(g_attributes[(int)a1].id, g_attributes[(int)a2].id) < 0;
		});
		LocalVector2<Skill> skills;
		for(int i = 0; i<(int)Skill::MAX; ++i)
			skills.push_back((Skill)i);
		std::sort(skills.begin(), skills.end(),
			[](Skill s1, Skill s2) -> bool
		{
			return strcmp(g_skills[(int)s1].id, g_skills[(int)s2].id) < 0;
		});
		LocalString str = "List of attributes: ";
		Join(attribs.Get(), str.get_ref(), ", ",
			[](Attribute a)
		{
			return g_attributes[(int)a].id;
		});
		str += ".";
		MSG(str.c_str());
		str = "List of skills: ";
		Join(skills.Get(), str.get_ref(), ", ",
			[](Skill s)
		{
			return g_skills[(int)s].id;
		});
		str += ".";
		MSG(str.c_str());
	}
	else
	{
		int co;
		bool skill;
		const string& s = cmd_t.MustGetItem();
		AttributeInfo* ai = AttributeInfo::Find(s);
		if(ai)
		{
			co = (int)ai->attrib_id;
			skill = false;
		}
		else
		{
			SkillInfo* si = SkillInfo::Find(s);
			if(si)
			{
				co = (int)si->skill_id;
				skill = true;
			}
			else
			{
				MSG(Format("Invalid attribute/skill '%s'.", s.c_str()));
				return;
			}
		}

		if(!cmd_t.Next())
			MSG("You need to enter number!");
		else
		{
			int num = cmd_t.MustGetInt();

			if(IsLocal())
			{
				if(skill)
				{
					if(!set)
						num += pc->unit->unmod_stats.skill[co];
					int v = clamp(num, 0, SkillInfo::MAX);
					if(v != pc->unit->unmod_stats.skill[co])
						pc->unit->Set((Skill)co, v);
				}
				else
				{
					if(!set)
						num += pc->unit->unmod_stats.attrib[co];
					int v = clamp(num, 1, AttributeInfo::MAX);
					if(v != pc->unit->unmod_stats.attrib[co])
						pc->unit->Set((Attribute)co, v);
				}
			}
			else
			{
				NetChange& c = Add1(net_changes);
				c.type = (set ? NetChange::CHEAT_SETSTAT : NetChange::CHEAT_MODSTAT);
				c.id = co;
				c.ile = (skill ? 1 : 0);
				c.i = num;
			}
		}
	}
}

//=================================================================================================
void Game::TargetCommand(TargetCmd cmd)
{
	Unit* u;
	if(cmd_t.Next() && cmd_t.GetInt() == 1)
		u = pc->unit;
	else if(selected_unit)
		u = selected_unit;
	else
	{
		MSG("No unit selected.");
		return;
	}

	if(IsLocal())
	{
		switch(cmd)
		{
		case TargetCmd::Hurt:
			GiveDmg(GetContext(*u), nullptr, 100.f, *u);
			break;
		case TargetCmd::BreakAction:
			BreakAction(*u, false, true);
			break;
		case TargetCmd::Fall:
			UnitFall(*u);
			break;
		}
	}
	else
	{
		NetChange& c = Add1(net_changes);
		switch(cmd)
		{
		case TargetCmd::Hurt:
			c.type = NetChange::CHEAT_HURT;
			break;
		case TargetCmd::BreakAction:
			c.type = NetChange::CHEAT_BREAK_ACTION;
			break;
		case TargetCmd::Fall:
			c.type = NetChange::CHEAT_FALL;
			break;
		}
		c.unit = u;
	}
}
