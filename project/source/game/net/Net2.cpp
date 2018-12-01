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
#include "GlobalGui.h"
#include "ServerPanel.h"
#include "Journal.h"
#include "PlayerInfo.h"
#include "World.h"
#include "QuestManager.h"
#include "Quest_Secret.h"
#include "GameStats.h"
#include "SoundManager.h"
#include "Team.h"
#include "LobbyApi.h"

Net N;
const float CHANGE_LEVEL_TIMER = 5.f;
const int CLOSE_PEER_TIMER = 1000; // ms

//=================================================================================================
Net::Net() : peer(nullptr), current_packet(nullptr), mp_load(false), mp_use_interp(true), mp_interp(0.05f), was_client(false)
{
	api = new LobbyApi;
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
	delete api;
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
void Net::ClosePeer(bool wait)
{
	assert(peer);

	Info("Net peer shutdown.");
	peer->Shutdown(wait ? CLOSE_PEER_TIMER : 0);
	if(IsClient())
		was_client = true;
	changes.clear();
	SetMode(Mode::Singleplayer);
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

	if(!server_hidden)
		api->RegisterServer(server_name, max_players, GetServerFlags(), port);

	Info("Server created. Waiting for connection.");

	SetMode(Mode::Server);
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
	for(PlayerInfo* info : players)
	{
		if(info->id == Team.my_id)
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
void Net::KickPlayer(PlayerInfo& info)
{
	// send kick message
	BitStreamWriter f;
	f << ID_SERVER_CLOSE;
	f << (byte)ServerClose_Kicked;
	SendServer(f, MEDIUM_PRIORITY, RELIABLE, info.adr, Stream_None);

	info.state = PlayerInfo::REMOVING;

	Game& game = Game::Get();
	ServerPanel* server_panel = game.gui->server;
	if(server_panel->visible)
	{
		server_panel->AddMsg(Format(game.txPlayerKicked, info.name.c_str()));
		Info("Player %s was kicked.", info.name.c_str());

		if(active_players > 2)
			server_panel->AddLobbyUpdate(Int2(Lobby_KickPlayer, info.id));

		server_panel->CheckReady();
		server_panel->UpdateServerInfo();
	}
	else
	{
		info.left = PlayerInfo::LEFT_KICK;
		players_left = true;
	}
}

//=================================================================================================
void Net::FilterServerChanges()
{
	for(vector<NetChange>::iterator it = changes.begin(), end = changes.end(); it != end;)
	{
		if(FilterOut(*it))
		{
			if(it + 1 == end)
			{
				changes.pop_back();
				break;
			}
			else
			{
				std::iter_swap(it, end - 1);
				changes.pop_back();
				end = changes.end();
			}
		}
		else
			++it;
	}

	for(PlayerInfo* info : players)
	{
		for(vector<NetChangePlayer>::iterator it = info->changes.begin(), end = info->changes.end(); it != end;)
		{
			if(FilterOut(*it))
			{
				if(it + 1 == end)
				{
					info->changes.pop_back();
					break;
				}
				else
				{
					std::iter_swap(it, end - 1);
					info->changes.pop_back();
					end = info->changes.end();
				}
			}
			else
				++it;
		}
	}
}

//=================================================================================================
bool Net::FilterOut(NetChangePlayer& c)
{
	switch(c.type)
	{
	case NetChangePlayer::GOLD_MSG:
	case NetChangePlayer::DEVMODE:
	case NetChangePlayer::GOLD_RECEIVED:
	case NetChangePlayer::ADDED_ITEMS_MSG:
	case NetChangePlayer::GAME_MESSAGE:
	case NetChangePlayer::RUN_SCRIPT_RESULT:
	case NetChangePlayer::GENERIC_CMD_RESPONSE:
	case NetChangePlayer::GAME_MESSAGE_FORMATTED:
		return false;
	default:
		return true;
	}
}

//=================================================================================================
void Net::WriteWorldData(BitStreamWriter& f)
{
	Info("Preparing world data.");
	Game& game = Game::Get();

	f << ID_WORLD_DATA;

	// world
	W.Write(f);

	// quests
	QM.Write(f);

	// rumors
	f.WriteStringArray<byte, word>(game.gui->journal->GetRumors());

	// stats
	GameStats::Get().Write(f);

	// mp vars
	WriteNetVars(f);
	if(!net_strs.empty())
		StringPool.Free(net_strs);

	// secret note text
	f << Quest_Secret::GetNote().desc;

	f.WriteCasted<byte>(0xFF);
}

//=================================================================================================
void Net::WritePlayerStartData(BitStreamWriter& f, PlayerInfo& info)
{
	f << ID_PLAYER_START_DATA;

	// flags
	byte flags = 0;
	if(info.devmode)
		flags |= SF_DEVMODE;
	if(mp_load)
	{
		if(info.u->invisible)
			flags |= SF_INVISIBLE;
		if(info.u->player->noclip)
			flags |= SF_NOCLIP;
		if(info.u->player->godmode)
			flags |= SF_GODMODE;
	}
	if(Game::Get().noai)
		flags |= SF_NOAI;
	f << flags;

	// notes
	f.WriteStringArray<word, word>(info.notes);

	f.WriteCasted<byte>(0xFF);
}

//=================================================================================================
void Net::WriteLevelData(BitStream& stream, bool loaded_resources)
{
	BitStreamWriter f(stream);
	f << ID_LEVEL_DATA;
	f << mp_load;
	f << loaded_resources;

	// level
	L.Write(f);

	// items preload
	std::set<const Item*>& items_load = Game::Get().items_load;
	f << items_load.size();
	for(const Item* item : items_load)
	{
		f << item->id;
		if(item->IsQuest())
			f << item->refid;
	}

	f.WriteCasted<byte>(0xFF);
}

//=================================================================================================
int Net::GetServerFlags()
{
	int flags = 0;
	if(!password.empty())
		flags |= SERVER_PASSWORD;
	if(mp_load)
		flags |= SERVER_SAVED;
	return flags;
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
void Net::FilterClientChanges()
{
	for(vector<NetChange>::iterator it = changes.begin(), end = changes.end(); it != end;)
	{
		if(FilterOut(*it))
		{
			if(it + 1 == end)
			{
				changes.pop_back();
				break;
			}
			else
			{
				std::iter_swap(it, end - 1);
				changes.pop_back();
				end = changes.end();
			}
		}
		else
			++it;
	}
}


//=================================================================================================
bool Net::FilterOut(NetChange& c)
{
	switch(c.type)
	{
	case NetChange::CHANGE_EQUIPMENT:
		return IsServer();
	case NetChange::CHANGE_FLAGS:
	case NetChange::UPDATE_CREDIT:
	case NetChange::ALL_QUESTS_COMPLETED:
	case NetChange::CHANGE_LOCATION_STATE:
	case NetChange::ADD_RUMOR:
	case NetChange::ADD_NOTE:
	case NetChange::REGISTER_ITEM:
	case NetChange::ADD_QUEST:
	case NetChange::UPDATE_QUEST:
	case NetChange::RENAME_ITEM:
	case NetChange::REMOVE_PLAYER:
	case NetChange::CHANGE_LEADER:
	case NetChange::RANDOM_NUMBER:
	case NetChange::CHEAT_SKIP_DAYS:
	case NetChange::CHEAT_NOCLIP:
	case NetChange::CHEAT_GODMODE:
	case NetChange::CHEAT_INVISIBLE:
	case NetChange::CHEAT_ADD_ITEM:
	case NetChange::CHEAT_ADD_GOLD:
	case NetChange::CHEAT_SET_STAT:
	case NetChange::CHEAT_MOD_STAT:
	case NetChange::CHEAT_REVEAL:
	case NetChange::GAME_OVER:
	case NetChange::CHEAT_CITIZEN:
	case NetChange::WORLD_TIME:
	case NetChange::TRAIN_MOVE:
	case NetChange::ADD_LOCATION:
	case NetChange::REMOVE_CAMP:
	case NetChange::CHEAT_NOAI:
	case NetChange::END_OF_GAME:
	case NetChange::UPDATE_FREE_DAYS:
	case NetChange::CHANGE_MP_VARS:
	case NetChange::PAY_CREDIT:
	case NetChange::GIVE_GOLD:
	case NetChange::DROP_GOLD:
	case NetChange::HERO_LEAVE:
	case NetChange::PAUSED:
	case NetChange::CLOSE_ENCOUNTER:
	case NetChange::GAME_STATS:
	case NetChange::CHANGE_ALWAYS_RUN:
		return false;
	case NetChange::TALK:
	case NetChange::TALK_POS:
		if(IsServer() && c.str)
		{
			StringPool.Free(c.str);
			RemoveElement(net_strs, c.str);
			c.str = nullptr;
		}
		return true;
	case NetChange::RUN_SCRIPT:
		StringPool.Free(c.str);
		return true;
	default:
		return true;
	}
}

//=================================================================================================
bool Net::ReadWorldData(BitStreamReader& f)
{
	Game& game = Game::Get();

	// world
	if(!W.Read(f))
	{
		Error("Read world: Broken packet for world data.");
		return false;
	}

	// quests
	if(!QM.Read(f))
		return false;

	// rumors
	f.ReadStringArray<byte, word>(game.gui->journal->GetRumors());
	if(!f)
	{
		Error("Read world: Broken packet for rumors.");
		return false;
	}

	// game stats
	GameStats::Get().Read(f);
	if(!f)
	{
		Error("Read world: Broken packet for game stats.");
		return false;
	}

	// mp vars
	ReadNetVars(f);
	if(!f)
	{
		Error("Read world: Broken packet for mp vars.");
		return false;
	}

	// secret note text
	f >> Quest_Secret::GetNote().desc;
	if(!f)
	{
		Error("Read world: Broken packet for secret note text.");
		return false;
	}

	// checksum
	byte check;
	f >> check;
	if(!f || check != 0xFF)
	{
		Error("Read world: Broken checksum.");
		return false;
	}

	// load music
	if(!game.sound_mgr->IsMusicDisabled())
	{
		game.LoadMusic(MusicType::Boss, false);
		game.LoadMusic(MusicType::Death, false);
		game.LoadMusic(MusicType::Travel, false);
	}

	return true;
}

//=================================================================================================
bool Net::ReadPlayerStartData(BitStreamReader& f)
{
	Game& game = Game::Get();

	byte flags;
	f >> flags;
	f.ReadStringArray<word, word>(game.gui->journal->GetNotes());
	if(!f)
		return false;

	if(IS_SET(flags, SF_DEVMODE))
		game.devmode = true;
	if(IS_SET(flags, SF_INVISIBLE))
		game.invisible = true;
	if(IS_SET(flags, SF_NOCLIP))
		game.noclip = true;
	if(IS_SET(flags, SF_GODMODE))
		game.godmode = true;
	if(IS_SET(flags, SF_NOAI))
		game.noai = true;

	// checksum
	byte check;
	f >> check;
	if(!f || check != 0xFF)
	{
		Error("Read player start data: Broken checksum.");
		return false;
	}

	return true;
}

//=================================================================================================
bool Net::ReadLevelData(BitStreamReader& f)
{
	Game& game = Game::Get();
	game.cam.Reset();
	game.pc_data.rot_buf = 0.f;
	W.RemoveBossLevel();

	bool loaded_resources;
	f >> N.mp_load;
	f >> loaded_resources;
	if(!f)
	{
		Error("Read level: Broken packet for loading info.");
		return false;
	}

	if(!L.Read(f, loaded_resources))
		return false;

	// items to preload
	uint items_load_count = f.Read<uint>();
	if(!f.Ensure(items_load_count * 2))
	{
		Error("Read level: Broken items preload count.");
		return false;
	}
	std::set<const Item*>& items_load = game.items_load;
	items_load.clear();
	for(uint i = 0; i < items_load_count; ++i)
	{
		const string& item_id = f.ReadString1();
		if(!f)
		{
			Error("Read level: Broken item preload '%u'.", i);
			return false;
		}
		if(item_id[0] != '$')
		{
			auto item = Item::TryGet(item_id);
			if(!item)
			{
				Error("Read level: Missing item preload '%s'.", item_id.c_str());
				return false;
			}
			items_load.insert(item);
		}
		else
		{
			int refid = f.Read<int>();
			if(!f)
			{
				Error("Read level: Broken quest item preload '%u'.", i);
				return false;
			}
			const Item* item = QM.FindQuestItemClient(item_id.c_str(), refid);
			if(!item)
			{
				Error("Read level: Missing quest item preload '%s' (%d).", item_id.c_str(), refid);
				return false;
			}
			const Item* base = Item::TryGet(item_id.c_str() + 1);
			if(!base)
			{
				Error("Read level: Missing quest item preload base '%s' (%d).", item_id.c_str(), refid);
				return false;
			}
			items_load.insert(base);
			items_load.insert(item);
		}
	}

	// checksum
	byte check;
	f >> check;
	if(!f || check != 0xFF)
	{
		Error("Read level: Broken checksum.");
		return false;
	}

	return true;
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
