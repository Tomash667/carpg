#include "Pch.h"
#include "GameCore.h"
#include "ServerPanel.h"
#include "Game.h"
#include "Language.h"
#include "InfoBox.h"
#include "BitStreamFunc.h"
#include "ResourceManager.h"
#include "GameGui.h"
#include "CreateCharacterPanel.h"
#include "MainMenu.h"
#include "Content.h"
#include "Version.h"
#include "Level.h"
#include "PlayerInfo.h"
#include "Team.h"
#include "LobbyApi.h"
#include "CommandParser.h"

FIXME;
//-----------------------------------------------------------------------------
#ifdef _DEBUG
const int STARTUP_TIMER = 3;
#else
const int STARTUP_TIMER = 3;
#endif
const float T_WAIT_FOR_HELLO = 5.f;
const float T_LOBBY_UPDATE = 0.2f;

//-----------------------------------------------------------------------------
enum ButtonId
{
	IdPickCharacter = GuiEvent_Custom,
	IdReady,
	IdKick,
	IdLeader,
	IdStart,
	IdCancel
};

//=================================================================================================
ServerPanel::ServerPanel(const DialogInfo& info) : DialogBox(info), autoready(false), autopick_class(nullptr)
{
	size = Int2(540, 510);
	bts.resize(6);

	const Int2 bt_size(180, 44);

	bts[0].id = IdPickCharacter;
	bts[0].parent = this;
	bts[0].pos = Int2(340, 30);
	bts[0].size = bt_size;

	bts[1].id = IdReady;
	bts[1].parent = this;
	bts[1].pos = Int2(340, 80);
	bts[1].size = bt_size;

	bts[2].id = IdKick;
	bts[2].parent = this;
	bts[2].pos = Int2(340, 130);
	bts[2].size = bt_size;

	bts[3].id = IdLeader;
	bts[3].parent = this;
	bts[3].pos = Int2(340, 180);
	bts[3].size = bt_size;

	bts[4].id = IdStart;
	bts[4].parent = this;
	bts[4].pos = Int2(340, 230);
	bts[4].size = bt_size;

	bts[5].id = IdCancel;
	bts[5].parent = this;
	bts[5].pos = Int2(340, 280);
	bts[5].size = bt_size;

	grid.pos = Int2(10, 10);
	grid.size = Int2(320, 300);
	grid.event = GridEvent(this, &ServerPanel::GetCell);
	grid.selection_type = Grid::BACKGROUND;
	grid.selection_color = Color(0, 255, 0, 128);

	itb.parent = this;
	itb.max_cache = 10;
	itb.max_lines = 100;
	itb.esc_clear = false;
	itb.lose_focus = false;
	itb.pos = Int2(10, 320);
	itb.size = Int2(320, 182);
	itb.event = InputTextBox::InputEvent(this, &ServerPanel::OnInput);
	itb.Init();

	visible = false;
}

//=================================================================================================
void ServerPanel::Init()
{
	random_class = reinterpret_cast<Class*>(&random_class);

	autostart_count = game->cfg.GetUint("autostart");
	if(autostart_count > MAX_PLAYERS)
		autostart_count = 0;

	const string& class_id = game->cfg.GetString("autopick", "");
	if(!class_id.empty())
	{
		if(class_id == "random")
			autopick_class = random_class;
		else
		{
			autopick_class = Class::TryGet(class_id);
			if(autopick_class)
			{
				if(!autopick_class->IsPickable())
				{
					Warn("Settings [autopick]: Class '%s' is not pickable by players.", class_id.c_str());
					autopick_class = nullptr;
				}
			}
			else
				Warn("Settings [autopick]: Invalid class '%s'.", class_id.c_str());
		}
	}
}

//=================================================================================================
void ServerPanel::LoadLanguage()
{
	Language::Section s = Language::GetSection("ServerPanel");
	txReady = s.Get("ready");
	txNotReady = s.Get("notReady");
	txStart = s.Get("start");
	txStop = s.Get("stop");
	txPickChar = s.Get("pickChar");
	txKick = s.Get("kick");
	txNone = s.Get("none");
	txSetLeader = s.Get("setLeader");
	txNick = s.Get("nick");
	txChar = s.Get("char");
	txLoadedCharInfo = s.Get("loadedCharInfo");
	txNotLoadedCharInfo = s.Get("notLoadedCharInfo");
	txChangeChar = s.Get("changeChar");
	txCantKickMyself = s.Get("cantKickMyself");
	txCantKickUnconnected = s.Get("cantKickUnconnected");
	txReallyKick = s.Get("reallyKick");
	txAlreadyLeader = s.Get("alreadyLeader");
	txLeaderChanged = s.Get("leaderChanged");
	txNotJoinedYet = s.Get("notJoinedYet");
	txNotAllReady = s.Get("notAllReady");
	txStartingIn = s.Get("startingIn");
	txStartingStop = s.Get("startingStop");
	txDisconnecting = s.Get("disconnecting");
	txYouAreLeader = s.Get("youAreLeader");
	txJoined = s.Get("joined");
	txPlayerLeft = s.Get("playerLeft");
	txNeedSelectedPlayer = s.Get("needSelectedPlayer");
	txServerText = s.Get("serverText");
	txDisconnected = s.Get("disconnected");
	txClosing = s.Get("closing");
	txKicked = s.Get("kicked");
	txUnknown = s.Get("unknown");
	txUnconnected = s.Get("unconnected");
	txIpLostConnection = s.Get("ipLostConnection");
	txPlayerLostConnection = s.Get("playerLostConnection");
	txLeft = s.Get("left");
	txStartingGame = s.Get("startingGame");
	txWaitingForServer = s.Get("waitingForServer");
	txRegisterFailed = s.Get("registerFailed");
	txPlayerDisconnected = s.Get("playerDisconnected");

	bts[0].text = txPickChar; // change char
	bts[1].text = txReady; // not ready
	bts[2].text = txKick; // cancel
	bts[3].text = txSetLeader;
	bts[4].text = txStart;
	bts[5].text = gui->txCancel;

	grid.AddColumn(Grid::IMG, 20);
	grid.AddColumn(Grid::TEXT_COLOR, 140, txNick);
	grid.AddColumn(Grid::TEXT, 140, txChar);
	grid.Init();
}

//=================================================================================================
void ServerPanel::LoadData()
{
	tReady = res_mgr->Load<Texture>("gotowy.png");
	tNotReady = res_mgr->Load<Texture>("niegotowy.png");
}

//=================================================================================================
void ServerPanel::Draw(ControlDrawData*)
{
	DrawPanel();

	// controls
	int count = (Net::IsServer() ? 6 : 3);
	for(int i = 0; i < count; ++i)
		bts[i].Draw();
	itb.Draw();
	grid.Draw();

	// text
	Rect r = { 340 + global_pos.x, 355 + global_pos.y, 340 + 185 + global_pos.x, 355 + 160 + global_pos.y };
	gui->DrawText(GameGui::font, Format(txServerText, server_name.c_str(), net->active_players, max_players, net->password.empty() ? gui->txNo : gui->txYes),
		0, Color::Black, r, &r);
}

//=================================================================================================
void ServerPanel::Update(float dt)
{
	// update controls
	int count = (Net::IsServer() ? 6 : 3);
	for(int i = 0; i < count; ++i)
	{
		bts[i].mouse_focus = focus;
		bts[i].Update(dt);
	}
	itb.mouse_focus = focus;
	itb.Update(dt);
	grid.focus = focus;
	grid.Update(dt);

	if(focus && input->Focus() && input->PressedRelease(Key::Escape))
		Event(GuiEvent(Net::IsServer() ? IdCancel : IdKick));

	if(visible)
		UpdateLobby(dt);
}

//=================================================================================================
void ServerPanel::UpdateLobby(float dt)
{
	if(autoready)
	{
		PlayerInfo& info = net->GetMe();
		if(!info.ready && info.clas)
		{
			info.ready = true;
			bts[1].state = Button::NONE;
			ChangeReady();
		}
		autoready = false;
	}

	if(Net::IsServer())
		UpdateLobbyServer(dt);
	else
		UpdateLobbyClient(dt);
}

//=================================================================================================
void ServerPanel::UpdateLobbyClient(float dt)
{
	// obs³uga komunikatów otrzymywanych przez klienta
	for(Packet* packet = net->peer->Receive(); packet; net->peer->DeallocatePacket(packet), packet = net->peer->Receive())
	{
		// ignore messages from master server (disconnect notification)
		if(packet->systemAddress == net->master_server_adr)
			continue;

		BitStreamReader reader(packet);
		byte msg_id;
		reader >> msg_id;

		switch(msg_id)
		{
		case ID_SAY:
			net->Client_Say(reader);
			break;
		case ID_WHISPER:
			net->Client_Whisper(reader);
			break;
		case ID_SERVER_SAY:
			net->Client_ServerSay(reader);
			break;
		case ID_LOBBY_UPDATE:
			DoLobbyUpdate(reader);
			break;
		case ID_DISCONNECTION_NOTIFICATION:
		case ID_CONNECTION_LOST:
		case ID_SERVER_CLOSE:
			{
				cstring reason, reason_eng;
				if(msg_id == ID_DISCONNECTION_NOTIFICATION)
				{
					reason = txPlayerDisconnected;
					reason_eng = "disconnected";
				}
				else if(msg_id == ID_CONNECTION_LOST)
				{
					reason = game->txLostConnection;
					reason_eng = "lost connection";
				}
				else if(packet->length == 2 && Any(packet->data[1], 0, 1))
				{
					switch(packet->data[1])
					{
					default:
					case ServerClose_Closing:
						reason = txClosing;
						reason_eng = "closing";
						break;
					case ServerClose_Kicked:
						reason = txKicked;
						reason_eng = "kicked";
						break;
					}
				}
				else
				{
					reason = txUnknown;
					reason_eng = "unknown";
				}

				Info("ServerPanel: Disconnected from server: %s.", reason_eng);
				gui->SimpleDialog(Format(txUnconnected, reason), nullptr);

				CloseDialog();
				net->peer->DeallocatePacket(packet);
				net->ClosePeer(true);
				return;
			}
		case ID_TIMER:
			if(packet->length != 2)
				Error("ServerPanel: Broken packet ID_TIMER.");
			else
			{
				Info("ServerPanel: Starting in %d...", packet->data[1]);
				AddMsg(Format(txStartingIn, packet->data[1]));
			}
			break;
		case ID_END_TIMER:
			Info("ServerPanel: Startup canceled.");
			AddMsg(txStartingStop);
			break;
		case ID_PICK_CHARACTER:
			if(packet->length != 2)
				Error("ServerPanel: Broken packet ID_PICK_CHARACTER.");
			else
			{
				bool ok = (packet->data[1] != 0);
				if(ok)
					Info("ServerPanel: Character pick accepted.");
				else
				{
					Warn("ServerPanel: Character pick refused.");
					PlayerInfo& info = net->GetMe();
					info.ready = false;
					info.clas = nullptr;
					bts[0].state = Button::NONE;
					bts[0].text = txPickChar;
					bts[1].state = Button::DISABLED;
				}
			}
			break;
		case ID_STARTUP:
			if(packet->length != 2)
				Error("ServerPanel: Broken packet ID_STARTUP.");
			else
			{
				Info("ServerPanel: Starting in 0...");
				AddMsg(Format(txStartingIn, 0));

				// close lobby and wait for server
				net->mp_quickload = false;
				net->mp_load_worldmap = (packet->data[1] == 1);
				Info("ServerPanel: Waiting for server.");
				game->LoadingStart(net->mp_load_worldmap ? 4 : 9);
				game_gui->main_menu->visible = false;
				CloseDialog();
				game_gui->info_box->Show(txWaitingForServer);
				game->net_mode = Game::NM_TRANSFER;
				game->net_state = NetState::Client_BeforeTransfer;
				net->peer->DeallocatePacket(packet);
				return;
			}
			break;
		default:
			Warn("ServerPanel: Unknown packet: %u.", msg_id);
			break;
		}
	}
}

//=================================================================================================
bool ServerPanel::DoLobbyUpdate(BitStreamReader& f)
{
	byte count;
	f >> count;
	if(!f)
	{
		Error("ServerPanel: Broken packet ID_LOBBY_UPDATE.");
		return false;
	}

	for(int i = 0; i < count; ++i)
	{
		byte type, id;
		f >> type;
		f >> id;
		if(!f)
		{
			Error("ServerPanel: Broken packet ID_LOBBY_UPDATE at index %d.", i);
			return false;
		}

		switch(type)
		{
		case Lobby_UpdatePlayer:
			{
				PlayerInfo* info = net->TryGetPlayer(id);
				if(!info)
				{
					Error("ServerPanel: Broken Lobby_UpdatePlayer, invalid player id %d.", id);
					return false;
				}
				f >> info->ready;
				const string& class_id = f.ReadString1();
				if(!f)
				{
					Error("ServerPanel: Broken Lobby_UpdatePlayer.");
					return false;
				}
				Class* clas = Class::TryGet(class_id);
				if(!clas)
				{
					Error("ServerPanel: Broken Lobby_UpdatePlayer, player '%s' have invalid class '%s'.", info->name.c_str(), class_id.c_str());
					return false;
				}
				else if(!clas->IsPickable())
				{
					Error("ServerPanel: Broken Lobby_UpdatePlayer, player 's' have non pickable class '%s'.", info->name.c_str(), class_id.c_str());
					return false;
				}
				else
					info->clas = clas;
			}
			break;
		case Lobby_AddPlayer:
			{
				const string& name = f.ReadString1();
				if(!f)
				{
					Error("ServerPanel: Broken Lobby_AddPlayer.");
					return false;
				}
				else if(id != team->my_id)
				{
					PlayerInfo* pinfo = new PlayerInfo;
					net->players.push_back(pinfo);

					PlayerInfo& info = *pinfo;
					info.state = PlayerInfo::IN_LOBBY;
					info.id = id;
					info.loaded = true;
					info.name = name;
					grid.AddItem();

					AddMsg(Format(txJoined, name.c_str()));
					Info("ServerPanel: Player %s joined lobby.", name.c_str());
				}
			}
			break;
		case Lobby_RemovePlayer:
		case Lobby_KickPlayer:
			{
				bool is_kick = (type == Lobby_KickPlayer);
				PlayerInfo* info = net->TryGetPlayer(id);
				if(!info)
				{
					Error("ServerPanel: Broken Lobby_Remove/KickPlayer, invalid player id %d.", id);
					return false;
				}
				Info("ServerPanel: Player %s %s.", info->name.c_str(), is_kick ? "was kicked" : "left lobby");
				AddMsg(Format(is_kick ? game->txPlayerKicked : txPlayerLeft, info->name.c_str()));
				int index = info->GetIndex();
				grid.RemoveItem(index);
				delete info;
				net->players.erase(index);
			}
			break;
		case Lobby_ChangeCount:
			net->active_players = id;
			break;
		case Lobby_ChangeLeader:
			{
				PlayerInfo* info = net->TryGetPlayer(id);
				if(!info)
				{
					Error("ServerPanel: Broken Lobby_ChangeLeader, invalid player id %d", id);
					return false;
				}
				Info("%s is now leader.", info->name.c_str());
				team->leader_id = id;
				if(team->my_id == id)
					AddMsg(txYouAreLeader);
				else
					AddMsg(Format(txLeaderChanged, info->name.c_str()));
			}
			break;
		default:
			Error("ServerPanel: Broken packet ID_LOBBY_UPDATE, unknown change type %d.", type);
			return false;
		}
	}

	return true;
}

//=================================================================================================
void ServerPanel::UpdateLobbyServer(float dt)
{
	// handle autostart
	if(!starting && autostart_count != 0u && autostart_count <= net->active_players)
	{
		bool ok = true;
		for(PlayerInfo& info : net->players)
		{
			if(!info.ready)
			{
				ok = false;
				break;
			}
		}

		if(ok)
		{
			Info("ServerPanel: Automatic server startup.");
			autostart_count = 0u;
			Start();
		}
	}

	for(Packet* packet = net->peer->Receive(); packet; net->peer->DeallocatePacket(packet), packet = net->peer->Receive())
	{
		BitStreamReader reader(packet);
		byte msg_id;
		reader >> msg_id;

		// pobierz informacje o graczu
		PlayerInfo* info = net->FindPlayer(packet->systemAddress);
		if(info && info->state == PlayerInfo::REMOVING)
		{
			if(msg_id == ID_DISCONNECTION_NOTIFICATION || msg_id == ID_CONNECTION_LOST)
			{
				// nie odes³a³ informacji tylko siê roz³¹czy³
				Info(!info->name.empty() ? Format("ServerPanel: Player %s has disconnected.", info->name.c_str()) :
					Format("ServerPanel: %s has disconnected.", packet->systemAddress.ToString()));
				--net->active_players;
				OnChangePlayersCount();
				int index = info->GetIndex();
				grid.RemoveItem(index);
				delete info;
				net->players.erase(index);
			}
			else
			{
				// nieznany komunikat od roz³¹czanego gracz, ignorujemy go
				Info("ServerPanel: Ignoring packet from %s while disconnecting.",
					!info->name.empty() ? info->name.c_str() : packet->systemAddress.ToString());
			}

			continue;
		}

		switch(msg_id)
		{
		case ID_UNCONNECTED_PING:
		case ID_UNCONNECTED_PING_OPEN_CONNECTIONS:
			assert(!info);
			Info("ServerPanel: Ping from %s.", packet->systemAddress.ToString());
			break;
		case ID_CONNECTION_REQUEST_ACCEPTED:
			// connected to master server
			{
				assert(net->master_server_state == MasterServerState::Connecting);
				Info("ServerPanel: Connected to master server.");
				net->master_server_state = MasterServerState::Registering;
				net->master_server_adr = packet->systemAddress;
				BitStreamWriter f;
				f << ID_MASTER_HOST;
				f << server_name;
				f << max_players;
				f << net->GetServerFlags();
				f << VERSION;
				net->peer->Send(&f.GetBitStream(), IMMEDIATE_PRIORITY, RELIABLE_ORDERED, 1, packet->systemAddress, false);
			}
			break;
		case ID_CONNECTION_ATTEMPT_FAILED:
			Info("ServerPanel: Failed to connect to master server.");
			net->master_server_state = MasterServerState::NotConnected;
			AddMsg(txRegisterFailed);
			break;
		case ID_MASTER_HOST:
			{
				byte result = 0xFF;
				reader >> result;
				if(result != 0)
				{
					Info("ServerPanel: Failed to register server (%u).", result);
					net->peer->CloseConnection(net->master_server_adr, true, 1, IMMEDIATE_PRIORITY);
					net->master_server_state = MasterServerState::NotConnected;
					net->master_server_adr = UNASSIGNED_SYSTEM_ADDRESS;
					AddMsg(txRegisterFailed);
				}
				else
				{
					Info("ServerPanel: Registered server.");
					net->master_server_state = MasterServerState::Connected;
					api->StartPunchthrough(nullptr);
					if(net->active_players != 1)
					{
						BitStreamWriter f;
						f << ID_MASTER_UPDATE;
						f << net->active_players;
						net->peer->Send(&f.GetBitStream(), IMMEDIATE_PRIORITY, RELIABLE_ORDERED, 1, packet->systemAddress, false);
					}
				}
			}
			break;
		case ID_NAT_PUNCHTHROUGH_SUCCEEDED:
			Info("ServerPanel: Client punchthrough succeeded.");
			break;
		case ID_NEW_INCOMING_CONNECTION:
			if(info)
				Warn("ServerPanel: Packet about connecting from already connected player %s.", info->name.c_str());
			else
			{
				Info("ServerPanel: New connection from %s.", packet->systemAddress.ToString());

				// add new player
				PlayerInfo* pinfo = new PlayerInfo;
				net->players.push_back(pinfo);
				PlayerInfo& info = *pinfo;
				info.adr = packet->systemAddress;
				info.id = net->GetNewPlayerId();
				grid.AddItem();

				// wait to receive info about version, nick
				info.state = PlayerInfo::WAITING_FOR_HELLO;
				info.timer = T_WAIT_FOR_HELLO;
				info.devmode = game->default_player_devmode;
				++net->active_players;
				OnChangePlayersCount();
			}
			break;
		case ID_DISCONNECTION_NOTIFICATION:
		case ID_CONNECTION_LOST:
			// klient siê roz³¹czy³
			{
				bool dis = (msg_id == ID_CONNECTION_LOST);
				if(!info)
				{
					Info(dis ? "ServerPanel: Disconnect notification from %s." :
						"ServerPanel: Lost connection with %s.", packet->systemAddress.ToString());
				}
				else
				{
					int index = info->GetIndex();
					grid.RemoveItem(index);
					if(info->state == PlayerInfo::WAITING_FOR_HELLO)
					{
						// roz³¹czy³ siê przed przyjêciem do lobby, mo¿na go usun¹æ
						AddMsg(Format(dis ? txDisconnected : txIpLostConnection, packet->systemAddress.ToString()));
						Info("ServerPanel: %s %s.", packet->systemAddress.ToString(), dis ? "disconnected" : "lost connection");
						delete info;
						net->players.erase(index);
						--net->active_players;
						OnChangePlayersCount();
					}
					else
					{
						// roz³¹czy³ siê bêd¹c w lobby
						--net->active_players;
						AddMsg(Format(dis ? txLeft : txPlayerLostConnection, info->name.c_str()));
						Info("ServerPanel: Player %s %s.", info->name.c_str(), dis ? "disconnected" : "lost connection");
						OnChangePlayersCount();
						if(net->active_players > 1)
							AddLobbyUpdate(Int2(Lobby_RemovePlayer, info->id));
						if(team->leader_id == info->id)
						{
							// serwer zostaje przywódc¹
							Info("ServerPanel: You are leader now.");
							team->leader_id = team->my_id;
							if(net->active_players > 1)
								AddLobbyUpdate(Int2(Lobby_ChangeLeader, 0));
							AddMsg(txYouAreLeader);
						}
						delete info;
						net->players.erase(index);
						CheckReady();
					}
				}
			}
			break;
		case ID_HELLO:
			// informacje od gracza
			if(!info)
				Warn("ServerPanel: Packet ID_HELLO from unconnected client %s.", packet->systemAddress.ToString());
			else if(info->state != PlayerInfo::WAITING_FOR_HELLO)
				Warn("ServerPanel: Packet ID_HELLO from player %s who already joined.", info->name.c_str());
			else
			{
				int version;
				cstring reason_text = nullptr;
				int include_extra = 0;
				uint my_crc, player_crc;
				Content::Id type;
				cstring type_str;
				JoinResult reason = JoinResult::Ok;

				reader >> version;
				content.ReadCrc(reader);
				reader >> info->name;
				if(!reader)
				{
					// failed to read packet
					reason = JoinResult::BrokenPacket;
					reason_text = Format("ServerPanel: Broken packet ID_HELLO from %s.", packet->systemAddress.ToString());
				}
				else if(version != VERSION)
				{
					// version mismatch
					reason = JoinResult::InvalidVersion;
					reason_text = Format("UpdateLobbbyNet: Invalid version from %s. Our (%s) vs (%s).", packet->systemAddress.ToString(),
						VersionToString(version), VERSION_STR);
				}
				else if(!content.ValidateCrc(type, my_crc, player_crc, type_str))
				{
					// invalid game type manager crc
					reason = JoinResult::InvalidCrc;
					reason_text = Format("ServerPanel: Invalid %s crc from %s. Our (%p) vs (%p).", type_str, packet->systemAddress.ToString(), my_crc,
						player_crc);
					include_extra = 2;
				}
				else if(!net->ValidateNick(info->name.c_str()))
				{
					// invalid nick
					reason = JoinResult::InvalidNick;
					reason_text = Format("ServerPanel: Invalid nick (%s) from %s.", info->name.c_str(), packet->systemAddress.ToString());
				}
				else
				{
					// check if nick is unique
					bool ok = true;
					for(PlayerInfo& info2 : net->players)
					{
						if(info2.id != info->id && info2.state == PlayerInfo::IN_LOBBY && info2.name == info->name)
						{
							ok = false;
							break;
						}
					}
					if(!ok)
					{
						// nick is already used
						reason = JoinResult::TakenNick;
						reason_text = Format("ServerPanel: Nick already in use (%s) from %s.", info->name.c_str(), packet->systemAddress.ToString());
					}
				}

				BitStreamWriter fw;
				if(reason != JoinResult::Ok)
				{
					Warn(reason_text);
					fw << ID_CANT_JOIN;
					fw.WriteCasted<byte>(reason);
					if(include_extra != 0)
						fw << my_crc;
					if(include_extra == 2)
						fw.WriteCasted<byte>(type);
					net->SendServer(fw, MEDIUM_PRIORITY, RELIABLE, packet->systemAddress);
					info->state = PlayerInfo::REMOVING;
					info->timer = T_WAIT_FOR_DISCONNECT;
				}
				else
				{
					// everything is ok, let player join
					if(net->active_players > 2)
						AddLobbyUpdate(Int2(Lobby_AddPlayer, info->id));
					fw << ID_JOIN;
					fw.WriteCasted<byte>(info->id);
					fw.WriteCasted<byte>(net->active_players);
					fw.WriteCasted<byte>(team->leader_id);
					fw.Write0();
					int count = 0;
					for(PlayerInfo& info2 : net->players)
					{
						if(info2.id == info->id || info2.state != PlayerInfo::IN_LOBBY)
							continue;
						++count;
						fw.WriteCasted<byte>(info2.id);
						fw.WriteCasted<byte>(info2.ready ? 1 : 0);
						if(info2.clas)
							fw << info2.clas->id;
						else
							fw.Write0();
						fw << info2.name;
					}
					fw.Patch<byte>(4, count);
					if(net->mp_load)
					{
						// informacja o postaci w zapisie
						PlayerInfo* old = net->FindOldPlayer(info->name.c_str());
						if(old)
						{
							fw.WriteCasted<byte>(2);
							fw << old->clas->id;

							info->clas = old->clas;
							info->loaded = true;
							info->devmode = old->devmode;
							if(game->default_player_devmode)
								info->devmode = true;
							info->hd.CopyFrom(old->hd);
							info->notes = old->notes;

							if(net->active_players > 2)
								AddLobbyUpdate(Int2(Lobby_UpdatePlayer, info->id));

							Info("ServerPanel: Player %s is using loaded character.", info->name.c_str());
						}
						else
							fw.Write1();
					}
					else
						fw.Write0();
					net->SendServer(fw, HIGH_PRIORITY, RELIABLE, packet->systemAddress);
					info->state = PlayerInfo::IN_LOBBY;

					AddMsg(Format(txJoined, info->name.c_str()));
					Info("ServerPanel: Player %s (%s) joined, id: %d.", info->name.c_str(), packet->systemAddress.ToString(), info->id);

					CheckReady();
				}
			}
			break;
		case ID_CHANGE_READY:
			if(!info)
				Warn("ServerPanel: Packet ID_CHANGE_READY from unconnected client %s.", packet->systemAddress.ToString());
			else
			{
				bool ready;
				reader >> ready;
				if(!reader)
					Error("ServerPanel: Broken packet ID_CHANGE_READY from client %s.", info->name.c_str());
				else if(ready != info->ready)
				{
					info->ready = ready;
					if(net->active_players > 2)
						AddLobbyUpdate(Int2(Lobby_UpdatePlayer, info->id));
					CheckReady();
				}
			}
			break;
		case ID_SAY:
			if(!info)
				Warn("ServerPanel: Packet ID_SAY from unconnected client %s.", packet->systemAddress.ToString());
			else
				net->Server_Say(reader, *info);
			break;
		case ID_WHISPER:
			if(!info)
				Warn("ServerPanel: Packet ID_WHISPER from unconnected client %s.", packet->systemAddress.ToString());
			else
				net->Server_Whisper(reader, *info);
			break;
		case ID_LEAVE:
			if(!info)
				Warn("ServerPanel: Packet ID_LEAVE from unconnected client %.", packet->systemAddress.ToString());
			else
			{
				cstring name = info->state == PlayerInfo::WAITING_FOR_HELLO ? info->adr.ToString() : info->name.c_str();
				AddMsg(Format(txPlayerLeft, name));
				Info("ServerPanel: Player %s left lobby.", name);
				--net->active_players;
				OnChangePlayersCount();
				if(net->active_players > 1 && info->state == PlayerInfo::IN_LOBBY)
					AddLobbyUpdate(Int2(Lobby_RemovePlayer, info->id));
				int index = info->GetIndex();
				grid.RemoveItem(index);
				net->peer->CloseConnection(packet->systemAddress, true);
				delete info;
				net->players.erase(index);
				CheckReady();
			}
			break;
		case ID_PICK_CHARACTER:
			if(!info || info->state != PlayerInfo::IN_LOBBY)
				Warn("ServerPanel: Packet ID_PICK_CHARACTER from player not in lobby %s.", packet->systemAddress.ToString());
			else
			{
				Class* old_class = info->clas;
				bool old_ready = info->ready;
				int result = ReadCharacterData(reader, info->clas, info->hd, info->cc);
				bool ok = false;
				if(result == 0)
				{
					reader >> info->ready;
					if(reader)
					{
						ok = true;
						Info("Received character from '%s'.", info->name.c_str());
					}
					else
						Error("ServerPanel: Broken packet ID_PICK_CHARACTER from '%s'.", info->name.c_str());
				}
				else
				{
					cstring err[3] = {
						"read error",
						"value error",
						"validation error"
					};

					Error("ServerPanel: Packet ID_PICK_CHARACTER from '%s' %s.", info->name.c_str(), err[result - 1]);
				}

				if(!ok)
				{
					info->ready = false;
					info->clas = nullptr;
				}
				CheckReady();

				// send info to other net->active_players
				if((old_ready != info->ready || old_class != info->clas) && net->active_players > 2)
					AddLobbyUpdate(Int2(Lobby_UpdatePlayer, info->id));

				// send result
				byte packet[2] = { ID_PICK_CHARACTER, byte(ok ? 1 : 0) };
				net->peer->Send((cstring)packet, 2, HIGH_PRIORITY, RELIABLE, 0, info->adr, false);
			}
			break;
		default:
			Warn("ServerPanel: Unknown packet from %s: %u.", info ? info->name.c_str() : packet->systemAddress.ToString(), msg_id);
			break;
		}
	}

	int index = 0;
	LoopAndRemove(net->players, [&](PlayerInfo& info)
	{
		if(info.state != PlayerInfo::IN_LOBBY)
		{
			info.timer -= dt;
			if(info.timer <= 0.f)
			{
				// czas oczekiwania min¹³, zkickuj
				Info("ServerPanel: Removed %s due to inactivity.", info.adr.ToString());
				net->peer->CloseConnection(info.adr, false);
				--net->active_players;
				if(net->active_players > 1)
					AddLobbyUpdate(Int2(Lobby_RemovePlayer, 0));
				delete &info;
				grid.RemoveItem(index);
				return true;
			}
			else
			{
				++index;
				return false;
			}
		}
		else
		{
			++index;
			return false;
		}
	});

	// wysy³anie aktualizacji lobby
	update_timer += dt;
	if(update_timer >= T_LOBBY_UPDATE)
	{
		update_timer = 0.f;
		if(!lobby_updates.empty())
		{
			assert(lobby_updates.size() < 255);
			if(net->active_players > 1)
			{
				// aktualizacje w lobby
				BitStreamWriter f;
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
							PlayerInfo* info = net->TryGetPlayer(u.y);
							if(info)
							{
								++count;
								f.WriteCasted<byte>(u.x);
								f.WriteCasted<byte>(info->id);
								f << info->ready;
								f << info->clas->id;
							}
						}
						break;
					case Lobby_AddPlayer:
						{
							PlayerInfo* info = net->TryGetPlayer(u.y);
							if(info)
							{
								++count;
								f.WriteCasted<byte>(u.x);
								f.WriteCasted<byte>(info->id);
								f << info->name;
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
						f.WriteCasted<byte>(net->active_players);
						break;
					case Lobby_ChangeLeader:
						++count;
						f.WriteCasted<byte>(u.x);
						f.WriteCasted<byte>(team->leader_id);
						break;
					default:
						assert(0);
						break;
					}
				}
				f.Patch<byte>(1, count);
				net->SendAll(f, HIGH_PRIORITY, RELIABLE_ORDERED);
			}
			lobby_updates.clear();
		}
	}

	// starting game
	if(starting)
	{
		startup_timer -= dt;
		int sec = int(startup_timer) + 1;
		int d = -1;
		if(startup_timer <= 0.f)
		{
			// disconnect from master server
			if(net->master_server_state == MasterServerState::Connected)
				net->peer->CloseConnection(net->master_server_adr, true, 1, IMMEDIATE_PRIORITY);
			net->master_server_state = MasterServerState::NotConnected;
			net->master_server_adr = UNASSIGNED_SYSTEM_ADDRESS;
			api->EndPunchthrough();
			// change server status
			Info("ServerPanel: Starting game.");
			starting = false;
			d = 0;
			net->peer->SetMaximumIncomingConnections(0);
			net->peer->SetOfflinePingResponse(nullptr, 0);
			game->net_mode = Game::NM_TRANSFER_SERVER;
			game->net_timer = game->mp_timeout;
			game->net_state = NetState::Server_Starting;
			// kick players that connected but didn't join
			LoopAndRemove(net->players, [](PlayerInfo& info)
			{
				if(info.state != PlayerInfo::IN_LOBBY)
				{
					net->peer->CloseConnection(info.adr, true);
					Warn("ServerPanel: Disconnecting %s.", info.adr.ToString());
					delete &info;
					--net->active_players;
					return true;
				}
				return false;
			});
			CloseDialog();
			game_gui->info_box->Show(txStartingGame);
			BitStreamWriter f;
			f << ID_STARTUP;
			f << (byte)(net->mp_load && !game_level->is_open ? 1 : 0);
			net->peer->Send(&f.GetBitStream(), IMMEDIATE_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
			return;
		}
		else if(sec != last_startup_sec)
		{
			last_startup_sec = sec;
			d = sec;
		}

		if(d != -1)
		{
			Info("ServerPanel: Starting in %d...", d);
			AddMsg(Format(txStartingIn, d));
			BitStreamWriter f;
			f << ID_TIMER;
			f << (byte)d;
			net->peer->Send(&f.GetBitStream(), IMMEDIATE_PRIORITY, RELIABLE, 0, net->master_server_adr, true);
		}
	}
}

//=================================================================================================
void ServerPanel::OnChangePlayersCount()
{
	if(net->master_server_state == MasterServerState::Connected && game->net_mode != Game::NM_QUITTING_SERVER)
	{
		BitStreamWriter f;
		f << ID_MASTER_UPDATE;
		f << net->active_players;
		net->peer->Send(&f.GetBitStream(), IMMEDIATE_PRIORITY, RELIABLE_ORDERED, 1, net->master_server_adr, false);
	}
	if(net->active_players > 1)
		AddLobbyUpdate(Int2(Lobby_ChangeCount, 0));
	UpdateServerInfo();
}

//=================================================================================================
void ServerPanel::UpdateServerInfo()
{
	// 0 char C
	// 1 char A
	// 2-5 int - version
	// 6 byte - players count
	// 7 byte - max players
	// 8 byte - flags
	// 9+ byte - name
	BitStreamWriter f;
	f.WriteCasted<byte>('C');
	f.WriteCasted<byte>('A');
	f << VERSION;
	f.WriteCasted<byte>(net->active_players);
	f.WriteCasted<byte>(max_players);
	f.WriteCasted<byte>(net->GetServerFlags());
	f << server_name;

	net->peer->SetOfflinePingResponse(f.GetData(), f.GetSize());
}

//=================================================================================================
void ServerPanel::Event(GuiEvent e)
{
	switch(e)
	{
	case GuiEvent_Show:
	case GuiEvent_WindowResize:
		if(e == GuiEvent_Show)
		{
			visible = true;
			itb.focus = true;
			itb.Event(GuiEvent_GainFocus);
		}
		global_pos = pos = (gui->wnd_size - size) / 2;
		for(int i = 0; i < 6; ++i)
			bts[i].global_pos = global_pos + bts[i].pos;
		itb.Event(GuiEvent_Moved);
		grid.Move(global_pos);
		break;
	case GuiEvent_GainFocus:
		itb.focus = true;
		itb.Event(GuiEvent_GainFocus);
		break;
	case GuiEvent_LostFocus:
		grid.LostFocus();
		itb.focus = false;
		itb.Event(GuiEvent_LostFocus);
		break;
	case GuiEvent_Close:
		visible = false;
		grid.LostFocus();
		itb.focus = false;
		itb.Event(GuiEvent_LostFocus);
		break;
	case IdPickCharacter: // pick character / change character
		{
			PlayerInfo& info = net->GetMe();
			if(info.clas)
			{
				// already have character, redo
				if(info.ready)
				{
					// uncheck ready
					info.ready = false;
					ChangeReady();
				}
				game_gui->ShowCreateCharacterPanel(false, true);
			}
			else
				game_gui->ShowCreateCharacterPanel(false);
		}
		break;
	case IdReady: // ready / unready
		{
			PlayerInfo& info = net->GetMe();
			info.ready = !info.ready;
			ChangeReady();
		}
		break;
	case IdKick: // kick / cancel
		if(Net::IsServer())
		{
			if(grid.selected == -1)
				AddMsg(txNeedSelectedPlayer);
			else if(grid.selected == 0)
				AddMsg(txCantKickMyself);
			else
			{
				PlayerInfo& info = net->players[grid.selected];
				if(info.state != PlayerInfo::IN_LOBBY)
					AddMsg(txCantKickUnconnected);
				else
				{
					// na pewno?
					kick_id = info.id;
					DialogInfo di;
					di.event = DialogEvent(this, &ServerPanel::OnKick);
					di.name = "kick";
					di.order = ORDER_TOP;
					di.parent = this;
					di.pause = false;
					di.text = Format(txReallyKick, info.name.c_str());
					di.type = DIALOG_YESNO;
					gui->ShowDialog(di);
				}
				return;
			}
		}
		else
			ExitLobby();
		break;
	case IdLeader: // change leader
		if(grid.selected == -1)
			AddMsg(txNeedSelectedPlayer);
		else
		{
			PlayerInfo& info = net->players[grid.selected];
			if(info.id == team->leader_id)
				AddMsg(txAlreadyLeader);
			else if(info.state == PlayerInfo::IN_LOBBY)
			{
				team->leader_id = info.id;
				AddLobbyUpdate(Int2(Lobby_ChangeLeader, 0));
				AddMsg(Format(txLeaderChanged, info.name.c_str()));
			}
			else
				AddMsg(txNotJoinedYet);
		}
		break;
	case IdStart: // start game / stop
		if(!starting)
		{
			cstring error_text = nullptr;

			for(PlayerInfo& info : net->players)
			{
				if(!info.ready)
				{
					error_text = txNotAllReady;
					AddMsg(error_text);
					break;
				}
			}

			if(!error_text)
				Start();
		}
		else
			StopStartup();
		break;
	case IdCancel: // cancel
		ExitLobby();
		break;
	}
}

//=================================================================================================
void ServerPanel::Show()
{
	starting = false;
	update_timer = 0.f;

	bts[0].text = txPickChar;
	bts[0].state = Button::NONE;
	bts[1].text = txReady;
	bts[1].state = Button::DISABLED;

	if(Net::IsServer())
	{
		bts[2].text = txKick;
		bts[4].state = Button::DISABLED;
		bts[4].text = txStart;
	}
	else
		bts[2].text = gui->txCancel;

	itb.Reset();
	grid.Reset();
	grid.AddItems(net->players.size());

	gui->ShowDialog(this);
}

//=================================================================================================
void ServerPanel::GetCell(int item, int column, Cell& cell)
{
	PlayerInfo& info = net->players[item];

	if(column == 0)
		cell.img = (info.ready ? tReady : tNotReady);
	else if(column == 1)
	{
		cell.text_color->text = (info.state == PlayerInfo::IN_LOBBY ? info.name.c_str() : info.adr.ToString());
		cell.text_color->color = (info.id == team->leader_id ? Color(0xFFFFD700) : Color::Black);
	}
	else
		cell.text = (info.clas ? info.clas->name.c_str() : txNone);
}

//=================================================================================================
void ServerPanel::ExitLobby(VoidF callback)
{
	Info("ServerPanel: Exiting lobby.");

	if(Net::IsServer())
	{
		if(net->mp_load)
			game->ClearGame();

		Info("ServerPanel: Closing server.");

		api->EndPunchthrough();
		// zablokuj do³¹czanie
		net->peer->SetMaximumIncomingConnections(0);
		// wy³¹cz info o serwerze
		net->peer->SetOfflinePingResponse(nullptr, 0);

		if(net->active_players > 1)
		{
			// roz³¹cz graczy
			Info("ServerPanel: Disconnecting clients.");
			const byte b[] = { ID_SERVER_CLOSE, ServerClose_Closing };
			net->peer->Send((cstring)b, 2, IMMEDIATE_PRIORITY, RELIABLE, 0, net->master_server_adr, true);
			game->net_mode = Game::NM_QUITTING_SERVER;
			--net->active_players;
			game->net_timer = T_WAIT_FOR_DISCONNECT;
			game_gui->info_box->Show(txDisconnecting);
			game->net_callback = callback;
		}
		else
		{
			// nie ma graczy, mo¿na zamkn¹æ
			net->ClosePeer(net->master_server_state >= MasterServerState::Connecting);
			CloseDialog();
			if(callback)
				callback();
		}
	}
	else
	{
		BitStreamWriter f;
		f << ID_LEAVE;
		net->SendClient(f, IMMEDIATE_PRIORITY, RELIABLE);
		game_gui->info_box->Show(txDisconnecting);
		game->net_mode = Game::NM_QUITTING;
		game->net_timer = T_WAIT_FOR_DISCONNECT;
		game->net_callback = callback;
		CloseDialog();
	}
}

//=================================================================================================
void ServerPanel::OnKick(int id)
{
	if(id == BUTTON_YES)
	{
		PlayerInfo* info = net->TryGetPlayer(kick_id);
		if(info)
			net->KickPlayer(*info);
	}
}

//=================================================================================================
void ServerPanel::OnInput(const string& str)
{
	if(str[0] == '/')
		cmdp->ParseCommand(str.substr(1), CommandParser::PrintMsgFunc(this, &ServerPanel::AddMsg), PS_LOBBY);
	else
	{
		// wyœlij tekst
		if(net->active_players != 1)
		{
			BitStreamWriter f;
			f << ID_SAY;
			f.WriteCasted<byte>(team->my_id);
			f << str;
			if(Net::IsServer())
				net->SendAll(f, MEDIUM_PRIORITY, RELIABLE);
			else
				net->SendClient(f, MEDIUM_PRIORITY, RELIABLE);
		}
		cstring s = Format("%s: %s", game->player_name.c_str(), str.c_str());
		AddMsg(s);
		Info(s);
	}
}

//=================================================================================================
void ServerPanel::StopStartup()
{
	Info("Startup canceled.");
	AddMsg(txStartingStop);
	starting = false;
	bts[4].text = txStart;

	if(net->active_players > 1)
	{
		byte c = ID_END_TIMER;
		net->peer->Send((cstring)&c, 1, IMMEDIATE_PRIORITY, RELIABLE, 0, net->master_server_adr, true);
	}
}

//=================================================================================================
void ServerPanel::UseLoadedCharacter(bool have)
{
	if(have)
	{
		Info("ServerPanel: Joined loaded game with existing character.");
		autopick_class = nullptr;
		bts[0].state = Button::DISABLED;
		bts[0].text = txChangeChar;
		bts[1].state = Button::NONE;
		AddMsg(txLoadedCharInfo);
	}
	else
	{
		Info("ServerPanel: Joined loaded game without loaded character.");
		AddMsg(txNotLoadedCharInfo);
	}
}

//=================================================================================================
void ServerPanel::CheckAutopick()
{
	if(autopick_class)
	{
		Info("ServerPanel: Autopicking character.");
		if(autopick_class == random_class)
			autopick_class = nullptr;
		PickClass(autopick_class, true);
		autopick_class = nullptr;
		autoready = false;
	}
}

//=================================================================================================
// Pick selected class - for nullptr select random
void ServerPanel::PickClass(Class* clas, bool ready)
{
	PlayerInfo& info = net->GetMe();
	info.clas = clas;
	game->RandomCharacter(info.clas, game_gui->create_character->last_hair_color_index, info.hd, info.cc);
	bts[0].text = txChangeChar;
	bts[1].state = Button::NONE;
	bts[1].text = (ready ? txNotReady : txReady);
	info.ready = ready;
	if(Net::IsClient())
	{
		Info("ServerPanel: Sent pick class packet.");
		BitStreamWriter f;
		f << ID_PICK_CHARACTER;
		WriteCharacterData(f, info.clas, info.hd, info.cc);
		f << ready;
		net->SendClient(f, IMMEDIATE_PRIORITY, RELIABLE);
	}
	else
	{
		if(net->active_players > 1)
			AddLobbyUpdate(Int2(Lobby_UpdatePlayer, 0));
		CheckReady();
	}
}

//=================================================================================================
void ServerPanel::AddLobbyUpdate(const Int2& u)
{
	for(Int2& update : lobby_updates)
	{
		if(update == u)
			return;
	}
	lobby_updates.push_back(u);
}

//=================================================================================================
void ServerPanel::CheckReady()
{
	bool all_ready = true;

	for(PlayerInfo& info : net->players)
	{
		if(!info.ready)
		{
			all_ready = false;
			break;
		}
	}

	if(all_ready)
		bts[4].state = Button::NONE;
	else
		bts[4].state = Button::DISABLED;

	if(starting)
		StopStartup();
}

//=================================================================================================
void ServerPanel::ChangeReady()
{
	if(Net::IsServer())
	{
		// zmieñ info
		if(net->active_players > 1)
			AddLobbyUpdate(Int2(Lobby_UpdatePlayer, 0));
		CheckReady();
	}
	else
	{
		BitStreamWriter f;
		f << ID_CHANGE_READY;
		f << net->GetMe().ready;
		net->SendClient(f, HIGH_PRIORITY, RELIABLE_ORDERED);
	}

	bts[1].text = (net->GetMe().ready ? txNotReady : txReady);
}

//=================================================================================================
bool ServerPanel::Quickstart()
{
	if(Net::IsServer())
	{
		if(!starting)
		{
			PlayerInfo& info = net->GetMe();
			if(!info.ready)
			{
				if(!info.clas)
					PickClass(nullptr, true);
				else
				{
					info.ready = true;
					ChangeReady();
				}
			}

			for(PlayerInfo& info : net->players)
			{
				if(!info.ready)
					return false;
			}

			Start();
		}
	}
	else
	{
		PlayerInfo& info = net->GetMe();
		if(!info.ready)
		{
			if(!info.clas)
				PickClass(nullptr, true);
			else
			{
				info.ready = true;
				ChangeReady();
			}
		}
	}

	return true;
}

//=================================================================================================
cstring ServerPanel::TryStart()
{
	if(starting)
		return "Server is already starting.";

	for(PlayerInfo& info : net->players)
	{
		if(!info.ready)
			return "Not everyone is ready.";
	}

	Start();
	return nullptr;
}

//=================================================================================================
void ServerPanel::Start()
{
	starting = true;
	last_startup_sec = STARTUP_TIMER;
	startup_timer = float(STARTUP_TIMER);
	BitStreamWriter f;
	f << ID_TIMER;
	f << (byte)STARTUP_TIMER;
	net->SendAll(f, IMMEDIATE_PRIORITY, RELIABLE);
	bts[4].text = txStop;
	AddMsg(Format(txStartingIn, STARTUP_TIMER));
	Info("ServerPanel: Starting in %d...", STARTUP_TIMER);
}
