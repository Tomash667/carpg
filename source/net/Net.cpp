#include "Pch.h"
#include "Net.h"

#include "Game.h"
#include "GameGui.h"
#include "GameMessages.h"
#include "Language.h"
#include "LobbyApi.h"
#include "MpBox.h"
#include "PacketLogger.h"
#include "PlayerInfo.h"
#include "ServerPanel.h"
#include "Team.h"

#include <Notifications.h>
#include <ResourceManager.h>

Net* net;
vector<NetChange> Net::changes;
Net::Mode Net::mode;
const int CLOSE_PEER_TIMER = 1000; // ms

//=================================================================================================
Net::Net() : peer(nullptr), packetLogger(nullptr), mpLoad(false), mpUseInterp(true), mpInterp(0.05f), wasClient(false), journalChanges(false)
{
}

//=================================================================================================
Net::~Net()
{
	DeleteElements(players);
	DeleteElements(oldPlayers);
	if(peer)
		RakPeerInterface::DestroyInstance(peer);
	delete api;
	delete packetLogger;
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
	tFastTravel = resMgr->LoadInstant<Texture>("fast_travel.png");
	tFastTravelDeny = resMgr->LoadInstant<Texture>("fast_travel_deny.png");
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
	if(game->cfg.GetBool("packetLogger"))
	{
		packetLogger = new PacketLogger;
		peer->AttachPlugin(packetLogger);
	}
}

//=================================================================================================
void Net::ClosePeer(bool wait, bool checkWasClient)
{
	assert(peer);

	Info("Net peer shutdown.");
	peer->Shutdown(wait ? CLOSE_PEER_TIMER : 0);
	if(IsClient() && checkWasClient)
		wasClient = true;
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
			info.fastTravel = false;

		if(who == 0)
			GetMe().fastTravel = true;
		else
			team->GetLeader()->player->playerInfo->fastTravel = true;
		fastTravel = true;
		fastTravelTimer = 0.f;
	}

	// send to server/players
	if(who == 0 || who == 1)
	{
		NetChange& c = PushChange(NetChange::FAST_TRAVEL);
		c.id = FAST_TRAVEL_START;
		c.count = 0;
	}

	// show notification
	if((who == 0 && IsServer()) || (who == 2 && team->IsLeader()))
	{
		if(fastTravelNotification)
			fastTravelNotification->Close();
		fastTravelNotification = gameGui->notifications->Add(txFastTravelWaiting, tFastTravel, -1.f);
	}
	else if(who == 1 || (who == 2 && !team->IsLeader()))
	{
		if(fastTravelNotification)
			fastTravelNotification->Close();
		fastTravelNotification = gameGui->notifications->Add(txFastTravelText, tFastTravel, -1.f);
		fastTravelNotification->buttons = true;
		fastTravelNotification->callback = delegate<void(bool)>(this, &Net::OnFastTravel);
	}
}

//=================================================================================================
void Net::CancelFastTravel(int mode, int id)
{
	fastTravel = false;

	if(!fastTravelNotification)
		return;

	switch(mode)
	{
	case FAST_TRAVEL_CANCEL:
		if(id != team->myId)
		{
			PlayerInfo* info = TryGetPlayer(id);
			cstring name = info ? info->name.c_str() : nullptr;
			fastTravelNotification->icon = tFastTravelDeny;
			fastTravelNotification->text = Format(txFastTravelCancel, name);
			fastTravelNotification->Close(3.f);
		}
		else
			fastTravelNotification->Close();
		break;
	case FAST_TRAVEL_DENY:
		{
			PlayerInfo* info = TryGetPlayer(id);
			cstring name = info ? info->name.c_str() : nullptr;
			fastTravelNotification->icon = tFastTravelDeny;
			fastTravelNotification->text = Format(txFastTravelDeny, name);
			fastTravelNotification->Close(3.f);
		}
		break;
	case FAST_TRAVEL_NOT_SAFE:
		fastTravelNotification->icon = tFastTravelDeny;
		fastTravelNotification->text = txFastTravelNotSafe;
		fastTravelNotification->Close(3.f);
		break;
	case FAST_TRAVEL_IN_PROGRESS:
		fastTravelNotification->Close();
		break;
	}

	fastTravelNotification->buttons = false;
	fastTravelNotification = nullptr;

	if(IsServer() || (team->IsLeader() && mode == FAST_TRAVEL_CANCEL))
	{
		NetChange& c = PushChange(NetChange::FAST_TRAVEL);
		c.id = mode;
		c.count = id;
	}
}

//=================================================================================================
void Net::ClearFastTravel()
{
	if(fastTravelNotification)
	{
		fastTravelNotification->Close();
		fastTravelNotification = nullptr;
	}
	fastTravel = false;
}

//=================================================================================================
void Net::OnFastTravel(bool accept)
{
	if(accept)
	{
		PlayerInfo& me = GetMe();
		fastTravelNotification->buttons = false;
		fastTravelNotification->text = txFastTravelWaiting;
		me.fastTravel = true;

		if(IsServer())
		{
			NetChange& c = PushChange(NetChange::FAST_TRAVEL_VOTE);
			c.id = me.id;
		}
		else
		{
			NetChange& c = PushChange(NetChange::FAST_TRAVEL);
			c.id = FAST_TRAVEL_ACCEPT;
		}
	}
	else
	{
		fastTravel = false;
		fastTravelNotification->Close();
		fastTravelNotification = nullptr;

		NetChange& c = PushChange(NetChange::FAST_TRAVEL);
		c.id = FAST_TRAVEL_DENY;
		c.count = team->myId;
	}
}
