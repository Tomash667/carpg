#include "Pch.h"
#include "Base.h"
#include "Game.h"
#include "BitStreamFunc.h"
#include "Version.h"
#include "Inventory.h"
#include "Journal.h"
#include "TeamPanel.h"
#include "ErrorHandler.h"

extern bool merchant_buy[];
extern bool blacksmith_buy[];
extern bool alchemist_buy[];
extern bool innkeeper_buy[];
extern bool foodseller_buy[];

//=================================================================================================
inline void WriteBaseItem(BitStream& s, const Item& item)
{
	WriteString1(s, item.id);
	if(item.id[0] == '$')
		s.Write(item.refid);
}

//=================================================================================================
inline void WriteBaseItem(BitStream& s, const Item* item)
{
	if(item)
		WriteBaseItem(s, *item);
	else
		s.WriteCasted<byte>(0);
}

//=================================================================================================
inline void WriteItemList(BitStream& s, vector<ItemSlot>& items)
{
	s.Write(items.size());
	for(vector<ItemSlot>::iterator it = items.begin(), end = items.end(); it != end; ++it)
	{
		WriteBaseItem(s, *it->item);
		s.Write(it->count);
	}
}

//=================================================================================================
inline void WriteItemListTeam(BitStream& s, vector<ItemSlot>& items)
{
	s.Write(items.size());
	for(vector<ItemSlot>::iterator it = items.begin(), end = items.end(); it != end; ++it)
	{
		WriteBaseItem(s, *it->item);
		s.Write(it->count);
		s.Write(it->team_count);
	}
}

//=================================================================================================
bool Game::ReadItemList(BitStream& s, vector<ItemSlot>& items)
{
	uint count;
	if(!s.Read(count))
		return false;

	items.resize(count);
	for(vector<ItemSlot>::iterator it = items.begin(), end = items.end(); it != end; ++it)
	{
		if(ReadItemAndFind(s, it->item) == -1 || !s.Read(it->count))
			return false;
		it->team_count = 0;
	}

	return true;
}

//=================================================================================================
bool Game::ReadItemListTeam(BitStream& s, vector<ItemSlot>& items)
{
	uint count;
	if(!s.Read(count))
		return false;

	items.resize(count);
	for(vector<ItemSlot>::iterator it = items.begin(), end = items.end(); it != end; ++it)
	{
		if(ReadItemAndFind(s, it->item) == -1 || !s.Read(it->count) || !s.Read(it->team_count))
			return false;
	}

	return true;
}

//=================================================================================================
void Game::InitServer()
{
	LOG(Format("Creating server (port %d)...", mp_port));

	if(!peer)
		peer = RakNet::RakPeerInterface::GetInstance();

	SocketDescriptor sd(mp_port, 0);
	sd.socketFamily = AF_INET;
	StartupResult r = peer->Startup(max_players+4, &sd, 1);
	if(r != RAKNET_STARTED)
	{
		ERROR(Format("Failed to create server: RakNet error %d.", r));
		throw Format(txCreateServerFailed, r);
	}

	if(!server_pswd.empty())
	{
		LOG("Set server password.");
		peer->SetIncomingPassword(server_pswd.c_str(), server_pswd.length());
	}

	peer->SetMaximumIncomingConnections((word)max_players+4);
#ifdef _DEBUG
	peer->SetTimeoutTime(60*60*1000, UNASSIGNED_SYSTEM_ADDRESS);
#endif

	LOG("Server created. Waiting for connection.");

	sv_online = true;
	sv_server = true;
	sv_startup = false;
	LOG("sv_online = true");
}

//=================================================================================================
void Game::InitClient()
{
	LOG("Initlializing client...");

	if(!peer)
		peer = RakNet::RakPeerInterface::GetInstance();

	RakNet::SocketDescriptor sd;
	sd.socketFamily = AF_INET;
	StartupResult r = peer->Startup(1, &sd, 1);
	if(r != RAKNET_STARTED)
	{
		ERROR(Format("Failed to create client: RakNet error %d.", r));
		throw Format(txInitConnectionFailed, r);
	}
	peer->SetMaximumIncomingConnections(0);

	sv_online = true;
	sv_server = false;
	LOG("sv_online = true");

#ifdef _DEBUG
	peer->SetTimeoutTime(60*60*1000, UNASSIGNED_SYSTEM_ADDRESS);
#endif
}

//=================================================================================================
void Game::UpdateServerInfo()
{
	// 0 char C
	// 1 char A
	// 2-5 int - wersja
	// 6 byte - gracze
	// 7 byte - max graczy
	// 8 byte - flagi (0x01 - has³o, 0x02 - wczytana gra)
	// 9+ byte - nazwa
	server_info.Reset();
	server_info.WriteCasted<byte>('C');
	server_info.WriteCasted<byte>('A');
	server_info.Write(VERSION);
	server_info.WriteCasted<byte>(players);
	server_info.WriteCasted<byte>(max_players);
	byte flags = 0;
	if(!server_pswd.empty())
		flags |= 0x01;
	if(mp_load)
		flags |= 0x02;
	server_info.WriteCasted<byte>(flags);
	WriteString1(server_info, server_name);

	peer->SetOfflinePingResponse((cstring)server_info.GetData(), server_info.GetNumberOfBytesUsed());
}

//=================================================================================================
int Game::FindPlayerIndex(cstring nick, bool not_left)
{
	assert(nick);

	int index = 0;
	for(vector<PlayerInfo>::iterator it = game_players.begin(), end = game_players.end(); it != end; ++it, ++index)
	{
		if(it->name == nick)
		{
			if(not_left && it->left)
				return -1;
			return index;
		}
	}

	return -1;
}

//=================================================================================================
void Game::AddServerMsg(cstring msg)
{
	assert(msg);
	cstring s = Format(txServer, msg);
	if(server_panel->visible)
		server_panel->AddMsg(s);
	else
	{
		AddGameMsg(msg, 2.f+float(strlen(msg))/10);
		AddMultiMsg(s);
	}
}

//=================================================================================================
void Game::KickPlayer(int index)
{
	PlayerInfo& info = game_players[index];

	// wyœlij informacje o kicku
	packet_data.resize(2);
	packet_data[0] = ID_SERVER_CLOSE;
	packet_data[1] = 1;
	peer->Send((cstring)&packet_data[0], 2, MEDIUM_PRIORITY, RELIABLE, 0, info.adr, false);

	info.state = PlayerInfo::REMOVING;

	AddMsg(Format(txPlayerKicked, info.name.c_str()));
	LOG(Format("Player %s was kicked.", info.name.c_str()));

	if(leader_id == info.id)
	{
		// serwer zostaje przywódc¹
		leader_id = my_id;
		if(players > 2)
		{
			if(server_panel->visible)
				AddLobbyUpdate(INT2(Lobby_ChangeLeader,0));
			else
			{
				NetChange& c = Add1(net_changes);
				c.type = NetChange::CHANGE_LEADER;
				c.id = my_id;
				leader = pc->unit;
			}
		}
		if(dialog_enc)
			dialog_enc->bts[0].state = Button::NONE;
		AddMsg(txYouAreLeader);
	}

	if(server_panel->visible)
	{
		if(players > 2)
			AddLobbyUpdate(INT2(Lobby_KickPlayer,info.id));

		// puki co tylko dla lobby
		CheckReady();

		UpdateServerInfo();
	}
	else
		info.left_reason = PlayerInfo::LEFT_KICK;
}

//=================================================================================================
int Game::GetPlayerIndex(int id)
{
	assert(in_range(id, 0, 255));
	int index = 0;
	for(vector<PlayerInfo>::iterator it = game_players.begin(), end = game_players.end(); it != end; ++it, ++index)
	{
		if(it->id == id)
			return index;
	}
	return -1;
}

//=================================================================================================
int Game::FindPlayerIndex(const SystemAddress& adr)
{
	assert(adr != UNASSIGNED_SYSTEM_ADDRESS);
	int index = 0;
	for(vector<PlayerInfo>::iterator it = game_players.begin(), end = game_players.end(); it != end; ++it, ++index)
	{
		if(it->adr == adr)
			return index;
	}
	return -1;
}

//=================================================================================================
void Game::PrepareLevelData(BitStream& s)
{
	byte ile;

	s.Reset();
	s.WriteCasted<byte>(ID_LEVEL_DATA);
	s.WriteCasted<byte>(mp_load ? 1 : 0);

	// jeœli to zewnêtrzna lokacja
	if(location->outside)
	{
		OutsideLocation* outside = (OutsideLocation*)location;
		s.Write((cstring)outside->tiles, sizeof(TerrainTile)*outside->size*outside->size);
		int size2 = outside->size+1;
		s.Write((cstring)outside->h, sizeof(float)*size2*size2);
		if(location->type == L_CITY || location->type == L_VILLAGE)
		{
			City* city = (City*)location;
			WriteBool(s, city->have_exit);
			s.WriteCasted<byte>(city->entry_points.size());
			for(vector<EntryPoint>::const_iterator entry_it = city->entry_points.begin(), entry_end = city->entry_points.end(); entry_it != entry_end; ++entry_it)
			{
				s.Write(entry_it->exit_area);
				s.Write(entry_it->exit_y);
			}
			s.WriteCasted<byte>(city->buildings.size());
			for(vector<CityBuilding>::iterator it = city->buildings.begin(), end = city->buildings.end(); it != end; ++it)
			{
				s.WriteCasted<byte>(it->type);
				s.Write(it->pt.x);
				s.Write(it->pt.y);
				s.WriteCasted<byte>(it->rot);
			}
			s.WriteCasted<byte>(city->inside_buildings.size());
			for(vector<InsideBuilding*>::iterator it = city->inside_buildings.begin(), end = city->inside_buildings.end(); it != end; ++it)
			{
				InsideBuilding& ib = **it;
				s.Write(ib.level_shift.x);
				s.Write(ib.level_shift.y);
				s.WriteCasted<byte>(ib.type);
				// u¿ywalne obiekty
				ile = (byte)ib.useables.size();
				s.Write(ile);
				for(vector<Useable*>::iterator it2 = ib.useables.begin(), end2 = ib.useables.end(); it2 != end2; ++it2)
					(*it2)->Write(s);
				// jednostki
				ile = (byte)ib.units.size();
				s.Write(ile);
				for(vector<Unit*>::iterator it2 = ib.units.begin(), end2 = ib.units.end(); it2 != end2; ++it2)
					WriteUnit(s, **it2);
				// drzwi
				ile = (byte)ib.doors.size();
				s.Write(ile);
				for(vector<Door*>::iterator it2 = ib.doors.begin(), end2 = ib.doors.end(); it2 != end2; ++it2)
					WriteDoor(s, **it2);
				// przedmioty na ziemi
				ile = (byte)ib.items.size();
				s.Write(ile);
				for(vector<GroundItem*>::iterator it2 = ib.items.begin(), end2 = ib.items.end(); it2 != end2; ++it2)
					WriteItem(s, **it2);
				// krew
				word ile2 = (word)ib.bloods.size();
				s.Write(ile2);
				for(vector<Blood>::iterator it2 = ib.bloods.begin(), end2 = ib.bloods.end(); it2 != end2; ++it2)
					it2->Write(s);
				// obiekty
				ile = (byte)ib.objects.size();
				s.Write(ile);
				for(vector<Object>::iterator it2 = ib.objects.begin(), end2 = ib.objects.end(); it2 != end2; ++it2)
					it2->Write(s);
				// œwiat³a
				ile = (byte)ib.lights.size();
				s.Write(ile);
				for(vector<Light>::iterator it2 = ib.lights.begin(), end2 = ib.lights.end(); it2 != end2; ++it2)
					it2->Write(s);
				// zmienne
				s.Write((cstring)&ib.xsphere_pos, sizeof(ib.xsphere_pos));
				s.Write((cstring)&ib.enter_area, sizeof(ib.enter_area));
				s.Write((cstring)&ib.exit_area, sizeof(ib.exit_area));
				s.Write(ib.top);
				s.Write(ib.xsphere_radius);
				s.WriteCasted<byte>(ib.type);
				s.Write(ib.enter_y);
			}
		}
		s.Write(light_angle);
	}
	else
	{
		InsideLocation* inside = (InsideLocation*)location;
		InsideLocationLevel& lvl = inside->GetLevelData();
		s.WriteCasted<byte>(inside->target);
		s.WriteCasted<byte>(inside->from_portal ? 1 : 0);
		// mapa
		s.WriteCasted<byte>(lvl.w);
		s.Write((cstring)lvl.map, sizeof(Pole)*lvl.w*lvl.h);
		// œwiat³a
		byte ile = (byte)lvl.lights.size();
		s.Write(ile);
		for(vector<Light>::iterator it = lvl.lights.begin(), end = lvl.lights.end(); it != end; ++it)
			it->Write(s);
		// pokoje
		ile = (byte)lvl.rooms.size();
		s.Write(ile);
		for(vector<Room>::iterator it = lvl.rooms.begin(), end = lvl.rooms.end(); it != end; ++it)
			it->Write(s);
		// pu³apki
		ile = (byte)lvl.traps.size();
		s.Write(ile);
		for(vector<Trap*>::iterator it = lvl.traps.begin(), end = lvl.traps.end(); it != end; ++it)
			WriteTrap(s, **it);
		// drzwi
		ile = (byte)lvl.doors.size();
		s.Write(ile);
		for(vector<Door*>::iterator it = lvl.doors.begin(), end = lvl.doors.end(); it != end; ++it)
			WriteDoor(s, **it);
		// schody
		WriteStruct(s, lvl.staircase_up);
		WriteStruct(s, lvl.staircase_down);
		s.WriteCasted<byte>(lvl.staircase_up_dir);
		s.WriteCasted<byte>(lvl.staircase_down_dir);
		WriteBool(s, lvl.staircase_down_in_wall);
	}

	// u¿ywalne obiekty
	ile = (byte)local_ctx.useables->size();
	s.Write(ile);
	for(vector<Useable*>::iterator it = local_ctx.useables->begin(), end = local_ctx.useables->end(); it != end; ++it)
		(*it)->Write(s);
	// jednostki
	ile = (byte)local_ctx.units->size();
	s.Write(ile);
	for(vector<Unit*>::iterator it = local_ctx.units->begin(), end = local_ctx.units->end(); it != end; ++it)
		WriteUnit(s, **it);
	// przedmioty na ziemi
	ile = (byte)local_ctx.items->size();
	s.Write(ile);
	for(vector<GroundItem*>::iterator it = local_ctx.items->begin(), end = local_ctx.items->end(); it != end; ++it)
		WriteItem(s, **it);
	// krew
	word ile2 = (word)local_ctx.bloods->size();
	s.Write(ile2);
	for(vector<Blood>::iterator it = local_ctx.bloods->begin(), end = local_ctx.bloods->end(); it != end; ++it)
		it->Write(s);
	// obiekty
	ile2 = (word)local_ctx.objects->size();
	s.Write(ile2);
	for(vector<Object>::iterator it = local_ctx.objects->begin(), end = local_ctx.objects->end(); it != end; ++it)
		it->Write(s);
	// skrzynie
	ile = (byte)local_ctx.chests->size();
	s.Write(ile);
	for(vector<Chest*>::iterator it = local_ctx.chests->begin(), end = local_ctx.chests->end(); it != end; ++it)
		WriteChest(s, **it);

	// portal(e)
	Portal* portal = location->portal;
	while(portal)
	{
		s.WriteCasted<byte>(1);
		s.Write((cstring)&portal->pos, sizeof(portal->pos));
		s.Write(portal->rot);
		s.WriteCasted<byte>(portal->target_loc == -1 ? 0 : 1);

		portal = portal->next_portal;
	}
	s.WriteCasted<byte>(0);

	// zapisane pociski, czary, eksplozje itp
	if(mp_load)
	{
		// pociski/czary
		ile = (byte)local_ctx.bullets->size();
		s.Write(ile);
		for(vector<Bullet>::iterator it = local_ctx.bullets->begin(), end = local_ctx.bullets->end(); it != end; ++it)
		{
			Bullet& b = *it;
			WriteStruct(s, b.pos);
			WriteStruct(s, b.rot);
			s.Write(b.speed);
			s.Write(b.yspeed);
			s.Write(b.timer);
			s.Write(b.owner ? b.owner->netid : -1);
			if(b.spell)
				s.WriteCasted<byte>(b.spell->id);
			else
				s.WriteCasted<byte>(0xFF);
		}

		// eksplozje
		ile = (byte)local_ctx.explos->size();
		s.Write(ile);
		for(vector<Explo*>::iterator it = local_ctx.explos->begin(), end = local_ctx.explos->end(); it != end; ++it)
		{
			Explo& e = **it;
			WriteString1(s, e.tex.res->filename);
			WriteStruct(s, e.pos);
			s.Write(e.size);
			s.Write(e.sizemax);
		}

		// elektro
		ile = (byte)local_ctx.electros->size();
		s.Write(ile);
		for(vector<Electro*>::iterator it = local_ctx.electros->begin(), end = local_ctx.electros->end(); it != end; ++it)
		{
			Electro& e = **it;
			s.Write(e.netid);
			s.WriteCasted<byte>(e.lines.size());
			for(vector<ElectroLine>::iterator it2 = e.lines.begin(), end2 = e.lines.end(); it2 != end2; ++it2)
			{
				// wystarczy³o by zapisywaæ koniec tylko dla ostatniej linii, ale nie chce mi siê kombinowaæ, mo¿e kiedyœ doda siê t¹ optymalizacje :P
				WriteStruct(s, it2->pts.front());
				WriteStruct(s, it2->pts.back());
				s.Write(it2->t);
			}
		}
	}
}

//=================================================================================================
void Game::WriteUnit(BitStream& s, Unit& unit)
{
	WriteString1(s, unit.data->id);
	s.Write(unit.netid);
	if(unit.type == Unit::HUMAN)
	{
		s.WriteCasted<byte>(unit.human_data->hair);
		s.WriteCasted<byte>(unit.human_data->beard);
		s.WriteCasted<byte>(unit.human_data->mustache);
		s.Write((cstring)&unit.human_data->hair_color, sizeof(unit.human_data->hair_color));
		s.Write(unit.human_data->height);
	}
	// przedmioty
	if(unit.type != Unit::ANIMAL)
	{
		byte zero = 0;
		if(unit.HaveWeapon())
			WriteString1(s, unit.GetWeapon().id);
		else
			s.Write(zero);
		if(unit.HaveBow())
			WriteString1(s, unit.GetBow().id);
		else
			s.Write(zero);
		if(unit.HaveShield())
			WriteString1(s, unit.GetShield().id);
		else
			s.Write(zero);
		if(unit.HaveArmor())
			WriteString1(s, unit.GetArmor().id);
		else
			s.Write(zero);
	}
	s.WriteCasted<byte>(unit.live_state);
	s.Write((cstring)&unit.pos, sizeof(unit.pos));
	s.Write(unit.rot);
	s.Write(unit.hp);
	s.Write(unit.hpmax);
	s.Write(unit.netid);
	s.WriteCasted<char>(unit.in_arena);
	byte b;
	if(unit.IsHero())
		b = 1;
	else if(unit.IsPlayer())
		b = 2;
	else
		b = 0;
	s.Write(b);
	if(unit.IsHero())
	{
		WriteString1(s, unit.hero->name);
		s.WriteCasted<byte>(unit.hero->clas);
		b = 0;
		if(unit.hero->know_name)
			b |= 0x01;
		if(unit.hero->team_member)
			b |= 0x02;
		s.Write(b);
		s.Write(unit.hero->credit);
	}
	else if(unit.IsPlayer())
	{
		WriteString1(s, unit.player->name);
		s.WriteCasted<byte>(unit.player->clas);
		s.WriteCasted<byte>(unit.player->id);
		s.Write(unit.player->credit);
		s.Write(unit.player->free_days);
	}
	if(unit.IsAI())
	{
		b = 0;
		if(unit.dont_attack)
			b |= 0x01;
		if(unit.assist)
			b |= 0x02;
		if(unit.ai->state != AIController::Idle)
			b |= 0x04;
		if(unit.attack_team)
			b |= 0x08;
		s.Write(b);
	}
	if(mp_load)
	{
		s.Write(unit.netid);
		unit.ani->Write(s);
		s.WriteCasted<byte>(unit.animation);
		s.WriteCasted<byte>(unit.current_animation);
		s.WriteCasted<byte>(unit.animation_state);
		s.WriteCasted<byte>(unit.attack_id);
		s.WriteCasted<byte>(unit.action);
		s.WriteCasted<byte>(unit.weapon_taken);
		s.WriteCasted<byte>(unit.weapon_hiding);
		s.WriteCasted<byte>(unit.weapon_state);
		WriteStruct(s, unit.target_pos);
		WriteStruct(s, unit.target_pos2);
		s.Write(unit.timer);
		if(unit.used_item)
			WriteString1(s, unit.used_item->id);
		else
			s.WriteCasted<byte>(0);
		int netid = (unit.useable ? unit.useable->netid : -1);
		s.Write(netid);
	}
}

//=================================================================================================
void Game::WriteDoor(BitStream& s, Door& door)
{
	s.Write((cstring)&door.pos, sizeof(door.pos));
	s.Write(door.rot);
	s.Write((cstring)&door.pt, sizeof(door.pt));
	s.WriteCasted<byte>(door.locked);
	s.WriteCasted<byte>(door.state);
	s.Write(door.netid);
	WriteBool(s, door.door2);
}

//=================================================================================================
void Game::WriteItem(BitStream& s, GroundItem& item)
{
	s.Write(item.netid);
	s.Write((cstring)&item.pos, sizeof(item.pos));
	s.Write(item.rot);
	s.WriteCasted<byte>(item.count);
	s.WriteCasted<byte>(item.team_count);
	WriteString1(s, item.item->id);
	if(item.item->IsQuest())
		s.Write(item.item->refid);
}

//=================================================================================================
void Game::WriteChest(BitStream& s, Chest& chest)
{
	s.Write((cstring)&chest.pos, sizeof(chest.pos));
	s.Write(chest.rot);
	s.Write(chest.netid);
}

//=================================================================================================
void Game::WriteTrap(BitStream& s, Trap& trap)
{
	s.WriteCasted<byte>(trap.base->type);
	s.WriteCasted<byte>(trap.state);
	s.WriteCasted<byte>(trap.dir);
	s.Write(trap.netid);
	s.Write((cstring)&trap.tile, sizeof(trap.tile));
	s.Write((cstring)&trap.pos, sizeof(trap.pos));
	s.Write(trap.obj.rot.y);

	if(mp_load)
	{
		s.WriteCasted<byte>(trap.state);
		s.Write(trap.time);
	}
}

//=================================================================================================
#define MD "missing data (" ## STRING(__LINE__) ## ")"
cstring Game::ReadLevelData(BitStream& s)
{
	cam.Reset();
	player_rot_buf = 0.f;
	show_mp_panel = true;
	boss_level_mp = false;
	open_location = 0;

	byte b, ile;
	if(!s.Read(b))
		return MD;
	//bool all = (b==1);
	mp_load = (b == 1);

	if(!location->outside)
	{
		InsideLocation* inside = (InsideLocation*)location;
		inside->SetActiveLevel(dungeon_level);
	}
	ApplyContext(location, local_ctx);
	local_ctx_valid = true;
	city_ctx = NULL;

	// jeœli to zewnêtrzna lokacja
	if(location->outside)
	{
		SetOutsideParams();

		OutsideLocation* outside = (OutsideLocation*)location;
		int size11 = outside->size*outside->size;
		int size22 = outside->size+1;
		size22 *= size22;
		if(!outside->tiles)
			outside->tiles = new TerrainTile[size11];
		if(!outside->h)
			outside->h = new float[size22];
		if(!s.Read((char*)outside->tiles, sizeof(TerrainTile)*size11) || !s.Read((char*)outside->h, sizeof(float)*size22))
			return MD;
		ApplyTiles(outside->h, outside->tiles);
		SpawnTerrainCollider();
		if(location->type == L_CITY || location->type == L_VILLAGE)
		{
			City* city = (City*)location;
			city_ctx = city;
			byte count;
			if(!ReadBool(s, city->have_exit) || !s.Read(count))
				return MD;
			city->entry_points.resize(count);
			for(vector<EntryPoint>::iterator entry_it = city->entry_points.begin(), entry_end = city->entry_points.end(); entry_it != entry_end; ++entry_it)
			{
				if(!s.Read(entry_it->exit_area) || !s.Read(entry_it->exit_y))
					return MD;
			}
			if(!s.Read(count))
				return MD;
			city->buildings.resize(count);
			for(vector<CityBuilding>::iterator it = city->buildings.begin(), end = city->buildings.end(); it != end; ++it)
			{
				if(!s.ReadCasted<byte>(it->type) || !s.Read(it->pt.x) || !s.Read(it->pt.y) || !s.ReadCasted<byte>(it->rot))
					return MD;
			}
			if(!s.Read(count))
				return MD;
			city->inside_buildings.resize(count);
			int index = 0;
			for(vector<InsideBuilding*>::iterator it = city->inside_buildings.begin(), end = city->inside_buildings.end(); it != end; ++it, ++index)
			{
				*it = new InsideBuilding;
				InsideBuilding& ib = **it;
				ApplyContext(*it, (*it)->ctx);
				(*it)->ctx.building_id = index;
				if(!s.Read(ib.level_shift.x) || !s.Read(ib.level_shift.y) || !s.ReadCasted<byte>(ib.type))
					return MD;
				ib.offset = VEC2(512.f*ib.level_shift.x+256.f, 512.f*ib.level_shift.y+256.f);
				ProcessBuildingObjects((*it)->ctx, city_ctx, &ib, buildings[ib.type].inside_mesh, NULL, 0, 0, VEC3(ib.offset.x,0,ib.offset.y), ib.type, NULL, true);
				// u¿ywalne obiekty
				if(!s.Read(ile))
					return MD;
				ib.useables.resize(ile);
				for(vector<Useable*>::iterator it2 = ib.useables.begin(), end2 = ib.useables.end(); it2 != end2; ++it2)
				{
					*it2 = new Useable;
					if(!(*it2)->Read(s))
						return MD;
				}
				// jednostki
				if(!s.Read(ile))
					return MD;
				ib.units.resize(ile);
				for(vector<Unit*>::iterator it2 = ib.units.begin(), end2 = ib.units.end(); it2 != end2; ++it2)
				{
					*it2 = new Unit;
					if(!ReadUnit(s, **it2))
						return MD;
					(*it2)->in_building = index;
				}
				// drzwi
				if(!s.Read(ile))
					return MD;
				ib.doors.resize(ile);
				for(vector<Door*>::iterator it2 = ib.doors.begin(), end2 = ib.doors.end(); it2 != end2; ++it2)
				{
					*it2 = new Door;
					if(!ReadDoor(s, **it2))
						return MD;
				}
				// przedmioty na ziemi
				if(!s.Read(ile))
					return MD;
				ib.items.resize(ile);
				for(vector<GroundItem*>::iterator it2 = ib.items.begin(), end2 = ib.items.end(); it2 != end2; ++it2)
				{
					*it2 = new GroundItem;
					if(!ReadItem(s, **it2))
						return MD;
				}
				// krew
				word ile2;
				if(!s.Read(ile2))
					return MD;
				ib.bloods.resize(ile2);
				for(vector<Blood>::iterator it2 = ib.bloods.begin(), end2 = ib.bloods.end(); it2 != end2; ++it2)
				{
					if(!it2->Read(s))
						return MD;
				}
				// obiekty
				if(!s.Read(ile))
					return MD;
				ib.objects.resize(ile);
				for(vector<Object>::iterator it2 = ib.objects.begin(), end2 = ib.objects.end(); it2 != end2; ++it2)
				{
					if(!it2->Read(s))
						return MD;
				}
				// œwiat³a
				if(!s.Read(ile))
					return MD;
				ib.lights.resize(ile);
				for(vector<Light>::iterator it2 = ib.lights.begin(), end2 = ib.lights.end(); it2 != end2; ++it2)
				{
					if(!it2->Read(s))
						return MD;
				}
				// zmienne
				if(!s.Read((char*)&ib.xsphere_pos, sizeof(ib.xsphere_pos)) ||
					!s.Read((char*)&ib.enter_area, sizeof(ib.enter_area)) ||
					!s.Read((char*)&ib.exit_area, sizeof(ib.exit_area)) ||
					!s.Read(ib.top) ||
					!s.Read(ib.xsphere_radius) ||
					!s.ReadCasted<byte>(ib.type) ||
					!s.Read(ib.enter_y))
					return MD;
			}

			SpawnCityPhysics();
			RespawnBuildingPhysics();
			CreateCityMinimap();
		}
		else
			CreateForestMinimap();
		if(!s.Read(light_angle))
			return MD;
		SpawnOutsideBariers();
	}
	else
	{
		InsideLocation* inside = (InsideLocation*)location;
		InsideLocationLevel& lvl = inside->GetLevelData();
		if(!s.ReadCasted<byte>(inside->target))
			return MD;
		byte from_portal;
		if(!s.Read(from_portal))
			return MD;
		inside->from_portal = (from_portal == 1);
		// mapa
		if(!s.ReadCasted<byte>(lvl.w))
			return MD;
		lvl.h = lvl.w;
		if(!lvl.map)
			lvl.map = new Pole[lvl.w*lvl.h];
		if(!s.Read((char*)lvl.map, sizeof(Pole)*lvl.w*lvl.h))
			return MD;
		// œwiat³a
		byte ile;
		if(!s.Read(ile))
			return MD;
		lvl.lights.resize(ile);
		for(vector<Light>::iterator it = lvl.lights.begin(), end = lvl.lights.end(); it != end; ++it)
		{
			if(!it->Read(s))
				return MD;
		}
		// rooms
		if(!s.Read(ile))
			return MD;
		lvl.rooms.resize(ile);
		for(vector<Room>::iterator it = lvl.rooms.begin(), end = lvl.rooms.end(); it != end; ++it)
		{
			if(!it->Read(s))
				return MD;
		}
		// pu³apki
		if(!s.Read(ile))
			return MD;
		lvl.traps.resize(ile);
		for(vector<Trap*>::iterator it = lvl.traps.begin(), end = lvl.traps.end(); it != end; ++it)
		{
			*it = new Trap;
			if(!ReadTrap(s, **it))
				return MD;
		}
		// drzwi
		if(!s.Read(ile))
			return MD;
		lvl.doors.resize(ile);
		for(vector<Door*>::iterator it = lvl.doors.begin(), end = lvl.doors.end(); it != end; ++it)
		{
			*it = new Door;
			if(!ReadDoor(s, **it))
				return MD;
		}
		// schody
		if( !ReadStruct(s, lvl.staircase_up) ||
			!ReadStruct(s, lvl.staircase_down) ||
			!s.ReadCasted<byte>(lvl.staircase_up_dir) ||
			!s.ReadCasted<byte>(lvl.staircase_down_dir) ||
			!ReadBool(s, lvl.staircase_down_in_wall))
			return MD;

		BaseLocation& base = g_base_locations[inside->target];
		SetDungeonParamsAndTextures(base);

		SpawnDungeonColliders();
		CreateDungeonMinimap();
	}

	// u¿ywalne obiekty
	if(!s.Read(ile))
		return MD;
	local_ctx.useables->resize(ile);
	for(vector<Useable*>::iterator it = local_ctx.useables->begin(), end = local_ctx.useables->end(); it != end; ++it)
	{
		*it = new Useable;
		if(!(*it)->Read(s))
			return MD;
	}
	// jednostki
	if(!s.Read(ile))
		return MD;
	local_ctx.units->resize(ile);
	for(vector<Unit*>::iterator it = local_ctx.units->begin(), end = local_ctx.units->end(); it != end; ++it)
	{
		*it = new Unit;
		if(!ReadUnit(s, **it))
			return MD;
	}
	// przedmioty na ziemi
	if(!s.Read(ile))
		return MD;
	local_ctx.items->resize(ile);
	for(vector<GroundItem*>::iterator it = local_ctx.items->begin(), end = local_ctx.items->end(); it != end; ++it)
	{
		*it = new GroundItem;
		if(!ReadItem(s, **it))
			return MD;
	}
	// krew
	word ile2;
	if(!s.Read(ile2))
		return MD;
	local_ctx.bloods->resize(ile2);
	for(vector<Blood>::iterator it = local_ctx.bloods->begin(), end = local_ctx.bloods->end(); it != end; ++it)
	{
		if(!it->Read(s))
			return MD;
	}
	// obiekty
	if(!s.Read(ile2))
		return MD;
	local_ctx.objects->resize(ile2);
	for(vector<Object>::iterator it = local_ctx.objects->begin(), end = local_ctx.objects->end(); it != end; ++it)
	{
		if(!it->Read(s))
			return MD;
	}
	// skrzynie
	if(!s.Read(ile))
		return MD;
	local_ctx.chests->resize(ile);
	for(vector<Chest*>::iterator it = local_ctx.chests->begin(), end = local_ctx.chests->end(); it != end; ++it)
	{
		*it = new Chest;
		if(!ReadChest(s, **it))
			return MD;
	}

	// portal(e)
	location->portal = NULL;
	Portal* portal = NULL;
	do 
	{
		byte co;
		if(!s.Read(co))
			return MD;
		if(co == 1)
		{
			if(!portal)
			{
				portal = new Portal;
				location->portal = portal;
			}
			else
			{
				portal->next_portal = new Portal;
				portal = portal->next_portal;
			}

			portal->next_portal = NULL;

			if(	!s.Read((char*)&portal->pos, sizeof(portal->pos)) ||
				!s.Read(portal->rot) ||
				!s.Read(co))
				return MD;

			portal->target_loc = (co == 1 ? 0 : -1);
		}
		else
			break;
	}
	while(1);

	if(mp_load)
	{
		// pociski/czary
		if(!s.Read(ile))
			return MD;
		local_ctx.bullets->resize(ile);
		for(vector<Bullet>::iterator it = local_ctx.bullets->begin(), end = local_ctx.bullets->end(); it != end; ++it)
		{
			Bullet& b = *it;
			byte spel;
			int netid;
			if(	!ReadStruct(s, b.pos) ||
				!ReadStruct(s, b.rot) ||
				!s.Read(b.speed) ||
				!s.Read(b.yspeed) ||
				!s.Read(b.timer) ||
				!s.Read(netid) ||
				!s.Read(spel))
				return MD;
			if(spel == 0xFF)
			{
				b.spell = NULL;
				b.mesh = aArrow;
				b.pe = NULL;
				b.remove = false;
				b.tex = NULL;
				b.tex_size = 0.f;

				TrailParticleEmitter* tpe = new TrailParticleEmitter;
				tpe->fade = 0.3f;
				tpe->color1 = VEC4(1,1,1,0.5f);
				tpe->color2 = VEC4(1,1,1,0);
				tpe->Init(50);
				local_ctx.tpes->push_back(tpe);
				b.trail = tpe;

				TrailParticleEmitter* tpe2 = new TrailParticleEmitter;
				tpe2->fade = 0.3f;
				tpe2->color1 = VEC4(1,1,1,0.5f);
				tpe2->color2 = VEC4(1,1,1,0);
				tpe2->Init(50);
				local_ctx.tpes->push_back(tpe2);
				b.trail2 = tpe2;
			}
			else
			{
				b.spell = &g_spells[spel];
				Spell& spell = g_spells[spel];
				b.mesh = spell.mesh;
				b.tex = spell.tex;
				b.tex_size = spell.size;
				b.remove = false;
				b.trail = NULL;
				b.trail2 = NULL;
				b.pe = NULL;

				if(spell.tex_particle)
				{
					ParticleEmitter* pe = new ParticleEmitter;
					pe->tex = spell.tex_particle;
					pe->emision_interval = 0.1f;
					pe->life = -1;
					pe->particle_life = 0.5f;
					pe->emisions = -1;
					pe->spawn_min = 3;
					pe->spawn_max = 4;
					pe->max_particles = 50;
					pe->pos = b.pos;
					pe->speed_min = VEC3(-1,-1,-1);
					pe->speed_max = VEC3(1,1,1);
					pe->pos_min = VEC3(-spell.size, -spell.size, -spell.size);
					pe->pos_max = VEC3(spell.size, spell.size, spell.size);
					pe->size = spell.size_particle;
					pe->op_size = POP_LINEAR_SHRINK;
					pe->alpha = 1.f;
					pe->op_alpha = POP_LINEAR_SHRINK;
					pe->mode = 1;
					pe->Init();
					local_ctx.pes->push_back(pe);
					b.pe = pe;
				}
			}
			b.owner = (netid == -1 ? NULL : FindUnit(netid));
		}

		// eksplozje
		if(!s.Read(ile))
			return MD;
		local_ctx.explos->resize(ile);
		for(vector<Explo*>::iterator it = local_ctx.explos->begin(), end = local_ctx.explos->end(); it != end; ++it)
		{
			*it = new Explo;
			Explo& e = **it;
			if( !ReadString1(s) ||
				!ReadStruct(s, e.pos) ||
				!s.Read(e.size) ||
				!s.Read(e.sizemax))
				return MD;

			e.tex = LoadTex2(BUF);
		}

		// elektro
		if(!s.Read(ile))
			return MD;
		local_ctx.electros->resize(ile);
		Spell* electro = FindSpell("thunder_bolt");
		for(vector<Electro*>::iterator it = local_ctx.electros->begin(), end = local_ctx.electros->end(); it != end; ++it)
		{
			*it = new Electro;
			Electro& e = **it;
			e.spell = electro;
			e.valid = true;
			if(!s.Read(e.netid) || !s.Read(ile))
				return MD;
			e.lines.resize(ile);
			for(byte i=0; i<ile; ++i)
			{
				VEC3 from, to;
				float t;
				if(	!ReadStruct(s, from) ||
					!ReadStruct(s, to) ||
					!s.Read(t))
					return MD;
				e.AddLine(from, to);
				e.lines.back().t = t;
			}
		}
	}

	RespawnObjectColliders();
	local_ctx_valid = true;
	if(!boss_level_mp)
		SetMusic();

	InitQuadTree();

	return NULL;
}

//=================================================================================================
bool Game::ReadUnit(BitStream& s, Unit& unit)
{
	if(!ReadString1(s) || !s.Read(unit.netid))
		return false;

	unit.data = FindUnitData(BUF, false);
	if(!unit.data)
	{
		ERROR(Format("Missing base unit id '%s'!", BUF));
		return false;
	}

	if(IS_SET(unit.data->flags, F_HUMAN))
		unit.type = Unit::HUMAN;
	else if(IS_SET(unit.data->flags, F_HUMANOID))
		unit.type = Unit::HUMANOID;
	else
		unit.type = Unit::ANIMAL;

	if(unit.type == Unit::HUMAN)
	{
		unit.human_data = new Human;
		if(!s.ReadCasted<byte>(unit.human_data->hair) ||
			!s.ReadCasted<byte>(unit.human_data->beard) ||
			!s.ReadCasted<byte>(unit.human_data->mustache) ||
			!s.Read((char*)&unit.human_data->hair_color, sizeof(unit.human_data->hair_color)) ||
			!s.Read(unit.human_data->height))
			return false;
		if(unit.human_data->hair == 0xFF)
			unit.human_data->hair = -1;
		if(unit.human_data->beard == 0xFF)
			unit.human_data->beard = -1;
		if(unit.human_data->mustache == 0xFF)
			unit.human_data->mustache = -1;
		if(unit.human_data->hair >= MAX_HAIR ||
			unit.human_data->beard >= MAX_BEARD ||
			unit.human_data->mustache >= MAX_MUSTACHE ||
			!in_range(unit.human_data->height, 0.85f, 1.15f))
			return false;
		unit.ani = new AnimeshInstance(aHumanBase);
		unit.human_data->ApplyScale(aHumanBase);
	}
	else
	{
		unit.ani = new AnimeshInstance(unit.data->ani);
		unit.human_data = NULL;
	}
	// przedmioty
	if(unit.type != Unit::ANIMAL)
	{
		for(int i=0; i<SLOT_MAX; ++i)
		{
			// broñ
			if(!ReadString1(s))
				return false;
			if(!BUF[0])
				unit.slots[i] = NULL;
			else
			{
				const Item* item = FindItem(BUF);
				if(item && ItemTypeToSlot(item->type) == (ITEM_SLOT)i)
					unit.slots[i] = item;
				else
					return false;
			}
		}
	}
	else
	{
		for(int i=0; i<SLOT_MAX; ++i)
			unit.slots[i] = NULL;
	}
	// zmienne
	if(!s.ReadCasted<byte>(unit.live_state) ||
		!s.Read((char*)&unit.pos, sizeof(unit.pos)) ||
		!s.Read(unit.rot) ||
		!s.Read(unit.hp) ||
		!s.Read(unit.hpmax) ||
		!s.Read(unit.netid) ||
		!s.ReadCasted<char>(unit.in_arena))
		return false;
	// animation
	if(unit.IsAlive())
	{
		unit.ani->Play(NAMES::ani_stand, PLAY_PRIO1, 0);
		unit.animation = unit.current_animation = ANI_STAND;
	}
	else
	{
		unit.ani->Play(NAMES::ani_die, PLAY_PRIO1, 0);
		unit.animation = unit.current_animation = ANI_DIE;
	}
	unit.ani->SetToEnd();
	unit.ani->need_update = true;
	if(unit.ani->ani->head.n_groups > 1)
		unit.ani->groups[1].state = 0;
	unit.ani->ptr = &unit;
	// heros/gracz
	byte b;
	if(!s.Read(b))
		return false;
	if(b == 1) // bohater
	{
		unit.ai = (AIController*)1; // (X_X)
		unit.player = NULL;
		unit.hero = new HeroData;
		unit.hero->unit = &unit;
		if(!ReadString1(s, unit.hero->name) ||
			!s.ReadCasted<byte>(unit.hero->clas) ||
			!s.Read(b) ||
			!s.Read(unit.hero->credit))
			return false;
		unit.hero->know_name = IS_SET(b, 0x01);
		unit.hero->team_member = IS_SET(b, 0x02);
	}
	else if(b == 2) // gracz
	{
		unit.ai = NULL;
		unit.hero = NULL;
		unit.player = new PlayerController;
		unit.player->unit = &unit;
		if(!ReadString1(s, unit.player->name) ||
			!s.ReadCasted<byte>(unit.player->clas) ||
			!s.ReadCasted<byte>(unit.player->id) ||
			!s.Read(unit.player->credit) ||
			!s.Read(unit.player->free_days))
			return false;
		GetPlayerInfo(unit.player->id).u = &unit;
	}
	else // ai, nie bohater
	{
		unit.ai = (AIController*)1; // (X_X)
		unit.hero = NULL;
		unit.player = NULL;
	}

	if(unit.IsAI())
	{
		if(!s.ReadCasted<byte>(unit.ai_mode))
			return false;
	}

	unit.action = A_NONE;
	unit.weapon_taken = W_NONE;
	unit.weapon_hiding = W_NONE;
	unit.weapon_state = WS_HIDDEN;
	unit.talking = false;
	unit.busy = Unit::Busy_No;
	unit.in_building = -1;
	unit.frozen = 0;
	unit.useable = NULL;
	unit.used_item = NULL;
	unit.bow_instance = NULL;
	unit.ai = NULL;
	unit.animation = ANI_STAND;
	unit.current_animation = ANI_STAND;
	unit.timer = 0.f;
	unit.to_remove = false;
	unit.bubble = NULL;
	unit.interp = interpolators.Get();
	unit.interp->Reset(unit.pos, unit.rot);
	unit.visual_pos = unit.pos;
	unit.animation_state = 0;

	if(mp_load)
	{
		int useable_netid;
		if( s.Read(unit.netid) &&
			unit.ani->Read(s) &&
			s.ReadCasted<byte>(unit.animation) &&
			s.ReadCasted<byte>(unit.current_animation) &&
			s.ReadCasted<byte>(unit.animation_state) &&
			s.ReadCasted<byte>(unit.attack_id) &&
			s.ReadCasted<byte>(unit.action) &&
			s.ReadCasted<byte>(unit.weapon_taken) &&
			s.ReadCasted<byte>(unit.weapon_hiding) &&
			s.ReadCasted<byte>(unit.weapon_state) &&
			ReadStruct(s, unit.target_pos) &&
			ReadStruct(s, unit.target_pos2) &&
			s.Read(unit.timer) &&
			ReadString1(s) &&
			s.Read(useable_netid))
		{
			unit.used_item = (BUF[0] ? FindItem(BUF) : NULL);
			if(useable_netid == -1)
				unit.useable = NULL;
			else
			{
				unit.useable = FindUseable(useable_netid);
				if(unit.useable)
				{
					BaseUsable& base = g_base_usables[unit.useable->type];
					unit.use_rot = lookat_angle(unit.pos, unit.useable->pos);
					unit.useable->user = &unit;
				}
				else
					ERROR(Format("Can't find useable %d.", useable_netid));
			}

			if(unit.action == A_SHOOT)
			{
				vector<AnimeshInstance*>& bow_instances = Game::Get().bow_instances;
				if(bow_instances.empty())
					unit.bow_instance = new AnimeshInstance(unit.GetBow().ani);
				else
				{
					unit.bow_instance = bow_instances.back();
					bow_instances.pop_back();
					unit.bow_instance->ani = unit.GetBow().ani;
				}
				unit.bow_instance->Play(&unit.bow_instance->ani->anims[0], PLAY_ONCE|PLAY_PRIO1|PLAY_NO_BLEND, 0);
				unit.bow_instance->groups[0].speed = unit.ani->groups[1].speed;
				unit.bow_instance->groups[0].time = unit.ani->groups[1].time;
			}
		}
		else
			return false;		
	}

	// fizyka
	btCapsuleShape* caps = new btCapsuleShape(unit.GetUnitRadius(), max(MIN_H, unit.GetUnitHeight()));
	unit.cobj = new btCollisionObject;
	unit.cobj->setCollisionShape(caps);
	unit.cobj->setUserPointer(this);
	unit.cobj->setCollisionFlags(CG_UNIT);
	phy_world->addCollisionObject(unit.cobj);
	UpdateUnitPhysics(unit, unit.IsAlive() ? unit.pos : VEC3(1000,1000,1000));

	// muzyka bossa
	if(IS_SET(unit.data->flags2, F2_BOSS) && !boss_level_mp)
	{
		boss_level_mp = true;
		SetMusic();
	}

	unit.prev_pos = unit.pos;
	unit.speed = unit.prev_speed = 0.f;
	unit.talking = false;

	return true;
}

//=================================================================================================
bool Game::ReadDoor(BitStream& s, Door& door)
{
	if(!s.Read((char*)&door.pos, sizeof(door.pos)) ||
		!s.Read(door.rot) ||
		!s.Read((char*)&door.pt, sizeof(door.pt)) ||
		!s.ReadCasted<byte>(door.locked) ||
		!s.ReadCasted<byte>(door.state) ||
		!s.Read(door.netid) ||
		!ReadBool(s, door.door2))
		return false;

	door.ani = new AnimeshInstance(door.door2 ? aDrzwi2 : aDrzwi);
	door.ani->groups[0].speed = 2.f;
	door.phy = new btCollisionObject;
	door.phy->setCollisionShape(shape_door);

	btTransform& tr = door.phy->getWorldTransform();
	VEC3 pos = door.pos;
	pos.y += 1.319f;
	tr.setOrigin(ToVector3(pos));
	tr.setRotation(btQuaternion(door.rot, 0, 0));
	phy_world->addCollisionObject(door.phy);

	if(door.state == Door::Open)
	{
		btVector3& pos = door.phy->getWorldTransform().getOrigin();
		pos.setY(pos.y() - 100.f);
		door.ani->SetToEnd(door.ani->ani->anims[0].name.c_str());
	}

	return true;
}

//=================================================================================================
bool Game::ReadItem(BitStream& s, GroundItem& item)
{
	if(	!s.Read(item.netid) ||
		!s.Read((char*)&item.pos, sizeof(item.pos)) ||
		!s.Read(item.rot) || 
		!s.ReadCasted<byte>(item.count) ||
		!s.ReadCasted<byte>(item.team_count) ||
		ReadItemAndFind(s, item.item) == -1)
		return false;
	else
		return true;
}

//=================================================================================================
bool Game::ReadChest(BitStream& s, Chest& chest)
{
	if( !s.Read((char*)&chest.pos, sizeof(chest.pos)) ||
		!s.Read(chest.rot) ||
		!s.Read(chest.netid))
		return false;
	chest.ani = new AnimeshInstance(aSkrzynia);
	return true;
}

//=================================================================================================
bool Game::ReadTrap(BitStream& s, Trap& trap)
{
	TRAP_TYPE type;
	if(	!s.ReadCasted<byte>(type) ||
		!s.ReadCasted<byte>(trap.state) ||
		!s.ReadCasted<byte>(trap.dir) ||
		!s.Read(trap.netid) ||
		!s.Read((char*)&trap.tile, sizeof(trap.tile)) ||
		!s.Read((char*)&trap.pos, sizeof(trap.pos)) ||
		!s.Read(trap.obj.rot.y))
		return false;
	trap.base = &g_traps[type];

	trap.state = 0;
	trap.obj.base = NULL;
	trap.obj.mesh = trap.base->mesh;
	trap.obj.pos = trap.pos;
	trap.obj.scale = 1.f;
	trap.obj.rot.x = trap.obj.rot.z = 0;
	trap.trigger = false;
	trap.hitted = NULL;

	if(mp_load)
	{
		if(	!s.ReadCasted<byte>(trap.state) ||
			!s.Read(trap.time))
			return false;
	}

	if(type == TRAP_ARROW || type == TRAP_POISON)
		trap.obj.rot = VEC3(0,0,0);
	else if(type == TRAP_SPEAR)
	{
		trap.obj2.base = NULL;
		trap.obj2.mesh = trap.base->mesh2;
		trap.obj2.pos = trap.obj.pos;
		trap.obj2.rot = trap.obj.rot;
		trap.obj2.scale = 1.f;
		trap.obj2.pos.y -= 2.f;
		trap.hitted = NULL;
	}
	else
		trap.obj.base = &obj_alpha;

	return true;
}

//=================================================================================================
void Game::SendPlayerData(int index)
{
	PlayerInfo& info = game_players[index];
	Unit& u = *info.u;

	net_stream2.Reset();
	net_stream2.Write(ID_PLAYER_DATA2);
	net_stream2.Write(u.netid);
	// przedmioty
	for(int i=0; i<SLOT_MAX; ++i)
	{
		if(u.slots[i])
			WriteString1(net_stream2, u.slots[i]->id);
		else
			net_stream2.WriteCasted<byte>(0);
	}
	byte b = (byte)u.items.size();
	net_stream2.Write(b);
	for(vector<ItemSlot>::iterator it = u.items.begin(), end = u.items.end(); it != end; ++it)
	{
		ItemSlot& slot = *it;
		if(slot.item)
		{
			WriteString1(net_stream2, slot.item->id);
			if(slot.item->id[0] == '$')
				net_stream2.Write(slot.item->refid);
			net_stream2.WriteCasted<byte>(slot.count);
			net_stream2.WriteCasted<byte>(slot.team_count);
		}
		else
			net_stream2.WriteCasted<byte>(0);
	}
	// dane
	u.stats.Write(net_stream2);
	u.unmod_stats.Write(net_stream2);
	net_stream2.Write(u.gold);
	u.player->Write(net_stream2);
	// inni cz³onkowie dru¿yny
	net_stream2.WriteCasted<byte>(team.size()-1);
	for(vector<Unit*>::iterator it = team.begin(), end = team.end(); it != end; ++it)
	{
		if(*it != &u)
			net_stream2.Write((*it)->netid);
	}
	net_stream2.WriteCasted<byte>(leader_id);

	if(mp_load)
	{
		int flags = 0;
		if(u.run_attack)
			flags |= 0x01;
		if(u.used_item_is_team)
			flags |= 0x02;
		net_stream2.Write(u.attack_power);
		net_stream2.Write(u.raise_timer);
		net_stream2.WriteCasted<byte>(flags);
	}

	peer->Send(&net_stream2, HIGH_PRIORITY, RELIABLE, 0, info.adr, false);
}

//=================================================================================================
cstring Game::ReadPlayerData(BitStream& s)
{
	int netid;
	byte b;

	if(!s.Read(netid))
		return MD;

	Unit* u = FindUnit(netid);
	if(!u)
		return Format("Missing unit with netid %d.", netid);
	game_players[0].u = u;
	pc = u->player;
	pc->player_info = &game_players[0];
	game_players[0].pc = pc;

	// przedmioty
	for(int i=0; i<SLOT_MAX; ++i)
	{
		if(!ReadString1(s))
			return MD;
		if(BUF[0])
			u->slots[i] = FindItem(BUF);
		else
			u->slots[i] = NULL;
	}
	if(!s.Read(b))
		return MD;
	u->items.resize(b);
	for(vector<ItemSlot>::iterator it = u->items.begin(), end = u->items.end(); it != end; ++it)
	{
		int w = ReadItemAndFind(s, it->item);
		if(w == -1)
			return MD;
		else if(w != 0)
		{
			if(	!s.ReadCasted<byte>(it->count) ||
				!s.ReadCasted<byte>(it->team_count))
				return MD;
		}
	}

	int credit = u->player->credit,
		free_days = u->player->free_days;

	u->player->Init(*u, true);

	if(	!u->stats.Read(s) ||
		!u->unmod_stats.Read(s) ||
		!s.Read(u->gold) ||
		!pc->Read(s))
		return MD;

	u->look_target = NULL;
	u->prev_speed = 0.f;
	u->run_attack = false;

	u->weight = 0;
	u->CalculateLoad();
	u->RecalculateWeight();

	u->player->credit = credit;
	u->player->free_days = free_days;
	u->player->is_local = true;

	// inni cz³onkowie dru¿yny
	team.clear();
	active_team.clear();
	team.push_back(u);
	active_team.push_back(u);
	byte ile;
	if(!s.Read(ile))
		return MD;
	for(byte i=0; i<ile; ++i)
	{
		int netid;
		if(!s.Read(netid))
			return MD;
		Unit* u2 = FindUnit(netid);
		if(!u2)
			return Format("Missing unit with netid %d.", netid);
		team.push_back(u2);
		if(u2->IsPlayer() || !u2->hero->free)
			active_team.push_back(u2);
	}
	if(!s.ReadCasted<byte>(leader_id))
		return MD;
	leader = GetPlayerInfo(leader_id).u;

	dialog_context.pc = u->player;
	pc->noclip = noclip;
	pc->godmode = godmode;
	pc->unit->invisible = invisible;

	if(mp_load)
	{
		int flags;
		if( !s.Read(u->attack_power) ||
			!s.Read(u->raise_timer) ||
			!s.ReadCasted<byte>(flags))
			return MD;
		u->run_attack = IS_SET(flags, 0x01);
		u->used_item_is_team = IS_SET(flags, 0x02);
	}

	mp_load = false;

	return NULL;
}

//=================================================================================================
Unit* Game::FindUnit(int netid)
{
	if(netid == -1)
		return NULL;

	for(vector<Unit*>::iterator it = local_ctx.units->begin(), end = local_ctx.units->end(); it != end; ++it)
	{
		if((*it)->netid == netid)
			return *it;
	}

	if(city_ctx)
	{
		for(vector<InsideBuilding*>::iterator it = city_ctx->inside_buildings.begin(), end = city_ctx->inside_buildings.end(); it != end; ++it)
		{
			for(vector<Unit*>::iterator it2 = (*it)->units.begin(), end2 = (*it)->units.end(); it2 != end2; ++it2)
			{
				if((*it2)->netid == netid)
					return *it2;
			}
		}
	}

	return NULL;
}

//=================================================================================================
void Game::UpdateServer(float dt)
{
	if(game_state == GS_LEVEL)
	{
		InterpolatePlayers(dt);
		pc->unit->changed = true;
	}

	Packet* packet;
	for(packet=peer->Receive(); packet; peer->DeallocatePacket(packet), packet=peer->Receive())
	{
		BitStream& stream = StreamStart(packet, Stream_UpdateGameServer);
		int player_index = FindPlayerIndex(packet->systemAddress);
		if(player_index == -1)
		{
ignore_him:
			LOG(Format("Ignoring packet from %s.", packet->systemAddress.ToString()));
			StreamEnd();
			continue;
		}

		PlayerInfo& info = game_players[player_index];
		if(info.left)
			goto ignore_him;

		byte msg_id;
		stream.Read(msg_id);

		switch(msg_id)
		{
		case ID_CONNECTION_LOST:
		case ID_DISCONNECTION_NOTIFICATION:
			LOG(Format(msg_id == ID_CONNECTION_LOST ? "Lost connection with player %s." : "Player %s has disconnected.", info.name.c_str()));
			players_left.push_back(info.id);
			info.left = true;
			info.left_reason = PlayerInfo::LEFT_QUIT;
			break;
		case ID_SAY:
			Server_Say(stream, info, packet);
			break;
		case ID_WHISPER:
			Server_Whisper(stream, info, packet);
			break;
		case ID_CONTROL:
			if(!ProcessControlMessageServer(stream, info))
			{
				peer->DeallocatePacket(packet);
				return;
			}
			break;
		default:
			WARN(Format("UpdateServer: Unknown packet from %s: %u.", info.name.c_str(), msg_id));
			StreamEnd(false);
			break;
		}

		StreamEnd();
	}

	ProcessLeftPlayers();

	update_timer += dt;
	if(update_timer >= TICK && players > 1)
	{
		bool last_anyone_talking = anyone_talking;
		anyone_talking = IsAnyoneTalking();
		if(last_anyone_talking != anyone_talking)
			PushNetChange(NetChange::CHANGE_FLAGS);

		update_timer = 0;
		net_stream.Reset();
		net_stream.WriteCasted<byte>(ID_CHANGES);

		// dodaj zmiany pozycji jednostek i ai_mode
		if(game_state == GS_LEVEL)
		{
			ServerProcessUnits(*local_ctx.units);
			if(city_ctx)
			{
				for(vector<InsideBuilding*>::iterator it = city_ctx->inside_buildings.begin(), end = city_ctx->inside_buildings.end(); it != end; ++it)
					ServerProcessUnits((*it)->units);
			}
		}

		// wyœlij odkryte kawa³ki minimapy
		if(!minimap_reveal_mp.empty())
		{
			if(game_state == GS_LEVEL)
				PushNetChange(NetChange::REVEAL_MINIMAP);
			else
				minimap_reveal_mp.clear();
		}

		// liczba zmian
		net_stream.WriteCasted<word>(net_changes.size());
		if(net_changes.size() >= 0xFFFF)
			ERROR(Format("Too many changes %d!", net_changes.size()));

		// zmiany
		WriteServerChanges();
		net_changes.clear();
		assert(net_talk.empty());
		peer->Send(&net_stream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, UNASSIGNED_SYSTEM_ADDRESS, true);

#ifdef _DEBUG
		int _net_player_updates = (int)net_changes_player.size();
#endif

		for(vector<PlayerInfo>::iterator it = game_players.begin(), end = game_players.end(); it != end; ++it)
		{
			PlayerInfo& info = *it;
			if(info.id == my_id || info.left)
				continue;
			// aktualizacja statystyk
			if(info.u->player->stat_flags != 0)
			{
				NetChangePlayer& c = Add1(net_changes_player);
				c.pc = info.u->player;
				c.id = c.pc->stat_flags;
				c.type = NetChangePlayer::PLAYER_STATS;
				info.NeedUpdate();
				c.pc->stat_flags = 0;
#ifdef _DEBUG
				++_net_player_updates;
#endif
			}
			// aktualizacja bufów
			int buffs = info.u->GetBuffs();
			if(buffs != info.buffs)
			{
				info.buffs = buffs;
				info.update_flags |= PlayerInfo::UF_BUFFS;
			}
			// wszystkie aktualizacje
			if(info.update_flags)
			{
				net_stream.Reset();
				net_stream.WriteCasted<byte>(ID_PLAYER_UPDATE);
				net_stream.WriteCasted<byte>(info.update_flags);
				if(IS_SET(info.update_flags, PlayerInfo::UF_POISON_DAMAGE))
					net_stream.Write(info.u->player->last_dmg_poison);
				if(IS_SET(info.update_flags, PlayerInfo::UF_NET_CHANGES))
				{
					byte ile = 0;
					BitSize_t offset = net_stream.GetNumberOfBytesUsed();
					net_stream.Write(ile);
					for(vector<NetChangePlayer>::iterator it2 = net_changes_player.begin(), end2 = net_changes_player.end(); it2 != end2; ++it2)
					{
						if(it2->pc == it->u->player)
						{
#ifdef _DEBUG
							--_net_player_updates;
#endif
							NetChangePlayer& c = *it2;
							++ile;
							net_stream.WriteCasted<byte>(c.type);
							switch(c.type)
							{
							case NetChangePlayer::PICKUP:
								net_stream.Write(c.id);
								net_stream.Write(c.ile);
								break;
							case NetChangePlayer::LOOT:
								net_stream.WriteCasted<byte>(c.id);
								if(c.id == 1)
									WriteItemListTeam(net_stream, *it->u->player->chest_trade);
								break;
							case NetChangePlayer::START_SHARE:
							case NetChangePlayer::START_GIVE:
								{
									Unit& u = *it->u->player->action_unit;
									net_stream.Write(u.weight);
									net_stream.Write(u.weight_max);
									net_stream.Write(u.gold);
									u.stats.Write(net_stream);
									WriteItemListTeam(net_stream, u.items);
								}
								break;
							case NetChangePlayer::GOLD_MSG:
								net_stream.WriteCasted<byte>(c.id);
								net_stream.Write(c.ile);
								break;
							case NetChangePlayer::START_DIALOG:
								net_stream.Write(c.id);
								break;
							case NetChangePlayer::SHOW_DIALOG_CHOICES:
								{
									DialogContext& ctx = *it->u->player->dialog_ctx;
									net_stream.WriteCasted<byte>(ctx.choices.size());
									net_stream.WriteCasted<char>(ctx.dialog_esc);
									for(vector<DialogChoice>::iterator it3 = ctx.choices.begin(), end3 = ctx.choices.end(); it3 != end3; ++it3)
										WriteString1(net_stream, it3->msg);
								}
								break;
							case NetChangePlayer::END_DIALOG:
							case NetChangePlayer::USE_USEABLE:
							case NetChangePlayer::PREPARE_WARP:
							case NetChangePlayer::ENTER_ARENA:
							case NetChangePlayer::START_ARENA_COMBAT:
							case NetChangePlayer::EXIT_ARENA:
							case NetChangePlayer::END_FALLBACK:
							case NetChangePlayer::BREAK_ACTION:
							case NetChangePlayer::ADDED_ITEM_MSG:
								break;
							case NetChangePlayer::START_TRADE:
								net_stream.Write(c.id);
								WriteItemList(net_stream, *it->u->player->chest_trade);
								break;
							case NetChangePlayer::SET_FROZEN:
							case NetChangePlayer::CHEATS:
							case NetChangePlayer::PVP:
							case NetChangePlayer::NO_PVP:
							case NetChangePlayer::CANT_LEAVE_LOCATION:
							case NetChangePlayer::REST:
							case NetChangePlayer::IS_BETTER_ITEM:
								net_stream.WriteCasted<byte>(c.id);
								break;
							case NetChangePlayer::REMOVE_QUEST_ITEM:
							case NetChangePlayer::LOOK_AT:
								net_stream.Write(c.id);
								break;
							case NetChangePlayer::ADD_ITEMS:
								{
									net_stream.Write(c.id);
									net_stream.Write(c.ile);
									WriteString1(net_stream, c.item->id);
									if(c.item->id[0] == '$')
										net_stream.Write(c.item->refid);
								}
								break;
							case NetChangePlayer::TRAIN:
								net_stream.WriteCasted<byte>(c.id);
								net_stream.WriteCasted<byte>(c.ile);
								break;
							case NetChangePlayer::UNSTUCK:
								WriteStruct(net_stream, c.pos);
								break;
							case NetChangePlayer::GOLD_RECEIVED:
								net_stream.Write(c.id);
								net_stream.Write(c.ile);
								break;
							case NetChangePlayer::GAIN_STAT:
								net_stream.WriteCasted<byte>(c.id);
								net_stream.WriteCasted<byte>(c.a);
								net_stream.WriteCasted<byte>(c.ile);
								break;
							case NetChangePlayer::ADD_ITEMS_TRADER:
								net_stream.Write(c.id);
								net_stream.Write(c.ile);
								net_stream.Write(c.a);
								WriteBaseItem(net_stream, c.item);
								break;
							case NetChangePlayer::ADD_ITEMS_CHEST:
								net_stream.Write(c.id);
								net_stream.Write(c.ile);
								net_stream.Write(c.a);
								WriteBaseItem(net_stream, c.item);
								break;
							case NetChangePlayer::REMOVE_ITEMS:
								net_stream.Write(c.id);
								net_stream.Write(c.ile);
								break;
							case NetChangePlayer::REMOVE_ITEMS_TRADER:
								net_stream.Write(c.id);
								net_stream.Write(c.ile);
								net_stream.Write(c.a);
								break;
							case NetChangePlayer::UPDATE_TRADER_GOLD:
								net_stream.Write(c.id);
								net_stream.Write(c.ile);
								break;
							case NetChangePlayer::UPDATE_TRADER_INVENTORY:
								net_stream.Write(c.unit->netid);
								WriteItemListTeam(net_stream, c.unit->items);
								break;
							case NetChangePlayer::PLAYER_STATS:
								net_stream.Write(c.id);
								if(IS_SET(c.id, STAT_KILLS))
									net_stream.Write(c.pc->kills);
								if(IS_SET(c.id, STAT_DMG_DONE))
									net_stream.Write(c.pc->dmg_done);
								if(IS_SET(c.id, STAT_DMG_TAKEN))
									net_stream.Write(c.pc->dmg_taken);
								if(IS_SET(c.id, STAT_KNOCKS))
									net_stream.Write(c.pc->knocks);
								if(IS_SET(c.id, STAT_ARENA_FIGHTS))
									net_stream.Write(c.pc->arena_fights);
								break;
							case NetChangePlayer::ADDED_ITEMS_MSG:
								net_stream.WriteCasted<byte>(c.ile);
								break;
							case NetChangePlayer::STAT_CHANGED:
							case NetChangePlayer::ADD_PERK:
								net_stream.WriteCasted<byte>(c.id);
								net_stream.Write(c.ile);
								break;
							default:
								WARN(Format("UpdateServer: Unknown player %s change %d.", it2->pc->name.c_str(), c.type));
								assert(0);
								break;
							}
						}
					}
					net_stream.GetData()[offset] = ile;
				}
				if(IS_SET(it->update_flags, PlayerInfo::UF_GOLD))
					net_stream.Write(it->u->gold);
				if(IS_SET(it->update_flags, PlayerInfo::UF_ALCOHOL))
					net_stream.Write(it->u->alcohol);
				if(IS_SET(it->update_flags, PlayerInfo::UF_BUFFS))
					net_stream.WriteCasted<byte>(it->buffs);
				peer->Send(&net_stream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, it->adr, false);
				it->update_flags = 0;
			}
		}

#ifdef _DEBUG
		for(vector<NetChangePlayer>::iterator it = net_changes_player.begin(), end = net_changes_player.end(); it != end; ++it)
		{
			if(GetPlayerInfo(it->pc).left)
				--_net_player_updates;
		}
#endif

		assert(_net_player_updates == 0);
		net_changes_player.clear();
	}
}

bool Game::ProcessControlMessageServer(BitStream& stream, PlayerInfo& info)
{
	bool move_info;
	if(!ReadBool(stream, move_info))
	{
		ERROR(Format("UpdateServer: Broken packet ID_CONTROL from %s.", info.name.c_str()));
		StreamEnd(false);
		return true;
	}

	Unit& unit = *info.u;
	PlayerController& player = *info.u->player;

	// movment/animation info
	if(move_info)
	{
		if(!info.warping && game_state == GS_LEVEL)
		{
			VEC3 new_pos;
			float rot;

			if(!ReadStruct(stream, new_pos)
				|| !stream.Read(rot)
				|| !stream.Read(unit.ani->groups[0].speed))
			{
				ERROR(Format("UpdateServer: Broken packet ID_CONTROL(2) from %s.", info.name.c_str()));
				StreamEnd(false);
				return true;
			}

			if(distance(unit.pos, new_pos) >= 10.f)
			{
				// too big change in distance, warp unit to old position
				WARN(Format("UpdateServer: Invalid unit movment from %s ((%g,%g,%g) -> (%g,%g,%g)).", info.name.c_str(), unit.pos.x, unit.pos.y, unit.pos.z,
					new_pos.x, new_pos.y, new_pos.z));
				WarpUnit(unit, unit.pos);
				unit.interp->Add(unit.pos, rot);
			}
			else
			{
				if(player.noclip || unit.useable || CheckMoveNet(unit, new_pos))
				{
					// update position
					if(!equal(unit.pos, new_pos) && !location->outside)
					{
						// reveal minimap
						INT2 new_tile(int(new_pos.x/2), int(new_pos.z/2));
						if(INT2(int(unit.pos.x/2), int(unit.pos.z/2)) != new_tile)
							DungeonReveal(new_tile);
					}
					unit.pos = new_pos;
					UpdateUnitPhysics(unit, unit.pos);
					unit.interp->Add(unit.pos, rot);
				}
				else
				{
					// player is now stuck inside something, unstick him
					unit.interp->Add(unit.pos, rot);
					NetChangePlayer& c = Add1(net_changes_player);
					c.type = NetChangePlayer::UNSTUCK;
					c.pc = &player;
					c.pos = unit.pos;
					info.NeedUpdate();
				}
			}
		}
		else
		{
			// player is warping or not in level, skip movment
			if(!SkipBitstream(stream, sizeof(VEC3)+sizeof(float)*2))
			{
				ERROR(Format("UpdateServer: Broken packet ID_CONTROL(3) from %s.", info.name.c_str()));
				StreamEnd(false);
				return true;
			}
		}

		// animation
		Animation ani;
		if(!stream.ReadCasted<byte>(ani))
		{
			ERROR(Format("UpdateServer: Broken packet ID_CONTROL(4) from %s.", info.name.c_str()));
			StreamEnd(false);
			return true;
		}
		if(unit.animation != ANI_PLAY && ani != ANI_PLAY)
			unit.animation = ani;
	}

	// count of changes
	byte changes;
	if(!stream.Read(changes))
	{
		ERROR(Format("UpdateServer: Broken packet ID_CONTROL(5) from %s.", info.name.c_str()));
		StreamEnd(false);
		return true;
	}

	// process changes
	for(byte change_i = 0; change_i<changes; ++change_i)
	{
		// change type
		NetChange::TYPE type;
		if(!stream.ReadCasted<byte>(type))
		{
			ERROR(Format("UpdateServer: Broken packet ID_CONTROL(6) from %s.", info.name.c_str()));
			StreamEnd(false);
			return true;
		}

		switch(type)
		{
		// player change equipped items
		case NetChange::CHANGE_EQUIPMENT:
			{
				int i_index;

				if(!stream.Read(i_index))
				{
					ERROR(Format("Update server: Broken CHANGE_EQUIPMENT from %s.", info.name.c_str()));
					StreamEnd(false);
				}
				else if(i_index >= 0)
				{
					// equipping item
					if(uint(i_index) >= unit.items.size())
					{
						ERROR(Format("Update server: CHANGE_EQUIPMENT from %s, invalid index %d.", info.name.c_str(), i_index));
						StreamEnd(false);
						break;
					}

					ItemSlot& slot = unit.items[i_index];
					if(!slot.item->IsWearableByHuman())
					{
						ERROR(Format("Update server: CHANGE_EQUIPMENT from %s, item at index %d (%s) is not wearable.",
							info.name.c_str(), i_index, slot.item->id.c_str()));
						StreamEnd(false);
						break;
					}

					ITEM_SLOT slot_type = ItemTypeToSlot(slot.item->type);
					if(unit.slots[slot_type])
					{
						std::swap(unit.slots[slot_type], slot.item);
						SortItems(unit.items);
					}
					else
					{
						unit.slots[slot_type] = slot.item;
						unit.items.erase(unit.items.begin()+i_index);
					}

					// send to other players
					if(players > 2)
					{
						NetChange& c = Add1(net_changes);
						c.type = NetChange::CHANGE_EQUIPMENT;
						c.unit = &unit;
						c.id = slot_type;
					}
				}
				else
				{
					// removing item
					ITEM_SLOT slot = IIndexToSlot(i_index);

					if(slot < SLOT_WEAPON || slot >= SLOT_MAX)
					{
						ERROR(Format("Update server: CHANGE_EQUIPMENT from %s, invalid slot type %d.", info.name.c_str(), slot));
						StreamEnd(false);
					}
					else if(!unit.slots[slot])
					{
						ERROR(Format("Update server: CHANGE_EQUIPMENT from %s, empty slot type %d.", info.name.c_str(), slot));
						StreamEnd(false);
					}
					else
					{
						unit.AddItem(unit.slots[slot], 1u, false);
						unit.weight -= unit.slots[slot]->weight;
						unit.slots[slot] = NULL;

						// send to other players
						if(players > 2)
						{
							NetChange& c = Add1(net_changes);
							c.type = NetChange::CHANGE_EQUIPMENT;
							c.unit = &unit;
							c.id = slot;
						}
					}
				}
			}
			break;
		// player take/hide weapon
		case NetChange::TAKE_WEAPON:
			{
				bool hide;
				WeaponType weapon_type;

				if(!ReadBool(stream, hide)
					|| !stream.ReadCasted<byte>(weapon_type))
				{
					ERROR(Format("Update server: Broken TAKE_WEAPON from %s.", info.name.c_str()));
					StreamEnd(false);
				}
				else
				{
					unit.ani->groups[1].speed = 1.f;
					SetUnitWeaponState(unit, !hide, weapon_type);
					// send to other players
					if(players > 2)
					{
						NetChange& c = Add1(net_changes);
						c.unit = &unit;
						c.type = NetChange::TAKE_WEAPON;
						c.id = (hide ? 1 : 0);
					}
				}
			}
			break;
		// player attacks
		case NetChange::ATTACK:
			{
				byte typeflags;
				float speed;

				if(!stream.Read(typeflags)
					|| !stream.Read(speed))
				{
					ERROR(Format("Update server: Broken ATTACK from %s.", info.name.c_str()));
					StreamEnd(false);
				}
				else
				{
					byte type = (typeflags & 0xF);

					// force taken weapon in hand
					unit.weapon_state = WS_TAKEN;

					switch(type)
					{
					case AID_Attack:
						if(unit.action == A_ATTACK && unit.animation_state == 0)
						{
							unit.attack_power = unit.ani->groups[1].time / unit.GetAttackFrame(0);
							unit.animation_state = 1;
							unit.ani->groups[1].speed = unit.attack_power + unit.GetAttackSpeed();
							unit.attack_power += 1.f;
						}
						else
						{
							if(sound_volume && unit.data->sounds->sound[SOUND_ATTACK] && rand2()%4 == 0)
								PlayAttachedSound(unit, unit.data->sounds->sound[SOUND_ATTACK], 1.f, 10.f);
							unit.action = A_ATTACK;
							unit.attack_id = ((typeflags & 0xF0)>>4);
							unit.attack_power = 1.f;
							unit.ani->Play(NAMES::ani_attacks[unit.attack_id], PLAY_PRIO1|PLAY_ONCE|PLAY_RESTORE, 1);
							unit.ani->groups[1].speed = speed;
							unit.animation_state = 1;
							unit.hitted = false;
						}
						unit.player->Train(TrainWhat::AttackStart, 0.f, 0);
						break;
					case AID_PowerAttack:
						{
							if(sound_volume && unit.data->sounds->sound[SOUND_ATTACK] && rand2()%4 == 0)
								PlayAttachedSound(unit, unit.data->sounds->sound[SOUND_ATTACK], 1.f, 10.f);
							unit.action = A_ATTACK;
							unit.attack_id = ((typeflags & 0xF0)>>4);
							unit.attack_power = 1.f;
							unit.ani->Play(NAMES::ani_attacks[unit.attack_id], PLAY_PRIO1|PLAY_ONCE|PLAY_RESTORE, 1);
							unit.ani->groups[1].speed = speed;
							unit.animation_state = 0;
							unit.hitted = false;
						}
						break;
					case AID_Shoot:
					case AID_StartShoot:
						if(unit.action == A_SHOOT && unit.animation_state == 0)
							unit.animation_state = 1;
						else
						{
							unit.ani->Play(NAMES::ani_shoot, PLAY_PRIO1|PLAY_ONCE|PLAY_RESTORE, 1);
							unit.ani->groups[1].speed = speed;
							unit.action = A_SHOOT;
							unit.animation_state = (type == AID_Shoot ? 1 : 0);
							unit.hitted = false;
							if(!unit.bow_instance)
							{
								if(bow_instances.empty())
									unit.bow_instance = new AnimeshInstance(unit.GetBow().ani);
								else
								{
									unit.bow_instance = bow_instances.back();
									bow_instances.pop_back();
									unit.bow_instance->ani = unit.GetBow().ani;
								}
							}
							unit.bow_instance->Play(&unit.bow_instance->ani->anims[0], PLAY_ONCE|PLAY_PRIO1|PLAY_NO_BLEND, 0);
							unit.bow_instance->groups[0].speed = unit.ani->groups[1].speed;
						}
						if(type == AID_Shoot)
						{
							if(!stream.Read(info.yspeed))
							{
								ERROR(Format("Update server: Broken ATTACK(2) from %s.", info.name.c_str()));
								StreamEnd(false);
							}
						}
						break;
					case AID_Block:
						{
							unit.action = A_BLOCK;
							unit.ani->Play(NAMES::ani_block, PLAY_PRIO1|PLAY_STOP_AT_END|PLAY_RESTORE, 1);
							unit.ani->groups[1].speed = 1.f;
							unit.ani->groups[1].blend_max = speed;
							unit.animation_state = 0;
						}
						break;
					case AID_Bash:
						{
							unit.action = A_BASH;
							unit.animation_state = 0;
							unit.ani->Play(NAMES::ani_bash, PLAY_ONCE|PLAY_PRIO1|PLAY_RESTORE, 1);
							unit.ani->groups[1].speed = 2.f;
							unit.ani->frame_end_info2 = false;
							unit.hitted = false;
							unit.player->Train(TrainWhat::BashStart, 0.f, 0);
						}
						break;
					case AID_RunningAttack:
						{
							if(sound_volume && unit.data->sounds->sound[SOUND_ATTACK] && rand2()%4 == 0)
								PlayAttachedSound(unit, unit.data->sounds->sound[SOUND_ATTACK], 1.f, 10.f);
							unit.action = A_ATTACK;
							unit.attack_id = ((typeflags & 0xF0)>>4);
							unit.attack_power = 1.5f;
							unit.run_attack = true;
							unit.ani->Play(NAMES::ani_attacks[unit.attack_id], PLAY_PRIO1|PLAY_ONCE|PLAY_RESTORE, 1);
							unit.ani->groups[1].speed = speed;
							unit.animation_state = 1;
							unit.hitted = false;
						}
						break;
					case AID_StopBlock:
						{
							unit.action = A_NONE;
							unit.ani->frame_end_info2 = false;
							unit.ani->Deactivate(1);
						}
						break;
					}

					// send to other players
					if(players > 2)
					{
						NetChange& c = Add1(net_changes);
						c.unit = &unit;
						c.type = NetChange::ATTACK;
						c.id = typeflags;
						c.f[1] = speed;
					}
				}
			}
			break;
		// player drops item
		case NetChange::DROP_ITEM:
			{
				int i_index, count;

				if(!stream.Read(i_index)
					|| !stream.Read(count))
				{
					ERROR(Format("Update server: Broken DROP_ITEM from %s.", info.name.c_str()));
					StreamEnd(false);
				}
				else if(count == 0)
				{
					ERROR(Format("Update server: DROP_ITEM from %s, count %d.", info.name.c_str(), count));
					StreamEnd(false);
				}
				else
				{
					GroundItem* item;

					if(i_index >= 0)
					{
						// dropping unequipped item
						if(i_index >= (int)unit.items.size())
						{
							ERROR(Format("Update server: DROP_ITEM from %s, invalid index %d (count %d).", info.name.c_str(), i_index, count));
							StreamEnd(false);
							break;
						}

						ItemSlot& sl = unit.items[i_index];
						if(count > (int)sl.count)
						{
							ERROR(Format("Update server: DROP_ITEM from %s, index %d (count %d) have only %d count.", info.name.c_str(), i_index,
								count, sl.count));
							StreamEnd(false);
							count = sl.count;
						}
						sl.count -= count;
						unit.weight -= sl.item->weight*count;
						item = new GroundItem;
						item->item = sl.item;
						item->count = count;
						item->team_count = min(count, (int)sl.team_count);
						sl.team_count -= item->team_count;
						if(sl.count == 0)
							unit.items.erase(unit.items.begin()+i_index);
					}
					else
					{
						// dropping equipped item
						ITEM_SLOT slot_type = IIndexToSlot(i_index);
						if(!IsValid(slot_type))
						{
							ERROR(Format("Update server: DROP_ITEM from %s, invalid slot %d.", info.name.c_str(), slot_type));
							StreamEnd(false);
							break;
						}

						const Item*& slot = unit.slots[slot_type];
						if(!slot)
						{
							ERROR(Format("Update server: DROP_ITEM from %s, empty slot %d.", info.name.c_str(), slot_type));
							StreamEnd(false);
							break;
						}

						unit.weight -= slot->weight*count;
						item = new GroundItem;
						item->item = slot;
						item->count = 1;
						item->team_count = 0;
						slot = NULL;

						// send info about changing equipment to other players
						if(players > 2)
						{
							NetChange& c = Add1(net_changes);
							c.type = NetChange::CHANGE_EQUIPMENT;
							c.unit = &unit;
							c.id = slot_type;
						}
					}

					unit.action = A_ANIMATION;
					unit.ani->Play("wyrzuca", PLAY_ONCE|PLAY_PRIO2, 0);
					unit.ani->frame_end_info = false;
					item->pos = unit.pos;
					item->pos.x -= sin(unit.rot)*0.25f;
					item->pos.z -= cos(unit.rot)*0.25f;
					item->rot = random(MAX_ANGLE);
					if(!CheckMoonStone(item, unit))
						AddGroundItem(GetContext(unit), item);

					// send to other players
					if(players > 2)
					{
						NetChange& c = Add1(net_changes);
						c.type = NetChange::DROP_ITEM;
						c.unit = &unit;
					}
				}
			}
			break;
		// player wants to pick up item
		case NetChange::PICKUP_ITEM:
			{
				int netid;
				if(!stream.Read(netid))
				{
					ERROR(Format("Update server: Broken PICKUP_ITEM from %s.", info.name.c_str()));
					StreamEnd(false);
					break;
				}

				LevelContext* ctx;
				GroundItem* item = FindItemNetid(netid, &ctx);
				if(!item)
				{
					ERROR(Format("Update server: PICKUP_ITEM from %s, missing item %d.", info.name.c_str(), netid));
					StreamEnd(false);
					break;
				}

				// add item
				unit.AddItem(item->item, item->count, item->team_count);

				// start animation
				bool up_animation = (item->pos.y > unit.pos.y+0.5f);
				unit.action = A_PICKUP;
				unit.animation = ANI_PLAY;
				unit.ani->Play(up_animation ? "podnosi_gora" : "podnosi", PLAY_ONCE|PLAY_PRIO2, 0);
				unit.ani->frame_end_info = false;

				// send pickup acceptation
				NetChangePlayer& c = Add1(net_changes_player);
				c.pc = &player;
				c.type = NetChangePlayer::PICKUP;
				c.id = item->count;
				c.ile = item->team_count;
				info.NeedUpdate();

				// send remove item to all players
				NetChange& c2 = Add1(net_changes);
				c2.type = NetChange::REMOVE_ITEM;
				c2.id = item->netid;

				// send info to other players about picking item
				if(players > 2)
				{
					NetChange& c3 = Add1(net_changes);
					c3.type = NetChange::PICKUP_ITEM;
					c3.unit = &unit;
					c3.ile = (up_animation ? 1 : 0);
				}

				// remove item
				if(before_player == BP_ITEM && before_player_ptr.item == item)
					before_player = BP_NONE;
				DeleteElement(*ctx->items, item);
			}
			break;
		// player consume item
		case NetChange::CONSUME_ITEM:
			{
				int index;
				if(!stream.Read(index))
				{
					ERROR(Format("Update server: Broken CONSUME_ITEM from %s.", info.name.c_str()));
					StreamEnd(false);
				}
				else
					unit.ConsumeItem(index);
			}
			break;
		// player wants to loot unit
		case NetChange::LOOT_UNIT:
			{
				int netid;
				if(!stream.Read(netid))
				{
					ERROR(Format("Update server: Broken LOOT_UNIT from %s.", info.name.c_str()));
					StreamEnd(false);
					break;
				}

				Unit* looted_unit = FindUnit(netid);
				if(!looted_unit)
				{
					ERROR(Format("Update server: LOOT_UNIT from %s, missing unit %d.", info.name.c_str(), netid));
					StreamEnd(false);
					break;
				}

				NetChangePlayer& c = Add1(net_changes_player);
				c.type = NetChangePlayer::LOOT;
				c.pc = &player;
				if(looted_unit->busy == Unit::Busy_Looted)
				{
					// someone else is already looting unit
					c.id = 0;
				}
				else
				{
					// start looting unit
					c.id = 1;
					looted_unit->busy = Unit::Busy_Looted;
					player.action = PlayerController::Action_LootUnit;
					player.action_unit = looted_unit;
					player.chest_trade = &looted_unit->items;
				}
				info.NeedUpdate();
			}
			break;
		// player wants to loot chest
		case NetChange::LOOT_CHEST:
			{
				int netid;
				if(!stream.Read(netid))
				{
					ERROR(Format("Update server: Broken LOOT_CHEST from %s.", info.name.c_str()));
					StreamEnd(false);
					break;
				}

				Chest* chest = FindChest(netid);
				if(!chest)
				{
					ERROR(Format("Update server: LOOT_CHEST from %s, missing chest %d.", info.name.c_str(), netid));
					StreamEnd(false);
					break;
				}

				NetChangePlayer& c = Add1(net_changes_player);
				c.type = NetChangePlayer::LOOT;
				c.pc = info.u->player;
				if(chest->looted)
				{
					// someone else is already looting this chest
					c.id = 0;
				}
				else
				{
					// start looting chest
					c.id = 1;
					chest->looted = true;
					player.action = PlayerController::Action_LootChest;
					player.action_chest = chest;
					player.chest_trade = &chest->items;

					// send info about opening chest
					NetChange& c2 = Add1(net_changes);
					c2.type = NetChange::CHEST_OPEN;
					c2.id = chest->netid;

					// animation / sound
					chest->ani->Play(&chest->ani->ani->anims[0], PLAY_PRIO1|PLAY_ONCE|PLAY_STOP_AT_END, 0);
					if(sound_volume)
					{
						VEC3 pos = chest->pos;
						pos.y += 0.5f;
						PlaySound3d(sChestOpen, pos, 2.f, 5.f);
					}

					// event handler
					if(chest->handler)
						chest->handler->HandleChestEvent(ChestEventHandler::Opened);
				}
				info.NeedUpdate();
			}
			break;
		// player gets item from unit or container
		case NetChange::GET_ITEM:
			{
				int i_index, count;
				if(!stream.Read(i_index)
					|| !stream.Read(count))
				{
					ERROR(Format("Update server: Broken GET_ITEM from %s.", info.name.c_str()));
					StreamEnd(false);
					break;
				}

				if(!player.IsTrading())
				{
					ERROR(Format("Update server: GET_ITEM, player %s is not trading.", info.name.c_str()));
					StreamEnd(false);
					break;
				}

				if(i_index >= 0)
				{
					// getting not equipped item
					if(i_index >= (int)player.chest_trade->size())
					{
						ERROR(Format("Update server: GET_ITEM from %s, invalid index %d.", info.name.c_str(), i_index));
						StreamEnd(false);
						break;
					}

					ItemSlot& slot = player.chest_trade->at(i_index);
					if(count < 1 || count >(int)slot.count)
					{
						ERROR(Format("Update server: GET_ITEM from %s, invalid item count %d (have %d).", info.name.c_str(), count, slot.count));
						StreamEnd(false);
						break;
					}

					if(slot.item->type == IT_GOLD)
					{
						// special handling of gold
						uint team_count = min(slot.team_count, (uint)count);
						if(team_count == 0)
						{
							unit.gold += slot.count;
							info.UpdateGold();
						}
						else
						{
							AddGold(team_count);
							uint ile = slot.count - team_count;
							if(ile)
							{
								unit.gold += ile;
								info.UpdateGold();
							}
						}
						slot.count -= count;
						if(slot.count == 0)
							player.chest_trade->erase(player.chest_trade->begin()+i_index);
						else
							slot.team_count -= team_count;
					}
					else
					{
						// player get item from corpse/chest/npc or bought from trader
						uint team_count = (player.action == PlayerController::Action_Trade ? 0 : min((uint)count, slot.team_count));
						AddItem(unit, slot.item, (uint)count, team_count, false);
						if(player.action == PlayerController::Action_Trade)
							unit.gold -= GetItemPrice(slot.item, unit, true) * count;
						else if(player.action == PlayerController::Action_ShareItems&& slot.item->type == IT_CONSUMEABLE
							&& slot.item->ToConsumeable().effect == E_HEAL)
							player.action_unit->ai->have_potion = 1;
						if(player.action != PlayerController::Action_LootChest)
						{
							player.action_unit->weight -= slot.item->weight*count;
							if(player.action == PlayerController::Action_LootUnit && slot.item == player.action_unit->used_item)
							{
								player.action_unit->used_item = NULL;
								// removed item from hand, send info to other players
								if(players > 2)
								{
									NetChange& c = Add1(net_changes);
									c.type = NetChange::REMOVE_USED_ITEM;
									c.unit = player.action_unit;
								}
							}
						}
						slot.count -= count;
						if(slot.count == 0)
							player.chest_trade->erase(player.chest_trade->begin()+i_index);
						else
							slot.team_count -= team_count;
					}
				}
				else
				{
					// getting equipped item
					ITEM_SLOT type = IIndexToSlot(i_index);
					if(player.action == PlayerController::Action_LootChest || type < SLOT_WEAPON || type >= SLOT_MAX || !player.action_unit->slots[type])
					{
						ERROR(Format("Update server: GET_ITEM from %s, invalid or empty slot %d.", info.name.c_str(), type));
						StreamEnd(false);
						break;
					}

					// get equipped item from unit
					const Item*& slot = player.action_unit->slots[type];
					AddItem(unit, slot, 1u, 1u, false);
					if(player.action == PlayerController::Action_LootUnit && type == IT_WEAPON && slot == player.action_unit->used_item)
					{
						player.action_unit->used_item = NULL;
						// removed item from hand, send info to other players
						if(players > 2)
						{
							NetChange& c = Add1(net_changes);
							c.type = NetChange::REMOVE_USED_ITEM;
							c.unit = player.action_unit;
						}
					}
					player.action_unit->weight -= slot->weight;
					slot = NULL;

					// send info about changing equipment of looted unit
					if(players > 2)
					{
						NetChange& c = Add1(net_changes);
						c.type = NetChange::CHANGE_EQUIPMENT;
						c.unit = player.action_unit;
						c.id = type;
					}
				}
			}
			break;
		// player puts item into unit or container
		case NetChange::PUT_ITEM:
			{
				int i_index;
				uint count;
				if(!stream.Read(i_index)
					|| !stream.Read(count))
				{
					ERROR(Format("Update server: Broken PUT_ITEM from %s.", info.name.c_str()));
					StreamEnd(false);
					break;
				}

				if(!player.IsTrading())
				{
					ERROR(Format("Update server: PUT_ITEM, player %s is not trading.", info.name.c_str()));
					StreamEnd(false);
					break;
				}

				if(i_index >= 0)
				{
					// put not equipped item
					if(i_index >= (int)unit.items.size())
					{
						ERROR(Format("Update server: PUT_ITEM from %s, invalid index %d.", info.name.c_str(), i_index));
						StreamEnd(false);
						break;
					}

					ItemSlot& slot = unit.items[i_index];
					if(count < 1 || count > slot.count)
					{
						ERROR(Format("Update server: PUT_ITEM from %s, invalid count %u (have %u).", info.name.c_str(), count, slot.count));
						StreamEnd(false);
						break;
					}

					uint team_count = min(count, slot.team_count);

					// add item
					if(player.action == PlayerController::Action_LootChest)
						AddItem(*player.action_chest, slot.item, count, team_count, false);
					else if(player.action == PlayerController::Action_Trade)
					{
						InsertItem(*player.chest_trade, slot.item, count, team_count);
						int price = GetItemPrice(slot.item, unit, false);
						if(team_count)
							AddGold(price * team_count);
						uint normal_count = count - team_count;
						if(normal_count)
						{
							unit.gold += price * normal_count;
							info.UpdateGold();
						}
					}
					else
					{
						Unit* t = player.action_unit;
						uint add_as_team = team_count;
						if(player.action == PlayerController::Action_ShareItems)
						{
							if(slot.item->type == IT_CONSUMEABLE && slot.item->ToConsumeable().effect == E_HEAL)
								t->ai->have_potion = 2;
						}
						else if(player.action == PlayerController::Action_GiveItems)
						{
							add_as_team = 0;
							int price = GetItemPrice(slot.item, unit, false);
							if(t->gold >= price)
							{
								t->gold -= price;
								unit.gold += price;
							}
							if(slot.item->type == IT_CONSUMEABLE && slot.item->ToConsumeable().effect == E_HEAL)
								t->ai->have_potion = 2;
						}
						AddItem(*t, slot.item, count, add_as_team, false);
						if(player.action == PlayerController::Action_ShareItems || player.action == PlayerController::Action_GiveItems)
						{
							if(slot.item->type == IT_CONSUMEABLE && t->ai->have_potion == 0)
								t->ai->have_potion = 1;
							if(player.action == PlayerController::Action_GiveItems)
							{
								UpdateUnitInventory(*t);
								NetChangePlayer& c = Add1(net_changes_player);
								c.type = NetChangePlayer::UPDATE_TRADER_INVENTORY;
								c.pc = &player;
								c.unit = t;
								info.NeedUpdate();
							}
						}
					}

					// remove item
					unit.weight -= slot.item->weight * count;
					slot.count -= count;
					if(slot.count == 0)
						unit.items.erase(unit.items.begin() + i_index);
					else
						slot.team_count -= team_count;
				}
				else
				{
					// put equipped item
					ITEM_SLOT type = IIndexToSlot(i_index);
					if(type < SLOT_WEAPON || type >= SLOT_MAX || !unit.slots[type])
					{
						ERROR(Format("Update server: PUT_ITEM from %s, invalid or empty slot %d.", info.name.c_str(), type));
						StreamEnd(false);
						break;
					}

					const Item*& slot = unit.slots[type];
					int price = GetItemPrice(slot, unit, false);
					// add new item
					if(player.action == PlayerController::Action_LootChest)
						AddItem(*player.action_chest, slot, 1u, 0u, false);
					else if(player.action == PlayerController::Action_Trade)
					{
						InsertItem(*player.chest_trade, slot, 1u, 0u);
						unit.gold += price;
					}
					else
					{
						AddItem(*player.action_unit, slot, 1u, 0u, false);
						if(player.action == PlayerController::Action_GiveItems)
						{
							if(player.action_unit->gold >= price)
							{
								// sold for gold
								player.action_unit->gold -= price;
								unit.gold += price;
							}
							UpdateUnitInventory(*player.action_unit);
							NetChangePlayer& c = Add1(net_changes_player);
							c.type = NetChangePlayer::UPDATE_TRADER_INVENTORY;
							c.pc = &player;
							c.id = player.action_unit->netid;
							info.NeedUpdate();
						}
					}
					// remove equipped
					unit.weight -= slot->weight;
					slot = NULL;
					// send info about changing equipment
					if(players > 2)
					{
						NetChange& c = Add1(net_changes);
						c.type = NetChange::CHANGE_EQUIPMENT;
						c.unit = &unit;
						c.id = type;
					}
				}
			}
			break;
		// player picks up all items from corpse/chest
		case NetChange::GET_ALL_ITEMS:
			{
				if(!player.IsTrading())
				{
					ERROR(Format("Update server: GET_ALL_ITEM, player %s is not trading.", info.name.c_str()));
					StreamEnd(false);
					break;
				}

				bool changes = false;

				// slots
				if(player.action != PlayerController::Action_LootChest)
				{
					const Item** slots = player.action_unit->slots;
					for(int i = 0; i<SLOT_MAX; ++i)
					{
						if(slots[i])
						{
							InsertItemBare(unit.items, slots[i]);
							slots[i] = NULL;
							changes = true;

							// send info about changing equipment
							if(players > 2)
							{
								NetChange& c = Add1(net_changes);
								c.type = NetChange::CHANGE_EQUIPMENT;
								c.unit = player.action_unit;
								c.id = i;
							}
						}
					}

					// reset weight
					player.action_unit->weight = 0;
				}

				// not equipped items
				for(ItemSlot& slot : *player.chest_trade)
				{
					if(!slot.item)
						continue;

					if(slot.item->type == IT_GOLD)
						unit.AddItem(&gold_item, slot.count, slot.team_count);
					else
					{
						InsertItemBare(unit.items, slot.item, slot.count, slot.team_count);
						changes = true;
					}
				}
				player.chest_trade->clear();

				if(changes)
					SortItems(unit.items);
			}
			break;
		// player ends trade
		case NetChange::STOP_TRADE:
			if(!player.IsTrading())
			{
				ERROR(Format("Update server: STOP_TRADE, player %s is not trading.", info.name.c_str()));
				StreamEnd(false);
				break;
			}

			if(player.action == PlayerController::Action_LootChest)
			{
				player.action_chest->looted = false;
				player.action_chest->ani->Play(&player.action_chest->ani->ani->anims[0], PLAY_PRIO1|PLAY_ONCE|PLAY_STOP_AT_END|PLAY_BACK, 0);
				if(sound_volume)
				{
					VEC3 pos = player.action_chest->pos;
					pos.y += 0.5f;
					PlaySound3d(sChestClose, pos, 2.f, 5.f);
				}
				NetChange& c = Add1(net_changes);
				c.type = NetChange::CHEST_CLOSE;
				c.id = player.action_chest->netid;
			}
			else
			{
				player.action_unit->busy = Unit::Busy_No;
				player.action_unit->look_target = NULL;
			}
			player.action = PlayerController::Action_None;
			if(player.dialog_ctx->next_talker)
			{
				Unit* t = player.dialog_ctx->next_talker;
				player.dialog_ctx->next_talker = NULL;
				t->auto_talk = 0;
				StartDialog(*player.dialog_ctx, t, player.dialog_ctx->next_dialog);
			}
			break;
		// player takes item for credit
		case NetChange::TAKE_ITEM_CREDIT:
			{
				int index;
				if(!stream.Read(index))
				{
					ERROR(Format("Update server: Broken TAKE_ITEM_CREDIT from %s.", info.name.c_str()));
					StreamEnd(false);
					break;
				}

				if(index < 0 || index >= (int)unit.items.size())
				{
					ERROR(Format("Update server: TAKE_ITEM_CREDIT from %s, invalid index %d.", info.name.c_str(), index));
					StreamEnd(false);
					break;
				}

				ItemSlot& slot = unit.items[index];
				if(slot.item->IsWearableByHuman() && slot.team_count != 0)
				{
					slot.team_count = 0;
					player.credit += slot.item->value/2;
					CheckCredit(true);
				}
				else
				{
					ERROR(Format("Update server: TAKE_ITEM_CREDIT from %s, item %s (count %u, team count %u).", info.name.c_str(), slot.item->id.c_str(),
						slot.count, slot.team_count));
					StreamEnd(false);
				}
			}
			break;
		// unit plays idle animation
		case NetChange::IDLE:
			{
				byte index;
				if(!stream.Read(index))
				{
					ERROR(Format("Update server: Broken IDLE from %s.", info.name.c_str()));
					StreamEnd(false);
				}
				else if(index >= unit.data->idles->size())
				{
					ERROR(Format("Update server: IDLE from %s, invalid animation index %u.", info.name.c_str(), index));
					StreamEnd(false);
				}
				else
				{
					unit.ani->Play(unit.data->idles->at(index).c_str(), PLAY_ONCE, 0);
					unit.ani->frame_end_info = false;
					unit.animation = ANI_IDLE;
					// send info to other players
					if(players > 2)
					{
						NetChange& c = Add1(net_changes);
						c.type = NetChange::IDLE;
						c.unit = &unit;
						c.id = index;
					}
				}
			}
			break;
		// player start dialog
		case NetChange::TALK:
			{
				int netid;
				if(!stream.Read(netid))
				{
					ERROR(Format("Update server: Broken TALK from %s.", info.name.c_str()));
					StreamEnd(false);
					break;
				}

				Unit* talk_to = FindUnit(netid);
				if(!talk_to)
				{
					ERROR(Format("Update server: TALK from %s, missing unit %d.", info.name.c_str(), netid));
					StreamEnd(false);
					break;
				}

				NetChangePlayer& c = Add1(net_changes_player);
				c.pc = &player;
				c.type = NetChangePlayer::START_DIALOG;
				if(talk_to->busy != Unit::Busy_No || !talk_to->CanTalk())
				{
					// can't talk to unit
					c.id = -1;
				}
				else
				{
					// start dialog
					c.id = talk_to->netid;
					talk_to->auto_talk = 0;
					StartDialog(*player.dialog_ctx, talk_to);
				}
				info.NeedUpdate();
			}
			break;
		// player selected dialog choice
		case NetChange::CHOICE:
			{
				byte choice;
				if(!stream.Read(choice))
				{
					ERROR(Format("Update server: Broken CHOICE from %s.", info.name.c_str()));
					StreamEnd(false);
				}
				else if(player.dialog_ctx->show_choices && choice < player.dialog_ctx->choices.size())
					player.dialog_ctx->choice_selected = choice;
				else
				{
					ERROR(Format("Update server: CHOICE from %s, not in dialog or invalid choice %u.", info.name.c_str(), choice));
					StreamEnd(false);
				}
			}
			break;
			// pomijanie dialogu przez gracza
		case NetChange::SKIP_DIALOG:
			{
				int skip_id;
				if(!stream.Read(skip_id))
				{
					ERROR(Format("Update server: Broken SKIP_DIALOG from %s.", info.name.c_str()));
					StreamEnd(false);
				}
				else
				{
					DialogContext& ctx = *player.dialog_ctx;
					if(ctx.dialog_wait > 0.f && ctx.dialog_mode && !ctx.show_choices && ctx.skip_id == skip_id && ctx.can_skip)
						ctx.choice_selected = 1;
				}
			}
			break;
		// player wants to enter building
		case NetChange::ENTER_BUILDING:
			{
				byte building_index;
				if(!stream.Read(building_index))
				{
					ERROR(Format("Update server: Broken ENTER_BUILDING from %s.", info.name.c_str()));
					StreamEnd(false);
				}
				else if(city_ctx && building_index < city_ctx->inside_buildings.size())
				{
					WarpData& warp = Add1(mp_warps);
					warp.u = &unit;
					warp.where = building_index;
					warp.timer = 1.f;
					unit.frozen = 2;
				}
				else
				{
					ERROR(Format("Update server: ENTER_BUILDING from %s, invalid building index %u.", info.name.c_str(), building_index));
					StreamEnd(false);
				}
			}
			break;
		// player wants to exit building
		case NetChange::EXIT_BUILDING:
			if(unit.in_building != -1)
			{
				WarpData& warp = Add1(mp_warps);
				warp.u = &unit;
				warp.where = -1;
				warp.timer = 1.f;
				unit.frozen = 2;
			}
			else
			{
				ERROR(Format("Update server: EXIT_BUILDING from %s, unit not in building.", info.name.c_str()));
				StreamEnd(false);
			}
			break;
		// notify about warping
		case NetChange::WARP:
			info.warping = false;
			break;
		// player added note to journal
		case NetChange::ADD_NOTE:
			{
				string& str = Add1(info.notes);
				if(!ReadString1(stream, str))
				{
					info.notes.pop_back();
					ERROR(Format("Update server: Broken ADD_NOTE from %s.", info.name.c_str()));
					StreamEnd(false);
				}
			}
			break;
		// get random number for player
		case NetChange::RANDOM_NUMBER:
			{
				byte number;
				if(!stream.Read(number))
				{
					ERROR(Format("Update server: Broken RANDOM_NUMBER from %s.", info.name.c_str()));
					StreamEnd(false);
				}
				else
				{
					AddMsg(Format(txRolledNumber, info.name.c_str(), number));
					// send to other players
					if(players > 2)
					{
						NetChange& c = Add1(net_changes);
						c.type = NetChange::RANDOM_NUMBER;
						c.unit = info.u;
						c.id = number;
					}
				}
			}
			break;
		// player wants to start/stop using useable
		case NetChange::USE_USEABLE:
			{
				int useable_netid;
				byte state; // 0-stop, 1-start
				if(!stream.Read(useable_netid)
					|| !stream.Read(state))
				{
					ERROR(Format("Update server: Broken USE_USEABLE from %s.", info.name.c_str()));
					StreamEnd(false);
					break;
				}

				Useable* useable = FindUseable(useable_netid);
				if(!useable)
				{
					ERROR(Format("Update server: USE_USEABLE from %s, missing useable %d.", info.name.c_str(), useable_netid));
					StreamEnd(false);
					break;
				}

				if(state == 1)
				{
					// use useable
					if(useable->user)
					{
						// someone else is already using this
						NetChangePlayer& c = Add1(net_changes_player);
						c.type = NetChangePlayer::USE_USEABLE;
						c.pc = &player;
						info.NeedUpdate();
					}
					else
					{
						BaseUsable& base = g_base_usables[useable->type];

						unit.action = A_ANIMATION2;
						unit.animation = ANI_PLAY;
						unit.ani->Play(base.anim, PLAY_PRIO1, 0);
						unit.useable = useable;
						unit.target_pos = unit.pos;
						unit.target_pos2 = useable->pos;
						if(g_base_usables[useable->type].limit_rot == 4)
							unit.target_pos2 -= VEC3(sin(useable->rot)*1.5f, 0, cos(useable->rot)*1.5f);
						unit.timer = 0.f;
						unit.animation_state = AS_ANIMATION2_MOVE_TO_OBJECT;
						unit.use_rot = lookat_angle(unit.pos, unit.useable->pos);
						unit.used_item = base.item;
						if(unit.used_item)
						{
							unit.weapon_taken = W_NONE;
							unit.weapon_state = WS_HIDDEN;
						}
						useable->user = &unit;

						// send info to players
						NetChange& c = Add1(net_changes);
						c.type = NetChange::USE_USEABLE;
						c.unit = info.u;
						c.id = useable_netid;
						c.ile = state;
					}
				}
				else
				{
					// stop using useable
					if(useable->user == &unit)
					{
						unit.action = A_NONE;
						unit.animation = ANI_STAND;
						useable->user = NULL;
						if(unit.live_state == Unit::ALIVE)
							unit.used_item = NULL;

						// send info to other players
						if(players > 2)
						{
							NetChange& c = Add1(net_changes);
							c.type = NetChange::USE_USEABLE;
							c.unit = info.u;
							c.id = useable_netid;
							c.ile = state;
						}
					}
					else
					{
						ERROR(Format("Update server: USE_USEABLE from %s, useable %d is used by %d (%s).", info.name.c_str(), useable_netid,
							useable->user->netid, useable->user->data->id.c_str()));
						StreamEnd(false);
					}
				}
			}
			break;
		// player used cheat 'suicide'
		case NetChange::CHEAT_SUICIDE:
			if(info.cheats)
				GiveDmg(GetContext(unit), NULL, unit.hpmax, unit);
			else
			{
				ERROR(Format("Update server: Player %s used CHEAT_SUICIDE without cheats.", info.name.c_str()));
				StreamEnd(false);
			}
			break;
		// player used cheat 'godmode' 
		case NetChange::CHEAT_GODMODE:
			{
				bool state;
				if(!ReadBool(stream, state))
				{
					ERROR(Format("Update server: Broken CHEAT_GODMODE from %s.", info.name.c_str()));
					StreamEnd(false);
				}
				else if(info.cheats)
					player.godmode = state;
				else
				{
					ERROR(Format("Update server: Player %s used CHEAT_GODMODE without cheats.", info.name.c_str()));
					StreamEnd(false);
				}
			}
			break;
		// player stands up
		case NetChange::STAND_UP:
			UnitStandup(unit);
			// send to other players
			if(players > 2)
			{
				NetChange& c = Add1(net_changes);
				c.type = NetChange::STAND_UP;
				c.unit = &unit;
			}
			break;
		// player used cheat 'noclip'
		case NetChange::CHEAT_NOCLIP:
			{
				bool state;
				if(!ReadBool(stream, state))
				{
					ERROR(Format("Update server: Broken CHEAT_NOCLIP from %s.", info.name.c_str()));
					StreamEnd(false);
				}
				else if(info.cheats)
					player.noclip = state;
				else
				{
					ERROR(Format("Update server: Player %s used CHEAT_NOCLIP without cheats.", info.name.c_str()));
					StreamEnd(false);
				}
			}
			break;
		// player used cheat 'invisible'
		case NetChange::CHEAT_INVISIBLE:
			{
				bool state;
				if(!ReadBool(stream, state))
				{
					ERROR(Format("Update server: Broken CHEAT_INVISIBLE from %s.", info.name.c_str()));
					StreamEnd(false);
				}
				else if(info.cheats)
					unit.invisible = state;
				else
				{
					ERROR(Format("Update server: Player %s used CHEAT_INVISIBLE without cheats.", info.name.c_str()));
					StreamEnd(false);
				}
			}
			break;
		// player used cheat 'scare'
		case NetChange::CHEAT_SCARE:
			if(info.cheats)
			{
				for(AIController* ai : ais)
				{
					if(IsEnemy(*ai->unit, unit) && distance(ai->unit->pos, unit.pos) < ALERT_RANGE.x && CanSee(*ai->unit, unit))
						ai->morale = -10;
				}
			}
			else
			{
				ERROR(Format("Update server: Player %s used CHEAT_SCARE without cheats.", info.name.c_str()));
				StreamEnd(false);
			}
			break;
		// player used cheat 'killall'
		case NetChange::CHEAT_KILLALL:
			{
				int ignored_netid;
				byte type;
				if(!stream.Read(ignored_netid)
					|| !stream.Read(type))
				{
					ERROR(Format("Update server: Broken CHEAT_KILLALL from %s.", info.name.c_str()));
					StreamEnd(false);
					break;
				}

				if(!info.cheats)
				{
					ERROR(Format("Update server: Player %s used CHEAT_KILLALL without cheats.", info.name.c_str()));
					StreamEnd(false);
					break;
				}

				Unit* ignored;
				if(ignored_netid == -1)
					ignored = NULL;
				else
				{
					ignored = FindUnit(ignored_netid);
					if(!ignored)
					{
						ERROR(Format("Update server: CHEAT_KILLALL from %s, missing unit %d.", info.name.c_str(), ignored_netid));
						StreamEnd(false);
						break;
					}
				}

				if(!Cheat_KillAll(type, unit, ignored))
				{
					ERROR(Format("Update server: CHEAT_KILLALL from %s, invalid type %u.", info.name.c_str(), type));
					StreamEnd(false);
				}
			}
			break;
			// sprawdza czy podany przedmiot jest lepszy dla postaci z która dokonuje wymiany
		case NetChange::IS_BETTER_ITEM:
			{
				int i_index;
				if(!stream.Read(i_index))
				{
					ERROR(Format("Update server: Broken IS_BETTER_ITEM from %s.", info.name.c_str()));
					StreamEnd(false);
					break;
				}

				NetChangePlayer& c = Add1(net_changes_player);
				c.type = NetChangePlayer::IS_BETTER_ITEM;
				c.id = 0;
				c.pc = &player;
				info.NeedUpdate();

				if(player.action == PlayerController::Action_GiveItems)
				{
					const Item* item = unit.GetIIndexItem(i_index);
					if(item)
					{
						if(IsBetterItem(*player.action_unit, item))
							c.id = 1;
					}
					else
					{
						ERROR(Format("Update server: IS_BETTER_ITEM from %s, invalid i_index %d.", info.name.c_str(), i_index));
						StreamEnd(false);
					}
				}
				else
				{
					ERROR(Format("Update server: IS_BETTER_ITEM from %s, player is not giving items.", info.name.c_str()));
					StreamEnd(false);
				}
			}
			break;
		// player used cheat 'citizen'
		case NetChange::CHEAT_CITIZEN:
			if(info.cheats)
			{
				if(bandyta || atak_szalencow)
				{
					bandyta = false;
					atak_szalencow = false;
					PushNetChange(NetChange::CHANGE_FLAGS);
				}
			}
			else
			{
				ERROR(Format("Update server: Player %s used CHEAT_CITIZEN without cheats.", info.name.c_str()));
				StreamEnd(false);
			}
			break;
		// player used cheat 'heal'
		case NetChange::CHEAT_HEAL:
			if(info.cheats)
			{
				unit.hp = unit.hpmax;
				NetChange& c = Add1(net_changes);
				c.type = NetChange::UPDATE_HP;
				c.unit = &unit;
			}
			else
			{
				ERROR(Format("Update server: Player %s used CHEAT_HEAL without cheats.", info.name.c_str()));
				StreamEnd(false);
			}
			break;
		// player used cheat 'kill'
		case NetChange::CHEAT_KILL:
			{
				int netid;
				if(!stream.Read(netid))
				{
					ERROR(Format("Update server: Broken CHEAT_KILL from %s.", info.name.c_str()));
					StreamEnd(false);
				}
				else if(!info.cheats)
				{
					ERROR(Format("Update server: Player %s used CHEAT_KILL without cheats.", info.name.c_str()));
					StreamEnd(false);
				}
				else
				{
					Unit* target = FindUnit(netid);
					if(!target)
					{
						ERROR(Format("Update server: CHEAT_KILL from %s, missing unit %d.", info.name.c_str(), netid));
						StreamEnd(false);
					}
					else
						GiveDmg(GetContext(*target), NULL, target->hpmax, *target);
				}
			}
			break;
		// player used cheat 'healunit'
		case NetChange::CHEAT_HEALUNIT:
			{
				int netid;
				if(!stream.Read(netid))
				{
					ERROR(Format("Update server: Broken CHEAT_HEALUNIT from %s.", info.name.c_str()));
					StreamEnd(false);
				}
				else if(!info.cheats)
				{
					ERROR(Format("Update server: Player %s used CHEAT_HEALUNIT without cheats.", info.name.c_str()));
					StreamEnd(false);
				}
				else
				{
					Unit* target = FindUnit(netid);
					if(!target)
					{
						ERROR(Format("Update server: CHEAT_HEALUNIT from %s, missing unit %d.", info.name.c_str(), netid));
						StreamEnd(false);
					}
					else
					{
						target->hp = target->hpmax;
						NetChange& c = Add1(net_changes);
						c.type = NetChange::UPDATE_HP;
						c.unit = target;
					}
				}
			}
			break;
		// player used cheat 'reveal'
		case NetChange::CHEAT_REVEAL:
			if(info.cheats)
				Cheat_Reveal();
			else
			{
				ERROR(Format("Update server: Player %s used CHEAT_REVEAL without cheats.", info.name.c_str()));
				StreamEnd(false);
			}
			break;
		// player used cheat 'goto_map'
		case NetChange::CHEAT_GOTO_MAP:
			if(info.cheats)
			{
				ExitToMap();
				return false;
			}
			else
			{
				ERROR(Format("Update server: Player %s used CHEAT_GOTO_MAP without cheats.", info.name.c_str()));
				StreamEnd(false);
			}
			break;
		// player used cheat 'show_minimap'
		case NetChange::CHEAT_SHOW_MINIMAP:
			if(info.cheats)
				Cheat_ShowMinimap();
			else
			{
				ERROR(Format("Update server: Player %s used CHEAT_SHOW_MINIMAP without cheats.", info.name.c_str()));
				StreamEnd(false);
			}
			break;
		// player used cheat 'addgold'
		case NetChange::CHEAT_ADDGOLD:
			{
				int count;
				if(!stream.Read(count))
				{
					ERROR(Format("Update server: Broken CHEAT_ADDGOLD from %s.", info.name.c_str()));
					StreamEnd(false);
				}
				else if(!info.cheats)
				{
					ERROR(Format("Update server: Player %s used CHEAT_ADDGOLD without cheats.", info.name.c_str()));
					StreamEnd(false);
				}
				else
				{
					unit.gold = max(unit.gold + count, 0);
					info.UpdateGold();
				}
			}
			break;
		// player used cheat 'addgold_team'
		case NetChange::CHEAT_ADDGOLD_TEAM:
			{
				int count;
				if(!stream.Read(count))
				{
					ERROR(Format("Update server: Broken CHEAT_ADDGOLD_TEAM from %s.", info.name.c_str()));
					StreamEnd(false);
				}
				else if(!info.cheats)
				{
					ERROR(Format("Update server: Player %s used CHEAT_ADDGOLD_TEAM without cheats.", info.name.c_str()));
					StreamEnd(false);
				}
				else if(count <= 0)
				{
					ERROR(Format("Update server: CHEAT_ADDGOLD_TEAM from %s, invalid count %d.", info.name.c_str(), count));
					StreamEnd(false);
				}
				else
					AddGold(count);
			}
			break;
		// player used cheat 'additem' or 'addteam'
		case NetChange::CHEAT_ADDITEM:
			{
				byte count;
				bool is_team;
				if(!ReadString1(stream)
					|| !stream.Read(count)
					|| !stream.Read(is_team))
				{
					ERROR(Format("Update server: Broken CHEAT_ADDITEM from %s.", info.name.c_str()));
					StreamEnd(false);
				}
				else if(!info.cheats)
				{
					ERROR(Format("Update server: Player %s used CHEAT_ADDITEM without cheats.", info.name.c_str()));
					StreamEnd(false);
				}
				else
				{
					const Item* item = FindItem(BUF);
					if(item && count)
						AddItem(*info.u, item, count, is_team);
					else
					{
						ERROR(Format("Update server: CHEAT_ADDITEM from %s, missing item %s or invalid count %u.", info.name.c_str(), BUF, count));
						StreamEnd(false);
					}
				}
			}
			break;
		// player used cheat 'skip_days'
		case NetChange::CHEAT_SKIP_DAYS:
			{
				int count;
				if(!stream.Read(count))
				{
					ERROR(Format("Update server: Broken CHEAT_SKIP_DAYS from %s.", info.name.c_str()));
					StreamEnd(false);
				}
				else if(!info.cheats)
				{
					ERROR(Format("Update server: Player %s used CHEAT_SKIP_DAYS without cheats.", info.name.c_str()));
					StreamEnd(false);
				}
				else
					WorldProgress(count, WPM_SKIP);
			}
			break;
		// player used cheat 'warp'
		case NetChange::CHEAT_WARP:
			{
				byte building_type;
				if(!stream.Read(building_type))
				{
					ERROR(Format("Update server: Broken CHEAT_WARP from %s.", info.name.c_str()));
					StreamEnd(false);
				}
				else if(!info.cheats)
				{
					ERROR(Format("Update server: Player %s used CHEAT_WARP without cheats.", info.name.c_str()));
					StreamEnd(false);
				}
				else if(unit.frozen != 0)
				{
					ERROR(Format("Update server: CHEAT_WARP from %s, unit is frozen.", info.name.c_str()));
					StreamEnd(false);
				}
				else
				{
					int id;
					InsideBuilding* building;
					if(building_type == B_INN)
						building = city_ctx->FindInn(id);
					else
						building = city_ctx->FindInsideBuilding((BUILDING)building_type, id);
					if(building)
					{
						WarpData& warp = Add1(mp_warps);
						warp.u = &unit;
						warp.where = id;
						warp.timer = 1.f;
						unit.frozen = 2;
						Net_PrepareWarp(info.u->player);
					}
					else
					{
						ERROR(Format("Update server: CHEAT_WARP from %s, missing building type %u.", info.name.c_str(), building_type));
						StreamEnd(false);
					}
				}
			}
			break;
		// player used cheat 'spawn_unit'
		case NetChange::CHEAT_SPAWN_UNIT:
			{
				byte count;
				char level, in_arena;
				if(!ReadString1(stream)
					|| !stream.Read(count)
					|| !stream.Read(level)
					|| !stream.Read(in_arena))
				{
					ERROR(Format("Update server: Broken CHEAT_SPAWN_UNIT from %s.", info.name.c_str()));
					StreamEnd(false);
				}
				else if(!info.cheats)
				{
					ERROR(Format("Update server: Player %s used CHEAT_SPAWN_UNIT without cheats.", info.name.c_str()));
					StreamEnd(false);
				}
				else
				{
					UnitData* data = FindUnitData(BUF, false);
					if(!data)
					{
						ERROR(Format("Update server: CHEAT_SPAWN_UNIT from %s, invalid unit id %s.", info.name.c_str(), BUF));
						StreamEnd(false);
					}
					else
					{
						if(in_arena < -1 || in_arena > 1)
							in_arena = -1;

						LevelContext& ctx = GetContext(*info.u);
						VEC3 pos = info.u->GetFrontPos();

						for(byte i = 0; i<count; ++i)
						{
							Unit* spawned = SpawnUnitNearLocation(ctx, pos, *data, &unit.pos, level);
							if(!spawned)
							{
								WARN(Format("Update server: CHEAT_SPAWN_UNIT from %s, no free space for unit.", info.name.c_str()));
								break;
							}
							else if(in_arena != -1)
							{
								spawned->in_arena = in_arena;
								at_arena.push_back(spawned);
							}
							if(IsOnline())
								Net_SpawnUnit(spawned);
						}
					}
				}
			}
			break;
		// player used cheat 'setstat' or 'modstat'
		case NetChange::CHEAT_SETSTAT:
		case NetChange::CHEAT_MODSTAT:
			{
				cstring name = (type == NetChange::CHEAT_SETSTAT ? "CHEAT_SETSTAT" : "CHEAT_MODSTAT");

				byte what;
				bool is_skill;
				char value;
				if(!stream.Read(what)
					|| !ReadBool(stream, is_skill)
					|| !stream.Read(value))
				{
					ERROR(Format("Update server: Broken %s from %s.", name, info.name.c_str()));
					StreamEnd(false);
				}
				else if(!info.cheats)
				{
					ERROR(Format("Update server: Player %s used %s without cheats.", info.name.c_str(), name));
					StreamEnd(false);
				}
				else if(is_skill)
				{
					if(what < (int)Skill::MAX)
					{
						int num = +value;
						if(type == NetChange::CHEAT_MODSTAT)
							num += info.u->unmod_stats.skill[what];
						int v = clamp(num, 0, SkillInfo::MAX);
						if(v != info.u->unmod_stats.skill[what])
						{
							info.u->Set((Skill)what, v);
							NetChangePlayer& c = AddChange(NetChangePlayer::STAT_CHANGED, info.pc);
							c.id = (int)ChangedStatType::SKILL;
							c.a = what;
							c.ile = v;
						}
					}
					else
					{
						ERROR(Format("Update server: %s from %s, invalid skill %d.", name, info.name.c_str(), what));
						StreamEnd(false);
					}
				}
				else
				{
					if(what < (int)Attribute::MAX)
					{
						int num = +value;
						if(type == NetChange::CHEAT_MODSTAT)
							num += info.u->unmod_stats.attrib[what];
						int v = clamp(num, 1, AttributeInfo::MAX);
						if(v != info.u->unmod_stats.attrib[what])
						{
							info.u->Set((Attribute)what, v);
							NetChangePlayer& c = AddChange(NetChangePlayer::STAT_CHANGED, info.pc);
							c.id = (int)ChangedStatType::ATTRIBUTE;
							c.a = what;
							c.ile = v;
						}
					}
					else
					{
						ERROR(Format("Update server: %s from %s, invalid attribute %d.", name, info.name.c_str(), what));
						StreamEnd(false);
					}
				}
			}
			break;
		// response to pvp request
		case NetChange::PVP:
			{
				bool accepted;
				if(!ReadBool(stream, accepted))
				{
					ERROR(Format("Update server: Broken PVP from %s.", info.name.c_str()));
					StreamEnd(false);
				}
				else if(pvp_response.ok && pvp_response.to == info.u)
				{
					if(accepted)
						StartPvp(pvp_response.from->player, pvp_response.to);
					else
					{
						if(pvp_response.from->player == pc)
							AddMsg(Format(txPvpRefuse, info.name.c_str()));
						else
						{
							NetChangePlayer& c = Add1(net_changes_player);
							c.type = NetChangePlayer::NO_PVP;
							c.pc = pvp_response.from->player;
							c.id = pvp_response.to->player->id;
							GetPlayerInfo(pvp_response.from->player).NeedUpdate();
						}
					}

					if(pvp_response.ok && pvp_response.to == pc->unit)
					{
						if(dialog_pvp)
						{
							GUI.CloseDialog(dialog_pvp);
							delete dialog_pvp;
							dialog_pvp = NULL;
						}
						pvp_response.ok = false;
					}
				}
			}
			break;
		// leader wants to leave location
		case NetChange::LEAVE_LOCATION:
			{
				char type;
				if(!stream.Read(type))
				{
					ERROR(Format("Update server: Broken LEAVE_LOCATION from %s.", info.name.c_str()));
					StreamEnd(false);
				}
				else if(!IsLeader(unit))
				{
					ERROR(Format("Update server: LEAVE_LOCATION from %s, player is not leader.", info.name.c_str()));
					StreamEnd(false);
				}
				else
				{
					int result = CanLeaveLocation(*info.u);
					if(result == 0)
					{
						PushNetChange(NetChange::LEAVE_LOCATION);
						if(type == WHERE_OUTSIDE)
							fallback_co = FALLBACK_EXIT;
						else if(type == WHERE_LEVEL_UP)
						{
							fallback_co = FALLBACK_CHANGE_LEVEL;
							fallback_1 = -1;
						}
						else if(type == WHERE_LEVEL_DOWN)
						{
							fallback_co = FALLBACK_CHANGE_LEVEL;
							fallback_1 = +1;
						}
						else
						{
							if(location->TryGetPortal(type))
							{
								fallback_co = FALLBACK_USE_PORTAL;
								fallback_1 = type;
							}
							else
							{
								ERROR(Format("Update server: LEAVE_LOCATION from %s, invalid type %d.", type));
								StreamEnd(false);
								break;
							}
						}

						fallback_t = -1.f;
						for(Unit* team_member : team)
							team_member->frozen = 2;
					}
					else
					{
						// can't leave
						NetChangePlayer& c = Add1(net_changes_player);
						c.type = NetChangePlayer::CANT_LEAVE_LOCATION;
						c.pc = &player;
						c.id = result;
						info.NeedUpdate();
					}
				}
			}
			break;
		// player open/close door
		case NetChange::USE_DOOR:
			{
				int netid;
				bool is_closing;
				if(!stream.Read(netid)
					|| !ReadBool(stream, is_closing))
				{
					ERROR(Format("Update server: Broken USE_DOOR from %s.", info.name.c_str()));
					StreamEnd(false);
					break;
				}

				Door* door = FindDoor(netid);
				if(!door)
				{
					ERROR(Format("Update server: USE_DOOR from %s, missing door %d.", info.name.c_str(), netid));
					StreamEnd(false);
					break;
				}

				bool ok = true;
				if(is_closing)
				{
					// closing door
					if(door->state == Door::Open)
					{
						door->state = Door::Closing;
						door->ani->Play(&door->ani->ani->anims[0], PLAY_ONCE|PLAY_STOP_AT_END|PLAY_NO_BLEND|PLAY_BACK, 0);
						door->ani->frame_end_info = false;
					}
					else if(door->state == Door::Opening)
					{
						door->state = Door::Closing2;
						door->ani->Play(&door->ani->ani->anims[0], PLAY_ONCE|PLAY_STOP_AT_END|PLAY_BACK, 0);
						door->ani->frame_end_info = false;
					}
					else if(door->state == Door::Opening2)
					{
						door->state = Door::Closing;
						door->ani->Play(&door->ani->ani->anims[0], PLAY_ONCE|PLAY_STOP_AT_END|PLAY_BACK, 0);
						door->ani->frame_end_info = false;
					}
					else
						ok = false;
				}
				else
				{
					// opening door
					if(door->state == Door::Closed)
					{
						door->locked = LOCK_NONE;
						door->state = Door::Opening;
						door->ani->Play(&door->ani->ani->anims[0], PLAY_ONCE|PLAY_STOP_AT_END|PLAY_NO_BLEND, 0);
						door->ani->frame_end_info = false;
					}
					else if(door->state == Door::Closing)
					{
						door->locked = LOCK_NONE;
						door->state = Door::Opening2;
						door->ani->Play(&door->ani->ani->anims[0], PLAY_ONCE|PLAY_STOP_AT_END, 0);
						door->ani->frame_end_info = false;
					}
					else if(door->state == Door::Closing2)
					{
						door->locked = LOCK_NONE;
						door->state = Door::Opening;
						door->ani->Play(&door->ani->ani->anims[0], PLAY_ONCE|PLAY_STOP_AT_END, 0);
						door->ani->frame_end_info = false;
					}
					else
						ok = false;
				}

				if(ok && sound_volume && rand2() == 0)
				{
					SOUND snd;
					if(is_closing && rand2()%2 == 0)
						snd = sDoorClose;
					else
						snd = sDoor[rand2()%3];
					VEC3 pos = door->pos;
					pos.y += 1.5f;
					PlaySound3d(snd, pos, 2.f, 5.f);
				}
			}
			break;
			// podró¿ do innej lokacji
		case NetChange::TRAVEL:
			{
				byte loc;
				if(!stream.Read(loc))
				{
					ERROR(Format("Update server: Broken TRAVEL from %s.", info.name.c_str()));
					StreamEnd(false);
				}
				else if(!IsLeader(unit))
				{
					ERROR(Format("Update server: LEAVE_LOCATION from %s, player is not leader.", info.name.c_str()));
					StreamEnd(false);
				}
				else
				{
					// start travel
					world_state = WS_TRAVEL;
					current_location = -1;
					travel_time = 0.f;
					travel_day = 0;
					travel_start = world_pos;
					picked_location = loc;
					Location& l = *locations[picked_location];
					world_dir = angle(world_pos.x, -world_pos.y, l.pos.x, -l.pos.y);
					travel_time2 = 0.f;

					// leave current location
					if(open_location != -1)
					{
						LeaveLocation();
						open_location = -1;
					}

					// send info to players
					NetChange& c = Add1(net_changes);
					c.type = NetChange::TRAVEL;
					c.id = loc;
				}
			}
			break;
		// enter current location
		case NetChange::ENTER_LOCATION:
			if(game_state == GS_WORLDMAP && world_state == WS_MAIN && IsLeader(info.u))
			{
				if(EnterLocation())
					return false;
			}
			else
			{
				ERROR(Format("Update server: ENTER_LOCATION from %s, not leader or not on map.", info.name.c_str()));
				StreamEnd(false);
			}
			break;
		// player is training dexterity by moving
		case NetChange::TRAIN_MOVE:
			player.Train(TrainWhat::Move, 0.f, 0);
			break;
		// close encounter message box
		case NetChange::CLOSE_ENCOUNTER:
			if(dialog_enc)
			{
				GUI.CloseDialog(dialog_enc);
				delete dialog_enc;
				dialog_enc = NULL;
			}
			world_state = WS_TRAVEL;
			PushNetChange(NetChange::CLOSE_ENCOUNTER);
			Event_RandomEncounter(0);
			return false;
		// player used cheat to change level (<>+shift+ctrl)
		case NetChange::CHEAT_CHANGE_LEVEL:
			{
				bool is_down;
				if(!ReadBool(stream, is_down))
				{
					ERROR(Format("Update server: Broken CHEAT_CHANGE_LEVEL from %s.", info.name.c_str()));
					StreamEnd(false);
				}
				else if(!info.cheats)
				{
					ERROR(Format("Update server: Player %s used CHEAT_CHANGE_LEVEL without cheats.", info.name.c_str()));
					StreamEnd(false);
				}
				else if(location->outside)
					ChangeLevel(is_down ? -1 : +1);
			}
			break;
		// player used cheat to warp to stairs (<>+shift)
		case NetChange::CHEAT_WARP_TO_STAIRS:
			{
				bool is_down;
				if(!ReadBool(stream, is_down))
				{
					ERROR(Format("Update server: Broken CHEAT_CHANGE_LEVEL from %s.", info.name.c_str()));
					StreamEnd(false);
				}
				else if(!info.cheats)
				{
					ERROR(Format("Update server: Player %s used CHEAT_CHANGE_LEVEL without cheats.", info.name.c_str()));
					StreamEnd(false);
				}
				else
				{
					InsideLocation* inside = (InsideLocation*)location;
					InsideLocationLevel& lvl = inside->GetLevelData();

					if(!is_down)
					{
						INT2 tile = lvl.GetUpStairsFrontTile();
						unit.rot = dir_to_rot(lvl.staircase_up_dir);
						WarpUnit(unit, VEC3(2.f*tile.x+1.f, 0.f, 2.f*tile.y+1.f));
					}
					else
					{
						INT2 tile = lvl.GetDownStairsFrontTile();
						unit.rot = dir_to_rot(lvl.staircase_down_dir);
						WarpUnit(unit, VEC3(2.f*tile.x+1.f, 0.f, 2.f*tile.y+1.f));
					}
				}
			}
			break;
		// player used cheat 'noai'
		case NetChange::CHEAT_NOAI:
			{
				bool state;
				if(!ReadBool(stream, state))
				{
					ERROR(Format("Update server: Broken CHEAT_NOAI from %s.", info.name.c_str()));
					StreamEnd(false);
				}
				else if(info.cheats)
				{
					if(noai != state)
					{
						noai = state;
						NetChange& c = Add1(net_changes);
						c.type = NetChange::CHEAT_NOAI;
						c.id = (state ? 1 : 0);
					}
				}
				else
				{
					ERROR(Format("Update server: Player %s used CHEAT_NOAI without cheats.", info.name.c_str()));
					StreamEnd(false);
				}
			}
		// player rest in inn
		case NetChange::REST:
			{
				byte days;
				if(!stream.Read(days))
				{
					ERROR(Format("Update server: Broken REST from %s.", info.name.c_str()));
					StreamEnd(false);
				}
				else
				{
					player.Rest(days, true);
					UseDays(&player, days);
					NetChangePlayer& c = Add1(net_changes_player);
					c.type = NetChangePlayer::END_FALLBACK;
					c.pc = &player;
					info.NeedUpdate();
				}
			}
			break;
		// player trains
		case NetChange::TRAIN:
			{
				byte type, stat_type;
				//byte co, co2;
				if(!stream.Read(type)
					|| !stream.Read(stat_type))
				{
					ERROR(Format("Update server: Broken TRAIN from %s.", info.name.c_str()));
					StreamEnd(false);
				}
				else
				{
					if(type == 2)
						TournamentTrain(unit);
					else
					{
						cstring error = NULL;
						if(type == 0)
						{
							if(stat_type >= (byte)Attribute::MAX)
								error = "attribute";
						}
						else
						{
							if(stat_type >= (byte)Skill::MAX)
								error = "skill";
						}
						if(error)
						{
							ERROR(Format("Update server: TRAIN from %s, invalid %d %u.", info.name.c_str(), error, stat_type));
							StreamEnd(false);
							break;
						}
						Train(unit, type == 1, stat_type);
					}
					player.Rest(10, false);
					UseDays(&player, 10);
					NetChangePlayer& c = Add1(net_changes_player);
					c.type = NetChangePlayer::END_FALLBACK;
					c.pc = &player;
					info.NeedUpdate();
				}
			}
			break;
		// player wants to change leader
		case NetChange::CHANGE_LEADER:
			{
				byte id;
				if(!stream.Read(id))
				{
					ERROR(Format("Update server: Broken CHANGE_LEADER from %s.", info.name.c_str()));
					StreamEnd(false);
				}
				else if(leader_id != info.id)
				{
					ERROR(Format("Update server: CHANGE_LEADER from %s, player is not leader.", info.name.c_str()));
					StreamEnd(false);
				}
				else
				{
					PlayerInfo* new_leader = GetPlayerInfoTry(id);
					if(!new_leader)
					{
						ERROR(Format("Update server: CHANGE_LEADER from %s, invalid player id %u.", id));
						StreamEnd(false);
						break;
					}

					leader_id = id;
					leader = new_leader->u;

					if(leader_id == my_id)
						AddMsg(txYouAreLeader);
					else
						AddMsg(Format(txPcIsLeader, leader->player->name.c_str()));

					NetChange& c = Add1(net_changes);
					c.type = NetChange::CHANGE_LEADER;
					c.id = id;
				}
			}
			break;
		// player pays credit
		case NetChange::PAY_CREDIT:
			{
				int count;
				if(!stream.Read(count))
				{
					ERROR(Format("Update server: Broken PAY_CREDIT from %s.", info.name.c_str()));
					StreamEnd(false);
				}
				else if(unit.gold < count || player.credit < count || count < 0)
				{
					ERROR(Format("Update server: PAY_CREDIT from %s, invalid count %d (gold %d, credit %d).",
						info.name.c_str(), count, unit.gold, player.credit));
					StreamEnd(false);
				}
				else
				{
					unit.gold -= count;
					PayCredit(&player, count);
				}
			}
			break;
		// player give gold to unit
		case NetChange::GIVE_GOLD:
			{
				int netid, count;
				if(!stream.Read(netid)
					|| !stream.Read(count))
				{
					ERROR(Format("Update server: Broken GIVE_GOLD from %s.", info.name.c_str()));
					StreamEnd(false);
					break;
				}

				Unit* target = FindTeamMember(netid);
				if(!target)
				{
					ERROR(Format("Update server: GIVE_GOLD from %s, missing unit %d.", info.name.c_str(), netid));
					StreamEnd(false);
				}
				else if(target == &unit)
				{
					ERROR(Format("Update server: GIVE_GOLD from %s, wants to give gold to himself.", info.name.c_str()));
					StreamEnd(false);
				}
				else if(count > unit.gold || count < 0)
				{
					ERROR(Format("Update server: GIVE_GOLD from %s, invalid count %d (have %d).", info.name.c_str(), count, unit.gold));
					StreamEnd(false);
				}
				else
				{
					// give gold
					target->gold += count;
					unit.gold -= count;
					if(target->IsPlayer())
					{
						// message about getting gold
						if(target->player != pc)
						{
							NetChangePlayer& c = Add1(net_changes_player);
							c.type = NetChangePlayer::GOLD_RECEIVED;
							c.id = info.id;
							c.ile = count;
							c.pc = target->player;
							GetPlayerInfo(target->player).NeedUpdateAndGold();
						}
						else
						{
							AddMultiMsg(Format(txReceivedGold, count, info.name.c_str()));
							if(sound_volume)
								PlaySound2d(sMoneta);
						}
					}
					else if(player.IsTradingWith(target))
					{
						// message about changing trader gold
						NetChangePlayer& c = Add1(net_changes_player);
						c.type = NetChangePlayer::UPDATE_TRADER_GOLD;
						c.pc = &player;
						c.id = target->netid;
						c.ile = target->gold;
						info.NeedUpdateAndGold();
					}
				}
			}
			break;
		// player drops gold on group
		case NetChange::DROP_GOLD:
			{
				int count;
				if(!stream.Read(count))
				{
					ERROR(Format("Update server: Broken DROP_GOLD from %s.", info.name.c_str()));
					StreamEnd(false);
				}
				else if(count > 0 && count <= unit.gold && unit.IsStanding() && unit.action == A_NONE)
				{
					unit.gold -= count;

					// animation
					unit.action = A_ANIMATION;
					unit.ani->Play("wyrzuca", PLAY_ONCE|PLAY_PRIO2, 0);
					unit.ani->frame_end_info = false;

					// create item
					GroundItem* item = new GroundItem;
					item->item = &gold_item;
					item->count = count;
					item->team_count = 0;
					item->pos = unit.pos;
					item->pos.x -= sin(unit.rot)*0.25f;
					item->pos.z -= cos(unit.rot)*0.25f;
					item->rot = random(MAX_ANGLE);
					AddGroundItem(GetContext(*info.u), item);

					// send info to other players
					if(players > 2)
					{
						NetChange& c = Add1(net_changes);
						c.type = NetChange::DROP_ITEM;
						c.unit = &unit;
					}
				}
				else
				{
					ERROR(Format("Update server: DROP_GOLD from %s, invalid count %d or busy.", info.name.c_str()));
					StreamEnd(false);
				}
			}
			break;
		// player puts gold into container
		case NetChange::PUT_GOLD:
			{
				int count;
				if(!stream.Read(count))
				{
					ERROR(Format("Update server: Broken PUT_GOLD from %s.", info.name.c_str()));
					StreamEnd(false);
				}
				else if(count < 0 || count > unit.gold)
				{
					ERROR(Format("Update server: PUT_GOLD from %s, invalid count %d (have %d).", info.name.c_str(), count, unit.gold));
					StreamEnd(false);
				}
				else if(player.action != PlayerController::Action_LootChest && player.action != PlayerController::Action_LootUnit)
				{
					ERROR(Format("Update server: PUT_GOLD from %s, player is not trading.", info.name.c_str()));
					StreamEnd(false);
				}
				else
				{
					InsertItem(*player.chest_trade, &gold_item, count, 0);
					unit.gold -= count;
				}
			}
			break;
		// player used cheat for fast travel on map
		case NetChange::CHEAT_TRAVEL:
			{
				byte location_index;
				if(!stream.Read(location_index))
				{
					ERROR(Format("Update server: Broken CHEAT_TRAVEL from %s.", info.name.c_str()));
					StreamEnd(false);
				}
				else if(!info.cheats)
				{
					ERROR(Format("Update server: Player %s used CHEAT_TRAVEL without cheats.", info.name.c_str()));
					StreamEnd(false);
				}
				else if(!IsLeader(unit))
				{
					ERROR(Format("Update server: CHEAT_TRAVEL from %s, player is not leader.", info.name.c_str()));
					StreamEnd(false);
				}
				else
				{
					current_location = location_index;
					Location& loc = *locations[location_index];
					if(loc.state == LS_KNOWN)
						loc.state = LS_VISITED;
					world_pos = loc.pos;
					if(open_location != -1)
					{
						LeaveLocation();
						open_location = -1;
					}
					// inform other players
					if(players > 2)
					{
						NetChange& c = Add1(net_changes);
						c.type = NetChange::CHEAT_TRAVEL;
						c.id = location_index;
					}
				}
			}
			break;
		// player used cheat 'hurt'
		case NetChange::CHEAT_HURT:
			{
				int netid;
				if(!stream.Read(netid))
				{
					ERROR(Format("Update server: Broken CHEAT_HURT from %s.", info.name.c_str()));
					StreamEnd(false);
				}
				else if(!info.cheats)
				{
					ERROR(Format("Update server: Player %s used CHEAT_HURT without cheats.", info.name.c_str()));
					StreamEnd(false);
				}
				else
				{
					Unit* target = FindUnit(netid);
					if(target)
						GiveDmg(GetContext(*target), NULL, 100.f, *target);
					else
					{
						ERROR(Format("Update server: CHEAT_HURT from %s, missing unit %d.", info.name.c_str(), netid));
						StreamEnd(false);
					}
				}
			}
			break;
		// player used cheat 'break_action'
		case NetChange::CHEAT_BREAK_ACTION:
			{
				int netid;
				if(!stream.Read(netid))
				{
					ERROR(Format("Update server: Broken CHEAT_BREAK_ACTION from %s.", info.name.c_str()));
					StreamEnd(false);
				}
				else if(!info.cheats)
				{
					ERROR(Format("Update server: Player %s used CHEAT_BREAK_ACTION without cheats.", info.name.c_str()));
					StreamEnd(false);
				}
				else
				{
					Unit* target = FindUnit(netid);
					if(target)
					{
						BreakAction2(*target);
						if(target->IsPlayer() && target->player != pc)
						{
							NetChangePlayer& c = Add1(net_changes_player);
							c.type = NetChangePlayer::BREAK_ACTION;
							c.pc = target->player;
							GetPlayerInfo(c.pc).NeedUpdate();
						}
					}
					else
					{
						ERROR(Format("Update server: CHEAT_BREAK_ACTION from %s, missing unit %d.", info.name.c_str(), netid));
						StreamEnd(false);
					}
				}
			}
			break;
		// player used cheat 'fall'
		case NetChange::CHEAT_FALL:
			{
				int netid;
				if(!stream.Read(netid))
				{
					ERROR(Format("Update server: Broken CHEAT_FALL from %s.", info.name.c_str()));
					StreamEnd(false);
				}
				else if(!info.cheats)
				{
					ERROR(Format("Update server: Player %s used CHEAT_FALL without cheats.", info.name.c_str()));
					StreamEnd(false);
				}
				else
				{
					Unit* target = FindUnit(netid);
					if(target)
						UnitFall(*target);
					else
					{
						ERROR(Format("Update server: CHEAT_FALL from %s, missing unit %d.", info.name.c_str(), netid));
						StreamEnd(false);
					}
				}
			}
			break;
		// player yell to move ai
		case NetChange::YELL:
			PlayerYell(unit);
			break;
		// invalid change
		default:
			ERROR(Format("Update server: Invalid change type %u from %s.", type, info.name.c_str()));
			StreamEnd(false);
			break;
		}

		byte checksum;
		if(!stream.Read(checksum) || checksum != 0xFF)
		{
			ERROR(Format("Update server: Invalid checksum from %s (%u).", info.name.c_str(), change_i));
			StreamEnd(false);
			return true;
		}
	}

	return true;
}

//=================================================================================================
void Game::WriteServerChanges()
{
	for(vector<NetChange>::iterator it = net_changes.begin(), end = net_changes.end(); it != end; ++it)
	{
		NetChange& c = *it;
		net_stream.WriteCasted<byte>(c.type);
		switch(c.type)
		{
		case NetChange::UNIT_POS:
			{
				Unit& u = *c.unit;
				net_stream.Write(u.netid);
				WriteStruct(net_stream, u.pos);
				net_stream.Write(u.rot);
				net_stream.Write(u.ani->groups[0].speed);
				net_stream.WriteCasted<byte>(u.animation);
			}
			break;
		case NetChange::CHANGE_EQUIPMENT:
			net_stream.Write(c.unit->netid);
			net_stream.WriteCasted<byte>(c.id);
			WriteBaseItem(net_stream, c.unit->slots[c.id]);
			break;
		case NetChange::TAKE_WEAPON:
			{
				Unit& u = *c.unit;
				net_stream.Write(u.netid);
				WriteBool(net_stream, c.id != 0);
				net_stream.WriteCasted<byte>(c.id == 0 ? u.weapon_taken : u.weapon_hiding);
			}
			break;
		case NetChange::ATTACK:
			{
				Unit&u = *c.unit;
				net_stream.Write(u.netid);
				byte b = (byte)c.id;
				b |= ((u.attack_id&0xF)<<4);
				net_stream.Write(b);
				net_stream.Write(c.f[1]);
			}
			break;
		case NetChange::CHANGE_FLAGS:
			{
				byte b = 0;
				if(bandyta)
					b |= 0x01;
				if(atak_szalencow)
					b |= 0x02;
				if(anyone_talking)
					b |= 0x04;
				net_stream.Write(b);
			}
			break;
		case NetChange::UPDATE_HP:
			net_stream.Write(c.unit->netid);
			net_stream.Write(c.unit->hp);
			net_stream.Write(c.unit->hpmax);
			break;
		case NetChange::SPAWN_BLOOD:
			net_stream.WriteCasted<byte>(c.id);
			net_stream.Write((cstring)&c.pos, sizeof(VEC3));
			break;
		case NetChange::HURT_SOUND:
		case NetChange::DIE:
		case NetChange::FALL:
		case NetChange::DROP_ITEM:
		case NetChange::STUNNED:
		case NetChange::HELLO:
		case NetChange::TELL_NAME:
		case NetChange::STAND_UP:
		case NetChange::SHOUT:
		case NetChange::CAST_SPELL:
		case NetChange::CREATE_DRAIN:
		case NetChange::HERO_LEAVE:
		case NetChange::REMOVE_USED_ITEM:
		case NetChange::USEABLE_SOUND:
			net_stream.Write(c.unit->netid);
			break;
		case NetChange::PICKUP_ITEM:
			net_stream.Write(c.unit->netid);
			WriteBool(net_stream, c.ile != 0);
			break;
		case NetChange::SPAWN_ITEM:
			WriteItem(net_stream, *c.item);
			break;
		case NetChange::REMOVE_ITEM:
			net_stream.Write(c.id);
			break;
		case NetChange::CONSUME_ITEM:
			{
				net_stream.Write(c.unit->netid);
				const Item* item = (const Item*)c.id;
				WriteString1(net_stream, item->id);
				WriteBool(net_stream, c.ile != 0);
			}
			break;
		case NetChange::HIT_SOUND:
			net_stream.Write((cstring)&c.pos, sizeof(VEC3));
			net_stream.WriteCasted<byte>(c.id);
			net_stream.WriteCasted<byte>(c.ile);
			break;
		case NetChange::SHOOT_ARROW:
			{
				int netid = (c.unit ? c.unit->netid : -1);
				net_stream.Write(netid);
				net_stream.Write((cstring)&c.pos, sizeof(VEC3));
				net_stream.Write(c.f[0]);
				net_stream.Write(c.f[1]);
				net_stream.Write(c.f[2]);
			}
			break;
		case NetChange::UPDATE_CREDIT:
			{
				byte ile = (byte)active_team.size();
				net_stream.Write(ile);
				for(vector<Unit*>::iterator it2 = active_team.begin(), end2 = active_team.end(); it2 != end2; ++it2)
				{
					Unit& u = **it2;
					net_stream.Write(u.netid);
					net_stream.Write(u.IsPlayer() ? u.player->credit : u.hero->credit);
				}
			}
			break;
		case NetChange::UPDATE_FREE_DAYS:
			{
				for(vector<PlayerInfo>::iterator it2 = game_players.begin(), end2 = game_players.end(); it2 != end2; ++it2)
				{
					if(!it2->left)
					{
						net_stream.Write(it2->u->netid);
						net_stream.Write(it2->u->player->free_days);
					}
				}
				net_stream.WriteCasted<int>(-1);
			}
			break;
		case NetChange::IDLE:
			net_stream.Write(c.unit->netid);
			net_stream.WriteCasted<byte>(c.id);
			break;
		case NetChange::ALL_QUESTS_COMPLETED:
		case NetChange::GAME_OVER:
		case NetChange::LEAVE_LOCATION:
		case NetChange::EXIT_TO_MAP:
		case NetChange::EVIL_SOUND:
		case NetChange::CLOSE_ENCOUNTER:
		case NetChange::CLOSE_PORTAL:
		case NetChange::CLEAN_ALTAR:
		case NetChange::CHEAT_SHOW_MINIMAP:
		case NetChange::END_OF_GAME:
		case NetChange::GAME_SAVED:
		case NetChange::ACADEMY_TEXT:
			break;
		case NetChange::CHEST_OPEN:
		case NetChange::CHEST_CLOSE:
		case NetChange::KICK_NPC:
		case NetChange::REMOVE_UNIT:
		case NetChange::REMOVE_TRAP:
		case NetChange::TRIGGER_TRAP:
			net_stream.Write(c.id);
			break;
		case NetChange::TALK:
			net_stream.Write(c.unit->netid);
			net_stream.WriteCasted<byte>(c.id);
			net_stream.Write(c.ile);
			WriteString1(net_stream, *c.str);
			StringPool.Free(c.str);
			RemoveElement(net_talk, c.str);
			break;
		case NetChange::CHANGE_LOCATION_STATE:
			net_stream.WriteCasted<byte>(c.id);
			net_stream.WriteCasted<byte>(c.ile);
			break;
		case NetChange::ADD_RUMOR:
			WriteString1(net_stream, plotki[c.id]);
			break;
		case NetChange::HAIR_COLOR:
			net_stream.Write(c.unit->netid);
			net_stream.Write((cstring)&c.unit->human_data->hair_color, sizeof(VEC4));
			break;
		case NetChange::WARP:
			net_stream.Write(c.unit->netid);
			net_stream.WriteCasted<char>(c.unit->in_building);
			WriteStruct(net_stream, c.unit->pos);
			net_stream.Write(c.unit->rot);
			break;
		case NetChange::REGISTER_ITEM:
			{
				const Item* item = c.base_item;
				WriteString1(net_stream, item->id);
				WriteString1(net_stream, item->name);
				WriteString1(net_stream, item->desc);
				net_stream.Write(item->refid);
			}
			break;
		case NetChange::ADD_QUEST:
		case NetChange::ADD_QUEST_MAIN:
			{
				Quest* q = FindQuest(c.id, false);
				net_stream.Write(q->refid);
				WriteString1(net_stream, q->name);
				WriteString1(net_stream, q->msgs[0]);
				WriteString1(net_stream, q->msgs[1]);
			}
			break;
		case NetChange::UPDATE_QUEST:
			{
				Quest* q = FindQuest(c.id, false);
				net_stream.Write(q->refid);
				net_stream.WriteCasted<byte>(q->state);
				WriteString1(net_stream, q->msgs.back());
			}
			break;
		case NetChange::RENAME_ITEM:
			{
				const Item* item = c.base_item;
				net_stream.Write(item->refid);
				WriteString1(net_stream, item->id);
				WriteString1(net_stream, item->name);
			}
			break;
		case NetChange::UPDATE_QUEST_MULTI:
			{
				Quest* q = FindQuest(c.id, false);
				net_stream.Write(q->refid);
				net_stream.WriteCasted<byte>(q->state);
				net_stream.WriteCasted<byte>(c.ile);
				for(int i = 0; i<c.ile; ++i)
					WriteString1(net_stream, q->msgs[q->msgs.size()-c.ile+i]);
			}
			break;
		case NetChange::CHANGE_LEADER:
		case NetChange::ARENA_SOUND:
		case NetChange::TRAVEL:
		case NetChange::CHEAT_TRAVEL:
		case NetChange::REMOVE_CAMP:
		case NetChange::CHEAT_NOAI:
		case NetChange::PAUSED:
			net_stream.WriteCasted<byte>(c.id);
			break;
		case NetChange::RANDOM_NUMBER:
			net_stream.WriteCasted<byte>(c.unit->player->id);
			net_stream.WriteCasted<byte>(c.id);
			break;
		case NetChange::REMOVE_PLAYER:
			net_stream.WriteCasted<byte>(c.id);
			net_stream.WriteCasted<byte>(c.ile);
			break;
		case NetChange::USE_USEABLE:
			net_stream.Write(c.unit->netid);
			net_stream.Write(c.id);
			net_stream.WriteCasted<byte>(c.ile);
			break;
		case NetChange::RECRUIT_NPC:
			net_stream.Write(c.unit->netid);
			WriteBool(net_stream, c.unit->hero->free);
			break;
		case NetChange::SPAWN_UNIT:
			WriteUnit(net_stream, *c.unit);
			break;
		case NetChange::CHANGE_ARENA_STATE:
			net_stream.Write(c.unit->netid);
			net_stream.WriteCasted<char>(c.unit->in_arena);
			break;
		case NetChange::WORLD_TIME:
			net_stream.Write(worldtime);
			net_stream.WriteCasted<byte>(day);
			net_stream.WriteCasted<byte>(month);
			net_stream.WriteCasted<byte>(year);
			break;
		case NetChange::USE_DOOR:
			net_stream.Write(c.id);
			net_stream.WriteCasted<byte>(c.ile);
			break;
		case NetChange::CREATE_EXPLOSION:
			net_stream.WriteCasted<byte>(c.id);
			net_stream.Write((cstring)&c.pos, sizeof(c.pos));
			break;
		case NetChange::ENCOUNTER:
			WriteString1(net_stream, *c.str);
			StringPool.Free(c.str);
			break;
		case NetChange::ADD_LOCATION:
			{
				Location& loc = *locations[c.id];
				net_stream.WriteCasted<byte>(c.id);
				net_stream.WriteCasted<byte>(loc.type);
				if(loc.type == L_DUNGEON || loc.type == L_CRYPT)
					net_stream.WriteCasted<byte>(loc.GetLastLevel()+1);
				net_stream.WriteCasted<byte>(loc.state);
				net_stream.Write(loc.pos.x);
				net_stream.Write(loc.pos.y);
				WriteString1(net_stream, loc.name);
			}
			break;
		case NetChange::CHANGE_AI_MODE:
			{
				net_stream.Write(c.unit->netid);
				byte mode = 0;
				if(c.unit->dont_attack)
					mode |= 0x01;
				if(c.unit->assist)
					mode |= 0x02;
				if(c.unit->ai->state != AIController::Idle)
					mode |= 0x04;
				if(c.unit->attack_team)
					mode |= 0x08;
				net_stream.Write(mode);
			}
			break;
		case NetChange::CHANGE_UNIT_BASE:
			net_stream.Write(c.unit->netid);
			WriteString1(net_stream, c.unit->data->id);
			break;
		case NetChange::CREATE_SPELL_BALL:
			net_stream.Write(c.unit->netid);
			net_stream.Write((cstring)&c.pos, sizeof(c.pos));
			net_stream.Write(c.f[0]);
			net_stream.Write(c.f[1]);
			net_stream.WriteCasted<byte>(c.i);
			break;
		case NetChange::SPELL_SOUND:
			net_stream.WriteCasted<byte>(c.id);
			net_stream.Write((cstring)&c.pos, sizeof(c.pos));
			break;
		case NetChange::CREATE_ELECTRO:
			net_stream.Write(c.e_id);
			net_stream.Write((cstring)&c.pos, sizeof(c.pos));
			net_stream.Write((cstring)c.f, sizeof(VEC3));
			break;
		case NetChange::UPDATE_ELECTRO:
			net_stream.Write(c.e_id);
			net_stream.Write((cstring)&c.pos, sizeof(c.pos));
			break;
		case NetChange::ELECTRO_HIT:
		case NetChange::RAISE_EFFECT:
		case NetChange::HEAL_EFFECT:
			net_stream.Write((cstring)&c.pos, sizeof(c.pos));
			break;
		case NetChange::REVEAL_MINIMAP:
			net_stream.WriteCasted<word>(minimap_reveal_mp.size());
			for(vector<INT2>::iterator it2 = minimap_reveal_mp.begin(), end2 = minimap_reveal_mp.end(); it2 != end2; ++it2)
			{
				net_stream.WriteCasted<byte>(it2->x);
				net_stream.WriteCasted<byte>(it2->y);
			}
			minimap_reveal_mp.clear();
			break;
		case NetChange::CHANGE_MP_VARS:
			WriteNetVars(net_stream);
			break;
		case NetChange::SECRET_TEXT:
			WriteString1(net_stream, GetSecretNote()->desc);
			break;
		case NetChange::UPDATE_MAP_POS:
			WriteStruct(net_stream, world_pos);
			break;
		case NetChange::GAME_STATS:
			net_stream.Write(total_kills);
			break;
		default:
			WARN(Format("UpdateServer: Unknown change %d.", c.type));
			assert(0);
			break;
		}
	}
}

//=================================================================================================
void Game::UpdateClient(float dt)
{
	if(game_state == GS_LEVEL)
	{
		// interpolacja pozycji gracza
		if(interpolate_timer > 0.f)
		{
			interpolate_timer -= dt;
			if(interpolate_timer >= 0.f)
				pc->unit->visual_pos = lerp(pc->unit->visual_pos, pc->unit->pos, (0.1f-interpolate_timer)*10);
			else
				pc->unit->visual_pos = pc->unit->pos;
		}

		// interpolacja pozycji/obrotu postaci
		InterpolateUnits(dt);
	}

	bool exit_from_server = false;

	Packet* packet;
	for(packet=peer->Receive(); packet; peer->DeallocatePacket(packet), packet=peer->Receive())
	{
		BitStream& stream = StreamStart(packet, Stream_UpdateGameClient);
		byte msg_id;
		stream.Read(msg_id);

		switch(msg_id)
		{
		case ID_CONNECTION_LOST:
		case ID_DISCONNECTION_NOTIFICATION:
			{
				cstring text, text_eng;
				if(packet->data[0] == ID_CONNECTION_LOST)
				{
					text = txLostConnection;
					text_eng = "Update client: Lost connection with server.";
				}
				else
				{
					text = txYouDisconnected;
					text_eng = "Update client: You have been disconnected.";
				}
				LOG(text_eng);
				StreamEnd();
				peer->DeallocatePacket(packet);
				ExitToMenu();
				GUI.SimpleDialog(text, NULL);
				return;
			}
		case ID_SAY:
			Client_Say(stream);
			break;
		case ID_WHISPER:
			Client_Whisper(stream);
			break;
		case ID_SERVER_SAY:
			Client_ServerSay(stream);
			break;
		case ID_SERVER_CLOSE:
			{
				LOG("Update client: You have been kicked out.");
				StreamEnd();
				peer->DeallocatePacket(packet);
				ExitToMenu();
				GUI.SimpleDialog(txYouKicked, NULL);
				return;
			}
		case ID_CHANGE_LEVEL:
			{
				byte loc, level;
				if(!stream.Read(loc)
					|| !stream.Read(level))
				{
					ERROR("Update client: Broken ID_CHANGE_LEVEL.");
					StreamEnd(false);
				}
				else
				{
					byte loc = packet->data[1];
					byte level = packet->data[2];
					current_location = loc;
					location = locations[loc];
					dungeon_level = level;
					LOG(Format("Update client: Level change to %s (%d, level %d).", location->name.c_str(), current_location, dungeon_level));
					info_box->Show(txGeneratingLocation);
					LeaveLevel();
					net_mode = NM_TRANSFER;
					net_state = 2;
					clear_color = BLACK;
					load_screen->visible = true;
					game_gui->visible = false;
					world_map->visible = false;
					if(pvp_response.ok && pvp_response.to == pc->unit)
					{
						if(dialog_pvp)
						{
							GUI.CloseDialog(dialog_pvp);
							delete dialog_pvp;
							dialog_pvp = NULL;
						}
						pvp_response.ok = false;
					}
					if(dialog_enc)
					{
						GUI.CloseDialog(dialog_enc);
						delete dialog_enc;
						dialog_enc = NULL;
					}
					peer->DeallocatePacket(packet);
					StreamEnd(false);
					Net_FilterClientChanges();
					LoadingStart(4);
					return;
				}
			}
			break;
		case ID_CHANGES:
			if(!ProcessControlMessageClient(stream, exit_from_server))
			{
				peer->DeallocatePacket(packet);
				return;
			}
			break;
		case ID_PLAYER_UPDATE:
			if(!ProcessControlMessageClientForMe(stream))
			{
				peer->DeallocatePacket(packet);
				return;
			}
			break;
		default:
			WARN(Format("UpdateClient: Unknown packet from server: %u.", msg_id));
			StreamEnd(false);
			break;
		}

		StreamEnd();
	}

	// wyœli moj¹ pozycjê/akcjê
	update_timer += dt;
	if(update_timer >= TICK)
	{
		update_timer = 0;
		net_stream.Reset();
		net_stream.WriteCasted<byte>(ID_CONTROL);
		if(game_state == GS_LEVEL)
		{
			net_stream.WriteCasted<byte>(1);
			net_stream.Write((cstring)&pc->unit->pos, sizeof(VEC3));
			net_stream.Write(pc->unit->rot);
			net_stream.Write(pc->unit->ani->groups[0].speed);
			net_stream.WriteCasted<byte>(pc->unit->animation);
		}
		else
			net_stream.WriteCasted<byte>(0);
		// zmiany
		net_stream.WriteCasted<byte>(net_changes.size());
		if(net_changes.size() > 255)
			ERROR(Format("Too many changes %d!", net_changes.size()));
		for(vector<NetChange>::iterator it = net_changes.begin(), end = net_changes.end(); it != end; ++it)
		{
			net_stream.WriteCasted<byte>(it->type);
			switch(it->type)
			{
			case NetChange::CHANGE_EQUIPMENT:
			case NetChange::IS_BETTER_ITEM:
			case NetChange::CHEAT_TRAVEL:
			case NetChange::CONSUME_ITEM:
				net_stream.Write(it->id);
				break;
			case NetChange::TAKE_WEAPON:
				WriteBool(net_stream, it->id != 0);
				net_stream.WriteCasted<byte>(it->id == 0 ? pc->unit->weapon_taken : pc->unit->weapon_hiding);
				break;
			case NetChange::ATTACK:
				{
					byte b = (byte)it->id;
					b |= ((pc->unit->attack_id&0xF)<<4);
					net_stream.Write(b);
					net_stream.Write(it->f[1]);
					if(it->id == 2)
						net_stream.Write(PlayerAngleY()*12);
				}
				break;
			case NetChange::DROP_ITEM:
				net_stream.Write(it->id);
				net_stream.Write(it->ile);
				break;
			case NetChange::IDLE:
			case NetChange::CHOICE:
			case NetChange::ENTER_BUILDING:
			case NetChange::CHANGE_LEADER:
			case NetChange::RANDOM_NUMBER:
			case NetChange::CHEAT_GODMODE:
			case NetChange::CHEAT_INVISIBLE:
			case NetChange::CHEAT_NOCLIP:
			case NetChange::CHEAT_WARP:
			case NetChange::PVP:
			case NetChange::TRAVEL:
			case NetChange::CHEAT_CHANGE_LEVEL:
			case NetChange::CHEAT_WARP_TO_STAIRS:
			case NetChange::CHEAT_NOAI:
			case NetChange::REST:
				net_stream.WriteCasted<byte>(it->id);
				break;
			case NetChange::PICKUP_ITEM:
			case NetChange::LOOT_UNIT:
			case NetChange::TALK:
			case NetChange::LOOT_CHEST:
			case NetChange::SKIP_DIALOG:
			case NetChange::CHEAT_ADDGOLD:
			case NetChange::CHEAT_ADDGOLD_TEAM:
			case NetChange::CHEAT_SKIP_DAYS:
			case NetChange::PAY_CREDIT:
			case NetChange::DROP_GOLD:
			case NetChange::TAKE_ITEM_CREDIT:
				net_stream.Write(it->id);
				break;
			case NetChange::STOP_TRADE:
			case NetChange::GET_ALL_ITEMS:
			case NetChange::EXIT_BUILDING:
			case NetChange::WARP:
			case NetChange::CHEAT_SUICIDE:
			case NetChange::STAND_UP:
			case NetChange::CHEAT_SCARE:
			case NetChange::CHEAT_CITIZEN:
			case NetChange::CHEAT_HEAL:
			case NetChange::CHEAT_REVEAL:
			case NetChange::CHEAT_GOTO_MAP:
			case NetChange::CHEAT_SHOW_MINIMAP:
			case NetChange::ENTER_LOCATION:
			case NetChange::TRAIN_MOVE:
			case NetChange::CLOSE_ENCOUNTER:
			case NetChange::YELL:
				break;
			case NetChange::ADD_NOTE:
				WriteString1(net_stream, notes.back());
				break;
			case NetChange::USE_USEABLE:
				net_stream.Write(it->id);
				net_stream.WriteCasted<byte>(it->ile);
				break;
			case NetChange::CHEAT_KILLALL:
				net_stream.Write(it->unit ? it->unit->netid : -1);
				net_stream.WriteCasted<byte>(it->id);
				break;
			case NetChange::CHEAT_KILL:
			case NetChange::CHEAT_HEALUNIT:
			case NetChange::CHEAT_HURT:
			case NetChange::CHEAT_BREAK_ACTION:
			case NetChange::CHEAT_FALL:
				net_stream.Write(it->unit->netid);
				break;
			case NetChange::CHEAT_ADDITEM:
				WriteString1(net_stream, it->base_item->id);
				net_stream.WriteCasted<byte>(it->ile);
				net_stream.WriteCasted<byte>(it->id);
				break;
			case NetChange::CHEAT_SPAWN_UNIT:
				WriteString1(net_stream, it->base_unit->id);
				net_stream.WriteCasted<byte>(it->ile);
				net_stream.WriteCasted<char>(it->id);
				net_stream.WriteCasted<char>(it->i);
				break;
			case NetChange::CHEAT_SETSTAT:
			case NetChange::CHEAT_MODSTAT:
				net_stream.WriteCasted<byte>(it->id);
				net_stream.WriteCasted<byte>(it->ile);
				net_stream.WriteCasted<char>(it->i);
				break;
			case NetChange::LEAVE_LOCATION:
				net_stream.WriteCasted<char>(it->id);
				break;
			case NetChange::USE_DOOR:
				net_stream.Write(it->id);
				net_stream.WriteCasted<byte>(it->ile);
				break;
			case NetChange::TRAIN:
				net_stream.WriteCasted<byte>(it->id);
				net_stream.WriteCasted<byte>(it->ile);
				break;
			case NetChange::GIVE_GOLD:
			case NetChange::GET_ITEM:
			case NetChange::PUT_ITEM:
				net_stream.Write(it->id);
				net_stream.Write(it->ile);
				break;
			case NetChange::PUT_GOLD:
				net_stream.Write(it->ile);
				break;
			default:
				WARN(Format("UpdateClient: Unknown change %d.", it->type));
				assert(0);
				break;
			}
		}
		net_changes.clear();
		peer->Send(&net_stream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, server, false);
	}

	if(exit_from_server)
		peer->Shutdown(1000);
}

//=================================================================================================
bool Game::ProcessControlMessageClient(BitStream& stream, bool& exit_from_server)
{
	// read count
	word changes;
	if(!stream.Read(changes))
	{
		ERROR("Update client: Broken ID_CHANGES.");
		StreamEnd(false);
		return true;
	}

	// read changes
	for(word change_i = 0; change_i<changes; ++change_i)
	{
		// read type
		NetChange::TYPE type;
		if(!stream.ReadCasted<byte>(type))
		{
			ERROR("Update client: Broken ID_CHANGES(2).");
			StreamEnd(false);
			return true;
		}

		// process
		switch(type)
		{
		// unit position/rotation/animation
		case NetChange::UNIT_POS:
			{
				int netid;
				VEC3 pos;
				float rot, speed;
				Animation ani;

				if(!stream.Read(netid)
					|| !ReadStruct(stream, pos)
					|| !stream.Read(rot)
					|| !stream.Read(speed)
					|| !stream.ReadCasted<byte>(ani))
				{
					ERROR("Update client: Broken UNIT_POS.");
					StreamEnd(false);
					break;
				}

				Unit* unit = FindUnit(netid);
				if(!unit)
				{
					ERROR(Format("Update client: UNIT_POS, missing unit %d.", netid));
					StreamEnd(false);
				}
				else if(unit != pc->unit)
				{
					unit->pos = pos;
					unit->ani->groups[0].speed = speed;
					assert(ani < ANI_MAX);
					if(unit->animation != ANI_PLAY && ani != ANI_PLAY)
						unit->animation = ani;
					UpdateUnitPhysics(*unit, unit->pos);
					unit->interp->Add(pos, rot);
				}
			}
			break;
		// unit changed equipped item
		case NetChange::CHANGE_EQUIPMENT:
			{
				// [int(netid)-jednostka, byte(id)-item slot, Item-przedmiot]
				int netid;
				ITEM_SLOT type;
				const Item* item;
				cstring error;
				if(!stream.Read(netid)
					|| !stream.ReadCasted<byte>(type)
					|| ReadItemAndFind2(stream, item, error) == -2)
				{
					ERROR("Update client: Broken CHANGE_EQUIPMENT.");
					StreamEnd(false);
				}
				else if(error)
				{
					ERROR(Format("Update client: CHANGE_EQUIPMENT, %s", error));
					StreamEnd(false);
				}
				else if(!IsValid(type))
				{
					ERROR(Format("Update client: CHANGE_EQUIPMENT, invalid slot %d.", type));
					StreamEnd(false);
				}
				else
				{
					Unit* target = FindUnit(netid);
					if(!target)
					{
						ERROR(Format("Update client: CHANGE_EQUIPMENT, missing unit %d.", netid));
						StreamEnd(false);
					}
					else
						target->slots[type] = item;
				}
			}
			break;
		// unit take/hide weapon
		case NetChange::TAKE_WEAPON:
			{
				int netid;
				bool hide;
				WeaponType type;
				if(!stream.Read(netid)
					|| !ReadBool(stream, hide)
					|| !stream.ReadCasted<byte>(type))
				{
					ERROR("Update client: Broken TAKE_WEAPON.");
					StreamEnd(false);
				}
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
					{
						ERROR(Format("Update client: TAKE_WEAPON, missing unit %d.", netid));
						StreamEnd(false);
					}
					else if(unit != pc->unit)
					{
						if(unit->ani->ani->head.n_groups > 1)
							unit->ani->groups[1].speed = 1.f;
						SetUnitWeaponState(*unit, !hide, type);
					}
				}
			}
			break;
		// unit attack
		case NetChange::ATTACK:
			{
				int netid;
				byte typeflags;
				float speed;
				if(!stream.Read(netid)
					|| !stream.Read(typeflags)
					|| !stream.Read(speed))
				{
					ERROR("Update client: Broken ATTACK.");
					StreamEnd(false);
					break;
				}

				Unit* unit_ptr = FindUnit(netid);
				if(!unit_ptr)
				{
					ERROR(Format("Update client: ATTACK, missing unit %d.", netid));
					StreamEnd(false);
					break;
				}

				// don't start attack if this is local unit
				if(unit_ptr == pc->unit)
					break;

				Unit& unit = *unit_ptr;
				byte type = (typeflags & 0xF);
				int group = unit.ani->ani->head.n_groups - 1;
				unit.weapon_state = WS_TAKEN;

				switch(type)
				{
				case AID_Attack:
					if(unit.action == A_ATTACK && unit.animation_state == 0)
					{
						unit.animation_state = 1;
						unit.ani->groups[1].speed = speed;
					}
					else
					{
						if(sound_volume > 0 && unit.data->sounds->sound[SOUND_ATTACK] && rand2()%4 == 0)
							PlayAttachedSound(unit, unit.data->sounds->sound[SOUND_ATTACK], 1.f, 10.f);
						unit.action = A_ATTACK;
						unit.attack_id = ((typeflags & 0xF0)>>4);
						unit.attack_power = 1.f;
						unit.ani->Play(NAMES::ani_attacks[unit.attack_id], PLAY_PRIO1|PLAY_ONCE|PLAY_RESTORE, group);
						unit.ani->groups[group].speed = speed;
						unit.animation_state = 1;
						unit.hitted = false;
					}
					break;
				case AID_PowerAttack:
					{
						if(sound_volume > 0 && unit.data->sounds->sound[SOUND_ATTACK] && rand2()%4 == 0)
							PlayAttachedSound(unit, unit.data->sounds->sound[SOUND_ATTACK], 1.f, 10.f);
						unit.action = A_ATTACK;
						unit.attack_id = ((typeflags & 0xF0)>>4);
						unit.attack_power = 1.f;
						unit.ani->Play(NAMES::ani_attacks[unit.attack_id], PLAY_PRIO1|PLAY_ONCE|PLAY_RESTORE, group);
						unit.ani->groups[group].speed = speed;
						unit.animation_state = 0;
						unit.hitted = false;
					}
					break;
				case AID_Shoot:
				case AID_StartShoot:
					if(unit.action == A_SHOOT && unit.animation_state == 0)
						unit.animation_state = 1;
					else
					{
						unit.ani->Play(NAMES::ani_shoot, PLAY_PRIO1|PLAY_ONCE|PLAY_RESTORE, group);
						unit.ani->groups[group].speed = speed;
						unit.action = A_SHOOT;
						unit.animation_state = (type == AID_Shoot ? 1 : 0);
						unit.hitted = false;
						if(!unit.bow_instance)
						{
							if(bow_instances.empty())
								unit.bow_instance = new AnimeshInstance(unit.GetBow().ani);
							else
							{
								unit.bow_instance = bow_instances.back();
								bow_instances.pop_back();
								unit.bow_instance->ani = unit.GetBow().ani;
							}
						}
						unit.bow_instance->Play(&unit.bow_instance->ani->anims[0], PLAY_ONCE|PLAY_PRIO1|PLAY_NO_BLEND, 0);
						unit.bow_instance->groups[0].speed = unit.ani->groups[group].speed;
					}
					break;
				case AID_Block:
					{
						unit.action = A_BLOCK;
						unit.ani->Play(NAMES::ani_block, PLAY_PRIO1|PLAY_STOP_AT_END|PLAY_RESTORE, group);
						unit.ani->groups[1].speed = 1.f;
						unit.ani->groups[1].blend_max = speed;
						unit.animation_state = 0;
					}
					break;
				case AID_Bash:
					{
						unit.action = A_BASH;
						unit.animation_state = 0;
						unit.ani->Play(NAMES::ani_bash, PLAY_ONCE|PLAY_PRIO1|PLAY_RESTORE, group);
						unit.ani->groups[1].speed = 2.f;
						unit.ani->frame_end_info2 = false;
						unit.hitted = false;
					}
					break;
				case AID_RunningAttack:
					{
						if(sound_volume > 0 && unit.data->sounds->sound[SOUND_ATTACK] && rand2()%4 == 0)
							PlayAttachedSound(unit, unit.data->sounds->sound[SOUND_ATTACK], 1.f, 10.f);
						unit.action = A_ATTACK;
						unit.attack_id = ((typeflags & 0xF0)>>4);
						unit.attack_power = 1.5f;
						unit.run_attack = true;
						unit.ani->Play(NAMES::ani_attacks[unit.attack_id], PLAY_PRIO1|PLAY_ONCE|PLAY_RESTORE, group);
						unit.ani->groups[group].speed = speed;
						unit.animation_state = 1;
						unit.hitted = false;
					}
					break;
				case AID_StopBlock:
					{
						unit.action = A_NONE;
						unit.ani->frame_end_info2 = false;
						unit.ani->Deactivate(group);
						unit.ani->groups[1].speed = 1.f;
					}
					break;
				}
			}
			break;
		// change of game flags
		case NetChange::CHANGE_FLAGS:
			{
				byte flags;
				if(!stream.Read(flags))
				{
					ERROR("Update client: Broken CHANGE_FLAGS.");
					StreamEnd(false);
				}
				else
				{
					bandyta = IS_SET(flags, 0x01);
					atak_szalencow = IS_SET(flags, 0x02);
					anyone_talking = IS_SET(flags, 0x04);
				}
			}
			break;
		// update unit hp
		case NetChange::UPDATE_HP:
			{
				int netid;
				float hp, hpmax;
				if(!stream.Read(netid)
					|| !stream.Read(hp)
					|| !stream.Read(hpmax))
				{
					ERROR("Update client: Broken UPDATE_HP.");
					StreamEnd(false);
				}
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
					{
						ERROR(Format("Update client: UPDATE_HP, missing unit %d.", netid));
						StreamEnd(false);
					}
					else if(unit == pc->unit)
					{
						// handling of previous hp
						float hp_prev = unit->hp;
						unit->hp = hp;
						unit->hpmax = hpmax;
						hp_prev -= unit->hp;
						if(hp_prev > 0.f)
							pc->last_dmg += hp_prev;
					}
					else
					{
						unit->hp = hp;
						unit->hpmax = hpmax;
					}
				}
			}
			break;
		// spawn blood
		case NetChange::SPAWN_BLOOD:
			{
				byte type;
				VEC3 pos;
				if(!stream.Read(type)
					|| !ReadStruct(stream, pos))
				{
					ERROR("Update client: Broken SPAWN_BLOOD.");
					StreamEnd(false);
				}
				else
				{
					ParticleEmitter* pe = new ParticleEmitter;
					pe->tex = tKrew[type];
					pe->emision_interval = 0.01f;
					pe->life = 5.f;
					pe->particle_life = 0.5f;
					pe->emisions = 1;
					pe->spawn_min = 10;
					pe->spawn_max = 15;
					pe->max_particles = 15;
					pe->pos = pos;
					pe->speed_min = VEC3(-1, 0, -1);
					pe->speed_max = VEC3(1, 1, 1);
					pe->pos_min = VEC3(-0.1f, -0.1f, -0.1f);
					pe->pos_max = VEC3(0.1f, 0.1f, 0.1f);
					pe->size = 0.3f;
					pe->op_size = POP_LINEAR_SHRINK;
					pe->alpha = 0.9f;
					pe->op_alpha = POP_LINEAR_SHRINK;
					pe->mode = 0;
					pe->Init();
					GetContext(pos).pes->push_back(pe);
				}
			}
			break;
		// play unit hurt sound
		case NetChange::HURT_SOUND:
			{
				int netid;
				if(!stream.Read(netid))
				{
					ERROR("Update client: Broken HURT_SOUND.");
					StreamEnd(false);
				}
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
					{
						ERROR(Format("Update client: HURT_SOUND, missing unit %d.", netid));
						StreamEnd(false);
					}
					else if(sound_volume)
						PlayAttachedSound(*unit, unit->data->sounds->sound[SOUND_PAIN], 2.f, 15.f);
				}
			}
			break;
		// unit dies
		case NetChange::DIE:
			{
				int netid;
				if(!stream.Read(netid))
				{
					ERROR("Update client: Broken DIE.");
					StreamEnd(false);
				}
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
					{
						ERROR(Format("Update client: DIE, missing unit %d.", netid));
						StreamEnd(false);
					}
					else
						UnitDie(*unit, NULL, NULL);
				}
			}
			break;
		// unit falls on ground
		case NetChange::FALL:
			{
				int netid;
				if(!stream.Read(netid))
				{
					ERROR("Update client: Broken FALL.");
					StreamEnd(false);
				}
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
					{
						ERROR(Format("Update client: FALL, missing unit %d.", netid));
						StreamEnd(false);
					}
					else
						UnitFall(*unit);
				}
			}
			break;
		// unit drops item animation
		case NetChange::DROP_ITEM:
			{
				int netid;
				if(!stream.Read(netid))
				{
					ERROR("Update client: Broken DROP_ITEM.");
					StreamEnd(false);
				}
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
					{
						ERROR(Format("Update client: DROP_ITEM, missing unit %d.", netid));
						StreamEnd(false);
					}
					else if(unit != pc->unit)
					{
						unit->action = A_ANIMATION;
						unit->ani->Play("wyrzuca", PLAY_ONCE|PLAY_PRIO2, 0);
						unit->ani->frame_end_info = false;
					}
				}
			}
			break;
		// spawn item on ground
		case NetChange::SPAWN_ITEM:
			{
				GroundItem* item = new GroundItem;
				if(!ReadItem(stream, *item))
				{
					ERROR("Update client: Broken SPAWN_ITEM.");
					StreamEnd(false);
					delete item;
				}
				else
					GetContext(item->pos).items->push_back(item);
			}
			break;
		// unit picks up item
		case NetChange::PICKUP_ITEM:
			{
				int netid;
				bool up_animation;
				if(!stream.Read(netid)
					|| !ReadBool(stream, up_animation))
				{
					ERROR("Update client: Broken PICKUP_ITEM.");
					StreamEnd(false);
				}
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
					{
						ERROR(Format("Update client: PICKUP_ITEM, missing unit %d.", netid));
						StreamEnd(false);
					}
					else if(unit != pc->unit)
					{
						unit->action = A_PICKUP;
						unit->animation = ANI_PLAY;
						unit->ani->Play(up_animation ? "podnosi_gora" : "podnosi", PLAY_ONCE|PLAY_PRIO2, 0);
						unit->ani->frame_end_info = false;
					}
				}
			}
			break;
		// remove ground item (picked by someone)
		case NetChange::REMOVE_ITEM:
			{
				int netid;
				if(!stream.Read(netid))
				{
					ERROR("Update client: Broken REMOVE_ITEM.");
					StreamEnd(false);
				}
				else
				{
					LevelContext* ctx;
					GroundItem* item = FindItemNetid(netid, &ctx);
					if(!item)
					{
						ERROR(Format("Update client: REMOVE_ITEM, missing ground item %d.", netid));
						StreamEnd(false);
					}
					else
					{
						if(before_player == BP_ITEM && before_player_ptr.item == item)
							before_player = BP_NONE;
						if(picking_item_state == 1 && picking_item == item)
							picking_item_state = 2;
						else
							delete item;
						RemoveElement(ctx->items, item);
					}
				}
			}
			break;
		// unit consume item
		case NetChange::CONSUME_ITEM:
			{
				int netid;
				bool force;
				const Item* item;
				cstring error;
				if(!stream.Read(netid)
					|| ReadItemAndFind2(stream, item, error) == -2
					|| !ReadBool(stream, force))
				{
					ERROR("Update client: Broken CONSUME_ITEM.");
					StreamEnd(false);
				}
				else if(error)
				{
					ERROR(Format("Update client: CONSUME_ITEM, %s", error));
					StreamEnd(false);
				}
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
					{
						ERROR(Format("Update client: CONSUME_ITEM, missing unit %d.", netid));
						StreamEnd(false);
					}
					else if(item->type != IT_CONSUMEABLE)
					{
						ERROR(Format("Update client: CONSUME_ITEM, %s is not consumeable.", item->id.c_str()));
						StreamEnd(false);
					}
					else if(unit != pc->unit || force)
						unit->ConsumeItem(item->ToConsumeable(), false, false);
				}
			}
			break;
		// play hit sound
		case NetChange::HIT_SOUND:
			{
				VEC3 pos;
				MATERIAL_TYPE mat1, mat2;
				if(!ReadStruct(stream, pos)
					|| !stream.ReadCasted<byte>(mat1)
					|| !stream.ReadCasted<byte>(mat2))
				{
					ERROR("Update client: Broken HIT_SOUND.");
					StreamEnd(false);
				}
				else if(sound_volume)
					PlaySound3d(GetMaterialSound(mat1, mat2), pos, 2.f, 10.f);
			}
			break;
		// unit get stunned
		case NetChange::STUNNED:
			{
				int netid;
				if(!stream.Read(netid))
				{
					ERROR("Update client: Broken STUNNED.");
					StreamEnd(false);
				}
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
					{
						ERROR(Format("Update client: STUNNED, missing unit %d.", netid));
						StreamEnd(false);
					}
					else
					{
						BreakAction2(*unit);

						if(unit->action != A_POSITION)
							unit->action = A_PAIN;
						else
							unit->animation_state = 1;

						if(unit->ani->ani->head.n_groups == 2)
						{
							unit->ani->frame_end_info2 = false;
							unit->ani->Play(NAMES::ani_hurt, PLAY_PRIO1|PLAY_ONCE, 1);
						}
						else
						{
							unit->ani->frame_end_info = false;
							unit->ani->Play(NAMES::ani_hurt, PLAY_PRIO3|PLAY_ONCE, 0);
							unit->animation = ANI_PLAY;
						}
					}
				}
			}
			break;
		// create shooted arrow
		case NetChange::SHOOT_ARROW:
			{
				int netid;
				VEC3 pos;
				float rotX, rotY, speedY;
				if(!stream.Read(netid)
					|| !ReadStruct(stream, pos)
					|| !stream.Read(rotY)
					|| !stream.Read(speedY)
					|| !stream.Read(rotX))
				{
					ERROR("Update client: Broken SHOOT_ARROW.");
					StreamEnd(false);
				}
				else
				{
					Unit* owner;
					if(netid == -1)
						owner = NULL;
					else
					{
						owner = FindUnit(netid);
						if(!owner)
						{
							ERROR(Format("Update client: SHOOT_ARROW, missing unit %d.", netid));
							StreamEnd(false);
							break;
						}
					}

					LevelContext& ctx = GetContext(pos);

					Bullet& b = Add1(ctx.bullets);
					b.mesh = aArrow;
					b.pos = pos;
					b.rot = VEC3(rotY, 0, rotX);
					b.yspeed = speedY;
					b.owner = NULL;
					b.pe = NULL;
					b.remove = false;
					b.speed = ARROW_SPEED;
					b.spell = NULL;
					b.tex = NULL;
					b.tex_size = 0.f;
					b.timer = ARROW_TIMER;
					b.owner = owner;

					TrailParticleEmitter* tpe = new TrailParticleEmitter;
					tpe->fade = 0.3f;
					tpe->color1 = VEC4(1, 1, 1, 0.5f);
					tpe->color2 = VEC4(1, 1, 1, 0);
					tpe->Init(50);
					ctx.tpes->push_back(tpe);
					b.trail = tpe;

					TrailParticleEmitter* tpe2 = new TrailParticleEmitter;
					tpe2->fade = 0.3f;
					tpe2->color1 = VEC4(1, 1, 1, 0.5f);
					tpe2->color2 = VEC4(1, 1, 1, 0);
					tpe2->Init(50);
					ctx.tpes->push_back(tpe2);
					b.trail2 = tpe2;

					if(sound_volume)
						PlaySound3d(sBow[rand2()%2], b.pos, 2.f, 8.f);
				}
			}
			break;
		// update team credit
		case NetChange::UPDATE_CREDIT:
			{
				byte count;
				if(!stream.Read(count))
				{
					ERROR("Update client: Broken UPDATE_CREDIT.");
					StreamEnd(false);
				}
				else
				{
					for(byte i = 0; i<count; ++i)
					{
						int netid, credit;
						if(!stream.Read(netid)
							|| !stream.Read(credit))
						{
							ERROR("Update client: Broken UPDATE_CREDIT(2).");
							StreamEnd(false);
						}
						else
						{
							Unit* unit = FindUnit(netid);
							if(!unit)
							{
								ERROR(Format("Update client: UPDATE_CREDIT, missing unit %d.", netid));
								StreamEnd(false);
							}
							else
							{
								if(unit->IsPlayer())
									unit->player->credit = credit;
								else
									unit->hero->credit = credit;
							}
						}
					}
				}
			}
			break;
		// unit playes idle animation
		case NetChange::IDLE:
			{
				int netid;
				byte animation_index;
				if(!stream.Read(netid)
					|| !stream.Read(animation_index))
				{
					ERROR("Update client: Broken IDLE.");
					StreamEnd(false);
				}
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
					{
						ERROR(Format("Update client: IDLE, missing unit %d.", netid));
						StreamEnd(false);
					}
					else if(animation_index >= unit->data->idles->size())
					{
						ERROR(Format("Update client: IDLE, invalid animation index %u (count %u).", animation_index, unit->data->idles->size()));
						StreamEnd(false);
					}
					else
					{
						unit->ani->Play(unit->data->idles->at(animation_index).c_str(), PLAY_ONCE, 0);
						unit->ani->frame_end_info = false;
						unit->animation = ANI_IDLE;
					}
				}
			}
			break;
		// play unit hello sound
		case NetChange::HELLO:
			{
				int netid;
				if(!stream.Read(netid))
				{
					ERROR("Update client: Broken HELLO.");
					StreamEnd(false);
				}
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
					{
						ERROR(Format("Update client: BROKEN, missing unit %d.", netid));
						StreamEnd(false);
					}
					else
					{
						SOUND snd = GetTalkSound(*unit);
						if(sound_volume && snd)
							PlayAttachedSound(*unit, snd, 2.f, 5.f);
					}
				}
			}
			break;
		// info about completing all unique quests
		case NetChange::ALL_QUESTS_COMPLETED:
			unique_completed_show = true;
			break;
		// unit talks
		case NetChange::TALK:
			{
				int netid, skip_id;
				byte animation;
				if(!stream.Read(netid)
					|| !stream.Read(animation)
					|| !stream.Read(skip_id)
					|| ReadString1(stream))
				{
					ERROR("Update client: Broken TALK.");
					StreamEnd(false);
				}
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
					{
						ERROR(Format("Update client: TALK, missing unit %d.", netid));
						StreamEnd(false);
					}
					else
					{
						game_gui->AddSpeechBubble(unit, BUF);
						unit->bubble->skip_id = skip_id;

						if(animation != 0)
						{
							unit->ani->Play(animation == 1 ? "i_co" : "pokazuje", PLAY_ONCE|PLAY_PRIO2, 0);
							unit->animation = ANI_PLAY;
							unit->action = A_ANIMATION;
						}

						if(dialog_context.dialog_mode && dialog_context.talker == unit)
						{
							dialog_context.dialog_s_text = BUF;
							dialog_context.dialog_text = dialog_context.dialog_s_text.c_str();
							dialog_context.dialog_wait = 1.f;
							dialog_context.skip_id = skip_id;
						}
						else if(pc->action == PlayerController::Action_Talk && pc->action_unit == unit)
						{
							predialog = BUF;
							dialog_context.skip_id = skip_id;
						}
					}
				}
			}
			break;
		// change location state
		case NetChange::CHANGE_LOCATION_STATE:
			{
				byte location_index, state;
				if(!stream.Read(location_index)
					|| !stream.Read(state))
				{
					ERROR("Update client: Broken CHANGE_LOCATION_STATE.");
					StreamEnd(false);
				}
				else
				{
					Location* loc = NULL;
					if(location_index < locations.size())
						loc = locations[location_index];
					if(!loc)
					{
						ERROR(Format("Update client: CHANGE_LOCATION_STATE, missing location %u.", location_index));
						StreamEnd(false);
					}
					else
					{
						if(state == 0)
							loc->state = LS_KNOWN;
						else if(state == 1)
							loc->state = LS_VISITED;
					}
				}
			}
			break;
		// add rumor to journal
		case NetChange::ADD_RUMOR:
			if(!ReadString1(stream))
			{
				ERROR("Update client: Broken ADD_RUMOR.");
				StreamEnd(false);
			}
			else
			{
				AddGameMsg3(GMS_ADDED_RUMOR);
				plotki.push_back(BUF);
				game_gui->journal->NeedUpdate(Journal::Rumors);
			}
			break;
		// hero tells his name
		case NetChange::TELL_NAME:
			{
				int netid;
				if(!stream.Read(netid))
				{
					ERROR("Update client: Broken TELL_NAME.");
					StreamEnd(false);
				}
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
					{
						ERROR(Format("Update client: TELL_NAME, missing unit %d.", netid));
						StreamEnd(false);
					}
					else if(!unit->IsHero())
					{
						ERROR(Format("Update client: TELL_NAME, unit %d (%s) is not a hero.", netid, unit->data->id.c_str()));
						StreamEnd(false);
					}
					else
						unit->hero->know_name = true;
				}
			}
			break;
		// change unit hair color
		case NetChange::HAIR_COLOR:
			{
				int netid;
				VEC4 hair_color;
				if(!stream.Read(netid)
					|| !ReadStruct(stream, hair_color))
				{
					ERROR("Update client: Broken HAIR_COLOR.");
					StreamEnd(false);
				}
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
					{
						ERROR(Format("Update client: HAIR_COLOR, missing unit %d.", netid));
						StreamEnd(false);
					}
					else if(unit->type != Unit::HUMAN)
					{
						ERROR(Format("Update client: HAIR_COLOR, unit %d (%s) is not human.", netid, unit->data->id.c_str()));
						StreamEnd(false);
					}
					else
						unit->human_data->hair_color = hair_color;
				}
			}
			break;
		// warp unit
		case NetChange::WARP:
			{
				int netid;
				char in_building;
				VEC3 pos;
				float rot;

				if(!stream.Read(netid)
					|| !stream.Read(in_building)
					|| !ReadStruct(stream, pos)
					|| !stream.Read(rot))
				{
					ERROR("Update client: Broken WARP.");
					StreamEnd(false);
					break;
				}
				
				Unit* unit = FindUnit(netid);
				if(!unit)
				{
					ERROR(Format("Update client: WARP, missing unit %d.", netid));
					StreamEnd(false);
					break;
				}

				int old_in_building = unit->in_building;
				unit->in_building = in_building;
				unit->pos = pos;
				unit->rot = rot;

				unit->visual_pos = unit->pos;
				if(unit->interp)
					unit->interp->Reset(unit->pos, unit->rot);

				if(old_in_building != unit->in_building)
				{
					RemoveElement(GetContextFromInBuilding(old_in_building).units, unit);
					GetContextFromInBuilding(unit->in_building).units->push_back(unit);
				}
				if(unit == pc->unit)
				{
					if(fallback_co == FALLBACK_WAIT_FOR_WARP)
						fallback_co = FALLBACK_NONE;
					else if(fallback_co == FALLBACK_ARENA)
					{
						pc->unit->frozen = 1;
						fallback_co = FALLBACK_NONE;
					}
					else if(fallback_co == FALLBACK_ARENA_EXIT)
					{
						pc->unit->frozen = 0;
						fallback_co = FALLBACK_NONE;

						if(pc->unit->hp <= 0.f)
						{
							pc->unit->HealPoison();
							pc->unit->live_state = Unit::ALIVE;
							pc->unit->ani->Play("wstaje2", PLAY_ONCE|PLAY_PRIO3, 0);
							pc->unit->action = A_ANIMATION;
						}
					}
					PushNetChange(NetChange::WARP);
					interpolate_timer = 0.f;
					player_rot_buf = 0.f;
					cam.Reset();
					player_rot_buf = 0.f;
				}
			}
			break;
		// register new item
		case NetChange::REGISTER_ITEM:
			if(!ReadString1(stream))
			{
				ERROR("Update client: Broken REGISTER_ITEM.");
				StreamEnd(false);
			}
			else
			{
				const Item* base;
				if(BUF[0] == '$')
					base = FindItem(BUF + 1);
				else
					base = FindItem(BUF);
				if(!base)
				{
					ERROR(Format("Update client: REGISTER_ITEM, missing base item %s.", BUF));
					StreamEnd(false);
					if(!SkipString1(stream)
						|| !SkipString1(stream)
						|| !Skip(stream, sizeof(int)))
					{
						ERROR("Update client: Broken REGISTER_ITEM(2).");
					}
				}
				else
				{
					Item* item = CreateItemCopy(base);
					if(!ReadString1(stream, item->name)
						|| !ReadString1(stream, item->desc)
						|| !stream.Read(item->refid))
					{
						ERROR("Update client: Broken REGISTER_ITEM(3).");
						StreamEnd(false);
					}
					else
					{
						item->id = BUF;
						quest_items.push_back(item);
					}
				}
			}
			break;
		// added (main) quest
		case NetChange::ADD_QUEST:
		case NetChange::ADD_QUEST_MAIN:
			{
				cstring name = (type == NetChange::ADD_QUEST ? "ADD_QUEST" : "ADD_QUEST_MAIN");

				int refid;
				if(!stream.Read(refid)
					|| !ReadString1(stream))
				{
					ERROR(Format("Update client: Broken %s.", name));
					StreamEnd(false);
					break;
				}

				PlaceholderQuest* quest = new PlaceholderQuest;
				quest->quest_index = quests.size();
				quest->name = BUF;
				quest->msgs.resize(2);

				if(!ReadString1(stream, quest->msgs[0])
					|| !ReadString1(stream, quest->msgs[1]))
				{
					ERROR(Format("Update client: Broken %s(2).", name));
					StreamEnd(false);
					delete quest;
					break;
				}

				quest->state = Quest::Started;
				game_gui->journal->NeedUpdate(Journal::Quests, quest->quest_index);
				if(type == NetChange::ADD_QUEST)
					AddGameMsg3(GMS_JOURNAL_UPDATED);
				else
					GUI.SimpleDialog(txQuest[270], NULL);
				quests.push_back(quest);
			}
			break;
		// update quest
		case NetChange::UPDATE_QUEST:
			{
				int refid;
				byte state;
				if(!stream.Read(refid)
					|| !stream.Read(state)
					|| !ReadString1(stream))
				{
					ERROR("Update client: Broken UPDATE_QUEST.");
					StreamEnd(false);
					break;
				}

				Quest* quest = FindQuest(refid);
				if(!quest)
				{
					ERROR(Format("Update client: UPDATE_QUEST, missing quest %d.", refid));
					StreamEnd(false);
				}
				else
				{
					quest->state = (Quest::State)state;
					quest->msgs.push_back(BUF);
					game_gui->journal->NeedUpdate(Journal::Quests, quest->quest_index);
					AddGameMsg3(GMS_JOURNAL_UPDATED);
				}
			}
			break;
		// item rename
		case NetChange::RENAME_ITEM:
			{
				int refid;
				if(!stream.Read(refid)
					|| !ReadString1(stream))
				{
					ERROR("Update client: Broken RENAME_ITEM.");
					StreamEnd(false);
				}
				else
				{
					bool found = false;
					for(Item* item : quest_items)
					{
						if(item->refid == refid && item->id == BUF)
						{
							if(!ReadString1(stream, item->name))
							{
								ERROR("Update client: Broken RENAME_ITEM(2).");
								StreamEnd(false);
							}
							found = true;
							break;
						}
					}
					if(!found)
					{
						ERROR(Format("Update client: RENAME_ITEM, missing quest item %d.", refid));
						StreamEnd(false);
						SkipString1(stream);
					}
				}
			}
			break;
		// update quest with multiple texts
		case NetChange::UPDATE_QUEST_MULTI:
			{
				int refid;
				byte state, count;
				if(!stream.Read(refid)
					|| !stream.Read(state)
					|| !stream.Read(count))
				{
					ERROR("Update client: Broken UPDATE_QUEST_MULTI.");
					StreamEnd(false);
					break;
				}

				Quest* quest = FindQuest(refid);
				if(!quest)
				{
					ERROR(Format("Update client: UPDATE_QUEST_MULTI, missing quest %d.", refid));
					StreamEnd(false);
					if(!SkipStringArray<byte, byte>(stream))
						ERROR("Update client: Broken UPDATE_QUEST_MULTI(2).");
				}
				else
				{
					quest->state = (Quest::State)state;
					for(byte i = 0; i<count; ++i)
					{
						if(!ReadString1(stream, Add1(quest->msgs)))
						{
							ERROR(Format("Update client: Broken UPDATE_QUEST_MULTI(3) on index %u.", i));
							StreamEnd(false);
							quest->msgs.pop_back();
							break;
						}
					}
					game_gui->journal->NeedUpdate(Journal::Quests, quest->quest_index);
					AddGameMsg3(GMS_JOURNAL_UPDATED);
				}
			}
			break;
		// change leader notification
		case NetChange::CHANGE_LEADER:
			{
				byte id;
				if(!stream.Read(id))
				{
					ERROR("Update client: Broken CHANGE_LEADER.");
					StreamEnd(false);
				}
				else
				{
					PlayerInfo* info = GetPlayerInfoTry(id);
					if(info && !info->left)
					{
						leader_id = id;
						if(leader_id == my_id)
							AddMsg(txYouAreLeader);
						else
							AddMsg(Format(txPcIsLeader, info->name.c_str()));
						leader = info->u;

						if(dialog_enc)
							dialog_enc->bts[0].state = (IsLeader() ? Button::NONE : Button::DISABLED);

						ActivateChangeLeaderButton(IsLeader());
					}
					else
					{
						ERROR(Format("Update client: CHANGE_LEADER, missing player %u.", id));
						StreamEnd(false);
					}
				}
			}
			break;
		// player get random number
		case NetChange::RANDOM_NUMBER:
			{
				byte player_id, number;
				if(!stream.Read(player_id)
					|| !stream.Read(number))
				{
					ERROR("Update client: Broken RANDOM_NUMBER.");
					StreamEnd(false);
				}
				else if(player_id != my_id)
				{
					PlayerInfo* info = GetPlayerInfoTry(player_id);
					if(info)
						AddMsg(Format(txRolledNumber, info->name.c_str(), number));
					else
					{
						ERROR(Format("Update client: RANDOM_NUMBER, missing player %u.", player_id));
						StreamEnd(false);
					}
				}
			}
			break;
			// usuwanie gracza
		case NetChange::REMOVE_PLAYER:
			{
				byte player_id, reason;
				if(!stream.Read(player_id)
					|| !stream.Read(reason))
				{
					ERROR("Update client: Broken REMOVE_PLAYER.");
					StreamEnd(false);
				}
				else
				{
					PlayerInfo* info = GetPlayerInfoTry(player_id);
					if(!info)
					{
						ERROR(Format("Update client: REMOVE_PLAYER, missing player %u.", player_id));
						StreamEnd(false);
					}
					else
					{
						info->left = true;
						AddMsg(Format("%s %s.", info->name.c_str(), reason == 1 ? txPcWasKicked : txPcLeftGame));

						if(info->u)
						{
							if(info->u == before_player_ptr.unit)
								before_player = BP_NONE;
							RemoveElement(team, info->u);
							RemoveElement(active_team, info->u);

							if(reason == PlayerInfo::LEFT_LOADING)
							{
								if(info->u->interp)
									interpolators.Free(info->u->interp);
								if(info->u->cobj)
									delete info->u->cobj->getCollisionShape();
								delete info->u->ani;
								delete info->u;
								info->u = NULL;
							}
							else
							{
								info->u->to_remove = true;
								to_remove.push_back(info->u);

								if(info->u->useable)
									info->u->useable->user = NULL;
							}
						}
					}
				}
			}
			break;
		// unit uses useable object
		case NetChange::USE_USEABLE:
			{
				int netid, useable_netid;
				byte state;
				if(!stream.Read(netid)
					|| !stream.Read(useable_netid)
					|| !stream.Read(state))
				{
					ERROR("Update client: Broken USE_USEABLE.");
					StreamEnd(false);
					break;
				}

				Unit* unit = FindUnit(netid);
				if(!unit)
				{
					ERROR(Format("Update client: USE_USEABLE, missing unit %d.", netid));
					StreamEnd(false);
					break;
				}

				Useable* useable = FindUseable(useable_netid);
				if(!useable)
				{
					ERROR(Format("Update client: USE_USEABLE, missing useable %d.", useable_netid));
					StreamEnd(false);
					break;
				}
				
				if(type == 1 || type == 2)
				{
					BaseUsable& base = g_base_usables[useable->type];

					unit->action = A_ANIMATION2;
					unit->animation = ANI_PLAY;
					unit->ani->Play(state == 2 ? "czyta_papiery" : base.anim, PLAY_PRIO1, 0);
					unit->useable = useable;
					unit->target_pos = unit->pos;
					unit->target_pos2 = useable->pos;
					if(g_base_usables[useable->type].limit_rot == 4)
						unit->target_pos2 -= VEC3(sin(useable->rot)*1.5f, 0, cos(useable->rot)*1.5f);
					unit->timer = 0.f;
					unit->animation_state = AS_ANIMATION2_MOVE_TO_OBJECT;
					unit->use_rot = lookat_angle(unit->pos, unit->useable->pos);
					unit->used_item = base.item;
					if(unit->used_item)
					{
						unit->weapon_taken = W_NONE;
						unit->weapon_state = WS_HIDDEN;
					}
					useable->user = unit;

					if(before_player == BP_USEABLE && before_player_ptr.useable == useable)
						before_player = BP_NONE;
				}
				else if(unit->player != pc)
				{
					if(state == 0)
						useable->user = NULL;
					unit->action = A_NONE;
					unit->animation = ANI_STAND;
					if(unit->live_state == Unit::ALIVE)
						unit->used_item = NULL;
				}
			}
			break;
		// unit stands up
		case NetChange::STAND_UP:
			{
				int netid;
				if(!stream.Read(netid))
				{
					ERROR("Update client: Broken STAND_UP.");
					StreamEnd(false);
				}
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
					{
						ERROR(Format("Update client: STAND_UP, missing unit %d.", netid));
						StreamEnd(false);
					}
					else
						UnitStandup(*unit);
				}
			}
			break;
		// game over
		case NetChange::GAME_OVER:
			LOG("Update client: Game over - all players died.");
			SetMusic(MUSIC_CRYPT);
			CloseAllPanels();
			++death_screen;
			death_fade = 0;
			death_solo = false;
			exit_from_server = true;
			break;
		// recruit npc to team
		case NetChange::RECRUIT_NPC:
			{
				int netid;
				bool free;
				if(!stream.Read(netid)
					|| !ReadBool(stream, free))
				{
					ERROR("Update client: Broken RECRUIT_NPC.");
					StreamEnd(false);
				}
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
					{
						ERROR(Format("Update client: RECRUIT_NPC, missing unit %d.", netid));
						StreamEnd(false);
					}
					else if(!unit->IsHero())
					{
						ERROR(Format("Update client: RECRUIT_NPC, unit %d (%s) is not a hero.", netid, unit->data->id.c_str()));
						StreamEnd(false);
					}
					else
					{
						unit->hero->team_member = true;
						unit->hero->free = free;
						unit->hero->credit = 0;
						team.push_back(unit);
						if(!free)
							active_team.push_back(unit);
						if(game_gui->team_panel->visible)
							game_gui->team_panel->Changed();
					}
				}
			}
			break;
		// kick npc out of team
		case NetChange::KICK_NPC:
			{
				int netid;
				if(!stream.Read(netid))
				{
					ERROR("Update client: Broken KICK_NPC.");
					StreamEnd(false);
				}
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
					{
						ERROR(Format("Update client: KICK_NPC, missing unit %d.", netid));
						StreamEnd(false);
					}
					else if(!unit->IsHero() || !unit->hero->team_member)
					{
						ERROR(Format("Update client: KICK_NPC, unit %d (%s) is not a team member.", netid, unit->data->id.c_str()));
						StreamEnd(false);
					}
					else
					{
						unit->hero->team_member = false;
						RemoveElement(team, unit);
						if(!unit->hero->free)
							RemoveElement(active_team, unit);
						if(game_gui->team_panel->visible)
							game_gui->team_panel->Changed();
					}
				}
			}
			break;
			// usuwanie jednostki
		case NetChange::REMOVE_UNIT:
			{
				int netid;
				if(!stream.Read(netid))
				{
					ERROR("Update client: Broken REMOVE_UNIT.");
					StreamEnd(false);
				}
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
					{
						ERROR(Format("Update client: REMOVE_UNIT, missing unit %d.", netid));
						StreamEnd(false);
					}
					else
					{
						unit->to_remove = true;
						to_remove.push_back(unit);
					}
				}
			}
			break;
		// spawn new unit
		case NetChange::SPAWN_UNIT:
			{
				Unit* unit = new Unit;
				if(!ReadUnit(stream, *unit))
				{
					ERROR("Update client: Broken SPAWN_UNIT.");
					StreamEnd(false);
					delete unit;
				}
				else
				{
					LevelContext& ctx = GetContext(unit->pos);
					ctx.units->push_back(unit);
					unit->in_building = ctx.building_id;
				}
			}
			break;
		// change unit arena state
		case NetChange::CHANGE_ARENA_STATE:
			{
				int netid;
				char state;
				if(!stream.Read(netid)
					|| !stream.Read(state))
				{
					ERROR("Update client: Broken CHANGE_ARENA_STATE.");
					StreamEnd(false);
				}
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
					{
						ERROR(Format("Update client: CHANGE_ARENA_STATE, missing unit %d.", netid));
						StreamEnd(false);
					}
					else
					{
						if(state < -1 || state > 1)
							state = -1;
						unit->in_arena = state;
					}
				}
			}
			break;
		// plays arena sound
		case NetChange::ARENA_SOUND:
			{
				byte type;
				if(!stream.Read(type))
				{
					ERROR("Update client: Broken ARENA_SOUND.");
					StreamEnd(false);
				}
				else if(sound_volume && city_ctx && city_ctx->type == L_CITY && GetArena()->ctx.building_id == pc->unit->in_building)
				{
					SOUND snd;
					if(type == 0)
						snd = sArenaFight;
					else if(type == 1)
						snd = sArenaWygrana;
					else
						snd = sArenaPrzegrana;
					PlaySound2d(snd);
				}
			}
			break;
		// unit shout after seeing enemy
		case NetChange::SHOUT:
			{
				int netid;
				char state;
				if(!stream.Read(netid)
					|| !stream.Read(state))
				{
					ERROR("Update client: Broken CHANGE_ARENA_STATE.");
					StreamEnd(false);
				}
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
					{
						ERROR(Format("Update client: CHANGE_ARENA_STATE, missing unit %d.", netid));
						StreamEnd(false);
					}
					else if(sound_volume)
						PlayAttachedSound(*unit, unit->data->sounds->sound[SOUND_SEE_ENEMY], 3.f, 20.f);
				}
			}
		// leaving notification
		case NetChange::LEAVE_LOCATION:
			fallback_co = FALLBACK_WAIT_FOR_WARP;
			fallback_t = -1.f;
			pc->unit->frozen = 2;
			break;
		// exit to map
		case NetChange::EXIT_TO_MAP:
			ExitToMap();
			break;
			// podró¿ do innej lokacji
		case NetChange::TRAVEL:
			{
				byte loc;
				if(s.Read(loc))
				{
					world_state = WS_TRAVEL;
					current_location = -1;
					travel_time = 0.f;
					travel_day = 0;
					travel_start = world_pos;
					picked_location = loc;
					Location& l = *locations[picked_location];
					world_dir = angle(world_pos.x, -world_pos.y, l.pos.x, -l.pos.y);
					travel_time2 = 0.f;

					// opuœæ aktualn¹ lokalizacje
					if(open_location != -1)
					{
						LeaveLocation();
						open_location = -1;
					}
				}
				else
					READ_ERROR("TRAVEL");
			}
			break;
			// zmiana daty w grze
		case NetChange::WORLD_TIME:
			{
				int czas;
				byte dzien, miesiac, rok;
				if(s.Read(czas) && s.Read(dzien) && s.Read(miesiac) && s.Read(rok))
				{
					worldtime = czas;
					day = dzien;
					month = miesiac;
					year = rok;
				}
				else
					READ_ERROR("WORLD_TIME");
			}
			break;
			// ktoœ otwiera/zamyka drzwi
		case NetChange::USE_DOOR:
			{
				int netid;
				byte state;
				if(s.Read(netid) && s.Read(state))
				{
					Door* door = FindDoor(netid);
					if(door)
					{
						bool ok = true;

						if(state == 1)
						{
							// zamykanie
							if(door->state == Door::Open)
							{
								door->state = Door::Closing;
								door->ani->Play(&door->ani->ani->anims[0], PLAY_ONCE|PLAY_STOP_AT_END|PLAY_NO_BLEND|PLAY_BACK, 0);
								door->ani->frame_end_info = false;
							}
							else if(door->state == Door::Opening)
							{
								door->state = Door::Closing2;
								door->ani->Play(&door->ani->ani->anims[0], PLAY_ONCE|PLAY_STOP_AT_END|PLAY_BACK, 0);
								door->ani->frame_end_info = false;
							}
							else if(door->state == Door::Opening2)
							{
								door->state = Door::Closing;
								door->ani->Play(&door->ani->ani->anims[0], PLAY_ONCE|PLAY_STOP_AT_END|PLAY_BACK, 0);
								door->ani->frame_end_info = false;
							}
							else
								ok = false;
						}
						else
						{
							// otwieranie
							if(door->state == Door::Closed)
							{
								door->locked = LOCK_NONE;
								door->state = Door::Opening;
								door->ani->Play(&door->ani->ani->anims[0], PLAY_ONCE|PLAY_STOP_AT_END|PLAY_NO_BLEND, 0);
								door->ani->frame_end_info = false;
							}
							else if(door->state == Door::Closing)
							{
								door->locked = LOCK_NONE;
								door->state = Door::Opening2;
								door->ani->Play(&door->ani->ani->anims[0], PLAY_ONCE|PLAY_STOP_AT_END, 0);
								door->ani->frame_end_info = false;
							}
							else if(door->state == Door::Closing2)
							{
								door->locked = LOCK_NONE;
								door->state = Door::Opening;
								door->ani->Play(&door->ani->ani->anims[0], PLAY_ONCE|PLAY_STOP_AT_END, 0);
								door->ani->frame_end_info = false;
							}
							else
								ok = false;
						}

						if(ok && sound_volume && rand2() == 0)
						{
							SOUND snd;
							if(state == 1 && rand2()%2 == 0)
								snd = sDoorClose;
							else
								snd = sDoor[rand2()%3];
							VEC3 pos = door->pos;
							pos.y += 1.5f;
							PlaySound3d(snd, pos, 2.f, 5.f);
						}
					}
					else
						WARN(Format("USE_DOOR, missing unit %d.", netid));
				}
				else
					READ_ERROR("USE_DOOR");
			}
			break;
			// otwarcie skrzyni
		case NetChange::CHEST_OPEN:
			{
				int netid;
				if(s.Read(netid))
				{
					Chest* chest = FindChest(netid);
					if(chest)
					{
						chest->ani->Play(&chest->ani->ani->anims[0], PLAY_PRIO1|PLAY_ONCE|PLAY_STOP_AT_END, 0);
						if(sound_volume)
						{
							VEC3 pos = chest->pos;
							pos.y += 0.5f;
							PlaySound3d(sChestOpen, pos, 2.f, 5.f);
						}
					}
				}
				else
					READ_ERROR("OPEN_CHEST");
			}
			break;
			// zamkniêcie skrzyni
		case NetChange::CHEST_CLOSE:
			{
				int netid;
				if(s.Read(netid))
				{
					Chest* chest = FindChest(netid);
					if(chest)
					{
						chest->ani->Play(&chest->ani->ani->anims[0], PLAY_PRIO1|PLAY_ONCE|PLAY_STOP_AT_END|PLAY_BACK, 0);
						if(sound_volume)
						{
							VEC3 pos = chest->pos;
							pos.y += 0.5f;
							PlaySound3d(sChestClose, pos, 2.f, 5.f);
						}
					}
				}
				else
					READ_ERROR("OPEN_CHEST");
			}
			break;
			// efekt eksplozji
		case NetChange::CREATE_EXPLOSION:
			{
				byte id;
				VEC3 pos;
				if(s.Read(id) && ReadStruct(s, pos))
				{
					if(id >= n_spells || !IS_SET(g_spells[id].flags, Spell::Explode))
						WARN(Format("CREATE_EXPLOSION, spell %d is not explosion.", id));
					else
					{
						Spell& fireball = g_spells[id];

						Explo* explo = new Explo;
						explo->pos = pos;
						explo->size = 0.f;
						explo->sizemax = 2.f;
						explo->tex = fireball.tex_explode;
						explo->owner = NULL;

						if(sound_volume)
							PlaySound3d(fireball.sound_hit, explo->pos, fireball.sound_hit_dist.x, fireball.sound_hit_dist.y);

						GetContext(pos).explos->push_back(explo);
					}
				}
				else
					READ_ERROR("CREATE_EXPLOSION");
			}
			break;
			// usuniêcie pu³apki
		case NetChange::REMOVE_TRAP:
			{
				int netid;
				if(s.Read(netid))
				{
					if(!RemoveTrap(netid))
						WARN(Format("REMOVE_TRAP, missing trap %d.", netid));
				}
				else
					READ_ERROR("REMOVE_TRAP");
			}
			break;
			// uruchomienie pu³apki
		case NetChange::TRIGGER_TRAP:
			{
				int netid;
				if(s.Read(netid))
				{
					Trap* trap = FindTrap(netid);
					if(trap)
						trap->trigger = true;
					else
						WARN(Format("TRIGGER_TRAP, missing trap %d.", netid));
				}
				else
					READ_ERROR("TRIGGER_TRAP");
			}
			break;
			// dŸwiêk w zadaniu z³o
		case NetChange::EVIL_SOUND:
			if(sound_volume)
				PlaySound2d(sEvil);
			break;
			// spotkanie na mapie œwiata
		case NetChange::ENCOUNTER:
			{
				DialogInfo info;
				info.event = DialogEvent(this, &Game::Event_StartEncounter);
				info.name = "encounter";
				info.order = ORDER_TOP;
				info.parent = NULL;
				info.pause = true;
				info.type = DIALOG_OK;

				if(ReadString1(s, info.text))
				{
					dialog_enc = GUI.ShowDialog(info);

					if(!IsLeader())
						dialog_enc->bts[0].state = Button::DISABLED;

					world_state = WS_ENCOUNTER;
				}
				else
					READ_ERROR("ENCOUNTER");
			}
			break;
			// rozpoczyna spotkanie
		case NetChange::CLOSE_ENCOUNTER:
			if(dialog_enc)
			{
				GUI.CloseDialog(dialog_enc);
				delete dialog_enc;
				dialog_enc = NULL;
			}
			world_state = WS_TRAVEL;
			break;
			// zamykanie portalu
		case NetChange::CLOSE_PORTAL:
			if(location->portal)
			{
				delete location->portal;
				location->portal = NULL;
			}
			else
				WARN("CLOSE_PORTAL, missing portal.");
			break;
			// czyszczenie o³tarza w queœcie z³o
		case NetChange::CLEAN_ALTAR:
			{
				// zmieñ obiekt
				Obj* o = FindObject("bloody_altar");
				int index = 0;
				for(vector<Object>::iterator it = local_ctx.objects->begin(), end = local_ctx.objects->end(); it != end; ++it, ++index)
				{
					if(it->base == o)
						break;
				}
				Object& obj = local_ctx.objects->at(index);
				obj.base = FindObject("altar");
				obj.mesh = obj.base->ani;

				// usuñ cz¹steczki
				float best_dist = 999.f;
				ParticleEmitter* pe = NULL;
				for(vector<ParticleEmitter*>::iterator it = local_ctx.pes->begin(), end = local_ctx.pes->end(); it != end; ++it)
				{
					if((*it)->tex == tKrew[BLOOD_RED])
					{
						float dist = distance((*it)->pos, obj.pos);
						if(dist < best_dist)
						{
							best_dist = dist;
							pe = *it;
						}
					}
				}
				assert(pe);
				pe->destroy = true;
			}
			break;
			// dodano now¹ lokacjê
		case NetChange::ADD_LOCATION:
			{
				byte id;
				LOCATION type;
				if(s.ReadCasted<byte>(id) && s.ReadCasted<byte>(type))
				{
					Location* loc;
					if(type == L_DUNGEON || type == L_CRYPT)
					{
						byte ile;
						if(!s.Read(ile))
						{
							READ_ERROR("ADD_LOCATION(2)");
							break;
						}
						if(ile == 1)
							loc = new SingleInsideLocation;
						else
							loc = new MultiInsideLocation(ile);
					}
					else if(type == L_CAVE)
						loc = new CaveLocation;
					else
						loc = new OutsideLocation;
					loc->type = type;

					if(s.ReadCasted<byte>(loc->state) &&
						s.Read(loc->pos.x) &&
						s.Read(loc->pos.y) &&
						ReadString1(s, loc->name))
					{
						if(id >= locations.size())
							locations.resize(id+1, NULL);
						locations[id] = loc;
					}
					else
						READ_ERROR("ADD_LOCATION(3)");
				}
				else
					READ_ERROR("ADD_LOCAION");
			}
			break;
			// usuniêto obóz
		case NetChange::REMOVE_CAMP:
			{
				byte id;
				if(s.Read(id))
				{
					delete locations[id];
					if(id == locations.size()-1)
						locations.pop_back();
					else
						locations[id] = NULL;
				}
				else
					READ_ERROR("REMOVE_CAMP");
			}
			break;
			// zmiana trybu ai
		case NetChange::CHANGE_AI_MODE:
			{
				int netid;
				byte mode;
				if(s.Read(netid) && s.Read(mode))
				{
					Unit* u = FindUnit(netid);
					if(u)
						u->ai_mode = mode;
					else
						WARN(Format("CHANGE_AI_MODE, missing unit %d.", netid));
				}
				else
					READ_ERROR("CHANGE_AI_MODE");
			}
			break;
			// zmiana bazowego typu jednostki
		case NetChange::CHANGE_UNIT_BASE:
			{
				int netid;
				if(s.Read(netid) && ReadString1(net_stream))
				{
					Unit* u = FindUnit(netid);
					if(u)
					{
						UnitData* ud = FindUnitData(BUF, false);
						if(ud)
							u->data = ud;
						else
							WARN(Format("CHANGE_UNIT_BASE, missing base unit '%s'.", BUF));
					}
					else
						WARN(Format("CHANGE_UNIT_BASE, missing unit %d.", netid));
				}
				else
					READ_ERROR("CHANGE_UNIT_BASE");
			}
			break;
			// jednostka zaczyna rzucaæ czar
		case NetChange::CAST_SPELL:
			{
				int netid;
				if(s.Read(netid))
				{
					Unit* u = FindUnit(netid);
					if(u)
					{
						u->action = A_CAST;
						u->attack_id = i;
						u->animation_state = 0;

						if(u->ani->ani->head.n_groups == 2)
						{
							u->ani->frame_end_info2 = false;
							u->ani->Play("cast", PLAY_ONCE|PLAY_PRIO1, 1);
						}
						else
						{
							u->ani->frame_end_info = false;
							u->animation = ANI_PLAY;
							u->ani->Play("cast", PLAY_ONCE|PLAY_PRIO1, 0);
						}
					}
					else
						WARN(Format("CAST_SPELL, missing unit %d.", netid));
				}
				else
					READ_ERROR("CAST_SPELL");
			}
			break;
			// efekt rzucenia czaru - pocisk
		case NetChange::CREATE_SPELL_BALL:
			{
				int netid;
				VEC3 pos;
				float rotY, speedY;
				byte type;

				if(s.Read(netid) &&
					s.Read((char*)&pos, sizeof(pos)) &&
					s.Read(rotY) &&
					s.Read(speedY) &&
					s.Read(type))
				{
					Unit* u = FindUnit(netid);
					if(!u && netid != -1)
						WARN(Format("CREATE_SPELL_BALL, missing unit %d.", netid));

					Spell& spell = g_spells[type];
					LevelContext& ctx = GetContext(pos);

					Bullet& b = Add1(ctx.bullets);

					b.pos = pos;
					b.rot = VEC3(0, rotY, 0);
					b.mesh = spell.mesh;
					b.tex = spell.tex;
					b.tex_size = spell.size;
					b.speed = spell.speed;
					b.timer = spell.range/(spell.speed-1);
					b.remove = false;
					b.trail = NULL;
					b.trail2 = NULL;
					b.pe = NULL;
					b.spell = &spell;
					b.start_pos = b.pos;
					b.owner = u;
					b.yspeed = speedY;

					if(spell.tex_particle)
					{
						ParticleEmitter* pe = new ParticleEmitter;
						pe->tex = spell.tex_particle;
						pe->emision_interval = 0.1f;
						pe->life = -1;
						pe->particle_life = 0.5f;
						pe->emisions = -1;
						pe->spawn_min = 3;
						pe->spawn_max = 4;
						pe->max_particles = 50;
						pe->pos = b.pos;
						pe->speed_min = VEC3(-1, -1, -1);
						pe->speed_max = VEC3(1, 1, 1);
						pe->pos_min = VEC3(-spell.size, -spell.size, -spell.size);
						pe->pos_max = VEC3(spell.size, spell.size, spell.size);
						pe->size = spell.size_particle;
						pe->op_size = POP_LINEAR_SHRINK;
						pe->alpha = 1.f;
						pe->op_alpha = POP_LINEAR_SHRINK;
						pe->mode = 1;
						pe->Init();
						ctx.pes->push_back(pe);
						b.pe = pe;
					}
				}
				else
					READ_ERROR("CREATE_SPELL_BALL");
			}
			break;
			// dŸwiêk rzucania czaru
		case NetChange::SPELL_SOUND:
			{
				byte type;
				VEC3 pos;
				if(s.Read(type) && s.Read((char*)&pos, sizeof(pos)))
				{
					if(sound_volume)
					{
						Spell& spell = g_spells[type];
						PlaySound3d(spell.sound_cast, pos, spell.sound_cast_dist.x, spell.sound_cast_dist.y);
					}
				}
				else
					READ_ERROR("SPELL_SOUND");
			}
			break;
			// efekt wyssania krwii
		case NetChange::CREATE_DRAIN:
			{
				int netid;
				if(s.Read(netid))
				{
					Unit* u = FindUnit(netid);
					if(u)
					{
						LevelContext& ctx = GetContext(*u);
						Drain& drain = Add1(ctx.drains);
						drain.from = NULL;
						drain.to = u;
						drain.pe = ctx.pes->back();
						drain.t = 0.f;
						drain.pe->manual_delete = 1;
						drain.pe->speed_min = VEC3(-3, 0, -3);
						drain.pe->speed_max = VEC3(3, 3, 3);
					}
					else
						WARN(Format("CREATE_DRAIN, missing unit %d.", netid));
				}
				else
					READ_ERROR("CREATE_DRAIN");
			}
			break;
			// efekt czaru piorun
		case NetChange::CREATE_ELECTRO:
			{
				int netid;
				VEC3 p1, p2;
				if(s.Read(netid) && s.Read((char*)&p1, sizeof(p1)) && s.Read((char*)&p2, sizeof(p2)))
				{
					Electro* e = new Electro;
					e->spell = FindSpell("thunder_bolt");
					e->start_pos = p1;
					e->netid = netid;
					e->AddLine(p1, p2);
					e->valid = true;
					GetContext(p1).electros->push_back(e);
				}
				else
					READ_ERROR("CREATE_ELECTRO");
			}
			break;
			// dodaje kolejny kawa³ek elektro
		case NetChange::UPDATE_ELECTRO:
			{
				int netid;
				VEC3 pos;
				if(s.Read(netid) && s.Read((char*)&pos, sizeof(pos)))
				{
					Electro* e = FindElectro(netid);
					if(e)
					{
						VEC3 from = e->lines.back().pts.back();
						e->AddLine(from, pos);
					}
					else
						WARN(Format("UPDATE_ELECTRO, missing electro %d.", netid));
				}
				else
					READ_ERROR("UPDATE_ELECTRO");
			}
			break;
			// efekt trafienia przez elektro
		case NetChange::ELECTRO_HIT:
			{
				VEC3 pos;
				if(s.Read((char*)&pos, sizeof(pos)))
				{
					Spell* spell = FindSpell("thunder_bolt");

					// dŸwiêk
					if(sound_volume && spell->sound_hit)
						PlaySound3d(spell->sound_hit, pos, spell->sound_hit_dist.x, spell->sound_hit_dist.y);

					// cz¹steczki
					if(spell->tex_particle)
					{
						ParticleEmitter* pe = new ParticleEmitter;
						pe->tex = spell->tex_particle;
						pe->emision_interval = 0.01f;
						pe->life = 0.f;
						pe->particle_life = 0.5f;
						pe->emisions = 1;
						pe->spawn_min = 8;
						pe->spawn_max = 12;
						pe->max_particles = 12;
						pe->pos = pos;
						pe->speed_min = VEC3(-1.5f, -1.5f, -1.5f);
						pe->speed_max = VEC3(1.5f, 1.5f, 1.5f);
						pe->pos_min = VEC3(-spell->size, -spell->size, -spell->size);
						pe->pos_max = VEC3(spell->size, spell->size, spell->size);
						pe->size = spell->size_particle;
						pe->op_size = POP_LINEAR_SHRINK;
						pe->alpha = 1.f;
						pe->op_alpha = POP_LINEAR_SHRINK;
						pe->mode = 1;
						pe->Init();

						GetContext(pos).pes->push_back(pe);
					}
				}
				else
					READ_ERROR("ELECTRO_HIT");
			}
			break;
			// efekt o¿ywiania
		case NetChange::RAISE_EFFECT:
			{
				VEC3 pos;
				if(s.Read((char*)&pos, sizeof(pos)))
				{
					Spell& spell = *FindSpell("raise");

					ParticleEmitter* pe = new ParticleEmitter;
					pe->tex = spell.tex_particle;
					pe->emision_interval = 0.01f;
					pe->life = 0.f;
					pe->particle_life = 0.5f;
					pe->emisions = 1;
					pe->spawn_min = 16;
					pe->spawn_max = 25;
					pe->max_particles = 25;
					pe->pos = pos;
					pe->speed_min = VEC3(-1.5f, -1.5f, -1.5f);
					pe->speed_max = VEC3(1.5f, 1.5f, 1.5f);
					pe->pos_min = VEC3(-spell.size, -spell.size, -spell.size);
					pe->pos_max = VEC3(spell.size, spell.size, spell.size);
					pe->size = spell.size_particle;
					pe->op_size = POP_LINEAR_SHRINK;
					pe->alpha = 1.f;
					pe->op_alpha = POP_LINEAR_SHRINK;
					pe->mode = 1;
					pe->Init();

					GetContext(pos).pes->push_back(pe);
				}
				else
					READ_ERROR("RAISE_EFFECT");
			}
			break;
			// efekt leczenia
		case NetChange::HEAL_EFFECT:
			{
				VEC3 pos;
				if(s.Read((char*)&pos, sizeof(pos)))
				{
					Spell& spell = *FindSpell("heal");

					ParticleEmitter* pe = new ParticleEmitter;
					pe->tex = spell.tex_particle;
					pe->emision_interval = 0.01f;
					pe->life = 0.f;
					pe->particle_life = 0.5f;
					pe->emisions = 1;
					pe->spawn_min = 16;
					pe->spawn_max = 25;
					pe->max_particles = 25;
					pe->pos = pos;
					pe->speed_min = VEC3(-1.5f, -1.5f, -1.5f);
					pe->speed_max = VEC3(1.5f, 1.5f, 1.5f);
					pe->pos_min = VEC3(-spell.size, -spell.size, -spell.size);
					pe->pos_max = VEC3(spell.size, spell.size, spell.size);
					pe->size = spell.size_particle;
					pe->op_size = POP_LINEAR_SHRINK;
					pe->alpha = 1.f;
					pe->op_alpha = POP_LINEAR_SHRINK;
					pe->mode = 1;
					pe->Init();

					GetContext(pos).pes->push_back(pe);
				}
				else
					READ_ERROR("HEAL_EFFECT");
			}
			break;
			// odkrywanie ca³ej minimapy dziêki kodowi 'show_minimap'
		case NetChange::CHEAT_SHOW_MINIMAP:
			Cheat_ShowMinimap();
			break;
			// odkrywanie kawa³ka minimapy
		case NetChange::REVEAL_MINIMAP:
			{
				word ile;
				if(s.Read(ile))
				{
					for(word i = 0; i<ile; ++i)
					{
						byte x, y;
						if(s.Read(x) && s.Read(y))
							minimap_reveal.push_back(INT2(x, y));
						else
							READ_ERROR("REVEAL_MINIMAP(2)");
					}
				}
				else
					READ_ERROR("REVEAL_MINIMAP");
			}
			break;
			// obs³uga kodu 'noai'
		case NetChange::CHEAT_NOAI:
			{
				byte co;
				if(s.Read(co))
					noai = (co == 1);
				else
					READ_ERROR("CHEAT_NOAI");
			}
			break;
			// koniec gry - czas przejœæ na emeryturê
		case NetChange::END_OF_GAME:
			LOG("Game over: you are too old.");
			koniec_gry = true;
			death_fade = 0.f;
			exit_from_server = true;
			break;
			// aktualizacja wolnych dni
		case NetChange::UPDATE_FREE_DAYS:
			{
				int netid;
				do
				{
					if(!s.Read(netid))
					{
						READ_ERROR("UPDATE_FREE_DAYS");
						break;
					}

					if(netid == -1)
						break;

					int ile;
					if(!s.Read(ile))
					{
						READ_ERROR("UPDATE_FREE_DAYS(2)");
						break;
					}

					Unit* u = FindUnit(netid);
					if(u && u->IsPlayer())
						u->player->free_days = ile;
					else if(u)
						WARN(Format("UPDATE_FREE_DAYS, %d is not a player.", netid));
					else
						WARN(Format("UPDATE_FREE_DAYS, missing unit %d.", netid));
				}
				while(1);
			}
			break;
			// zmiana zmiennych mp
		case NetChange::CHANGE_MP_VARS:
			if(!ReadNetVars(s))
				READ_ERROR("CHANGE_MP_VARS");
			break;
			// informacja o zapisaniu gry
		case NetChange::GAME_SAVED:
			AddMultiMsg("Gra zapisana.");
			break;
			// ai opuœci³ dru¿yne bo by³o za du¿o osób
		case NetChange::HERO_LEAVE:
			{
				int netid;
				if(s.Read(netid))
				{
					Unit* u = FindUnit(netid);
					if(u)
						AddMultiMsg(Format(txMpNPCLeft, u->GetName()));
					else
						WARN(Format("HERO_LEAVE, missing unit %d.", netid));
				}
				else
					READ_ERROR("HERO_LEAVE");
			}
			break;
			// gra zatrzymana/wznowiona
		case NetChange::PAUSED:
			{
				byte co;
				if(s.Read(co))
				{
					paused = (co == 1);
					AddMultiMsg(paused ? txGamePaused : txGameResumed);
				}
				else
					READ_ERROR("PAUSED");
			}
			break;
			// aktualizacja tekstu listu sekretu
		case NetChange::SECRET_TEXT:
			if(!ReadString1(s, GetSecretNote()->desc))
				READ_ERROR("SECRET_TEXT");
			break;
			// aktualizacja pozycji na mapie œwiata
		case NetChange::UPDATE_MAP_POS:
			if(!ReadStruct(s, world_pos))
				READ_ERROR("UPDATE_MAP_POS");
			break;
			// cheat na zmianê pozycji na mapie œwiata
		case NetChange::CHEAT_TRAVEL:
			{
				int id;
				if(s.Read(id))
				{
					current_location = id;
					Location& loc = *locations[current_location];
					if(loc.state == LS_KNOWN)
						loc.state = LS_VISITED;
					world_pos = loc.pos;
					if(open_location != -1)
					{
						LeaveLocation();
						open_location = -1;
					}
				}
				else
					READ_ERROR("CHEAT_TRAVEL");
			}
			break;
			// usuwa u¿ywany przedmiot z d³oni
		case NetChange::REMOVE_USED_ITEM:
			{
				int netid;
				if(s.Read(netid))
				{
					Unit* u = FindUnit(netid);
					if(u)
						u->used_item = NULL;
					else
						ERROR(Format("REMOVE_USED_ITEM, missing unit %d.", netid));
				}
				else
					READ_ERROR("REMOVE_USED_ITEM");
			}
			break;
			// statystyki gry
		case NetChange::GAME_STATS:
			if(!s.Read(total_kills))
				READ_ERROR("GAME_STATS");
			break;
			// dŸwiêk u¿ywania obiektu przez postaæ
		case NetChange::USEABLE_SOUND:
			{
				int netid;
				if(s.Read(netid))
				{
					Unit* u = FindUnit(netid);
					if(u)
					{
						if(sound_volume && u != pc->unit && u->useable)
							PlaySound3d(u->useable->GetBase()->sound, u->GetCenter(), 2.f, 5.f);
					}
					else
						ERROR(Format("USEABLE_SOUND, missing unit %d.", netid));
				}
				else
					READ_ERROR("USEABLE_SOUND");
			}
			break;
			// show text when trying to enter academy
		case NetChange::ACADEMY_TEXT:
			ShowAcademyText();
			break;
		default:
			WARN(Format("Unknown change type %d.", type));
			assert(0);
			break;
		}
	}

	return true;
}

//=================================================================================================
bool Game::ProcessControlMessageClientForMe(BitStream& stream)
{
	/*{
		byte flags;
		BitStream s(packet->data+1, packet->length-1, false);
		s.Read(flags);
		// zmiana obra¿eñ od trucizny
		if(IS_SET(flags, PlayerInfo::UF_POISON_DAMAGE))
			s.Read(pc->last_dmg_poison);
		// ró¿ne zmiany dotycz¹ce gracza
		if(IS_SET(flags, PlayerInfo::UF_NET_CHANGES))
		{
			byte ile;
			s.Read(ile);
			for(byte i = 0; i<ile; ++i)
			{
				NetChangePlayer::TYPE type;
				s.ReadCasted<byte>(type);
				switch(type)
				{
					// podnoszenie przedmiotu
				case NetChangePlayer::PICKUP:
					{
						int count, team_count;
						if(s.Read(count) && s.Read(team_count))
						{
							AddItem(*pc->unit, picking_item->item, (uint)count, (uint)team_count);
							if(picking_item->item->type == IT_GOLD && sound_volume)
								PlaySound2d(sMoneta);
							if(picking_item_state == 2)
								delete picking_item;
							picking_item_state = 0;
						}
						else
							READ_ERROR("PICKUP");
					}
					break;
					// rozpoczynanie ograbiania skrzyni/zw³ok
				case NetChangePlayer::LOOT:
					{
						byte co;
						if(!s.Read(co))
						{
							READ_ERROR("LOOT");
							break;
						}
						if(co == 0)
						{
							AddGameMsg3(GMS_IS_LOOTED);
							pc->action = PlayerController::Action_None;
							break;
						}

						// odczytaj przedmioty
						if(!ReadItemListTeam(s, *pc->chest_trade))
						{
							READ_ERROR("LOOT(2)");
							break;
						}

						// start trade
						if(pc->action == PlayerController::Action_LootUnit)
							StartTrade(I_LOOT_BODY, *pc->action_unit);
						else
							StartTrade(I_LOOT_CHEST, pc->action_chest->items);
					}
					break;
					// wiadomoœæ o otrzymanym z³ocie
				case NetChangePlayer::GOLD_MSG:
					{
						byte b;
						int ile;
						if(s.Read(b) && s.Read(ile))
						{
							if(b == 1)
								AddGameMsg(Format(txGoldPlus, ile), 3.f);
							else
								AddGameMsg(Format(txQuestCompletedGold, ile), 4.f);
						}
						else
							READ_ERROR("GOLD_MSG");
					}
					break;
					// gracz rozmawia z postaci¹ (lub próbuje)
				case NetChangePlayer::START_DIALOG:
					{
						int netid;
						if(s.Read(netid))
						{
							if(netid == -1)
							{
								pc->action = PlayerController::Action_None;
								AddGameMsg3(GMS_UNIT_BUSY);
							}
							else
							{
								Unit* u = FindUnit(netid);
								if(u)
								{
									pc->action = PlayerController::Action_Talk;
									pc->action_unit = u;
									StartDialog(dialog_context, u);
									if(!predialog.empty())
									{
										dialog_context.dialog_s_text = predialog;
										dialog_context.dialog_text = dialog_context.dialog_s_text.c_str();
										dialog_context.dialog_wait = 1.f;
										predialog.clear();
									}
									else if(u->bubble)
									{
										dialog_context.dialog_s_text = u->bubble->text;
										dialog_context.dialog_text = dialog_context.dialog_s_text.c_str();
										dialog_context.dialog_wait = 1.f;
										dialog_context.skip_id = u->bubble->skip_id;
									}
									before_player = BP_NONE;
								}
								else
									WARN(Format("START_DIALOG, missing unit %d.", netid));
							}
						}
						else
							READ_ERROR("START_DIALOG");
					}
					break;
					// dostêpne opcje dialogowe
				case NetChangePlayer::SHOW_DIALOG_CHOICES:
					{
						byte ile;
						char esc;
						if(s.Read(ile) && s.Read(esc))
						{
							dialog_context.choice_selected = 0;
							dialog_context.show_choices = true;
							dialog_context.dialog_esc = esc;
							dialog_choices.resize(ile);
							for(byte i = 0; i<ile; ++i)
							{
								if(!ReadString1(s, dialog_choices[i]))
								{
									READ_ERROR("SHOW_DIALOG_CHOICES(2)");
									break;
								}
							}
							game_gui->UpdateScrollbar(dialog_choices.size());
						}
						else
							READ_ERROR("SHOW_DIALOG_CHOICES");
					}
					break;
					// koniec dialogu
				case NetChangePlayer::END_DIALOG:
					dialog_context.dialog_mode = false;
					if(pc->action == PlayerController::Action_Talk)
						pc->action = PlayerController::Action_None;
					pc->unit->look_target = NULL;
					break;
					// rozpoczynanie wymiany
				case NetChangePlayer::START_TRADE:
					{
						int netid;
						if(!s.Read(netid))
						{
							READ_ERROR("START_TRADE");
							break;
						}

						Unit* t = FindUnit(netid);
						if(t)
						{
							if(!ReadItemList(s, chest_trade))
							{
								READ_ERROR("START_TRADE(2)");
								break;
							}

							const string& id = t->data->id;

							if(id == "blacksmith" || id == "q_orkowie_kowal")
								trader_buy = blacksmith_buy;
							else if(id == "merchant" || id == "tut_czlowiek")
								trader_buy = merchant_buy;
							else if(id == "alchemist")
								trader_buy = alchemist_buy;
							else if(id == "innkeeper")
								trader_buy = innkeeper_buy;
							else if(id == "food_seller")
								trader_buy = foodseller_buy;

							StartTrade(I_TRADE, chest_trade, t);
						}
						else
							ERROR(Format("START_TRADE: No unit with id %d.", netid));
					}
					break;
					// dodaje kilka takich samych przedmiotów do ekwipunku
				case NetChangePlayer::ADD_ITEMS:
					{
						int team_count, count;
						const Item* item;
						if(s.Read(team_count) && s.Read(count) && ReadItemAndFind(s, item) != -1)
						{
							if(item && count >= team_count)
								AddItem(*pc->unit, item, (uint)count, (uint)team_count);
							else
								ERROR(Format("ADD_ITEMS: Item 0x%p, count %u, team count %u.", item, count, team_count));
						}
						else
							READ_ERROR("ADD_ITEMS");
					}
					break;
					// dodaje przedmioty do ekwipunku handlarza
				case NetChangePlayer::ADD_ITEMS_TRADER:
					{
						int netid, count, team_count;
						const Item* item;
						if(s.Read(netid) && s.Read(count) && s.Read(team_count) && ReadItemAndFind(s, item) != -1)
						{
							Unit* u = FindUnit(netid);
							if(u)
							{
								if(pc->IsTradingWith(u))
									AddItem(*u, item, (uint)count, (uint)team_count);
							}
							else
								ERROR(Format("ADD_ITEMS_TRADER: Missing unit %d (Item %s, count %u, team count %u).", netid, item->id.c_str(), count, team_count));
						}
						else
							READ_ERROR("ADD_ITEMS_TRADER");
					}
					break;
					// dodaje przedmioty do skrzyni która jest u¿ywana
				case NetChangePlayer::ADD_ITEMS_CHEST:
					{
						int netid, count, team_count;
						const Item* item;
						if(s.Read(netid) && s.Read(count) && s.Read(team_count) && ReadItemAndFind(s, item) != -1)
						{
							Chest* c = FindChest(netid);
							if(c)
							{
								if(pc->action == PlayerController::Action_LootChest && pc->action_chest == c)
									AddItem(*c, item, (uint)count, (uint)team_count);
							}
							else
								ERROR(Format("ADD_ITEMS_CHEST: Missing chest %d (Item %s, count %u, team count %u).", netid, item->id.c_str(), count, team_count));
						}
						else
							READ_ERROR("ADD_ITEMS_CHEST");
					}
					break;
					// usuwa przedmiot z ekwipunku
				case NetChangePlayer::REMOVE_ITEMS:
					{
						int index, count;
						if(s.Read(index) && s.Read(count))
							RemoveItem(*pc->unit, index, count);
						else
							READ_ERROR("REMOVE_ITEMS");
					}
					break;
					// usuwa przedmiot z ekwipunku handlarza
				case NetChangePlayer::REMOVE_ITEMS_TRADER:
					{
						int netid, index, count;
						if(s.Read(netid) && s.Read(count) && s.Read(index))
						{
							Unit* u = FindUnit(netid);
							if(u)
							{
								if(pc->IsTradingWith(u))
									RemoveItem(*u, index, count);
							}
							else
								ERROR(Format("REMOVE_ITEMS_TRADER: Missing unit %d (Index %d, count %u).", netid, index, count));
						}
						else
							READ_ERROR("REMOVE_ITEMS_TRADER");
					}
					break;
					//zmienia stan zamro¿enia postaci
				case NetChangePlayer::SET_FROZEN:
					if(!s.ReadCasted<byte>(pc->unit->frozen))
						READ_ERROR("SET_FROZEN");
					break;
					// usuwa questowy przedmiot z ekwipunku
				case NetChangePlayer::REMOVE_QUEST_ITEM:
					{
						int refid;
						if(s.Read(refid))
							pc->unit->RemoveQuestItem(refid);
						else
							READ_ERROR("REMOVE_QUEST_ITEM");
					}
					break;
					// informacja ¿e obiekt jest ju¿ u¿ywany
				case NetChangePlayer::USE_USEABLE:
					AddGameMsg3(GMS_USED);
					break;
					// zmiana trybu kodów
				case NetChangePlayer::CHEATS:
					{
						byte co;
						if(s.Read(co))
						{
							if(co == 1)
							{
								AddMsg(txCanUseCheats);
								cheats = true;
							}
							else
							{
								AddMsg(txCantUseCheats);
								cheats = false;
							}
						}
						else
							READ_ERROR("CHEATS");
					}
					break;
					// pocz¹tek wymiany/dawania przedmiotów
				case NetChangePlayer::START_SHARE:
				case NetChangePlayer::START_GIVE:
					{
						bool is_share = (type == NetChangePlayer::START_SHARE);
						Unit& u = *pc->action_unit;
						if(s.Read(u.weight) && s.Read(u.weight_max) && s.Read(u.gold) && u.stats.Read(s) && ReadItemListTeam(s, u.items))
							StartTrade(is_share ? I_SHARE : I_GIVE, u);
						else
						{
							if(is_share)
								READ_ERROR("START_SHARE");
							else
								READ_ERROR("START_GIVE");
						}
					}
					break;
					// odpowiedŸ serwera czy dany przedmiot jest lepszy
				case NetChangePlayer::IS_BETTER_ITEM:
					{
						byte co;
						if(s.Read(co))
							game_gui->inv_trade_mine->IsBetterItemResponse(co == 1);
						else
							READ_ERROR("IS_BETTER_ITEM");
					}
					break;
					// pytanie o pvp
				case NetChangePlayer::PVP:
					{
						byte id;
						if(s.Read(id))
						{
							pvp_unit = GetPlayerInfo(id).u;
							DialogInfo info;
							info.event = DialogEvent(this, &Game::Event_Pvp);
							info.name = "pvp";
							info.order = ORDER_TOP;
							info.parent = NULL;
							info.pause = false;
							info.text = Format(txPvp, pvp_unit->player->name.c_str());
							info.type = DIALOG_YESNO;
							dialog_pvp = GUI.ShowDialog(info);

							pvp_response.ok = true;
							pvp_response.timer = 0.f;
							pvp_response.to = pc->unit;
						}
						else
							READ_ERROR("PVP");
					}
					break;
					// przygotowanie do przeniesienia
				case NetChangePlayer::PREPARE_WARP:
					fallback_co = FALLBACK_WAIT_FOR_WARP;
					fallback_t = -1.f;
					pc->unit->frozen = 2;
					break;
					// wchodzenie na arenê
				case NetChangePlayer::ENTER_ARENA:
					fallback_co = FALLBACK_ARENA;
					fallback_t = -1.f;
					pc->unit->frozen = 2;
					break;
					// pocz¹tek walki na arenie
				case NetChangePlayer::START_ARENA_COMBAT:
					pc->unit->frozen = 0;
					break;
					// opuszczanie areny
				case NetChangePlayer::EXIT_ARENA:
					fallback_co = FALLBACK_ARENA_EXIT;
					fallback_t = -1.f;
					pc->unit->frozen = 2;
					break;
					// gracz nie zaakceptowa³ pvp
				case NetChangePlayer::NO_PVP:
					{
						byte id;
						if(s.Read(id))
						{
							PlayerInfo* info = GetPlayerInfoTry(id);
							if(info)
								AddMsg(Format(txPvpRefuse, info->name.c_str()));
							else
								WARN(Format("NO_PVP, missing player %d.", id));
						}
						else
							READ_ERROR("NO_PVP");
					}
					break;
					// nie mo¿na opuœciæ lokacji
				case NetChangePlayer::CANT_LEAVE_LOCATION:
					{
						byte why;
						if(s.Read(why))
							AddGameMsg3(why == 1 ? GMS_GATHER_TEAM : GMS_NOT_IN_COMBAT);
						else
							READ_ERROR("CANT_LEAVE_LOCATION");
					}
					break;
					// patrzenie siê na postaæ
				case NetChangePlayer::LOOK_AT:
					{
						int netid;
						if(s.Read(netid))
						{
							if(netid == -1)
								pc->unit->look_target = NULL;
							else
							{
								Unit* u = FindUnit(netid);
								if(u)
									pc->unit->look_target = u;
								else
									WARN(Format("LOOK_AT, missing unit %d.", netid));
							}
						}
						else
							READ_ERROR("LOOK_AT");
					}
					break;
					// koniec fallbacku
				case NetChangePlayer::END_FALLBACK:
					if(fallback_co == FALLBACK_CLIENT)
						fallback_co = FALLBACK_CLIENT2;
					break;
					// reakcja na odpoczynek w karczmie
				case NetChangePlayer::REST:
					{
						byte ile;
						if(s.Read(ile))
						{
							fallback_co = FALLBACK_REST;
							fallback_t = -1.f;
							fallback_1 = ile;
							pc->unit->frozen = 2;
						}
						else
							READ_ERROR("REST");
					}
					break;
					// reakcja na trenowanie
				case NetChangePlayer::TRAIN:
					{
						byte co, co2;
						if(s.Read(co) && s.Read(co2))
						{
							fallback_co = FALLBACK_TRAIN;
							fallback_t = -1.f;
							fallback_1 = co;
							fallback_2 = co2;
							pc->unit->frozen = 2;
						}
						else
							READ_ERROR("TRAIN");
					}
					break;
					// gracz wszed³ w jakieœ blokuj¹ce miejsce i zosta³ cofniêty
				case NetChangePlayer::UNSTUCK:
					{
						VEC3 new_pos;
						if(ReadStruct(s, new_pos))
						{
							pc->unit->pos = new_pos;
							interpolate_timer = 0.1f;
						}
						else
							READ_ERROR("UNSTUCK");
					}
					break;
					// komunikat o otrzymaniu z³ota od innego gracza
				case NetChangePlayer::GOLD_RECEIVED:
					{
						int player_id, ile;
						if(s.Read(player_id) && s.Read(ile))
						{
							PlayerInfo* info = GetPlayerInfoTry(player_id);
							if(info && ile>0)
							{
								AddMultiMsg(Format(txReceivedGold, ile, info->name.c_str()));
								if(sound_volume)
									PlaySound2d(sMoneta);
							}
							else if(!info)
								ERROR(Format("GOLD_RECEIVED, invalid player id %d.", player_id));
							else
								ERROR(Format("GOLD_RECEIVED, %d gold from %s (%d).", ile, info->name.c_str(), player_id));
						}
						else
							READ_ERROR("GOLD_RECEIVED");
					}
					break;
					// message about gaining attribute/skill
				case NetChangePlayer::GAIN_STAT:
					{
						byte is_skill, what, value;
						if(s.Read(is_skill) && s.Read(what) && s.Read(value))
							ShowStatGain(is_skill != 0, what, value);
						else
							READ_ERROR("GAIN_STAT");
					}
					break;
					// przerywa akcje gracza
				case NetChangePlayer::BREAK_ACTION:
					BreakPlayerAction(pc);
					break;
					// aktualizacja z³ota handlarza
				case NetChangePlayer::UPDATE_TRADER_GOLD:
					{
						int netid, ile;
						if(!s.Read(netid) || !s.Read(ile))
							READ_ERROR("UPDATE_TRADER_GOLD");
						else
						{
							Unit* u = FindUnit(netid);
							if(!u)
								ERROR(Format("UPDATE_TRADER_GOLD, missing unit with netid %d.", netid));
							else if(!pc->IsTradingWith(u))
								WARN(Format("UPDATE_TRADER_GOLD, not trading with %s (%d).", u->data->id.c_str(), netid));
							else
								u->gold = ile;
						}
					}
					break;
					// aktualizacja ekwipunku handlarza
				case NetChangePlayer::UPDATE_TRADER_INVENTORY:
					{
						int netid;
						if(!s.Read(netid))
							READ_ERROR("UPDATE_TRADER_INVENTORY");
						else
						{
							Unit* u = FindUnit(netid);
							if(!u)
								ERROR(Format("UPDATE_TRADER_INVENTORY, missing unit with netid %d.", netid));
							else if(!pc->IsTradingWith(u))
								ERROR(Format("UPDATE_TRADER_INVENTORY, not trading with %s (%d).", u->data->id.c_str(), netid));
							else if(!ReadItemListTeam(s, u->items))
								READ_ERROR("UPDATE_TRADER_INVENTORY(2)");
						}
					}
					break;
					// aktualizacja statystyk gracza
				case NetChangePlayer::PLAYER_STATS:
					{
						int flags;
						if(!s.Read(flags))
							READ_ERROR("PLAYER_STATS");
						else if(flags == 0)
							ERROR("PLAYER_STATS: 0 flags set.");
						else
						{
							int set_flags = count_bits(flags);
							// odczytaj do tymczasowego bufora
							if(!s.Read(BUF, sizeof(int)*set_flags))
								READ_ERROR("PLAYER_STATS(2)");
							else if(pc)
							{
								int* buf = (int*)BUF;
								if(IS_SET(flags, STAT_KILLS))
									pc->kills = *buf++;
								if(IS_SET(flags, STAT_DMG_DONE))
									pc->dmg_done = *buf++;
								if(IS_SET(flags, STAT_DMG_TAKEN))
									pc->dmg_taken = *buf++;
								if(IS_SET(flags, STAT_KNOCKS))
									pc->knocks = *buf++;
								if(IS_SET(flags, STAT_ARENA_FIGHTS))
									pc->arena_fights = *buf++;
							}
						}
					}
					break;
					// informacja o dostaniu przedmiotu
				case NetChangePlayer::ADDED_ITEM_MSG:
					AddGameMsg3(GMS_ADDED_ITEM);
					break;
					// informacja o dostaniu kilku przedmiotów
				case NetChangePlayer::ADDED_ITEMS_MSG:
					{
						byte ile;
						if(s.Read(ile))
							AddGameMsg(Format(txGmsAddedItems, (int)ile), 3.f);
						else
							READ_ERROR("ADDED_ITEMS_MSG");
					}
					break;
					// player stat changed
				case NetChangePlayer::STAT_CHANGED:
					{
						byte type, what;
						int value;
						if(s.Read(type) && s.Read(what) && s.Read(value))
						{
							switch((ChangedStatType)type)
							{
							case ChangedStatType::ATTRIBUTE:
								pc->unit->Set((Attribute)what, value);
								break;
							case ChangedStatType::SKILL:
								pc->unit->Set((Skill)what, value);
								break;
							case ChangedStatType::BASE_ATTRIBUTE:
								pc->SetBase((Attribute)what, value);
								break;
							case ChangedStatType::BASE_SKILL:
								pc->SetBase((Skill)what, value);
								break;
							}
						}
						else
							READ_ERROR("STAT_CHANGED");
					}
					break;
					// player gained perk
				case NetChangePlayer::ADD_PERK:
					{
						byte id;
						int value;
						if(s.Read(id) && s.Read(value))
							pc->perks.push_back(TakenPerk((Perk)id, value));
						else
							READ_ERROR("ADD_PERK");
					}
					break;
				default:
					WARN(Format("Unknown player change type %d.", type));
					assert(0);
					break;
				}
			}
		}
		if(pc)
		{
			// zmiana z³ota postaci
			if(IS_SET(flags, PlayerInfo::UF_GOLD))
				s.Read(pc->unit->gold);
			// alkohol
			if(IS_SET(flags, PlayerInfo::UF_ALCOHOL))
				s.Read(pc->unit->alcohol);
			// buffy
			if(IS_SET(flags, PlayerInfo::UF_BUFFS))
				s.ReadCasted<byte>(GetPlayerInfo(pc).buffs);
		}
	}*/

	return true;
}

//=================================================================================================
void Game::Client_Say(BitStream& stream)
{
	byte id;

	if(stream.Read(id) || !ReadString1(stream))
	{
		ERROR("Client_Say: Broken packet.");
		StreamEnd(false);
	}
	else
	{
		int index = GetPlayerIndex(id);
		if(index == -1)
		{
			ERROR(Format("Client_Say: Broken packet, missing player %d.", id));
			StreamEnd(false);
		}
		else
		{
			PlayerInfo& info = game_players[index];
			cstring s = Format("%s: %s", info.name.c_str(), BUF);
			AddMsg(s);
			if(game_state == GS_LEVEL)
				game_gui->AddSpeechBubble(info.u, BUF);
		}
	}
}

//=================================================================================================
void Game::Client_Whisper(BitStream& stream)
{
	byte id;

	if(!stream.Read(id) || !ReadString1(stream))
	{
		ERROR("Client_Whisper: Broken packet.");
		StreamEnd(false);
	}
	else
	{
		int index = GetPlayerIndex(id);
		if(index == -1)
		{
			ERROR(Format("Client_Whisper: Broken packet, missing player %d.", id));
			StreamEnd(false);
		}
		else
		{
			cstring s = Format("%s@: %s", game_players[index].name.c_str(), BUF);
			AddMsg(s);
		}
	}
}

//=================================================================================================
void Game::Client_ServerSay(BitStream& stream)
{
	if(!ReadString1(stream))
	{
		ERROR("Client_ServerSay: Broken packet.");
		StreamEnd(false);
	}
	else
		AddServerMsg(BUF);
}

//=================================================================================================
void Game::Server_Say(BitStream& stream, PlayerInfo& info, Packet* packet)
{
	byte id;

	if(!stream.Read(id) || !ReadString1(stream))
	{
		ERROR(Format("Server_Say: Broken packet from %s: %s.", info.name.c_str()));
		StreamEnd(false);
	}
	else
	{
		// id gracza jest ignorowane przez serwer ale mo¿na je sprawdziæ
		assert(id == info.id);

		cstring str = Format("%s: %s", info.name.c_str(), BUF);
		AddMsg(str);

		// przeœlij dalej
		if(players > 2)
			peer->Send(&stream, MEDIUM_PRIORITY, RELIABLE, 0, packet->systemAddress, true);

		if(game_state == GS_LEVEL)
			game_gui->AddSpeechBubble(info.u, BUF);
	}
}

//=================================================================================================
void Game::Server_Whisper(BitStream& stream, PlayerInfo& info, Packet* packet)
{
	byte id;

	if(!stream.Read(id) || !ReadString1(stream))
	{
		ERROR(Format("Server_Whisper: Broken packet from %s.", info.name.c_str()));
		StreamEnd(false);
	}
	else
	{
		if(id == my_id)
		{
			// wiadomoœæ do mnie
			cstring str = Format("%s@: %s", info.name.c_str(), BUF);
			AddMsg(str);
		}
		else
		{
			// wiadomoœæ do kogoœ innego
			int index = GetPlayerIndex(id);
			if(index == -1)
			{
				ERROR(Format("Server_Whisper: Broken packet from %s to missing player %d.", info.name.c_str(), id));
				StreamEnd(false);
			}
			else
			{
				PlayerInfo& info2 = game_players[index];
				packet->data[1] = (byte)info.id;
				peer->Send((cstring)packet->data, packet->length, MEDIUM_PRIORITY, RELIABLE, 0, info2.adr, false);
			}
		}
	}
}

//=================================================================================================
void Game::ServerProcessUnits(vector<Unit*>& units)
{
	for(vector<Unit*>::iterator it = units.begin(), end = units.end(); it != end; ++it)
	{
		if((*it)->changed)
		{
			NetChange& c = Add1(net_changes);
			c.type = NetChange::UNIT_POS;
			c.unit = *it;
		}
		if((*it)->IsAI() && (*it)->ai->change_ai_mode)
		{
			NetChange& c = Add1(net_changes);
			c.type = NetChange::CHANGE_AI_MODE;
			c.unit = *it;
			(*it)->ai->change_ai_mode = false;
		}
	}
}

//=================================================================================================
GroundItem* Game::FindItemNetid(int netid, LevelContext** ctx)
{
	for(vector<GroundItem*>::iterator it = local_ctx.items->begin(), end = local_ctx.items->end(); it != end; ++it)
	{
		if((*it)->netid == netid)
		{
			if(ctx)
				*ctx = &local_ctx;
			return *it;
		}
	}

	if(city_ctx)
	{
		for(vector<InsideBuilding*>::iterator it = city_ctx->inside_buildings.begin(), end = city_ctx->inside_buildings.end(); it != end; ++it)
		{
			for(vector<GroundItem*>::iterator it2 = (*it)->items.begin(), end2 = (*it)->items.end(); it2 != end2; ++it2)
			{
				if((*it2)->netid == netid)
				{
					if(ctx)
						*ctx = &(*it)->ctx;
					return *it2;
				}
			}
		}
	}

	return NULL;
}

//=================================================================================================
void Game::UpdateWarpData(float dt)
{
	for(vector<WarpData>::iterator it = mp_warps.begin(), end = mp_warps.end(); it != end;)
	{
		if((it->timer -= dt*2) <= 0.f)
		{
			UnitWarpData& uwd = Add1(unit_warp_data);
			uwd.unit = it->u;
			uwd.where = it->where;

			for(vector<Unit*>::iterator it2 = team.begin(), end2 = team.end(); it2 != end2; ++it2)
			{
				if((*it2)->IsHero() && (*it2)->hero->following == it->u)
				{
					UnitWarpData& uwd = Add1(unit_warp_data);
					uwd.unit = *it2;
					uwd.where = it->where;
				}
			}

			NetChangePlayer& c = Add1(net_changes_player);
			c.type = NetChangePlayer::SET_FROZEN;
			c.pc = it->u->player;
			c.id = 0;
			GetPlayerInfo(c.pc->id).NeedUpdate();

			it->u->frozen = 0;

			it = mp_warps.erase(it);
			end = mp_warps.end();
		}
		else
			++it;
	}
}

//=================================================================================================
void Game::Net_OnNewGameClient()
{
	DeleteElements(quest_items);
	cheats = CHEATS_START_MODE;
	train_move = 0.f;
	anyone_talking = false;
	godmode = false;
	noclip = false;
	invisible = false;
	interpolate_timer = 0.f;
	net_changes.clear();
	if(!net_talk.empty())
		StringPool.Free(net_talk);
	paused = false;
	hardcore_mode = false;
	game_gui->mp_box->Reset();
	game_gui->mp_box->visible = true;
}

//=================================================================================================
void Game::Net_OnNewGameServer()
{
	players = 1;
	game_players.clear();
	my_id = 0;
	leader_id = 0;
	last_id = 0;
	paused = false;
	hardcore_mode = false;

	if(!mp_load)
	{
		PlayerInfo& sp = Add1(game_players);
		sp.clas = Class::INVALID;
		sp.ready = false;
		sp.name = player_name;
		sp.id = 0;
		sp.state = PlayerInfo::IN_LOBBY;
		sp.left = false;
		sp.loaded = false;

		netid_counter = 0;
		item_netid_counter = 0;
		chest_netid_counter = 0;
		skip_id_counter = 0;
		useable_netid_counter = 0;
		trap_netid_counter = 0;
		door_netid_counter = 0;
		electro_netid_counter = 0;

		server_panel->grid.AddItem();
		server_panel->CheckAutopick();
	}
	else
	{
		// szukaj postaci serwera w zapisie
		PlayerInfo& sp = Add1(game_players);
		PlayerInfo* old = FindOldPlayer(player_name.c_str());
		sp.ready = false;
		sp.name = player_name;
		sp.id = 0;
		sp.state = PlayerInfo::IN_LOBBY;
		sp.left = false;

		server_panel->grid.AddItem();

		if(old)
		{
			sp.cheats = old->cheats;
			sp.clas = old->clas;
			sp.hd.CopyFrom(old->hd);
			sp.loaded = true;
			server_panel->UseLoadedCharacter(true);
		}
		else
		{
			sp.loaded = false;
			sp.clas = Class::INVALID;
			server_panel->UseLoadedCharacter(false);
			server_panel->CheckAutopick();
		}
	}

	update_timer = 0.f;
	pvp_response.ok = false;
	anyone_talking = false;
	mp_warps.clear();
	minimap_reveal_mp.clear();
	if(!mp_load)
		net_changes.clear(); // przy wczytywaniu jest czyszczone przed wczytaniem i w net_changes s¹ zapisane quest_items
	if(!net_talk.empty())
		StringPool.Free(net_talk);
	net_changes_player.clear();
	max_players2 = max_players;
	server_name2 = server_name;
	enter_pswd = (server_pswd.empty() ? "" : "1");
	game_gui->mp_box->Reset();
	game_gui->mp_box->visible = true;
}

//=================================================================================================
const Item* Game::FindQuestItemClient(cstring id, int refid) const
{
	assert(id);

	for(Item* item : quest_items)
	{
		if(item->id == id && (refid == -1 || item->IsQuest(refid)))
			return item;
	}
	
	return NULL;
}

//=================================================================================================
Useable* Game::FindUseable(int netid)
{
	for(vector<Useable*>::iterator it = local_ctx.useables->begin(), end = local_ctx.useables->end(); it != end; ++it)
	{
		if((*it)->netid == netid)
			return *it;
	}

	if(city_ctx)
	{
		for(vector<InsideBuilding*>::iterator it = city_ctx->inside_buildings.begin(), end = city_ctx->inside_buildings.end(); it != end; ++it)
		{
			if(*it)
			{
				for(vector<Useable*>::iterator it2 = (*it)->useables.begin(), end2 = (*it)->useables.end(); it2 != end2; ++it2)
				{
					if((*it2)->netid == netid)
						return *it2;
				}
			}
		}
	}

	return NULL;
}

//=================================================================================================
int Game::ReadItemAndFind(BitStream& s, const Item*& item) const
{
	if(!ReadString1(s))
	{
		item = NULL;
		return -1;
	}

	if(BUF[0] == 0)
	{
		item = NULL;
		return 0;
	}

	if(BUF[0] == '$')
	{
		int quest_refid;
		if(!s.Read(quest_refid))
		{
			item = NULL;
			return -1;
		}

		item = FindQuestItemClient(BUF, quest_refid);
		if(!item)
		{
			WARN(Format("Missing quest item '%s' (%d) from client.", BUF, quest_refid));
			return 2;
		}
		else
			return 1;
	}
	else
	{
		if(strcmp(BUF, "gold") == 0)
		{
			item = &gold_item;
			return 1;
		}
		else
		{
			item = FindItem(BUF);
			if(!item)
			{
				WARN(Format("Missing item '%s' for client.", BUF));
				return 2;
			}
			else
				return 1;
		}
	}
}

//=================================================================================================
int Game::ReadItemAndFind2(BitStream& s, const Item*& item, cstring& error) const
{
	error = NULL;
	item = NULL;

	if(!ReadString1(s))
		return -2;

	if(BUF[0] == 0)
		return 0;

	if(BUF[0] == '$')
	{
		int quest_refid;
		if(!s.Read(quest_refid))
			return -2;

		item = FindQuestItemClient(BUF, quest_refid);
		if(!item)
		{
			error = Format("Missing quest item '%s' (%d).", BUF, quest_refid);
			return -1;
		}
		else
			return 1;
	}
	else
	{
		if(strcmp(BUF, "gold") == 0)
		{
			item = &gold_item;
			return 1;
		}
		else
		{
			item = FindItem(BUF);
			if(!item)
			{
				error = Format("Missing item '%s'.", BUF);
				return -1;
			}
			else
				return 1;
		}
	}
}

//=================================================================================================
Door* Game::FindDoor(int netid)
{
	if(local_ctx.doors)
	{
		for(vector<Door*>::iterator it = local_ctx.doors->begin(), end = local_ctx.doors->end(); it != end; ++it)
		{
			if((*it)->netid == netid)
				return *it;
		}
	}

	if(city_ctx)
	{
		for(vector<InsideBuilding*>::iterator it2 = city_ctx->inside_buildings.begin(), end2 = city_ctx->inside_buildings.end(); it2 != end2; ++it2)
		{
			for(vector<Door*>::iterator it = (*it2)->doors.begin(), end = (*it2)->doors.end(); it != end; ++it)
			{
				if((*it)->netid == netid)
					return *it;
			}
		}
	}

	return NULL;
}

//=================================================================================================
Trap* Game::FindTrap(int netid)
{
	if(local_ctx.traps)
	{
		for(vector<Trap*>::iterator it = local_ctx.traps->begin(), end = local_ctx.traps->end(); it != end; ++it)
		{
			if((*it)->netid == netid)
				return *it;
		}
	}

	return NULL;
}

//=================================================================================================
Chest* Game::FindChest(int netid)
{
	if(local_ctx.chests)
	{
		for(vector<Chest*>::iterator it = local_ctx.chests->begin(), end = local_ctx.chests->end(); it != end; ++it)
		{
			if((*it)->netid == netid)
				return *it;
		}
	}

	return NULL;
}

//=================================================================================================
bool Game::RemoveTrap(int netid)
{
	if(local_ctx.traps)
	{
		for(vector<Trap*>::iterator it = local_ctx.traps->begin(), end = local_ctx.traps->end(); it != end; ++it)
		{
			if((*it)->netid == netid)
			{
				delete *it;
				local_ctx.traps->erase(it);
				return true;
			}
		}
	}

	return false;
}

//=================================================================================================
Electro* Game::FindElectro(int netid)
{
	for(vector<Electro*>::iterator it = local_ctx.electros->begin(), end = local_ctx.electros->end(); it != end; ++it)
	{
		if((*it)->netid == netid)
			return *it;
	}

	if(city_ctx)
	{
		for(vector<InsideBuilding*>::iterator it = city_ctx->inside_buildings.begin(), end = city_ctx->inside_buildings.end(); it != end; ++it)
		{
			for(vector<Electro*>::iterator it2 = (*it)->ctx.electros->begin(), end2 = (*it)->ctx.electros->end(); it2 != end2; ++it2)
			{
				if((*it2)->netid == netid)
					return *it2;
			}
		}
	}

	return NULL;
}

//=================================================================================================
void Game::ReequipItemsMP(Unit& u)
{
	if(players > 1)
	{
		const Item* prev_slots[SLOT_MAX];

		for(int i=0; i<SLOT_MAX; ++i)
			prev_slots[i] = u.slots[i];

		u.ReequipItems();

		for(int i=0; i<SLOT_MAX; ++i)
		{
			if(u.slots[i] != prev_slots[i])
			{
				NetChange& c = Add1(net_changes);
				c.type = NetChange::CHANGE_EQUIPMENT;
				c.unit = &u;
				c.id = i;
			}
		}
	}
	else
		u.ReequipItems();
}

//=================================================================================================
void Game::UseDays(PlayerController* player, int ile)
{
	assert(player && ile > 0);

	if(player->free_days >= ile)
		player->free_days -= ile;
	else
	{
		ile -= player->free_days;
		player->free_days = 0;

		for(vector<PlayerInfo>::iterator it = game_players.begin(), end = game_players.end(); it != end; ++it)
		{
			if(!it->left && it->u->player != player)
				it->u->player->free_days += ile;
		}

		WorldProgress(ile, WPM_NORMAL);
	}

	PushNetChange(NetChange::UPDATE_FREE_DAYS);
}

//=================================================================================================
PlayerInfo* Game::FindOldPlayer(cstring nick)
{
	assert(nick);

	for(vector<PlayerInfo>::iterator it = old_players.begin(), end = old_players.end(); it != end; ++it)
	{
		if(it->name == nick)
			return &*it;
	}

	return NULL;
}

//=================================================================================================
void Game::PrepareWorldData(BitStream& s)
{
	LOG("Preparing world data.");

	s.Reset();
	s.WriteCasted<byte>(ID_WORLD_DATA);

	// lokacje
	s.WriteCasted<byte>(locations.size());
	for(vector<Location*>::iterator it = locations.begin(), end = locations.end(); it != end; ++it)
	{
		if(*it)
		{
			Location& loc = **it;
			s.WriteCasted<byte>(loc.type);
			if(loc.type == L_DUNGEON || loc.type == L_CRYPT)
				s.WriteCasted<byte>(loc.GetLastLevel()+1);
			else if(loc.type == L_CITY || loc.type == L_VILLAGE)
			{
				City& city = (City&)loc;
				s.WriteCasted<byte>(city.citizens);
				s.WriteCasted<word>(city.citizens_world);
				if(loc.type == L_VILLAGE)
				{
					Village& village = (Village&)city;
					s.WriteCasted<byte>(village.v_buildings[0]);
					s.WriteCasted<byte>(village.v_buildings[1]);
				}
			}
			s.WriteCasted<byte>(loc.state);
			WriteStruct(s, loc.pos);
			WriteString1(s, loc.name);
		}
		else
			s.WriteCasted<byte>(L_NULL);
	}
	s.WriteCasted<byte>(current_location);

	// questy
	word ile = (word)quests.size();
	s.Write(ile);
	for(vector<Quest*>::iterator it = quests.begin(), end = quests.end(); it != end; ++it)
	{
		s.Write((*it)->refid);
		s.WriteCasted<byte>((*it)->state);
		WriteString1(s, (*it)->name);
		WriteStringArray<byte,word>(s, (*it)->msgs);
	}

	// plotki
	WriteStringArray<byte,word>(s, plotki);

	// czas gry
	s.WriteCasted<byte>(year);
	s.WriteCasted<byte>(month);
	s.WriteCasted<byte>(day);
	s.Write(worldtime);
	s.Write(gt_hour);
	s.WriteCasted<byte>(gt_minute);
	s.WriteCasted<byte>(gt_second);

	// mp vars
	WriteNetVars(s);

	// questowe przedmioty
	s.WriteCasted<word>(net_changes.size());
	for(vector<NetChange>::iterator it = net_changes.begin(), end = net_changes.end(); it != end; ++it)
	{
		assert(it->type == NetChange::REGISTER_ITEM);
		const Item* item = it->base_item;
		WriteString1(s, item->id);
		WriteString1(s, item->name);
		WriteString1(s, item->desc);
		s.Write(item->refid);
	}
	net_changes.clear();
	if(!net_talk.empty())
		StringPool.Free(net_talk);

	// tekst listu sekret
	WriteString1(s, GetSecretNote()->desc);

	// pozycja spotkania na mapie
	if(world_state == WS_TRAVEL)
	{
		s.WriteCasted<byte>(1);
		s.Write(picked_location);
		s.Write(travel_day);
		s.Write(travel_time);
		s.Write(travel_start);
		WriteStruct(s, world_pos);
	}
	else
		s.WriteCasted<byte>(0);
}

//=================================================================================================
#undef READ_ERROR
#define READ_ERROR ERROR("Read error in ReadWorldData (" STRING(__LINE__) ").")
bool Game::ReadWorldData(BitStream& s)
{
	// odczytaj lokacje
	byte ile;
	s.Read(ile); // odczyt sprawdzony w ID_WORLD_DATA (minimum 2 d³ugoœci)

	locations.resize(ile);
	int index = 0;
	for(vector<Location*>::iterator it = locations.begin(), end = locations.end(); it != end; ++it, ++index)
	{
		LOCATION type;

		if(!s.ReadCasted<byte>(type))
		{
			READ_ERROR;
			return false;
		}

		if(type == L_NULL)
		{
			*it = NULL;
			continue;
		}

		Location* loc;
		switch(type)
		{
		case L_DUNGEON:
		case L_CRYPT:
			{
				byte levels;
				if(!s.Read(levels))
				{
					READ_ERROR;
					return false;
				}

				if(levels == 0)
				{
					ERROR(Format("Broken packet ID_WORLD_DATA, location %d with 0 levels.", index));
					return false;
				}
				else if(levels == 1)
					loc = new SingleInsideLocation;
				else
					loc = new MultiInsideLocation(levels);
			}
			break;
		case L_CAVE:
			loc = new CaveLocation;
			break;
		case L_FOREST:
		case L_ENCOUNTER:
		case L_CAMP:
		case L_MOONWELL:
		case L_ACADEMY:
			loc = new OutsideLocation;
			break;
		case L_CITY:
		case L_VILLAGE:
			{
				byte citizens;
				word world_citizens;
				if(!s.Read(citizens) || !s.Read(world_citizens))
				{
					READ_ERROR;
					return false;
				}

				if(type == L_CITY)
				{
					City* city = new City;
					loc = city;
					city->citizens = citizens;
					city->citizens_world = world_citizens;
				}
				else
				{
					Village* village = new Village;
					loc = village;
					village->citizens = citizens;
					village->citizens_world = world_citizens;

					if(!s.ReadCasted<byte>(village->v_buildings[0]) || !s.ReadCasted<byte>(village->v_buildings[1]))
					{
						delete village;
						READ_ERROR;
						return false;
					}

					if(village->v_buildings[0] > B_NONE)
					{
						delete village;
						ERROR(Format("Broken packet ID_WORLD_DATA, unknown building type %d.", village->v_buildings[0]));
						return false;
					}

					if(village->v_buildings[1] > B_NONE)
					{
						delete village;
						ERROR(Format("Broken packet ID_WORLD_DATA, unknown building type %d (2).", village->v_buildings[1]));
						return false;
					}
				}
			}
			break;
		default:
			ERROR(Format("Broken packet ID_WORLD_DATA, unknown location type %d.", type));
			return false;
		}

		// stan lokacji
		if(	!s.ReadCasted<byte>(loc->state) ||
			!ReadStruct(s, loc->pos) ||
			!ReadString1(s, loc->name))
		{
			delete loc;
			READ_ERROR;
			return false;
		}

		*it = loc;
		loc->type = type;
		
		if(loc->state > LS_CLEARED)
		{
			ERROR(Format("Broken packet ID_WORLD_DATA, unknown location state %d.", loc->state));
			return false;
		}
	}

	// aktualna lokacja
	if(!s.ReadCasted<byte>(current_location))
	{
		READ_ERROR;
		return false;
	}
	world_pos = locations[current_location]->pos;
	locations[current_location]->state = LS_VISITED;

	// questy
	word ile2;
	if(!s.Read(ile2))
	{
		READ_ERROR;
		return false;
	}
	quests.resize(ile2);
	index = 0;
	for(vector<Quest*>::iterator it = quests.begin(), end = quests.end(); it != end; ++it, ++index)
	{
		*it = new PlaceholderQuest;
		(*it)->quest_index = index;
		if(	!s.Read((*it)->refid) ||
			!s.ReadCasted<byte>((*it)->state) ||
			!ReadString1(s, (*it)->name) ||
			!ReadStringArray<byte,word>(s, (*it)->msgs))
		{
			READ_ERROR;
			return false;
		}
	}

	// plotki
	if(!ReadStringArray<byte,word>(s, plotki))
	{
		READ_ERROR;
		return false;
	}

	// czas
	if(	!s.ReadCasted<byte>(year) ||
		!s.ReadCasted<byte>(month) ||
		!s.ReadCasted<byte>(day) ||
		!s.Read(worldtime) ||
		!s.Read(gt_hour) ||
		!s.ReadCasted<byte>(gt_minute) ||
		!s.ReadCasted<byte>(gt_second))
	{
		READ_ERROR;
		return false;
	}

	// mp vars
	if(!ReadNetVars(s))
	{
		READ_ERROR;
		return false;
	}

	// questowe przedmioty
	if(!s.Read(ile2))
	{
		READ_ERROR;
		return false;
	}
	quest_items.reserve(ile2);
	for(word i = 0; i < ile2; ++i)
	{
		if(ReadString1(s))
		{
			const Item* base;
			if(BUF[0] == '$')
				base = FindItem(BUF + 1);
			else
				base = FindItem(BUF);
			if(base)
			{
				Item* item = CreateItemCopy(base);
				if(ReadString1(s, item->name) &&
					ReadString1(s, item->desc) &&
					s.Read(item->refid))
				{
					item->id = BUF;
					quest_items.push_back(item);
				}
				else
				{
					READ_ERROR;
					delete item;
					return false;
				}
			}
			else
			{
				READ_ERROR;
				return false;
			}
		}
		else
		{
			READ_ERROR;
			return false;
		}
	}

	if(!ReadString1(s, GetSecretNote()->desc))
	{
		READ_ERROR;
		return false;
	}

	// pozycja spotkania na mapie
	byte on_travel;
	if(!s.Read(on_travel))
	{
		READ_ERROR;
		return false;
	}

	if(on_travel == 1)
	{
		world_state = WS_TRAVEL;
		if( !s.Read(picked_location) ||
			!s.Read(travel_day) ||
			!s.Read(travel_time) ||
			!s.Read(travel_start) ||
			!ReadStruct(s, world_pos))
		{
			READ_ERROR;
			return false;
		}
	}

	return true;
}

//=================================================================================================
void Game::WriteNetVars(BitStream& s)
{
	WriteBool(s, mp_use_interp);
	s.Write(mp_interp);
}

//=================================================================================================
bool Game::ReadNetVars(BitStream& s)
{
	return ReadBool(s, mp_use_interp) && 
		s.Read(mp_interp);
}

//=================================================================================================
void Game::InterpolateUnits(float dt)
{
	for(vector<Unit*>::iterator it = local_ctx.units->begin(), end = local_ctx.units->end(); it != end; ++it)
	{
		Unit* u = *it;
		if(u != pc->unit)
			UpdateInterpolator(u->interp, dt, u->visual_pos, u->rot);
		if(u->ani->ani->head.n_groups == 1)
		{
			if(!u->ani->groups[0].anim)
			{
				u->action = A_NONE;
				u->animation = ANI_STAND;
			}
		}
		else
		{
			if(!u->ani->groups[0].anim && !u->ani->groups[1].anim)
			{
				u->action = A_NONE;
				u->animation = ANI_STAND;
			}
		}
	}
	if(city_ctx)
	{
		for(vector<InsideBuilding*>::iterator it2 = city_ctx->inside_buildings.begin(), end2 = city_ctx->inside_buildings.end(); it2 != end2; ++it2)
		{
			for(vector<Unit*>::iterator it = (*it2)->units.begin(), end = (*it2)->units.end(); it != end; ++it)
			{
				Unit* u = *it;
				if(u != pc->unit)
					UpdateInterpolator(u->interp, dt, u->visual_pos, u->rot);
				if(u->ani->ani->head.n_groups == 1)
				{
					if(!u->ani->groups[0].anim)
					{
						u->action = A_NONE;
						u->animation = ANI_STAND;
					}
				}
				else
				{
					if(!u->ani->groups[0].anim && !u->ani->groups[1].anim)
					{
						u->action = A_NONE;
						u->animation = ANI_STAND;
					}
				}
			}
		}
	}
}

//=================================================================================================
void Game::InterpolatePlayers(float dt)
{
	for(vector<PlayerInfo>::iterator it = game_players.begin(), end = game_players.end(); it != end; ++it)
	{
		if(it->id != my_id && !it->left)
			UpdateInterpolator(it->u->interp, dt, it->u->visual_pos, it->u->rot);
	}
}

//=================================================================================================
void EntityInterpolator::Reset(const VEC3& pos, float rot)
{
	valid_entries = 1;
	entries[0].pos = pos;
	entries[0].rot = rot;
	entries[0].timer = 0.f;
}

//=================================================================================================
void EntityInterpolator::Add(const VEC3& pos, float rot)
{
	for(int i = MAX_ENTRIES - 1; i>0; --i)
		entries[i] = entries[i - 1];

	entries[0].pos = pos;
	entries[0].rot = rot;
	entries[0].timer = 0.f;

	valid_entries = min(valid_entries + 1, MAX_ENTRIES);
}

//=================================================================================================
void Game::UpdateInterpolator(EntityInterpolator* e, float dt, VEC3& pos, float& rot)
{
	assert(e);

	for(int i=0; i<EntityInterpolator::MAX_ENTRIES; ++i)
		e->entries[i].timer += dt;

	if(mp_use_interp)
	{
		if(e->entries[0].timer > mp_interp)
		{
			// nie ma nowszej klatki
			// extrapolation ? nie dziœ...
			pos = e->entries[0].pos;
			rot = e->entries[0].rot;
		}
		else
		{
			// znajdŸ odpowiednie klatki
			for(int i=0; i<e->valid_entries; ++i)
			{
				if(equal(e->entries[i].timer, mp_interp))
				{
					// równe trafienie w klatke
					pos = e->entries[i].pos;
					rot = e->entries[i].rot;
					return;
				}
				else if(e->entries[i].timer > mp_interp)
				{
					// interpolacja pomiêdzy dwoma klatkami ([i-1],[i])
					EntityInterpolator::Entry& e1 = e->entries[i-1];
					EntityInterpolator::Entry& e2 = e->entries[i];
					float t = (mp_interp - e1.timer)/(e2.timer - e1.timer);
					pos = lerp(e1.pos, e2.pos, t);
					rot = clip(slerp(e1.rot, e2.rot, t));
					return;
				}
			}

			// brak ruchu do tej pory
		}
	}
	else
	{
		// nie u¿ywa interpolacji
		pos = e->entries[0].pos;
		rot = e->entries[0].rot;
	}

	assert(rot >= 0.f && rot < PI*2);
}

//=================================================================================================
void Game::WritePlayerStartData(BitStream& s, PlayerInfo& info)
{
	// flagi
	byte flagi = 0;
	if(info.cheats)
		flagi |= 0x01;
	if(mp_load)
	{
		if(info.u->invisible)
			flagi |= 0x02;
		if(info.u->player->noclip)
			flagi |= 0x04;
		if(info.u->player->godmode)
			flagi |= 0x08;
	}
	// mo¿na by to daæ w PrepareWorldData ale skoro ju¿ tu jest
	if(noai)
		flagi |= 0x10;
	s.Write(flagi);

	// notatki
	WriteStringArray<word,word>(s, info.notes);
}

//=================================================================================================
bool Game::ReadPlayerStartData(BitStream& s)
{
	byte flagi;

	if(	!s.Read(flagi) ||
		!ReadStringArray<word,word>(s, notes))
		return false;

	if(IS_SET(flagi, 0x01))
		cheats = true;
	if(IS_SET(flagi, 0x02))
		invisible = true;
	if(IS_SET(flagi, 0x04))
		noclip = true;
	if(IS_SET(flagi, 0x08))
		godmode = true;
	if(IS_SET(flagi, 0x10))
		noai = true;

	return true;
}

//=================================================================================================
bool Game::CheckMoveNet(Unit& unit, const VEC3& pos)
{
	global_col.clear();

	const float radius = unit.GetUnitRadius();
	IgnoreObjects ignore = {0};
	Unit* ignored[] = { &unit, NULL };
	const void* ignored_objects[2] = {NULL, NULL};
	if(unit.useable)
		ignored_objects[0] = unit.useable;
	ignore.ignored_units = (const Unit**)ignored;
	ignore.ignored_objects = ignored_objects;
	
	GatherCollisionObjects(GetContext(unit), global_col, pos, radius, &ignore);

	if(global_col.empty())
		return true;

	return !Collide(global_col, pos, radius);
}

//=================================================================================================
Unit* Game::FindTeamMember(const string& name)
{
	for(vector<Unit*>::iterator it = active_team.begin(), end = active_team.end(); it != end; ++it)
	{
		if((*it)->GetName() == name)
			return *it;
	}
	return NULL;
}

//=================================================================================================
Unit* Game::FindTeamMember(int netid)
{
	for(vector<Unit*>::iterator it = active_team.begin(), end = active_team.end(); it != end; ++it)
	{
		if((*it)->netid == netid)
			return *it;
	}
	return NULL;
}

//=================================================================================================
void Game::Net_PreSave()
{
	// poinformuj graczy o zapisywaniu
	//byte b = ID_SAVING;
	//peer->Send((char*)&b, 1, IMMEDIATE_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);

	// players_left
	ProcessLeftPlayers();
}

//=================================================================================================
void Game::ProcessLeftPlayers()
{
	for(vector<int>::iterator it = players_left.begin(), end = players_left.end(); it != end; ++it)
	{
		// kolejnoœæ zmian ma znaczenie, najpierw przerywa jego akcjê a dopiero potem usuwa bo inaczej komunikat siê zepsuje
		PlayerInfo& info = GetPlayerInfo(*it);
		NetChange& c = Add1(net_changes);
		c.type = NetChange::REMOVE_PLAYER;
		c.id = *it;
		c.ile = info.left_reason;

		//ConvertPlayerToAI(info);

		--players;
		if(info.left_reason != PlayerInfo::LEFT_KICK)
		{
			LOG(Format("Player %s left game.", info.name.c_str()));
			AddMsg(Format(txPlayerLeft, info.name.c_str()));
		}

		if(info.u)
		{
			if(before_player_ptr.unit == info.u)
				before_player = BP_NONE;
			if(info.left_reason == PlayerInfo::LEFT_LOADING || game_state == GS_WORLDMAP)
			{
				if(open_location != -1)
					RemoveElement(GetContext(*info.u).units, info.u);
				RemoveElement(team, info.u);
				RemoveElement(active_team, info.u);
				if(info.u->interp)
					interpolators.Free(info.u->interp);
				if(info.u->cobj)
					delete info.u->cobj->getCollisionShape();
				delete info.u;
				info.u = NULL;
			}
			else
			{
				if(info.u->useable)
					info.u->useable->user = NULL;
				switch(info.u->player->action)
				{
				case PlayerController::Action_LootChest:
					{
						info.u->player->action_chest->looted = false;
						// zamykanie skrzyni
						info.u->player->action_chest->ani->Play(&info.u->player->action_chest->ani->ani->anims[0], PLAY_PRIO1|PLAY_ONCE|PLAY_STOP_AT_END|PLAY_BACK, 0);
						if(sound_volume)
						{
							VEC3 pos = info.u->player->action_chest->pos;
							pos.y += 0.5f;
							PlaySound3d(sChestClose, pos, 2.f, 5.f);
						}
						NetChange& c = Add1(net_changes);
						c.type = NetChange::CHEST_CLOSE;
						c.id = info.u->player->action_chest->netid;
					}
					break;
				case PlayerController::Action_LootUnit:
					info.u->player->action_unit->busy = Unit::Busy_No;
					break;
				case PlayerController::Action_Trade:
				case PlayerController::Action_Talk:
				case PlayerController::Action_GiveItems:
				case PlayerController::Action_ShareItems:
					info.u->player->action_unit->busy = Unit::Busy_No;
					info.u->player->action_unit->look_target = NULL;
					break;
				}

				if(chlanie_stan >= 3)
					RemoveElementTry(chlanie_ludzie, info.u);
				if(!arena_free)
					RemoveElementTry(at_arena, info.u);
				if(zawody_stan >= IS_TRWAJA)
				{
					RemoveElementTry(zawody_ludzie, info.u);
					for(vector<std::pair<Unit*, Unit*> >::iterator it = zawody_walczacy.begin(), end = zawody_walczacy.end(); it != end; ++it)
					{
						if(it->first == info.u)
						{
							it->first = NULL;
							break;
						}
						else if(it->second == info.u)
						{
							it->second = NULL;
							break;
						}
					}
					if(zawody_niewalczacy == info.u)
						zawody_niewalczacy = NULL;
					if(zawody_drugi_zawodnik == info.u)
						zawody_niewalczacy = NULL;
				}

				RemoveElement(team, info.u);
				RemoveElement(active_team, info.u);
				to_remove.push_back(info.u);
				info.u->to_remove = true;
				info.u = NULL;
			}
		}

		if(leader_id == c.id)
		{
			leader_id = my_id;
			leader = pc->unit;
			NetChange& c2 = Add1(net_changes);
			c2.type = NetChange::CHANGE_LEADER;
			c2.id = my_id;

			if(dialog_enc)
				dialog_enc->bts[0].state = Button::NONE;
		}

		CheckCredit();
	}
	players_left.clear();
}

//=================================================================================================
bool Game::FilterOut(NetChange& c)
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
	case NetChange::ADD_QUEST_MAIN:
	case NetChange::UPDATE_QUEST:
	case NetChange::RENAME_ITEM:
	case NetChange::UPDATE_QUEST_MULTI:
	case NetChange::REMOVE_PLAYER:
	case NetChange::CHANGE_LEADER:
	case NetChange::RANDOM_NUMBER:
	case NetChange::CHEAT_SKIP_DAYS:
	case NetChange::CHEAT_NOCLIP:
	case NetChange::CHEAT_GODMODE:
	case NetChange::CHEAT_INVISIBLE:
	case NetChange::CHEAT_ADDITEM:
	case NetChange::CHEAT_ADDGOLD:
	case NetChange::CHEAT_ADDGOLD_TEAM:
	case NetChange::CHEAT_SETSTAT:
	case NetChange::CHEAT_MODSTAT:
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
		return false;
	case NetChange::TALK:
		if(IsServer() && c.str)
		{
			StringPool.Free(c.str);
			RemoveElement(net_talk, c.str);
			c.str = NULL;
		}
		return true;
	default:
		return true;
		break;
	}
}

//=================================================================================================
bool Game::FilterOut(NetChangePlayer& c)
{
	switch(c.type)
	{
	case NetChangePlayer::GOLD_MSG:
	case NetChangePlayer::CHEATS:
	case NetChangePlayer::GOLD_RECEIVED:
	case NetChangePlayer::GAIN_STAT:
	case NetChangePlayer::ADDED_ITEM_MSG:
	case NetChangePlayer::ADDED_ITEMS_MSG:
		return false;
	default:
		return true;
		break;
	}
}

//=================================================================================================
void Game::Net_FilterClientChanges()
{
	for(vector<NetChange>::iterator it = net_changes.begin(), end = net_changes.end(); it != end;)
	{
		if(FilterOut(*it))
		{
			if(it+1 == end)
			{
				net_changes.pop_back();
				break;
			}
			else
			{
				std::iter_swap(it, end-1);
				net_changes.pop_back();
				end = net_changes.end();
			}
		}
		else
			++it;
	}
}

//=================================================================================================
void Game::Net_FilterServerChanges()
{
	for(vector<NetChange>::iterator it = net_changes.begin(), end = net_changes.end(); it != end;)
	{
		if(FilterOut(*it))
		{
			if(it+1 == end)
			{
				net_changes.pop_back();
				break;
			}
			else
			{
				std::iter_swap(it, end-1);
				net_changes.pop_back();
				end = net_changes.end();
			}
		}
		else
			++it;
	}

	for(vector<NetChangePlayer>::iterator it = net_changes_player.begin(), end = net_changes_player.end(); it != end;)
	{
		if(FilterOut(*it))
		{
			if(it+1 == end)
			{
				net_changes_player.pop_back();
				break;
			}
			else
			{
				std::iter_swap(it, end-1);
				net_changes_player.pop_back();
				end = net_changes_player.end();
			}
		}
		else
			++it;
	}
}

//=================================================================================================
void Game::ClosePeer(bool wait)
{
	assert(peer);

	LOG("Peer shutdown.");
	peer->Shutdown(wait ? I_SHUTDOWN : 0);
	LOG("sv_online = false");
	if(!sv_server)
		was_client = true;
	net_changes.clear();
	net_changes_player.clear();
	sv_online = false;
}

//=================================================================================================
void Game::RemovePlayerOnLoad(PlayerInfo& info)
{
	delete info.u;
	RemoveElementOrder(team, info.u);
	RemoveElementOrder(active_team, info.u);
	if(leader == info.u)
		leader_id = -1;
	--players;
	peer->CloseConnection(info.adr, true, 0, IMMEDIATE_PRIORITY);
}

//=================================================================================================
BitStream& Game::StreamStart(Packet* packet, StreamLogType type)
{
	assert(packet);
	assert(!current_packet);

	current_packet = packet;
	current_stream.~BitStream();
	new((void*)&current_stream)BitStream(packet->data, packet->length, false);
	ErrorHandler::Get().StreamStart(current_packet, (int)type);

	return current_stream;
}

//=================================================================================================
void Game::StreamEnd(bool ok)
{
	if(!current_packet)
		return;

	ErrorHandler::Get().StreamEnd(ok);
	current_packet = NULL;
}
