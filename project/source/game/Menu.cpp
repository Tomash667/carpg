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

extern string g_ctime;

// STA£E
const float T_WAIT_FOR_HELLO = 5.f;
const float T_TRY_CONNECT = 5.f;
const float T_LOBBY_UPDATE = 0.2f;
const int T_SHUTDOWN = 1000;
const float T_WAIT_FOR_DATA = 5.f;

//=================================================================================================
bool Game::CanShowMenu()
{
	return !GUI.HaveDialog() && !game_gui->HavePanelOpen() && !main_menu->visible && game_state != GS_MAIN_MENU && death_screen != 3 && !koniec_gry
		&& !dialog_context.dialog_mode;
}

//=================================================================================================
void Game::ShowMenu()
{
	GUI.ShowDialog(game_menu);
}

//=================================================================================================
void Game::MainMenuEvent(int id)
{
	switch(id)
	{
	case MainMenu::IdNewGame:
		Net::SetMode(Net::Mode::Singleplayer);
		ShowCreateCharacterPanel(true);
		break;
	case MainMenu::IdLoadGame:
		Net::SetMode(Net::Mode::Singleplayer);
		ShowLoadPanel();
		break;
	case MainMenu::IdMultiplayer:
		mp_load = false;
		multiplayer_panel->Show();
		break;
	case MainMenu::IdOptions:
		ShowOptions();
		break;
	case MainMenu::IdInfo:
		GUI.SimpleDialog(Format(main_menu->txInfoText, VERSION_STR, g_ctime.c_str()), nullptr);
		break;
	case MainMenu::IdWebsite:
		io::OpenUrl(Format("http://carpg.pl/redirect.php?language=%s", Language::prefix.c_str()));
		break;
	case MainMenu::IdQuit:
		Quit();
		break;
	}
}

//=================================================================================================
void Game::MenuEvent(int index)
{
	switch(index)
	{
	case GameMenu::IdReturnToGame: // wróæ
		GUI.CloseDialog(game_menu);
		break;
	case GameMenu::IdSaveGame: // zapisz
		ShowSavePanel();
		break;
	case GameMenu::IdLoadGame: // wczytaj
		ShowLoadPanel();
		break;
	case GameMenu::IdOptions: // opcje
		ShowOptions();
		break;
	case GameMenu::IdExit: // wróæ do menu
		{
			DialogInfo info;
			info.event = delegate<void(int)>(this, &Game::OnExit);
			info.name = "exit_to_menu";
			info.parent = nullptr;
			info.pause = true;
			info.text = game_menu->txExitToMenuDialog;
			info.order = ORDER_TOP;
			info.type = DIALOG_YESNO;

			GUI.ShowDialog(info);
		}
		break;
	case GameMenu::IdQuit: // wyjdŸ
		ShowQuitDialog();
		break;
	}
}

//=================================================================================================
void Game::OptionsEvent(int index)
{
	if(index == Options::IdOk)
	{
		GUI.CloseDialog(options);
		SaveOptions();
		return;
	}

	switch(index)
	{
	case Options::IdFullscreen:
		ChangeMode(options->check[0].checked);
		break;
	case Options::IdChangeRes:
		break;
	case Options::IdSoundVolume:
		sound_mgr->SetSoundVolume(options->sound_volume);
		break;
	case Options::IdMusicVolume:
		sound_mgr->SetMusicVolume(options->music_volume);
		break;
	case Options::IdMouseSensitivity:
		mouse_sensitivity = options->mouse_sensitivity;
		mouse_sensitivity_f = Lerp(0.5f, 1.5f, float(mouse_sensitivity) / 100);
		break;
	case Options::IdGrassRange:
		grass_range = (float)options->grass_range;
		break;
	case Options::IdControls:
		GUI.ShowDialog(controls);
		break;
	case Options::IdGlow:
		cl_glow = options->check[1].checked;
		break;
	case Options::IdNormal:
		cl_normalmap = options->check[2].checked;
		break;
	case Options::IdSpecular:
		cl_specularmap = options->check[3].checked;
		break;
	case Options::IdVsync:
		SetVsync(!GetVsync());
		break;
	}
}

//=================================================================================================
void Game::SaveLoadEvent(int id)
{
	if(id == SaveLoad::IdCancel)
	{
		mp_load = false;
		GUI.CloseDialog(saveload);
	}
	else
	{
		// zapisz/wczytaj
		if(saveload->save_mode)
		{
			// zapisywanie
			SaveSlot& slot = saveload->slots[saveload->choice];
			if(saveload->choice == MAX_SAVE_SLOTS - 1)
			{
				// szybki zapis
				GUI.CloseDialog(saveload);
				SaveGameSlot(saveload->choice + 1, saveload->txQuickSave);
			}
			else
			{
				// podaj tytu³ zapisu
				cstring names[] = { nullptr, saveload->txSave };
				if(slot.valid)
					save_input_text = slot.text;
				else if(hardcore_mode)
					save_input_text = hardcore_savename;
				else
					save_input_text.clear();
				GetTextDialogParams params(saveload->txSaveName, save_input_text);
				params.custom_names = names;
				params.event = delegate<void(int)>(this, &Game::SaveEvent);
				params.parent = saveload;
				GetTextDialog::Show(params);
			}
		}
		else
		{
			// wczytywanie
			try
			{
				GUI.CloseDialog(saveload);
				if(game_menu->visible)
					GUI.CloseDialog(game_menu);

				LoadGameSlot(saveload->choice + 1);

				if(mp_load)
					create_server_panel->Show();
			}
			catch(const SaveException& ex)
			{
				Error("Failed to load game: %s", ex.msg);
				cstring dialog_text;
				if(ex.localized_msg)
					dialog_text = Format("%s%s", txLoadError, ex.localized_msg);
				else
					dialog_text = txLoadErrorGeneric;
				GUI.SimpleDialog(dialog_text, saveload);
				mp_load = false;
			}
		}
	}
}

//=================================================================================================
void Game::SaveEvent(int id)
{
	if(id == BUTTON_OK)
	{
		if(SaveGameSlot(saveload->choice + 1, save_input_text.c_str()))
		{
			// zapisywanie siê uda³o, zamknij okno zapisu
			GUI.CloseDialog(saveload);
		}
	}
}

//=================================================================================================
void Game::SaveOptions()
{
	cfg.Add("fullscreen", IsFullscreen());
	cfg.Add("cl_glow", cl_glow);
	cfg.Add("cl_normalmap", cl_normalmap);
	cfg.Add("cl_specularmap", cl_specularmap);
	cfg.Add("sound_volume", Format("%d", sound_mgr->GetSoundVolume()));
	cfg.Add("music_volume", Format("%d", sound_mgr->GetMusicVolume()));
	cfg.Add("mouse_sensitivity", Format("%d", mouse_sensitivity));
	cfg.Add("grass_range", Format("%g", grass_range));
	cfg.Add("resolution", Format("%dx%d", GetWindowSize().x, GetWindowSize().y));
	cfg.Add("refresh", Format("%d", wnd_hz));
	cfg.Add("skip_tutorial", skip_tutorial);
	cfg.Add("language", Language::prefix.c_str());
	int ms, msq;
	GetMultisampling(ms, msq);
	cfg.Add("multisampling", Format("%d", ms));
	cfg.Add("multisampling_quality", Format("%d", msq));
	cfg.Add("vsync", GetVsync());
	SaveCfg();
}

//=================================================================================================
void Game::ShowOptions()
{
	GUI.ShowDialog(options);
}

//=================================================================================================
void Game::ShowSavePanel()
{
	saveload->SetSaveMode(true, Net::IsOnline(), Net::IsOnline() ? multi_saves : single_saves);
	GUI.ShowDialog(saveload);
}

//=================================================================================================
void Game::ShowLoadPanel()
{
	saveload->SetSaveMode(false, mp_load, mp_load ? multi_saves : single_saves);
	GUI.ShowDialog(saveload);
}

//=================================================================================================
void Game::StartNewGame()
{
	HumanData hd;
	hd.Get(*create_character->unit->human_data);
	NewGameCommon(create_character->clas, create_character->player_name.c_str(), hd, create_character->cc, false);
}

//=================================================================================================
void Game::OnQuit(int id)
{
	if(id == BUTTON_YES)
	{
		GUI.GetDialog("dialog_alt_f4")->visible = false;
		Quit();
	}
}

//=================================================================================================
void Game::OnExit(int id)
{
	if(id == BUTTON_YES)
		ExitToMenu();
}

//=================================================================================================
void Game::ShowCreateCharacterPanel(bool require_name, bool redo)
{
	if(redo)
	{
		PlayerInfo& info = N.GetMe();
		create_character->ShowRedo(info.clas, hair_redo_index, info.hd, info.cc);
	}
	else
		create_character->Show(require_name);
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
	in_tutorial = tutorial;
	main_menu->visible = false;
	Net::SetMode(Net::Mode::Singleplayer);
	game_state = GS_LEVEL;
	hardcore_mode = hardcore_option;

	UnitData& ud = *ClassInfo::classes[(int)clas].unit_data;

	Unit* u = CreateUnit(ud, -1, nullptr, nullptr, false);
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
	if(finished_tutorial)
	{
		u->AddItem(Item::Get("book_adventurer"), 1u, false);
		finished_tutorial = false;
	}
	dialog_context.pc = pc;

	ClearGameVarsOnNewGame();
	SetGamePanels();

	if(!tutorial && cc.HavePerk(Perk::Leader))
	{
		Unit* npc = CreateUnit(ClassInfo::GetRandomData(), 2, nullptr, nullptr, false);
		npc->ai = new AIController;
		npc->ai->Init(npc);
		npc->hero->know_name = true;
		Team.AddTeamMember(npc, false);
		Team.free_recruit = false;
		npc->hero->SetupMelee();
	}
	game_gui->Setup();

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
	player_name = multiplayer_panel->textbox.GetText();

	if(id == MultiplayerPanel::IdCancel)
	{
		multiplayer_panel->CloseDialog();
		return;
	}

	// sprawdŸ czy podano nick
	if(player_name.empty())
	{
		GUI.SimpleDialog(multiplayer_panel->txNeedEnterNick, multiplayer_panel);
		return;
	}

	// sprawdŸ czy nick jest poprawny
	if(!ValidateNick(player_name.c_str()))
	{
		GUI.SimpleDialog(multiplayer_panel->txEnterValidNick, multiplayer_panel);
		return;
	}

	// zapisz nick
	cfg.Add("nick", player_name.c_str());
	SaveCfg();

	switch(id)
	{
	case MultiplayerPanel::IdJoinLan:
		// do³¹cz lan
		pick_server_panel->Show();
		break;
	case MultiplayerPanel::IdJoinIp:
		{
			// wyœwietl okno na podawanie adresu ip
			GetTextDialogParams params(txEnterIp, server_ip);
			params.event = DialogEvent(this, &Game::OnEnterIp);
			params.limit = 100;
			params.parent = multiplayer_panel;
			GetTextDialog::Show(params);
		}
		break;
	case MultiplayerPanel::IdCreate:
		// za³ó¿ serwer
		create_server_panel->Show();
		break;
	case MultiplayerPanel::IdLoad:
		// wczytaj grê
		mp_load = true;
		Net::changes.clear();
		if(!net_talk.empty())
			StringPool.Free(net_talk);
		ShowLoadPanel();
		break;
	}
}

//=================================================================================================
void Game::CreateServerEvent(int id)
{
	if(id == BUTTON_CANCEL)
	{
		if(mp_load)
		{
			ClearGame();
			mp_load = false;
		}
		create_server_panel->CloseDialog();
	}
	else
	{
		// kopiuj
		N.server_name = create_server_panel->textbox[0].GetText();
		N.max_players = atoi(create_server_panel->textbox[1].GetText().c_str());
		N.password = create_server_panel->textbox[2].GetText();

		// sprawdŸ dane
		cstring error_text = nullptr;
		Control* give_focus = nullptr;
		if(N.server_name.empty())
		{
			error_text = create_server_panel->txEnterServerName;
			give_focus = &create_server_panel->textbox[0];
		}
		else if(!InRange(N.max_players, MIN_PLAYERS, MAX_PLAYERS))
		{
			error_text = Format(create_server_panel->txInvalidPlayersCount, MAX_PLAYERS);
			give_focus = &create_server_panel->textbox[1];
		}
		if(error_text)
		{
			GUI.SimpleDialog(error_text, create_server_panel);
			// daj focus textboxowi w którym by³ b³¹d
			create_server_panel->cont.give_focus = give_focus;
			return;
		}

		// zapisz
		cfg.Add("server_name", N.server_name.c_str());
		cfg.Add("server_pswd", N.password.c_str());
		cfg.Add("server_players", Format("%d", N.max_players));
		SaveCfg();

		// zamknij okna
		create_server_panel->CloseDialog();
		multiplayer_panel->CloseDialog();

		// jeœli ok to uruchom serwer
		try
		{
			N.InitServer();
		}
		catch(cstring err)
		{
			GUI.SimpleDialog(err, main_menu);
			return;
		}

		// przejdŸ do okna serwera
		server_panel->Show();
		Net_OnNewGameServer();
		N.UpdateServerInfo();

		if(change_title_a)
			ChangeTitle();
	}
}

//=================================================================================================
void Game::CheckReady()
{
	bool all_ready = true;

	for(PlayerInfo* info : N.players)
	{
		if(!info->ready)
		{
			all_ready = false;
			break;
		}
	}

	if(all_ready)
		server_panel->bts[4].state = Button::NONE;
	else
		server_panel->bts[4].state = Button::DISABLED;

	if(N.starting)
		server_panel->StopStartup();
}

//=================================================================================================
void Game::ChangeReady()
{
	if(Net::IsServer())
	{
		// zmieñ info
		if(N.active_players > 1)
			AddLobbyUpdate(Int2(Lobby_UpdatePlayer, 0));
		CheckReady();
	}
	else
	{
		PlayerInfo& info = N.GetMe();
		byte b[] = { ID_CHANGE_READY, (byte)(info.ready ? 1 : 0) };
		N.peer->Send((cstring)b, 2, HIGH_PRIORITY, RELIABLE_ORDERED, 1, server, false);
		N.StreamWrite(b, 2, Stream_UpdateLobbyClient, server);
	}

	server_panel->bts[1].text = (N.GetMe().ready ? server_panel->txNotReady : server_panel->txReady);
}

//=================================================================================================
void Game::AddMsg(cstring msg)
{
	if(server_panel->visible)
		server_panel->AddMsg(msg);
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
		if(adr.FromString(server_ip.c_str()) && adr != UNASSIGNED_SYSTEM_ADDRESS)
		{
			// zapisz ip
			cfg.Add("server_ip", server_ip.c_str());
			SaveCfg();

			try
			{
				N.InitClient();
			}
			catch(cstring error)
			{
				GUI.SimpleDialog(error, multiplayer_panel);
				return;
			}

			Info("Pinging %s...", server_ip.c_str());
			info_box->Show(txConnecting);
			net_mode = NM_CONNECT_IP;
			net_state = NetState::Client_PingIp;
			net_timer = T_CONNECT_PING;
			net_tries = I_CONNECT_TRIES;
			net_adr = adr.ToString(false);
			N.peer->Ping(net_adr.c_str(), (word)N.port, false);
		}
		else
			GUI.SimpleDialog(txInvalidIp, multiplayer_panel);
	}
}

//=================================================================================================
void Game::EndConnecting(cstring msg, bool wait)
{
	info_box->CloseDialog();
	if(msg)
		GUI.SimpleDialog(msg, pick_server_panel->visible ? (DialogBox*)pick_server_panel : (DialogBox*)multiplayer_panel);
	if(wait)
		ForceRedraw();
	ClosePeer(wait);
}

//=================================================================================================
void Game::GenericInfoBoxUpdate(float dt)
{
	switch(net_mode)
	{
	case NM_CONNECT_IP:
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
				Warn("NM_CONNECT_IP(0): Connection timeout.");
				EndConnecting(txConnectTimeout);
				return;
			}
			else
			{
				// ping another time...
				Info("Pinging %s...", net_adr.c_str());
				N.peer->Ping(net_adr.c_str(), (word)N.port, false);
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
				Warn("NM_CONNECT_IP(0): Unknown server response: %u.", msg_id);
				N.StreamError();
				continue;
			}

			if(packet->length == sizeof(TimeMS) + 1)
			{
				Warn("NM_CONNECT_IP(0): Server not set SetOfflinePingResponse yet.");
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
				Warn("NM_CONNECT_IP(0): Broken server response.");
				N.StreamError();
				continue;
			}
			if(sign_ca[0] != 'C' || sign_ca[1] != 'A')
			{
				// invalid signature, this is not carpg server
				Warn("NM_CONNECT_IP(0): Invalid server signature 0x%x%x.", byte(sign_ca[0]), byte(sign_ca[1]));
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
				N.StreamError("NM_CONNECT_IP(0): Broken server message.");
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
				Error("NM_CONNECT_IP(0): Invalid client version '%s' vs server '%s'.", VERSION_STR, s);
				N.peer->DeallocatePacket(packet);
				EndConnecting(Format(txConnectVersion, VERSION_STR, s));
				return;
			}

			// set server status
			max_players2 = max_players;
			server_name2 = server_name_r;
			Info("NM_CONNECT_IP(0): Server information. Name:%s; players:%d/%d; flags:%d.", server_name2.c_str(), players, max_players, flags);
			if(IS_SET(flags, 0xFC))
				Warn("NM_CONNECT_IP(0): Unknown server flags.");
			server = packet->systemAddress;
			enter_pswd.clear();

			if(IS_SET(flags, 0x01))
			{
				// password is required
				net_state = NetState::Client_WaitingForPassword;
				info_box->Show(txWaitingForPswd);
				GetTextDialogParams params(Format(txEnterPswd, server_name2.c_str()), enter_pswd);
				params.event = DialogEvent(this, &Game::OnEnterPassword);
				params.limit = 16;
				params.parent = info_box;
				GetTextDialog::Show(params);
				Info("NM_CONNECT_IP(0): Waiting for password...");
			}
			else
			{
				// try to connect
				ConnectionAttemptResult result = N.peer->Connect(server.ToString(false), (word)N.port, nullptr, 0);
				if(result == CONNECTION_ATTEMPT_STARTED)
				{
					// connecting...
					net_timer = T_CONNECT;
					net_state = NetState::Client_Connecting;
					info_box->Show(Format(txConnectingTo, server_name2.c_str()));
					Info("NM_CONNECT_IP(0): Connecting to server %s...", server_name2.c_str());
				}
				else
				{
					// something went wrong
					Error("NM_CONNECT_IP(0): Can't connect to server: raknet error %d.", result);
					EndConnecting(txConnectRaknet);
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
					Info("NM_CONNECT_IP(2): Connected with server.");
					net_stream.Reset();
					BitStreamWriter f(net_stream);
					f << ID_HELLO;
					f << VERSION;
					content::WriteCrc(f);
					f << player_name;
					N.peer->Send(&net_stream, IMMEDIATE_PRIORITY, RELIABLE, 0, server, false);
					N.StreamWrite(net_stream, Stream_Connect, server);
				}
				break;
			case ID_JOIN:
				// server accepted and sent info about players and my id
				{
					int count, load_char;
					reader.ReadCasted<byte>(my_id);
					reader.ReadCasted<byte>(N.active_players);
					reader.ReadCasted<byte>(leader_id);
					reader.ReadCasted<byte>(count);
					if(!reader)
					{
						N.StreamError("NM_CONNECT_IP(2): Broken packet ID_JOIN.");
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
					info.id = my_id;
					info.state = PlayerInfo::IN_LOBBY;
					info.update_flags = 0;
					info.left = PlayerInfo::LEFT_NO;
					info.loaded = false;
					info.buffs = 0;

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
							N.StreamError("NM_CONNECT_IP(2): Broken packet ID_JOIN(2).");
							N.peer->DeallocatePacket(packet);
							EndConnecting(txCantJoin, true);
							return;
						}

						// verify player class
						if(!ClassInfo::IsPickable(info2.clas) && info2.clas != Class::INVALID)
						{
							N.StreamError("NM_CONNECT_IP(2): Broken packet ID_JOIN, player %s has class %d.", info2.name.c_str(), info2.clas);
							N.peer->DeallocatePacket(packet);
							EndConnecting(txCantJoin, true);
							return;
						}
					}

					// read save information
					reader.ReadCasted<byte>(load_char);
					if(!reader)
					{
						N.StreamError("NM_CONNECT_IP(2): Broken packet ID_JOIN(4).");
						N.peer->DeallocatePacket(packet);
						EndConnecting(txCantJoin, true);
						return;
					}
					reader.ReadCasted<byte>(info.clas);
					if(load_char == 2 && !reader)
					{
						N.StreamError("NM_CONNECT_IP(2): Broken packet ID_JOIN(3).");
						N.peer->DeallocatePacket(packet);
						EndConnecting(txCantJoin, true);
						return;
					}

					N.StreamEnd();
					N.peer->DeallocatePacket(packet);

					// read leader
					int index = GetPlayerIndex(leader_id);
					if(index == -1)
					{
						Error("NM_CONNECT_IP(2): Broken packet ID_JOIN, no player with leader id %d.", leader_id);
						EndConnecting(txCantJoin, true);
						return;
					}

					// go to lobby
					if(multiplayer_panel->visible)
						multiplayer_panel->CloseDialog();
					server_panel->Show();
					server_panel->grid.AddItems(count + 1);
					if(load_char != 0)
						server_panel->UseLoadedCharacter(load_char == 2);
					if(load_char != 2)
						server_panel->CheckAutopick();
					info_box->CloseDialog();
					if(change_title_a)
						ChangeTitle();
					return;
				}
				break;
			case ID_CANT_JOIN:
				// server refused to join
				{
					cstring reason, reason_eng;

					const JoinResult type = (packet->length >= 2 ? (JoinResult)packet->data[1] : JoinResult::OtherError);

					switch(type)
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
								if(content::GetCrc((content::Id)type, my_crc, type_str))
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
						Warn("NM_CONNECT_IP(2): Can't connect to server: %s.", reason_eng);
					else
						Warn("NM_CONNECT_IP(2): Can't connect to server (%d).", type);
					if(reason)
						EndConnecting(Format("%s:\n%s", txCantJoin2, reason), true);
					else
						EndConnecting(txCantJoin2, true);
					return;
				}
			case ID_CONNECTION_LOST:
			case ID_DISCONNECTION_NOTIFICATION:
				// lost connecting with server or was kicked out
				Warn(msg_id == ID_CONNECTION_LOST ? "NM_CONNECT_IP(2): Lost connection with server." : "NM_CONNECT_IP(2): Disconnected from server.");
				N.StreamEnd();
				N.peer->DeallocatePacket(packet);
				EndConnecting(txLostConnection);
				return;
			case ID_INVALID_PASSWORD:
				// password is invalid
				Warn("NM_CONNECT_IP(2): Invalid password.");
				N.StreamEnd();
				N.peer->DeallocatePacket(packet);
				EndConnecting(txInvalidPswd);
				return;
			default:
				N.StreamError();
				Warn("NM_CONNECT_IP(2): Unknown packet from server %u.", msg_id);
				break;
			}

			N.StreamEnd();
		}

		net_timer -= dt;
		if(net_timer <= 0.f)
		{
			Warn("NM_CONNECT_IP(2): Connection timeout.");
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
			ClosePeer();
			info_box->CloseDialog();
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
						info_box->Show(txGeneratingWorld);
						break;
					case 1:
						info_box->Show(txPreparingWorld);
						break;
					case 2:
						info_box->Show(txWaitingForPlayers);
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
				ClearGameVarsOnNewGame();
				Net_OnNewGameClient();

				fallback_type = FALLBACK::NONE;
				fallback_t = -0.5f;
				net_state = NetState::Client_ReceivedWorldData;

				if(ReadWorldData(reader))
				{
					// odeœlij informacje o gotowoœci
					byte b[] = { ID_READY, 0 };
					N.peer->Send((cstring)b, 2, HIGH_PRIORITY, RELIABLE, 0, server, false);
					N.StreamWrite(b, 2, Stream_Transfer, server);
					info_box->Show(txLoadedWorld);
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
				if(ReadPlayerStartData(reader))
				{
					// odeœlij informacje o gotowoœci
					if(mp_load_worldmap)
						LoadResources("", true);
					else
						LoadingStep("");
					byte b[] = { ID_READY, 1 };
					N.peer->Send((cstring)b, 2, HIGH_PRIORITY, RELIABLE, 0, server, false);
					N.StreamWrite(b, 2, Stream_Transfer, server);
					info_box->Show(txLoadedPlayer);
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
						info_box->Show(txGeneratingLocation);
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
				info_box->Show(txLoadingLocation);
				LoadingStep("");
				if(!ReadLevelData(reader))
				{
					N.StreamError("NM_TRANSFER: Failed to read location data.");
					N.peer->DeallocatePacket(packet);
					ClearAndExitToMenu(txLoadingLocationError);
					return;
				}
				else
				{
					Info("NM_TRANSFER: Loaded level data.");
					byte b[] = { ID_READY, 2 };
					N.peer->Send((cstring)b, 2, HIGH_PRIORITY, RELIABLE, 0, server, false);
					N.StreamWrite(b, 2, Stream_Transfer, server);
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
				info_box->Show(txLoadingChars);
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
					mp_load = false;
					byte b[] = { ID_READY, 3 };
					N.peer->Send((cstring)b, 2, HIGH_PRIORITY, RELIABLE, 0, server, false);
					N.StreamWrite(b, 2, Stream_Transfer, server);
				}
			}
			else
				N.StreamError("NM_TRANSFER: Received ID_PLAYER_DATA with net state %d.", net_state);
			break;
		case ID_START:
			if(mp_load_worldmap)
			{
				// start on worldmap
				if(net_state == NetState::Client_ReceivedPlayerStartData)
				{
					mp_load_worldmap = false;
					net_state = NetState::Client_StartOnWorldmap;
					Info("NM_TRANSFER: Starting at world map.");
					clear_color = Color::White;
					game_state = GS_WORLDMAP;
					load_screen->visible = false;
					main_menu->visible = false;
					game_gui->visible = false;
					world_map->visible = true;
					W.SetState(World::State::ON_MAP);
					info_box->CloseDialog();
					update_timer = 0.f;
					leader_id = 0;
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
					load_screen->visible = false;
					main_menu->visible = false;
					game_gui->visible = true;
					world_map->visible = false;
					info_box->CloseDialog();
					update_timer = 0.f;
					fallback_type = FALLBACK::NONE;
					fallback_t = -0.5f;
					cam.Reset();
					pc_data.rot_buf = 0.f;
					if(change_title_a)
						ChangeTitle();
					N.StreamEnd();
					N.peer->DeallocatePacket(packet);
					SetGamePanels();
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
			ClosePeer();
			if(server_panel->visible)
				server_panel->CloseDialog();
			if(net_callback)
				net_callback();
			info_box->CloseDialog();
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
		ClosePeer();
		if(server_panel->visible)
			server_panel->CloseDialog();
		if(net_callback)
			net_callback();
		info_box->CloseDialog();
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
				players_left = true;
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
						net_stream2.Reset();
						BitStreamWriter f(net_stream2);
						WritePlayerStartData(f, info);
						N.peer->Send(&net_stream2, MEDIUM_PRIORITY, RELIABLE, 0, info.adr, false);
						N.StreamWrite(net_stream2, Stream_TransferServer, info.adr);
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
		if(!mp_load)
		{
			info_box->Show(txGeneratingWorld);

			// send info about generating world
			if(N.active_players > 1)
			{
				byte b[] = { ID_STATE, 0 };
				N.peer->Send((cstring)b, 2, IMMEDIATE_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
				N.StreamWrite(b, 2, Stream_TransferServer, UNASSIGNED_SYSTEM_ADDRESS);
			}

			// do it
			ClearGameVarsOnNewGame();
			Team.free_recruit = false;
			fallback_type = FALLBACK::NONE;
			fallback_t = -0.5f;
			main_menu->visible = false;
			load_screen->visible = true;
			clear_color = Color::Black;
			GenerateWorld();
			QM.InitQuests(devmode);
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
			info_box->Show(txSendingWorld);

			// send info about preparing world data
			byte b[] = { ID_STATE, 1 };
			N.peer->Send((cstring)b, 2, IMMEDIATE_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
			N.StreamWrite(b, 2, Stream_TransferServer, UNASSIGNED_SYSTEM_ADDRESS);

			// prepare & send world data
			net_stream.Reset();
			BitStreamWriter f(net_stream);
			PrepareWorldData(f);
			N.peer->Send(&net_stream, IMMEDIATE_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
			N.StreamWrite(net_stream, Stream_TransferServer, UNASSIGNED_SYSTEM_ADDRESS);
			Info("NM_TRANSFER_SERVER: Send world data, size %d.", net_stream.GetNumberOfBytesUsed());
			net_state = NetState::Server_WaitForPlayersToLoadWorld;
			net_timer = mp_timeout;
			for(auto info : N.players)
			{
				if(info->id != my_id)
					info->ready = false;
				else
					info->ready = true;
			}
			info_box->Show(txWaitingForPlayers);
		}
		else
			net_state = NetState::Server_EnterLocation;

		vector<Unit*> prev_team;

		// create team
		if(mp_load)
			prev_team = Team.members;
		Team.members.clear();
		Team.active_members.clear();
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

				if(mp_load)
					PreloadUnit(u);
			}
			else
			{
				PlayerInfo* old = FindOldPlayer(info.name.c_str());
				old->loaded = true;
				u = old->u;
				info.u = u;
				u->player->id = info.id;
			}

			Team.members.push_back(u);
			Team.active_members.push_back(u);

			info.pc = u->player;
			u->player->player_info = &info;
			if(info.id == my_id)
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

		if(!mp_load && leader_perk > 0 && Team.GetActiveTeamSize() < Team.GetMaxSize())
		{
			Unit* npc = CreateUnit(ClassInfo::GetRandomData(), 2 * leader_perk, nullptr, nullptr, false);
			npc->ai = new AIController;
			npc->ai->Init(npc);
			npc->hero->know_name = true;
			Team.AddTeamMember(npc, false);
			npc->hero->SetupMelee();
		}

		// recalculate credit if someone left
		if(anyone_left)
			CheckCredit(false, true);

		// set leader
		int index = GetPlayerIndex(leader_id);
		if(index == -1 || N.players[index]->left != PlayerInfo::LEFT_NO)
		{
			leader_id = 0;
			Team.leader = N.GetMe().u;
		}
		else
			Team.leader = N.players[index]->u;

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
					players_left = true;
					info.left = PlayerInfo::LEFT_TIMEOUT;
					anyone_removed = true;
				}
			}
			ok = true;
			if(anyone_removed)
			{
				CheckCredit(false, true);
				if(leader_id == -1)
				{
					leader_id = 0;
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
		info_box->Show(txLoadingLevel);
		if(!mp_load)
			EnterLocation();
		else
		{
			if(!L.is_open)
			{
				// saved on worldmap
				byte b = ID_START;
				N.peer->Send((cstring)&b, 1, IMMEDIATE_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
				N.StreamWrite(&b, 1, Stream_TransferServer, UNASSIGNED_SYSTEM_ADDRESS);

				DeleteOldPlayers();

				clear_color = clear_color2;
				game_state = GS_WORLDMAP;
				load_screen->visible = false;
				world_map->visible = true;
				game_gui->visible = false;
				main_menu->visible = false;
				mp_load = false;
				clear_color = Color::White;
				W.SetState(World::State::ON_MAP);
				update_timer = 0.f;
				SetMusic(MusicType::Travel);
				ProcessLeftPlayers();
				info_box->CloseDialog();
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
					if(info->id == my_id)
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
							WarpNearLocation(L.local_ctx, *info.u, pos, 4.f, false, 20);
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
						InsideLocation* inside = (InsideLocation*)L.location;
						InsideLocationLevel& lvl = inside->GetLevelData();
						if(L.enter_from == ENTER_FROM_DOWN_LEVEL)
						{
							pos = pt_to_pos(lvl.GetDownStairsFrontTile());
							rot = dir_to_rot(lvl.staircase_down_dir);
						}
						else
						{
							pos = pt_to_pos(lvl.GetUpStairsFrontTile());
							rot = dir_to_rot(lvl.staircase_up_dir);
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
							WarpNearLocation(L.local_ctx, *info.u, pos, L.location->outside ? 4.f : 2.f, false, 20);
							info.u->rot = rot;
							info.u->interp->Reset(info.u->pos, info.u->rot);
						}
					}
				}

				DeleteOldPlayers();
				LoadResources("", false);

				net_mode = NM_SERVER_SEND;
				net_state = NetState::Server_Send;
				if(N.active_players > 1)
				{
					net_stream.Reset();
					PrepareLevelData(net_stream, false);
					Info("NM_TRANSFER_SERVER: Generated level packet: %d.", net_stream.GetNumberOfBytesUsed());
					info_box->Show(txWaitingForPlayers);
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
				players_left = true;
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
					// wyœlij dane poziomu
					N.peer->Send(&net_stream, HIGH_PRIORITY, RELIABLE, 0, packet->systemAddress, false);
					N.StreamWrite(net_stream, Stream_TransferServer, packet->systemAddress);
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
			Info("NM_SERVER_SEND: Ignoring packet ID_CONTROL from %s.", info.name.c_str());
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
				players_left = true;
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
		for(auto info : N.players)
			info->update_timer = 0.f;
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
		info_box->CloseDialog();
		load_screen->visible = false;
		main_menu->visible = false;
		game_gui->visible = true;
		world_map->visible = false;
		mp_load = false;
		SetMusic();
		SetGamePanels();
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
					ClosePeer();
					info_box->CloseDialog();
					if(server_panel->visible)
						server_panel->CloseDialog();
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
		ClosePeer();
		info_box->CloseDialog();
		if(server_panel->visible)
			server_panel->CloseDialog();
		if(net_callback)
			net_callback();
	}
}

void Game::OnEnterPassword(int id)
{
	if(id == BUTTON_CANCEL)
	{
		// nie podano has³a
		EndConnecting(nullptr);
	}
	else
	{
		// podano has³o do serwera
		Info("Password entered.");
		// po³¹cz
		ConnectionAttemptResult result = N.peer->Connect(server.ToString(false), (word)N.port, enter_pswd.c_str(), enter_pswd.length());
		if(result == CONNECTION_ATTEMPT_STARTED)
		{
			net_state = NetState::Client_Connecting;;
			net_timer = T_CONNECT;
			info_box->Show(Format(txConnectingTo, server_name2.c_str()));
			Info("NM_CONNECT_IP(1): Connecting to server %s...", server_name2.c_str());
		}
		else
		{
			Info("NM_CONNECT_IP(1): Can't connect to server: raknet error %d.", result);
			EndConnecting(txConnectRaknet);
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
		Info("sv_online = true");
		Net::SetMode(Net::Mode::Client);
		info_box->Show(txConnecting);
		net_mode = NM_CONNECT_IP;
		net_state = NetState::Client_PingIp;
		net_timer = T_CONNECT_PING;
		net_tries = I_CONNECT_TRIES;
#ifdef _DEBUG
		net_tries *= 2;
#endif
		net_adr = adr.ToString(false);
		N.peer->Ping(net_adr.c_str(), (word)N.port, false);
	}
	else
		Warn("Can't quick connect to server, invalid ip.");
}

void Game::AddMultiMsg(cstring _msg)
{
	assert(_msg);

	game_gui->mp_box->itb.Add(_msg);
}

void Game::Quit()
{
	Info("Game: Quit.");

	if(main_menu->check_version_thread)
	{
		TerminateThread(main_menu->check_version_thread, 0);
		CloseHandle(main_menu->check_version_thread);
		main_menu->check_version_thread = nullptr;
	}

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
	if(info_box->visible)
	{
		switch(net_mode)
		{
		case NM_QUITTING:
		case NM_QUITTING_SERVER:
			if(!net_callback)
				net_callback = f;
			break;
		case NM_CONNECT_IP:
		case NM_TRANSFER:
			info_box->Show(txDisconnecting);
			ForceRedraw();
			ClosePeer(true);
			f();
			break;
		case NM_TRANSFER_SERVER:
		case NM_SERVER_SEND:
			Info("ServerPanel: Closing server.");

			// zablokuj do³¹czanie
			N.peer->SetMaximumIncomingConnections(0);
			// wy³¹cz info o serwerze
			N.peer->SetOfflinePingResponse(nullptr, 0);

			if(N.active_players > 1)
			{
				// roz³¹cz graczy
				Info("ServerPanel: Disconnecting clients.");
				const byte b[] = { ID_SERVER_CLOSE, 0 };
				N.peer->Send((cstring)b, 2, IMMEDIATE_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
				N.StreamWrite(b, 2, Stream_TransferServer, UNASSIGNED_SYSTEM_ADDRESS);
				net_mode = Game::NM_QUITTING_SERVER;
				--N.active_players;
				net_timer = T_WAIT_FOR_DISCONNECT;
				info_box->Show(server_panel->txDisconnecting);
				net_callback = f;
			}
			else
			{
				// nie ma graczy, mo¿na zamkn¹æ
				info_box->CloseDialog();
				ClosePeer();
				f();
			}
			break;
		}
	}
	else if(server_panel->visible)
		server_panel->ExitLobby(f);
	else
	{
		if(Net::IsServer())
		{
			Info("ServerPanel: Closing server.");

			// zablokuj do³¹czanie
			N.peer->SetMaximumIncomingConnections(0);
			// wy³¹cz info o serwerze
			N.peer->SetOfflinePingResponse(nullptr, 0);

			if(N.active_players > 1)
			{
				// roz³¹cz graczy
				Info("ServerPanel: Disconnecting clients.");
				const byte b[] = { ID_SERVER_CLOSE, 0 };
				N.peer->Send((cstring)b, 2, IMMEDIATE_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
				N.StreamWrite(b, 2, Stream_TransferServer, UNASSIGNED_SYSTEM_ADDRESS);
				net_mode = Game::NM_QUITTING_SERVER;
				--N.active_players;
				net_timer = T_WAIT_FOR_DISCONNECT;
				info_box->Show(server_panel->txDisconnecting);
				net_callback = f;
			}
			else
			{
				// nie ma graczy, mo¿na zamkn¹æ
				ClosePeer();
				if(f)
					f();
			}
		}
		else
		{
			info_box->Show(txDisconnecting);
			ForceRedraw();
			ClosePeer(true);
			f();
		}
	}
}

bool Game::ValidateNick(cstring nick)
{
	assert(nick);

	int len = strlen(nick);
	if(len == 0)
		return false;

	for(int i = 0; i < len; ++i)
	{
		if(!(isalnum(nick[i]) || nick[i] == '_'))
			return false;
	}

	if(strcmp(nick, "all") == 0)
		return false;

	return true;
}

void Game::UpdateLobbyNet(float dt)
{
	if(autoready)
	{
		PlayerInfo& info = N.GetMe();
		if(!info.ready)
		{
			info.ready = true;
			ChangeReady();
		}
		autoready = false;
	}

	if(Net::IsServer())
		UpdateLobbyNetServer(dt);
	else
		UpdateLobbyNetClient(dt);
}

void Game::UpdateLobbyNetClient(float dt)
{
	// obs³uga komunikatów otrzymywanych przez klienta
	for(Packet* packet = N.peer->Receive(); packet; N.peer->DeallocatePacket(packet), packet = N.peer->Receive())
	{
		BitStream& stream = N.StreamStart(packet, Stream_UpdateLobbyClient);
		BitStreamReader reader(stream);
		byte msg_id;
		reader >> msg_id;

		switch(msg_id)
		{
		case ID_SAY:
			Client_Say(reader);
			break;
		case ID_WHISPER:
			Client_Whisper(reader);
			break;
		case ID_SERVER_SAY:
			Client_ServerSay(reader);
			break;
		case ID_LOBBY_UPDATE:
			if(!DoLobbyUpdate(reader))
				N.StreamError();
			break;
		case ID_DISCONNECTION_NOTIFICATION:
		case ID_CONNECTION_LOST:
		case ID_SERVER_CLOSE:
			{
				cstring reason, reason_eng;
				if(msg_id == ID_DISCONNECTION_NOTIFICATION)
				{
					reason = txDisconnected;
					reason_eng = "disconnected";
				}
				else if(msg_id == ID_CONNECTION_LOST)
				{
					reason = txLostConnection;
					reason_eng = "lost connection";
				}
				else if(packet->length == 2)
				{
					switch(packet->data[1])
					{
					case 0:
						reason = txClosing;
						reason_eng = "closing";
						break;
					case 1:
						reason = txKicked;
						reason_eng = "kicked";
						break;
					default:
						reason = Format(txUnknown, packet->data[1]);
						reason_eng = Format("unknown (%d)", packet->data[1]);
						break;
					}
				}
				else
				{
					reason = txUnknown2;
					reason_eng = "unknown";
				}

				Info("UpdateLobbyNet: Disconnected from server: %s.", reason_eng);
				GUI.SimpleDialog(Format(txUnconnected, reason), nullptr);

				server_panel->CloseDialog();
				N.StreamEnd();
				N.peer->DeallocatePacket(packet);
				ClosePeer(true);
				return;
			}
		case ID_TIMER:
			if(packet->length != 2)
				N.StreamError("UpdateLobbyNet: Broken packet ID_TIMER.");
			else
			{
				Info("UpdateLobbyNet: Starting in %d...", packet->data[1]);
				server_panel->AddMsg(Format(server_panel->txStartingIn, packet->data[1]));
			}
			break;
		case ID_END_TIMER:
			Info("UpdateLobbyNet: Startup canceled.");
			AddMsg(server_panel->txStartingStop);
			break;
		case ID_PICK_CHARACTER:
			if(packet->length != 2)
				N.StreamError("UpdateLobbyNet: Broken packet ID_PICK_CHARACTER.");
			else
			{
				bool ok = (packet->data[1] != 0);
				if(ok)
					Info("UpdateLobbyNet: Character pick accepted.");
				else
				{
					Warn("UpdateLobbyNet: Character pick refused.");
					PlayerInfo& info = N.GetMe();
					info.ready = false;
					info.clas = Class::INVALID;
					server_panel->bts[0].state = Button::NONE;
					server_panel->bts[0].text = server_panel->txPickChar;
					server_panel->bts[1].state = Button::DISABLED;
				}
			}
			break;
		case ID_STARTUP:
			if(packet->length != 2)
				N.StreamError("UpdateLobbyNet: Broken packet ID_STARTUP.");
			else
			{
				Info("UpdateLobbyNet: Starting in 0...");
				server_panel->AddMsg(Format(server_panel->txStartingIn, 0));

				// close lobby and wait for server
				mp_load_worldmap = (packet->data[1] == 1);
				Info("UpdateLobbyNet: Waiting for server.");
				LoadingStart(mp_load_worldmap ? 4 : 9);
				main_menu->visible = false;
				server_panel->CloseDialog();
				info_box->Show(txWaitingForServer);
				net_mode = NM_TRANSFER;
				net_state = NetState::Client_BeforeTransfer;
				N.StreamEnd();
				N.peer->DeallocatePacket(packet);
				return;
			}
			break;
		default:
			Warn("UpdateLobbyNet: Unknown packet: %u.", msg_id);
			N.StreamError();
			break;
		}

		N.StreamEnd();
	}
}

void Game::UpdateLobbyNetServer(float dt)
{
	if(!N.starting && autostart_count != 0u && autostart_count <= N.active_players)
	{
		bool ok = true;
		for(auto info : N.players)
		{
			if(!info->ready)
			{
				ok = false;
				break;
			}
		}

		if(ok)
		{
			// automatyczne uruchamianie gry, rozpocznij odliczanie
			Info("UpdateLobbyNet: Automatic server startup.");
			autostart_count = 0u;
			N.starting = true;
			startup_timer = float(STARTUP_TIMER);
			last_startup_id = STARTUP_TIMER;
			byte b[] = { ID_TIMER, STARTUP_TIMER };
			N.peer->Send((cstring)b, 2, IMMEDIATE_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
			N.StreamWrite(b, 2, Stream_UpdateLobbyServer, UNASSIGNED_SYSTEM_ADDRESS);
			server_panel->bts[5].text = server_panel->txStop;
			server_panel->AddMsg(Format(server_panel->txStarting, STARTUP_TIMER));
			Info("UpdateLobbyNet: Starting in %d...", STARTUP_TIMER);
		}
	}

	for(Packet* packet = N.peer->Receive(); packet; N.peer->DeallocatePacket(packet), packet = N.peer->Receive())
	{
		BitStream& stream = N.StreamStart(packet, Stream_UpdateLobbyServer);
		BitStreamReader reader(stream);
		byte msg_id;
		reader >> msg_id;

		// pobierz informacje o graczu
		PlayerInfo* info = N.FindPlayer(packet->systemAddress);
		if(info && info->state == PlayerInfo::REMOVING)
		{
			if(msg_id == ID_DISCONNECTION_NOTIFICATION || msg_id == ID_CONNECTION_LOST)
			{
				// nie odes³a³ informacji tylko siê roz³¹czy³
				Info(!info->name.empty() ? Format("UpdateLobbyNet: Player %s has disconnected.", info->name.c_str()) :
					Format("UpdateLobbyNet: %s has disconnected.", packet->systemAddress.ToString()));
				--N.active_players;
				if(N.active_players > 1)
					AddLobbyUpdate(Int2(Lobby_ChangeCount, 0));
				N.UpdateServerInfo();
				int index = info->GetIndex();
				server_panel->grid.RemoveItem(index);
				auto it = N.players.begin() + index;
				delete *it;
				N.players.erase(it);
			}
			else
			{
				// nieznany komunikat od roz³¹czanego gracz, ignorujemy go
				Info("UpdateLobbyNet: Ignoring packet from %s while disconnecting.",
					!info->name.empty() ? info->name.c_str() : packet->systemAddress.ToString());
			}

			N.StreamEnd();
			continue;
		}

		switch(msg_id)
		{
		case ID_UNCONNECTED_PING:
		case ID_UNCONNECTED_PING_OPEN_CONNECTIONS:
			assert(!info);
			Info("UpdateLobbyNet: Ping from %s.", packet->systemAddress.ToString());
			break;
		case ID_NEW_INCOMING_CONNECTION:
			if(info)
				Warn("UpdateLobbyNet: Packet about connecting from already connected player %s.", info->name.c_str());
			else
			{
				Info("UpdateLobbyNet: New connection from %s.", packet->systemAddress.ToString());

				// ustal id dla nowego gracza
				do
				{
					last_id = (last_id + 1) % 256;
					bool ok = true;
					for(auto info : N.players)
					{
						if(info->id == last_id)
						{
							ok = false;
							break;
						}
					}
					if(ok)
						break;
				} while(1);

				// dodaj
				auto pinfo = new PlayerInfo;
				N.players.push_back(pinfo);

				auto& info = *pinfo;
				info.adr = packet->systemAddress;
				info.id = last_id;
				server_panel->grid.AddItem();

				if(N.active_players == N.max_players)
				{
					// brak miejsca na serwerze, wyœlij komunikat i czekaj a¿ siê roz³¹czy
					byte b[] = { ID_CANT_JOIN, 0 };
					N.peer->Send((cstring)b, 2, MEDIUM_PRIORITY, RELIABLE, 0, packet->systemAddress, false);
					N.StreamWrite(b, 2, Stream_UpdateLobbyServer, packet->systemAddress);
					info.state = PlayerInfo::REMOVING;
					info.timer = T_WAIT_FOR_DISCONNECT;
					info.left = PlayerInfo::LEFT_SERVER_FULL;
				}
				else
				{
					// czekaj a¿ wyœle komunikat o wersji, nick
					info.state = PlayerInfo::WAITING_FOR_HELLO;
					info.timer = T_WAIT_FOR_HELLO;
					info.devmode = default_player_devmode;
					info.buffs = 0;
					if(N.active_players > 1)
						AddLobbyUpdate(Int2(Lobby_ChangeCount, 0));
					++N.active_players;
					N.UpdateServerInfo();
				}
			}
			break;
		case ID_DISCONNECTION_NOTIFICATION:
		case ID_CONNECTION_LOST:
			// klient siê roz³¹czy³
			{
				bool dis = (msg_id == ID_CONNECTION_LOST);
				if(!info)
				{
					Warn(dis ? "UpdateLobbyNet: Disconnect notification from unconnected address %s." :
						"UpdateLobbyNet: Lost connection with unconnected address %s.", packet->systemAddress.ToString());
				}
				else
				{
					int index = info->GetIndex();
					server_panel->grid.RemoveItem(index);
					if(info->state == PlayerInfo::WAITING_FOR_HELLO)
					{
						// roz³¹czy³ siê przed przyjêciem do lobby, mo¿na go usun¹æ
						server_panel->AddMsg(Format(dis ? txDisconnected : txLost, packet->systemAddress.ToString()));
						Info("UpdateLobbyNet: %s %s.", packet->systemAddress.ToString(), dis ? "disconnected" : "lost connection");
						auto it = N.players.begin() + index;
						delete *it;
						N.players.erase(it);
						--N.active_players;
						if(N.active_players > 1)
							AddLobbyUpdate(Int2(Lobby_ChangeCount, 0));
						N.UpdateServerInfo();
					}
					else
					{
						// roz³¹czy³ siê bêd¹c w lobby
						--N.active_players;
						server_panel->AddMsg(Format(dis ? txLeft : txLost2, info->name.c_str()));
						Info("UpdateLobbyNet: Player %s %s.", info->name.c_str(), dis ? "disconnected" : "lost connection");
						if(N.active_players > 1)
						{
							AddLobbyUpdate(Int2(Lobby_RemovePlayer, info->id));
							AddLobbyUpdate(Int2(Lobby_ChangeCount, 0));
						}
						if(leader_id == info->id)
						{
							// serwer zostaje przywódc¹
							Info("UpdateLobbyNet: You are leader now.");
							leader_id = my_id;
							if(N.active_players > 1)
								AddLobbyUpdate(Int2(Lobby_ChangeLeader, 0));
							server_panel->AddMsg(server_panel->txYouAreLeader);
						}
						auto it = N.players.begin() + index;
						delete *it;
						N.players.erase(it);
						N.UpdateServerInfo();
						CheckReady();
					}
				}
			}
			break;
		case ID_HELLO:
			// informacje od gracza
			if(!info)
				Warn("UpdateLobbyNet: Packet ID_HELLO from unconnected client %s.", packet->systemAddress.ToString());
			else if(info->state != PlayerInfo::WAITING_FOR_HELLO)
				Warn("UpdateLobbyNet: Packet ID_HELLO from player %s who already joined.", info->name.c_str());
			else
			{
				int version;
				cstring reason_text = nullptr;
				int include_extra = 0;
				uint my_crc, player_crc;
				content::Id type;
				cstring type_str;
				JoinResult reason = JoinResult::Ok;

				reader >> version;
				content::ReadCrc(reader);
				reader >> info->name;
				if(!reader)
				{
					// failed to read packet
					reason = JoinResult::BrokenPacket;
					reason_text = Format("UpdateLobbyNet: Broken packet ID_HELLO from %s.", packet->systemAddress.ToString());
				}
				else if(version != VERSION)
				{
					// version mismatch
					reason = JoinResult::InvalidVersion;
					reason_text = Format("UpdateLobbbyNet: Invalid version from %s. Our (%s) vs (%s).", packet->systemAddress.ToString(),
						VersionToString(version), VERSION_STR);
				}
				else if(!content::ValidateCrc(type, my_crc, player_crc, type_str))
				{
					// invalid game type manager crc
					reason = JoinResult::InvalidCrc;
					reason_text = Format("UpdateLobbyNet: Invalid %s crc from %s. Our (%p) vs (%p).", type_str, packet->systemAddress.ToString(), my_crc,
						player_crc);
					include_extra = 2;
				}
				else if(!ValidateNick(info->name.c_str()))
				{
					// invalid nick
					reason = JoinResult::InvalidNick;
					reason_text = Format("UpdateLobbyNet: Invalid nick (%s) from %s.", info->name.c_str(), packet->systemAddress.ToString());
				}
				else
				{
					// check if nick is unique
					bool ok = true;
					for(auto info2 : N.players)
					{
						if(info2->id != info->id && info2->state == PlayerInfo::IN_LOBBY && info2->name == info->name)
						{
							ok = false;
							break;
						}
					}
					if(!ok)
					{
						// nick is already used
						reason = JoinResult::TakenNick;
						reason_text = Format("UpdateLobbyNet: Nick already in use (%s) from %s.", info->name.c_str(), packet->systemAddress.ToString());
					}
				}

				net_stream.Reset();
				BitStreamWriter fw(net_stream);
				if(reason != JoinResult::Ok)
				{
					Warn(reason_text);
					N.StreamError();
					fw << ID_CANT_JOIN;
					fw.WriteCasted<byte>(reason);
					if(include_extra != 0)
						fw << my_crc;
					if(include_extra == 2)
						fw.WriteCasted<byte>(type);
					N.peer->Send(&net_stream, MEDIUM_PRIORITY, RELIABLE, 0, packet->systemAddress, false);
					N.StreamWrite(net_stream, Stream_UpdateLobbyServer, packet->systemAddress);
					info->state = PlayerInfo::REMOVING;
					info->timer = T_WAIT_FOR_DISCONNECT;
				}
				else
				{
					// everything is ok, let player join
					if(N.active_players > 2)
						AddLobbyUpdate(Int2(Lobby_AddPlayer, info->id));
					fw << ID_JOIN;
					fw.WriteCasted<byte>(info->id);
					fw.WriteCasted<byte>(N.active_players);
					fw.WriteCasted<byte>(leader_id);
					fw.Write0();
					int count = 0;
					for(auto info2 : N.players)
					{
						if(info2->id == info->id || info2->state != PlayerInfo::IN_LOBBY)
							continue;
						++count;
						fw.WriteCasted<byte>(info2->id);
						fw.WriteCasted<byte>(info2->ready ? 1 : 0);
						fw.WriteCasted<byte>(info2->clas);
						fw << info2->name;
					}
					fw.Patch<byte>(4, count);
					if(mp_load)
					{
						// informacja o postaci w zapisie
						PlayerInfo* old = FindOldPlayer(info->name.c_str());
						if(old)
						{
							fw.WriteCasted<byte>(2);
							fw.WriteCasted<byte>(old->clas);

							info->clas = old->clas;
							info->loaded = true;
							info->devmode = old->devmode;
							info->hd.CopyFrom(old->hd);
							info->notes = old->notes;

							if(N.active_players > 2)
								AddLobbyUpdate(Int2(Lobby_UpdatePlayer, info->id));

							Info("UpdateLobbyNet: Player %s is using loaded character.", info->name.c_str());
						}
						else
							fw.Write1();
					}
					else
						fw.Write0();
					N.peer->Send(&net_stream, HIGH_PRIORITY, RELIABLE, 0, packet->systemAddress, false);
					N.StreamWrite(net_stream, Stream_UpdateLobbyServer, packet->systemAddress);
					info->state = PlayerInfo::IN_LOBBY;

					server_panel->AddMsg(Format(server_panel->txJoined, info->name.c_str()));
					Info("UpdateLobbyNet: Player %s (%s) joined, id: %d.", info->name.c_str(), packet->systemAddress.ToString(), info->id);

					CheckReady();
				}
			}
			break;
		case ID_CHANGE_READY:
			if(!info)
				Warn("UpdateLobbyNet: Packet ID_CHANGE_READY from unconnected client %s.", packet->systemAddress.ToString());
			else
			{
				bool ready;
				reader >> ready;
				if(!reader)
					N.StreamError("UpdateLobbyNet: Broken packet ID_CHANGE_READY from client %s.", info->name.c_str());
				else if(ready != info->ready)
				{
					info->ready = ready;
					if(N.active_players > 2)
						AddLobbyUpdate(Int2(Lobby_UpdatePlayer, info->id));
					CheckReady();
				}
			}
			break;
		case ID_SAY:
			if(!info)
				Warn("UpdateLobbyNet: Packet ID_SAY from unconnected client %s.", packet->systemAddress.ToString());
			else
				Server_Say(stream, *info, packet);
			break;
		case ID_WHISPER:
			if(!info)
				Warn("UpdateLobbyNet: Packet ID_WHISPER from unconnected client %s.", packet->systemAddress.ToString());
			else
				Server_Whisper(reader, *info, packet);
			break;
		case ID_LEAVE:
			if(!info)
				Warn("UpdateLobbyNet: Packet ID_LEAVE from unconnected client %.", packet->systemAddress.ToString());
			else
			{
				// czekano na dane a on zquitowa³ mimo braku takiej mo¿liwoœci :S
				cstring name = info->state == PlayerInfo::WAITING_FOR_HELLO ? info->adr.ToString() : info->name.c_str();
				server_panel->AddMsg(Format(server_panel->txPlayerLeft, name));
				Info("UpdateLobbyNet: Player %s left lobby.", name);
				--N.active_players;
				if(N.active_players > 1)
				{
					AddLobbyUpdate(Int2(Lobby_ChangeCount, 0));
					if(info->state == PlayerInfo::IN_LOBBY)
						AddLobbyUpdate(Int2(Lobby_RemovePlayer, info->id));
				}
				int index = info->GetIndex();
				server_panel->grid.RemoveItem(index);
				N.UpdateServerInfo();
				N.peer->CloseConnection(packet->systemAddress, true);
				auto it = N.players.begin() + index;
				delete *it;
				N.players.erase(it);
				CheckReady();
			}
			break;
		case ID_PICK_CHARACTER:
			if(!info || info->state != PlayerInfo::IN_LOBBY)
				Warn("UpdateLobbyNet: Packet ID_PICK_CHARACTER from player not in lobby %s.", packet->systemAddress.ToString());
			else
			{
				Class old_class = info->clas;
				bool old_ready = info->ready;
				int result = ReadCharacterData(reader, info->clas, info->hd, info->cc);
				byte ok = 0;
				if(result == 0)
				{
					reader >> info->ready;
					if(reader)
					{
						ok = 1;
						Info("Received character from '%s'.", info->name.c_str());
					}
					else
						N.StreamError("UpdateLobbyNet: Broken packet ID_PICK_CHARACTER from '%s'.", info->name.c_str());
				}
				else
				{
					cstring err[3] = {
						"read error",
						"value error",
						"validation error"
					};

					N.StreamError("UpdateLobbyNet: Packet ID_PICK_CHARACTER from '%s' %s.", info->name.c_str(), err[result - 1]);
				}

				if(ok == 0)
				{
					info->ready = false;
					info->clas = Class::INVALID;
				}
				CheckReady();

				// send info to other N.active_players
				if((old_ready != info->ready || old_class != info->clas) && N.active_players > 2)
					AddLobbyUpdate(Int2(Lobby_UpdatePlayer, info->id));

				// send result
				byte packet[2] = { ID_PICK_CHARACTER, ok };
				N.peer->Send((cstring)packet, 2, HIGH_PRIORITY, RELIABLE, 0, info->adr, false);
				N.StreamWrite(packet, 2, Stream_UpdateLobbyServer, info->adr);
			}
			break;
		default:
			Warn("UpdateLobbyNet: Unknown packet from %s: %u.", info ? info->name.c_str() : packet->systemAddress.ToString(), msg_id);
			N.StreamError();
			break;
		}

		N.StreamEnd();
	}

	int index = 0;
	for(vector<PlayerInfo*>::iterator it = N.players.begin(), end = N.players.end(); it != end;)
	{
		auto& info = **it;
		if(info.state != PlayerInfo::IN_LOBBY)
		{
			info.timer -= dt;
			if(info.timer <= 0.f)
			{
				// czas oczekiwania min¹³, zkickuj
				Info("UpdateLobbyNet: Removed %s due to inactivity.", info.adr.ToString());
				N.peer->CloseConnection(info.adr, false);
				--N.active_players;
				if(N.active_players > 1)
					AddLobbyUpdate(Int2(Lobby_RemovePlayer, 0));
				delete *it;
				it = N.players.erase(it);
				end = N.players.end();
				server_panel->grid.RemoveItem(index);
			}
			else
			{
				++index;
				++it;
			}
		}
		else
		{
			++it;
			++index;
		}
	}

	// wysy³anie aktualizacji lobby
	update_timer += dt;
	if(update_timer >= T_LOBBY_UPDATE)
	{
		update_timer = 0.f;
		if(!lobby_updates.empty())
		{
			assert(lobby_updates.size() < 255);
			if(N.active_players > 1)
			{
				// aktualizacje w lobby
				net_stream.Reset();
				BitStreamWriter f(net_stream);
				f << ID_LOBBY_UPDATE;
				f.Write0();
				int count = 0;
				for(uint i = 0; i < min(lobby_updates.size(), 255u); ++i)
				{
					Int2& u = lobby_updates[i];
					switch(u.x)
					{
					case Lobby_UpdatePlayer:
						{
							int index = GetPlayerIndex(u.y);
							if(index != -1)
							{
								PlayerInfo& info = *N.players[index];
								++count;
								f.WriteCasted<byte>(u.x);
								f.WriteCasted<byte>(info.id);
								f << info.ready;
								f.WriteCasted<byte>(info.clas);
							}
						}
						break;
					case Lobby_AddPlayer:
						{
							int index = GetPlayerIndex(u.y);
							if(index != -1)
							{
								PlayerInfo& info = *N.players[index];
								++count;
								f.WriteCasted<byte>(u.x);
								f.WriteCasted<byte>(info.id);
								f << info.name;
							}
						}
						break;
					case Lobby_RemovePlayer:
					case Lobby_KickPlayer:
						++count;
						f.WriteCasted<byte>(u.x);
						f.WriteCasted<byte>(u.y);
						break;
					case Lobby_ChangeCount:
						++count;
						f.WriteCasted<byte>(u.x);
						f.WriteCasted<byte>(N.active_players);
						break;
					case Lobby_ChangeLeader:
						++count;
						f.WriteCasted<byte>(u.x);
						f.WriteCasted<byte>(leader_id);
						break;
					default:
						assert(0);
						break;
					}
				}
				f.Patch<byte>(1, count);
				N.peer->Send(&net_stream, HIGH_PRIORITY, RELIABLE_ORDERED, 2, UNASSIGNED_SYSTEM_ADDRESS, true);
				N.StreamWrite(net_stream, Stream_UpdateLobbyServer, UNASSIGNED_SYSTEM_ADDRESS);
			}
			lobby_updates.clear();
		}
	}

	// starting game
	if(N.starting)
	{
		startup_timer -= dt;
		int co = int(startup_timer) + 1;
		int d = -1;
		if(startup_timer <= 0.f)
		{
			// change server status
			Info("UpdateLobbyNet: Starting game.");
			N.starting = false;
			d = 0;
			N.peer->SetMaximumIncomingConnections(0);
			net_mode = NM_TRANSFER_SERVER;
			net_timer = mp_timeout;
			net_state = NetState::Server_Starting;
			// kick N.active_players that connected but didn't join
			for(vector<PlayerInfo*>::iterator it = N.players.begin(), end = N.players.end(); it != end;)
			{
				auto& info = **it;
				if(info.state != PlayerInfo::IN_LOBBY)
				{
					N.peer->CloseConnection(info.adr, true);
					Warn("UpdateLobbyNet: Disconnecting %s.", info.adr.ToString());
					delete *it;
					it = N.players.erase(it);
					end = N.players.end();
					--N.active_players;
				}
				else
					++it;
			}
			server_panel->CloseDialog();
			info_box->Show(txStartingGame);
		}
		else if(co != last_startup_id)
		{
			last_startup_id = co;
			d = co;
		}
		if(d != -1)
		{
			Info("UpdateLobbyNet: Starting in %d...", d);
			server_panel->AddMsg(Format(server_panel->txStartingIn, d));
			byte b[2];
			if(d == 0)
			{
				b[0] = ID_STARTUP;
				b[1] = (mp_load && !L.is_open ? 1 : 0);
			}
			else
			{
				b[0] = ID_TIMER;
				b[1] = (byte)d;
			}
			N.peer->Send((cstring)b, 2, IMMEDIATE_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
			N.StreamWrite(b, 2, Stream_UpdateLobbyServer, UNASSIGNED_SYSTEM_ADDRESS);
		}
	}
}

bool Game::DoLobbyUpdate(BitStreamReader& f)
{
	byte count;
	f >> count;
	if(!f)
	{
		Error("UpdateLobbyNet: Broken packet ID_LOBBY_UPDATE.");
		return false;
	}

	for(int i = 0; i < count; ++i)
	{
		byte type, id;
		f >> type;
		f >> id;
		if(!f)
		{
			Error("UpdateLobbyNet: Broken packet ID_LOBBY_UPDATE at index %d.", i);
			return false;
		}

		switch(type)
		{
		case Lobby_UpdatePlayer: // aktualizuj gracza
			{
				int index = GetPlayerIndex(id);
				if(index == -1)
				{
					Error("UpdateLobbyNet: Broken Lobby_UpdatePlayer, invalid player id %d.", id);
					return false;
				}
				PlayerInfo& info = *N.players[index];
				f >> info.ready;
				f.ReadCasted<byte>(info.clas);
				if(!f)
				{
					Error("UpdateLobbyNet: Broken Lobby_UpdatePlayer.");
					return false;
				}
				if(!ClassInfo::IsPickable(info.clas))
				{
					Error("UpdateLobbyNet: Broken Lobby_UpdatePlayer, player %d have class %d: %s.", id, info.clas);
					return false;
				}
			}
			break;
		case Lobby_AddPlayer: // dodaj gracza
			{
				const string& name = f.ReadString1();
				if(!f)
				{
					Error("UpdateLobbyNet: Broken Lobby_AddPlayer.");
					return false;
				}
				else if(id != my_id)
				{
					auto pinfo = new PlayerInfo;
					N.players.push_back(pinfo);

					auto& info = *pinfo;
					info.state = PlayerInfo::IN_LOBBY;
					info.id = id;
					info.loaded = true;
					info.name = name;
					server_panel->grid.AddItem();

					server_panel->AddMsg(Format(server_panel->txJoined, name.c_str()));
					Info("UpdateLobbyNet: Player %s joined lobby.", name.c_str());
				}
			}
			break;
		case Lobby_RemovePlayer: // usuñ gracza
		case Lobby_KickPlayer:
			{
				bool is_kick = (type == Lobby_KickPlayer);
				int index = GetPlayerIndex(id);
				if(index == -1)
				{
					Error("UpdateLobbyNet: Broken Lobby_Remove/KickPlayer, invalid player id %d.", id);
					return false;
				}
				PlayerInfo& info = *N.players[index];
				Info("UpdateLobbyNet: Player %s %s.", info.name.c_str(), is_kick ? "was kicked" : "left lobby");
				server_panel->AddMsg(Format(is_kick ? txPlayerKicked : txPlayerLeft, info.name.c_str()));
				server_panel->grid.RemoveItem(index);
				auto it = N.players.begin() + index;
				delete *it;
				N.players.erase(it);
			}
			break;
		case Lobby_ChangeCount: // zmieñ liczbê graczy
			N.active_players = id;
			break;
		case Lobby_ChangeLeader: // zmiana przywódcy
			{
				int index = GetPlayerIndex(id);
				if(index == -1)
				{
					Error("UpdateLobbyNet: Broken Lobby_ChangeLeader, invalid player id %d", id);
					return false;
				}
				PlayerInfo& info = *N.players[index];
				Info("%s is now leader.", info.name.c_str());
				leader_id = id;
				if(my_id == id)
					server_panel->AddMsg(server_panel->txYouAreLeader);
				else
					server_panel->AddMsg(Format(server_panel->txLeaderChanged, info.name.c_str()));
			}
			break;
		default:
			Error("UpdateLobbyNet: Broken packet ID_LOBBY_UPDATE, unknown change type %d.", type);
			return false;
		}
	}

	return true;
}

void Game::ShowQuitDialog()
{
	DialogInfo di;
	di.text = txReallyQuit;
	di.event = delegate<void(int)>(this, &Game::OnQuit);
	di.type = DIALOG_YESNO;
	di.name = "dialog_alt_f4";
	di.parent = nullptr;
	di.order = ORDER_TOPMOST;
	di.pause = true;

	GUI.ShowDialog(di);
}

void Game::OnCreateCharacter(int id)
{
	if(id != BUTTON_OK)
		return;

	if(Net::IsOnline())
	{
		PlayerInfo& info = N.GetMe();
		server_panel->bts[1].state = Button::NONE;
		server_panel->bts[0].text = server_panel->txChangeChar;
		// set data
		Class old_class = info.clas;
		info.clas = create_character->clas;
		info.hd.Get(*create_character->unit->human_data);
		info.cc = create_character->cc;
		// send info to other N.active_players about changing my class
		if(Net::IsServer())
		{
			if(info.clas != old_class && N.active_players > 1)
				AddLobbyUpdate(Int2(Lobby_UpdatePlayer, 0));
		}
		else
		{
			net_stream.Reset();
			BitStreamWriter f(net_stream);
			f << ID_PICK_CHARACTER;
			WriteCharacterData(f, info.clas, info.hd, info.cc);
			f << false;
			N.peer->Send(&net_stream, IMMEDIATE_PRIORITY, RELIABLE, 0, server, false);
			N.StreamWrite(net_stream, Stream_UpdateLobbyClient, server);
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
	if(id == BUTTON_CANCEL)
	{
		ClosePeer();
		N.peer->Shutdown(0);
		pick_server_panel->CloseDialog();
	}
	else
	{
		PickServerPanel::ServerData& info = pick_server_panel->servers[pick_server_panel->grid.selected];
		server = info.adr;
		server_name2 = info.name;
		max_players2 = info.max_players;
		N.active_players = info.active_players;
		if(IS_SET(info.flags, SERVER_PASSWORD))
		{
			// podaj has³o
			net_mode = NM_CONNECT_IP;
			net_state = NetState::Client_WaitingForPassword;
			net_tries = 1;
			info_box->Show(txWaitingForPswd);
			GetTextDialogParams params(Format(txEnterPswd, server_name2.c_str()), enter_pswd);
			params.event = DialogEvent(this, &Game::OnEnterPassword);
			params.limit = 16;
			params.parent = info_box;
			GetTextDialog::Show(params);
			Info("OnPickServer: Waiting for password...");
			pick_server_panel->CloseDialog();
		}
		else
		{
			// po³¹cz
			ConnectionAttemptResult result = N.peer->Connect(info.adr.ToString(false), (word)N.port, nullptr, 0);
			if(result == CONNECTION_ATTEMPT_STARTED)
			{
				// trwa ³¹czenie z serwerem
				net_mode = NM_CONNECT_IP;
				net_timer = T_CONNECT;
				net_tries = 1;
				net_state = NetState::Client_Connecting;
				info_box->Show(Format(txConnectingTo, info.name.c_str()));
				Info("OnPickServer: Connecting to server %s...", info.name.c_str());
				pick_server_panel->CloseDialog();
			}
			else
			{
				// b³¹d ³¹czenie z serwerem
				Error("NM_CONNECT_IP(0): Can't connect to server: raknet error %d.", result);
				EndConnecting(txConnectRaknet);
			}
		}
	}
}

void Game::DeleteOldPlayers()
{
	const bool in_level = L.is_open;
	for(vector<PlayerInfo*>::iterator it = old_players.begin(), end = old_players.end(); it != end; ++it)
	{
		auto& info = **it;
		if(!info.loaded && info.u)
		{
			if(in_level)
				RemoveElement(L.GetContext(*info.u).units, info.u);
			if(info.u->cobj)
			{
				delete info.u->cobj->getCollisionShape();
				phy_world->removeCollisionObject(info.u->cobj);
				delete info.u->cobj;
			}
			delete info.u;
		}
	}
	DeleteElements(old_players);
}

void Game::ClearAndExitToMenu(cstring msg)
{
	assert(msg);
	Info("Clear game and exit to menu.");
	ClearGame();
	ClosePeer();
	ExitToMenu();
	GUI.SimpleDialog(msg, main_menu);
}

void Game::AddLobbyUpdate(const Int2& u)
{
	for(Int2& update : lobby_updates)
	{
		if(update == u)
			return;
	}
	lobby_updates.push_back(u);
}
