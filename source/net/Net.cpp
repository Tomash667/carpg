#include "Pch.h"
#include "GameCore.h"
#include "Net.h"
#include "LobbyApi.h"
#include "Language.h"
#include "Game.h"
#include "ResourceManager.h"
#include "PlayerInfo.h"
#include "Team.h"
#include "GameGui.h"
#include "Notifications.h"
#include "ServerPanel.h"
#include "GameMessages.h"
#include "MpBox.h"
#include "PacketLogger.h"

Net* global::net;
vector<NetChange> Net::changes;
Net::Mode Net::mode;
const int CLOSE_PEER_TIMER = 1000; // ms

//=================================================================================================
Net::Net() : peer(nullptr), packet_logger(nullptr), mp_load(false), mp_use_interp(true), mp_interp(0.05f), was_client(false)
{
}

//=================================================================================================
Net::~Net()
{
	DeleteElements(players);
	DeleteElements(old_players);
	if(peer)
		RakPeerInterface::DestroyInstance(peer);
	delete api;
	delete packet_logger;
}

//=================================================================================================
void Net::Init()
{
	api = new LobbyApi;
	api->Init(game->cfg);
}

//=================================================================================================
void Net::LoadLanguage()
{
	txCreateServerFailed = Str("createServerFailed");
	txInitConnectionFailed = Str("initConnectionFailed");
	txFastTravelText = Str("fastTravelText");
	txFastTravelWaiting = Str("fastTravelWaiting");
	txFastTravelCancel = Str("fastTravelCancel");
	txFastTravelDeny = Str("fastTravelDeny");
}

//=================================================================================================
void Net::LoadData()
{
	tFastTravel = res_mgr->LoadInstant<Texture>("fast_travel.png");
	tFastTravelDeny = res_mgr->LoadInstant<Texture>("fast_travel_deny.png");
}

//=================================================================================================
bool Net::ValidateNick(cstring nick)
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

//=================================================================================================
PlayerInfo* Net::FindPlayer(Cstring nick)
{
	assert(nick);
	for(PlayerInfo& info : players)
	{
		if(info.left == PlayerInfo::LEFT_NO && info.name == nick)
			return &info;
	}
	return nullptr;
}

//=================================================================================================
PlayerInfo* Net::FindPlayer(const SystemAddress& adr)
{
	assert(adr != UNASSIGNED_SYSTEM_ADDRESS);
	for(PlayerInfo& info : players)
	{
		if(info.adr == adr)
			return &info;
	}
	return nullptr;
}

//=================================================================================================
PlayerInfo* Net::TryGetPlayer(int id)
{
	for(PlayerInfo& info : players)
	{
		if(info.id == id)
		{
			if(info.left == PlayerInfo::LEFT_NO)
				return &info;
			break;
		}
	}
	return nullptr;
}

//=================================================================================================
void Net::OpenPeer()
{
	if(peer)
		return;

	peer = RakPeerInterface::GetInstance();
#ifdef TEST_LAG
	peer->ApplyNetworkSimulator(0.f, TEST_LAG, 0);
#endif
	if(game->cfg.GetBool("packet_logger"))
	{
		packet_logger = new PacketLogger;
		peer->AttachPlugin(packet_logger);
	}
}

//=================================================================================================
void Net::ClosePeer(bool wait, bool check_was_client)
{
	assert(peer);

	Info("Net peer shutdown.");
	peer->Shutdown(wait ? CLOSE_PEER_TIMER : 0);
	if(IsClient() && check_was_client)
		was_client = true;
	changes.clear();
	SetMode(Mode::Singleplayer);
}

//=================================================================================================
void Net::StartFastTravel(int who)
{
	// mark players
	if(IsServer() || who == 2)
	{
		for(PlayerInfo& info : players)
			info.fast_travel = false;
		if(who == 0)
			GetMe().fast_travel = true;
		else
			team->GetLeader()->player->player_info->fast_travel = true;
		fast_travel = true;
		fast_travel_timer = 0.f;
	}

	// send to server/players
	if(who == 0 || who == 1)
	{
		NetChange& c = Add1(changes);
		c.type = NetChange::FAST_TRAVEL;
		c.id = FAST_TRAVEL_START;
		c.count = 0;
	}

	// show notification
	if((who == 0 && IsServer()) || (who == 2 && team->IsLeader()))
	{
		if(fast_travel_notif)
			fast_travel_notif->Close();
		fast_travel_notif = game_gui->notifications->Add(txFastTravelWaiting, tFastTravel, -1.f);
	}
	else if(who == 1 || (who == 2 && !team->IsLeader()))
	{
		if(fast_travel_notif)
			fast_travel_notif->Close();
		fast_travel_notif = game_gui->notifications->Add(txFastTravelText, tFastTravel, -1.f);
		fast_travel_notif->buttons = true;
		fast_travel_notif->callback = delegate<void(bool)>(this, &Net::OnFastTravel);
	}
}

//=================================================================================================
void Net::CancelFastTravel(int mode, int id)
{
	fast_travel = false;

	if(!fast_travel_notif)
		return;

	switch(mode)
	{
	case FAST_TRAVEL_CANCEL:
		if(id != team->my_id)
		{
			PlayerInfo* info = TryGetPlayer(id);
			cstring name = info ? info->name.c_str() : nullptr;
			fast_travel_notif->icon = tFastTravelDeny;
			fast_travel_notif->text = Format(txFastTravelCancel, name);
			fast_travel_notif->Close(3.f);
		}
		else
			fast_travel_notif->Close();
		break;
	case FAST_TRAVEL_DENY:
		{
			PlayerInfo* info = TryGetPlayer(id);
			cstring name = info ? info->name.c_str() : nullptr;
			fast_travel_notif->icon = tFastTravelDeny;
			fast_travel_notif->text = Format(txFastTravelDeny, name);
			fast_travel_notif->Close(3.f);
		}
		break;
	case FAST_TRAVEL_NOT_SAFE:
		fast_travel_notif->icon = tFastTravelDeny;
		fast_travel_notif->text = txFastTravelNotSafe;
		fast_travel_notif->Close(3.f);
		break;
	case FAST_TRAVEL_IN_PROGRESS:
		fast_travel_notif->Close();
		break;
	}

	fast_travel_notif->buttons = false;
	fast_travel_notif = nullptr;

	if(IsServer() || (team->IsLeader() && mode == FAST_TRAVEL_CANCEL))
	{
		NetChange& c = Add1(changes);
		c.type = NetChange::FAST_TRAVEL;
		c.id = mode;
		c.count = id;
	}
}

//=================================================================================================
void Net::ClearFastTravel()
{
	if(fast_travel_notif)
	{
		fast_travel_notif->Close();
		fast_travel_notif = nullptr;
	}
	fast_travel = false;
}

//=================================================================================================
void Net::OnFastTravel(bool accept)
{
	if(accept)
	{
		PlayerInfo& me = GetMe();
		fast_travel_notif->buttons = false;
		fast_travel_notif->text = txFastTravelWaiting;
		me.fast_travel = true;

		NetChange& c = Add1(changes);
		if(IsServer())
		{
			c.type = NetChange::FAST_TRAVEL_VOTE;
			c.id = me.id;
		}
		else
		{
			c.type = NetChange::FAST_TRAVEL;
			c.id = FAST_TRAVEL_ACCEPT;
		}
	}
	else
	{
		fast_travel = false;
		fast_travel_notif->Close();
		fast_travel_notif = nullptr;

		NetChange& c = Add1(changes);
		c.type = NetChange::FAST_TRAVEL;
		c.id = FAST_TRAVEL_DENY;
		c.count = team->my_id;
	}
}

//=================================================================================================
void Net::AddServerMsg(cstring msg)
{
	assert(msg);
	cstring s = Format(game->txServer, msg);
	if(game_gui->server->visible)
		game_gui->server->AddMsg(s);
	else
	{
		game_gui->messages->AddGameMsg(msg, 2.f + float(strlen(msg)) / 10);
		game_gui->mp_box->Add(s);
	}
}
