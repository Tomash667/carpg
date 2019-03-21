#include "Pch.h"
#include "GameCore.h"
#include "Game.h"
#include "Language.h"
#include "Terrain.h"
#include "Version.h"
#include "City.h"
#include "InsideLocation.h"
#include "GameGui.h"
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
#include "GlobalGui.h"
#include "PlayerInfo.h"
#include "LobbyApi.h"
#include "Render.h"

extern string g_ctime;

// consts
const float T_TRY_CONNECT = 5.f;
const int T_SHUTDOWN = 1000;
const float T_WAIT_FOR_DATA = 5.f;

//=================================================================================================
bool Game::CanShowMenu()
{
	return !GUI.HaveDialog() && !gui->game_gui->HavePanelOpen() && !gui->main_menu->visible && game_state != GS_MAIN_MENU && death_screen != 3 && !end_of_game
		&& !dialog_context.dialog_mode;
}

//=================================================================================================
void Game::SaveOptions()
{
	Render* render = GetRender();
	cfg.Add("fullscreen", IsFullscreen());
	cfg.Add("cl_glow", cl_glow);
	cfg.Add("cl_normalmap", cl_normalmap);
	cfg.Add("cl_specularmap", cl_specularmap);
	cfg.Add("sound_volume", sound_mgr->GetSoundVolume());
	cfg.Add("music_volume", sound_mgr->GetMusicVolume());
	cfg.Add("mouse_sensitivity", settings.mouse_sensitivity);
	cfg.Add("grass_range", settings.grass_range);
	cfg.Add("resolution", Format("%dx%d", GetWindowSize().x, GetWindowSize().y));
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
	hd.Get(*gui->create_character->unit->human_data);
	NewGameCommon(gui->create_character->clas, gui->create_character->player_name.c_str(), hd, gui->create_character->cc, false);
}

//=================================================================================================
void Game::StartQuickGame()
{
	Net::SetMode(Net::Mode::Singleplayer);

	Class clas = quickstart_class;
	HumanData hd;
	CreatedCharacter cc;
	int hair_index;

	RandomCharacter(clas, hair_index, hd, cc);
	NewGameCommon(clas, quickstart_name.c_str(), hd, cc, false);
}

//=================================================================================================
void Game::NewGameCommon(Class clas, cstring name, HumanData& hd, CreatedCharacter& cc, bool tutorial)
{
	L.entering = true;
	QM.quest_tutorial->in_tutorial = tutorial;
	gui->main_menu->visible = false;
	Net::SetMode(Net::Mode::Singleplayer);
	game_state = GS_LEVEL;
	hardcore_mode = hardcore_option;

	UnitData& ud = *ClassInfo::classes[(int)clas].unit_data;

	Unit* u = CreateUnit(ud, -1, nullptr, nullptr, false, true);
	u->ApplyHumanData(hd);
	Team.members.clear();
	Team.active_members.clear();
	Team.members.push_back(u);
	Team.active_members.push_back(u);
	Team.leader = u;

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
	if(QM.quest_tutorial->finished_tutorial)
	{
		u->AddItem(Item::Get("book_adventurer"), 1u, false);
		QM.quest_tutorial->finished_tutorial = false;
	}
	dialog_context.pc = pc;

	Team.CalculatePlayersLevel();
	ClearGameVars(true);
	gui->Setup(pc);

	if(!tutorial && cc.HavePerk(Perk::Leader))
	{
		Unit* npc = CreateUnit(ClassInfo::GetRandomData(), -1, nullptr, nullptr, false);
		npc->ai = new AIController;
		npc->ai->Init(npc);
		npc->hero->know_name = true;
		Team.AddTeamMember(npc, false);
		--Team.free_recruits;
		npc->hero->SetupMelee();
	}
	gui->game_gui->Setup();

	fallback_type = FALLBACK::NONE;
	fallback_t = -0.5f;
	if(change_title_a)
		ChangeTitle();

	if(!tutorial)
	{
		GenerateWorld();
		QM.InitQuests(devmode);
		if(!sound_mgr->IsMusicDisabled())
		{
			LoadMusic(MusicType::Boss, false);
			LoadMusic(MusicType::Death, false);
			LoadMusic(MusicType::Travel, false);
		}
		EnterLocation();
	}
}

//=================================================================================================
void Game::MultiplayerPanelEvent(int id)
{
	player_name = Trimmed(gui->multiplayer->textbox.GetText());

	// check nick
	if(player_name.empty())
	{
		GUI.SimpleDialog(gui->multiplayer->txNeedEnterNick, gui->multiplayer);
		return;
	}
	if(!N.ValidateNick(player_name.c_str()))
	{
		GUI.SimpleDialog(gui->multiplayer->txEnterValidNick, gui->multiplayer);
		return;
	}

	// save nick
	cfg.Add("nick", player_name);
	SaveCfg();

	switch(id)
	{
	case MultiplayerPanel::IdJoinLan:
		gui->pick_server->Show();
		break;
	case MultiplayerPanel::IdJoinIp:
		{
			GetTextDialogParams params(txEnterIp, server_ip);
			params.event = DialogEvent(this, &Game::OnEnterIp);
			params.limit = 100;
			params.parent = gui->multiplayer;
			GetTextDialog::Show(params);
		}
		break;
	case MultiplayerPanel::IdCreate:
		gui->create_server->Show();
		break;
	case MultiplayerPanel::IdLoad:
		N.mp_load = true;
		Net::changes.clear();
		if(!N.net_strs.empty())
			StringPool.Free(N.net_strs);
		gui->saveload->ShowLoadPanel();
		break;
	}
}

//=================================================================================================
void Game::CreateServerEvent(int id)
{
	if(id == BUTTON_CANCEL)
	{
		if(N.mp_load)
		{
			ClearGame();
			N.mp_load = false;
		}
		gui->create_server->CloseDialog();
	}
	else
	{
		// copy settings
		N.server_name = Trimmed(gui->create_server->textbox[0].GetText());
		N.max_players = atoi(gui->create_server->textbox[1].GetText().c_str());
		N.password = gui->create_server->textbox[2].GetText();
		N.server_lan = gui->create_server->checkbox.checked;

		// check settings
		cstring error_text = nullptr;
		Control* give_focus = nullptr;
		if(N.server_name.empty())
		{
			error_text = gui->create_server->txEnterServerName;
			give_focus = &gui->create_server->textbox[0];
		}
		else if(!InRange(N.max_players, MIN_PLAYERS, MAX_PLAYERS))
		{
			error_text = Format(gui->create_server->txInvalidPlayersCount, MAX_PLAYERS);
			give_focus = &gui->create_server->textbox[1];
		}
		if(error_text)
		{
			GUI.SimpleDialog(error_text, gui->create_server);
			gui->create_server->cont.give_focus = give_focus;
			return;
		}

		// save settings
		cfg.Add("server_name", N.server_name);
		cfg.Add("server_pswd", N.password);
		cfg.Add("server_players", N.max_players);
		cfg.Add("server_lan", N.server_lan);
		SaveCfg();

		// close dialog windows
		gui->create_server->CloseDialog();
		gui->multiplayer->CloseDialog();

		try
		{
			N.InitServer();
		}
		catch(cstring err)
		{
			GUI.SimpleDialog(err, gui->main_menu);
			return;
		}

		Net_OnNewGameServer();
	}
}

//=================================================================================================
void Game::AddMsg(cstring msg)
{
	if(gui->server->visible)
		gui->server->AddMsg(msg);
	else
		AddMultiMsg(msg);
}

//=================================================================================================
void Game::RandomCharacter(Class& clas, int& hair_index, HumanData& hd, CreatedCharacter& cc)
{
	if(clas == Class::RANDOM)
		clas = ClassInfo::GetRandomPlayer();
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
				N.InitClient();
			}
			catch(cstring error)
			{
				GUI.SimpleDialog(error, gui->multiplayer);
				return;
			}

			Info("Pinging %s...", adr.ToString());
			gui->info_box->Show(txConnecting);
			net_mode = NM_CONNECTING;
			net_state = NetState::Client_PingIp;
			net_timer = T_CONNECT_PING;
			net_tries = I_CONNECT_TRIES;
			N.ping_adr = adr;
			if(N.ping_adr.GetPort() == 0)
				N.ping_adr.SetPortHostOrder(N.port);
			N.peer->Ping(N.ping_adr.ToString(false), N.ping_adr.GetPort(), false);
		}
		else
			GUI.SimpleDialog(txInvalidIp, gui->multiplayer);
	}
}

//=================================================================================================
void Game::EndConnecting(cstring msg, bool wait)
{
	gui->info_box->CloseDialog();
	if(msg)
		GUI.SimpleDialog(msg, gui->pick_server->visible ? (DialogBox*)gui->pick_server : (DialogBox*)gui->multiplayer);
	if(wait)
		ForceRedraw();
	if(!gui->pick_server->visible)
		N.ClosePeer(wait);
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
				Info("Pinging %s...", N.ping_adr.ToString());
				N.peer->Ping(N.ping_adr.ToString(false), N.ping_adr.GetPort(), false);
				--net_tries;
				net_timer = T_CONNECT_PING;
			}
		}

		// waiting for connection
		Packet* packet;
		for(packet = N.peer->Receive(); packet; N.peer->DeallocatePacket(packet), packet = N.peer->Receive())
		{
			BitStream& stream = N.StreamStart(packet, Stream_PingIp);
			BitStreamReader f(stream);
			byte msg_id;
			f >> msg_id;

			if(msg_id != ID_UNCONNECTED_PONG)
			{
				// unknown packet from server
				Warn("NM_CONNECTING(0): Unknown server response: %u.", msg_id);
				N.StreamError();
				continue;
			}

			if(packet->length == sizeof(TimeMS) + 1)
			{
				Warn("NM_CONNECTING(0): Server not set SetOfflinePingResponse yet.");
				N.StreamError();
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
				N.StreamError();
				continue;
			}
			if(sign_ca[0] != 'C' || sign_ca[1] != 'A')
			{
				// invalid signature, this is not carpg server
				Warn("NM_CONNECTING(0): Invalid server signature 0x%x%x.", byte(sign_ca[0]), byte(sign_ca[1]));
				N.StreamError();
				N.peer->DeallocatePacket(packet);
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
				N.StreamError("NM_CONNECTING(0): Broken server message.");
				N.peer->DeallocatePacket(packet);
				EndConnecting(txConnectInvalid);
				return;
			}

			N.StreamEnd();
			N.peer->DeallocatePacket(packet);

			// check version
			if(version != VERSION)
			{
				// version mismatch
				cstring s = VersionToString(version);
				Error("NM_CONNECTING(0): Invalid client version '%s' vs server '%s'.", VERSION_STR, s);
				N.peer->DeallocatePacket(packet);
				EndConnecting(Format(txConnectVersion, VERSION_STR, s));
				return;
			}

			// set server status
			gui->server->max_players = max_players;
			gui->server->server_name = server_name_r;
			Info("NM_CONNECTING(0): Server information. Name:%s; players:%d/%d; flags:%d.", server_name_r.c_str(), players, max_players, flags);
			if(IS_SET(flags, 0xFC))
				Warn("NM_CONNECTING(0): Unknown server flags.");
			N.server = packet->systemAddress;
			enter_pswd.clear();

			if(IS_SET(flags, 0x01))
			{
				// password is required
				net_state = NetState::Client_WaitingForPassword;
				gui->info_box->Show(txWaitingForPswd);
				GetTextDialogParams params(Format(txEnterPswd, server_name_r.c_str()), enter_pswd);
				params.event = DialogEvent(this, &Game::OnEnterPassword);
				params.limit = 16;
				params.parent = gui->info_box;
				GetTextDialog::Show(params);
				Info("NM_CONNECTING(0): Waiting for password...");
			}
			else
			{
				// try to connect
				ConnectionAttemptResult result = N.peer->Connect(N.ping_adr.ToString(false), N.ping_adr.GetPort(), nullptr, 0);
				if(result == CONNECTION_ATTEMPT_STARTED)
				{
					// connecting...
					net_timer = T_CONNECT;
					net_state = NetState::Client_Connecting;
					gui->info_box->Show(Format(txConnectingTo, server_name_r.c_str()));
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
		for(packet = N.peer->Receive(); packet; N.peer->DeallocatePacket(packet), packet = N.peer->Receive())
		{
			// ignore messages from proxy (disconnect notification)
			if(packet->systemAddress != N.server)
				continue;

			BitStream& stream = N.StreamStart(packet, Stream_Connect);
			BitStreamReader reader(stream);
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
					N.SendClient(f, IMMEDIATE_PRIORITY, RELIABLE, Stream_Connect);
				}
				break;
			case ID_JOIN:
				// server accepted and sent info about players and my id
				{
					int count, load_char;
					reader.ReadCasted<byte>(Team.my_id);
					reader.ReadCasted<byte>(N.active_players);
					reader.ReadCasted<byte>(Team.leader_id);
					reader.ReadCasted<byte>(count);
					if(!reader)
					{
						N.StreamError("NM_CONNECTING(2): Broken packet ID_JOIN.");
						N.peer->DeallocatePacket(packet);
						EndConnecting(txCantJoin, true);
						return;
					}

					// add player
					DeleteElements(N.players);
					auto pinfo = new PlayerInfo;
					N.players.push_back(pinfo);
					auto& info = *pinfo;
					info.clas = Class::INVALID;
					info.ready = false;
					info.name = player_name;
					info.id = Team.my_id;
					info.state = PlayerInfo::IN_LOBBY;
					info.update_flags = 0;
					info.left = PlayerInfo::LEFT_NO;
					info.loaded = false;

					// read other players
					for(int i = 0; i < count; ++i)
					{
						pinfo = new PlayerInfo;
						N.players.push_back(pinfo);
						PlayerInfo& info2 = *pinfo;
						info2.state = PlayerInfo::IN_LOBBY;
						info2.left = PlayerInfo::LEFT_NO;
						info2.loaded = false;

						reader.ReadCasted<byte>(info2.id);
						reader >> info2.ready;
						reader.ReadCasted<byte>(info2.clas);
						reader >> info2.name;
						if(!reader)
						{
							N.StreamError("NM_CONNECTING(2): Broken packet ID_JOIN(2).");
							N.peer->DeallocatePacket(packet);
							EndConnecting(txCantJoin, true);
							return;
						}

						// verify player class
						if(!ClassInfo::IsPickable(info2.clas) && info2.clas != Class::INVALID)
						{
							N.StreamError("NM_CONNECTING(2): Broken packet ID_JOIN, player %s has class %d.", info2.name.c_str(), info2.clas);
							N.peer->DeallocatePacket(packet);
							EndConnecting(txCantJoin, true);
							return;
						}
					}

					// read save information
					reader.ReadCasted<byte>(load_char);
					if(!reader)
					{
						N.StreamError("NM_CONNECTING(2): Broken packet ID_JOIN(4).");
						N.peer->DeallocatePacket(packet);
						EndConnecting(txCantJoin, true);
						return;
					}
					if(load_char == 2)
					{
						reader.ReadCasted<byte>(info.clas);
						if(!reader)
						{
							N.StreamError("NM_CONNECTING(2): Broken packet ID_JOIN(3).");
							N.peer->DeallocatePacket(packet);
							EndConnecting(txCantJoin, true);
							return;
						}
					}

					N.StreamEnd();
					N.peer->DeallocatePacket(packet);

					// read leader
					if(!N.TryGetPlayer(Team.leader_id))
					{
						Error("NM_CONNECTING(2): Broken packet ID_JOIN, no player with leader id %d.", Team.leader_id);
						EndConnecting(txCantJoin, true);
						return;
					}

					// go to lobby
					if(gui->pick_server->visible)
						gui->pick_server->CloseDialog();
					if(gui->multiplayer->visible)
						gui->multiplayer->CloseDialog();
					gui->server->Show();
					if(load_char != 0)
						gui->server->UseLoadedCharacter(load_char == 2);
					if(load_char != 2)
						gui->server->CheckAutopick();
					gui->info_box->CloseDialog();
					if(change_title_a)
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

					N.StreamEnd();
					N.peer->DeallocatePacket(packet);
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
				N.StreamEnd();
				N.peer->DeallocatePacket(packet);
				EndConnecting(txLostConnection);
				return;
			case ID_CONNECTION_ATTEMPT_FAILED:
				Warn("NM_CONNECTING(2): Connection attempt failed.");
				N.StreamEnd();
				N.peer->DeallocatePacket(packet);
				EndConnecting(txConnectionFailed);
				return;
			case ID_INVALID_PASSWORD:
				// password is invalid
				Warn("NM_CONNECTING(2): Invalid password.");
				N.StreamEnd();
				N.peer->DeallocatePacket(packet);
				EndConnecting(txInvalidPswd);
				return;
			case ID_OUT_OF_BAND_INTERNAL:
				// used by punchthrough, ignore
				break;
			default:
				N.StreamError();
				Warn("NM_CONNECTING(2): Unknown packet from server %u.", msg_id);
				break;
			}

			N.StreamEnd();
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
		for(packet = N.peer->Receive(); packet; N.peer->DeallocatePacket(packet), packet = N.peer->Receive())
		{
			BitStream& stream = N.StreamStart(packet, Stream_Connect);
			BitStreamReader reader(stream);
			byte msg_id;
			reader >> msg_id;

			switch(msg_id)
			{
			case ID_CONNECTION_REQUEST_ACCEPTED:
				{
					Info("NM_CONNECTING(3): Connected with proxy, starting nat punchthrough.");
					net_timer = 10.f;
					net_state = NetState::Client_Punchthrough;
					PickServerPanel::ServerData& info = gui->pick_server->servers[gui->pick_server->grid.selected];
					RakNetGUID guid;
					guid.FromString(info.guid.c_str());
					N.master_server_adr = packet->systemAddress;
					N.api->StartPunchthrough(&guid);
				}
				break;
			case ID_NAT_TARGET_NOT_CONNECTED:
			case ID_NAT_TARGET_UNRESPONSIVE:
			case ID_NAT_CONNECTION_TO_TARGET_LOST:
			case ID_NAT_PUNCHTHROUGH_FAILED:
				Warn("NM_CONNECTING(3): Punchthrough failed (%d).", msg_id);
				N.api->EndPunchthrough();
				EndConnecting(txConnectionFailed);
				break;
			case ID_NAT_PUNCHTHROUGH_SUCCEEDED:
				{
					Info("NM_CONNECTING(3): Punchthrough succeeded, connecting to server %s.", packet->systemAddress.ToString());
					N.api->EndPunchthrough();
					N.peer->CloseConnection(N.master_server_adr, true, 1, IMMEDIATE_PRIORITY);
					PickServerPanel::ServerData& info = gui->pick_server->servers[gui->pick_server->grid.selected];
					N.server = packet->systemAddress;
					gui->server->server_name = info.name;
					gui->server->max_players = info.max_players;
					N.active_players = info.active_players;
					ConnectionAttemptResult result = N.peer->Connect(N.server.ToString(false), N.server.GetPort(), enter_pswd.c_str(), enter_pswd.length());
					if(result == CONNECTION_ATTEMPT_STARTED)
					{
						net_mode = NM_CONNECTING;
						net_timer = T_CONNECT;
						net_tries = 1;
						net_state = NetState::Client_Connecting;
						gui->info_box->Show(Format(txConnectingTo, info.name.c_str()));
						N.StreamEnd();
						N.peer->DeallocatePacket(packet);
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
				N.StreamError();
				Warn("NM_CONNECTING(3): Unknown packet from proxy %u.", msg_id);
				break;
			}

			N.StreamEnd();
		}

		net_timer -= dt;
		if(net_timer <= 0.f)
		{
			Warn("NM_CONNECTING(3): Connection to proxy timeout.");
			N.api->EndPunchthrough();
			EndConnecting(txConnectTimeout);
		}
	}
}

void Game::UpdateClientTransfer(float dt)
{
	Packet* packet;
	for(packet = N.peer->Receive(); packet; N.peer->DeallocatePacket(packet), packet = N.peer->Receive())
	{
		BitStream& stream = N.StreamStart(packet, Stream_Transfer);
		BitStreamReader reader(stream);
		byte msg_id;
		reader >> msg_id;

		switch(msg_id)
		{
		case ID_DISCONNECTION_NOTIFICATION:
		case ID_CONNECTION_LOST:
			Info("NM_TRANSFER: Lost connection with server.");
			N.StreamEnd();
			N.peer->DeallocatePacket(packet);
			N.ClosePeer(false, true);
			gui->info_box->CloseDialog();
			ExitToMenu();
			GUI.SimpleDialog(txLostConnection, nullptr);
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
						gui->info_box->Show(txGeneratingWorld);
						break;
					case 1:
						gui->info_box->Show(txPreparingWorld);
						break;
					case 2:
						gui->info_box->Show(txWaitingForPlayers);
						break;
					}
				}
				else
					N.StreamError("NM_TRANSFER: Broken packet ID_STATE.");
			}
			break;
		case ID_WORLD_DATA:
			if(net_state == NetState::Client_BeforeTransfer)
			{
				Info("NM_TRANSFER: Received world data.");

				LoadingStep("");
				ClearGameVars(true);
				Net_OnNewGameClient();

				fallback_type = FALLBACK::NONE;
				fallback_t = -0.5f;
				net_state = NetState::Client_ReceivedWorldData;

				if(N.ReadWorldData(reader))
				{
					// send ready message
					BitStreamWriter f;
					f << ID_READY;
					f << (byte)0;
					N.SendClient(f, HIGH_PRIORITY, RELIABLE, Stream_Transfer);
					gui->info_box->Show(txLoadedWorld);
					LoadingStep("");
					Info("NM_TRANSFER: Loaded world data.");
				}
				else
				{
					N.StreamError("NM_TRANSFER: Failed to read world data.");
					N.peer->DeallocatePacket(packet);
					ClearAndExitToMenu(txWorldDataError);
					return;
				}
			}
			else
				N.StreamError("NM_TRANSFER: Received ID_WORLD_DATA with net state %d.", net_state);
			break;
		case ID_PLAYER_START_DATA:
			if(net_state == NetState::Client_ReceivedWorldData)
			{
				net_state = NetState::Client_ReceivedPlayerStartData;
				LoadingStep("");
				if(N.ReadPlayerStartData(reader))
				{
					// send ready message
					if(N.mp_load_worldmap)
						LoadResources("", true);
					else
						LoadingStep("");
					BitStreamWriter f;
					f << ID_READY;
					f << (byte)1;
					N.SendClient(f, HIGH_PRIORITY, RELIABLE, Stream_Transfer);
					gui->info_box->Show(txLoadedPlayer);
					Info("NM_TRANSFER: Loaded player start data.");
				}
				else
				{
					N.StreamError("NM_TRANSFER: Failed to read player data.");
					N.peer->DeallocatePacket(packet);
					ClearAndExitToMenu(txPlayerDataError);
					return;
				}
			}
			else
				N.StreamError("NM_TRANSFER: Received ID_PLAYER_START_DATA with net state %d.", net_state);
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
					if(W.VerifyLocation(loc))
					{
						if(game_state == GS_LOAD)
							LoadingStep("");
						else
							LoadingStart(4);
						W.ChangeLevel(loc, encounter);
						L.dungeon_level = level;
						Info("NM_TRANSFER: Level change to %s (id:%d, level:%d).", L.location->name.c_str(), L.location_index, L.dungeon_level);
						gui->info_box->Show(txGeneratingLocation);
					}
					else
						N.StreamError("NM_TRANSFER: Broken packet ID_CHANGE_LEVEL, invalid location %u.", loc);
				}
				else
					N.StreamError("NM_TRANSFER: Broken packet ID_CHANGE_LEVEL.");
			}
			else
				N.StreamError("NM_TRANSFER: Received ID_CHANGE_LEVEL with net state %d.", net_state);
			break;
		case ID_LEVEL_DATA:
			if(net_state == NetState::Client_ChangingLevel)
			{
				net_state = NetState::Client_ReceivedLevelData;
				gui->info_box->Show(txLoadingLocation);
				LoadingStep("");
				if(!N.ReadLevelData(reader))
				{
					N.StreamError("NM_TRANSFER: Failed to read location data.");
					N.peer->DeallocatePacket(packet);
					ClearAndExitToMenu(txLoadingLocationError);
					return;
				}
				else
				{
					Info("NM_TRANSFER: Loaded level data.");
					BitStreamWriter f;
					f << ID_READY;
					f << (byte)2;
					N.SendClient(f, HIGH_PRIORITY, RELIABLE, Stream_Transfer);
					LoadingStep("");
				}
			}
			else
				N.StreamError("NM_TRANSFER: Received ID_LEVEL_DATA with net state %d.", net_state);
			break;
		case ID_PLAYER_DATA:
			if(net_state == NetState::Client_ReceivedLevelData)
			{
				net_state = NetState::Client_ReceivedPlayerData;
				gui->info_box->Show(txLoadingChars);
				LoadingStep("");
				if(!ReadPlayerData(reader))
				{
					N.StreamError("NM_TRANSFER: Failed to read player data.");
					N.peer->DeallocatePacket(packet);
					ClearAndExitToMenu(txLoadingCharsError);
					return;
				}
				else
				{
					Info("NM_TRANSFER: Loaded player data.");
					LoadResources("", false);
					N.mp_load = false;
					BitStreamWriter f;
					f << ID_READY;
					f << (byte)3;
					N.SendClient(f, HIGH_PRIORITY, RELIABLE, Stream_Transfer);
				}
			}
			else
				N.StreamError("NM_TRANSFER: Received ID_PLAYER_DATA with net state %d.", net_state);
			break;
		case ID_START:
			if(N.mp_load_worldmap)
			{
				// start on worldmap
				if(net_state == NetState::Client_ReceivedPlayerStartData)
				{
					N.mp_load_worldmap = false;
					net_state = NetState::Client_StartOnWorldmap;
					Info("NM_TRANSFER: Starting at world map.");
					clear_color = Color::White;
					game_state = GS_WORLDMAP;
					gui->load_screen->visible = false;
					gui->main_menu->visible = false;
					gui->game_gui->visible = false;
					gui->world_map->Show();
					W.SetState(World::State::ON_MAP);
					gui->info_box->CloseDialog();
					N.update_timer = 0.f;
					Team.leader_id = 0;
					Team.leader = nullptr;
					pc = nullptr;
					SetMusic(MusicType::Travel);
					if(change_title_a)
						ChangeTitle();
					N.StreamEnd();
					N.peer->DeallocatePacket(packet);
				}
				else
				{
					N.StreamError("NM_TRANSFER: Received ID_START at worldmap with net state %d.", net_state);
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
					clear_color = clear_color2;
					game_state = GS_LEVEL;
					gui->load_screen->visible = false;
					gui->main_menu->visible = false;
					gui->game_gui->visible = true;
					gui->world_map->Hide();
					gui->info_box->CloseDialog();
					N.update_timer = 0.f;
					fallback_type = FALLBACK::NONE;
					fallback_t = -0.5f;
					cam.Reset();
					pc_data.rot_buf = 0.f;
					if(change_title_a)
						ChangeTitle();
					N.StreamEnd();
					N.peer->DeallocatePacket(packet);
					gui->Setup(pc);
					OnEnterLevelOrLocation();
				}
				else
					N.StreamError("NM_TRANSFER: Received ID_START with net state %d.", net_state);
			}
			return;
		default:
			Warn("NM_TRANSFER: Unknown packet %u.", msg_id);
			N.StreamError();
			break;
		}

		N.StreamEnd();
	}
}

void Game::UpdateClientQuiting(float dt)
{
	Packet* packet;
	for(packet = N.peer->Receive(); packet; N.peer->DeallocatePacket(packet), packet = N.peer->Receive())
	{
		BitStream& stream = N.StreamStart(packet, Stream_Quitting);
		byte msg_id;
		stream.Read(msg_id);
		N.StreamEnd();

		if(msg_id == ID_DISCONNECTION_NOTIFICATION || msg_id == ID_CONNECTION_LOST)
		{
			Info("NM_QUITTING: Server accepted disconnection.");
			N.peer->DeallocatePacket(packet);
			N.ClosePeer();
			if(gui->server->visible)
				gui->server->CloseDialog();
			if(net_callback)
				net_callback();
			gui->info_box->CloseDialog();
			return;
		}
		else
			Info("NM_QUITTING: Ignored packet %u.", msg_id);
	}

	net_timer -= dt;
	if(net_timer <= 0.f)
	{
		Warn("NM_QUITTING: Disconnected without accepting.");
		N.peer->DeallocatePacket(packet);
		N.ClosePeer();
		if(gui->server->visible)
			gui->server->CloseDialog();
		if(net_callback)
			net_callback();
		gui->info_box->CloseDialog();
	}
}

void Game::UpdateServerTransfer(float dt)
{
	Packet* packet;
	for(packet = N.peer->Receive(); packet; N.peer->DeallocatePacket(packet), packet = N.peer->Receive())
	{
		BitStream& stream = N.StreamStart(packet, Stream_TransferServer);

		PlayerInfo* ptr_info = N.FindPlayer(packet->systemAddress);
		if(!ptr_info)
		{
			Info("NM_TRANSFER_SERVER: Ignoring packet from %s.", packet->systemAddress.ToString());
			N.StreamEnd();
			continue;
		}
		if(ptr_info->left != PlayerInfo::LEFT_NO)
		{
			Info("NM_TRANSFER_SERVER: Packet from %s who left game.", ptr_info->name.c_str());
			N.StreamEnd();
			continue;
		}

		PlayerInfo& info = *ptr_info;
		byte msg_id;
		stream.Read(msg_id);

		switch(msg_id)
		{
		case ID_DISCONNECTION_NOTIFICATION:
		case ID_CONNECTION_LOST:
			{
				Info("NM_TRANSFER_SERVER: Player %s left game.", info.name.c_str());
				--N.active_players;
				N.players_left = true;
				info.left = (msg_id == ID_CONNECTION_LOST ? PlayerInfo::LEFT_DISCONNECTED : PlayerInfo::LEFT_QUIT);
			}
			break;
		case ID_READY:
			{
				byte type;
				if(net_state != NetState::Server_WaitForPlayersToLoadWorld)
				{
					Warn("NM_TRANSFER_SERVER: Unexpected packet ID_READY from %s.", info.name.c_str());
					N.StreamError();
				}
				else if(stream.Read(type))
				{
					if(type == 0)
					{
						Info("NM_TRANSFER_SERVER: %s read world data.", info.name.c_str());
						BitStreamWriter f;
						N.WritePlayerStartData(f, info);
						N.SendServer(f, MEDIUM_PRIORITY, RELIABLE, info.adr, Stream_TransferServer);
					}
					else if(type == 1)
					{
						Info("NM_TRANSFER_SERVER: %s is ready.", info.name.c_str());
						info.ready = true;
					}
					else
					{
						Warn("NM_TRANSFER_SERVER: Unknown ID_READY %u from %s.", type, info.name.c_str());
						N.StreamError();
					}
				}
				else
				{
					Warn("NM_TRANSFER_SERVER: Broken packet ID_READY from %s.", info.name.c_str());
					N.StreamError();
				}
			}
			break;
		default:
			Warn("NM_TRANSFER_SERVER: Unknown packet from %s: %u.", info.name.c_str(), msg_id);
			N.StreamError();
			break;
		}

		N.StreamEnd();
	}

	if(net_state == NetState::Server_Starting)
	{
		if(!N.mp_load)
		{
			gui->info_box->Show(txGeneratingWorld);

			// send info about generating world
			if(N.active_players > 1)
			{
				byte b[] = { ID_STATE, 0 };
				N.peer->Send((cstring)b, 2, IMMEDIATE_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
				N.StreamWrite(b, 2, Stream_TransferServer, UNASSIGNED_SYSTEM_ADDRESS);
			}

			// do it
			ClearGameVars(true);
			Team.free_recruits = 3 - N.active_players;
			if(Team.free_recruits < 0)
				Team.free_recruits = 0;
			fallback_type = FALLBACK::NONE;
			fallback_t = -0.5f;
			gui->main_menu->visible = false;
			gui->load_screen->visible = true;
			clear_color = Color::Black;
			N.prepare_world = true;
			GenerateWorld();
			QM.InitQuests(devmode);
			N.prepare_world = false;
			if(!sound_mgr->IsMusicDisabled())
			{
				LoadMusic(MusicType::Boss, false);
				LoadMusic(MusicType::Death, false);
				LoadMusic(MusicType::Travel, false);
			}
		}

		net_state = NetState::Server_Initialized;
	}
	else if(net_state == NetState::Server_Initialized)
	{
		// prepare world data if there is any players
		if(N.active_players > 1)
		{
			gui->info_box->Show(txSendingWorld);

			// send info about preparing world data
			byte b[] = { ID_STATE, 1 };
			N.peer->Send((cstring)b, 2, IMMEDIATE_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
			N.StreamWrite(b, 2, Stream_TransferServer, UNASSIGNED_SYSTEM_ADDRESS);

			// prepare & send world data
			BitStreamWriter f;
			N.WriteWorldData(f);
			N.SendAll(f, IMMEDIATE_PRIORITY, RELIABLE, Stream_TransferServer);
			Info("NM_TRANSFER_SERVER: Send world data, size %d.", f.GetSize());
			net_state = NetState::Server_WaitForPlayersToLoadWorld;
			net_timer = mp_timeout;
			for(auto info : N.players)
			{
				if(info->id != Team.my_id)
					info->ready = false;
				else
					info->ready = true;
			}
			gui->info_box->Show(txWaitingForPlayers);
		}
		else
			net_state = NetState::Server_EnterLocation;

		vector<Unit*> prev_team;

		// create team
		if(N.mp_load)
			prev_team = Team.members;
		Team.members.clear();
		Team.active_members.clear();
		L.entering = true;
		const bool in_level = L.is_open;
		int leader_perk = 0;
		for(auto pinfo : N.players)
		{
			auto& info = *pinfo;
			if(info.left != PlayerInfo::LEFT_NO)
				continue;

			Unit* u;
			if(!info.loaded)
			{
				UnitData& ud = *ClassInfo::classes[(int)info.clas].unit_data;

				u = CreateUnit(ud, -1, nullptr, nullptr, in_level, true);
				info.u = u;
				u->ApplyHumanData(info.hd);
				u->mesh_inst->need_update = true;

				u->fake_unit = true; // to prevent sending hp changed message set temporary as fake unit
				u->player = new PlayerController;
				u->player->id = info.id;
				u->player->name = info.name;
				u->player->Init(*u);
				info.cc.Apply(*u->player);
				u->fake_unit = false;

				if(info.cc.HavePerk(Perk::Leader))
					++leader_perk;

				if(N.mp_load)
					PreloadUnit(u);
			}
			else
			{
				PlayerInfo* old = N.FindOldPlayer(info.name.c_str());
				old->loaded = true;
				u = old->u;
				info.u = u;
				u->player->id = info.id;
			}

			Team.members.push_back(u);
			Team.active_members.push_back(u);

			info.pc = u->player;
			u->player->player_info = &info;
			if(info.id == Team.my_id)
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
		Team.CalculatePlayersLevel();

		// add ai
		bool anyone_left = false;
		for(Unit* unit : prev_team)
		{
			if(unit->IsPlayer())
				continue;

			if(unit->hero->free)
				Team.members.push_back(unit);
			else
			{
				if(Team.active_members.size() < Team.GetMaxSize())
				{
					Team.members.push_back(unit);
					Team.active_members.push_back(unit);
				}
				else
				{
					// too many team members, kick ai
					NetChange& c = Add1(Net::changes);
					c.type = NetChange::HERO_LEAVE;
					c.unit = unit;

					AddMultiMsg(Format(txMpNPCLeft, unit->hero->name.c_str()));
					if(L.city_ctx)
						unit->hero->mode = HeroData::Wander;
					else
						unit->hero->mode = HeroData::Leave;
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

		if(!N.mp_load && leader_perk > 0 && Team.GetActiveTeamSize() < Team.GetMaxSize())
		{
			UnitData& ud = ClassInfo::GetRandomData();
			int level = ud.level.x + 2 * (leader_perk - 1);
			Unit* npc = CreateUnit(ud, level, nullptr, nullptr, false);
			npc->ai = new AIController;
			npc->ai->Init(npc);
			npc->hero->know_name = true;
			Team.AddTeamMember(npc, false);
			npc->hero->SetupMelee();
		}
		L.entering = false;

		// recalculate credit if someone left
		if(anyone_left)
			Team.CheckCredit(false, true);

		// set leader
		PlayerInfo* leader_info = N.TryGetPlayer(Team.leader_id);
		if(leader_info)
			Team.leader = leader_info->u;
		else
		{
			Team.leader_id = 0;
			Team.leader = N.GetMe().u;
		}

		// send info
		if(N.active_players > 1)
		{
			byte b[] = { ID_STATE, 2 };
			N.peer->Send((cstring)b, 2, IMMEDIATE_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
			N.StreamWrite(b, 2, Stream_TransferServer, UNASSIGNED_SYSTEM_ADDRESS);
		}
	}
	else if(net_state == NetState::Server_WaitForPlayersToLoadWorld)
	{
		// wait for all players
		bool ok = true;
		for(auto info : N.players)
		{
			if(info->left != PlayerInfo::LEFT_NO)
				continue;
			if(!info->ready)
			{
				ok = false;
				break;
			}
		}
		net_timer -= dt;
		if(!ok && net_timer <= 0.f)
		{
			bool anyone_removed = false;
			for(auto pinfo : N.players)
			{
				auto& info = *pinfo;
				if(info.left != PlayerInfo::LEFT_NO)
					continue;
				if(!info.ready)
				{
					Info("NM_TRANSFER_SERVER: Disconnecting player %s due no response.", info.name.c_str());
					--N.active_players;
					N.players_left = true;
					info.left = PlayerInfo::LEFT_TIMEOUT;
					anyone_removed = true;
				}
			}
			ok = true;
			if(anyone_removed)
			{
				Team.CheckCredit(false, true);
				if(Team.leader_id == -1)
				{
					Team.leader_id = 0;
					Team.leader = N.GetMe().u;
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
		gui->info_box->Show(txLoadingLevel);
		if(!N.mp_load)
			EnterLocation();
		else
		{
			if(!L.is_open)
			{
				// saved on worldmap
				byte b = ID_START;
				N.peer->Send((cstring)&b, 1, IMMEDIATE_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
				N.StreamWrite(&b, 1, Stream_TransferServer, UNASSIGNED_SYSTEM_ADDRESS);

				N.DeleteOldPlayers();

				clear_color = clear_color2;
				game_state = GS_WORLDMAP;
				gui->load_screen->visible = false;
				gui->world_map->Show();
				gui->game_gui->visible = false;
				gui->main_menu->visible = false;
				N.mp_load = false;
				clear_color = Color::White;
				W.SetState(World::State::ON_MAP);
				N.update_timer = 0.f;
				SetMusic(MusicType::Travel);
				ProcessLeftPlayers();
				gui->info_box->CloseDialog();
			}
			else
			{
				LoadingStart(1);

				BitStreamWriter f;
				f << ID_CHANGE_LEVEL;
				f << (byte)W.GetCurrentLocationIndex();
				f << (byte)L.dungeon_level;
				f << (W.GetState() == World::State::INSIDE_ENCOUNTER);
				int ack = N.SendAll(f, HIGH_PRIORITY, RELIABLE_WITH_ACK_RECEIPT, Stream_TransferServer);
				for(auto info : N.players)
				{
					if(info->id == Team.my_id)
						info->state = PlayerInfo::IN_GAME;
					else
					{
						info->state = PlayerInfo::WAITING_FOR_RESPONSE;
						info->ack = ack;
						info->timer = mp_timeout;
					}
				}

				// find player that was in save and is playing now (first check leader)
				Unit* center_unit = nullptr;
				if(Team.leader->player->player_info->loaded)
					center_unit = Team.leader;
				else
				{
					// anyone
					for(auto info : N.players)
					{
						if(info->loaded)
						{
							center_unit = info->u;
							break;
						}
					}
				}

				// set position of new units that didn't exists in save (warp to old unit or level entrance)
				if(center_unit)
				{
					// get positon of unit or building entrance
					Vec3 pos;
					if(center_unit->in_building == -1)
						pos = center_unit->pos;
					else
					{
						InsideBuilding* inside = L.city_ctx->inside_buildings[center_unit->in_building];
						Vec2 p = inside->enter_area.Midpoint();
						pos = Vec3(p.x, inside->enter_y, p.y);
					}

					// warp
					for(auto pinfo : N.players)
					{
						auto& info = *pinfo;
						if(!info.loaded)
						{
							L.local_ctx.units->push_back(info.u);
							L.WarpNearLocation(L.local_ctx, *info.u, pos, 4.f, false, 20);
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

					if(L.city_ctx)
						L.city_ctx->GetEntry(pos, rot);
					else if(L.enter_from >= ENTER_FROM_PORTAL && (portal = L.location->GetPortal(L.enter_from)) != nullptr)
					{
						pos = portal->pos + Vec3(sin(portal->rot) * 2, 0, cos(portal->rot) * 2);
						rot = Clip(portal->rot + PI);
					}
					else if(L.location->type == L_DUNGEON || L.location->type == L_CRYPT)
					{
						InsideLocation* inside = static_cast<InsideLocation*>(L.location);
						InsideLocationLevel& lvl = inside->GetLevelData();
						if(L.enter_from == ENTER_FROM_DOWN_LEVEL)
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
						W.GetOutsideSpawnPoint(pos, rot);

					// warp
					for(auto pinfo : N.players)
					{
						auto& info = *pinfo;
						if(!info.loaded)
						{
							L.local_ctx.units->push_back(info.u);
							L.WarpNearLocation(L.local_ctx, *info.u, pos, L.location->outside ? 4.f : 2.f, false, 20);
							info.u->rot = rot;
							info.u->interp->Reset(info.u->pos, info.u->rot);
						}
					}
				}

				N.DeleteOldPlayers();
				LoadResources("", false);

				net_mode = NM_SERVER_SEND;
				net_state = NetState::Server_Send;
				if(N.active_players > 1)
				{
					prepared_stream.Reset();
					N.WriteLevelData(prepared_stream, false);
					Info("NM_TRANSFER_SERVER: Generated level packet: %d.", prepared_stream.GetNumberOfBytesUsed());
					gui->info_box->Show(txWaitingForPlayers);
				}
			}
		}
	}
}

void Game::UpdateServerSend(float dt)
{
	Packet* packet;
	for(packet = N.peer->Receive(); packet; N.peer->DeallocatePacket(packet), packet = N.peer->Receive())
	{
		BitStream& stream = N.StreamStart(packet, Stream_ServerSend);

		PlayerInfo* ptr_info = N.FindPlayer(packet->systemAddress);
		if(!ptr_info)
		{
			Info("NM_SERVER_SEND: Ignoring packet from %s.", packet->systemAddress.ToString());
			N.StreamEnd();
			continue;
		}
		else if(ptr_info->left != PlayerInfo::LEFT_NO)
		{
			Info("NM_SERVER_SEND: Packet from %s who left game.", ptr_info->name.c_str());
			N.StreamEnd();
			continue;
		}

		PlayerInfo& info = *ptr_info;
		byte msg_id;
		stream.Read(msg_id);

		switch(packet->data[0])
		{
		case ID_DISCONNECTION_NOTIFICATION:
		case ID_CONNECTION_LOST:
			{
				Info("NM_SERVER_SEND: Player %s left game.", info.name.c_str());
				--N.active_players;
				N.players_left = true;
				info.left = (msg_id == ID_CONNECTION_LOST ? PlayerInfo::LEFT_DISCONNECTED : PlayerInfo::LEFT_QUIT);
			}
			return;
		case ID_SND_RECEIPT_ACKED:
			{
				int ack;
				if(!stream.Read(ack))
					N.StreamError("NM_SERVER_SEND: Broken packet ID_SND_RECEIPT_ACKED from %s.", info.name.c_str());
				else if(info.state != PlayerInfo::WAITING_FOR_RESPONSE || ack != info.ack)
				{
					Warn("NM_SERVER_SEND: Unexpected packet ID_SND_RECEIPT_ACKED from %s.", info.name.c_str());
					N.StreamError();
				}
				else
				{
					// send level data
					N.peer->Send(&prepared_stream, HIGH_PRIORITY, RELIABLE, 0, packet->systemAddress, false);
					N.StreamWrite(prepared_stream, Stream_TransferServer, packet->systemAddress);
					info.timer = mp_timeout;
					info.state = PlayerInfo::WAITING_FOR_DATA;
					Info("NM_SERVER_SEND: Send level data to %s.", info.name.c_str());
				}
			}
			break;
		case ID_READY:
			if(info.state == PlayerInfo::IN_GAME || info.state == PlayerInfo::WAITING_FOR_RESPONSE)
				N.StreamError("NM_SERVER_SEND: Unexpected packet ID_READY from %s.", info.name.c_str());
			else if(info.state == PlayerInfo::WAITING_FOR_DATA)
			{
				Info("NM_SERVER_SEND: Send player data to %s.", info.name.c_str());
				info.state = PlayerInfo::WAITING_FOR_DATA2;
				SendPlayerData(info);
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
			N.StreamError();
			break;
		}

		N.StreamEnd();
	}

	bool ok = true;
	for(vector<PlayerInfo*>::iterator it = N.players.begin(), end = N.players.end(); it != end;)
	{
		auto& info = **it;
		if(info.state != PlayerInfo::IN_GAME && info.left == PlayerInfo::LEFT_NO)
		{
			info.timer -= dt;
			if(info.timer <= 0.f)
			{
				// timeout, remove player
				Info("NM_SERVER_SEND: Disconnecting player %s due to no response.", info.name.c_str());
				N.peer->CloseConnection(info.adr, true, 0, IMMEDIATE_PRIORITY);
				--N.active_players;
				N.players_left = true;
				info.left = PlayerInfo::LEFT_TIMEOUT;
			}
			else
			{
				ok = false;
				++it;
			}
		}
		else
			++it;
	}
	if(ok)
	{
		if(N.active_players > 1)
		{
			byte b = ID_START;
			N.peer->Send((cstring)&b, 1, HIGH_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
			N.StreamWrite(&b, 1, Stream_TransferServer, UNASSIGNED_SYSTEM_ADDRESS);
		}
		for(vector<Unit*>::iterator it = L.local_ctx.units->begin(), end = L.local_ctx.units->end(); it != end; ++it)
			(*it)->changed = false;
		if(L.city_ctx)
		{
			for(vector<InsideBuilding*>::iterator it = L.city_ctx->inside_buildings.begin(), end = L.city_ctx->inside_buildings.end(); it != end; ++it)
			{
				for(vector<Unit*>::iterator it2 = (*it)->units.begin(), end2 = (*it)->units.end(); it2 != end2; ++it2)
					(*it2)->changed = false;
			}
		}
		Info("NM_SERVER_SEND: All players ready. Starting game.");
		clear_color = clear_color2;
		game_state = GS_LEVEL;
		gui->info_box->CloseDialog();
		gui->load_screen->visible = false;
		gui->main_menu->visible = false;
		gui->game_gui->visible = true;
		gui->world_map->Hide();
		N.mp_load = false;
		SetMusic();
		gui->Setup(pc);
		ProcessLeftPlayers();
	}
}

void Game::UpdateServerQuiting(float dt)
{
	Packet* packet;
	for(packet = N.peer->Receive(); packet; N.peer->DeallocatePacket(packet), packet = N.peer->Receive())
	{
		BitStream& stream = N.StreamStart(packet, Stream_QuittingServer);
		PlayerInfo* ptr_info = N.FindPlayer(packet->systemAddress);
		if(!ptr_info)
		{
			N.StreamError();
			Warn("NM_QUITTING_SERVER: Ignoring packet from unconnected player %s.", packet->systemAddress.ToString());
		}
		else
		{
			PlayerInfo& info = *ptr_info;
			byte msg_id;
			stream.Read(msg_id);
			if(msg_id == ID_DISCONNECTION_NOTIFICATION || msg_id == ID_CONNECTION_LOST)
			{
				if(info.state == PlayerInfo::IN_LOBBY)
					Info("NM_QUITTING_SERVER: Player %s left lobby.", info.name.c_str());
				else
					Info("NM_QUITTING_SERVER: %s left lobby.", packet->systemAddress.ToString());
				--N.active_players;
				if(N.active_players == 0)
				{
					N.StreamEnd();
					N.peer->DeallocatePacket(packet);
					Info("NM_QUITTING_SERVER: All players disconnected from server. Closing...");
					N.ClosePeer();
					gui->info_box->CloseDialog();
					if(gui->server->visible)
						gui->server->CloseDialog();
					if(net_callback)
						net_callback();
					return;
				}
			}
			else
			{
				Info("NM_QUITTING_SERVER: Ignoring packet from %s.",
					info.state == PlayerInfo::IN_LOBBY ? info.name.c_str() : packet->systemAddress.ToString());
			}
			N.StreamEnd();
		}
	}

	net_timer -= dt;
	if(net_timer <= 0.f)
	{
		Warn("NM_QUITTING_SERVER: Not all players disconnected on time. Closing server...");
		N.ClosePeer(N.master_server_state >= MasterServerState::Connecting);
		gui->info_box->CloseDialog();
		if(gui->server->visible)
			gui->server->CloseDialog();
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
		ConnectionAttemptResult result = N.peer->Connect(LobbyApi::API_URL, (word)LobbyApi::PROXY_PORT, nullptr, 0);
		if(result == CONNECTION_ATTEMPT_STARTED)
		{
			net_state = NetState::Client_ConnectingProxy;
			net_timer = T_CONNECT;
			gui->info_box->Show(txConnectingProxy);
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
		ConnectionAttemptResult result = N.peer->Connect(N.ping_adr.ToString(false), N.ping_adr.GetPort(), enter_pswd.c_str(), enter_pswd.length());
		if(result == CONNECTION_ATTEMPT_STARTED)
		{
			net_state = NetState::Client_Connecting;
			net_timer = T_CONNECT;
			gui->info_box->Show(Format(txConnectingTo, gui->server->server_name.c_str()));
			Info("NM_CONNECTING(1): Connecting to server %s...", gui->server->server_name.c_str());
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
	DoPseudotick();
}

void Game::QuickJoinIp()
{
	SystemAddress adr = UNASSIGNED_SYSTEM_ADDRESS;
	if(adr.FromString(server_ip.c_str()) && adr != UNASSIGNED_SYSTEM_ADDRESS)
	{
		try
		{
			N.InitClient();
		}
		catch(cstring error)
		{
			GUI.SimpleDialog(error, nullptr);
			return;
		}

		Info("Pinging %s...", server_ip.c_str());
		Net::SetMode(Net::Mode::Client);
		gui->info_box->Show(txConnecting);
		net_mode = NM_CONNECTING;
		net_state = NetState::Client_PingIp;
		net_timer = T_CONNECT_PING;
		net_tries = I_CONNECT_TRIES;
#ifdef _DEBUG
		net_tries *= 2;
#endif
		N.ping_adr = adr;
		N.peer->Ping(N.ping_adr.ToString(false), N.ping_adr.GetPort(), false);
	}
	else
		Warn("Can't quick connect to server, invalid ip.");
}

void Game::AddMultiMsg(cstring _msg)
{
	assert(_msg);

	gui->mp_box->itb.Add(_msg);
}

void Game::Quit()
{
	Info("Game: Quit.");

	gui->main_menu->ShutdownThread();
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
	if(gui->info_box->visible)
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
			gui->info_box->Show(txDisconnecting);
			ForceRedraw();
			N.ClosePeer(true, true);
			f();
			break;
		case NM_TRANSFER_SERVER:
		case NM_SERVER_SEND:
			Info("ServerPanel: Closing server.");

			// disallow new connections
			N.peer->SetMaximumIncomingConnections(0);
			N.peer->SetOfflinePingResponse(nullptr, 0);

			if(N.active_players > 1)
			{
				// disconnect players
				Info("ServerPanel: Disconnecting clients.");
				const byte b[] = { ID_SERVER_CLOSE, ServerClose_Closing };
				N.peer->Send((cstring)b, 2, IMMEDIATE_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
				N.StreamWrite(b, 2, Stream_TransferServer, UNASSIGNED_SYSTEM_ADDRESS);
				net_mode = Game::NM_QUITTING_SERVER;
				--N.active_players;
				net_timer = T_WAIT_FOR_DISCONNECT;
				gui->info_box->Show(gui->server->txDisconnecting);
				net_callback = f;
			}
			else
			{
				// no players, close instantly
				gui->info_box->CloseDialog();
				N.ClosePeer(false, true);
				f();
			}
			break;
		}
	}
	else if(gui->server->visible)
		gui->server->ExitLobby(f);
	else
	{
		if(Net::IsServer())
		{
			Info("ServerPanel: Closing server.");

			// disallow new connections
			N.peer->SetMaximumIncomingConnections(0);
			N.peer->SetOfflinePingResponse(nullptr, 0);

			if(N.active_players > 1)
			{
				// disconnect players
				Info("ServerPanel: Disconnecting clients.");
				const byte b[] = { ID_SERVER_CLOSE, ServerClose_Closing };
				N.peer->Send((cstring)b, 2, IMMEDIATE_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
				N.StreamWrite(b, 2, Stream_TransferServer, UNASSIGNED_SYSTEM_ADDRESS);
				net_mode = Game::NM_QUITTING_SERVER;
				--N.active_players;
				net_timer = T_WAIT_FOR_DISCONNECT;
				gui->info_box->Show(gui->server->txDisconnecting);
				net_callback = f;
			}
			else
			{
				// no players, close instantly
				N.ClosePeer(false, true);
				if(f)
					f();
			}
		}
		else
		{
			gui->info_box->Show(txDisconnecting);
			ForceRedraw();
			N.ClosePeer(true, true);
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
		PlayerInfo& info = N.GetMe();
		gui->server->bts[1].state = Button::NONE;
		gui->server->bts[0].text = gui->server->txChangeChar;
		// set data
		Class old_class = info.clas;
		info.clas = gui->create_character->clas;
		info.hd.Get(*gui->create_character->unit->human_data);
		info.cc = gui->create_character->cc;
		// send info to other N.active_players about changing my class
		if(Net::IsServer())
		{
			if(info.clas != old_class && N.active_players > 1)
				gui->server->AddLobbyUpdate(Int2(Lobby_UpdatePlayer, 0));
		}
		else
		{
			BitStreamWriter f;
			f << ID_PICK_CHARACTER;
			WriteCharacterData(f, info.clas, info.hd, info.cc);
			f << false;
			N.SendClient(f, IMMEDIATE_PRIORITY, RELIABLE, Stream_UpdateLobbyClient);
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

			GUI.ShowDialog(info);
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
		GUI.GetDialog("tutorial_dialog")->visible = false;
		SaveOptions();
		if(id == BUTTON_YES)
			QM.quest_tutorial->Start();
		else
			StartNewGame();
	}
}

void Game::OnPickServer(int id)
{
	enter_pswd.clear();

	if(!gui->pick_server->IsLAN())
	{
		// connect to proxy server for nat punchthrough
		PickServerPanel::ServerData& info = gui->pick_server->servers[gui->pick_server->grid.selected];
		if(IS_SET(info.flags, SERVER_PASSWORD))
		{
			net_mode = NM_CONNECTING;
			net_state = NetState::Client_WaitingForPasswordProxy;
			net_tries = 1;
			gui->info_box->Show(txWaitingForPswd);
			GetTextDialogParams params(Format(txEnterPswd, gui->server->server_name.c_str()), enter_pswd);
			params.event = DialogEvent(this, &Game::OnEnterPassword);
			params.limit = 16;
			params.parent = gui->info_box;
			GetTextDialog::Show(params);
			Info("OnPickServer: Waiting for password...");
		}
		else
		{
			ConnectionAttemptResult result = N.peer->Connect(LobbyApi::API_URL, (word)LobbyApi::PROXY_PORT, nullptr, 0);
			if(result == CONNECTION_ATTEMPT_STARTED)
			{
				net_mode = NM_CONNECTING;
				net_state = NetState::Client_ConnectingProxy;
				net_timer = T_CONNECT;
				gui->info_box->Show(txConnectingProxy);
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
		PickServerPanel::ServerData& info = gui->pick_server->servers[gui->pick_server->grid.selected];
		N.server = info.adr;
		gui->server->server_name = info.name;
		gui->server->max_players = info.max_players;
		N.active_players = info.active_players;
		if(IS_SET(info.flags, SERVER_PASSWORD))
		{
			// enter password
			net_mode = NM_CONNECTING;
			net_state = NetState::Client_WaitingForPassword;
			net_tries = 1;
			N.ping_adr = info.adr;
			gui->info_box->Show(txWaitingForPswd);
			GetTextDialogParams params(Format(txEnterPswd, gui->server->server_name.c_str()), enter_pswd);
			params.event = DialogEvent(this, &Game::OnEnterPassword);
			params.limit = 16;
			params.parent = gui->info_box;
			GetTextDialog::Show(params);
			Info("OnPickServer: Waiting for password...");
		}
		else
		{
			// connect
			ConnectionAttemptResult result = N.peer->Connect(info.adr.ToString(false), (word)N.port, nullptr, 0);
			if(result == CONNECTION_ATTEMPT_STARTED)
			{
				// connecting in progress
				net_mode = NM_CONNECTING;
				net_timer = T_CONNECT;
				net_tries = 1;
				net_state = NetState::Client_Connecting;
				gui->info_box->Show(Format(txConnectingTo, info.name.c_str()));
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
	N.ClosePeer(false, true);
	ExitToMenu();
	GUI.SimpleDialog(msg, gui->main_menu);
}
