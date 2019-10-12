#include "Pch.h"
#include "GameCore.h"
#include "Game.h"
#include "Language.h"
#include "Terrain.h"
#include "Version.h"
#include "City.h"
#include "InsideLocation.h"
#include "LevelGui.h"
#include "MainMenu.h"
#include "GameMenu.h"
#include "MultiplayerPanel.h"
#include "Options.h"
#include "Controls.h"
#include "SaveLoadPanel.h"
#include "GetTextDialog.h"
#include "CreateServerPanel.h"
#include "CreateCharacterPanel.h"
#include "PickServerPanel.h"
#include "ServerPanel.h"
#include "InfoBox.h"
#include "LoadScreen.h"
#include "WorldMapGui.h"
#include "MpBox.h"
#include "AIController.h"
#include "BitStreamFunc.h"
#include "Team.h"
#include "SaveState.h"
#include "SoundManager.h"
#include "BitStreamFunc.h"
#include "Portal.h"
#include "EntityInterpolator.h"
#include "World.h"
#include "Level.h"
#include "QuestManager.h"
#include "Quest_Tutorial.h"
#include "GameGui.h"
#include "PlayerInfo.h"
#include "LobbyApi.h"
#include "Render.h"
#include "GameMessages.h"
#include "Engine.h"
#include "GameResources.h"

// consts
const float T_TRY_CONNECT = 5.f;
const int T_SHUTDOWN = 1000;
const float T_WAIT_FOR_DATA = 5.f;

//=================================================================================================
bool Game::CanShowMenu()
{
	return !gui->HaveDialog() && !game_gui->level_gui->HavePanelOpen() && !game_gui->main_menu->visible && game_state != GS_MAIN_MENU && death_screen != 3
		&& !end_of_game && !dialog_context.dialog_mode;
}

//=================================================================================================
void Game::SaveOptions()
{
	cfg.Add("fullscreen", engine->IsFullscreen());
	cfg.Add("cl_glow", cl_glow);
	cfg.Add("cl_normalmap", cl_normalmap);
	cfg.Add("cl_specularmap", cl_specularmap);
	cfg.Add("sound_volume", sound_mgr->GetSoundVolume());
	cfg.Add("music_volume", sound_mgr->GetMusicVolume());
	cfg.Add("mouse_sensitivity", settings.mouse_sensitivity);
	cfg.Add("grass_range", settings.grass_range);
	cfg.Add("resolution", engine->GetWindowSize());
	cfg.Add("refresh", render->GetRefreshRate());
	cfg.Add("skip_tutorial", skip_tutorial);
	cfg.Add("language", Language::prefix);
	int ms, msq;
	render->GetMultisampling(ms, msq);
	cfg.Add("multisampling", ms);
	cfg.Add("multisampling_quality", msq);
	cfg.Add("vsync", render->IsVsyncEnabled());
	SaveCfg();
}

//=================================================================================================
void Game::StartNewGame()
{
	HumanData hd;
	hd.Get(*game_gui->create_character->unit->human_data);
	NewGameCommon(game_gui->create_character->clas, game_gui->create_character->player_name.c_str(), hd, game_gui->create_character->cc, false);
}

//=================================================================================================
void Game::StartQuickGame()
{
	Net::SetMode(Net::Mode::Singleplayer);

	Class* clas = nullptr;
	const string& class_id = cfg.GetString("quickstart_class");
	if(!class_id.empty())
	{
		clas = Class::TryGet(class_id);
		if(clas)
		{
			if(!clas->IsPickable())
			{
				Warn("Settings [quickstart_class]: Class '%s' is not pickable by players.", class_id.c_str());
				clas = nullptr;
			}
		}
		else
			Warn("Settings [quickstart_class]: Invalid class '%s'.", class_id.c_str());
	}
	string quickstart_name = cfg.GetString("quickstart_name", "Test");
	if(quickstart_name.empty())
		quickstart_name = "Test";
	HumanData hd;
	CreatedCharacter cc;
	int hair_index;

	RandomCharacter(clas, hair_index, hd, cc);
	NewGameCommon(clas, quickstart_name.c_str(), hd, cc, false);
}

//=================================================================================================
void Game::NewGameCommon(Class* clas, cstring name, HumanData& hd, CreatedCharacter& cc, bool tutorial)
{
	ClearGameVars(true);
	game_level->entering = true;
	quest_mgr->quest_tutorial->in_tutorial = tutorial;
	game_gui->main_menu->visible = false;
	Net::SetMode(Net::Mode::Singleplayer);
	game_state = GS_LEVEL;
	hardcore_mode = hardcore_option;

	UnitData& ud = *clas->player;

	Unit* u = CreateUnit(ud, -1, nullptr, nullptr, false, true);
	u->area = nullptr;
	u->ApplyHumanData(hd);
	team->members.clear();
	team->active_members.clear();
	team->members.push_back(u);
	team->active_members.push_back(u);
	team->leader = u;

	u->player = new PlayerController;
	pc = u->player;
	pc->id = 0;
	pc->Init(*u);
	pc->name = name;
	pc->is_local = true;
	pc->unit->RecalculateWeight();
	pc->dialog_ctx = &dialog_context;
	pc->dialog_ctx->dialog_mode = false;
	pc->dialog_ctx->pc = pc;
	pc->dialog_ctx->is_local = true;
	cc.Apply(*pc);
	if(quest_mgr->quest_tutorial->finished_tutorial)
	{
		u->AddItem(Item::Get("book_adventurer"), 1u, false);
		quest_mgr->quest_tutorial->finished_tutorial = false;
	}
	dialog_context.pc = pc;

	team->CalculatePlayersLevel();
	game_gui->Setup(pc);

	if(!tutorial && cc.HavePerk(Perk::Leader))
	{
		Unit* npc = CreateUnit(*Class::GetRandomHeroData(), -1, nullptr, nullptr, false);
		npc->area = nullptr;
		npc->ai = new AIController;
		npc->ai->Init(npc);
		npc->hero->know_name = true;
		team->AddTeamMember(npc, HeroType::Normal);
		--team->free_recruits;
		npc->hero->SetupMelee();
	}
	game_gui->level_gui->Setup();

	fallback_type = FALLBACK::NONE;
	fallback_t = -0.5f;
	ChangeTitle();

	if(!tutorial)
	{
		GenerateWorld();
		quest_mgr->InitQuests();
		world->StartInLocation();
		game_res->LoadCommonMusic();
		EnterLocation();
	}
}

//=================================================================================================
void Game::MultiplayerPanelEvent(int id)
{
	player_name = Trimmed(game_gui->multiplayer->textbox.GetText());

	// check nick
	if(player_name.empty())
	{
		gui->SimpleDialog(game_gui->multiplayer->txNeedEnterNick, game_gui->multiplayer);
		return;
	}
	if(!net->ValidateNick(player_name.c_str()))
	{
		gui->SimpleDialog(game_gui->multiplayer->txEnterValidNick, game_gui->multiplayer);
		return;
	}

	// save nick
	cfg.Add("nick", player_name);
	SaveCfg();

	switch(id)
	{
	case MultiplayerPanel::IdJoinLan:
		game_gui->pick_server->Show();
		break;
	case MultiplayerPanel::IdJoinIp:
		{
			GetTextDialogParams params(txEnterIp, server_ip);
			params.event = DialogEvent(this, &Game::OnEnterIp);
			params.limit = 100;
			params.parent = game_gui->multiplayer;
			GetTextDialog::Show(params);
		}
		break;
	case MultiplayerPanel::IdCreate:
		game_gui->create_server->Show();
		break;
	case MultiplayerPanel::IdLoad:
		net->mp_load = true;
		net->mp_quickload = false;
		Net::changes.clear();
		if(!net->net_strs.empty())
			StringPool.Free(net->net_strs);
		game_gui->saveload->ShowLoadPanel();
		break;
	}
}

//=================================================================================================
void Game::CreateServerEvent(int id)
{
	if(id == BUTTON_CANCEL)
	{
		if(net->mp_load)
		{
			ClearGame();
			net->mp_load = false;
		}
		game_gui->create_server->CloseDialog();
	}
	else
	{
		// copy settings
		net->server_name = Trimmed(game_gui->create_server->textbox[0].GetText());
		net->max_players = atoi(game_gui->create_server->textbox[1].GetText().c_str());
		net->password = game_gui->create_server->textbox[2].GetText();
		net->server_lan = game_gui->create_server->checkbox.checked;

		// check settings
		cstring error_text = nullptr;
		Control* give_focus = nullptr;
		if(net->server_name.empty())
		{
			error_text = game_gui->create_server->txEnterServerName;
			give_focus = &game_gui->create_server->textbox[0];
		}
		else if(!InRange(net->max_players, MIN_PLAYERS, MAX_PLAYERS))
		{
			error_text = Format(game_gui->create_server->txInvalidPlayersCount, MAX_PLAYERS);
			give_focus = &game_gui->create_server->textbox[1];
		}
		if(error_text)
		{
			gui->SimpleDialog(error_text, game_gui->create_server);
			game_gui->create_server->cont.give_focus = give_focus;
			return;
		}

		// save settings
		cfg.Add("server_name", net->server_name);
		cfg.Add("server_pswd", net->password);
		cfg.Add("server_players", net->max_players);
		cfg.Add("server_lan", net->server_lan);
		SaveCfg();

		// close dialog windows
		game_gui->create_server->CloseDialog();
		game_gui->multiplayer->CloseDialog();

		try
		{
			net->InitServer();
		}
		catch(cstring err)
		{
			gui->SimpleDialog(err, game_gui->main_menu);
			return;
		}

		net->OnNewGameServer();
	}
}

//=================================================================================================
void Game::RandomCharacter(Class*& clas, int& hair_index, HumanData& hd, CreatedCharacter& cc)
{
	if(!clas)
		clas = Class::GetRandomPlayer();
	// appearance
	hd.beard = Rand() % MAX_BEARD - 1;
	hd.hair = Rand() % MAX_HAIR - 1;
	hd.mustache = Rand() % MAX_MUSTACHE - 1;
	hair_index = Rand() % n_hair_colors;
	hd.hair_color = g_hair_colors[hair_index];
	hd.height = Random(0.95f, 1.05f);
	// created character
	cc.Random(clas);
}

//=================================================================================================
void Game::OnEnterIp(int id)
{
	if(id == BUTTON_OK)
	{
		SystemAddress adr = UNASSIGNED_SYSTEM_ADDRESS;
		Trim(server_ip);
		if(adr.FromString(server_ip.c_str(), ':') && adr != UNASSIGNED_SYSTEM_ADDRESS)
		{
			// save ip
			cfg.Add("server_ip", server_ip.c_str());
			SaveCfg();

			try
			{
				net->InitClient();
			}
			catch(cstring error)
			{
				gui->SimpleDialog(error, game_gui->multiplayer);
				return;
			}

			Info("Pinging %s...", adr.ToString());
			game_gui->info_box->Show(txConnecting);
			net_mode = NM_CONNECTING;
			net_state = NetState::Client_PingIp;
			net_timer = T_CONNECT_PING;
			net_tries = I_CONNECT_TRIES;
			net->ping_adr = adr;
			if(net->ping_adr.GetPort() == 0)
				net->ping_adr.SetPortHostOrder(net->port);
			net->peer->Ping(net->ping_adr.ToString(false), net->ping_adr.GetPort(), false);
		}
		else
			gui->SimpleDialog(txInvalidIp, game_gui->multiplayer);
	}
}

//=================================================================================================
void Game::EndConnecting(cstring msg, bool wait)
{
	game_gui->info_box->CloseDialog();
	if(msg)
		gui->SimpleDialog(msg, game_gui->pick_server->visible ? (DialogBox*)game_gui->pick_server : (DialogBox*)game_gui->multiplayer);
	if(wait)
		ForceRedraw();
	if(!game_gui->pick_server->visible)
		net->ClosePeer(wait);
}

//=================================================================================================
void Game::GenericInfoBoxUpdate(float dt)
{
	switch(net_mode)
	{
	case NM_CONNECTING:
		UpdateClientConnectingIp(dt);
		break;
	case NM_TRANSFER:
		UpdateClientTransfer(dt);
		break;
	case NM_QUITTING:
		UpdateClientQuiting(dt);
		break;
	case NM_TRANSFER_SERVER:
		UpdateServerTransfer(dt);
		break;
	case NM_SERVER_SEND:
		UpdateServerSend(dt);
		break;
	case NM_QUITTING_SERVER:
		UpdateServerQuiting(dt);
		break;
	}
}

void Game::UpdateClientConnectingIp(float dt)
{
	if(net_state == NetState::Client_PingIp)
	{
		net_timer -= dt;
		if(net_timer <= 0.f)
		{
			if(net_tries == 0)
			{
				// connection timeout
				Warn("NM_CONNECTING(0): Connection timeout.");
				EndConnecting(txConnectTimeout);
				return;
			}
			else
			{
				// ping another time...
				Info("Pinging %s...", net->ping_adr.ToString());
				net->peer->Ping(net->ping_adr.ToString(false), net->ping_adr.GetPort(), false);
				--net_tries;
				net_timer = T_CONNECT_PING;
			}
		}

		// waiting for connection
		Packet* packet;
		for(packet = net->peer->Receive(); packet; net->peer->DeallocatePacket(packet), packet = net->peer->Receive())
		{
			BitStreamReader f(packet);
			byte msg_id;
			f >> msg_id;

			if(msg_id != ID_UNCONNECTED_PONG)
			{
				// unknown packet from server
				Warn("NM_CONNECTING(0): Unknown server response: %u.", msg_id);
				continue;
			}

			if(packet->length == sizeof(TimeMS) + 1)
			{
				Warn("NM_CONNECTING(0): Server not set SetOfflinePingResponse yet.");
				continue;
			}

			// packet structure
			// 0 char C
			// 1 char A
			// 2-5 int - version
			// 6 byte - players
			// 7 byte - max players
			// 8 byte - flags (0x01 - password, 0x02 - loaded game)
			// 9+ byte - name

			// header
			TimeMS time_ms;
			char sign_ca[2];
			f >> time_ms;
			f >> sign_ca;
			if(!f)
			{
				Warn("NM_CONNECTING(0): Broken server response.");
				continue;
			}
			if(sign_ca[0] != 'C' || sign_ca[1] != 'A')
			{
				// invalid signature, this is not carpg server
				Warn("NM_CONNECTING(0): Invalid server signature 0x%x%x.", byte(sign_ca[0]), byte(sign_ca[1]));
				net->peer->DeallocatePacket(packet);
				EndConnecting(txConnectInvalid);
				return;
			}

			// read data
			uint version;
			byte players, max_players, flags;
			f >> version;
			f >> players;
			f >> max_players;
			f >> flags;
			const string& server_name_r = f.ReadString1();
			if(!f)
			{
				Error("NM_CONNECTING(0): Broken server message.");
				net->peer->DeallocatePacket(packet);
				EndConnecting(txConnectInvalid);
				return;
			}

			net->peer->DeallocatePacket(packet);

			// check version
			if(version != VERSION)
			{
				// version mismatch
				cstring s = VersionToString(version);
				Error("NM_CONNECTING(0): Invalid client version '%s' vs server '%s'.", VERSION_STR, s);
				net->peer->DeallocatePacket(packet);
				EndConnecting(Format(txConnectVersion, VERSION_STR, s));
				return;
			}

			// set server status
			game_gui->server->max_players = max_players;
			game_gui->server->server_name = server_name_r;
			Info("NM_CONNECTING(0): Server information. Name:%s; players:%d/%d; flags:%d.", server_name_r.c_str(), players, max_players, flags);
			if(IsSet(flags, 0xFC))
				Warn("NM_CONNECTING(0): Unknown server flags.");
			net->server = packet->systemAddress;
			enter_pswd.clear();

			if(IsSet(flags, 0x01))
			{
				// password is required
				net_state = NetState::Client_WaitingForPassword;
				game_gui->info_box->Show(txWaitingForPswd);
				GetTextDialogParams params(Format(txEnterPswd, server_name_r.c_str()), enter_pswd);
				params.event = DialogEvent(this, &Game::OnEnterPassword);
				params.limit = 16;
				params.parent = game_gui->info_box;
				GetTextDialog::Show(params);
				Info("NM_CONNECTING(0): Waiting for password...");
			}
			else
			{
				// try to connect
				ConnectionAttemptResult result = net->peer->Connect(net->ping_adr.ToString(false), net->ping_adr.GetPort(), nullptr, 0);
				if(result == CONNECTION_ATTEMPT_STARTED)
				{
					// connecting...
					net_timer = T_CONNECT;
					net_state = NetState::Client_Connecting;
					game_gui->info_box->Show(Format(txConnectingTo, server_name_r.c_str()));
					Info("NM_CONNECTING(0): Connecting to server %s...", server_name_r.c_str());
				}
				else
				{
					// something went wrong
					Error("NM_CONNECTING(0): Can't connect to server: SLikeNet error %d.", result);
					EndConnecting(txConnectSLikeNet);
				}
			}

			return;
		}
	}
	else if(net_state == NetState::Client_Connecting)
	{
		Packet* packet;
		for(packet = net->peer->Receive(); packet; net->peer->DeallocatePacket(packet), packet = net->peer->Receive())
		{
			// ignore messages from proxy (disconnect notification)
			if(packet->systemAddress != net->server)
				continue;

			BitStreamReader reader(packet);
			byte msg_id;
			reader >> msg_id;

			switch(msg_id)
			{
			case ID_CONNECTION_REQUEST_ACCEPTED:
				{
					// server accepted connecting, send welcome message
					// byte - ID_HELLO
					// int - version
					// crc
					// string1 - nick
					Info("NM_CONNECTING(2): Connected with server.");
					BitStreamWriter f;
					f << ID_HELLO;
					f << VERSION;
					content.WriteCrc(f);
					f << player_name;
					net->SendClient(f, IMMEDIATE_PRIORITY, RELIABLE);
				}
				break;
			case ID_JOIN:
				// server accepted and sent info about players and my id
				{
					int count, load_char;
					reader.ReadCasted<byte>(team->my_id);
					reader.ReadCasted<byte>(net->active_players);
					reader.ReadCasted<byte>(team->leader_id);
					reader.ReadCasted<byte>(count);
					if(!reader)
					{
						Error("NM_CONNECTING(2): Broken packet ID_JOIN.");
						net->peer->DeallocatePacket(packet);
						EndConnecting(txCantJoin, true);
						return;
					}

					// add player
					DeleteElements(net->players);
					PlayerInfo* pinfo = new PlayerInfo;
					net->players.push_back(pinfo);
					PlayerInfo& info = *pinfo;
					info.clas = nullptr;
					info.ready = false;
					info.name = player_name;
					info.id = team->my_id;
					info.state = PlayerInfo::IN_LOBBY;
					info.update_flags = 0;
					info.left = PlayerInfo::LEFT_NO;
					info.loaded = false;

					// read other players
					string class_id;
					for(int i = 0; i < count; ++i)
					{
						pinfo = new PlayerInfo;
						net->players.push_back(pinfo);
						PlayerInfo& info2 = *pinfo;
						info2.state = PlayerInfo::IN_LOBBY;
						info2.left = PlayerInfo::LEFT_NO;
						info2.loaded = false;

						reader.ReadCasted<byte>(info2.id);
						reader >> info2.ready;
						reader >> class_id;
						reader >> info2.name;
						if(!reader)
						{
							Error("NM_CONNECTING(2): Broken packet ID_JOIN(2).");
							net->peer->DeallocatePacket(packet);
							EndConnecting(txCantJoin, true);
							return;
						}

						// verify player class
						if(class_id.empty())
							info2.clas = nullptr;
						else
						{
							info2.clas = Class::TryGet(class_id);
							if(!info2.clas || !info2.clas->IsPickable())
							{
								Error("NM_CONNECTING(2): Broken packet ID_JOIN, player '%s' has class '%s'.", info2.name.c_str(), class_id.c_str());
								net->peer->DeallocatePacket(packet);
								EndConnecting(txCantJoin, true);
								return;
							}
						}
					}

					// read save information
					reader.ReadCasted<byte>(load_char);
					if(!reader)
					{
						Error("NM_CONNECTING(2): Broken packet ID_JOIN(4).");
						net->peer->DeallocatePacket(packet);
						EndConnecting(txCantJoin, true);
						return;
					}
					if(load_char == 2)
					{
						reader >> class_id;
						info.clas = Class::TryGet(class_id);
						if(!reader)
						{
							Error("NM_CONNECTING(2): Broken packet ID_JOIN(3).");
							net->peer->DeallocatePacket(packet);
							EndConnecting(txCantJoin, true);
							return;
						}
						else if(!info.clas || !info.clas->IsPickable())
						{
							Error("NM_CONNECTING(2): Invalid loaded class '%s'.", class_id.c_str());
							net->peer->DeallocatePacket(packet);
							EndConnecting(txCantJoin, true);
							return;
						}
					}

					net->peer->DeallocatePacket(packet);

					// read leader
					if(!net->TryGetPlayer(team->leader_id))
					{
						Error("NM_CONNECTING(2): Broken packet ID_JOIN, no player with leader id %d.", team->leader_id);
						EndConnecting(txCantJoin, true);
						return;
					}

					// go to lobby
					if(game_gui->pick_server->visible)
						game_gui->pick_server->CloseDialog();
					if(game_gui->multiplayer->visible)
						game_gui->multiplayer->CloseDialog();
					game_gui->server->Show();
					if(load_char != 0)
						game_gui->server->UseLoadedCharacter(load_char == 2);
					if(load_char != 2)
						game_gui->server->CheckAutopick();
					game_gui->info_box->CloseDialog();
					ChangeTitle();
					return;
				}
				break;
			case ID_CANT_JOIN:
			case ID_NO_FREE_INCOMING_CONNECTIONS:
				// can't join server (full or other reason)
				{
					cstring reason, reason_eng;

					JoinResult result;
					if(msg_id == ID_NO_FREE_INCOMING_CONNECTIONS)
						result = JoinResult::FullServer;
					else
						result = (packet->length >= 2 ? (JoinResult)packet->data[1] : JoinResult::OtherError);

					switch(result)
					{
					case JoinResult::FullServer:
						reason = txServerFull;
						reason_eng = "server is full";
						break;
					case JoinResult::InvalidVersion:
						if(packet->length == 6)
						{
							int w;
							memcpy(&w, packet->data + 2, 4);
							cstring s = VersionToString(w);
							reason = Format(txInvalidVersion2, VERSION, s);
							reason_eng = Format("invalid version (%s) vs server (%s)", VERSION, s);
						}
						else
						{
							reason = txInvalidVersion;
							reason_eng = "invalid version";
						}
						break;
					case JoinResult::TakenNick:
						reason = txNickUsed;
						reason_eng = "nick used";
						break;
					case JoinResult::BrokenPacket:
						reason = txInvalidData;
						reason_eng = "invalid data";
						break;
					case JoinResult::InvalidNick:
						reason = txInvalidNick;
						reason_eng = "invalid nick";
						break;
					case JoinResult::InvalidCrc:
						{
							reason_eng = "invalid unknown type crc";
							reason = txInvalidCrc;

							if(packet->length == 7)
							{
								uint server_crc;
								byte type;
								memcpy(&server_crc, packet->data + 2, 4);
								memcpy(&type, packet->data + 6, 1);
								uint my_crc;
								cstring type_str;
								if(content.GetCrc((Content::Id)type, my_crc, type_str))
									reason_eng = Format("invalid %s crc (%p) vs server (%p)", type_str, my_crc, server_crc);
							}
						}
						break;
					case JoinResult::OtherError:
					default:
						reason = nullptr;
						reason_eng = nullptr;
						break;
					}

					net->peer->DeallocatePacket(packet);
					if(reason_eng)
						Warn("NM_CONNECTING(2): Can't connect to server: %s.", reason_eng);
					else
						Warn("NM_CONNECTING(2): Can't connect to server (%d).", result);
					if(reason)
						EndConnecting(Format("%s:\n%s", txCantJoin2, reason), true);
					else
						EndConnecting(txCantJoin2, true);
					return;
				}
			case ID_CONNECTION_LOST:
			case ID_DISCONNECTION_NOTIFICATION:
				// lost connecting with server or was kicked out
				Warn(msg_id == ID_CONNECTION_LOST ? "NM_CONNECTING(2): Lost connection with server." : "NM_CONNECTING(2): Disconnected from server.");
				net->peer->DeallocatePacket(packet);
				EndConnecting(txLostConnection);
				return;
			case ID_CONNECTION_ATTEMPT_FAILED:
				Warn("NM_CONNECTING(2): Connection attempt failed.");
				net->peer->DeallocatePacket(packet);
				EndConnecting(txConnectionFailed);
				return;
			case ID_INVALID_PASSWORD:
				// password is invalid
				Warn("NM_CONNECTING(2): Invalid password.");
				net->peer->DeallocatePacket(packet);
				EndConnecting(txInvalidPswd);
				return;
			case ID_OUT_OF_BAND_INTERNAL:
				// used by punchthrough, ignore
				break;
			default:
				Warn("NM_CONNECTING(2): Unknown packet from server %u.", msg_id);
				break;
			}
		}

		net_timer -= dt;
		if(net_timer <= 0.f)
		{
			Warn("NM_CONNECTING(2): Connection timeout.");
			EndConnecting(txConnectTimeout);
		}
	}
	else if(net_state == NetState::Client_ConnectingProxy || net_state == NetState::Client_Punchthrough)
	{
		Packet* packet;
		for(packet = net->peer->Receive(); packet; net->peer->DeallocatePacket(packet), packet = net->peer->Receive())
		{
			BitStreamReader reader(packet);
			byte msg_id;
			reader >> msg_id;

			switch(msg_id)
			{
			case ID_CONNECTION_REQUEST_ACCEPTED:
				{
					Info("NM_CONNECTING(3): Connected with proxy, starting nat punchthrough.");
					net_timer = 10.f;
					net_state = NetState::Client_Punchthrough;
					PickServerPanel::ServerData& info = game_gui->pick_server->servers[game_gui->pick_server->grid.selected];
					RakNetGUID guid;
					guid.FromString(info.guid.c_str());
					net->master_server_adr = packet->systemAddress;
					api->StartPunchthrough(&guid);
				}
				break;
			case ID_NAT_TARGET_NOT_CONNECTED:
			case ID_NAT_TARGET_UNRESPONSIVE:
			case ID_NAT_CONNECTION_TO_TARGET_LOST:
			case ID_NAT_PUNCHTHROUGH_FAILED:
				Warn("NM_CONNECTING(3): Punchthrough failed (%d).", msg_id);
				api->EndPunchthrough();
				EndConnecting(txConnectionFailed);
				break;
			case ID_NAT_PUNCHTHROUGH_SUCCEEDED:
				{
					Info("NM_CONNECTING(3): Punchthrough succeeded, connecting to server %s.", packet->systemAddress.ToString());
					api->EndPunchthrough();
					net->peer->CloseConnection(net->master_server_adr, true, 1, IMMEDIATE_PRIORITY);
					PickServerPanel::ServerData& info = game_gui->pick_server->servers[game_gui->pick_server->grid.selected];
					net->server = packet->systemAddress;
					game_gui->server->server_name = info.name;
					game_gui->server->max_players = info.max_players;
					net->active_players = info.active_players;
					ConnectionAttemptResult result = net->peer->Connect(net->server.ToString(false), net->server.GetPort(), enter_pswd.c_str(), enter_pswd.length());
					if(result == CONNECTION_ATTEMPT_STARTED)
					{
						net_mode = NM_CONNECTING;
						net_timer = T_CONNECT;
						net_tries = 1;
						net_state = NetState::Client_Connecting;
						game_gui->info_box->Show(Format(txConnectingTo, info.name.c_str()));
						net->peer->DeallocatePacket(packet);
						return;
					}
					else
					{
						Error("OnPickServer: Can't connect to server: SLikeNet error %d.", result);
						EndConnecting(txConnectSLikeNet);
					}
				}
				break;
			default:
				Warn("NM_CONNECTING(3): Unknown packet from proxy %u.", msg_id);
				break;
			}
		}

		net_timer -= dt;
		if(net_timer <= 0.f)
		{
			Warn("NM_CONNECTING(3): Connection to proxy timeout.");
			api->EndPunchthrough();
			EndConnecting(txConnectTimeout);
		}
	}
}

void Game::UpdateClientTransfer(float dt)
{
	Packet* packet;
	for(packet = net->peer->Receive(); packet; net->peer->DeallocatePacket(packet), packet = net->peer->Receive())
	{
		BitStreamReader reader(packet);
		byte msg_id;
		reader >> msg_id;

		switch(msg_id)
		{
		case ID_DISCONNECTION_NOTIFICATION:
		case ID_CONNECTION_LOST:
			Info("NM_TRANSFER: Lost connection with server.");
			net->peer->DeallocatePacket(packet);
			net->ClosePeer(false, true);
			game_gui->info_box->CloseDialog();
			ExitToMenu();
			gui->SimpleDialog(txLostConnection, nullptr);
			return;
		case ID_SERVER_CLOSE:
			Info("NM_TRANSFER: Server close, failed to load game.");
			net->peer->DeallocatePacket(packet);
			net->ClosePeer(true, true);
			game_gui->info_box->CloseDialog();
			ExitToMenu();
			gui->SimpleDialog(txServerFailedToLoadSave, nullptr);
			return;
		case ID_STATE:
			{
				byte state;
				reader >> state;
				if(reader.IsOk() && InRange(state, 0ui8, 2ui8))
				{
					switch(packet->data[1])
					{
					case 0:
						game_gui->info_box->Show(txGeneratingWorld);
						break;
					case 1:
						game_gui->info_box->Show(txPreparingWorld);
						break;
					case 2:
						game_gui->info_box->Show(txWaitingForPlayers);
						break;
					}
				}
				else
					Error("NM_TRANSFER: Broken packet ID_STATE.");
			}
			break;
		case ID_WORLD_DATA:
			if(net_state == NetState::Client_BeforeTransfer)
			{
				Info("NM_TRANSFER: Received world data.");

				LoadingStep("");
				ClearGameVars(true);
				net->OnNewGameClient();

				fallback_type = FALLBACK::NONE;
				fallback_t = -0.5f;
				net_state = NetState::Client_ReceivedWorldData;

				if(net->ReadWorldData(reader))
				{
					// send ready message
					BitStreamWriter f;
					f << ID_READY;
					f << (byte)0;
					net->SendClient(f, HIGH_PRIORITY, RELIABLE);
					game_gui->info_box->Show(txLoadedWorld);
					LoadingStep("");
					Info("NM_TRANSFER: Loaded world data.");
				}
				else
				{
					Error("NM_TRANSFER: Failed to read world data.");
					net->peer->DeallocatePacket(packet);
					ClearAndExitToMenu(txWorldDataError);
					return;
				}
			}
			else
				Error("NM_TRANSFER: Received ID_WORLD_DATA with net state %d.", net_state);
			break;
		case ID_PLAYER_START_DATA:
			if(net_state == NetState::Client_ReceivedWorldData)
			{
				net_state = NetState::Client_ReceivedPlayerStartData;
				LoadingStep("");
				if(net->ReadPlayerStartData(reader))
				{
					// send ready message
					if(net->mp_load_worldmap)
						LoadResources("", true);
					else
						LoadingStep("");
					BitStreamWriter f;
					f << ID_READY;
					f << (byte)1;
					net->SendClient(f, HIGH_PRIORITY, RELIABLE);
					game_gui->info_box->Show(txLoadedPlayer);
					Info("NM_TRANSFER: Loaded player start data.");
				}
				else
				{
					Error("NM_TRANSFER: Failed to read player data.");
					net->peer->DeallocatePacket(packet);
					ClearAndExitToMenu(txPlayerDataError);
					return;
				}
			}
			else
				Error("NM_TRANSFER: Received ID_PLAYER_START_DATA with net state %d.", net_state);
			break;
		case ID_CHANGE_LEVEL:
			if(net_state == NetState::Client_ReceivedPlayerStartData)
			{
				net_state = NetState::Client_ChangingLevel;
				byte loc, level;
				bool encounter;
				reader >> loc;
				reader >> level;
				reader >> encounter;
				if(reader.IsOk())
				{
					if(world->VerifyLocation(loc))
					{
						if(game_state == GS_LOAD)
							LoadingStep("");
						else
							LoadingStart(4);
						world->ChangeLevel(loc, encounter);
						game_level->dungeon_level = level;
						Info("NM_TRANSFER: Level change to %s (id:%d, level:%d).", game_level->location->name.c_str(), game_level->location_index, game_level->dungeon_level);
						game_gui->info_box->Show(txGeneratingLocation);
					}
					else
						Error("NM_TRANSFER: Broken packet ID_CHANGE_LEVEL, invalid location %u.", loc);
				}
				else
					Error("NM_TRANSFER: Broken packet ID_CHANGE_LEVEL.");
			}
			else
				Error("NM_TRANSFER: Received ID_CHANGE_LEVEL with net state %d.", net_state);
			break;
		case ID_LEVEL_DATA:
			if(net_state == NetState::Client_ChangingLevel)
			{
				net_state = NetState::Client_ReceivedLevelData;
				game_gui->info_box->Show(txLoadingLocation);
				LoadingStep("");
				if(!net->ReadLevelData(reader))
				{
					Error("NM_TRANSFER: Failed to read location data.");
					net->peer->DeallocatePacket(packet);
					ClearAndExitToMenu(txLoadingLocationError);
					return;
				}
				else
				{
					Info("NM_TRANSFER: Loaded level data.");
					BitStreamWriter f;
					f << ID_READY;
					f << (byte)2;
					net->SendClient(f, HIGH_PRIORITY, RELIABLE);
					LoadingStep("");
				}
			}
			else
				Error("NM_TRANSFER: Received ID_LEVEL_DATA with net state %d.", net_state);
			break;
		case ID_PLAYER_DATA:
			if(net_state == NetState::Client_ReceivedLevelData)
			{
				net_state = NetState::Client_ReceivedPlayerData;
				game_gui->info_box->Show(txLoadingChars);
				LoadingStep("");
				if(!net->ReadPlayerData(reader))
				{
					Error("NM_TRANSFER: Failed to read player data.");
					net->peer->DeallocatePacket(packet);
					ClearAndExitToMenu(txLoadingCharsError);
					return;
				}
				else
				{
					Info("NM_TRANSFER: Loaded player data.");
					LoadResources("", false);
					net->mp_load = false;
					BitStreamWriter f;
					f << ID_READY;
					f << (byte)3;
					net->SendClient(f, HIGH_PRIORITY, RELIABLE);
				}
			}
			else
				Error("NM_TRANSFER: Received ID_PLAYER_DATA with net state %d.", net_state);
			break;
		case ID_START:
			if(net->mp_load_worldmap)
			{
				// start on worldmap
				if(net_state == NetState::Client_ReceivedPlayerStartData)
				{
					net->mp_load_worldmap = false;
					net_state = NetState::Client_StartOnWorldmap;
					Info("NM_TRANSFER: Starting at world map.");
					clear_color = Color::White;
					game_state = GS_WORLDMAP;
					game_gui->load_screen->visible = false;
					game_gui->main_menu->visible = false;
					game_gui->level_gui->visible = false;
					game_gui->world_map->Show();
					world->SetState(World::State::ON_MAP);
					game_gui->info_box->CloseDialog();
					net->update_timer = 0.f;
					team->leader_id = 0;
					team->leader = nullptr;
					SetMusic(MusicType::Travel);
					ChangeTitle();
					if(net->mp_quickload)
					{
						net->mp_quickload = false;
						game_gui->mp_box->Add(txGameLoaded);
						game_gui->messages->AddGameMsg3(GMS_GAME_LOADED);
					}
					net->peer->DeallocatePacket(packet);
				}
				else
				{
					Error("NM_TRANSFER: Received ID_START at worldmap with net state %d.", net_state);
					break;
				}
			}
			else
			{
				// start in location
				if(net_state == NetState::Client_ReceivedPlayerData)
				{
					net_state = NetState::Client_Start;
					Info("NM_TRANSFER: Level started.");
					clear_color = clear_color_next;
					game_state = GS_LEVEL;
					game_gui->load_screen->visible = false;
					game_gui->main_menu->visible = false;
					game_gui->level_gui->visible = true;
					game_gui->world_map->Hide();
					game_gui->info_box->CloseDialog();
					net->update_timer = 0.f;
					fallback_type = FALLBACK::NONE;
					fallback_t = -0.5f;
					game_level->camera.Reset();
					pc->data.rot_buf = 0.f;
					ChangeTitle();
					if(net->mp_quickload)
					{
						net->mp_quickload = false;
						game_gui->mp_box->Add(txGameLoaded);
						game_gui->messages->AddGameMsg3(GMS_GAME_LOADED);
					}
					net->peer->DeallocatePacket(packet);
					game_gui->Setup(pc);
					OnEnterLevelOrLocation();
				}
				else
					Error("NM_TRANSFER: Received ID_START with net state %d.", net_state);
			}
			return;
		default:
			Warn("NM_TRANSFER: Unknown packet %u.", msg_id);
			break;
		}
	}
}

void Game::UpdateClientQuiting(float dt)
{
	Packet* packet;
	for(packet = net->peer->Receive(); packet; net->peer->DeallocatePacket(packet), packet = net->peer->Receive())
	{
		BitStreamReader f(packet);
		byte msg_id;
		f >> msg_id;

		if(msg_id == ID_DISCONNECTION_NOTIFICATION || msg_id == ID_CONNECTION_LOST)
		{
			Info("NM_QUITTING: Server accepted disconnection.");
			net->peer->DeallocatePacket(packet);
			net->ClosePeer();
			if(game_gui->server->visible)
				game_gui->server->CloseDialog();
			if(net_callback)
				net_callback();
			game_gui->info_box->CloseDialog();
			return;
		}
		else
			Info("NM_QUITTING: Ignored packet %u.", msg_id);
	}

	net_timer -= dt;
	if(net_timer <= 0.f)
	{
		Warn("NM_QUITTING: Disconnected without accepting.");
		net->peer->DeallocatePacket(packet);
		net->ClosePeer();
		if(game_gui->server->visible)
			game_gui->server->CloseDialog();
		if(net_callback)
			net_callback();
		game_gui->info_box->CloseDialog();
	}
}

void Game::UpdateServerTransfer(float dt)
{
	Packet* packet;
	for(packet = net->peer->Receive(); packet; net->peer->DeallocatePacket(packet), packet = net->peer->Receive())
	{
		BitStreamReader f(packet);

		PlayerInfo* ptr_info = net->FindPlayer(packet->systemAddress);
		if(!ptr_info)
		{
			Info("NM_TRANSFER_SERVER: Ignoring packet from %s.", packet->systemAddress.ToString());
			continue;
		}
		if(ptr_info->left != PlayerInfo::LEFT_NO)
		{
			Info("NM_TRANSFER_SERVER: Packet from %s who left game.", ptr_info->name.c_str());
			continue;
		}

		PlayerInfo& info = *ptr_info;
		byte msg_id;
		f >> msg_id;

		switch(msg_id)
		{
		case ID_DISCONNECTION_NOTIFICATION:
		case ID_CONNECTION_LOST:
			{
				Info("NM_TRANSFER_SERVER: Player %s left game.", info.name.c_str());
				--net->active_players;
				net->players_left = true;
				info.left = (msg_id == ID_CONNECTION_LOST ? PlayerInfo::LEFT_DISCONNECTED : PlayerInfo::LEFT_QUIT);
			}
			break;
		case ID_READY:
			{
				if(net_state != NetState::Server_WaitForPlayersToLoadWorld)
				{
					Warn("NM_TRANSFER_SERVER: Unexpected packet ID_READY from %s.", info.name.c_str());
					break;
				}

				byte type;
				f >> type;
				if(!f)
				{
					Warn("NM_TRANSFER_SERVER: Broken packet ID_READY from %s.", info.name.c_str());
					break;
				}

				if(type == 0)
				{
					Info("NM_TRANSFER_SERVER: %s read world data.", info.name.c_str());
					BitStreamWriter f;
					net->WritePlayerStartData(f, info);
					net->SendServer(f, MEDIUM_PRIORITY, RELIABLE, info.adr);
				}
				else if(type == 1)
				{
					Info("NM_TRANSFER_SERVER: %s is ready.", info.name.c_str());
					info.ready = true;
				}
				else
					Warn("NM_TRANSFER_SERVER: Unknown ID_READY %u from %s.", type, info.name.c_str());
			}
			break;
		default:
			Warn("NM_TRANSFER_SERVER: Unknown packet from %s: %u.", info.name.c_str(), msg_id);
			break;
		}
	}

	if(net_state == NetState::Server_Starting)
	{
		if(!net->mp_load)
		{
			game_gui->info_box->Show(txGeneratingWorld);

			// send info about generating world
			if(net->active_players > 1)
			{
				byte b[] = { ID_STATE, 0 };
				net->peer->Send((cstring)b, 2, IMMEDIATE_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
			}

			// do it
			ClearGameVars(true);
			team->free_recruits = 3 - net->active_players;
			if(team->free_recruits < 0)
				team->free_recruits = 0;
			fallback_type = FALLBACK::NONE;
			fallback_t = -0.5f;
			game_gui->main_menu->visible = false;
			game_gui->load_screen->visible = true;
			clear_color = Color::Black;
			net->prepare_world = true;
			GenerateWorld();
			quest_mgr->InitQuests();
			world->StartInLocation();
			net->prepare_world = false;
			game_res->LoadCommonMusic();
		}

		net_state = NetState::Server_Initialized;
	}
	else if(net_state == NetState::Server_Initialized)
	{
		// prepare world data if there is any players
		if(net->active_players > 1)
		{
			game_gui->info_box->Show(txSendingWorld);

			// send info about preparing world data
			byte b[] = { ID_STATE, 1 };
			net->peer->Send((cstring)b, 2, IMMEDIATE_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);

			// prepare & send world data
			BitStreamWriter f;
			net->WriteWorldData(f);
			net->SendAll(f, IMMEDIATE_PRIORITY, RELIABLE);
			Info("NM_TRANSFER_SERVER: Send world data, size %d.", f.GetSize());
			net_state = NetState::Server_WaitForPlayersToLoadWorld;
			net_timer = mp_timeout;
			for(PlayerInfo& info : net->players)
			{
				if(info.id != team->my_id)
					info.ready = false;
				else
					info.ready = true;
			}
			game_gui->info_box->Show(txWaitingForPlayers);
		}
		else
			net_state = NetState::Server_EnterLocation;

		vector<Unit*> prev_team;

		// create team
		if(net->mp_load)
			prev_team = team->members.ptrs;
		team->members.clear();
		team->active_members.clear();
		game_level->entering = true;
		const bool in_level = game_level->is_open;
		int leader_perk = 0;
		for(PlayerInfo& info : net->players)
		{
			if(info.left != PlayerInfo::LEFT_NO)
				continue;

			Unit* u;
			if(!info.loaded)
			{
				u = CreateUnit(*info.clas->player, -1, nullptr, nullptr, in_level, true);
				u->area = nullptr;
				u->ApplyHumanData(info.hd);
				u->mesh_inst->need_update = true;
				info.u = u;

				u->fake_unit = true; // to prevent sending hp changed message set temporary as fake unit
				u->player = new PlayerController;
				u->player->id = info.id;
				u->player->name = info.name;
				u->player->Init(*u);
				info.cc.Apply(*u->player);
				u->fake_unit = false;

				if(info.cc.HavePerk(Perk::Leader))
					++leader_perk;

				if(net->mp_load)
					PreloadUnit(u);
			}
			else
			{
				PlayerInfo* old = net->FindOldPlayer(info.name.c_str());
				old->loaded = true;
				u = old->u;
				info.u = u;
				u->player->id = info.id;
			}

			team->members.push_back(u);
			team->active_members.push_back(u);

			info.pc = u->player;
			u->player->player_info = &info;
			if(info.id == team->my_id)
			{
				pc = u->player;
				u->player->dialog_ctx = &dialog_context;
				u->player->dialog_ctx->is_local = true;
				u->player->is_local = true;
			}
			else
			{
				u->player->dialog_ctx = new DialogContext;
				u->player->dialog_ctx->is_local = false;
			}
			u->player->dialog_ctx->dialog_mode = false;
			u->player->dialog_ctx->pc = u->player;
			u->interp = EntityInterpolator::Pool.Get();
			u->interp->Reset(u->pos, u->rot);
		}
		team->CalculatePlayersLevel();

		// add ai
		bool anyone_left = false;
		for(Unit* unit : prev_team)
		{
			if(unit->IsPlayer())
				continue;

			if(unit->hero->type != HeroType::Normal)
				team->members.push_back(unit);
			else
			{
				if(team->active_members.size() < team->GetMaxSize())
				{
					team->members.push_back(unit);
					team->active_members.push_back(unit);
				}
				else
				{
					// too many team members, kick ai
					NetChange& c = Add1(Net::changes);
					c.type = NetChange::HERO_LEAVE;
					c.unit = unit;

					game_gui->mp_box->Add(Format(txMpNPCLeft, unit->hero->name.c_str()));
					if(game_level->city_ctx)
						unit->OrderWander();
					else
						unit->OrderLeave();
					unit->hero->team_member = false;
					unit->hero->credit = 0;
					unit->ai->city_wander = false;
					unit->ai->loc_timer = Random(5.f, 10.f);
					unit->MakeItemsTeam(true);
					unit->temporary = true;

					anyone_left = true;
				}
			}
		}

		if(!net->mp_load && leader_perk > 0 && team->GetActiveTeamSize() < team->GetMaxSize())
		{
			UnitData& ud = *Class::GetRandomHeroData();
			int level = ud.level.x + 2 * (leader_perk - 1);
			Unit* npc = CreateUnit(ud, level, nullptr, nullptr, false);
			npc->area = nullptr;
			npc->ai = new AIController;
			npc->ai->Init(npc);
			npc->hero->know_name = true;
			team->AddTeamMember(npc, HeroType::Normal);
			npc->hero->SetupMelee();
		}
		game_level->entering = false;

		// recalculate credit if someone left
		if(anyone_left)
			team->CheckCredit(false, true);

		// set leader
		PlayerInfo* leader_info = net->TryGetPlayer(team->leader_id);
		if(leader_info)
			team->leader = leader_info->u;
		else
		{
			team->leader_id = 0;
			team->leader = net->GetMe().u;
		}

		// send info
		if(net->active_players > 1)
		{
			byte b[] = { ID_STATE, 2 };
			net->peer->Send((cstring)b, 2, IMMEDIATE_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
		}
	}
	else if(net_state == NetState::Server_WaitForPlayersToLoadWorld)
	{
		// wait for all players
		bool ok = true;
		for(PlayerInfo& info : net->players)
		{
			if(info.left != PlayerInfo::LEFT_NO)
				continue;
			if(!info.ready)
			{
				ok = false;
				break;
			}
		}
		net_timer -= dt;
		if(!ok && net_timer <= 0.f)
		{
			bool anyone_removed = false;
			for(PlayerInfo& info : net->players)
			{
				if(info.left != PlayerInfo::LEFT_NO)
					continue;
				if(!info.ready)
				{
					Info("NM_TRANSFER_SERVER: Disconnecting player %s due no response.", info.name.c_str());
					--net->active_players;
					net->players_left = true;
					info.left = PlayerInfo::LEFT_TIMEOUT;
					anyone_removed = true;
				}
			}
			ok = true;
			if(anyone_removed)
			{
				team->CheckCredit(false, true);
				if(team->leader_id == -1)
				{
					team->leader_id = 0;
					team->leader = net->GetMe().u;
				}
			}
		}
		if(ok)
		{
			Info("NM_TRANSFER_SERVER: All players ready.");
			net_state = NetState::Server_EnterLocation;
		}
	}
	else if(net_state == NetState::Server_EnterLocation)
	{
		// enter location
		game_gui->info_box->Show(txLoadingLevel);
		if(!net->mp_load)
			EnterLocation();
		else
		{
			if(!game_level->is_open)
			{
				// saved on worldmap
				byte b = ID_START;
				net->peer->Send((cstring)&b, 1, IMMEDIATE_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);

				net->DeleteOldPlayers();

				clear_color = clear_color_next;
				game_state = GS_WORLDMAP;
				game_gui->load_screen->visible = false;
				game_gui->world_map->Show();
				game_gui->level_gui->visible = false;
				game_gui->main_menu->visible = false;
				net->mp_load = false;
				clear_color = Color::White;
				world->SetState(World::State::ON_MAP);
				net->update_timer = 0.f;
				SetMusic(MusicType::Travel);
				net->ProcessLeftPlayers();
				if(net->mp_quickload)
				{
					net->mp_quickload = false;
					game_gui->mp_box->Add(txGameLoaded);
					game_gui->messages->AddGameMsg3(GMS_GAME_LOADED);
				}
				game_gui->info_box->CloseDialog();
			}
			else
			{
				if(!net->mp_quickload)
					LoadingStart(1);

				BitStreamWriter f;
				f << ID_CHANGE_LEVEL;
				f << (byte)world->GetCurrentLocationIndex();
				f << (byte)game_level->dungeon_level;
				f << (world->GetState() == World::State::INSIDE_ENCOUNTER);
				int ack = net->SendAll(f, HIGH_PRIORITY, RELIABLE_WITH_ACK_RECEIPT);
				for(PlayerInfo& info : net->players)
				{
					if(info.id == team->my_id)
						info.state = PlayerInfo::IN_GAME;
					else
					{
						info.state = PlayerInfo::WAITING_FOR_RESPONSE;
						info.ack = ack;
						info.timer = mp_timeout;
					}
				}

				// find player that was in save and is playing now (first check leader)
				Unit* center_unit = nullptr;
				if(team->leader->player->player_info->loaded)
					center_unit = team->leader;
				else
				{
					// anyone
					for(PlayerInfo& info : net->players)
					{
						if(info.loaded)
						{
							center_unit = info.u;
							break;
						}
					}
				}

				// set position of new units that didn't exists in save (warp to old unit or level entrance)
				if(center_unit)
				{
					// get positon of unit or building entrance
					Vec3 pos;
					if(center_unit->area->area_type == LevelArea::Type::Building)
					{
						InsideBuilding* inside = static_cast<InsideBuilding*>(center_unit->area);
						Vec2 p = inside->enter_region.Midpoint();
						pos = Vec3(p.x, inside->enter_y, p.y);
					}
					else
						pos = center_unit->pos;

					// warp
					for(PlayerInfo& info : net->players)
					{
						if(!info.loaded)
						{
							game_level->local_area->units.push_back(info.u);
							info.u->area = game_level->local_area;
							game_level->WarpNearLocation(*game_level->local_area, *info.u, pos, 4.f, false, 20);
							info.u->rot = Vec3::LookAtAngle(info.u->pos, pos);
							info.u->interp->Reset(info.u->pos, info.u->rot);
						}
					}
				}
				else
				{
					// find entrance position/rotation
					Vec3 pos;
					float rot;
					Portal* portal;

					if(game_level->city_ctx)
						game_level->city_ctx->GetEntry(pos, rot);
					else if(game_level->enter_from >= ENTER_FROM_PORTAL && (portal = game_level->location->GetPortal(game_level->enter_from)) != nullptr)
					{
						pos = portal->pos + Vec3(sin(portal->rot) * 2, 0, cos(portal->rot) * 2);
						rot = Clip(portal->rot + PI);
					}
					else if(game_level->location->type == L_DUNGEON)
					{
						InsideLocation* inside = static_cast<InsideLocation*>(game_level->location);
						InsideLocationLevel& lvl = inside->GetLevelData();
						if(game_level->enter_from == ENTER_FROM_DOWN_LEVEL)
						{
							pos = PtToPos(lvl.GetDownStairsFrontTile());
							rot = DirToRot(lvl.staircase_down_dir);
						}
						else
						{
							pos = PtToPos(lvl.GetUpStairsFrontTile());
							rot = DirToRot(lvl.staircase_up_dir);
						}
					}
					else
						world->GetOutsideSpawnPoint(pos, rot);

					// warp
					for(PlayerInfo& info : net->players)
					{
						if(!info.loaded)
						{
							game_level->local_area->units.push_back(info.u);
							info.u->area = game_level->local_area;
							game_level->WarpNearLocation(*game_level->local_area, *info.u, pos, game_level->location->outside ? 4.f : 2.f, false, 20);
							info.u->rot = rot;
							info.u->interp->Reset(info.u->pos, info.u->rot);
						}
					}
				}

				net->DeleteOldPlayers();
				if(!net->mp_quickload)
					LoadResources("", false);

				net_mode = NM_SERVER_SEND;
				net_state = NetState::Server_Send;
				if(net->active_players > 1)
				{
					prepared_stream.Reset();
					BitStreamWriter f(prepared_stream);
					net->WriteLevelData(f, false);
					Info("NM_TRANSFER_SERVER: Generated level packet: %d.", prepared_stream.GetNumberOfBytesUsed());
					game_gui->info_box->Show(txWaitingForPlayers);
				}
			}
		}
	}
}

void Game::UpdateServerSend(float dt)
{
	Packet* packet;
	for(packet = net->peer->Receive(); packet; net->peer->DeallocatePacket(packet), packet = net->peer->Receive())
	{
		BitStreamReader f(packet);
		PlayerInfo* ptr_info = net->FindPlayer(packet->systemAddress);
		if(!ptr_info)
		{
			Info("NM_SERVER_SEND: Ignoring packet from %s.", packet->systemAddress.ToString());
			continue;
		}
		else if(ptr_info->left != PlayerInfo::LEFT_NO)
		{
			Info("NM_SERVER_SEND: Packet from %s who left game.", ptr_info->name.c_str());
			continue;
		}

		PlayerInfo& info = *ptr_info;
		byte msg_id;
		f >> msg_id;

		switch(packet->data[0])
		{
		case ID_DISCONNECTION_NOTIFICATION:
		case ID_CONNECTION_LOST:
			{
				Info("NM_SERVER_SEND: Player %s left game.", info.name.c_str());
				--net->active_players;
				net->players_left = true;
				info.left = (msg_id == ID_CONNECTION_LOST ? PlayerInfo::LEFT_DISCONNECTED : PlayerInfo::LEFT_QUIT);
			}
			return;
		case ID_SND_RECEIPT_ACKED:
			{
				int ack;
				f >> ack;
				if(!f)
					Error("NM_SERVER_SEND: Broken packet ID_SND_RECEIPT_ACKED from %s.", info.name.c_str());
				else if(info.state != PlayerInfo::WAITING_FOR_RESPONSE || ack != info.ack)
					Warn("NM_SERVER_SEND: Unexpected packet ID_SND_RECEIPT_ACKED from %s.", info.name.c_str());
				else
				{
					// send level data
					net->peer->Send(&prepared_stream, HIGH_PRIORITY, RELIABLE, 0, packet->systemAddress, false);
					info.timer = mp_timeout;
					info.state = PlayerInfo::WAITING_FOR_DATA;
					Info("NM_SERVER_SEND: Send level data to %s.", info.name.c_str());
				}
			}
			break;
		case ID_READY:
			if(info.state == PlayerInfo::IN_GAME || info.state == PlayerInfo::WAITING_FOR_RESPONSE)
				Error("NM_SERVER_SEND: Unexpected packet ID_READY from %s.", info.name.c_str());
			else if(info.state == PlayerInfo::WAITING_FOR_DATA)
			{
				Info("NM_SERVER_SEND: Send player data to %s.", info.name.c_str());
				info.state = PlayerInfo::WAITING_FOR_DATA2;
				BitStreamWriter f;
				net->WritePlayerData(f, info);
				net->SendServer(f, HIGH_PRIORITY, RELIABLE, info.adr);
			}
			else
			{
				Info("NM_SERVER_SEND: Player %s is ready.", info.name.c_str());
				info.state = PlayerInfo::IN_GAME;
			}
			break;
		case ID_CONTROL:
			break;
		default:
			Warn("MN_SERVER_SEND: Invalid packet %d from %s.", msg_id, info.name.c_str());
			break;
		}
	}

	bool ok = true;
	for(PlayerInfo& info : net->players)
	{
		if(info.state != PlayerInfo::IN_GAME && info.left == PlayerInfo::LEFT_NO)
		{
			info.timer -= dt;
			if(info.timer <= 0.f)
			{
				// timeout, remove player
				Info("NM_SERVER_SEND: Disconnecting player %s due to no response.", info.name.c_str());
				net->peer->CloseConnection(info.adr, true, 0, IMMEDIATE_PRIORITY);
				--net->active_players;
				net->players_left = true;
				info.left = PlayerInfo::LEFT_TIMEOUT;
			}
			else
				ok = false;
		}
	}
	if(ok)
	{
		if(net->active_players > 1)
		{
			byte b = ID_START;
			net->peer->Send((cstring)&b, 1, HIGH_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
		}
		for(LevelArea& area : game_level->ForEachArea())
		{
			for(Unit* unit : area.units)
				unit->changed = false;
		}
		Info("NM_SERVER_SEND: All players ready. Starting game.");
		clear_color = clear_color_next;
		game_state = GS_LEVEL;
		game_gui->info_box->CloseDialog();
		game_gui->load_screen->visible = false;
		game_gui->main_menu->visible = false;
		game_gui->level_gui->visible = true;
		game_gui->world_map->Hide();
		net->mp_load = false;
		SetMusic();
		game_gui->Setup(pc);
		net->ProcessLeftPlayers();
		if(net->mp_quickload)
		{
			net->mp_quickload = false;
			game_gui->mp_box->Add(txGameLoaded);
			game_gui->messages->AddGameMsg3(GMS_GAME_LOADED);
		}
	}
}

void Game::UpdateServerQuiting(float dt)
{
	Packet* packet;
	for(packet = net->peer->Receive(); packet; net->peer->DeallocatePacket(packet), packet = net->peer->Receive())
	{
		BitStreamReader f(packet);
		PlayerInfo* ptr_info = net->FindPlayer(packet->systemAddress);
		if(!ptr_info)
			Warn("NM_QUITTING_SERVER: Ignoring packet from unconnected player %s.", packet->systemAddress.ToString());
		else
		{
			PlayerInfo& info = *ptr_info;
			byte msg_id;
			f >> msg_id;
			if(msg_id == ID_DISCONNECTION_NOTIFICATION || msg_id == ID_CONNECTION_LOST)
			{
				if(info.state == PlayerInfo::IN_LOBBY)
					Info("NM_QUITTING_SERVER: Player %s left lobby.", info.name.c_str());
				else
					Info("NM_QUITTING_SERVER: %s left lobby.", packet->systemAddress.ToString());
				--net->active_players;
				if(net->active_players == 0)
				{
					net->peer->DeallocatePacket(packet);
					Info("NM_QUITTING_SERVER: All players disconnected from server. Closing...");
					net->ClosePeer();
					game_gui->info_box->CloseDialog();
					if(game_gui->server->visible)
						game_gui->server->CloseDialog();
					if(net_callback)
						net_callback();
					return;
				}
			}
			else
			{
				Info("NM_QUITTING_SERVER: Ignoring packet %u from %s.", (byte)packet->data[0],
					info.state == PlayerInfo::IN_LOBBY ? info.name.c_str() : packet->systemAddress.ToString());
			}
		}
	}

	net_timer -= dt;
	if(net_timer <= 0.f)
	{
		Warn("NM_QUITTING_SERVER: Not all players disconnected on time. Closing server...");
		net->ClosePeer(net->master_server_state >= MasterServerState::Connecting);
		game_gui->info_box->CloseDialog();
		if(game_gui->server->visible)
			game_gui->server->CloseDialog();
		if(net_callback)
			net_callback();
	}
}

void Game::OnEnterPassword(int id)
{
	if(id == BUTTON_CANCEL)
	{
		EndConnecting(nullptr);
		return;
	}

	Info("Password entered.");
	if(net_state == NetState::Client_WaitingForPasswordProxy)
	{
		ConnectionAttemptResult result = net->peer->Connect(api->GetApiUrl(), api->GetProxyPort(), nullptr, 0);
		if(result == CONNECTION_ATTEMPT_STARTED)
		{
			net_state = NetState::Client_ConnectingProxy;
			net_timer = T_CONNECT;
			game_gui->info_box->Show(txConnectingProxy);
			Info("NM_CONNECTING(1): Connecting to proxy server...");
		}
		else
		{
			Error("NM_CONNECTING(1): Can't connect to proxy: SLikeNet error %d.", result);
			EndConnecting(txConnectSLikeNet);
		}
	}
	else
	{
		ConnectionAttemptResult result = net->peer->Connect(net->ping_adr.ToString(false), net->ping_adr.GetPort(), enter_pswd.c_str(), enter_pswd.length());
		if(result == CONNECTION_ATTEMPT_STARTED)
		{
			net_state = NetState::Client_Connecting;
			net_timer = T_CONNECT;
			game_gui->info_box->Show(Format(txConnectingTo, game_gui->server->server_name.c_str()));
			Info("NM_CONNECTING(1): Connecting to server %s...", game_gui->server->server_name.c_str());
		}
		else
		{
			Info("NM_CONNECTING(1): Can't connect to server: SLikeNet error %d.", result);
			EndConnecting(txConnectSLikeNet);
		}
	}
}

void Game::ForceRedraw()
{
	engine->DoPseudotick();
}

void Game::QuickJoinIp()
{
	SystemAddress adr = UNASSIGNED_SYSTEM_ADDRESS;
	if(adr.FromString(server_ip.c_str()) && adr != UNASSIGNED_SYSTEM_ADDRESS)
	{
		try
		{
			net->InitClient();
		}
		catch(cstring error)
		{
			gui->SimpleDialog(error, nullptr);
			return;
		}

		Info("Pinging %s...", server_ip.c_str());
		Net::SetMode(Net::Mode::Client);
		game_gui->info_box->Show(txConnecting);
		net_mode = NM_CONNECTING;
		net_state = NetState::Client_PingIp;
		net_timer = T_CONNECT_PING;
		net_tries = I_CONNECT_TRIES;
#ifdef _DEBUG
		net_tries *= 2;
#endif
		net->ping_adr = adr;
		net->peer->Ping(net->ping_adr.ToString(false), net->ping_adr.GetPort(), false);
	}
	else
		Warn("Can't quick connect to server, invalid ip.");
}

void Game::Quit()
{
	Info("Game: Quit.");
	if(Net::IsOnline())
		CloseConnection(VoidF(this, &Game::DoQuit));
	else
		DoQuit();
}

void Game::DoQuit()
{
	prev_game_state = game_state;
	game_state = GS_QUIT;
}

void Game::CloseConnection(VoidF f)
{
	if(game_gui->info_box->visible)
	{
		switch(net_mode)
		{
		case NM_QUITTING:
		case NM_QUITTING_SERVER:
			if(!net_callback)
				net_callback = f;
			break;
		case NM_CONNECTING:
		case NM_TRANSFER:
			game_gui->info_box->Show(txDisconnecting);
			ForceRedraw();
			net->ClosePeer(true, true);
			f();
			break;
		case NM_TRANSFER_SERVER:
		case NM_SERVER_SEND:
			Info("ServerPanel: Closing server.");

			// disallow new connections
			net->peer->SetMaximumIncomingConnections(0);
			net->peer->SetOfflinePingResponse(nullptr, 0);

			if(net->active_players > 1)
			{
				// disconnect players
				Info("ServerPanel: Disconnecting clients.");
				const byte b[] = { ID_SERVER_CLOSE, ServerClose_Closing };
				net->peer->Send((cstring)b, 2, IMMEDIATE_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
				net_mode = Game::NM_QUITTING_SERVER;
				--net->active_players;
				net_timer = T_WAIT_FOR_DISCONNECT;
				game_gui->info_box->Show(txDisconnecting);
				net_callback = f;
			}
			else
			{
				// no players, close instantly
				game_gui->info_box->CloseDialog();
				net->ClosePeer(false, true);
				f();
			}
			break;
		}
	}
	else if(game_gui->server->visible)
		game_gui->server->ExitLobby(f);
	else
	{
		if(Net::IsServer())
		{
			Info("ServerPanel: Closing server.");

			// disallow new connections
			net->peer->SetMaximumIncomingConnections(0);
			net->peer->SetOfflinePingResponse(nullptr, 0);

			if(net->active_players > 1)
			{
				// disconnect players
				Info("ServerPanel: Disconnecting clients.");
				const byte b[] = { ID_SERVER_CLOSE, ServerClose_Closing };
				net->peer->Send((cstring)b, 2, IMMEDIATE_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
				net_mode = Game::NM_QUITTING_SERVER;
				--net->active_players;
				net_timer = T_WAIT_FOR_DISCONNECT;
				game_gui->info_box->Show(txDisconnecting);
				net_callback = f;
			}
			else
			{
				// no players, close instantly
				net->ClosePeer(false, true);
				if(f)
					f();
			}
		}
		else
		{
			game_gui->info_box->Show(txDisconnecting);
			ForceRedraw();
			net->ClosePeer(true, true);
			f();
		}
	}
}

void Game::OnCreateCharacter(int id)
{
	if(id != BUTTON_OK)
		return;

	if(Net::IsOnline())
	{
		PlayerInfo& info = net->GetMe();
		game_gui->server->bts[1].state = Button::NONE;
		game_gui->server->bts[0].text = game_gui->server->txChangeChar;
		// set data
		Class* old_class = info.clas;
		info.clas = game_gui->create_character->clas;
		info.hd.Get(*game_gui->create_character->unit->human_data);
		info.cc = game_gui->create_character->cc;
		// send info to other net->active_players about changing my class
		if(Net::IsServer())
		{
			if(info.clas != old_class && net->active_players > 1)
				game_gui->server->AddLobbyUpdate(Int2(Lobby_UpdatePlayer, 0));
		}
		else
		{
			BitStreamWriter f;
			f << ID_PICK_CHARACTER;
			WriteCharacterData(f, info.clas, info.hd, info.cc);
			f << false;
			net->SendClient(f, IMMEDIATE_PRIORITY, RELIABLE);
			Info("Character sent to server.");
		}
	}
	else
	{
		if(!skip_tutorial)
		{
			DialogInfo info;
			info.event = DialogEvent(this, &Game::OnPlayTutorial);
			info.have_tick = true;
			info.name = "tutorial_dialog";
			info.order = ORDER_TOP;
			info.parent = nullptr;
			info.pause = false;
			info.text = txTutPlay;
			info.tick_text = txTutTick;
			info.type = DIALOG_YESNO;

			gui->ShowDialog(info);
		}
		else
			StartNewGame();
	}
}

void Game::OnPlayTutorial(int id)
{
	if(id == BUTTON_CHECKED)
		skip_tutorial = true;
	else if(id == BUTTON_UNCHECKED)
		skip_tutorial = false;
	else
	{
		gui->GetDialog("tutorial_dialog")->visible = false;
		SaveOptions();
		if(id == BUTTON_YES)
			quest_mgr->quest_tutorial->Start();
		else
			StartNewGame();
	}
}

void Game::OnPickServer(int id)
{
	enter_pswd.clear();

	if(!game_gui->pick_server->IsLAN())
	{
		// connect to proxy server for nat punchthrough
		PickServerPanel::ServerData& info = game_gui->pick_server->servers[game_gui->pick_server->grid.selected];
		if(IsSet(info.flags, SERVER_PASSWORD))
		{
			net_mode = NM_CONNECTING;
			net_state = NetState::Client_WaitingForPasswordProxy;
			net_tries = 1;
			game_gui->info_box->Show(txWaitingForPswd);
			GetTextDialogParams params(Format(txEnterPswd, game_gui->server->server_name.c_str()), enter_pswd);
			params.event = DialogEvent(this, &Game::OnEnterPassword);
			params.limit = 16;
			params.parent = game_gui->info_box;
			GetTextDialog::Show(params);
			Info("OnPickServer: Waiting for password...");
		}
		else
		{
			ConnectionAttemptResult result = net->peer->Connect(api->GetApiUrl(), api->GetProxyPort(), nullptr, 0);
			if(result == CONNECTION_ATTEMPT_STARTED)
			{
				net_mode = NM_CONNECTING;
				net_state = NetState::Client_ConnectingProxy;
				net_timer = T_CONNECT;
				game_gui->info_box->Show(txConnectingProxy);
				Info("OnPickServer: Connecting to proxy server...");
			}
			else
			{
				Error("OnPickServer: Can't connect to proxy: SLikeNet error %d.", result);
				EndConnecting(txConnectSLikeNet);
			}
		}
	}
	else
	{
		PickServerPanel::ServerData& info = game_gui->pick_server->servers[game_gui->pick_server->grid.selected];
		net->server = info.adr;
		game_gui->server->server_name = info.name;
		game_gui->server->max_players = info.max_players;
		net->active_players = info.active_players;
		if(IsSet(info.flags, SERVER_PASSWORD))
		{
			// enter password
			net_mode = NM_CONNECTING;
			net_state = NetState::Client_WaitingForPassword;
			net_tries = 1;
			net->ping_adr = info.adr;
			game_gui->info_box->Show(txWaitingForPswd);
			GetTextDialogParams params(Format(txEnterPswd, game_gui->server->server_name.c_str()), enter_pswd);
			params.event = DialogEvent(this, &Game::OnEnterPassword);
			params.limit = 16;
			params.parent = game_gui->info_box;
			GetTextDialog::Show(params);
			Info("OnPickServer: Waiting for password...");
		}
		else
		{
			// connect
			ConnectionAttemptResult result = net->peer->Connect(info.adr.ToString(false), (word)net->port, nullptr, 0);
			if(result == CONNECTION_ATTEMPT_STARTED)
			{
				// connecting in progress
				net_mode = NM_CONNECTING;
				net_timer = T_CONNECT;
				net_tries = 1;
				net_state = NetState::Client_Connecting;
				game_gui->info_box->Show(Format(txConnectingTo, info.name.c_str()));
				Info("OnPickServer: Connecting to server %s...", info.name.c_str());
			}
			else
			{
				// failed to startup connecting
				Error("OnPickServer: Can't connect to server: SLikeNet error %d.", result);
				EndConnecting(txConnectSLikeNet);
			}
		}
	}
}

void Game::ClearAndExitToMenu(cstring msg)
{
	assert(msg);
	Info("Clear game and exit to menu.");
	ClearGame();
	net->ClosePeer(false, true);
	ExitToMenu();
	gui->SimpleDialog(msg, game_gui->main_menu);
}
