#include "Pch.h"
#include "GameCore.h"
#include "Net.h"
#include "BitStreamFunc.h"
#include "Version.h"
#include "Level.h"
#include "Language.h"
#include "ErrorHandler.h"
#include "Game.h"
#include "GameFile.h"
#include "GroundItem.h"
#include "Chest.h"
#include "Trap.h"
#include "Door.h"
#include "EntityInterpolator.h"

Net N;
const float CHANGE_LEVEL_TIMER = 5.f;

//=================================================================================================
Net::Net() : peer(nullptr), current_packet(nullptr), mp_load(false), mp_use_interp(true), mp_interp(0.05f)
{
}

//=================================================================================================
void Net::LoadLanguage()
{
	txCreateServerFailed = Str("createServerFailed");
	txInitConnectionFailed = Str("initConnectionFailed");
}

//=================================================================================================
void Net::Cleanup()
{
	DeleteElements(players);
	DeleteElements(old_players);
	if(peer)
		RakPeerInterface::DestroyInstance(peer);
}

//=================================================================================================
void Net::WriteNetVars(BitStreamWriter& f)
{
	f << mp_use_interp;
	f << mp_interp;
}

//=================================================================================================
void Net::ReadNetVars(BitStreamReader& f)
{
	f >> mp_use_interp;
	f >> mp_interp;
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
	for(PlayerInfo* info : players)
	{
		if(info->left == PlayerInfo::LEFT_NO && info->name == nick)
			return info;
	}
	return nullptr;
}

//=================================================================================================
PlayerInfo* Net::FindPlayer(const SystemAddress& adr)
{
	assert(adr != UNASSIGNED_SYSTEM_ADDRESS);
	for(PlayerInfo* info : players)
	{
		if(info->adr == adr)
			return info;
	}
	return nullptr;
}

//=================================================================================================
PlayerInfo* Net::TryGetPlayer(int id)
{
	for(PlayerInfo* info : players)
	{
		if(info->id == id)
		{
			if(info->left == PlayerInfo::LEFT_NO)
				return info;
			break;
		}
	}
	return nullptr;
}

//=================================================================================================
void Net::InitServer()
{
	Info("Creating server (port %d)...", port);

	if(!peer)
		peer = RakPeerInterface::GetInstance();

	SocketDescriptor sd(port, 0);
	sd.socketFamily = AF_INET;
	StartupResult r = peer->Startup(max_players + 4, &sd, 1);
	if(r != RAKNET_STARTED)
	{
		Error("Failed to create server: RakNet error %d.", r);
		throw Format(txCreateServerFailed, r);
	}

	if(!password.empty())
	{
		Info("Set server password.");
		peer->SetIncomingPassword(password.c_str(), password.length());
	}

	peer->SetMaximumIncomingConnections((word)max_players + 4);
	DEBUG_DO(peer->SetTimeoutTime(60 * 60 * 1000, UNASSIGNED_SYSTEM_ADDRESS));

	Info("Server created. Waiting for connection.");

	SetMode(Mode::Server);
	Info("sv_online = true");
}

//=================================================================================================
void Net::OnChangeLevel(int level)
{
	BitStreamWriter f;
	f << ID_CHANGE_LEVEL;
	f << (byte)L.location_index;
	f << (byte)level;
	f << false;

	uint ack = SendAll(f, HIGH_PRIORITY, RELIABLE_WITH_ACK_RECEIPT, Stream_TransferServer);
	Game& game = Game::Get();
	for(PlayerInfo* info : players)
	{
		if(info->id == game.my_id)
			info->state = PlayerInfo::IN_GAME;
		else
		{
			info->state = PlayerInfo::WAITING_FOR_RESPONSE;
			info->ack = ack;
			info->timer = CHANGE_LEVEL_TIMER;
		}
	}
}

//=================================================================================================
void Net::SendServer(BitStreamWriter& f, PacketPriority priority, PacketReliability reliability, const SystemAddress& adr, StreamLogType type)
{
	assert(IsServer());
	peer->Send(&f.GetBitStream(), priority, reliability, 0, adr, false);
	StreamWrite(f.GetBitStream(), type, adr);
}

//=================================================================================================
uint Net::SendAll(BitStreamWriter& f, PacketPriority priority, PacketReliability reliability, StreamLogType type)
{
	assert(IsServer());
	if(active_players <= 1)
		return 0;
	uint ack = peer->Send(&f.GetBitStream(), priority, reliability, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
	StreamWrite(f.GetBitStream(), type, UNASSIGNED_SYSTEM_ADDRESS);
	return ack;
}

//=================================================================================================
int Net::GetNewPlayerId()
{
	while(true)
	{
		last_id = (last_id + 1) % 256;
		bool ok = true;
		for(PlayerInfo* info : players)
		{
			if(info->id == last_id)
			{
				ok = false;
				break;
			}
		}
		if(ok)
			return last_id;
	}
}

//=================================================================================================
PlayerInfo* Net::FindOldPlayer(cstring nick)
{
	assert(nick);

	for(auto info : old_players)
	{
		if(info->name == nick)
			return info;
	}

	return nullptr;
}

//=================================================================================================
void Net::DeleteOldPlayers()
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
				L.phy_world->removeCollisionObject(info.u->cobj);
				delete info.u->cobj;
			}
			delete info.u;
		}
	}
	DeleteElements(old_players);
}

//=================================================================================================
void Net::Save(GameWriter& f)
{
	f << server_name;
	f << password;
	f << active_players;
	f << max_players;
	f << last_id;
	uint count = 0;
	for(auto info : players)
	{
		if(info->left == PlayerInfo::LEFT_NO)
			++count;
	}
	f << count;
	for(PlayerInfo* info : players)
	{
		if(info->left == PlayerInfo::LEFT_NO)
			info->Save(f);
	}
	f << Unit::netid_counter;
	f << GroundItem::netid_counter;
	f << Chest::netid_counter;
	f << Usable::netid_counter;
	f << Trap::netid_counter;
	f << Door::netid_counter;
	f << Electro::netid_counter;
	f << mp_use_interp;
	f << mp_interp;
}

//=================================================================================================
void Net::Load(GameReader& f)
{
	f >> server_name;
	f >> password;
	f >> active_players;
	f >> max_players;
	f >> last_id;
	uint count;
	f >> count;
	DeleteElements(old_players);
	old_players.resize(count);
	for(uint i = 0; i < count; ++i)
	{
		old_players[i] = new PlayerInfo;
		old_players[i]->Load(f);
	}
	f >> Unit::netid_counter;
	f >> GroundItem::netid_counter;
	f >> Chest::netid_counter;
	f >> Usable::netid_counter;
	f >> Trap::netid_counter;
	f >> Door::netid_counter;
	f >> Electro::netid_counter;
	f >> mp_use_interp;
	f >> mp_interp;
}

//=================================================================================================
void Net::InterpolatePlayers(float dt)
{
	for(PlayerInfo* info : players)
	{
		if(!info->pc->is_local && info->left == PlayerInfo::LEFT_NO)
			info->u->interp->Update(dt, info->u->visual_pos, info->u->rot);
	}
}

//=================================================================================================
void Net::InitClient()
{
	Info("Initlializing client...");

	if(!peer)
		peer = RakPeerInterface::GetInstance();

	SocketDescriptor sd;
	sd.socketFamily = AF_INET;
	StartupResult r = peer->Startup(1, &sd, 1);
	if(r != RAKNET_STARTED)
	{
		Error("Failed to create client: RakNet error %d.", r);
		throw Format(txInitConnectionFailed, r);
	}
	peer->SetMaximumIncomingConnections(0);

	SetMode(Mode::Client);
	Info("sv_online = true");

	DEBUG_DO(peer->SetTimeoutTime(60 * 60 * 1000, UNASSIGNED_SYSTEM_ADDRESS));
}

//=================================================================================================
void Net::SendClient(BitStreamWriter& f, PacketPriority priority, PacketReliability reliability, StreamLogType type)
{
	assert(IsClient());
	peer->Send(&f.GetBitStream(), priority, reliability, 0, server, false);
	StreamWrite(f.GetBitStream(), type, server);
}

//=================================================================================================
void Net::InterpolateUnits(float dt)
{
	for(LevelContext& ctx : L.ForEachContext())
	{
		for(Unit* unit : *ctx.units)
		{
			if(!unit->IsLocal())
				unit->interp->Update(dt, unit->visual_pos, unit->rot);
			if(unit->mesh_inst->mesh->head.n_groups == 1)
			{
				if(!unit->mesh_inst->groups[0].anim)
				{
					unit->action = A_NONE;
					unit->animation = ANI_STAND;
				}
			}
			else
			{
				if(!unit->mesh_inst->groups[0].anim && !unit->mesh_inst->groups[1].anim)
				{
					unit->action = A_NONE;
					unit->animation = ANI_STAND;
				}
			}
		}
	}
}

//=================================================================================================
BitStream& Net::StreamStart(Packet* packet, StreamLogType type)
{
	assert(packet);
	assert(!current_packet);
	if(current_packet)
		StreamError("Unclosed stream.");

	current_packet = packet;
	current_stream.~BitStream();
	new((void*)&current_stream)BitStream(packet->data, packet->length, false);
	ErrorHandler::Get().StreamStart(current_packet, (int)type);

	return current_stream;
}

//=================================================================================================
void Net::StreamEnd()
{
	if(!current_packet)
		return;

	ErrorHandler::Get().StreamEnd(true);
	current_packet = nullptr;
}

//=================================================================================================
void Net::StreamError()
{
	if(!current_packet)
		return;

	ErrorHandler::Get().StreamEnd(false);
	current_packet = nullptr;
}

//=================================================================================================
void Net::StreamWrite(const void* data, uint size, StreamLogType type, const SystemAddress& adr)
{
	ErrorHandler::Get().StreamWrite(data, size, type, adr);
}
