#include "Pch.h"
#include "GameCore.h"
#include "ConsoleCommands.h"
#include "Game.h"
#include "Version.h"
#include "Terrain.h"
#include "Content.h"
#include "InsideLocation.h"
#include "City.h"
#include "ServerPanel.h"
#include "GameGui.h"
#include "Console.h"
#include "MpBox.h"
#include "AIController.h"
#include "BitStreamFunc.h"
#include "Team.h"
#include "SaveState.h"
#include "QuestManager.h"
#include "BuildingGroup.h"
#include "ScriptManager.h"
#include "World.h"
#include "Level.h"
#include "DirectX.h"

//-----------------------------------------------------------------------------
extern string g_ctime;
static PrintMsgFunc g_print_func;

//-----------------------------------------------------------------------------
void Msg(cstring msg)
{
	g_print_func(msg);
}
template<typename... Args>
void Msg(cstring msg, const Args&... args)
{
	g_print_func(Format(msg, args...));
}

//=================================================================================================
void Game::AddCommands()
{
	cmds.push_back(ConsoleCommand(&cl_fog, "cl_fog", "draw fog (cl_fog 0/1)", F_ANYWHERE | F_CHEAT | F_WORLD_MAP));
	cmds.push_back(ConsoleCommand(&cl_lighting, "cl_lighting", "use lighting (cl_lighting 0/1)", F_ANYWHERE | F_CHEAT | F_WORLD_MAP));
	cmds.push_back(ConsoleCommand(&draw_particle_sphere, "draw_particle_sphere", "draw particle extents sphere (draw_particle_sphere 0/1)", F_ANYWHERE | F_CHEAT | F_WORLD_MAP));
	cmds.push_back(ConsoleCommand(&draw_unit_radius, "draw_unit_radius", "draw units radius (draw_unit_radius 0/1)", F_ANYWHERE | F_CHEAT | F_WORLD_MAP));
	cmds.push_back(ConsoleCommand(&draw_hitbox, "draw_hitbox", "draw weapons hitbox (draw_hitbox 0/1)", F_ANYWHERE | F_CHEAT | F_WORLD_MAP));
	cmds.push_back(ConsoleCommand(&draw_phy, "draw_phy", "draw physical colliders (draw_phy 0/1)", F_ANYWHERE | F_CHEAT | F_WORLD_MAP));
	cmds.push_back(ConsoleCommand(&draw_col, "draw_col", "draw colliders (draw_col 0/1)", F_ANYWHERE | F_CHEAT | F_WORLD_MAP));
	cmds.push_back(ConsoleCommand(&game_speed, "speed", "game speed (speed 0-10)", F_CHEAT | F_WORLD_MAP | F_SINGLEPLAYER, 0.f, 10.f));
	cmds.push_back(ConsoleCommand(&next_seed, "next_seed", "Random seed used in next map generation", F_ANYWHERE | F_CHEAT | F_WORLD_MAP));
	cmds.push_back(ConsoleCommand(&dont_wander, "dont_wander", "citizens don't wander around city (dont_wander 0/1)", F_ANYWHERE | F_WORLD_MAP));
	cmds.push_back(ConsoleCommand(&draw_flags, "draw_flags", "set which elements of game draw (draw_flags int)", F_ANYWHERE | F_CHEAT | F_WORLD_MAP));
	cmds.push_back(ConsoleCommand(&mp_interp, "mp_interp", "interpolation interval (mp_interp 0.f-1.f)", F_MULTIPLAYER | F_WORLD_MAP | F_MP_VAR, 0.f, 1.f));
	cmds.push_back(ConsoleCommand(&mp_use_interp, "mp_use_interp", "set use of interpolation (mp_use_interp 0/1)", F_MULTIPLAYER | F_WORLD_MAP | F_MP_VAR));
	cmds.push_back(ConsoleCommand(&cl_postfx, "cl_postfx", "use post effects (cl_postfx 0/1)", F_ANYWHERE | F_WORLD_MAP));
	cmds.push_back(ConsoleCommand(&cl_normalmap, "cl_normalmap", "use normal mapping (cl_normalmap 0/1)", F_ANYWHERE | F_WORLD_MAP));
	cmds.push_back(ConsoleCommand(&cl_specularmap, "cl_specularmap", "use specular mapping (cl_specularmap 0/1)", F_ANYWHERE | F_WORLD_MAP));
	cmds.push_back(ConsoleCommand(&cl_glow, "cl_glow", "use glow (cl_glow 0/1)", F_ANYWHERE | F_WORLD_MAP));
	cmds.push_back(ConsoleCommand(&uv_mod, "uv_mod", "terrain uv mod (uv_mod 1-256)", F_ANYWHERE, 1, 256, VoidF(this, &Game::UvModChanged)));
	cmds.push_back(ConsoleCommand(&shader_version, "shader_version", "force shader version (shader_version 2/3)", F_ANYWHERE | F_WORLD_MAP, 2, 3, VoidF(this, &Game::ShaderVersionChanged)));
	cmds.push_back(ConsoleCommand(&profiler_mode, "profiler", "profiler execution: 0-disabled, 1-update, 2-rendering", F_ANYWHERE | F_WORLD_MAP, 0, 2));
	cmds.push_back(ConsoleCommand(&grass_range, "grass_range", "grass draw range", F_ANYWHERE | F_WORLD_MAP, 0.f));
	cmds.push_back(ConsoleCommand(&devmode, "devmode", "developer mode (devmode 0/1)", F_GAME | F_SERVER | F_WORLD_MAP | F_MENU));

	cmds.push_back(ConsoleCommand(CMD_WHISPER, "whisper", "send private message to player (whisper nick msg)", F_LOBBY | F_MULTIPLAYER | F_WORLD_MAP | F_NO_ECHO));
	cmds.push_back(ConsoleCommand(CMD_WHISPER, "w", "send private message to player, short from whisper (w nick msg)", F_LOBBY | F_MULTIPLAYER | F_WORLD_MAP | F_NO_ECHO));
	cmds.push_back(ConsoleCommand(CMD_SAY, "say", "send message to all players (say msg)", F_LOBBY | F_MULTIPLAYER | F_WORLD_MAP | F_NO_ECHO));
	cmds.push_back(ConsoleCommand(CMD_SAY, "s", "send message to all players, short from say (say msg)", F_LOBBY | F_MULTIPLAYER | F_WORLD_MAP | F_NO_ECHO));
	cmds.push_back(ConsoleCommand(CMD_SERVER_SAY, "server", "send message from server to all players (server msg)", F_LOBBY | F_MULTIPLAYER | F_SERVER | F_WORLD_MAP | F_NO_ECHO));
	cmds.push_back(ConsoleCommand(CMD_KICK, "kick", "kick player from server (kick nick)", F_LOBBY | F_MULTIPLAYER | F_SERVER | F_WORLD_MAP));
	cmds.push_back(ConsoleCommand(CMD_READY, "ready", "set player as ready/unready", F_LOBBY));
	cmds.push_back(ConsoleCommand(CMD_LEADER, "leader", "change team leader (leader nick)", F_LOBBY | F_MULTIPLAYER));
	cmds.push_back(ConsoleCommand(CMD_EXIT, "exit", "exit to menu", F_NOT_MENU | F_WORLD_MAP));
	cmds.push_back(ConsoleCommand(CMD_QUIT, "quit", "quit from game", F_ANYWHERE));
	cmds.push_back(ConsoleCommand(CMD_RANDOM, "random", "roll random number 1-100 or pick random character (random, random [name], use ? to get list)", F_ANYWHERE));
	cmds.push_back(ConsoleCommand(CMD_CMDS, "cmds", "show commands and write them to file commands.txt, with all show unavailable commands too (cmds [all])", F_ANYWHERE));
	cmds.push_back(ConsoleCommand(CMD_START, "start", "start server", F_LOBBY));
	cmds.push_back(ConsoleCommand(CMD_WARP, "warp", "move player into building (warp inn/arena/hall)", F_CHEAT | F_GAME));
	cmds.push_back(ConsoleCommand(CMD_KILLALL, "killall", "kills all enemy units in current level, with 1 it kills allies too, ignore unit in front of player (killall [0/1])", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_SAVE, "save", "save game (save 1-10 [text] or filename)", F_GAME | F_WORLD_MAP | F_SERVER));
	cmds.push_back(ConsoleCommand(CMD_LOAD, "load", "load game (load 1-10 or filename)", F_GAME | F_WORLD_MAP | F_MENU | F_SERVER));
	cmds.push_back(ConsoleCommand(CMD_SHOW_MINIMAP, "show_minimap", "reveal minimap", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_SKIP_DAYS, "skip_days", "skip days [skip_days [count])", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_LIST, "list", "display list of types, don't enter type to list possible choices (list type [filter])", F_ANYWHERE));
	cmds.push_back(ConsoleCommand(CMD_HEAL_UNIT, "heal_unit", "heal unit in front of player", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_SUICIDE, "suicide", "kill player", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_CITIZEN, "citizen", "citizens/crazies don't attack player or his team", F_GAME | F_CHEAT | F_WORLD_MAP));
	cmds.push_back(ConsoleCommand(CMD_SCREENSHOT, "screenshot", "save screenshot", F_ANYWHERE));
	cmds.push_back(ConsoleCommand(CMD_SCARE, "scare", "enemies escape", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_INVISIBLE, "invisible", "ai can't see player (invisible 0/1)", F_GAME | F_CHEAT | F_NO_ECHO));
	cmds.push_back(ConsoleCommand(CMD_GODMODE, "godmode", "player can't be killed (godmode 0/1)", F_ANYWHERE | F_CHEAT | F_NO_ECHO));
	cmds.push_back(ConsoleCommand(CMD_NOCLIP, "noclip", "turn off player collisions (noclip 0/1)", F_GAME | F_CHEAT | F_NO_ECHO));
	cmds.push_back(ConsoleCommand(CMD_GOTO_MAP, "goto_map", "transport player to world map", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_VERSION, "version", "displays game version", F_ANYWHERE));
	cmds.push_back(ConsoleCommand(CMD_REVEAL, "reveal", "reveal all locations on world map", F_GAME | F_CHEAT | F_WORLD_MAP));
	cmds.push_back(ConsoleCommand(CMD_MAP2CONSOLE, "map2console", "draw dungeon map in console", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_ADD_ITEM, "add_item", "add item to player inventory (add_item id [count])", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_ADD_TEAM_ITEM, "add_team_item", "add team item to player inventory (add_team_item id [count])", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_ADD_GOLD, "add_gold", "give gold to player (add_gold count)", F_GAME | F_CHEAT | F_WORLD_MAP));
	cmds.push_back(ConsoleCommand(CMD_ADD_TEAM_GOLD, "add_team_gold", "give gold to team (add_team_gold count)", F_GAME | F_CHEAT | F_WORLD_MAP));
	cmds.push_back(ConsoleCommand(CMD_SET_STAT, "set_stat", "set player statistics, use ? to get list (set_stat stat value)", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_MOD_STAT, "mod_stat", "modify player statistics, use ? to get list (mod_stat stat value)", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_HELP, "help", "display information about command (help [command])", F_ANYWHERE));
	cmds.push_back(ConsoleCommand(CMD_SPAWN_UNIT, "spawn_unit", "create unit in front of player (spawn_unit id [level count arena])", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_HEAL, "heal", "heal player", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_KILL, "kill", "kill unit in front of player", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_PLAYER_DEVMODE, "player_devmode", "get/set player developer mode in multiplayer (player_devmode nick/all [0/1])", F_MULTIPLAYER | F_WORLD_MAP | F_CHEAT | F_SERVER));
	cmds.push_back(ConsoleCommand(CMD_NOAI, "noai", "disable ai (noai 0/1)", F_CHEAT | F_GAME | F_WORLD_MAP | F_NO_ECHO));
	cmds.push_back(ConsoleCommand(CMD_PAUSE, "pause", "pause/unpause", F_GAME | F_SERVER));
	cmds.push_back(ConsoleCommand(CMD_MULTISAMPLING, "multisampling", "sets multisampling (multisampling type [quality])", F_ANYWHERE | F_WORLD_MAP | F_NO_ECHO));
	cmds.push_back(ConsoleCommand(CMD_QUICKSAVE, "quicksave", "save game on last slot", F_GAME | F_WORLD_MAP));
	cmds.push_back(ConsoleCommand(CMD_QUICKLOAD, "quickload", "load game from last slot", F_SINGLEPLAYER | F_WORLD_MAP | F_MENU));
	cmds.push_back(ConsoleCommand(CMD_RESOLUTION, "resolution", "show or change display resolution (resolution [w h hz])", F_ANYWHERE | F_WORLD_MAP));
	cmds.push_back(ConsoleCommand(CMD_QS, "qs", "pick Random character, get ready and start game", F_LOBBY));
	cmds.push_back(ConsoleCommand(CMD_CLEAR, "clear", "clear text", F_ANYWHERE | F_WORLD_MAP));
	cmds.push_back(ConsoleCommand(CMD_HURT, "hurt", "deal 100 damage to unit ('hurt 1' targets self)", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_BREAK_ACTION, "break_action", "break unit current action ('break 1' targets self)", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_FALL, "fall", "unit fall on ground for some time ('fall 1' targets self)", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_RELOAD_SHADERS, "reload_shaders", "reload shaders", F_ANYWHERE | F_WORLD_MAP));
	cmds.push_back(ConsoleCommand(CMD_TILE_INFO, "tile_info", "display info about map tile", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_SET_SEED, "set_seed", "set randomness seed", F_ANYWHERE | F_WORLD_MAP | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_CRASH, "crash", "crash game to death!", F_ANYWHERE | F_WORLD_MAP | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_FORCE_QUEST, "force_quest", "force next random quest to select (use list quest or none/reset)", F_SERVER | F_GAME | F_WORLD_MAP | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_STUN, "stun", "stun unit for time (stun [length=1] [1 = self])", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_REFRESH_COOLDOWN, "refresh_cooldown", "refresh action cooldown/charges", F_GAME | F_CHEAT));

	// verify all commands are added
#ifdef _DEBUG
	for(uint i = 0; i < CMD_MAX; ++i)
	{
		bool ok = false;
		for(auto& cmd : cmds)
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
#endif
}

//=================================================================================================
void Game::AddConsoleMsg(cstring msg)
{
	console->AddText(msg);
}

//=================================================================================================
void Game::ParseCommand(const string& _str, PrintMsgFunc print_func, PARSE_SOURCE source)
{
	if(!print_func)
		print_func = PrintMsgFunc(this, &Game::AddConsoleMsg);
	g_print_func = print_func;

	Tokenizer t(Tokenizer::F_JOIN_MINUS);
	t.FromString(_str);

	try
	{
		if(!t.Next())
			return;

		if(t.IsSymbol('#'))
		{
			Msg(_str.c_str());
			if(!devmode)
			{
				Msg("You can't use script command without devmode.");
				return;
			}
			if(game_state != GS_LEVEL)
			{
				Msg("Script commands can only be used ingame.");
				return;
			}

			cstring code = t.GetTextRest();
			if(Net::IsLocal())
			{
				string& output = SM.OpenOutput();
				SM.SetContext(pc, pc_data.selected_target);
				SM.RunScript(code);
				if(!output.empty())
					Msg(output.c_str());
				SM.CloseOutput();
			}
			else
			{
				NetChange& c = Add1(Net::changes);
				c.type = NetChange::RUN_SCRIPT;
				c.str = StringPool.Get();
				*c.str = code;
				c.id = (pc_data.selected_target ? pc_data.selected_target->netid : -1);
			}

			return;
		}

		const string& token = t.MustGetItem();

		for(vector<ConsoleCommand>::iterator it = cmds.begin(), end = cmds.end(); it != end; ++it)
		{
			if(token != it->name)
				continue;

			if(IS_SET(it->flags, F_CHEAT) && !devmode)
			{
				Msg("You can't use command '%s' without devmode.", token.c_str());
				return;
			}

			if(!IS_ALL_SET(it->flags, F_ANYWHERE))
			{
				if(server_panel->visible)
				{
					if(!IS_SET(it->flags, F_LOBBY))
					{
						Msg("You can't use command '%s' in server lobby.", token.c_str());
						return;
					}
					else if(IS_SET(it->flags, F_SERVER) && !Net::IsServer())
					{
						Msg("Only server can use command '%s'.", token.c_str());
						return;
					}
				}
				else if(game_state == GS_MAIN_MENU)
				{
					if(!IS_SET(it->flags, F_MENU))
					{
						Msg("You can't use command '%s' in menu.", token.c_str());
						return;
					}
				}
				else if(Net::IsOnline())
				{
					if(!IS_SET(it->flags, F_MULTIPLAYER))
					{
						Msg("You can't use command '%s' in multiplayer.", token.c_str());
						return;
					}
					else if(IS_SET(it->flags, F_SERVER) && !Net::IsServer())
					{
						Msg("Only server can use command '%s'.", token.c_str());
						return;
					}
				}
				else if(game_state == GS_WORLDMAP)
				{
					if(!IS_SET(it->flags, F_WORLD_MAP))
					{
						Msg("You can't use command '%s' on world map.", token.c_str());
						return;
					}
				}
				else
				{
					if(!IS_SET(it->flags, F_SINGLEPLAYER))
					{
						Msg("You can't use command '%s' in singleplayer.", token.c_str());
						return;
					}
				}
			}

			if(it->type == ConsoleCommand::VAR_BOOL)
			{
				if(t.Next())
				{
					if(IS_SET(it->flags, F_MP_VAR) && !Net::IsServer())
					{
						Msg("Only server can change this variable.");
						return;
					}

					bool value = t.MustGetBool();
					bool& var = it->Get<bool>();

					if(value != var)
					{
						var = value;
						if(IS_SET(it->flags, F_MP_VAR))
							Net::PushChange(NetChange::CHANGE_MP_VARS);
						if(it->changed)
							it->changed();
					}
				}
				Msg("%s = %d", it->name, *(bool*)it->var ? 1 : 0);
				return;
			}
			else if(it->type == ConsoleCommand::VAR_FLOAT)
			{
				float& f = it->Get<float>();

				if(t.Next())
				{
					if(IS_SET(it->flags, F_MP_VAR) && !Net::IsServer())
					{
						Msg("Only server can change this variable.");
						return;
					}

					float f2 = Clamp(t.MustGetNumberFloat(), it->_float.x, it->_float.y);
					if(!Equal(f, f2))
					{
						f = f2;
						if(IS_SET(it->flags, F_MP_VAR))
							Net::PushChange(NetChange::CHANGE_MP_VARS);
					}
				}
				Msg("%s = %g", it->name, f);
				return;
			}
			else if(it->type == ConsoleCommand::VAR_INT)
			{
				int& i = it->Get<int>();

				if(t.Next())
				{
					if(IS_SET(it->flags, F_MP_VAR) && !Net::IsServer())
					{
						Msg("Only server can change this variable.");
						return;
					}

					int i2 = Clamp(t.MustGetInt(), it->_int.x, it->_int.y);
					if(i != i2)
					{
						i = i2;
						if(IS_SET(it->flags, F_MP_VAR))
							Net::PushChange(NetChange::CHANGE_MP_VARS);
						if(it->changed)
							it->changed();
					}
				}
				Msg("%s = %d", it->name, i);
				return;
			}
			else if(it->type == ConsoleCommand::VAR_UINT)
			{
				uint& u = it->Get<uint>();

				if(t.Next())
				{
					if(IS_SET(it->flags, F_MP_VAR) && !Net::IsServer())
					{
						Msg("Only server can change this variable.");
						return;
					}

					uint u2 = Clamp(t.MustGetUint(), it->_uint.x, it->_uint.y);
					if(u != u2)
					{
						u = u2;
						if(IS_SET(it->flags, F_MP_VAR))
							Net::PushChange(NetChange::CHANGE_MP_VARS);
					}
				}
				Msg("%s = %u", it->name, u);
				return;
			}
			else
			{
				if(!IS_SET(it->flags, F_NO_ECHO))
					Msg(_str.c_str());

				switch(it->cmd)
				{
				case CMD_GOTO_MAP:
					if(Net::IsLocal())
						ExitToMap();
					else
						Net::PushChange(NetChange::CHEAT_GOTO_MAP);
					break;
				case CMD_VERSION:
					Msg("CaRpg version " VERSION_STR ", built %s.", g_ctime.c_str());
					break;
				case CMD_QUIT:
					if(t.Next() && t.IsInt() && t.GetInt() == 1)
						exit(0);
					else
						Quit();
					break;
				case CMD_REVEAL:
					if(Net::IsLocal())
						W.Reveal();
					else
						Net::PushChange(NetChange::CHEAT_REVEAL);
					break;
				case CMD_MAP2CONSOLE:
					if(game_state == GS_LEVEL && !L.location->outside)
					{
						InsideLocationLevel& lvl = ((InsideLocation*)L.location)->GetLevelData();
						rysuj_mape_konsola(lvl.map, lvl.w, lvl.h);
					}
					else
						Msg("You need to be inside dungeon!");
					break;
				case CMD_ADD_ITEM:
				case CMD_ADD_TEAM_ITEM:
					if(t.Next())
					{
						bool is_team = (it->cmd == CMD_ADD_TEAM_ITEM);
						const string& item_name = t.MustGetItem();
						const Item* item = Item::TryGet(item_name);
						if(!item || IS_SET(item->flags, ITEM_SECRET))
							Msg("Can't find item with id '%s'!", item_name.c_str());
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
							{
								PreloadItem(item);
								AddItem(*pc->unit, item, count, is_team);
							}
							else
							{
								NetChange& c = Add1(Net::changes);
								c.type = NetChange::CHEAT_ADD_ITEM;
								c.base_item = item;
								c.ile = count;
								c.id = is_team ? 1 : 0;
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
						bool is_team = (it->cmd == CMD_ADD_TEAM_GOLD);
						int count = t.MustGetInt();
						if(is_team && count <= 0)
							Msg("Gold count must by positive!");
						if(Net::IsLocal())
						{
							if(is_team)
								AddGold(count);
							else
								pc->unit->gold = max(pc->unit->gold + count, 0);
						}
						else
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::CHEAT_ADD_GOLD;
							c.id = (is_team ? 1 : 0);
							c.ile = count;
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
						LocalVector2<AttributeId> attribs;
						for(int i = 0; i < (int)AttributeId::MAX; ++i)
							attribs.push_back((AttributeId)i);
						std::sort(attribs.begin(), attribs.end(),
							[](AttributeId a1, AttributeId a2) -> bool
						{
							return strcmp(Attribute::attributes[(int)a1].id, Attribute::attributes[(int)a2].id) < 0;
						});
						LocalVector2<SkillId> skills;
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
						int co;
						bool skill;
						const string& s = t.MustGetItem();
						Attribute* ai = Attribute::Find(s);
						if(ai)
						{
							co = (int)ai->attrib_id;
							skill = false;
						}
						else
						{
							Skill* si = Skill::Find(s);
							if(si)
							{
								co = (int)si->skill_id;
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
									if(it->cmd == CMD_MOD_STAT)
										num += pc->unit->unmod_stats.skill[co];
									int v = Clamp(num, Skill::MIN, Skill::MAX);
									if(v != pc->unit->unmod_stats.skill[co])
										pc->unit->Set((SkillId)co, v);
								}
								else
								{
									if(it->cmd == CMD_MOD_STAT)
										num += pc->unit->unmod_stats.attrib[co];
									int v = Clamp(num, Attribute::MIN, Attribute::MAX);
									if(v != pc->unit->unmod_stats.attrib[co])
										pc->unit->Set((AttributeId)co, v);
								}
							}
							else
							{
								NetChange& c = Add1(Net::changes);
								c.type = (it->cmd == CMD_SET_STAT ? NetChange::CHEAT_SET_STAT : NetChange::CHEAT_MOD_STAT);
								c.id = co;
								c.ile = (skill ? 1 : 0);
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
								if(IS_SET(cmd.flags, F_CHEAT) && !devmode)
									ok = false;

								if(!IS_ALL_SET(it->flags, F_ANYWHERE))
								{
									if(server_panel->visible)
									{
										if(!IS_SET(it->flags, F_LOBBY))
											ok = false;
										else if(IS_SET(it->flags, F_SERVER) && !Net::IsServer())
											ok = false;
									}
									else if(game_state == GS_MAIN_MENU)
									{
										if(!IS_SET(it->flags, F_MENU))
											ok = false;
									}
									else if(Net::IsOnline())
									{
										if(!IS_SET(it->flags, F_MULTIPLAYER))
											ok = false;
										else if(IS_SET(it->flags, F_SERVER) && !Net::IsServer())
											ok = false;
									}
									else if(game_state == GS_WORLDMAP)
									{
										if(!IS_SET(it->flags, F_WORLD_MAP))
											ok = false;
									}
									else
									{
										if(!IS_SET(it->flags, F_SINGLEPLAYER))
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
						if(!data || IS_SET(data->flags, F_SECRET))
							Msg("Missing base unit '%s'!", id.c_str());
						else
						{
							int level = -1, ile = 1, in_arena = -1;
							if(t.Next())
							{
								level = t.MustGetInt();
								if(t.Next())
								{
									ile = t.MustGetInt();
									if(t.Next())
										in_arena = Clamp(t.MustGetInt(), -1, 1);
								}
							}

							if(Net::IsLocal())
							{
								LevelContext& ctx = GetContext(*pc->unit);

								for(int i = 0; i < ile; ++i)
								{
									Unit* u = SpawnUnitNearLocation(ctx, pc->unit->GetFrontPos(), *data, &pc->unit->pos, level);
									if(!u)
									{
										Msg("No free space for unit '%s'!", data->id.c_str());
										break;
									}
									else if(in_arena != -1)
									{
										u->in_arena = in_arena;
										at_arena.push_back(u);
									}
									if(Net::IsOnline())
										Net_SpawnUnit(u);
								}
							}
							else
							{
								NetChange& c = Add1(Net::changes);
								c.type = NetChange::CHEAT_SPAWN_UNIT;
								c.base_unit = data;
								c.ile = ile;
								c.id = level;
								c.i = in_arena;
							}
						}
					}
					else
						Msg("You need to enter unit id!");
					break;
				case CMD_HEAL:
					if(Net::IsLocal())
					{
						pc->unit->hp = pc->unit->hpmax;
						pc->unit->stamina = pc->unit->stamina_max;
						pc->unit->RemovePoison();
						pc->unit->RemoveEffect(EffectId::Stun);
						if(Net::IsOnline())
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::UPDATE_HP;
							c.unit = pc->unit;
						}
					}
					else
						Net::PushChange(NetChange::CHEAT_HEAL);
					break;
				case CMD_KILL:
					if(pc_data.selected_target)
					{
						if(Net::IsLocal())
							GiveDmg(GetContext(*pc->unit), nullptr, pc_data.selected_target->hpmax, *pc_data.selected_target);
						else
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::CHEAT_KILL;
							c.unit = pc_data.selected_target;
						}
					}
					else
						Msg("No unit selected.");
					break;
				case CMD_LIST:
					CmdList(t);
					break;
				case CMD_HEAL_UNIT:
					if(pc_data.selected_target)
					{
						if(Net::IsLocal())
						{
							pc_data.selected_target->hp = pc_data.selected_target->hpmax;
							pc_data.selected_target->stamina = pc_data.selected_target->stamina_max;
							pc_data.selected_target->RemovePoison();
							pc_data.selected_target->RemoveEffect(EffectId::Stun);
							if(Net::IsOnline())
							{
								NetChange& c = Add1(Net::changes);
								c.type = NetChange::UPDATE_HP;
								c.unit = pc_data.selected_target;
								if(pc_data.selected_target->player && !pc_data.selected_target->player->is_local)
									pc_data.selected_target->player->player_info->update_flags |= PlayerInfo::UF_STAMINA;
							}
						}
						else
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::CHEAT_HEAL_UNIT;
							c.unit = pc_data.selected_target;
						}
					}
					else
						Msg("No unit selected.");
					break;
				case CMD_SUICIDE:
					if(Net::IsLocal())
						GiveDmg(GetContext(*pc->unit), nullptr, pc->unit->hpmax, *pc->unit);
					else
						Net::PushChange(NetChange::CHEAT_SUICIDE);
					break;
				case CMD_CITIZEN:
					if(Team.is_bandit || Team.crazies_attack)
					{
						if(Net::IsLocal())
						{
							Team.is_bandit = false;
							Team.crazies_attack = false;
							if(Net::IsOnline())
								Net::PushChange(NetChange::CHANGE_FLAGS);
						}
						else
							Net::PushChange(NetChange::CHEAT_CITIZEN);
					}
					break;
				case CMD_SCREENSHOT:
					TakeScreenshot();
					break;
				case CMD_SCARE:
					if(Net::IsLocal())
					{
						for(AIController* ai : ais)
						{
							if(IsEnemy(*ai->unit, *pc->unit) && Vec3::Distance(ai->unit->pos, pc->unit->pos) < ALERT_RANGE.x && CanSee(*ai->unit, *pc->unit))
							{
								ai->morale = -10;
								ai->target_last_pos = pc->unit->pos;
							}
						}
					}
					else
						Net::PushChange(NetChange::CHEAT_SCARE);
					break;
				case CMD_INVISIBLE:
					if(t.Next())
					{
						if(!t.MustGetBool())
						{
							pc->unit->invisible = false;
							if(!Net::IsLocal())
							{
								invisible = false;
								NetChange& c = Add1(Net::changes);
								c.type = NetChange::CHEAT_INVISIBLE;
								c.id = 0;
							}
						}
						else
						{
							pc->unit->invisible = true;
							if(!Net::IsLocal())
							{
								invisible = true;
								NetChange& c = Add1(Net::changes);
								c.type = NetChange::CHEAT_INVISIBLE;
								c.id = 1;
							}
						}
					}
					Msg("invisible = %d", pc->unit->invisible ? 1 : 0);
					break;
				case CMD_GODMODE:
					if(t.Next())
					{
						if(!t.MustGetBool())
						{
							pc->godmode = false;
							if(!Net::IsLocal())
							{
								godmode = false;
								NetChange& c = Add1(Net::changes);
								c.type = NetChange::CHEAT_GODMODE;
								c.id = 0;
							}
						}
						else
						{
							pc->godmode = true;
							if(!Net::IsLocal())
							{
								godmode = true;
								NetChange& c = Add1(Net::changes);
								c.type = NetChange::CHEAT_GODMODE;
								c.id = 1;
							}
						}
					}
					Msg("godmode = %d", pc->godmode ? 1 : 0);
					break;
				case CMD_NOCLIP:
					if(t.Next())
					{
						if(!t.MustGetBool())
						{
							pc->noclip = false;
							if(!Net::IsLocal())
							{
								noclip = false;
								NetChange& c = Add1(Net::changes);
								c.type = NetChange::CHEAT_NOCLIP;
								c.id = 0;
							}
						}
						else
						{
							pc->noclip = true;
							if(!Net::IsLocal())
							{
								noclip = true;
								NetChange& c = Add1(Net::changes);
								c.type = NetChange::CHEAT_NOCLIP;
								c.id = 1;
							}
						}
					}
					Msg("noclip = %d", pc->noclip ? 1 : 0);
					break;
				case CMD_KILLALL:
					{
						int typ = 0;
						if(t.Next())
							typ = t.MustGetInt();
						Unit* ignore = nullptr;
						if(pc_data.before_player == BP_UNIT)
							ignore = pc_data.before_player_ptr.unit;
						if(!Cheat_KillAll(typ, *pc->unit, ignore))
							Msg("Unknown parameter '%d'.", typ);
					}
					break;
				case CMD_SAVE:
					if(CanSaveGame())
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
							else
							{
								slot = Clamp(t.MustGetInt(), 1, 10);
								if(t.Next())
									text = t.MustGetString();
							}
						}
						if(slot != -1)
							SaveGameSlot(slot, text->c_str());
						else
							SaveGameFilename(text.get_ref());
					}
					else
						Msg(Net::IsClient() ? "Only server can save game." : "You can't save game in this moment.");
					break;
				case CMD_LOAD:
					if(CanLoadGame())
					{
						Net::SetMode(Net::Mode::Singleplayer);
						LocalString name;
						int slot = 1;
						if(t.Next())
						{
							if(t.IsString())
							{
								name = t.GetString();
								slot = -1;
							}
							else
								slot = Clamp(t.MustGetInt(), 1, 10);
						}

						try
						{
							if(slot != -1)
								LoadGameSlot(slot);
							else
								LoadGameFilename(name.get_ref());
							GUI.CloseDialog(console);
						}
						catch(const SaveException& ex)
						{
							cstring error = Format("Failed to load game: %s", ex.msg);
							Error(error);
							Msg(error);
							if(!GUI.HaveDialog(console))
								GUI.ShowDialog(console);
						}
					}
					else
						Msg(Net::IsClient() ? "Only server can load game." : "You can't load game in this moment.");
					break;
				case CMD_SHOW_MINIMAP:
					if(Net::IsLocal())
						Cheat_ShowMinimap();
					else
						Net::PushChange(NetChange::CHEAT_SHOW_MINIMAP);
					break;
				case CMD_SKIP_DAYS:
					{
						int ile = 1;
						if(t.Next())
							ile = t.MustGetInt();
						if(ile > 0)
						{
							if(Net::IsLocal())
								W.Update(ile, World::UM_SKIP);
							else
							{
								NetChange& c = Add1(Net::changes);
								c.type = NetChange::CHEAT_SKIP_DAYS;
								c.id = ile;
							}
						}
					}
					break;
				case CMD_WARP:
					if(t.Next())
					{
						const string& type = t.MustGetItem();
						BuildingGroup* group = BuildingGroup::TryGet(type);
						if(!group)
						{
							Msg("Missing building group '%s'.", type.c_str());
							break;
						}

						bool ok = false;
						if(city_ctx)
						{
							int index;
							InsideBuilding* building = city_ctx->FindInsideBuilding(group, index);
							if(building)
							{
								// wejdŸ do budynku
								if(Net::IsLocal())
								{
									fallback_co = FALLBACK::ENTER;
									fallback_t = -1.f;
									fallback_1 = index;
									pc->unit->frozen = (pc->unit->usable ? FROZEN::YES_NO_ANIM : FROZEN::YES);
								}
								else
								{
									NetChange& c = Add1(Net::changes);
									c.type = NetChange::CHEAT_WARP;
									c.id = index;
								}
								ok = true;
							}
						}

						if(!ok)
							Msg("Missing building of type '%s'.", type.c_str());
					}
					else
						Msg("You need to enter where.");
					break;
				case CMD_WHISPER:
					if(t.Next())
					{
						const string& player_nick = t.MustGetItem();
						int index = FindPlayerIndex(player_nick.c_str(), true);
						if(index == -1)
							Msg("No player with nick '%s'.", player_nick.c_str());
						else if(t.NextLine())
						{
							const string& text = t.MustGetItem();
							PlayerInfo& info = *game_players[index];
							if(info.id == my_id)
								Msg("Whispers in your head: %s", text.c_str());
							else
							{
								net_stream.Reset();
								BitStreamWriter f(net_stream);
								f << ID_WHISPER;
								f.WriteCasted<byte>(Net::IsServer() ? my_id : info.id);
								f << text;
								peer->Send(&net_stream, MEDIUM_PRIORITY, RELIABLE, 0, Net::IsServer() ? info.adr : server, false);
								StreamWrite(net_stream, Stream_Chat, Net::IsServer() ? info.adr : server);
								cstring s = Format("@%s: %s", info.name.c_str(), text.c_str());
								AddMsg(s);
								Info(s);
							}
						}
						else
							Msg("You need to enter message.");
					}
					else
						Msg("You need to enter player nick.");
					break;
				case CMD_SERVER_SAY:
					if(t.NextLine())
					{
						const string& text = t.MustGetItem();
						if(players > 1)
						{
							net_stream.Reset();
							BitStreamWriter f(net_stream);
							f << ID_SERVER_SAY;
							f << text;
							peer->Send(&net_stream, MEDIUM_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
							StreamWrite(net_stream, Stream_Chat, UNASSIGNED_SYSTEM_ADDRESS);
						}
						AddServerMsg(text.c_str());
						Info("SERWER: %s", text.c_str());
						if(game_state == GS_LEVEL)
							game_gui->AddSpeechBubble(pc->unit, text.c_str());
					}
					else
						Msg("You need to enter message.");
					break;
				case CMD_KICK:
					if(t.Next())
					{
						const string& player_name = t.MustGetItem();
						int index = FindPlayerIndex(player_name.c_str(), true);
						if(index == -1)
							Msg("No player with nick '%s'.", player_name.c_str());
						else
							KickPlayer(index);
					}
					else
						Msg("You need to enter player nick.");
					break;
				case CMD_READY:
					{
						PlayerInfo& info = *game_players[0];
						info.ready = !info.ready;
						ChangeReady();
					}
					break;
				case CMD_LEADER:
					if(t.Next())
					{
						const string& player_name = t.MustGetItem();
						int index = FindPlayerIndex(player_name.c_str(), true);
						if(index == -1)
							Msg("No player with nick '%s'.", player_name.c_str());
						else
						{
							PlayerInfo& info = *game_players[index];
							if(leader_id == info.id)
								Msg("Player '%s' is already a leader.", player_name.c_str());
							else if(Net::IsServer() || leader_id == my_id)
							{
								if(server_panel->visible)
								{
									if(Net::IsServer())
									{
										leader_id = info.id;
										AddLobbyUpdate(Int2(Lobby_ChangeLeader, 0));
									}
									else
										Msg("You can't change a leader.");
								}
								else
								{
									NetChange& c = Add1(Net::changes);
									c.type = NetChange::CHANGE_LEADER;
									c.id = info.id;

									if(Net::IsServer())
									{
										leader_id = info.id;
										Team.leader = info.u;

										if(dialog_enc)
											dialog_enc->bts[0].state = (IsLeader() ? Button::NONE : Button::DISABLED);
									}
								}

								AddMsg(Format("Leader changed to '%s'.", info.name.c_str()));
							}
							else
								Msg("You can't change a leader.");
						}
					}
					else
						Msg("You need to enter leader nick.");
					break;
				case CMD_EXIT:
					ExitToMenu();
					break;
				case CMD_RANDOM:
					if(server_panel->visible)
					{
						if(t.Next())
						{
							if(t.IsSymbol('?'))
							{
								LocalVector2<Class> classes;
								for(ClassInfo& ci : ClassInfo::classes)
								{
									if(ci.pickable)
										classes.push_back(ci.class_id);
								}
								std::sort(classes.begin(), classes.end(),
									[](Class c1, Class c2) -> bool
								{
									return strcmp(ClassInfo::classes[(int)c1].id, ClassInfo::classes[(int)c2].id) < 0;
								});
								LocalString str = "List of classes: ";
								Join(classes.Get(), str.get_ref(), ", ",
									[](Class c)
								{
									return ClassInfo::classes[(int)c].id;
								});
								str += ".";
								Msg(str.c_str());
							}
							else
							{
								const string& clas = t.MustGetItem();
								ClassInfo* ci = ClassInfo::Find(clas);
								if(ci)
								{
									if(ClassInfo::IsPickable(ci->class_id))
									{
										server_panel->PickClass(ci->class_id, false);
										Msg("You picked Random character.");
									}
									else
										Msg("Class '%s' is not pickable by players.", clas.c_str());
								}
								else
									Msg("Invalid class name '%s'. Use 'Random ?' for list of classes.", clas.c_str());
							}
						}
						else
						{
							server_panel->PickClass(Class::RANDOM, false);
							Msg("You picked Random character.");
						}
					}
					else if(Net::IsOnline())
					{
						int n = Random(1, 100);
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::RANDOM_NUMBER;
						c.id = n;
						c.unit = pc->unit;
						AddMsg(Format("You rolled %d.", n));
					}
					else
						Msg("You rolled %d.", Random(1, 100));
					break;
				case CMD_START:
					{
						if(sv_startup)
						{
							Msg("Server is already starting.");
							break;
						}

						cstring error_text = nullptr;

						for(auto info : game_players)
						{
							if(!info->ready)
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
							packet_data[0] = ID_TIMER;
							packet_data[1] = (byte)STARTUP_TIMER;
							peer->Send((cstring)&packet_data[0], 2, IMMEDIATE_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
							StreamWrite(&packet_data[0], 2, Stream_UpdateLobbyServer, UNASSIGNED_SYSTEM_ADDRESS);
							server_panel->bts[4].text = server_panel->txStop;
							cstring s = Format(server_panel->txStartingIn, STARTUP_TIMER);
							AddMsg(s);
							Info(s);
						}
						else
							Msg(error_text);
					}
					break;
				case CMD_SAY:
					if(t.NextLine())
					{
						const string& text = t.MustGetItem();
						net_stream.Reset();
						BitStreamWriter f(net_stream);
						f << ID_SAY;
						f.WriteCasted<byte>(my_id);
						f << text;
						peer->Send(&net_stream, MEDIUM_PRIORITY, RELIABLE, 0, Net::IsServer() ? UNASSIGNED_SYSTEM_ADDRESS : server, Net::IsServer());
						StreamWrite(net_stream, Stream_Chat, Net::IsServer() ? UNASSIGNED_SYSTEM_ADDRESS : server);
						cstring s = Format("%s: %s", game_players[0]->name.c_str(), text.c_str());
						AddMsg(s);
						Info(s);
					}
					else
						Msg("You need to enter message.");
					break;
				case CMD_PLAYER_DEVMODE:
					if(t.Next())
					{
						string player_name = t.MustGetItem();
						if(t.Next())
						{
							bool b = t.MustGetBool();
							if(player_name == "all")
							{
								for(auto pinfo : game_players)
								{
									auto& info = *pinfo;
									if(info.left == PlayerInfo::LEFT_NO && info.devmode != b && info.id != 0)
									{
										info.devmode = b;
										NetChangePlayer& c = Add1(info.u->player->player_info->changes);
										c.type = NetChangePlayer::DEVMODE;
										c.id = (b ? 1 : 0);
									}
								}
							}
							else
							{
								int index = FindPlayerIndex(player_name.c_str(), true);
								if(index == -1)
									Msg("No player with nick '%s'.", player_name.c_str());
								else
								{
									PlayerInfo& info = *game_players[index];
									if(info.devmode != b)
									{
										info.devmode = b;
										NetChangePlayer& c = Add1(info.u->player->player_info->changes);
										c.type = NetChangePlayer::DEVMODE;
										c.id = (b ? 1 : 0);
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
								for(auto pinfo : game_players)
								{
									auto& info = *pinfo;
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
									Msg(s->c_str());
								}
								else
									Msg("No players.");
							}
							else
							{
								int index = FindPlayerIndex(player_name.c_str(), true);
								if(index == -1)
									Msg("No player with nick '%s'.", player_name.c_str());
								else
									Msg("Player devmode: %s(%d).", player_name.c_str(), game_players[index]->devmode ? 1 : 0);
							}
						}
					}
					else
						Msg("You need to enter player nick/all.");
					break;
				case CMD_NOAI:
					if(t.Next())
					{
						noai = t.MustGetBool();
						if(Net::IsOnline())
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::CHEAT_NOAI;
							c.id = (noai ? 1 : 0);
						}
					}
					Msg("noai = %d", noai ? 1 : 0);
					break;
				case CMD_PAUSE:
					PauseGame();
					break;
				case CMD_MULTISAMPLING:
					if(t.Next())
					{
						int type = t.MustGetInt();
						int level = -1;
						if(t.Next())
							level = t.MustGetInt();
						int wynik = ChangeMultisampling(type, level);
						if(wynik == 2)
						{
							int ms, msq;
							GetMultisampling(ms, msq);
							Msg("Changed multisampling to %d, %d.", ms, msq);
						}
						else
							Msg(wynik == 1 ? "This mode is already set." : "This mode is unavailable.");
					}
					else
					{
						int ms, msq;
						GetMultisampling(ms, msq);
						Msg("multisampling = %d, %d", ms, msq);
					}
					break;
				case CMD_QUICKSAVE:
					Quicksave(true);
					break;
				case CMD_QUICKLOAD:
					if(!Quickload(true))
						Msg("Missing quicksave.");
					break;
				case CMD_RESOLUTION:
					if(t.Next())
					{
						int w = t.MustGetInt(), h = -1, hz = -1;
						bool pick_h = true, pick_hz = true, valid = false;
						if(t.Next())
						{
							h = t.MustGetInt();
							pick_h = false;
							if(t.Next())
							{
								hz = t.MustGetInt();
								pick_hz = false;
							}
						}
						uint display_modes = d3d->GetAdapterModeCount(used_adapter, DISPLAY_FORMAT);
						for(uint i = 0; i < display_modes; ++i)
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
							ChangeMode(Int2(w, h), IsFullscreen(), hz);
						else
							Msg("Can't change resolution to %dx%d (%d Hz).", w, h, hz);
					}
					else
					{
						// wypisz aktualn¹ rozdzielczoœæ i dostêpne
						LocalString s = Format("Current resolution %dx%d (%d Hz). Available: ", GetWindowSize().x, GetWindowSize().y, wnd_hz);
						uint display_modes = d3d->GetAdapterModeCount(used_adapter, DISPLAY_FORMAT);
						for(uint i = 0; i < display_modes; ++i)
						{
							D3DDISPLAYMODE d_mode;
							V(d3d->EnumAdapterModes(used_adapter, DISPLAY_FORMAT, i, &d_mode));
							s += Format("%dx%d(%d)", d_mode.Width, d_mode.Height, d_mode.RefreshRate);
							if(i + 1 != display_modes)
								s += ", ";
						}
						Msg(s);
					}
					break;
				case CMD_QS:
					if(Net::IsServer())
					{
						if(!sv_startup)
						{
							PlayerInfo& info = *game_players[0];
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

							for(auto info : game_players)
							{
								if(!info->ready)
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
								packet_data[0] = ID_TIMER;
								packet_data[1] = (byte)STARTUP_TIMER;
								peer->Send((cstring)&packet_data[0], 2, IMMEDIATE_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
								StreamWrite(&packet_data[0], 2, Stream_UpdateLobbyServer, UNASSIGNED_SYSTEM_ADDRESS);
							}
							else
								Msg(error_text);
						}
					}
					else
					{
						PlayerInfo& info = *game_players[0];
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
					break;
				case CMD_CLEAR:
					switch(source)
					{
					case PS_CONSOLE:
						console->Reset();
						break;
					case PS_CHAT:
						game_gui->mp_box->itb.Reset();
						break;
					case PS_LOBBY:
						server_panel->itb.Reset();
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
						if(t.Next() && t.GetInt() == 1)
							u = pc->unit;
						else if(pc_data.selected_unit)
							u = pc_data.selected_unit;
						else
						{
							Msg("No unit selected.");
							break;
						}
						if(Net::IsLocal())
						{
							if(it->cmd == CMD_HURT)
								GiveDmg(GetContext(*u), nullptr, 100.f, *u);
							else if(it->cmd == CMD_BREAK_ACTION)
								BreakUnitAction(*u, BREAK_ACTION_MODE::NORMAL, true);
							else
								UnitFall(*u);
						}
						else
						{
							NetChange& c = Add1(Net::changes);
							if(it->cmd == CMD_HURT)
								c.type = NetChange::CHEAT_HURT;
							else if(it->cmd == CMD_BREAK_ACTION)
								c.type = NetChange::CHEAT_BREAK_ACTION;
							else
								c.type = NetChange::CHEAT_FALL;
							c.unit = u;
						}
					}
					break;
				case CMD_RELOAD_SHADERS:
					ReloadShaders();
					if(IsEngineShutdown())
						return;
					break;
				case CMD_TILE_INFO:
					if(L.location->outside && pc->unit->in_building == -1 && terrain->IsInside(pc->unit->pos))
					{
						OutsideLocation* outside = (OutsideLocation*)L.location;
						const TerrainTile& t = outside->tiles[pos_to_pt(pc->unit->pos)(outside->size)];
						Msg(t.GetInfo());
					}
					else
						Msg("You must stand on terrain tile.");
					break;
				case CMD_SET_SEED:
					t.Next();
					next_seed = t.MustGetUint();
					break;
				case CMD_CRASH:
					void DoCrash();
					DoCrash();
					break;
				case CMD_FORCE_QUEST:
					{
						if(t.Next())
						{
							const string& id = t.MustGetItem();
							if(!QM.SetForcedQuest(id))
								Msg("Invalid quest id '%s'.", id.c_str());
						}
						auto force = QM.GetForcedQuest();
						cstring name;
						if(force == Q_FORCE_DISABLED)
							name = "disabled";
						else if(force == Q_FORCE_NONE)
							name = "none";
						else
							name = QM.GetQuestInfos()[force].name;
						Msg("Forced quest: %s", name);
					}
					break;
				case CMD_STUN:
					{
						float length = 1.f;
						if(t.Next() && t.IsNumber())
						{
							length = t.GetFloat();
							if(length <= 0.f)
								break;
						}
						Unit* u;
						if(t.Next() && t.GetInt() == 1)
							u = pc->unit;
						else if(pc_data.selected_unit)
							u = pc_data.selected_unit;
						else
						{
							Msg("No unit selected.");
							break;
						}
						if(Net::IsLocal())
							u->ApplyStun(length);
						else
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::CHEAT_STUN;
							c.f[0] = length;
							c.unit = u;
						}
					}
					break;
				case CMD_REFRESH_COOLDOWN:
					pc->RefreshCooldown();
					if(!Net::IsLocal())
						Net::PushChange(NetChange::CHEAT_REFRESH_COOLDOWN);
					break;
				default:
					assert(0);
					break;
				}

				return;
			}
		}

		Msg("Unknown command '%s'!", token.c_str());
	}
	catch(const Tokenizer::Exception& e)
	{
		Msg("Failed to parse command: %s", e.str->c_str());
	}
}

//=================================================================================================
void Game::ShaderVersionChanged()
{
	ReloadShaders();
}

//=================================================================================================
void Game::CmdList(Tokenizer& t)
{
	if(!t.Next())
	{
		Msg("Display list of items/units/quests (item/items, itemn/item_names, unit/units, unitn/unit_names, quest/quests). Examples:");
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
		LIST_QUEST
	};

	LIST_TYPE list_type;
	const string& lis = t.MustGetItem();
	if(lis == "item" || lis == "items")
		list_type = LIST_ITEM;
	else if(lis == "itemn" || lis == "item_names")
		list_type = LIST_ITEM_NAME;
	else if(lis == "unit" || lis == "units")
		list_type = LIST_UNIT;
	else if(lis == "unitn" || lis == "unit_names")
		list_type = LIST_UNIT_NAME;
	else if(lis == "quest" || lis == "quests")
		list_type = LIST_QUEST;
	else
	{
		Msg("Unknown list type '%s'!", lis.c_str());
		return;
	}

	LocalString match;
	if(t.Next())
		match = t.MustGetText();

	switch(list_type)
	{
	case LIST_ITEM:
		{
			LocalVector2<const Item*> items;
			for(auto it : Item::items)
			{
				auto item = it.second;
				if(!IS_SET(item->flags, ITEM_SECRET) && (match.empty() || _strnicmp(match.c_str(), item->id.c_str(), match.length()) == 0))
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
			LocalVector2<const Item*> items;
			for(auto it : Item::items)
			{
				auto item = it.second;
				if(!IS_SET(item->flags, ITEM_SECRET) && (match.empty() || _strnicmp(match.c_str(), item->name.c_str(), match.length()) == 0))
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
			LocalVector2<UnitData*> units;
			for(auto unit : UnitData::units)
			{
				if(!IS_SET(unit->flags, F_SECRET) && (match.empty() || _strnicmp(match.c_str(), unit->id.c_str(), match.length()) == 0))
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
			LocalVector2<UnitData*> units;
			for(auto unit : UnitData::units)
			{
				if(!IS_SET(unit->flags, F_SECRET) && (match.empty() || _strnicmp(match.c_str(), unit->name.c_str(), match.length()) == 0))
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
			LocalVector2<const QuestInfo*> quests;
			for(auto& info : QM.GetQuestInfos())
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

			cstring quest_type[] = {
				"mayor",
				"captain",
				"random",
				"unique"
			};

			for(auto quest : quests)
			{
				cstring s2 = Format("%s (%s)", quest->name, quest_type[(int)quest->type]);
				Msg(s2);
				s += s2;
				s += "\n";
			}

			Info(s.c_str());
		}
		break;
	}
}
