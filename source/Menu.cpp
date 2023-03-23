#include "Pch.h"
#include "Game.h"

#include "AbilityPanel.h"
#include "AIController.h"
#include "BitStreamFunc.h"
#include "City.h"
#include "Controls.h"
#include "CreateCharacterPanel.h"
#include "CreateServerPanel.h"
#include "EntityInterpolator.h"
#include "GameGui.h"
#include "GameMenu.h"
#include "GameMessages.h"
#include "GameResources.h"
#include "InfoBox.h"
#include "InsideLocation.h"
#include "Language.h"
#include "Level.h"
#include "LevelGui.h"
#include "LoadScreen.h"
#include "LobbyApi.h"
#include "MainMenu.h"
#include "MpBox.h"
#include "MultiplayerPanel.h"
#include "Options.h"
#include "PickServerPanel.h"
#include "PlayerInfo.h"
#include "Portal.h"
#include "QuestManager.h"
#include "Quest_Tutorial.h"
#include "SaveLoadPanel.h"
#include "ServerPanel.h"
#include "Team.h"
#include "Utility.h"
#include "Version.h"
#include "World.h"
#include "WorldMapGui.h"

#include <Engine.h>
#include <GetTextDialog.h>
#include <Render.h>
#include <SceneManager.h>
#include <SoundManager.h>
#include <Terrain.h>

// consts
const float T_TRY_CONNECT = 5.f;
const int T_SHUTDOWN = 1000;
const float T_WAIT_FOR_DATA = 5.f;

//=================================================================================================
bool Game::CanShowMenu()
{
	return !gui->HaveDialog() && !gameGui->levelGui->HavePanelOpen() && !gameGui->mainMenu->visible
		&& gameState != GS_MAIN_MENU && !dialogContext.dialogMode;
}

//=================================================================================================
void Game::SaveOptions()
{
	cfg.Add("fullscreen", engine->IsFullscreen());
	cfg.Add("useNormalmap", sceneMgr->useNormalmap);
	cfg.Add("useSpecularmap", sceneMgr->useSpecularmap);
	cfg.Add("soundVolume", soundMgr->GetSoundVolume());
	cfg.Add("musicVolume", soundMgr->GetMusicVolume());
	cfg.Add("soundDevice", soundMgr->GetDevice().ToString());
	cfg.Add("mouseSensitivity", settings.mouseSensitivity);
	cfg.Add("grassRange", settings.grassRange);
	cfg.Add("resolution", engine->GetWindowSize());
	cfg.Add("skipTutorial", skipTutorial);
	cfg.Add("language", Language::prefix);
	int ms, msq;
	render->GetMultisampling(ms, msq);
	cfg.Add("multisampling", ms);
	cfg.Add("multisamplingQuality", msq);
	cfg.Add("vsync", render->IsVsyncEnabled());
	SaveCfg();
}

//=================================================================================================
void Game::StartNewGame()
{
	HumanData hd;
	hd.Get(*gameGui->createCharacter->unit->humanData);
	NewGameCommon(gameGui->createCharacter->clas, gameGui->createCharacter->playerName.c_str(), hd, gameGui->createCharacter->cc, false);
}

//=================================================================================================
void Game::StartQuickGame()
{
	Net::SetMode(Net::Mode::Singleplayer);

	Class* clas = nullptr;
	const string& classId = cfg.GetString("quickstartClass");
	if(!classId.empty())
	{
		clas = Class::TryGet(classId);
		if(clas)
		{
			if(!clas->IsPickable())
			{
				Warn("Settings [quickstartClass]: Class '%s' is not pickable by players.", classId.c_str());
				clas = nullptr;
			}
		}
		else
			Warn("Settings [quickstartClass]: Invalid class '%s'.", classId.c_str());
	}

	string quickstartName = cfg.GetString("quickstartName", "Test");
	if(quickstartName.empty())
		quickstartName = "Test";
	HumanData hd;
	CreatedCharacter cc;
	int hairIndex;

	RandomCharacter(clas, hairIndex, hd, cc);
	NewGameCommon(clas, quickstartName.c_str(), hd, cc, false);
}

//=================================================================================================
void Game::NewGameCommon(Class* clas, cstring name, HumanData& hd, CreatedCharacter& cc, bool tutorial)
{
	ClearGameVars(true);
	gameLevel->ready = false;
	questMgr->questTutorial->inTutorial = tutorial;
	gameGui->mainMenu->visible = false;
	Net::SetMode(Net::Mode::Singleplayer);
	hardcoreMode = hardcoreOption;

	Unit* u = gameLevel->CreateUnit(*clas->player, -1, false);
	u->locPart = nullptr;
	u->ApplyHumanData(hd);
	team->members.clear();
	team->activeMembers.clear();
	team->members.push_back(u);
	team->activeMembers.push_back(u);
	team->leader = u;

	pc = new PlayerController;
	pc->id = 0;
	pc->isLocal = true;
	pc->name = name;
	pc->Init(*u, &cc);

	pc->dialogCtx = &dialogContext;
	pc->dialogCtx->dialogMode = false;
	pc->dialogCtx->pc = pc;
	pc->dialogCtx->isLocal = true;

	if(questMgr->questTutorial->finishedTutorial)
	{
		u->AddItem(Item::Get("bookAdventurer"), 1u, false);
		questMgr->questTutorial->finishedTutorial = false;
	}

	gameLevel->camera.target = u;
	team->CalculatePlayersLevel();
	gameGui->Setup(pc);
	gameGui->ability->Refresh();

	if(!tutorial && cc.HavePerk(Perk::leader))
	{
		Unit* npc = gameLevel->CreateUnit(*Class::GetRandomHeroData(), -1, false);
		npc->locPart = nullptr;
		npc->ai = new AIController;
		npc->ai->Init(npc);
		npc->hero->knowName = true;
		team->AddMember(npc, HeroType::Normal);
		--team->freeRecruits;
		npc->hero->SetupMelee();
	}

	fallbackType = FALLBACK::NONE;
	fallbackTimer = -0.5f;
	SetTitle("SINGLE");

	if(!tutorial)
	{
		GenerateWorld();
		questMgr->InitQuests();
		world->StartInLocation();
		gameRes->LoadCommonMusic();
		EnterLocation();
	}
}

//=================================================================================================
void Game::MultiplayerPanelEvent(int id)
{
	playerName = Trimmed(gameGui->multiplayer->textbox.GetText());

	// check nick
	if(playerName.empty())
	{
		gui->SimpleDialog(gameGui->multiplayer->txNeedEnterNick, gameGui->multiplayer);
		return;
	}
	if(!net->ValidateNick(playerName.c_str()))
	{
		gui->SimpleDialog(gameGui->multiplayer->txEnterValidNick, gameGui->multiplayer);
		return;
	}

	// save nick
	cfg.Add("nick", playerName);
	SaveCfg();

	switch(id)
	{
	case MultiplayerPanel::IdJoinLan:
		gameGui->pickServer->Show();
		break;
	case MultiplayerPanel::IdJoinIp:
		{
			GetTextDialogParams params(txEnterIp, serverIp);
			params.event = DialogEvent(this, &Game::OnEnterIp);
			params.limit = 100;
			params.parent = gameGui->multiplayer;
			GetTextDialog::Show(params);
		}
		break;
	case MultiplayerPanel::IdCreate:
		gameGui->createServer->Show();
		break;
	case MultiplayerPanel::IdLoad:
		net->mpLoad = true;
		net->mpQuickload = false;
		Net::changes.clear();
		if(!net->netStrs.empty())
			StringPool.Free(net->netStrs);
		gameGui->saveload->ShowLoadPanel();
		break;
	}
}

//=================================================================================================
void Game::CreateServerEvent(int id)
{
	if(id == BUTTON_CANCEL)
	{
		if(net->mpLoad)
		{
			ClearGame();
			net->mpLoad = false;
		}
		gameGui->createServer->CloseDialog();
	}
	else
	{
		// copy settings
		net->serverName = Trimmed(gameGui->createServer->textbox[0].GetText());
		net->maxPlayers = atoi(gameGui->createServer->textbox[1].GetText().c_str());
		net->password = gameGui->createServer->textbox[2].GetText();
		net->serverLan = gameGui->createServer->checkbox.checked;

		// check settings
		cstring errorText = nullptr;
		Control* giveFocus = nullptr;
		if(net->serverName.empty())
		{
			errorText = gameGui->createServer->txEnterServerName;
			giveFocus = &gameGui->createServer->textbox[0];
		}
		else if(!InRange(net->maxPlayers, MIN_PLAYERS, MAX_PLAYERS))
		{
			errorText = Format(gameGui->createServer->txInvalidPlayersCount, MAX_PLAYERS);
			giveFocus = &gameGui->createServer->textbox[1];
		}
		if(errorText)
		{
			gui->SimpleDialog(errorText, gameGui->createServer);
			gameGui->createServer->cont.giveFocus = giveFocus;
			return;
		}

		// save settings
		cfg.Add("serverName", net->serverName);
		cfg.Add("serverPswd", net->password);
		cfg.Add("serverPlayers", net->maxPlayers);
		cfg.Add("serverLan", net->serverLan);
		SaveCfg();

		// close dialog windows
		gameGui->createServer->CloseDialog();
		gameGui->multiplayer->CloseDialog();

		try
		{
			net->InitServer();
		}
		catch(cstring err)
		{
			gui->SimpleDialog(err, gameGui->mainMenu);
			return;
		}

		net->OnNewGameServer();
	}
}

//=================================================================================================
void Game::RandomCharacter(Class*& clas, int& hairIndex, HumanData& hd, CreatedCharacter& cc)
{
	if(!clas)
		clas = Class::GetRandomPlayer();
	// appearance
	hd.beard = Rand() % MAX_BEARD - 1;
	hd.hair = Rand() % MAX_HAIR - 1;
	hd.mustache = Rand() % MAX_MUSTACHE - 1;
	hairIndex = Rand() % nHairColors;
	hd.hairColor = gHairColors[hairIndex];
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
		Trim(serverIp);
		if(adr.FromString(serverIp.c_str(), ':') && adr != UNASSIGNED_SYSTEM_ADDRESS)
		{
			// save ip
			cfg.Add("serverIp", serverIp.c_str());
			SaveCfg();

			try
			{
				net->InitClient();
			}
			catch(cstring error)
			{
				gui->SimpleDialog(error, gameGui->multiplayer);
				return;
			}

			Info("Pinging %s...", adr.ToString());
			gameGui->infoBox->Show(txConnecting);
			netMode = NM_CONNECTING;
			netState = NetState::Client_PingIp;
			netTimer = T_CONNECT_PING;
			netTries = I_CONNECT_TRIES;
			net->pingAdr = adr;
			if(net->pingAdr.GetPort() == 0)
				net->pingAdr.SetPortHostOrder(net->port);
			net->peer->Ping(net->pingAdr.ToString(false), net->pingAdr.GetPort(), false);
		}
		else
			gui->SimpleDialog(txInvalidIp, gameGui->multiplayer);
	}
}

//=================================================================================================
void Game::EndConnecting(cstring msg, bool wait)
{
	gameGui->infoBox->CloseDialog();
	if(msg)
		gui->SimpleDialog(msg, gameGui->pickServer->visible ? (DialogBox*)gameGui->pickServer : (DialogBox*)gameGui->multiplayer);
	if(wait)
		ForceRedraw();
	if(!gameGui->pickServer->visible)
		net->ClosePeer(wait);
	else
		net->peer->CloseConnection(net->server, true);
}

//=================================================================================================
void Game::GenericInfoBoxUpdate(float dt)
{
	switch(netMode)
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
	if(netState == NetState::Client_PingIp)
	{
		netTimer -= dt;
		if(netTimer <= 0.f)
		{
			if(netTries == 0)
			{
				// connection timeout
				Warn("NM_CONNECTING(0): Connection timeout.");
				EndConnecting(txConnectTimeout);
				return;
			}
			else
			{
				// ping another time...
				Info("Pinging %s...", net->pingAdr.ToString());
				net->peer->Ping(net->pingAdr.ToString(false), net->pingAdr.GetPort(), false);
				--netTries;
				netTimer = T_CONNECT_PING;
			}
		}

		// waiting for connection
		Packet* packet;
		for(packet = net->peer->Receive(); packet; net->peer->DeallocatePacket(packet), packet = net->peer->Receive())
		{
			BitStreamReader f(packet);
			byte msgId;
			f >> msgId;

			if(msgId != ID_UNCONNECTED_PONG)
			{
				// unknown packet from server
				Warn("NM_CONNECTING(0): Unknown server response: %u.", msgId);
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
			TimeMS timeMs;
			char signCa[2];
			f >> timeMs;
			f >> signCa;
			if(!f)
			{
				Warn("NM_CONNECTING(0): Broken server response.");
				continue;
			}
			if(signCa[0] != 'C' || signCa[1] != 'A')
			{
				// invalid signature, this is not carpg server
				Warn("NM_CONNECTING(0): Invalid server signature 0x%x%x.", byte(signCa[0]), byte(signCa[1]));
				net->peer->DeallocatePacket(packet);
				EndConnecting(txConnectInvalid);
				return;
			}

			// read data
			uint version;
			byte players, maxPlayers, flags;
			f >> version;
			f >> players;
			f >> maxPlayers;
			f >> flags;
			const string& serverNameRead = f.ReadString1();
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
			gameGui->server->maxPlayers = maxPlayers;
			gameGui->server->serverName = serverNameRead;
			Info("NM_CONNECTING(0): Server information. Name:%s; players:%d/%d; flags:%d.", serverNameRead.c_str(), players, maxPlayers, flags);
			if(IsSet(flags, 0xFC))
				Warn("NM_CONNECTING(0): Unknown server flags.");
			net->server = packet->systemAddress;
			enterPswd.clear();

			if(IsSet(flags, 0x01))
			{
				// password is required
				netState = NetState::Client_WaitingForPassword;
				gameGui->infoBox->Show(txWaitingForPswd);
				GetTextDialogParams params(Format(txEnterPswd, serverNameRead.c_str()), enterPswd);
				params.event = DialogEvent(this, &Game::OnEnterPassword);
				params.limit = 16;
				params.parent = gameGui->infoBox;
				GetTextDialog::Show(params);
				Info("NM_CONNECTING(0): Waiting for password...");
			}
			else
			{
				// try to connect
				ConnectionAttemptResult result = net->peer->Connect(net->pingAdr.ToString(false), net->pingAdr.GetPort(), nullptr, 0);
				if(result == CONNECTION_ATTEMPT_STARTED)
				{
					// connecting...
					netTimer = T_CONNECT;
					netState = NetState::Client_Connecting;
					gameGui->infoBox->Show(Format(txConnectingTo, serverNameRead.c_str()));
					Info("NM_CONNECTING(0): Connecting to server %s...", serverNameRead.c_str());
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
	else if(netState == NetState::Client_Connecting)
	{
		Packet* packet;
		for(packet = net->peer->Receive(); packet; net->peer->DeallocatePacket(packet), packet = net->peer->Receive())
		{
			// ignore messages from proxy (disconnect notification)
			if(packet->systemAddress != net->server)
				continue;

			BitStreamReader reader(packet);
			byte msgId;
			reader >> msgId;

			switch(msgId)
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
					f << playerName;
					net->SendClient(f, IMMEDIATE_PRIORITY, RELIABLE);
				}
				break;
			case ID_JOIN:
				// server accepted and sent info about players and my id
				{
					int count, loadChar;
					reader.ReadCasted<byte>(team->myId);
					reader.ReadCasted<byte>(net->activePlayers);
					reader.ReadCasted<byte>(team->leaderId);
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
					info.name = playerName;
					info.id = team->myId;
					info.state = PlayerInfo::IN_LOBBY;
					info.updateFlags = 0;
					info.left = PlayerInfo::LEFT_NO;
					info.loaded = false;

					// read other players
					string classId;
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
						reader >> classId;
						reader >> info2.name;
						if(!reader)
						{
							Error("NM_CONNECTING(2): Broken packet ID_JOIN(2).");
							net->peer->DeallocatePacket(packet);
							EndConnecting(txCantJoin, true);
							return;
						}

						// verify player class
						if(classId.empty())
							info2.clas = nullptr;
						else
						{
							info2.clas = Class::TryGet(classId);
							if(!info2.clas || !info2.clas->IsPickable())
							{
								Error("NM_CONNECTING(2): Broken packet ID_JOIN, player '%s' has class '%s'.", info2.name.c_str(), classId.c_str());
								net->peer->DeallocatePacket(packet);
								EndConnecting(txCantJoin, true);
								return;
							}
						}
					}

					// read save information
					reader.ReadCasted<byte>(loadChar);
					if(!reader)
					{
						Error("NM_CONNECTING(2): Broken packet ID_JOIN(4).");
						net->peer->DeallocatePacket(packet);
						EndConnecting(txCantJoin, true);
						return;
					}
					if(loadChar == 2)
					{
						reader >> classId;
						info.clas = Class::TryGet(classId);
						if(!reader)
						{
							Error("NM_CONNECTING(2): Broken packet ID_JOIN(3).");
							net->peer->DeallocatePacket(packet);
							EndConnecting(txCantJoin, true);
							return;
						}
						else if(!info.clas || !info.clas->IsPickable())
						{
							Error("NM_CONNECTING(2): Invalid loaded class '%s'.", classId.c_str());
							net->peer->DeallocatePacket(packet);
							EndConnecting(txCantJoin, true);
							return;
						}
					}

					net->peer->DeallocatePacket(packet);

					// read leader
					if(!net->TryGetPlayer(team->leaderId))
					{
						Error("NM_CONNECTING(2): Broken packet ID_JOIN, no player with leader id %d.", team->leaderId);
						EndConnecting(txCantJoin, true);
						return;
					}

					// go to lobby
					if(gameGui->pickServer->visible)
						gameGui->pickServer->CloseDialog();
					if(gameGui->multiplayer->visible)
						gameGui->multiplayer->CloseDialog();
					gameGui->server->Show();
					if(loadChar != 0)
						gameGui->server->UseLoadedCharacter(loadChar == 2);
					if(loadChar != 2)
						gameGui->server->CheckAutopick();
					gameGui->infoBox->CloseDialog();
					SetTitle("CLIENT");
					return;
				}
				break;
			case ID_CANT_JOIN:
			case ID_NO_FREE_INCOMING_CONNECTIONS:
				// can't join server (full or other reason)
				{
					cstring reason, reasonEng;

					JoinResult result;
					if(msgId == ID_NO_FREE_INCOMING_CONNECTIONS)
						result = JoinResult::FullServer;
					else
						result = (packet->length >= 2 ? (JoinResult)packet->data[1] : JoinResult::OtherError);

					switch(result)
					{
					case JoinResult::FullServer:
						reason = txServerFull;
						reasonEng = "server is full";
						break;
					case JoinResult::InvalidVersion:
						if(packet->length == 6)
						{
							int w;
							memcpy(&w, packet->data + 2, 4);
							cstring s = VersionToString(w);
							reason = Format(txInvalidVersion2, VERSION, s);
							reasonEng = Format("invalid version (%s) vs server (%s)", VERSION, s);
						}
						else
						{
							reason = txInvalidVersion;
							reasonEng = "invalid version";
						}
						break;
					case JoinResult::TakenNick:
						reason = txNickUsed;
						reasonEng = "nick used";
						break;
					case JoinResult::BrokenPacket:
						reason = txInvalidData;
						reasonEng = "invalid data";
						break;
					case JoinResult::InvalidNick:
						reason = txInvalidNick;
						reasonEng = "invalid nick";
						break;
					case JoinResult::InvalidCrc:
						{
							reasonEng = "invalid unknown type crc";
							reason = txInvalidCrc;

							if(packet->length == 7)
							{
								uint serverCrc;
								byte type;
								memcpy(&serverCrc, packet->data + 2, 4);
								memcpy(&type, packet->data + 6, 1);
								uint myCrc;
								cstring typeStr;
								if(content.GetCrc((Content::Id)type, myCrc, typeStr))
									reasonEng = Format("invalid %s crc (%p) vs server (%p)", typeStr, myCrc, serverCrc);
							}
						}
						break;
					case JoinResult::OtherError:
					default:
						reason = nullptr;
						reasonEng = nullptr;
						break;
					}

					net->peer->DeallocatePacket(packet);
					if(reasonEng)
						Warn("NM_CONNECTING(2): Can't connect to server: %s.", reasonEng);
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
				Warn(msgId == ID_CONNECTION_LOST ? "NM_CONNECTING(2): Lost connection with server." : "NM_CONNECTING(2): Disconnected from server.");
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
				Warn("NM_CONNECTING(2): Unknown packet from server %u.", msgId);
				break;
			}
		}

		netTimer -= dt;
		if(netTimer <= 0.f)
		{
			Warn("NM_CONNECTING(2): Connection timeout.");
			EndConnecting(txConnectTimeout);
		}
	}
	else if(netState == NetState::Client_ConnectingProxy || netState == NetState::Client_Punchthrough)
	{
		Packet* packet;
		for(packet = net->peer->Receive(); packet; net->peer->DeallocatePacket(packet), packet = net->peer->Receive())
		{
			BitStreamReader reader(packet);
			byte msgId;
			reader >> msgId;

			switch(msgId)
			{
			case ID_CONNECTION_REQUEST_ACCEPTED:
				{
					Info("NM_CONNECTING(3): Connected with proxy, starting nat punchthrough.");
					netTimer = 10.f;
					netState = NetState::Client_Punchthrough;
					PickServerPanel::ServerData& info = gameGui->pickServer->servers[gameGui->pickServer->grid.selected];
					RakNetGUID guid;
					guid.FromString(info.guid.c_str());
					net->masterServerAdr = packet->systemAddress;
					api->StartPunchthrough(&guid);
				}
				break;
			case ID_NAT_TARGET_NOT_CONNECTED:
			case ID_NAT_TARGET_UNRESPONSIVE:
			case ID_NAT_CONNECTION_TO_TARGET_LOST:
			case ID_NAT_PUNCHTHROUGH_FAILED:
				Warn("NM_CONNECTING(3): Punchthrough failed (%d).", msgId);
				api->EndPunchthrough();
				EndConnecting(txConnectionFailed);
				break;
			case ID_NAT_PUNCHTHROUGH_SUCCEEDED:
				{
					Info("NM_CONNECTING(3): Punchthrough succeeded, connecting to server %s.", packet->systemAddress.ToString());
					api->EndPunchthrough();
					net->peer->CloseConnection(net->masterServerAdr, true, 1, IMMEDIATE_PRIORITY);
					PickServerPanel::ServerData& info = gameGui->pickServer->servers[gameGui->pickServer->grid.selected];
					net->server = packet->systemAddress;
					gameGui->server->serverName = info.name;
					gameGui->server->maxPlayers = info.maxPlayers;
					net->activePlayers = info.activePlayers;
					ConnectionAttemptResult result = net->peer->Connect(net->server.ToString(false), net->server.GetPort(), enterPswd.c_str(), enterPswd.length());
					if(result == CONNECTION_ATTEMPT_STARTED)
					{
						netMode = NM_CONNECTING;
						netTimer = T_CONNECT;
						netTries = 1;
						netState = NetState::Client_Connecting;
						gameGui->infoBox->Show(Format(txConnectingTo, info.name.c_str()));
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
				Warn("NM_CONNECTING(3): Unknown packet from proxy %u.", msgId);
				break;
			}
		}

		netTimer -= dt;
		if(netTimer <= 0.f)
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
		byte msgId;
		reader >> msgId;

		switch(msgId)
		{
		case ID_DISCONNECTION_NOTIFICATION:
		case ID_CONNECTION_LOST:
			Info("NM_TRANSFER: Lost connection with server.");
			net->peer->DeallocatePacket(packet);
			net->ClosePeer(false, true);
			gameGui->infoBox->CloseDialog();
			ExitToMenu();
			gui->SimpleDialog(txLostConnection, nullptr);
			return;
		case ID_SERVER_CLOSE:
			Info("NM_TRANSFER: Server close, failed to load game.");
			net->peer->DeallocatePacket(packet);
			net->ClosePeer(true, true);
			gameGui->infoBox->CloseDialog();
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
						gameGui->infoBox->Show(txGeneratingWorld);
						break;
					case 1:
						gameGui->infoBox->Show(txPreparingWorld);
						break;
					case 2:
						gameGui->infoBox->Show(txWaitingForPlayers);
						break;
					}
				}
				else
					Error("NM_TRANSFER: Broken packet ID_STATE.");
			}
			break;
		case ID_WORLD_DATA:
			if(netState == NetState::Client_BeforeTransfer)
			{
				Info("NM_TRANSFER: Received world data.");

				LoadingStep("");
				ClearGameVars(true);
				net->OnNewGameClient();

				fallbackType = FALLBACK::NONE;
				fallbackTimer = -0.5f;
				netState = NetState::Client_ReceivedWorldData;

				if(net->ReadWorldData(reader))
				{
					// send ready message
					BitStreamWriter f;
					f << ID_READY;
					f << (byte)0;
					net->SendClient(f, HIGH_PRIORITY, RELIABLE);
					gameGui->infoBox->Show(txLoadedWorld);
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
				Error("NM_TRANSFER: Received ID_WORLD_DATA with net state %d.", netState);
			break;
		case ID_PLAYER_START_DATA:
			if(netState == NetState::Client_ReceivedWorldData)
			{
				netState = NetState::Client_ReceivedPlayerStartData;
				LoadingStep("");
				if(net->ReadPlayerStartData(reader))
				{
					// send ready message
					if(net->mpLoadWorldmap)
						LoadResources("", true);
					else
						LoadingStep("");
					BitStreamWriter f;
					f << ID_READY;
					f << (byte)1;
					net->SendClient(f, HIGH_PRIORITY, RELIABLE);
					gameGui->infoBox->Show(txLoadedPlayer);
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
				Error("NM_TRANSFER: Received ID_PLAYER_START_DATA with net state %d.", netState);
			break;
		case ID_CHANGE_LEVEL:
			if(netState == NetState::Client_ReceivedPlayerStartData)
			{
				netState = NetState::Client_ChangingLevel;
				byte loc, level;
				bool encounter;
				reader >> loc;
				reader >> level;
				reader >> encounter;
				if(reader.IsOk())
				{
					if(world->VerifyLocation(loc))
					{
						if(gameState == GS_LOAD)
							LoadingStep("");
						else
							LoadingStart(4);
						world->ChangeLevel(loc, encounter);
						gameLevel->dungeonLevel = level;
						Info("NM_TRANSFER: Level change to %s (id:%d, level:%d).", gameLevel->location->name.c_str(), gameLevel->locationIndex, gameLevel->dungeonLevel);
						gameGui->infoBox->Show(txGeneratingLocation);
					}
					else
						Error("NM_TRANSFER: Broken packet ID_CHANGE_LEVEL, invalid location %u.", loc);
				}
				else
					Error("NM_TRANSFER: Broken packet ID_CHANGE_LEVEL.");
			}
			else
				Error("NM_TRANSFER: Received ID_CHANGE_LEVEL with net state %d.", netState);
			break;
		case ID_LEVEL_DATA:
			if(netState == NetState::Client_ChangingLevel)
			{
				netState = NetState::Client_ReceivedLevelData;
				gameGui->infoBox->Show(txLoadingLocation);
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
				Error("NM_TRANSFER: Received ID_LEVEL_DATA with net state %d.", netState);
			break;
		case ID_PLAYER_DATA:
			if(netState == NetState::Client_ReceivedLevelData)
			{
				netState = NetState::Client_ReceivedPlayerData;
				gameGui->infoBox->Show(txLoadingChars);
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
					net->mpLoad = false;
					BitStreamWriter f;
					f << ID_READY;
					f << (byte)3;
					net->SendClient(f, HIGH_PRIORITY, RELIABLE);
				}
			}
			else
				Error("NM_TRANSFER: Received ID_PLAYER_DATA with net state %d.", netState);
			break;
		case ID_START:
			if(net->mpLoadWorldmap)
			{
				// start on worldmap
				if(netState == NetState::Client_ReceivedPlayerStartData)
				{
					net->mpLoadWorldmap = false;
					netState = NetState::Client_StartOnWorldmap;
					Info("NM_TRANSFER: Starting at world map.");
					gameState = GS_WORLDMAP;
					gameLevel->ready = false;
					gameGui->loadScreen->visible = false;
					gameGui->mainMenu->visible = false;
					gameGui->levelGui->visible = false;
					gameGui->worldMap->Show();
					world->SetState(World::State::ON_MAP);
					gameGui->infoBox->CloseDialog();
					net->updateTimer = 0.f;
					team->leaderId = 0;
					team->leader = nullptr;
					SetMusic(MusicType::Travel);
					SetTitle("CLIENT");
					if(net->mpQuickload)
					{
						net->mpQuickload = false;
						gameGui->mpBox->Add(txGameLoaded);
						gameGui->messages->AddGameMsg3(GMS_GAME_LOADED);
					}
					net->peer->DeallocatePacket(packet);
				}
				else
				{
					Error("NM_TRANSFER: Received ID_START at worldmap with net state %d.", netState);
					break;
				}
			}
			else
			{
				// start in location
				if(netState == NetState::Client_ReceivedPlayerData)
				{
					netState = NetState::Client_Start;
					Info("NM_TRANSFER: Level started.");
					gameState = GS_LEVEL;
					gameLevel->ready = true;
					if(gameLevel->location->state != LS_HIDDEN)
						gameLevel->location->state = LS_VISITED;
					gameGui->loadScreen->visible = false;
					gameGui->mainMenu->visible = false;
					gameGui->levelGui->visible = true;
					gameGui->worldMap->Hide();
					gameGui->infoBox->CloseDialog();
					net->updateTimer = 0.f;
					fallbackType = FALLBACK::NONE;
					fallbackTimer = -0.5f;
					gameLevel->camera.Reset();
					pc->data.rotBuf = 0.f;
					SetTitle("CLIENT");
					if(net->mpQuickload)
					{
						net->mpQuickload = false;
						gameGui->mpBox->Add(txGameLoaded);
						gameGui->messages->AddGameMsg3(GMS_GAME_LOADED);
					}
					net->peer->DeallocatePacket(packet);
					gameGui->Setup(pc);
					OnEnterLevelOrLocation();
				}
				else
					Error("NM_TRANSFER: Received ID_START with net state %d.", netState);
			}
			return;
		default:
			Warn("NM_TRANSFER: Unknown packet %u.", msgId);
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
		byte msgId;
		f >> msgId;

		if(msgId == ID_DISCONNECTION_NOTIFICATION || msgId == ID_CONNECTION_LOST)
		{
			Info("NM_QUITTING: Server accepted disconnection.");
			net->peer->DeallocatePacket(packet);
			net->ClosePeer();
			if(gameGui->server->visible)
				gameGui->server->CloseDialog();
			if(netCallback)
				netCallback();
			gameGui->infoBox->CloseDialog();
			return;
		}
		else
			Info("NM_QUITTING: Ignored packet %u.", msgId);
	}

	netTimer -= dt;
	if(netTimer <= 0.f)
	{
		Warn("NM_QUITTING: Disconnected without accepting.");
		net->peer->DeallocatePacket(packet);
		net->ClosePeer();
		if(gameGui->server->visible)
			gameGui->server->CloseDialog();
		if(netCallback)
			netCallback();
		gameGui->infoBox->CloseDialog();
	}
}

void Game::UpdateServerTransfer(float dt)
{
	Packet* packet;
	for(packet = net->peer->Receive(); packet; net->peer->DeallocatePacket(packet), packet = net->peer->Receive())
	{
		BitStreamReader f(packet);

		PlayerInfo* ptrInfo = net->FindPlayer(packet->systemAddress);
		if(!ptrInfo)
		{
			Info("NM_TRANSFER_SERVER: Ignoring packet from %s.", packet->systemAddress.ToString());
			continue;
		}
		if(ptrInfo->left != PlayerInfo::LEFT_NO)
		{
			Info("NM_TRANSFER_SERVER: Packet from %s who left game.", ptrInfo->name.c_str());
			continue;
		}

		PlayerInfo& info = *ptrInfo;
		byte msgId;
		f >> msgId;

		switch(msgId)
		{
		case ID_DISCONNECTION_NOTIFICATION:
		case ID_CONNECTION_LOST:
			{
				Info("NM_TRANSFER_SERVER: Player %s left game.", info.name.c_str());
				--net->activePlayers;
				net->playersLeft = true;
				info.left = (msgId == ID_CONNECTION_LOST ? PlayerInfo::LEFT_DISCONNECTED : PlayerInfo::LEFT_QUIT);
			}
			break;
		case ID_READY:
			{
				if(netState != NetState::Server_WaitForPlayersToLoadWorld)
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
			Warn("NM_TRANSFER_SERVER: Unknown packet from %s: %u.", info.name.c_str(), msgId);
			break;
		}
	}

	if(netState == NetState::Server_Starting)
	{
		// create team
		vector<Unit*> prevTeam;
		if(net->mpLoad)
			prevTeam = team->members.ptrs;
		else
			ClearGameVars(true);
		team->members.clear();
		team->activeMembers.clear();
		gameLevel->ready = false;
		const bool inLevel = gameLevel->isOpen;
		int leaderPerk = 0;
		for(PlayerInfo& info : net->players)
		{
			if(info.left != PlayerInfo::LEFT_NO)
				continue;

			Unit* u;
			if(!info.loaded)
			{
				u = gameLevel->CreateUnit(*info.clas->player, -1, inLevel);
				u->locPart = nullptr;
				u->ApplyHumanData(info.hd);
				u->meshInst->needUpdate = true;
				info.u = u;

				u->player = new PlayerController;
				u->player->id = info.id;
				u->player->name = info.name;
				u->player->Init(*u, &info.cc);

				if(info.cc.HavePerk(Perk::leader))
					++leaderPerk;

				if(net->mpLoad)
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
			team->activeMembers.push_back(u);

			info.pc = u->player;
			u->player->playerInfo = &info;
			if(info.id == team->myId)
			{
				pc = u->player;
				u->player->dialogCtx = &dialogContext;
				u->player->dialogCtx->isLocal = true;
				u->player->isLocal = true;
				gameGui->ability->Refresh();
				gameLevel->camera.target = u;
			}
			else
			{
				u->player->dialogCtx = new DialogContext;
				u->player->dialogCtx->isLocal = false;
			}
			u->player->dialogCtx->dialogMode = false;
			u->player->dialogCtx->pc = u->player;
			u->interp = EntityInterpolator::Pool.Get();
			u->interp->Reset(u->pos, u->rot);
		}
		team->CalculatePlayersLevel();

		// add ai
		bool anyoneLeft = false;
		for(Unit* unit : prevTeam)
		{
			if(unit->IsPlayer())
				continue;

			if(unit->hero->type != HeroType::Normal)
				team->members.push_back(unit);
			else
			{
				if(team->activeMembers.size() < team->GetMaxSize())
				{
					team->members.push_back(unit);
					team->activeMembers.push_back(unit);
				}
				else
				{
					// too many team members, kick ai
					NetChange& c = Net::PushChange(NetChange::HERO_LEAVE);
					c.unit = unit;

					gameGui->mpBox->Add(Format(txMpNPCLeft, unit->hero->name.c_str()));
					if(gameLevel->IsSafeSettlement())
						unit->OrderWander();
					else
						unit->OrderLeave();
					unit->hero->teamMember = false;
					unit->hero->credit = 0;
					unit->ai->cityWander = false;
					unit->ai->locTimer = Random(5.f, 10.f);
					unit->MakeItemsTeam(true);
					unit->temporary = true;

					anyoneLeft = true;
				}
			}
		}

		if(!net->mpLoad && leaderPerk > 0 && team->GetActiveTeamSize() < team->GetMaxSize())
		{
			UnitData& ud = *Class::GetRandomHeroData();
			int level = ud.level.x + 2 * (leaderPerk - 1);
			Unit* npc = gameLevel->CreateUnit(ud, level, false);
			npc->locPart = nullptr;
			npc->ai = new AIController;
			npc->ai->Init(npc);
			npc->hero->knowName = true;
			team->AddMember(npc, HeroType::Normal);
			npc->hero->SetupMelee();
		}

		// recalculate credit if someone left
		if(anyoneLeft)
			team->CheckCredit(false, true);

		// set leader
		PlayerInfo* leaderInfo = net->TryGetPlayer(team->leaderId);
		if(leaderInfo)
			team->leader = leaderInfo->u;
		else
		{
			team->leaderId = 0;
			team->leader = net->GetMe().u;
		}

		if(!net->mpLoad)
		{
			gameGui->infoBox->Show(txGeneratingWorld);

			// send info about generating world
			if(net->activePlayers > 1)
			{
				byte b[] = { ID_STATE, 0 };
				net->peer->Send((cstring)b, 2, IMMEDIATE_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
			}

			// do it
			team->freeRecruits = 3 - net->activePlayers;
			if(team->freeRecruits < 0)
				team->freeRecruits = 0;
			fallbackType = FALLBACK::NONE;
			fallbackTimer = -0.5f;
			gameGui->mainMenu->visible = false;
			gameGui->loadScreen->visible = true;
			net->prepareWorld = true;
			GenerateWorld();
			questMgr->InitQuests();
			world->StartInLocation();
			net->prepareWorld = false;
			gameRes->LoadCommonMusic();
		}

		netState = NetState::Server_Initialized;
	}
	else if(netState == NetState::Server_Initialized)
	{
		// prepare world data if there is any players
		if(net->activePlayers > 1)
		{
			gameGui->infoBox->Show(txSendingWorld);

			// send info about preparing world data
			byte b[] = { ID_STATE, 1 };
			net->peer->Send((cstring)b, 2, IMMEDIATE_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);

			// prepare & send world data
			BitStreamWriter f;
			net->WriteWorldData(f);
			net->SendAll(f, IMMEDIATE_PRIORITY, RELIABLE);
			Info("NM_TRANSFER_SERVER: Send world data, size %d.", f.GetSize());
			netState = NetState::Server_WaitForPlayersToLoadWorld;
			netTimer = mpTimeout;
			for(PlayerInfo& info : net->players)
				info.ready = (info.id == team->myId);
			gameGui->infoBox->Show(txWaitingForPlayers);
		}
		else
			netState = NetState::Server_EnterLocation;

		// send info
		if(net->activePlayers > 1)
		{
			byte b[] = { ID_STATE, 2 };
			net->peer->Send((cstring)b, 2, IMMEDIATE_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
		}
	}
	else if(netState == NetState::Server_WaitForPlayersToLoadWorld)
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

		netTimer -= dt;
		if(!ok && netTimer <= 0.f)
		{
			bool anyoneRemoved = false;
			for(PlayerInfo& info : net->players)
			{
				if(info.left != PlayerInfo::LEFT_NO)
					continue;
				if(!info.ready)
				{
					Info("NM_TRANSFER_SERVER: Disconnecting player %s due no response.", info.name.c_str());
					--net->activePlayers;
					net->playersLeft = true;
					info.left = PlayerInfo::LEFT_TIMEOUT;
					anyoneRemoved = true;
				}
			}

			ok = true;
			if(anyoneRemoved)
			{
				team->CheckCredit(false, true);
				if(team->leaderId == -1)
				{
					team->leaderId = 0;
					team->leader = net->GetMe().u;
				}
			}
		}

		if(ok)
		{
			Info("NM_TRANSFER_SERVER: All players ready.");
			netState = NetState::Server_EnterLocation;
		}
	}
	else if(netState == NetState::Server_EnterLocation)
	{
		// enter location
		gameGui->infoBox->Show(txLoadingLevel);
		if(!net->mpLoad)
			EnterLocation();
		else
		{
			if(!gameLevel->isOpen)
			{
				// saved on worldmap
				byte b = ID_START;
				net->peer->Send((cstring)&b, 1, IMMEDIATE_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);

				net->DeleteOldPlayers();

				gameState = GS_WORLDMAP;
				gameLevel->ready = false;
				gameGui->loadScreen->visible = false;
				gameGui->worldMap->Show();
				gameGui->levelGui->visible = false;
				gameGui->mainMenu->visible = false;
				net->mpLoad = false;
				world->SetState(World::State::ON_MAP);
				net->updateTimer = 0.f;
				SetMusic(MusicType::Travel);
				net->ProcessLeftPlayers();
				if(net->mpQuickload)
				{
					net->mpQuickload = false;
					gameGui->mpBox->Add(txGameLoaded);
					gameGui->messages->AddGameMsg3(GMS_GAME_LOADED);
				}
				gameGui->infoBox->CloseDialog();
			}
			else
			{
				if(!net->mpQuickload)
					LoadingStart(1);

				BitStreamWriter f;
				f << ID_CHANGE_LEVEL;
				f << (byte)world->GetCurrentLocationIndex();
				f << (byte)gameLevel->dungeonLevel;
				f << (world->GetState() == World::State::INSIDE_ENCOUNTER);
				int ack = net->SendAll(f, HIGH_PRIORITY, RELIABLE_WITH_ACK_RECEIPT);
				for(PlayerInfo& info : net->players)
				{
					if(info.id == team->myId)
						info.state = PlayerInfo::IN_GAME;
					else
					{
						info.state = PlayerInfo::WAITING_FOR_RESPONSE;
						info.ack = ack;
						info.timer = mpTimeout;
					}
				}

				// find player that was in save and is playing now (first check leader)
				Unit* centerUnit = nullptr;
				if(team->leader->player->playerInfo->loaded)
					centerUnit = team->leader;
				else
				{
					// anyone
					for(PlayerInfo& info : net->players)
					{
						if(info.loaded)
						{
							centerUnit = info.u;
							break;
						}
					}
				}

				// set position of new units that didn't exists in save (warp to old unit or level entrance)
				if(centerUnit)
				{
					// get positon of unit or building entrance
					Vec3 pos;
					if(centerUnit->locPart->partType == LocationPart::Type::Building)
					{
						InsideBuilding* inside = static_cast<InsideBuilding*>(centerUnit->locPart);
						pos = inside->insideSpawn;
					}
					else
						pos = centerUnit->pos;

					// warp
					for(PlayerInfo& info : net->players)
					{
						if(!info.loaded)
						{
							gameLevel->localPart->units.push_back(info.u);
							info.u->locPart = gameLevel->localPart;
							gameLevel->WarpNearLocation(*gameLevel->localPart, *info.u, pos, 4.f, false, 20);
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

					if(gameLevel->cityCtx)
						gameLevel->cityCtx->GetEntry(pos, rot);
					else if(gameLevel->enterFrom >= ENTER_FROM_PORTAL && (portal = gameLevel->location->GetPortal(gameLevel->enterFrom)) != nullptr)
					{
						pos = portal->pos + Vec3(sin(portal->rot) * 2, 0, cos(portal->rot) * 2);
						rot = Clip(portal->rot + PI);
					}
					else if(gameLevel->location->type == L_DUNGEON)
					{
						InsideLocation* inside = static_cast<InsideLocation*>(gameLevel->location);
						InsideLocationLevel& lvl = inside->GetLevelData();
						if(gameLevel->enterFrom == ENTER_FROM_NEXT_LEVEL)
						{
							pos = PtToPos(lvl.GetNextEntryFrontTile());
							rot = DirToRot(lvl.nextEntryDir);
						}
						else
						{
							pos = PtToPos(lvl.GetPrevEntryFrontTile());
							rot = DirToRot(lvl.prevEntryDir);
						}
					}
					else
						world->GetOutsideSpawnPoint(pos, rot);

					// warp
					for(PlayerInfo& info : net->players)
					{
						if(!info.loaded)
						{
							gameLevel->localPart->units.push_back(info.u);
							info.u->locPart = gameLevel->localPart;
							gameLevel->WarpNearLocation(*gameLevel->localPart, *info.u, pos, gameLevel->location->outside ? 4.f : 2.f, false, 20);
							info.u->rot = rot;
							info.u->interp->Reset(info.u->pos, info.u->rot);
						}
					}
				}

				net->DeleteOldPlayers();
				if(!net->mpQuickload)
					LoadResources("", false, false);

				netMode = NM_SERVER_SEND;
				netState = NetState::Server_Send;
				if(net->activePlayers > 1)
				{
					preparedStream.Reset();
					BitStreamWriter f(preparedStream);
					net->WriteLevelData(f, false);
					Info("NM_TRANSFER_SERVER: Generated level packet: %d.", preparedStream.GetNumberOfBytesUsed());
					gameGui->infoBox->Show(txWaitingForPlayers);
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
		PlayerInfo* ptrInfo = net->FindPlayer(packet->systemAddress);
		if(!ptrInfo)
		{
			Info("NM_SERVER_SEND: Ignoring packet from %s.", packet->systemAddress.ToString());
			continue;
		}
		else if(ptrInfo->left != PlayerInfo::LEFT_NO)
		{
			Info("NM_SERVER_SEND: Packet from %s who left game.", ptrInfo->name.c_str());
			continue;
		}

		PlayerInfo& info = *ptrInfo;
		byte msgId;
		f >> msgId;

		switch(packet->data[0])
		{
		case ID_DISCONNECTION_NOTIFICATION:
		case ID_CONNECTION_LOST:
			{
				Info("NM_SERVER_SEND: Player %s left game.", info.name.c_str());
				--net->activePlayers;
				net->playersLeft = true;
				info.left = (msgId == ID_CONNECTION_LOST ? PlayerInfo::LEFT_DISCONNECTED : PlayerInfo::LEFT_QUIT);
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
					net->peer->Send(&preparedStream, HIGH_PRIORITY, RELIABLE, 0, packet->systemAddress, false);
					info.timer = mpTimeout;
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
		case ID_PROGRESS:
			if(info.state == PlayerInfo::WAITING_FOR_DATA2 && info.left == PlayerInfo::LEFT_NO)
			{
				info.timer = 5.f;
				Info("NM_SERVER_SEND: Update progress from %s.", info.name.c_str());
			}
			break;
		case ID_CONTROL:
			break;
		default:
			Warn("MN_SERVER_SEND: Invalid packet %d from %s.", msgId, info.name.c_str());
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
				--net->activePlayers;
				net->playersLeft = true;
				info.left = PlayerInfo::LEFT_TIMEOUT;
			}
			else
				ok = false;
		}
	}
	if(ok)
	{
		if(net->activePlayers > 1)
		{
			byte b = ID_START;
			net->peer->Send((cstring)&b, 1, HIGH_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
		}
		for(LocationPart& locPart : gameLevel->ForEachPart())
		{
			for(Unit* unit : locPart.units)
				unit->changed = false;
		}
		Info("NM_SERVER_SEND: All players ready. Starting game.");
		gameState = GS_LEVEL;
		gameLevel->ready = true;
		gameGui->infoBox->CloseDialog();
		gameGui->loadScreen->visible = false;
		gameGui->mainMenu->visible = false;
		gameGui->levelGui->visible = true;
		gameGui->worldMap->Hide();
		net->mpLoad = false;
		SetMusic();
		gameGui->Setup(pc);
		net->ProcessLeftPlayers();
		if(net->mpQuickload)
		{
			net->mpQuickload = false;
			gameGui->mpBox->Add(txGameLoaded);
			gameGui->messages->AddGameMsg3(GMS_GAME_LOADED);
		}
	}
}

void Game::UpdateServerQuiting(float dt)
{
	Packet* packet;
	for(packet = net->peer->Receive(); packet; net->peer->DeallocatePacket(packet), packet = net->peer->Receive())
	{
		BitStreamReader f(packet);
		PlayerInfo* ptrInfo = net->FindPlayer(packet->systemAddress);
		if(!ptrInfo)
			Warn("NM_QUITTING_SERVER: Ignoring packet from unconnected player %s.", packet->systemAddress.ToString());
		else
		{
			PlayerInfo& info = *ptrInfo;
			byte msgId;
			f >> msgId;
			if(msgId == ID_DISCONNECTION_NOTIFICATION || msgId == ID_CONNECTION_LOST)
			{
				if(info.state == PlayerInfo::IN_LOBBY)
					Info("NM_QUITTING_SERVER: Player %s left lobby.", info.name.c_str());
				else
					Info("NM_QUITTING_SERVER: %s left lobby.", packet->systemAddress.ToString());
				--net->activePlayers;
				if(net->activePlayers == 0)
				{
					net->peer->DeallocatePacket(packet);
					Info("NM_QUITTING_SERVER: All players disconnected from server. Closing...");
					net->ClosePeer();
					game->SetTitle(nullptr);
					gameGui->infoBox->CloseDialog();
					if(gameGui->server->visible)
						gameGui->server->CloseDialog();
					if(netCallback)
						netCallback();
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

	netTimer -= dt;
	if(netTimer <= 0.f)
	{
		Warn("NM_QUITTING_SERVER: Not all players disconnected on time. Closing server...");
		net->ClosePeer(net->masterServerState >= MasterServerState::Connecting);
		game->SetTitle(nullptr);
		gameGui->infoBox->CloseDialog();
		if(gameGui->server->visible)
			gameGui->server->CloseDialog();
		if(netCallback)
			netCallback();
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
	if(netState == NetState::Client_WaitingForPasswordProxy)
	{
		ConnectionAttemptResult result = net->peer->Connect(api->GetApiUrl(), api->GetProxyPort(), nullptr, 0);
		if(result == CONNECTION_ATTEMPT_STARTED)
		{
			netState = NetState::Client_ConnectingProxy;
			netTimer = T_CONNECT;
			gameGui->infoBox->Show(txConnectingProxy);
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
		ConnectionAttemptResult result = net->peer->Connect(net->pingAdr.ToString(false), net->pingAdr.GetPort(), enterPswd.c_str(), enterPswd.length());
		if(result == CONNECTION_ATTEMPT_STARTED)
		{
			netState = NetState::Client_Connecting;
			netTimer = T_CONNECT;
			gameGui->infoBox->Show(Format(txConnectingTo, gameGui->server->serverName.c_str()));
			Info("NM_CONNECTING(1): Connecting to server %s...", gameGui->server->serverName.c_str());
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
	if(adr.FromString(serverIp.c_str()) && adr != UNASSIGNED_SYSTEM_ADDRESS)
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

		Info("Pinging %s...", serverIp.c_str());
		Net::SetMode(Net::Mode::Client);
		gameGui->infoBox->Show(txConnecting);
		netMode = NM_CONNECTING;
		netState = NetState::Client_PingIp;
		netTimer = T_CONNECT_PING;
		netTries = I_CONNECT_TRIES;
		if(IsDebug())
			netTries *= 2;
		net->pingAdr = adr;
		net->peer->Ping(net->pingAdr.ToString(false), net->pingAdr.GetPort(), false);
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
	prevGameState = gameState;
	gameState = GS_QUIT;
	gameLevel->ready = false;
}

void Game::CloseConnection(VoidF f)
{
	if(gameGui->infoBox->visible)
	{
		switch(netMode)
		{
		case NM_QUITTING:
		case NM_QUITTING_SERVER:
			if(!netCallback)
				netCallback = f;
			break;
		case NM_CONNECTING:
		case NM_TRANSFER:
			gameGui->infoBox->Show(txDisconnecting);
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

			if(net->activePlayers > 1)
			{
				// disconnect players
				Info("ServerPanel: Disconnecting clients.");
				const byte b[] = { ID_SERVER_CLOSE, ServerClose_Closing };
				net->peer->Send((cstring)b, 2, IMMEDIATE_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
				netMode = Game::NM_QUITTING_SERVER;
				--net->activePlayers;
				netTimer = T_WAIT_FOR_DISCONNECT;
				gameGui->infoBox->Show(txDisconnecting);
				netCallback = f;
			}
			else
			{
				// no players, close instantly
				gameGui->infoBox->CloseDialog();
				net->ClosePeer(false, true);
				f();
			}
			break;
		}
	}
	else if(gameGui->server->visible)
		gameGui->server->ExitLobby(f);
	else
	{
		if(Net::IsServer())
		{
			Info("ServerPanel: Closing server.");

			// disallow new connections
			net->peer->SetMaximumIncomingConnections(0);
			net->peer->SetOfflinePingResponse(nullptr, 0);

			if(net->activePlayers > 1)
			{
				// disconnect players
				Info("ServerPanel: Disconnecting clients.");
				const byte b[] = { ID_SERVER_CLOSE, ServerClose_Closing };
				net->peer->Send((cstring)b, 2, IMMEDIATE_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
				netMode = Game::NM_QUITTING_SERVER;
				--net->activePlayers;
				netTimer = T_WAIT_FOR_DISCONNECT;
				gameGui->infoBox->Show(txDisconnecting);
				netCallback = f;
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
			gameGui->infoBox->Show(txDisconnecting);
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
		gameGui->server->bts[1].state = Button::NONE;
		gameGui->server->bts[0].text = gameGui->server->txChangeChar;
		// set data
		Class* oldClass = info.clas;
		info.clas = gameGui->createCharacter->clas;
		info.hd.Get(*gameGui->createCharacter->unit->humanData);
		info.cc = gameGui->createCharacter->cc;
		// send info to other active players about changing my class
		if(Net::IsServer())
		{
			if(info.clas != oldClass && net->activePlayers > 1)
				gameGui->server->AddLobbyUpdate(Int2(Lobby_UpdatePlayer, 0));
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
		if(!skipTutorial)
		{
			DialogInfo info;
			info.event = DialogEvent(this, &Game::OnPlayTutorial);
			info.haveTick = true;
			info.name = "tutorial_dialog";
			info.order = DialogOrder::Top;
			info.parent = nullptr;
			info.pause = false;
			info.text = txTutPlay;
			info.tickText = txTutTick;
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
		skipTutorial = true;
	else if(id == BUTTON_UNCHECKED)
		skipTutorial = false;
	else
	{
		gui->GetDialog("tutorial_dialog")->visible = false;
		SaveOptions();
		if(id == BUTTON_YES)
			questMgr->questTutorial->Start();
		else
			StartNewGame();
	}
}

void Game::OnPickServer(int id)
{
	enterPswd.clear();

	if(!gameGui->pickServer->IsLAN())
	{
		// connect to proxy server for nat punchthrough
		PickServerPanel::ServerData& info = gameGui->pickServer->servers[gameGui->pickServer->grid.selected];
		if(IsSet(info.flags, SERVER_PASSWORD))
		{
			netMode = NM_CONNECTING;
			netState = NetState::Client_WaitingForPasswordProxy;
			netTries = 1;
			gameGui->infoBox->Show(txWaitingForPswd);
			GetTextDialogParams params(Format(txEnterPswd, gameGui->server->serverName.c_str()), enterPswd);
			params.event = DialogEvent(this, &Game::OnEnterPassword);
			params.limit = 16;
			params.parent = gameGui->infoBox;
			GetTextDialog::Show(params);
			Info("OnPickServer: Waiting for password...");
		}
		else
		{
			ConnectionAttemptResult result = net->peer->Connect(api->GetApiUrl(), api->GetProxyPort(), nullptr, 0);
			if(result == CONNECTION_ATTEMPT_STARTED)
			{
				netMode = NM_CONNECTING;
				netState = NetState::Client_ConnectingProxy;
				netTimer = T_CONNECT;
				gameGui->infoBox->Show(txConnectingProxy);
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
		PickServerPanel::ServerData& info = gameGui->pickServer->servers[gameGui->pickServer->grid.selected];
		net->server = info.adr;
		gameGui->server->serverName = info.name;
		gameGui->server->maxPlayers = info.maxPlayers;
		net->activePlayers = info.activePlayers;
		if(IsSet(info.flags, SERVER_PASSWORD))
		{
			// enter password
			netMode = NM_CONNECTING;
			netState = NetState::Client_WaitingForPassword;
			netTries = 1;
			net->pingAdr = info.adr;
			gameGui->infoBox->Show(txWaitingForPswd);
			GetTextDialogParams params(Format(txEnterPswd, gameGui->server->serverName.c_str()), enterPswd);
			params.event = DialogEvent(this, &Game::OnEnterPassword);
			params.limit = 16;
			params.parent = gameGui->infoBox;
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
				netMode = NM_CONNECTING;
				netTimer = T_CONNECT;
				netTries = 1;
				netState = NetState::Client_Connecting;
				gameGui->infoBox->Show(Format(txConnectingTo, info.name.c_str()));
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
	gui->SimpleDialog(msg, gameGui->mainMenu);
}

void Game::OnLoadProgress(float progress, cstring str)
{
	gameGui->loadScreen->SetProgressOptional(progress, str);
	if(progress >= 0.5f)
		utility::IncrementDelayLock();
	if(Net::IsClient() && loadingResources)
	{
		loadingDt += loadingTimer.Tick();
		if(loadingDt >= 2.5f)
		{
			BitStreamWriter f;
			f << ID_PROGRESS;
			net->SendClient(f, IMMEDIATE_PRIORITY, RELIABLE);
			loadingDt = 0;
		}
	}
}
