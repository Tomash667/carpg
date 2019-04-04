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
#include "GlobalGui.h"
#include "GameGui.h"
#include "WorldMapGui.h"
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
#include "Arena.h"
#include "Pathfinding.h"
#include "Quest_Tournament.h"
#include "PlayerInfo.h"
#include "CommandParser.h"
#include "Render.h"

//-----------------------------------------------------------------------------
extern string g_ctime;
PrintMsgFunc g_print_func;

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
	cmds.push_back(ConsoleCommand(&N.mp_interp, "mp_interp", "interpolation interval (mp_interp 0.f-1.f)", F_MULTIPLAYER | F_WORLD_MAP | F_MP_VAR, 0.f, 1.f));
	cmds.push_back(ConsoleCommand(&N.mp_use_interp, "mp_use_interp", "set use of interpolation (mp_use_interp 0/1)", F_MULTIPLAYER | F_WORLD_MAP | F_MP_VAR));
	cmds.push_back(ConsoleCommand(&cl_postfx, "cl_postfx", "use post effects (cl_postfx 0/1)", F_ANYWHERE | F_WORLD_MAP));
	cmds.push_back(ConsoleCommand(&cl_normalmap, "cl_normalmap", "use normal mapping (cl_normalmap 0/1)", F_ANYWHERE | F_WORLD_MAP));
	cmds.push_back(ConsoleCommand(&cl_specularmap, "cl_specularmap", "use specular mapping (cl_specularmap 0/1)", F_ANYWHERE | F_WORLD_MAP));
	cmds.push_back(ConsoleCommand(&cl_glow, "cl_glow", "use glow (cl_glow 0/1)", F_ANYWHERE | F_WORLD_MAP));
	cmds.push_back(ConsoleCommand(&uv_mod, "uv_mod", "terrain uv mod (uv_mod 1-256)", F_ANYWHERE, 1, 256, VoidF(this, &Game::UvModChanged)));
	cmds.push_back(ConsoleCommand(&profiler_mode, "profiler", "profiler execution: 0-disabled, 1-update, 2-rendering", F_ANYWHERE | F_WORLD_MAP, 0, 2));
	cmds.push_back(ConsoleCommand(&settings.grass_range, "grass_range", "grass draw range", F_ANYWHERE | F_WORLD_MAP, 0.f));
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
	cmds.push_back(ConsoleCommand(CMD_REVEAL_MINIMAP, "reveal_minimap", "reveal dungeon minimap", F_GAME | F_CHEAT));
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
	cmds.push_back(ConsoleCommand(CMD_QUICKLOAD, "quickload", "load game from last slot", F_GAME | F_WORLD_MAP | F_MENU | F_SERVER));
	cmds.push_back(ConsoleCommand(CMD_RESOLUTION, "resolution", "show or change display resolution (resolution [w h hz])", F_ANYWHERE | F_WORLD_MAP));
	cmds.push_back(ConsoleCommand(CMD_QS, "qs", "pick Random character, get ready and start game", F_LOBBY));
	cmds.push_back(ConsoleCommand(CMD_CLEAR, "clear", "clear text", F_ANYWHERE | F_WORLD_MAP));
	cmds.push_back(ConsoleCommand(CMD_HURT, "hurt", "deal 100 damage to unit ('hurt 1' targets self)", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_BREAK_ACTION, "break_action", "break unit current action ('break 1' targets self)", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_FALL, "fall", "unit fall on ground for some time ('fall 1' targets self)", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_RELOAD_SHADERS, "reload_shaders", "reload shaders", F_ANYWHERE | F_WORLD_MAP));
	cmds.push_back(ConsoleCommand(CMD_TILE_INFO, "tile_info", "display info about map tile", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_SET_SEED, "set_seed", "set randomness seed", F_ANYWHERE | F_WORLD_MAP | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_CRASH, "crash", "crash game to death!", F_SINGLEPLAYER | F_LOBBY | F_MENU | F_WORLD_MAP | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_FORCE_QUEST, "force_quest", "force next random quest to select (use list quest or none/reset)", F_SERVER | F_GAME | F_WORLD_MAP | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_STUN, "stun", "stun unit for time (stun [length=1] [1 = self])", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_REFRESH_COOLDOWN, "refresh_cooldown", "refresh action cooldown/charges", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_DRAW_PATH, "draw_path", "draw debug pathfinding, look at target", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_VERIFY, "verify", "verify game state integrity", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_ADD_EFFECT, "add_effect", "add effect to selected unit (add_effect effect <value_type> power [source [perk/time]])", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_REMOVE_EFFECT, "remove_effect", "remove effect from selected unit (remove_effect effect/source [perk] [value_type])", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_LIST_EFFECTS, "list_effects", "display selected unit effects", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_ADD_PERK, "add_perk", "add perk to selected unit (add_perk perk)", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_REMOVE_PERK, "remove_perk", "remove perk from selected unit (remove_perk perk)", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_LIST_PERKS, "list_perks", "display selected unit perks", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_SELECT, "select", "select and display currently selected target (select [me/show/target] - use target or show by default)", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_LIST_STATS, "list_stats", "display selected unit stats", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_ADD_LEARNING_POINTS, "add_learning_points", "add learning point to selected unit [count - default 1]", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_CLEAN_LEVEL, "clean_level", "remove all corpses and blood from level (clean_level [building_id])", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_ARENA, "arena", "spawns enemies on arena (example arena 3 rat vs 2 wolf)", F_GAME | F_CHEAT));
	cmds.push_back(ConsoleCommand(CMD_SHADER_VERSION, "shader_version", "force shader version (shader_version 2/3)", F_ANYWHERE | F_WORLD_MAP | F_NO_ECHO));

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
void Game::ParseCommand(const string& _str, PrintMsgFunc print_func, PARSE_SOURCE source)
{
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
				Msg("Script commands can only be used inside level.");
				return;
			}

			cstring code = t.GetTextRest();
			if(Net::IsLocal())
			{
				string& output = SM.OpenOutput();
				ScriptContext& ctx = SM.GetContext();
				ctx.pc = pc;
				ctx.target = pc_data.target_unit;
				SM.RunScript(code);
				if(!output.empty())
					Msg(output.c_str());
				ctx.pc = nullptr;
				ctx.target = nullptr;
				SM.CloseOutput();
			}
			else
			{
				NetChange& c = Add1(Net::changes);
				c.type = NetChange::RUN_SCRIPT;
				c.str = StringPool.Get();
				*c.str = code;
				c.id = (pc_data.target_unit ? pc_data.target_unit->netid : -1);
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
				if(gui->server->visible)
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
						InsideLocationLevel& lvl = static_cast<InsideLocation*>(L.location)->GetLevelData();
						Tile::DebugDraw(lvl.map, Int2(lvl.w, lvl.h));
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
						if(!item)
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
								c.count = count;
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
								Team.AddGold(count);
							else
								pc->unit->gold = max(pc->unit->gold + count, 0);
						}
						else
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::CHEAT_ADD_GOLD;
							c.id = (is_team ? 1 : 0);
							c.count = count;
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
										num += pc->unit->stats->skill[co];
									int v = Clamp(num, Skill::MIN, Skill::MAX);
									pc->unit->Set((SkillId)co, v);
								}
								else
								{
									if(it->cmd == CMD_MOD_STAT)
										num += pc->unit->stats->attrib[co];
									int v = Clamp(num, Attribute::MIN, Attribute::MAX);
									pc->unit->Set((AttributeId)co, v);
								}
							}
							else
							{
								NetChange& c = Add1(Net::changes);
								c.type = (it->cmd == CMD_SET_STAT ? NetChange::CHEAT_SET_STAT : NetChange::CHEAT_MOD_STAT);
								c.id = co;
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
								if(IS_SET(cmd.flags, F_CHEAT) && !devmode)
									ok = false;

								if(!IS_ALL_SET(it->flags, F_ANYWHERE))
								{
									if(gui->server->visible)
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
						else if(data->group == G_PLAYER)
							Msg("Can't spawn player unit.");
						else
						{
							int level = -1, count = 1, in_arena = -1;
							if(t.Next())
							{
								level = t.MustGetInt();
								if(t.Next())
								{
									count = t.MustGetInt();
									if(t.Next())
										in_arena = Clamp(t.MustGetInt(), -1, 1);
								}
							}

							if(Net::IsLocal())
							{
								LevelContext& ctx = L.GetContext(*pc->unit);

								for(int i = 0; i < count; ++i)
								{
									Unit* u = L.SpawnUnitNearLocation(ctx, pc->unit->GetFrontPos(), *data, &pc->unit->pos, level);
									if(!u)
									{
										Msg("No free space for unit '%s'!", data->id.c_str());
										break;
									}
									else if(in_arena != -1)
									{
										u->in_arena = in_arena;
										arena->units.push_back(u);
									}
								}
							}
							else
							{
								NetChange& c = Add1(Net::changes);
								c.type = NetChange::CHEAT_SPAWN_UNIT;
								c.base_unit = data;
								c.count = count;
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
					if(pc_data.target_unit && pc_data.target_unit->IsAlive())
					{
						if(Net::IsLocal())
							GiveDmg(L.GetContext(*pc->unit), nullptr, pc_data.target_unit->hpmax, *pc_data.target_unit);
						else
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::CHEAT_KILL;
							c.unit = pc_data.target_unit;
						}
					}
					else
						Msg("No unit in front of player.");
					break;
				case CMD_LIST:
					CmdList(t);
					break;
				case CMD_HEAL_UNIT:
					if(pc_data.target_unit)
					{
						if(Net::IsLocal())
						{
							pc_data.target_unit->hp = pc_data.target_unit->hpmax;
							pc_data.target_unit->stamina = pc_data.target_unit->stamina_max;
							pc_data.target_unit->RemovePoison();
							pc_data.target_unit->RemoveEffect(EffectId::Stun);
							if(Net::IsOnline())
							{
								NetChange& c = Add1(Net::changes);
								c.type = NetChange::UPDATE_HP;
								c.unit = pc_data.target_unit;
								if(pc_data.target_unit->player && !pc_data.target_unit->player->is_local)
									pc_data.target_unit->player->player_info->update_flags |= PlayerInfo::UF_STAMINA;
							}
						}
						else
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::CHEAT_HEAL_UNIT;
							c.unit = pc_data.target_unit;
						}
					}
					else
						Msg("No unit in front of player.");
					break;
				case CMD_SUICIDE:
					if(Net::IsLocal())
						GiveDmg(L.GetContext(*pc->unit), nullptr, pc->unit->hpmax, *pc->unit);
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
							if(ai->unit->IsEnemy(*pc->unit) && Vec3::Distance(ai->unit->pos, pc->unit->pos) < ALERT_RANGE && L.CanSee(*ai->unit, *pc->unit))
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
						int mode = 0;
						if(t.Next())
							mode = t.MustGetInt();
						Unit* ignore = nullptr;
						if(pc_data.before_player == BP_UNIT)
							ignore = pc_data.before_player_ptr.unit;
						if(!L.KillAll(mode, *pc->unit, ignore))
							Msg("Unknown mode '%d'.", mode);
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
							GUI.CloseDialog(gui->console);
						}
						catch(const SaveException& ex)
						{
							cstring error = Format("Failed to load game: %s", ex.msg);
							Error(error);
							Msg(error);
							if(!GUI.HaveDialog(gui->console))
								GUI.ShowDialog(gui->console);
						}
					}
					else
						Msg(Net::IsClient() ? "Only server can load game." : "You can't load game in this moment.");
					break;
				case CMD_REVEAL_MINIMAP:
					if(Net::IsLocal())
						L.RevealMinimap();
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
								W.Update(count, World::UM_SKIP);
							else
							{
								NetChange& c = Add1(Net::changes);
								c.type = NetChange::CHEAT_SKIP_DAYS;
								c.id = count;
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
						if(L.city_ctx)
						{
							int index;
							InsideBuilding* building = L.city_ctx->FindInsideBuilding(group, index);
							if(building)
							{
								// wejdŸ do budynku
								if(Net::IsLocal())
								{
									fallback_type = FALLBACK::ENTER;
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
						PlayerInfo* info = N.FindPlayer(player_nick);
						if(!info)
							Msg("No player with nick '%s'.", player_nick.c_str());
						else if(t.NextLine())
						{
							const string& text = t.MustGetItem();
							if(info->pc->IsLocal())
								Msg("Whispers in your head: %s", text.c_str());
							else
							{
								BitStreamWriter f;
								f << ID_WHISPER;
								f.WriteCasted<byte>(Net::IsServer() ? Team.my_id : info->id);
								f << text;
								if(Net::IsServer())
									N.SendServer(f, MEDIUM_PRIORITY, RELIABLE, info->adr);
								else
									N.SendClient(f, MEDIUM_PRIORITY, RELIABLE);
								cstring s = Format("@%s: %s", info->name.c_str(), text.c_str());
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
						if(N.active_players > 1)
						{
							BitStreamWriter f;
							f << ID_SERVER_SAY;
							f << text;
							N.SendAll(f, MEDIUM_PRIORITY, RELIABLE);
						}
						AddServerMsg(text.c_str());
						Info("SERVER: %s", text.c_str());
						if(game_state == GS_LEVEL)
							gui->game_gui->AddSpeechBubble(pc->unit, text.c_str());
					}
					else
						Msg("You need to enter message.");
					break;
				case CMD_KICK:
					if(t.Next())
					{
						const string& player_name = t.MustGetItem();
						PlayerInfo* info = N.FindPlayer(player_name);
						if(!info)
							Msg("No player with nick '%s'.", player_name.c_str());
						else
							N.KickPlayer(*info);
					}
					else
						Msg("You need to enter player nick.");
					break;
				case CMD_READY:
					{
						PlayerInfo& info = N.GetMe();
						info.ready = !info.ready;
						gui->server->ChangeReady();
					}
					break;
				case CMD_LEADER:
					if(!t.Next())
						Msg("You need to enter leader nick.");
					else
					{
						const string& player_name = t.MustGetItem();
						PlayerInfo* info = N.FindPlayer(player_name);
						if(!info)
							Msg("No player with nick '%s'.", player_name.c_str());
						else if(Team.leader_id == info->id)
							Msg("Player '%s' is already a leader.", player_name.c_str());
						else if(!Net::IsServer() && Team.leader_id != Team.my_id)
							Msg("You can't change a leader."); // must be current leader or server
						else
						{
							if(gui->server->visible)
							{
								if(Net::IsServer())
								{
									Team.leader_id = info->id;
									gui->server->AddLobbyUpdate(Int2(Lobby_ChangeLeader, 0));
								}
								else
									Msg("You can't change a leader.");
							}
							else
							{
								NetChange& c = Add1(Net::changes);
								c.type = NetChange::CHANGE_LEADER;
								c.id = info->id;

								if(Net::IsServer())
								{
									Team.leader_id = info->id;
									Team.leader = info->u;

									if(gui->world_map->dialog_enc)
										gui->world_map->dialog_enc->bts[0].state = (Team.IsLeader() ? Button::NONE : Button::DISABLED);
								}
							}

							AddMsg(Format("Leader changed to '%s'.", info->name.c_str()));
						}
					}
					break;
				case CMD_EXIT:
					ExitToMenu();
					break;
				case CMD_RANDOM:
					if(gui->server->visible)
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
										gui->server->PickClass(ci->class_id, false);
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
							gui->server->PickClass(Class::RANDOM, false);
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
						cstring msg = gui->server->TryStart();
						if(msg)
							Msg(msg);
					}
					break;
				case CMD_SAY:
					if(t.NextLine())
					{
						const string& text = t.MustGetItem();
						BitStreamWriter f;
						f << ID_SAY;
						f.WriteCasted<byte>(Team.my_id);
						f << text;
						if(Net::IsServer())
							N.SendAll(f, MEDIUM_PRIORITY, RELIABLE);
						else
							N.SendClient(f, MEDIUM_PRIORITY, RELIABLE);
						cstring s = Format("%s: %s", N.GetMe().name.c_str(), text.c_str());
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
								for(PlayerInfo* info : N.players)
								{
									if(info->left == PlayerInfo::LEFT_NO && info->devmode != b && info->id != 0)
									{
										info->devmode = b;
										NetChangePlayer& c = Add1(info->pc->player_info->changes);
										c.type = NetChangePlayer::DEVMODE;
										c.id = (b ? 1 : 0);
									}
								}
							}
							else
							{
								PlayerInfo* info = N.FindPlayer(player_name);
								if(!info)
									Msg("No player with nick '%s'.", player_name.c_str());
								else if(info->devmode != b)
								{
									info->devmode = b;
									NetChangePlayer& c = Add1(info->pc->player_info->changes);
									c.type = NetChangePlayer::DEVMODE;
									c.id = (b ? 1 : 0);
								}
							}
						}
						else
						{
							if(player_name == "all")
							{
								LocalString s = "Players devmode: ";
								bool any = false;
								for(PlayerInfo* info : N.players)
								{
									if(info->left == PlayerInfo::LEFT_NO && info->id != 0)
									{
										s += Format("%s(%d), ", info->name.c_str(), info->devmode ? 1 : 0);
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
								PlayerInfo* info = N.FindPlayer(player_name);
								if(!info)
									Msg("No player with nick '%s'.", player_name.c_str());
								else
									Msg("Player devmode: %s(%d).", player_name.c_str(), info->devmode ? 1 : 0);
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
						int result = GetRender()->SetMultisampling(type, level);
						if(result == 2)
						{
							int ms, msq;
							GetRender()->GetMultisampling(ms, msq);
							Msg("Changed multisampling to %d, %d.", ms, msq);
						}
						else
							Msg(result == 1 ? "This mode is already set." : "This mode is unavailable.");
					}
					else
					{
						int ms, msq;
						GetRender()->GetMultisampling(ms, msq);
						Msg("multisampling = %d, %d", ms, msq);
					}
					break;
				case CMD_QUICKSAVE:
					Quicksave(true);
					break;
				case CMD_QUICKLOAD:
					Quickload(true);
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
						vector<Resolution> resolutions;
						GetRender()->GetResolutions(resolutions);
						for(const Resolution& res : resolutions)
						{
							if(w == res.size.x)
							{
								if(pick_h)
								{
									if((int)res.size.y >= h)
									{
										h = res.size.y;
										if((int)res.hz > hz)
											hz = res.hz;
										valid = true;
									}
								}
								else if(h == res.size.y)
								{
									if(pick_hz)
									{
										if((int)res.hz > hz)
											hz = res.hz;
										valid = true;
									}
									else if(hz == res.hz)
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
						LocalString s = Format("Current resolution %dx%d (%d Hz). Available: ",
							GetWindowSize().x, GetWindowSize().y, GetRender()->GetRefreshRate());
						vector<Resolution> resolutions;
						GetRender()->GetResolutions(resolutions);
						for(const Resolution& res : resolutions)
							s += Format("%dx%d(%d), ", res.size.x, res.size.y, res.hz);
						s.pop(2u);
						Msg(s);
					}
					break;
				case CMD_QS:
					if(!gui->server->Quickstart())
						Msg("Not everyone is ready.");
					break;
				case CMD_CLEAR:
					switch(source)
					{
					case PS_CONSOLE:
						gui->console->Reset();
						break;
					case PS_CHAT:
						gui->mp_box->itb.Reset();
						break;
					case PS_LOBBY:
						gui->server->itb.Reset();
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
						else if(pc_data.target_unit)
							u = pc_data.target_unit;
						else
						{
							Msg("No unit in front of player.");
							break;
						}
						if(Net::IsLocal())
						{
							if(it->cmd == CMD_HURT)
								GiveDmg(L.GetContext(*u), nullptr, 100.f, *u);
							else if(it->cmd == CMD_BREAK_ACTION)
								u->BreakAction(Unit::BREAK_ACTION_MODE::NORMAL, true);
							else
								u->Fall();
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
					if(L.location->outside && pc->unit->in_building == -1 && L.terrain->IsInside(pc->unit->pos))
					{
						OutsideLocation* outside = static_cast<OutsideLocation*>(L.location);
						const TerrainTile& t = outside->tiles[PosToPt(pc->unit->pos)(outside->size)];
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
						int force = QM.GetForcedQuest();
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
						else if(pc_data.target_unit)
							u = pc_data.target_unit;
						else
						{
							Msg("No unit in front of player.");
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
				case CMD_DRAW_PATH:
					if(pc_data.before_player == BP_UNIT)
					{
						pathfinding->SetTarget(pc_data.before_player_ptr.unit);
						Msg("Set draw path target.");
					}
					else
					{
						pathfinding->SetTarget(nullptr);
						Msg("Removed draw path target.");
					}
					break;
				case CMD_VERIFY:
					W.VerifyObjects();
					break;
				case CMD_ADD_EFFECT:
					if(!t.Next())
					{
						Msg("Use 'list effect' to get list of existing effects. Some examples:");
						Msg("add_effect regeneration 5 - add permanent regeneration 5 hp/sec");
						Msg("add_effect melee_attack 30 perk strong_back - add 30 melee attack assigned to perk");
						Msg("add_effect magic_resistance 0.5 temporary 30 - add 50% magic resistance for 30 seconds");
						Msg("add_effect attribute str 5 temporary 10 - add +5 strength for 10 seconds");
					}
					else
					{
						Effect e;

						const string& effect_id = t.MustGetItem();
						e.effect = EffectInfo::TryGet(effect_id);
						if(e.effect == EffectId::None)
							t.Throw("Invalid effect '%s'.", effect_id.c_str());
						t.Next();

						EffectInfo& info = EffectInfo::effects[(int)e.effect];
						if(info.value_type == EffectInfo::None)
							e.value = -1;
						else
						{
							const string& value = t.MustGetItem();
							if(info.value_type == EffectInfo::Attribute)
							{
								Attribute* attrib = Attribute::Find(value);
								if(!attrib)
									t.Throw("Invalid attribute '%s' for effect '%s'.", value.c_str(), info.id);
								e.value = (int)attrib->attrib_id;
							}
							else
							{
								Skill* skill = Skill::Find(value);
								if(!skill)
									t.Throw("Invalid skill '%s' for effect '%s'.", value.c_str(), info.id);
								e.value = (int)skill->skill_id;
							}
							t.Next();
						}

						e.power = t.MustGetNumberFloat();
						if(t.Next())
						{
							const string& source_name = t.MustGetItem();
							if(source_name == "permanent")
							{
								e.source = EffectSource::Permanent;
								e.source_id = -1;
								e.time = 0;
							}
							else if(source_name == "perk")
							{
								e.source = EffectSource::Perk;
								t.Next();
								const string& perk_id = t.MustGetItem();
								PerkInfo* perk = PerkInfo::Find(perk_id);
								if(perk == nullptr)
									t.Throw("Invalid perk source '%s'.", perk_id.c_str());
								e.source_id = (int)perk->perk_id;
								e.time = 0;
							}
							else if(source_name == "temporary")
							{
								e.source = EffectSource::Temporary;
								e.source_id = -1;
								t.Next();
								e.time = t.MustGetNumberFloat();
							}
							else
								t.Throw("Invalid effect source '%s'.", source_name.c_str());
						}
						else
						{
							e.source = EffectSource::Permanent;
							e.source_id = -1;
							e.time = 0;
						}

						if(Net::IsLocal())
							pc_data.selected_unit->AddEffect(e);
						else
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::GENERIC_CMD;
							c << (byte)CMD_ADD_EFFECT
								<< pc_data.selected_unit->netid
								<< (char)e.effect
								<< (char)e.source
								<< (char)e.source_id
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
						int source_id = -1;
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
								const string& perk_name = t.MustGetItem();
								PerkInfo* info = PerkInfo::Find(perk_name);
								if(info != nullptr)
								{
									source_id = (int)info->perk_id;
									t.Next();
								}
							}
						}

						if(t.IsItem())
						{
							const string& effect_name = t.MustGetItem();
							effect = EffectInfo::TryGet(effect_name);
							if(effect == EffectId::None)
							{
								Msg("Invalid effect or source '%s'.", effect_name.c_str());
								break;
							}

							EffectInfo& info = EffectInfo::effects[(int)effect];
							if(info.value_type != EffectInfo::None && t.Next())
							{
								const string& value_str = t.MustGetItem();
								if(info.value_type == EffectInfo::Attribute)
								{
									Attribute* attrib = Attribute::Find(value_str);
									if(!attrib)
									{
										Msg("Invalid effect attribute '%s'.", value_str.c_str());
										break;
									}
									value = (int)attrib->attrib_id;
								}
								else
								{
									Skill* skill = Skill::Find(value_str);
									if(!skill)
									{
										Msg("Invalid effect skill '%s'.", value_str.c_str());
										break;
									}
									value = (int)skill->skill_id;
								}
							}
						}

						if(Net::IsLocal())
							cmdp->RemoveEffect(pc_data.selected_unit, effect, source, source_id, value);
						else
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::GENERIC_CMD;
							c << (byte)CMD_REMOVE_EFFECT
								<< pc_data.selected_unit->netid
								<< (char)effect
								<< (char)source
								<< (char)source_id
								<< (char)value;
						}
					}
					break;
				case CMD_LIST_EFFECTS:
					if(Net::IsLocal() || pc_data.selected_unit->IsLocal())
						cmdp->ListEffects(pc_data.selected_unit);
					else
					{
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::GENERIC_CMD;
						c << (byte)CMD_LIST_EFFECTS
							<< pc_data.selected_unit->netid;
					}
					break;
				case CMD_ADD_PERK:
					if(!pc_data.selected_unit->player)
						Msg("Only players have perks.");
					else if(!t.Next())
						Msg("Perk name required. Use 'list perks' to display list.");
					else
					{
						const string& name = t.MustGetItem();
						PerkInfo* info = PerkInfo::Find(name);
						if(!info)
						{
							Msg("Invalid perk '%s'.", name.c_str());
							break;
						}

						int value = -1;
						if(info->value_type == PerkInfo::Attribute)
						{
							if(!t.Next())
							{
								Msg("Perk '%s' require 'attribute' value.", info->id);
								break;
							}
							const string& attrib_name = t.MustGetItem();
							Attribute* attrib = Attribute::Find(attrib_name);
							if(!attrib)
							{
								Msg("Invalid attribute '%s' for perk '%s'.", attrib_name.c_str(), info->id);
								break;
							}
							value = (int)attrib->attrib_id;
						}
						else if(info->value_type == PerkInfo::Skill)
						{
							if(!t.Next())
							{
								Msg("Perk '%s' require 'skill' value.", info->id);
								break;
							}
							const string& skill_name = t.MustGetItem();
							Skill* skill = Skill::Find(skill_name);
							if(!skill)
							{
								Msg("Invalid skill '%s' for perk '%s'.", skill_name.c_str(), info->id);
								break;
							}
							value = (int)skill->skill_id;
						}

						if(Net::IsLocal())
							cmdp->AddPerk(pc_data.selected_unit->player, info->perk_id, value);
						else
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::GENERIC_CMD;
							c << (byte)CMD_ADD_PERK
								<< pc_data.selected_unit->netid
								<< (char)info->perk_id
								<< (char)value;
						}
					}
					break;
				case CMD_REMOVE_PERK:
					if(!pc_data.selected_unit->player)
						Msg("Only players have perks.");
					else if(!t.Next())
						Msg("Perk name required. Use 'list perks' to display list.");
					else
					{
						const string& name = t.MustGetItem();
						PerkInfo* info = PerkInfo::Find(name);
						if(!info)
						{
							Msg("Invalid perk '%s'.", name.c_str());
							break;
						}

						int value = -1;
						if(info->value_type == PerkInfo::Attribute)
						{
							if(!t.Next())
							{
								Msg("Perk '%s' require 'attribute' value.", info->id);
								break;
							}
							const string& attrib_name = t.MustGetItem();
							Attribute* attrib = Attribute::Find(attrib_name);
							if(!attrib)
							{
								Msg("Invalid attribute '%s' for perk '%s'.", attrib_name.c_str(), info->id);
								break;
							}
							value = (int)attrib->attrib_id;
						}
						else if(info->value_type == PerkInfo::Skill)
						{
							if(!t.Next())
							{
								Msg("Perk '%s' require 'skill' value.", info->id);
								break;
							}
							const string& skill_name = t.MustGetItem();
							Skill* skill = Skill::Find(skill_name);
							if(!skill)
							{
								Msg("Invalid skill '%s' for perk '%s'.", skill_name.c_str(), info->id);
								break;
							}
							value = (int)skill->skill_id;
						}

						if(Net::IsLocal())
							cmdp->RemovePerk(pc_data.selected_unit->player, info->perk_id, value);
						else
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::GENERIC_CMD;
							c << (byte)CMD_REMOVE_PERK
								<< pc_data.selected_unit->netid
								<< (char)info->perk_id
								<< (char)value;
						}
					}
					break;
				case CMD_LIST_PERKS:
					if(!pc_data.selected_unit->player)
						Msg("Only players have perks.");
					else if(Net::IsLocal() || pc_data.selected_unit->IsLocal())
						cmdp->ListPerks(pc_data.selected_unit->player);
					else
					{
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::GENERIC_CMD;
						c << (byte)CMD_LIST_PERKS
							<< pc_data.selected_unit->netid;
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
						else if(pc_data.before_player == BP_UNIT)
							select = SELECT_TARGET;
						else
							select = SELECT_SHOW;

						if(select == SELECT_ME)
							pc_data.selected_unit = pc->unit;
						else if(select == SELECT_TARGET)
						{
							if(!pc_data.target_unit)
							{
								Msg("No unit in front of player.");
								break;
							}
							pc_data.selected_unit = pc_data.target_unit;
						}
						Msg("Currently selected: %s %p [%d]", pc_data.selected_unit->data->id.c_str(), pc_data.selected_unit,
							Net::IsOnline() ? pc_data.selected_unit->netid : -1);
					}
					break;
				case CMD_LIST_STATS:
					if(Net::IsLocal() || pc_data.selected_unit->IsLocal())
						cmdp->ListStats(pc_data.selected_unit);
					else
					{
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::GENERIC_CMD;
						c << (byte)CMD_LIST_STATS
							<< pc_data.selected_unit->netid;
					}
					break;
				case CMD_ADD_LEARNING_POINTS:
					if(!pc_data.selected_unit->IsPlayer())
						Msg("Only players have learning points.");
					else
					{
						int count = 1;
						if(t.Next())
							count = t.MustGetInt();
						if(count < 1)
							break;
						if(Net::IsLocal())
							pc_data.selected_unit->player->AddLearningPoint(count);
						else
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::GENERIC_CMD;
							c << (byte)CMD_ADD_LEARNING_POINTS
								<< pc_data.selected_unit->netid
								<< count;
						}
					}
					break;
				case CMD_CLEAN_LEVEL:
					{
						int building_id = -2;
						if(t.Next())
							building_id = t.MustGetInt();
						if(Net::IsLocal())
							L.CleanLevel(building_id);
						else
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::CLEAN_LEVEL;
							c.id = building_id;
						}
					}
					break;
				case CMD_ARENA:
					if(!L.HaveArena())
						Msg("Arena required inside location.");
					else
					{
						cstring s = t.GetTextRest();
						if(Net::IsLocal())
							cmdp->ArenaCombat(s);
						else
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::CHEAT_ARENA;
							c.str = StringPool.Get();
							*c.str = s;
						}
					}
					break;
				case CMD_SHADER_VERSION:
					if(!t.Next() || !t.IsInt())
						Msg("shader_version: %d", GetRender()->GetShaderVersion());
					else
					{
						int value = t.GetInt();
						if(value != 2 && value != 3)
							Msg("Invalid shader version, must be 2 or 3.");
						else
						{
							GetRender()->SetShaderVersion(value);
							Msg("shader_version: %d", value);
							ReloadShaders();
						}
					}
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
void Game::CmdList(Tokenizer& t)
{
	if(!t.Next())
	{
		Msg("Display list of items/units/quests (item/items, itemn/item_names, unit/units, unitn/unit_names, quest(s), effect(s), perk(s)). Examples:");
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
	else if(lis == "effect" || lis == "effects")
		list_type = LIST_EFFECT;
	else if(lis == "perk" || lis == "perks")
		list_type = LIST_PERK;
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
			LocalVector2<const Item*> items;
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
	case LIST_EFFECT:
		{
			LocalVector2<const EffectInfo*> effects;
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
			LocalVector2<const PerkInfo*> perks;
			for(PerkInfo& info : PerkInfo::perks)
			{
				if(match.empty() || _strnicmp(match.c_str(), info.id, match.length()) == 0)
					perks.push_back(&info);
			}

			if(perks.empty())
			{
				Msg("No perks found starting with '%s'.", match.c_str());
				return;
			}

			std::sort(perks.begin(), perks.end(), [](const PerkInfo* perk1, const PerkInfo* perk2)
			{
				return strcoll(perk1->id, perk2->id) < 0;
			});

			LocalString s = Format("Perks list (%d):\n", perks.size());
			Msg("Perks list (%d):", perks.size());

			for(const PerkInfo* perk : perks)
			{
				cstring s2 = Format("%s (%s, %s)", perk->id, perk->name.c_str(), perk->desc.c_str());
				Msg(s2);
				s += s2;
				s += "\n";
			}

			Info(s.c_str());
		}
		break;
	}
}
