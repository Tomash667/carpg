#include "Pch.h"
#include "Base.h"
#include "Game.h"
#include "BitStreamFunc.h"
#include "Wersja.h"

extern const uint g_build;
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
	// ? - build
	server_info.Reset();
	server_info.WriteCasted<byte>('C');
	server_info.WriteCasted<byte>('A');
	server_info.Write(WERSJA);
	server_info.WriteCasted<byte>(players);
	server_info.WriteCasted<byte>(max_players);
	byte flags = 0;
	if(!server_pswd.empty())
		flags |= 0x01;
	if(mp_load)
		flags |= 0x02;
	server_info.WriteCasted<byte>(flags);
	WriteString1(server_info, server_name);
	server_info.Write(g_build);

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
		info.left_reason = 1;
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
					WriteUseable(s, **it2);
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
					WriteBlood(s, *it2);
				// obiekty
				ile = (byte)ib.objects.size();
				s.Write(ile);
				for(vector<Object>::iterator it2 = ib.objects.begin(), end2 = ib.objects.end(); it2 != end2; ++it2)
					WriteObject(s, *it2);
				// œwiat³a
				ile = (byte)ib.lights.size();
				s.Write(ile);
				for(vector<Light>::iterator it2 = ib.lights.begin(), end2 = ib.lights.end(); it2 != end2; ++it2)
					WriteLight(s, *it2);
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
		s.WriteCasted<byte>(inside->cel);
		s.WriteCasted<byte>(inside->from_portal ? 1 : 0);
		// mapa
		s.WriteCasted<byte>(lvl.w);
		s.Write((cstring)lvl.mapa, sizeof(Pole)*lvl.w*lvl.h);
		// œwiat³a
		byte ile = (byte)lvl.lights.size();
		s.Write(ile);
		for(vector<Light>::iterator it = lvl.lights.begin(), end = lvl.lights.end(); it != end; ++it)
			WriteLight(s, *it);
		// pokoje
		ile = (byte)lvl.pokoje.size();
		s.Write(ile);
		for(vector<Pokoj>::iterator it = lvl.pokoje.begin(), end = lvl.pokoje.end(); it != end; ++it)
			WriteRoom(s, *it);
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
		s.Write((cstring)&lvl.schody_gora, sizeof(lvl.schody_gora));
		s.Write((cstring)&lvl.schody_dol, sizeof(lvl.schody_dol));
		s.WriteCasted<byte>(lvl.schody_gora_dir);
		s.WriteCasted<byte>(lvl.schody_dol_dir);
		s.WriteCasted<byte>(lvl.schody_dol_w_scianie ? 1 : 0);
	}

	// u¿ywalne obiekty
	ile = (byte)local_ctx.useables->size();
	s.Write(ile);
	for(vector<Useable*>::iterator it = local_ctx.useables->begin(), end = local_ctx.useables->end(); it != end; ++it)
		WriteUseable(s, **it);
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
		WriteBlood(s, *it);
	// obiekty
	ile2 = (word)local_ctx.objects->size();
	s.Write(ile2);
	for(vector<Object>::iterator it = local_ctx.objects->begin(), end = local_ctx.objects->end(); it != end; ++it)
		WriteObject(s, *it);
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
			s.WriteCasted<byte>(e.tex.res->filename == "explosion.jpg" ? 1 : 0);
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
		s.Write(unit.hero->kredyt);
	}
	else if(unit.IsPlayer())
	{
		WriteString1(s, unit.player->name);
		s.WriteCasted<byte>(unit.player->clas);
		s.WriteCasted<byte>(unit.player->id);
		s.Write(unit.player->kredyt);
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
		s.WriteCasted<byte>(unit.animacja);
		s.WriteCasted<byte>(unit.animacja2);
		s.WriteCasted<byte>(unit.etap_animacji);
		s.WriteCasted<byte>(unit.attack_id);
		s.WriteCasted<byte>(unit.action);
		s.WriteCasted<byte>(unit.wyjeta);
		s.WriteCasted<byte>(unit.chowana);
		s.WriteCasted<byte>(unit.stan_broni);
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
void Game::WriteObject(BitStream& s, Object& obj)
{
	s.Write((cstring)&obj.pos, sizeof(obj.pos));
	s.Write((cstring)&obj.rot, sizeof(obj.rot));
	s.Write(obj.scale);
	if(obj.base)
		WriteString1(s, obj.base->id);
	else
	{
		byte zero = 0;
		s.Write(zero);
		WriteString1(s, obj.mesh->res->filename);
	}
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
void Game::WriteUseable(BitStream& s, Useable& use)
{
	s.Write(use.netid);
	s.Write((cstring)&use.pos, sizeof(use.pos));
	s.Write(use.rot);
	s.WriteCasted<byte>(use.type);
	s.WriteCasted<byte>(use.variant);
}

//=================================================================================================
void Game::WriteBlood(BitStream& s, Blood& blood)
{
	s.WriteCasted<byte>(blood.type);
	s.Write((cstring)&blood.pos, sizeof(blood.pos));
	s.Write((cstring)&blood.normal, sizeof(blood.normal));
	s.Write(blood.rot);
}

//=================================================================================================
void Game::WriteLight(BitStream& s, Light& light)
{
	s.Write((cstring)&light.pos, sizeof(light.pos));
	s.Write((cstring)&light.color, sizeof(light.color));
	s.Write(light.range);
}

//=================================================================================================
void Game::WriteChest(BitStream& s, Chest& chest)
{
	s.Write((cstring)&chest.pos, sizeof(chest.pos));
	s.Write(chest.rot);
	s.Write(chest.netid);
}

//=================================================================================================
void Game::WriteRoom(BitStream& s, Pokoj& room)
{
	s.Write((cstring)&room.pos, sizeof(room.pos));
	s.Write((cstring)&room.size, sizeof(room.size));
	byte ile = (byte)room.polaczone.size();
	s.Write(ile);
	for(byte i=0; i<ile; ++i)
		s.WriteCasted<byte>(room.polaczone[i]);
	s.WriteCasted<byte>(room.cel);
	s.WriteCasted<byte>(room.korytarz ? 1 : 0);
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
	show_mp_panel = true;
	first_frame = true;
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
					if(!ReadUseable(s, **it2))
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
					if(!ReadBlood(s, *it2))
						return MD;
				}
				// obiekty
				if(!s.Read(ile))
					return MD;
				ib.objects.resize(ile);
				for(vector<Object>::iterator it2 = ib.objects.begin(), end2 = ib.objects.end(); it2 != end2; ++it2)
				{
					if(!ReadObject(s, *it2))
						return MD;
				}
				// œwiat³a
				if(!s.Read(ile))
					return MD;
				ib.lights.resize(ile);
				for(vector<Light>::iterator it2 = ib.lights.begin(), end2 = ib.lights.end(); it2 != end2; ++it2)
				{
					if(!ReadLight(s, *it2))
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
		if(!s.ReadCasted<byte>(inside->cel))
			return MD;
		byte from_portal;
		if(!s.Read(from_portal))
			return MD;
		inside->from_portal = (from_portal == 1);
		// mapa
		if(!s.ReadCasted<byte>(lvl.w))
			return MD;
		lvl.h = lvl.w;
		if(!lvl.mapa)
			lvl.mapa = new Pole[lvl.w*lvl.h];
		if(!s.Read((char*)lvl.mapa, sizeof(Pole)*lvl.w*lvl.h))
			return MD;
		// œwiat³a
		byte ile;
		if(!s.Read(ile))
			return MD;
		lvl.lights.resize(ile);
		for(vector<Light>::iterator it = lvl.lights.begin(), end = lvl.lights.end(); it != end; ++it)
		{
			if(!ReadLight(s, *it))
				return MD;
		}
		// pokoje
		if(!s.Read(ile))
			return MD;
		lvl.pokoje.resize(ile);
		for(vector<Pokoj>::iterator it = lvl.pokoje.begin(), end = lvl.pokoje.end(); it != end; ++it)
		{
			if(!ReadRoom(s, *it))
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
		if(	!s.Read((char*)&lvl.schody_gora, sizeof(lvl.schody_gora)) ||
			!s.Read((char*)&lvl.schody_dol, sizeof(lvl.schody_dol)) ||
			!s.ReadCasted<byte>(lvl.schody_gora_dir) ||
			!s.ReadCasted<byte>(lvl.schody_dol_dir) ||
			!s.ReadCasted<byte>(ile))
			return MD;
		lvl.schody_dol_w_scianie = (ile == 1);

		BaseLocation& base = g_base_locations[inside->cel];
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
		if(!ReadUseable(s, **it))
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
		if(!ReadBlood(s, *it))
			return MD;
	}
	// obiekty
	if(!s.Read(ile2))
		return MD;
	local_ctx.objects->resize(ile2);
	for(vector<Object>::iterator it = local_ctx.objects->begin(), end = local_ctx.objects->end(); it != end; ++it)
	{
		if(!ReadObject(s, *it))
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

				if(spell.tex2)
				{
					ParticleEmitter* pe = new ParticleEmitter;
					pe->tex = spell.tex2;
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
					pe->size = spell.size2;
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
		Spell* fireball = FindSpell("fireball");
		for(vector<Explo*>::iterator it = local_ctx.explos->begin(), end = local_ctx.explos->end(); it != end; ++it)
		{
			*it = new Explo;
			Explo& e = **it;
			byte type;
			if(	!s.Read(type) ||
				!ReadStruct(s, e.pos) ||
				!s.Read(e.size) ||
				!s.Read(e.sizemax))
				return MD;

			if(type == 1)
				e.tex = g_spells[10].tex3;
			else
				e.tex = fireball->tex3;
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

	if(IS_SET(unit.data->flagi, F_CZLOWIEK))
		unit.type = Unit::HUMAN;
	else if(IS_SET(unit.data->flagi, F_HUMANOID))
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
	// animacja
	if(unit.IsAlive())
	{
		unit.ani->Play(NAMES::ani_stand, PLAY_PRIO1, 0);
		unit.animacja = unit.animacja2 = ANI_STOI;
	}
	else
	{
		unit.ani->Play(NAMES::ani_die, PLAY_PRIO1, 0);
		unit.animacja = unit.animacja2 = ANI_UMIERA;
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
			!s.Read(unit.hero->kredyt))
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
			!s.Read(unit.player->kredyt) ||
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
	unit.wyjeta = B_BRAK;
	unit.chowana = B_BRAK;
	unit.stan_broni = BRON_SCHOWANA;
	unit.talking = false;
	unit.busy = Unit::Busy_No;
	unit.in_building = -1;
	unit.frozen = 0;
	unit.useable = NULL;
	unit.used_item = NULL;
	unit.bow_instance = NULL;
	unit.ai = NULL;
	unit.animacja = ANI_STOI;
	unit.animacja2 = ANI_STOI;
	unit.timer = 0.f;
	unit.to_remove = false;
	unit.bubble = NULL;
	unit.interp = interpolators.Get();
	unit.interp->Reset(unit.pos, unit.rot);
	unit.visual_pos = unit.pos;
	unit.etap_animacji = 0;

	if(mp_load)
	{
		int useable_netid;
		if( s.Read(unit.netid) &&
			unit.ani->Read(s) &&
			s.ReadCasted<byte>(unit.animacja) &&
			s.ReadCasted<byte>(unit.animacja2) &&
			s.ReadCasted<byte>(unit.etap_animacji) &&
			s.ReadCasted<byte>(unit.attack_id) &&
			s.ReadCasted<byte>(unit.action) &&
			s.ReadCasted<byte>(unit.wyjeta) &&
			s.ReadCasted<byte>(unit.chowana) &&
			s.ReadCasted<byte>(unit.stan_broni) &&
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
	UpdateUnitPhysics(&unit, unit.IsAlive() ? unit.pos : VEC3(1000,1000,1000));

	// muzyka bossa
	if(IS_SET(unit.data->flagi2, F2_BOSS) && !boss_level_mp)
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
bool Game::ReadObject(BitStream& s, Object& obj)
{
	if(!s.Read((char*)&obj.pos, sizeof(obj.pos)) ||
		!s.Read((char*)&obj.rot, sizeof(obj.rot)) ||
		!s.Read(obj.scale))
		return false;
	if(!ReadString1(s))
		return false;
	else if(BUF[0])
	{
		obj.base = FindObject(BUF);
		if(!obj.base)
		{
			ERROR(Format("Missing base object '%s'!", BUF));
			return false;
		}
		obj.mesh = obj.base->ani;
	}
	else if(!ReadString1(s))
		return false;
	else
	{
		obj.mesh = LoadMesh(BUF);
		obj.base = NULL;
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
bool Game::ReadUseable(BitStream& s, Useable& use)
{
	if( !s.Read(use.netid) ||
		!s.Read((char*)&use.pos, sizeof(use.pos)) ||
		!s.Read(use.rot) ||
		!s.ReadCasted<byte>(use.type) ||
		!s.ReadCasted<byte>(use.variant))
		return false;
	use.user = NULL;
	return true;
}

//=================================================================================================
bool Game::ReadBlood(BitStream& s, Blood& blood)
{
	if( !s.ReadCasted<byte>(blood.type) ||
		!s.Read((char*)&blood.pos, sizeof(blood.pos)) ||
		!s.Read((char*)&blood.normal, sizeof(blood.normal)) ||
		!s.Read(blood.rot))
		return false;
	else
	{
		blood.size = 1.f;
		return true;
	}
}

//=================================================================================================
bool Game::ReadLight(BitStream& s, Light& light)
{
	return s.Read((char*)&light.pos, sizeof(light.pos)) &&
		s.Read((char*)&light.color, sizeof(light.color)) &&
		s.Read(light.range);
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
bool Game::ReadRoom(BitStream& s, Pokoj& room)
{
	byte ile;
	if(	!s.Read((char*)&room.pos, sizeof(room.pos)) ||
		!s.Read((char*)&room.size, sizeof(room.size)) ||
		!s.Read(ile))
		return false;
	room.polaczone.resize(ile);
	for(byte i=0; i<ile; ++i)
	{
		if(!s.ReadCasted<byte>(room.polaczone[i]))
			return false;
	}
	if(	!s.ReadCasted<byte>(room.cel) ||
		!s.ReadCasted<byte>(ile))
		return false;
	room.korytarz = (ile == 1);
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
	PlayerController& p = *u.player;
	net_stream2.Write(p.kills);
	net_stream2.Write(p.dmg_done);
	net_stream2.Write(p.dmg_taken);
	net_stream2.Write(p.knocks);
	net_stream2.Write(p.arena_fights);
	p.base_stats.Write(net_stream2);
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
		int flags;
		if(u.atak_w_biegu)
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

	u->player->Init(*u, true);

	if(	!u->stats.Read(s) ||
		!u->unmod_stats.Read(s) ||
		!s.Read(u->gold) ||
		!s.Read(pc->kills) ||
		!s.Read(pc->dmg_done) ||
		!s.Read(pc->dmg_taken) ||
		!s.Read(pc->knocks) ||
		!s.Read(pc->arena_fights) ||
		!pc->base_stats.Read(s))
		return MD;

	u->look_target = NULL;
	u->prev_speed = 0.f;
	u->atak_w_biegu = false;

	int kredyt = u->player->kredyt,
		free_days = u->player->free_days;

	u->weight = 0;
	u->CalculateLoad();
	u->RecalculateWeight();

	u->player->kredyt = kredyt;
	u->player->free_days = free_days;

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
		u->atak_w_biegu = IS_SET(flags, 0x01);
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
		int player_index = FindPlayerIndex(packet->systemAddress);
		if(player_index == -1)
		{
ignore_him:
			LOG(Format("Ignoring packet from %s: %s.", packet->systemAddress.ToString(), PacketToString(packet)));
			continue;
		}

		PlayerInfo& info = game_players[player_index];
		if(info.left)
			goto ignore_him;

		PlayerController* player = info.u->player;

#define NICK_WARN(x) WARN(Format(x, info.name.c_str()))
#define READ_ERROR(x) NICK_WARN("Read error " x " from %s.")
#define CHEAT_ERROR(x) NICK_WARN(x " from %s.")

		switch(packet->data[0])
		{
		case ID_CONNECTION_LOST:
		case ID_DISCONNECTION_NOTIFICATION:
			LOG(Format(packet->data[0] == ID_CONNECTION_LOST ? "Lost connection with player %s." : "Player %s has disconnected.", info.name.c_str()));
			players_left.push_back(info.id);
			info.left = true;
			info.left_reason = 0;
			break;
		case ID_SAY:
			Server_Say(packet, info);
			break;
		case ID_WHISPER:
			Server_Whisper(packet, info);
			break;
		case ID_CONTROL:
			if(packet->length < 3)
				WARN(Format("Broken packet ID_CONTROL from %s: %s.", info.name.c_str(), PacketToString(packet)));
			else
			{
				Unit& u = *info.u;
				BitStream s(packet->data+1, packet->length-1, false);
				ANIMACJA ani;
				byte b;
				s.Read(b);
				if(b == 1)
				{
					if(packet->length < sizeof(float)*5+2)
					{
						WARN(Format("Broken packet ID_CONTROL (2) from %s.", info.name.c_str(), PacketToString(packet)));
						break;
					}
					if(!info.warping && game_state == GS_LEVEL)
					{
						VEC3 new_pos;
						float rot;
						s.Read((char*)&new_pos, sizeof(VEC3));
						s.Read(rot);
						s.Read(u.ani->groups[0].speed);
						if(distance(u.pos, new_pos) >= 10.f)
						{
							WarpUnit(u, u.pos);
							WriteToInterpolator(u.interp, u.pos, rot);
						}
						else
						{
							if(player->noclip || u.useable || CheckMoveNet(&u, new_pos))
							{
								if(!equal(u.pos, new_pos) && !location->outside)
								{
									INT2 new_tile(int(new_pos.x/2), int(new_pos.z/2));
									if(INT2(int(u.pos.x/2), int(u.pos.z/2)) != new_tile)
										DungeonReveal(new_tile);
								}
								u.pos = new_pos;
								UpdateUnitPhysics(&u, u.pos);
								WriteToInterpolator(u.interp, u.pos, rot);
							}
							else
							{
								WriteToInterpolator(u.interp, u.pos, rot);
								NetChangePlayer& c = Add1(net_changes_player);
								c.type = NetChangePlayer::UNSTUCK;
								c.pc = info.u->player;
								c.pos = u.pos;
								info.NeedUpdate();
							}
						}
					}
					else
						SkipBitstream(s, sizeof(VEC3)+sizeof(float)*2);

					s.ReadCasted<byte>(ani);
					if(u.animacja != ANI_ODTWORZ && ani != ANI_ODTWORZ)
						u.animacja = ani;
				}
				byte changes;
				s.Read(changes);
				for(byte change_i=0; change_i<changes; ++change_i)
				{
					NetChange::TYPE type;
					if(!s.ReadCasted<byte>(type))
					{
						READ_ERROR("type");
						break;
					}
					switch(type)
					{
					// zmiana za³o¿onego przedmiotu przez gracza
					case NetChange::CHANGE_EQUIPMENT:
						{
							int i_index;
							if(s.Read(i_index))
							{
								if(i_index >= 0)
								{
									// zak³adanie przedmiotu
									if(uint(i_index) >= u.items.size())
										ERROR(Format("CHANGE_EQUIPMENT: Player %s, invalid index %d.", u.GetName(), i_index));
									else
									{
										ItemSlot& slot = u.items[i_index];
										if(slot.item->IsWearable())
										{
											ITEM_SLOT slot_type = ItemTypeToSlot(slot.item->type);
											if(u.slots[slot_type])
											{
												std::swap(u.slots[slot_type], slot.item);
												SortItems(u.items);
											}
											else
											{
												u.slots[slot_type] = slot.item;
												u.items.erase(u.items.begin()+i_index);
											}

											// przeka¿ dalej
											if(players > 2)
											{
												NetChange& c = Add1(net_changes);
												c.type = NetChange::CHANGE_EQUIPMENT;
												c.unit = &u;
												c.id = slot_type;
											}
										}
										else
											ERROR(Format("CHANGE_EQUIPMENT: Player %s index %d, not wearable item %s.", u.GetName(), i_index, slot.item->id));
									}
								}
								else
								{
									// zdejmowanie przedmiotu
									ITEM_SLOT slot = IIndexToSlot(i_index);
									if(slot < SLOT_WEAPON || slot >= SLOT_MAX)
										ERROR(Format("CHANGE_EQUIPMENT: Player %s, invalid slot type %d.", u.GetName(), slot));
									else if(!u.slots[slot])
										ERROR(Format("CHANGE_EQUIPMENT: Player %s, empty slot type %d.", u.GetName(), slot));
									else
									{
										u.AddItem(u.slots[slot], 1u, false);
										u.weight -= u.slots[slot]->weight;
										u.slots[slot] = NULL;

										// przeka¿ dalej
										if(players > 2)
										{
											NetChange& c = Add1(net_changes);
											c.type = NetChange::CHANGE_EQUIPMENT;
											c.unit = &u;
											c.id = slot;
										}
									}
								}
							}
							else
								READ_ERROR("CHANGE_EQUIPMENT");
						}
						break;
					// chowanie/wyjmowanie broni
					case NetChange::TAKE_WEAPON:
						{
							byte id, co;
							if(s.Read(id) && s.Read(co))
							{
								u.ani->groups[1].speed = 1.f;
								SetUnitWeaponState(u, id==0, (BRON)co);
								if(players > 2)
								{
									NetChange& c = Add1(net_changes);
									c.unit = &u;
									c.type = NetChange::TAKE_WEAPON;
									c.id = id;
								}
							}
							else
								READ_ERROR("TAKE_WEAPON");
						}
						break;
					// atakowanie
					case NetChange::ATTACK:
						{
							byte id;
							float speed;
							if(s.Read(id) && s.Read(speed))
							{
								byte co = (id&0xF);

								// upewnij siê ¿e ma wyjêt¹ broñ
								u.stan_broni = BRON_WYJETA;

								switch(co)
								{
								case AID_Attack:
									if(u.action == A_ATTACK && u.etap_animacji == 0)
									{
										u.attack_power = u.ani->groups[1].time / u.GetAttackFrame(0);
										u.etap_animacji = 1;
										u.ani->groups[1].speed = u.attack_power + u.GetAttackSpeed();
										u.attack_power += 1.f;
									}
									else
									{
										if(sound_volume && u.data->sounds->sound[SOUND_ATTACK] && rand2()%4 == 0)
											PlayAttachedSound(u, u.data->sounds->sound[SOUND_ATTACK], 1.f, 10.f);
										u.action = A_ATTACK;
										u.attack_id = ((id&0xF0)>>4);
										u.attack_power = 1.f;
										u.ani->Play(NAMES::ani_attacks[u.attack_id], PLAY_PRIO1|PLAY_ONCE|PLAY_RESTORE, 1);
										u.ani->groups[1].speed = speed;
										u.etap_animacji = 1;
										u.trafil = false;
									}
									break;
								case AID_PowerAttack:
									{
										if(sound_volume && u.data->sounds->sound[SOUND_ATTACK] && rand2()%4 == 0)
											PlayAttachedSound(u, u.data->sounds->sound[SOUND_ATTACK], 1.f, 10.f);
										u.action = A_ATTACK;
										u.attack_id = ((id&0xF0)>>4);
										u.attack_power = 1.f;
										u.ani->Play(NAMES::ani_attacks[u.attack_id], PLAY_PRIO1|PLAY_ONCE|PLAY_RESTORE, 1);
										u.ani->groups[1].speed = speed;
										u.etap_animacji = 0;
										u.trafil = false;
									}
									break;
								case AID_Shoot:
								case AID_StartShoot:
									if(u.action == A_SHOOT && u.etap_animacji == 0)
										u.etap_animacji = 1;
									else
									{
										u.ani->Play(NAMES::ani_shoot, PLAY_PRIO1|PLAY_ONCE|PLAY_RESTORE, 1);
										u.ani->groups[1].speed = speed;
										u.action = A_SHOOT;
										u.etap_animacji = (co == 2 ? 1 : 0);
										u.trafil = false;
										if(!u.bow_instance)
										{
											if(bow_instances.empty())
												u.bow_instance = new AnimeshInstance(u.GetBow().ani);
											else
											{
												u.bow_instance = bow_instances.back();
												bow_instances.pop_back();
												u.bow_instance->ani = u.GetBow().ani;
											}
										}
										u.bow_instance->Play(&u.bow_instance->ani->anims[0], PLAY_ONCE|PLAY_PRIO1|PLAY_NO_BLEND, 0);
										u.bow_instance->groups[0].speed = u.ani->groups[1].speed;
									}
									if(co == 2)
										s.Read(info.yspeed);
									break;
								case AID_Block:
									{
										u.action = A_BLOCK;
										u.ani->Play(NAMES::ani_block, PLAY_PRIO1|PLAY_STOP_AT_END|PLAY_RESTORE, 1);
										u.ani->groups[1].speed = 1.f;
										u.ani->groups[1].blend_max = speed;
										u.etap_animacji = 0;
									}
									break;
								case AID_Bash:
									{
										u.action = A_BASH;
										u.etap_animacji = 0;
										u.ani->Play(NAMES::ani_bash, PLAY_ONCE|PLAY_PRIO1|PLAY_RESTORE, 1);
										u.ani->groups[1].speed = 2.f;
										u.ani->frame_end_info2 = false;
										u.trafil = false;
									}
									break;
								case AID_RunningAttack:
									{
										if(sound_volume && u.data->sounds->sound[SOUND_ATTACK] && rand2()%4 == 0)
											PlayAttachedSound(u, u.data->sounds->sound[SOUND_ATTACK], 1.f, 10.f);
										u.action = A_ATTACK;
										u.attack_id = ((id&0xF0)>>4);
										u.attack_power = 1.5f;
										u.atak_w_biegu = true;
										u.ani->Play(NAMES::ani_attacks[u.attack_id], PLAY_PRIO1|PLAY_ONCE|PLAY_RESTORE, 1);
										u.ani->groups[1].speed = speed;
										u.etap_animacji = 1;
										u.trafil = false;
									}
									break;
								case AID_StopBlock:
									{
										u.action = A_NONE;
										u.ani->frame_end_info2 = false;
										u.ani->Deactivate(1);
									}
									break;
								}

								if(players > 2)
								{
									NetChange& c = Add1(net_changes);
									c.unit = &u;
									c.type = NetChange::ATTACK;
									c.id = id;
									c.f[1] = speed;
								}
							}
							else
								READ_ERROR("ATTACK");
						}
						break;
					// upuszczanie przedmiotu przez gracza
					case NetChange::DROP_ITEM:
						{
							int i_index, count;
							if(s.Read(i_index) && s.Read(count))
							{
								bool ok = true;
								if(count == 0)
								{
									ok = false;
									ERROR(Format("DROP_ITEM: Player %s, index %d, count %d.", info.name.c_str(), i_index, count));
								}
								else if(i_index >= 0)
								{
									// wyrzucanie nie za³o¿onego przedmiotu
									if(i_index >= (int)u.items.size())
									{
										ok = false;
										ERROR(Format("DROP_ITEM: Player %s, invalid index %d, count %d.", info.name.c_str(), i_index, count));
									}
									else
									{
										ItemSlot& sl = u.items[i_index];
										if(count > (int)sl.count)
										{
											ERROR(Format("DROP_ITEM: Player %d, index %d, count %d, item %s have %d (%d) count.", info.name.c_str(), i_index, count, sl.item->id, sl.count,
												sl.team_count));
											count = sl.count;
										}
										sl.count -= count;
										u.weight -= sl.item->weight*count;
										u.action = A_ANIMATION;
										u.ani->Play("wyrzuca", PLAY_ONCE|PLAY_PRIO2, 0);
										u.ani->frame_end_info = false;
										GroundItem* item = new GroundItem;
										item->item = sl.item;
										item->count = count;
										item->team_count = min(count, (int)sl.team_count);
										sl.team_count -= item->team_count;
										item->pos = u.pos;
										item->pos.x -= sin(u.rot)*0.25f;
										item->pos.z -= cos(u.rot)*0.25f;
										item->rot = random(MAX_ANGLE);
										if(!CheckMoonStone(item, &u))
											AddGroundItem(GetContext(u), item);
										if(sl.count == 0)
											u.items.erase(u.items.begin()+i_index);
									}
								}
								else
								{
									// wyrzucanie za³o¿onego przedmiotu
									ITEM_SLOT slot_type = IIndexToSlot(i_index);
									if(IsValidItemSlot(slot_type))
									{
										const Item*& slot = u.slots[slot_type];
										if(!slot)
										{
											ERROR(Format("DROP_ITEM: Player %s, missing slot item %d.", info.name.c_str(), slot_type));
											ok = false;
										}
										else
										{
											u.weight -= slot->weight*count;
											u.action = A_ANIMATION;
											u.ani->Play("wyrzuca", PLAY_ONCE|PLAY_PRIO2, 0);
											u.ani->frame_end_info = false;
											GroundItem* item = new GroundItem;
											item->item = slot;
											item->count = 1;
											item->team_count = 0;
											item->pos = u.pos;
											item->pos.x -= sin(u.rot)*0.25f;
											item->pos.z -= cos(u.rot)*0.25f;
											item->rot = random(MAX_ANGLE);
											AddGroundItem(GetContext(u), item);
											slot = NULL;

											// poinformuj pozosta³ych o zmianie za³o¿onego przedmiotu
											if(players > 2)
											{
												NetChange& c = Add1(net_changes);
												c.type = NetChange::CHANGE_EQUIPMENT;
												c.unit = &u;
												c.id = slot_type;
											}
										}
									}
									else
									{
										ERROR(Format("DROP_ITEM: Player %s, invalid slot type %d.", info.name.c_str(), slot_type));
										ok = false;
									}
								}

								if(ok && players > 2)
								{
									NetChange& c = Add1(net_changes);
									c.type = NetChange::DROP_ITEM;
									c.unit = &u;
								}
							}
							else
								READ_ERROR("DROP_ITEM");
						}
						break;
					// podnoszenie przedmiotu z ziemi
					case NetChange::PICKUP_ITEM:
						{
							int netid;
							if(s.Read(netid))
							{
								LevelContext* ctx;
								GroundItem* item_ptr = FindItemNetid(netid, &ctx);
								if(item_ptr)
								{
									GroundItem& item = *item_ptr;
									bool u_gory = (item.pos.y > u.pos.y+0.5f);
									u.AddItem(item.item, item.count, item.team_count);
									u.action = A_PICKUP;
									u.animacja = ANI_ODTWORZ;
									u.ani->Play(u_gory ? "podnosi_gora" : "podnosi", PLAY_ONCE|PLAY_PRIO2, 0);
									u.ani->frame_end_info = false;

									NetChangePlayer& c = Add1(net_changes_player);
									c.pc = info.u->player;
									c.type = NetChangePlayer::PICKUP;
									c.id = item.count;
									c.ile = item.team_count;

									info.update_flags |= PlayerInfo::UF_NET_CHANGES;

									NetChange& c2 = Add1(net_changes);
									c2.type = NetChange::REMOVE_ITEM;
									c2.id = item.netid;

									if(players > 2)
									{
										NetChange& c3 = Add1(net_changes);
										c3.type = NetChange::PICKUP_ITEM;
										c3.unit = &u;
										c3.ile = (u_gory ? 1 : 0);
									}

									if(before_player == BP_ITEM && before_player_ptr.item == item_ptr)
										before_player = BP_NONE;
									DeleteElement(*ctx->items, item_ptr);
								}
								else
									WARN(Format("PICKUP_ITEM, missing item %d.", netid));
							}
							else
								READ_ERROR("PICKUP_ITEM");
						}
						break;
					// gracz zjada/wypija coœ
					case NetChange::CONSUME_ITEM:
						{
							int index;
							if(s.Read(index))
								u.ConsumeItem(index);
							else
								READ_ERROR("CONSUME_ITEM");
						}
						break;
					// gracz zaczyna ograbiaæ zw³oki
					case NetChange::LOOT_UNIT:
						{
							int netid;
							if(s.Read(netid))
							{
								Unit* u2 = FindUnit(netid);
								if(u2)
								{
									NetChangePlayer& c = Add1(net_changes_player);
									c.type = NetChangePlayer::LOOT;
									c.pc = info.u->player;
									if(u2->busy == Unit::Busy_Looted)
										c.id = 0;
									else
									{
										c.id = 1;
										u2->busy = Unit::Busy_Looted;
										player->action = PlayerController::Action_LootUnit;
										player->action_unit = u2;
										player->chest_trade = &u2->items;
									}
									info.NeedUpdate();
								}
								else
									WARN(Format("LOOT_UNIT, missing unit %d.", netid));
							}
							else
								READ_ERROR("LOOT_UNIT");
						}
						break;
					// gracz zaczyna ograbiaæ skrzyniê
					case NetChange::LOOT_CHEST:
						{
							int netid;
							if(s.Read(netid))
							{
								Chest* chest = FindChest(netid);
								if(chest)
								{
									NetChangePlayer& c = Add1(net_changes_player);
									c.type = NetChangePlayer::LOOT;
									c.pc = info.u->player;
									if(chest->looted)
										c.id = 0;
									else
									{
										c.id = 1;
										chest->looted = true;
										player->action = PlayerController::Action_LootChest;
										player->action_chest = chest;
										player->chest_trade = &chest->items;

										NetChange& c2 = Add1(net_changes);
										c2.type = NetChange::CHEST_OPEN;
										c2.id = chest->netid;

										// animacja / dŸwiêk
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
								else
									WARN(Format("LOOT_CHEST, missing chest %d.", netid));
							}
							else
								READ_ERROR("LOOT_CHEST");
						}
						break;
					// zabieranie przedmiotu ze skrzyni/zw³ok/od innej postaci
					case NetChange::GET_ITEM:
						{
							int i_index, count;
							if(!s.Read(i_index) || !s.Read(count))
							{
								READ_ERROR("GET_ITEM");
								break;
							}

							if(!PlayerController::IsTrade(player->action))
							{
								ERROR(Format("GET_ITEM: Player %s is not trading (%d, index %d).", u.GetName(), player->action, i_index));
								break;
							}

							if(i_index >= 0)
							{
								if(i_index < (int)player->chest_trade->size())
								{
									ItemSlot& slot = player->chest_trade->at(i_index);
									if(count < 1 || count > (int)slot.count)
									{
										ERROR(Format("GET_ITEM: Player %s take item %d, count %d.", u.GetName(), i_index, count));
										break;
									}

									if(slot.item->type == IT_GOLD)
									{
										uint team_count = min(slot.team_count, (uint)count);
										if(team_count == 0)
										{
											u.gold += slot.count;
											info.UpdateGold();
										}
										else
										{
											AddGold(team_count);
											uint ile = slot.count - team_count;
											if(ile)
											{
												u.gold += ile;
												info.UpdateGold();
											}
										}
										slot.count -= count;
										if(slot.count == 0)
											player->chest_trade->erase(player->chest_trade->begin()+i_index);
										else
											slot.team_count -= team_count;
									}
									else
									{
										uint team_count = (player->action == PlayerController::Action_Trade ? 0 : min((uint)count, slot.team_count));
										AddItem(u, slot.item, (uint)count, team_count, false);
										if(player->action == PlayerController::Action_Trade)
											u.gold -= slot.item->value*count;
										else if(player->action == PlayerController::Action_ShareItems && slot.item->type == IT_CONSUMEABLE && slot.item->ToConsumeable().effect == E_HEAL)
											player->action_unit->ai->have_potion = 1;
										if(player->action != PlayerController::Action_LootChest)
										{
											Unit* t = player->action_unit;
											t->weight -= slot.item->weight*count;
											if(player->action == PlayerController::Action_LootUnit && slot.item == t->used_item)
											{
												t->used_item = NULL;
												if(players > 2)
												{
													NetChange& c = Add1(net_changes);
													c.type = NetChange::REMOVE_USED_ITEM;
													c.unit = t;
												}
											}
										}
										slot.count -= count;
										if(slot.count == 0)
											player->chest_trade->erase(player->chest_trade->begin()+i_index);
										else
											slot.team_count -= team_count;
									}
								}
								else
									ERROR(Format("GET_ITEM: Player %s get item %d (max %u).", u.GetName(), i_index, player->chest_trade->size()));
							}
							else
							{
								if(player->action != PlayerController::Action_LootChest)
								{
									ITEM_SLOT type = IIndexToSlot(i_index);
									if(type >= SLOT_WEAPON && type < SLOT_MAX)
									{
										Unit* t = player->action_unit;
										const Item*& slot = t->slots[type];
										// dodaj graczowi
										AddItem(u, slot, 1u, 1u, false);
										// usuñ przedmiot postaci
										if(player->action == PlayerController::Action_LootUnit && type == IT_WEAPON && slot == t->used_item)
										{
											t->used_item = NULL;
											if(players > 2)
											{
												NetChange& c = Add1(net_changes);
												c.type = NetChange::REMOVE_USED_ITEM;
												c.unit = t;
											}
										}
										t->weight -= slot->weight;
										slot = NULL;
										// komunikat o zmianie za³o¿onego ekwipunku
										if(players > 2)
										{
											NetChange& c = Add1(net_changes);
											c.type = NetChange::CHANGE_EQUIPMENT;
											c.unit = player->action_unit;
											c.id = type;
										}
									}
									else
										ERROR(Format("GET_ITEM: Invalid slot type %d.", type));
								}
								else
									ERROR(Format("GET_ITEM: Player %s get slot item %d from chest %d.", u.GetName(), i_index, player->action_chest->netid));
							}
						}
						break;
					// chowanie przedmiotu do skrzyni/zw³ok/dawanie innej postaci
					case NetChange::PUT_ITEM:
						{
							int i_index, _count;
							if(!s.Read(i_index) || !s.Read(_count))
							{
								READ_ERROR("PUT_ITEM");
								break;
							}

							const uint count = (uint)_count;

							if(!PlayerController::IsTrade(player->action))
							{
								ERROR(Format("PUT_ITEM: Player %s is not trading (%d, index %d).", u.GetName(), player->action, i_index));
								break;
							}

							if(i_index >= 0)
							{
								if(i_index < (int)u.items.size())
								{
									ItemSlot& slot = u.items[i_index];
									if(count < 1 || count > slot.count)
									{
										ERROR(Format("PUT_ITEM: Player %s put item %d, count %u.", u.GetName(), i_index, count));
										break;
									}

									uint team_count = min(count, slot.team_count);

									// dodaj przedmiot
									if(player->action == PlayerController::Action_LootChest)
										AddItem(*player->action_chest, slot.item, count, team_count, false);
									else if(player->action == PlayerController::Action_Trade)
									{
										InsertItem(*player->chest_trade, slot.item, count, team_count);
										if(team_count)
											AddGold((slot.item->value*team_count)/2);
										uint normal_count = count - team_count;
										if(normal_count)
										{
											u.gold += (slot.item->value*normal_count)/2;
											info.UpdateGold();
										}
									}
									else
									{
										Unit* t = player->action_unit;
										uint add_as_team = team_count;
										if(player->action == PlayerController::Action_ShareItems)
										{
											if(slot.item->type == IT_CONSUMEABLE && slot.item->ToConsumeable().effect == E_HEAL)
												t->ai->have_potion = 2;
										}
										else if(player->action == PlayerController::Action_GiveItems)
										{
											add_as_team = 0;
											int value = slot.item->value/2;
											if(t->gold >= value)
											{
												t->gold -= value;
												u.gold += value;
											}
											if(slot.item->type == IT_CONSUMEABLE && slot.item->ToConsumeable().effect == E_HEAL)
												t->ai->have_potion = 2;
										}
										AddItem(*t, slot.item, count, add_as_team, false);
										if(player->action == PlayerController::Action_ShareItems || player->action == PlayerController::Action_GiveItems)
										{
											if(slot.item->type == IT_CONSUMEABLE && t->ai->have_potion == 0)
												t->ai->have_potion = 1;
											if(player->action == PlayerController::Action_GiveItems)
											{
												UpdateUnitInventory(*t);
												NetChangePlayer& c = Add1(net_changes_player);
												c.type = NetChangePlayer::UPDATE_TRADER_INVENTORY;
												c.pc = player;
												c.unit = t;
												GetPlayerInfo(player).NeedUpdate();
											}
										}
									}

									// usuñ przedmiot
									u.weight -= slot.item->weight*count;
									slot.count -= count;
									if(slot.count == 0)
										u.items.erase(u.items.begin()+i_index);
									else
										slot.team_count -= team_count;
								}
								else
									ERROR(Format("PUT_ITEM: Player %s put item %d (max %u).", u.GetName(), i_index, player->chest_trade->size()));
							}
							else
							{
								ITEM_SLOT type = IIndexToSlot(i_index);
								if(type >= SLOT_WEAPON && type < SLOT_MAX)
								{
									const Item*& slot = u.slots[type];
									// dodaj nowy przedmiot
									if(player->action == PlayerController::Action_LootChest)
										AddItem(*player->action_chest, slot, 1u, 0u, false);
									else if(player->action == PlayerController::Action_Trade)
									{
										InsertItem(*player->chest_trade, slot, 1u, 0u);
										u.gold += slot->value/2;
									}
									else
									{
										Unit* t = player->action_unit;
										AddItem(*t, slot, 1u, 0u, false);
										if(player->action == PlayerController::Action_GiveItems)
										{
											int value = slot->value/2;
											if(t->gold >= value)
											{
												// sprzeda³ za z³oto
												t->gold -= value;
												u.gold += value;
											}
											UpdateUnitInventory(*t);
											NetChangePlayer& c = Add1(net_changes_player);
											c.type = NetChangePlayer::UPDATE_TRADER_INVENTORY;
											c.pc = player;
											c.id = t->netid;
											GetPlayerInfo(player).NeedUpdate();
										}
									}
									// usuñ za³o¿ony
									u.weight -= slot->weight;
									slot = NULL;
									// komunikat o zmianie za³o¿onych przedmiotów
									if(players > 2)
									{
										NetChange& c = Add1(net_changes);
										c.type = NetChange::CHANGE_EQUIPMENT;
										c.unit = &u;
										c.id = type;
									}
								}
								else
									ERROR(Format("PUT_ITEM: Invalid slot type %d.", type));
							}
						}
						break;
					// zabieranie wszystkich przedmiotów przez gracza ze skrzyni/zw³ok
					case NetChange::GET_ALL_ITEMS:
						{
							// przycisk - zabierz wszystko
							bool changes = false;

							// sloty
							if(player->action != PlayerController::Action_LootChest)
							{
								const Item** slots = player->action_unit->slots;
								for(int i=0; i<SLOT_MAX; ++i)
								{
									if(slots[i])
									{
										InsertItemBare(u.items, slots[i]);
										slots[i] = NULL;

										if(players > 2)
										{
											NetChange& c = Add1(net_changes);
											c.type = NetChange::CHANGE_EQUIPMENT;
											c.unit = player->action_unit;
											c.id = i;
										}

										changes = true;
									}
								}

								// wyzeruj wagê ekwipunku
								player->action_unit->weight = 0;
							}

							// zwyk³e przedmioty
							for(vector<ItemSlot>::iterator it = player->chest_trade->begin(), end = player->chest_trade->end(); it != end; ++it)
							{
								if(!it->item)
									continue;

								if(it->item->type == IT_GOLD)
									u.AddItem(&gold_item, it->count, it->team_count);
								else
								{
									InsertItemBare(u.items, it->item, it->count, it->team_count);
									changes = true;
								}
							}
							player->chest_trade->clear();

							if(changes)
								SortItems(u.items);
						}
						break;
					// przerywanie trybu wymiany przez gracza
					case NetChange::STOP_TRADE:
						if(player->action == PlayerController::Action_LootChest)
						{
							player->action_chest->looted = false;
							player->action_chest->ani->Play(&player->action_chest->ani->ani->anims[0], PLAY_PRIO1|PLAY_ONCE|PLAY_STOP_AT_END|PLAY_BACK, 0);
							if(sound_volume)
							{
								VEC3 pos = player->action_chest->pos;
								pos.y += 0.5f;
								PlaySound3d(sChestClose, pos, 2.f, 5.f);
							}
							NetChange& c = Add1(net_changes);
							c.type = NetChange::CHEST_CLOSE;
							c.id = player->action_chest->netid;
						}
						else
						{
							player->action_unit->busy = Unit::Busy_No;
							player->action_unit->look_target = NULL;
						}
						player->action = PlayerController::Action_None;
						if(player->dialog_ctx->next_talker)
						{
							Unit* t = player->dialog_ctx->next_talker;
							player->dialog_ctx->next_talker = NULL;
							t->auto_talk = 0;
							StartDialog(*player->dialog_ctx, t, player->dialog_ctx->next_dialog);
						}
						break;
					// zabieranie przedmiotu na kredyt przez gracza
					case NetChange::TAKE_ITEM_CREDIT:
						{
							byte id;
							if(s.Read(id))
							{
								u.items[id].team_count = 0;
								player->kredyt += u.items[id].item->value/2;
								CheckCredit(true);
							}
							else
								READ_ERROR("TAKE_ITEM_CREDIT");
						}
						break;
					// animacja idle gracza
					case NetChange::IDLE:
						{
							byte id;
							if(s.Read(id))
							{
								u.ani->Play(u.data->idles[id], PLAY_ONCE, 0);
								u.ani->frame_end_info = false;
								u.animacja = ANI_IDLE;
								if(players > 2)
								{
									NetChange& c = Add1(net_changes);
									c.type = NetChange::IDLE;
									c.unit = &u;
									c.id = id;
								}
							}
							else
								READ_ERROR("IDLE");
						}
						break;
					// rozmowa gracza z postaci¹
					case NetChange::TALK:
						{
							int netid;
							if(s.Read(netid))
							{
								Unit* u2 = FindUnit(netid);
								if(u2)
								{
									NetChangePlayer& c = Add1(net_changes_player);
									c.pc = info.u->player;
									c.type = NetChangePlayer::START_DIALOG;
									if(u2->busy != Unit::Busy_No || !u2->CanTalk())
										c.id = -1;
									else
									{
										c.id = u2->netid;
										u2->auto_talk = 0;
										StartDialog(*player->dialog_ctx, u2);
									}
									info.NeedUpdate();
								}
								else
									WARN(Format("TALK, missing unit %d.", netid));
							}
							else
								READ_ERROR("TALK");
						}
						break;
					// wybranie opcji dialogowej przez gracza
					case NetChange::CHOICE:
						{
							byte id;
							if(s.Read(id))
								info.u->player->dialog_ctx->choice_selected = id;
							else
								READ_ERROR("CHOICE");
						}
						break;
					// pomijanie dialogu przez gracza
					case NetChange::SKIP_DIALOG:
						{
							int skip_id;
							if(s.Read(skip_id))
							{
								DialogContext& ctx = *info.u->player->dialog_ctx;
								if(ctx.dialog_wait > 0.f && ctx.dialog_mode && !ctx.show_choices && ctx.skip_id == skip_id)
									ctx.choice_selected = 1;
							}
							else
								READ_ERROR("SKIP_DIALOG");
						}
						break;
					// wchodzenie do budynku
					case NetChange::ENTER_BUILDING:
						{
							byte gdzie;
							if(s.Read(gdzie))
							{
								WarpData& warp = Add1(mp_warps);
								warp.u = &u;
								warp.where = gdzie;
								warp.timer = 1.f;
								u.frozen = 2;
							}
							else
								READ_ERROR("ENTER_BUILDING");
						}
						break;
					// wychodzenie z budynku
					case NetChange::EXIT_BUILDING:
						{
							WarpData& warp = Add1(mp_warps);
							warp.u = &u;
							warp.where = -1;
							warp.timer = 1.f;
							u.frozen = 2;
						}
						break;
					// odsy³a informacje o przeniesieniu
					case NetChange::WARP:
						info.warping = false;
						break;
					// gracz doda³ notatkê do dziennika
					case NetChange::ADD_NOTE:
						{
							string& str = Add1(info.notes);
							if(!ReadString1(s, str))
							{
								info.notes.pop_back();
								READ_ERROR("ADD_NOTE");
							}
						}
						break;
					// losowanie liczby przez gracza
					case NetChange::RANDOM_NUMBER:
						{
							byte co;
							if(s.Read(co))
							{
								AddMsg(Format(txRolledNumber, info.name.c_str(), co));
								if(players > 2)
								{
									NetChange& c = Add1(net_changes);
									c.type = NetChange::RANDOM_NUMBER;
									c.unit = info.u;
									c.id = co;
								}
							}
							else
								READ_ERROR("RANDOM_NUMBER");
						}
						break;
					// u¿ywanie u¿ywalnego obiektu
					case NetChange::USE_USEABLE:
						{
							int netid;
							byte co;
							if(s.Read(netid) && s.Read(co))
							{
								Useable* use = FindUseable(netid);
								if(co == 1)
								{
									if(!use || use->user)
									{
										NetChangePlayer& c = Add1(net_changes_player);
										c.type = NetChangePlayer::USE_USEABLE;
										c.pc = info.u->player;
										info.NeedUpdate();
									}
									else
									{
										Unit& u = *info.u;

										BaseUsable& base = g_base_usables[use->type];

										u.action = A_ANIMATION2;
										u.animacja = ANI_ODTWORZ;
										u.ani->Play(base.anim, PLAY_PRIO1, 0);
										u.useable = use;
										u.target_pos = u.pos;
										u.target_pos2 = use->pos;
										if(g_base_usables[use->type].limit_rot == 4)
											u.target_pos2 -= VEC3(sin(use->rot)*1.5f,0,cos(use->rot)*1.5f);
										u.timer = 0.f;
										u.etap_animacji = 0;
										u.use_rot = lookat_angle(u.pos, u.useable->pos);
										u.used_item = base.item;
										if(u.used_item)
										{
											u.wyjeta = B_BRAK;
											u.stan_broni = BRON_SCHOWANA;
										}
										use->user = &u;

										NetChange& c = Add1(net_changes);
										c.type = NetChange::USE_USEABLE;
										c.unit = info.u;
										c.id = netid;
										c.ile = co;
									}
								}
								else if(use && use->user == info.u)
								{
									u.action = A_NONE;
									u.animacja = ANI_STOI;
									use->user = NULL;
									if(u.live_state == Unit::ALIVE)
										u.used_item = NULL;

									if(players > 2)
									{
										NetChange& c = Add1(net_changes);
										c.type = NetChange::USE_USEABLE;
										c.unit = info.u;
										c.id = netid;
										c.ile = co;
									}
								}
							}
							else
								READ_ERROR("USE_USEABLE");
						}
						break;
					// gracz u¿y³ kodu 'suicide'
					case NetChange::CHEAT_SUICIDE:
						if(info.cheats)
							GiveDmg(GetContext(*info.u), NULL, info.u->hpmax, *info.u);
						else
							CHEAT_ERROR("CHEAT_SUICIDE");
						break;
					// gracz u¿y³ kodu 'godmode'
					case NetChange::CHEAT_GODMODE:
						{
							byte b;
							if(s.Read(b))
							{
								if(info.cheats)
									info.u->player->godmode = (b == 1);
								else
									CHEAT_ERROR("CHEAT_GODMODE");
							}
							else
								READ_ERROR("CHEAT_GODMODE");
						}
						break;
					// wstawanie postaci z ziemi
					case NetChange::STAND_UP:
						{
							Unit* u = info.u;
							UnitStandup(*u);
							if(players > 2)
							{
								NetChange& c = Add1(net_changes);
								c.type = NetChange::STAND_UP;
								c.unit = u;
							}
						}
						break;
					// gracz u¿y³ kodu 'noclip'
					case NetChange::CHEAT_NOCLIP:
						{
							byte b;
							if(s.Read(b))
							{
								if(info.cheats)
									info.u->player->noclip = (b == 1);
								else
									CHEAT_ERROR("CHEAT_NOCLIP");
							}
							else
								READ_ERROR("CHEAT_NOCLIP");
						}
						break;
					// gracz u¿y³ kodu 'invisible'
					case NetChange::CHEAT_INVISIBLE:
						{
							byte b;
							if(s.Read(b))
							{
								if(info.cheats)
									info.u->invisible = (b == 1);
								else
									CHEAT_ERROR("CHEAT_INVISIBLE");
							}
							else
								READ_ERROR("CHEAT_INVISIBLE");
						}
						break;
					// gracz u¿y³ kodu 'scare'
					case NetChange::CHEAT_SCARE:
						if(info.cheats)
						{
							for(vector<AIController*>::iterator it = ais.begin(), end = ais.end(); it != end; ++it)
							{
								if(IsEnemy(*(*it)->unit, *info.u) && distance((*it)->unit->pos, info.u->pos) < alert_range.x && CanSee(*(*it)->unit, *info.u))
									(*it)->morale = -10;
							}
						}
						else
							CHEAT_ERROR("CHEAT_SCARE");
						break;
					// gracz u¿y³ kodu 'killall'
					case NetChange::CHEAT_KILL_ALL:
						{
							byte co;
							if(s.Read(co))
							{
								Unit* ignore = NULL;
								if(co == 3)
								{
									int netid;
									if(!s.Read(netid))
									{
										READ_ERROR("CHEAT_KILL_ALL(2)");
										break;
									}
									if(netid != -1)
									{
										ignore = FindUnit(netid);
										if(!ignore)
										{
											WARN(Format("CHEAT_KILL_ALL, missing unit %d, from %s.", netid, info.name.c_str()));
											break;
										}
									}
								}
								if(info.cheats)
								{
									if(!Cheat_KillAll(int(co), *info.u, ignore))
										WARN(Format("CHEAT_KILL_ERROR, unknown parameter %d.", int(co)));
								}
								else
									CHEAT_ERROR("CHEAT_KILL_ALL");
							}
							else
								READ_ERROR("CHEAT_KILL_ALL");
						}
						break;
					// sprawdza czy podany przedmiot jest lepszy dla postaci z która dokonuje wymiany
					case NetChange::IS_BETTER_ITEM:
						{
							int i_index;
							if(s.Read(i_index))
							{
								byte b = 0;
								if(info.u->player->action == PlayerController::Action_GiveItems)
								{
									const Item* item = info.u->GetIIndexItem(i_index);
									if(item)
									{
										if(IsBetterItem(*info.u->player->action_unit, item))
											b = 1;
									}
									else
										ERROR(Format("IS_BETTER_ITEM: Invalid i_index %d.", i_index));
								}
								else
									ERROR(Format("IS_BETTER_ITEM: Player %s not giving items.", info.u->GetName()));
								NetChangePlayer& c = Add1(net_changes_player);
								c.type = NetChangePlayer::IS_BETTER_ITEM;
								c.id = b;
								c.pc = info.u->player;
								info.NeedUpdate();
							}
							else
								READ_ERROR("IS_BETTER_ITEM");
						}
						break;
					// gracz u¿y³ kodu 'citzen'
					case NetChange::CHEAT_CITZEN:
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
							CHEAT_ERROR("CHEAT_CITZEN");
						break;
					// gracz u¿y³ kodu 'heal'
					case NetChange::CHEAT_HEAL:
						if(info.cheats)
						{
							info.u->hp = info.u->hpmax;
							NetChange& c = Add1(net_changes);
							c.type = NetChange::UPDATE_HP;
							c.unit = info.u;
						}
						else
							CHEAT_ERROR("CHEAT_HEAL");
						break;
					// gracz u¿y³ kodu 'kill'
					case NetChange::CHEAT_KILL:
						{
							int netid;
							if(s.Read(netid))
							{
								if(info.cheats)
								{
									Unit* u = FindUnit(netid);
									GiveDmg(GetContext(*u), NULL, u->hpmax, *u);
								}
								else
									CHEAT_ERROR("CHEAT_KILL");
							}
							else
								READ_ERROR("CHEAT_KILL");
						}
						break;
					// gracz u¿y³ kodu 'healunit'
					case NetChange::CHEAT_HEAL_UNIT:
						{
							int netid;
							if(s.Read(netid))
							{
								if(info.cheats)
								{
									Unit* u = FindUnit(netid);
									u->hp = u->hpmax;
									NetChange& c = Add1(net_changes);
									c.type = NetChange::UPDATE_HP;
									c.unit = u;
								}
								else
									CHEAT_ERROR("CHEAT_HEAL_UNIT");
							}
							else
								READ_ERROR("CHEAT_HEAL_UNIT");
						}
						break;
					// gracz u¿y³ kodu 'reveal'
					case NetChange::CHEAT_REVEAL:
						if(info.cheats)
							Cheat_Reveal();
						else
							CHEAT_ERROR("CHEAT_REVEAL");
						break;
					// gracz u¿y³ kodu 'goto_map'
					case NetChange::CHEAT_GOTO_MAP:
						if(info.cheats)
						{
							ExitToMap();
							peer->DeallocatePacket(packet);
							return;
						}
						else
							CHEAT_ERROR("CHEAT_GOTO_MAP");
						break;
					// gracz u¿y³ kodu 'show_minimap'
					case NetChange::CHEAT_SHOW_MINIMAP:
						if(info.cheats)
							Cheat_ShowMinimap();
						else
							CHEAT_ERROR("CHEAT_SHOW_MINIMAP");
						break;
					// gracz u¿y³ kodu 'addgold'
					case NetChange::CHEAT_ADD_GOLD:
						{
							int ile;
							if(s.Read(ile))
							{
								if(info.cheats)
								{
									info.u->gold = max(info.u->gold+ile, 0);
									info.update_flags |= PlayerInfo::UF_GOLD;
								}
								else
									CHEAT_ERROR("CHEAT_ADD_GOLD");
							}
							else
								READ_ERROR("CHEAT_ADD_GOLD");
						}
						break;
					// gracz u¿y³ kodu 'addgold_team'
					case NetChange::CHEAT_ADD_GOLD_TEAM:
						{
							int ile;
							if(s.Read(ile))
							{
								if(info.cheats)
								{
									if(ile < 0)
										WARN(Format("CHEAT_ADD_GOLD_TEAM, %s wants to add %d gold pieces.", info.name.c_str(), ile));
									else
										AddGold(ile);
								}
								else
									CHEAT_ERROR("CHEAT_ADD_GOLD_TEAM");
							}
							else
								READ_ERROR("CHEAT_ADD_GOLD_TEAM");
						}
						break;
					// gracz u¿y³ kodu 'additem'
					case NetChange::CHEAT_ADD_ITEM:
						{
							byte ile, is_team;
							if(ReadString1(s) && s.Read(ile) && s.Read(is_team))
							{
								if(info.cheats)
								{
									const Item* item = FindItem(BUF);
									if(item)
										AddItem(*info.u, item, (uint)ile, is_team == 1);
									else
										WARN(Format("CHEAT_ADD_ITEM, missing item %s (%d,%d).", BUF, ile, is_team, info.name.c_str()));
								}
								else
									CHEAT_ERROR("CHEAT_ADD_ITEM");
							}
							else
								READ_ERROR("CHEAT_ADD_ITEM");
						}
						break;
					// gracz u¿y³ kodu 'skip_days'
					case NetChange::CHEAT_SKIP_DAYS:
						{
							int ile;
							if(s.Read(ile))
							{
								if(info.cheats)
									WorldProgress(ile, WPM_SKIP);
								else
									CHEAT_ERROR("CHEAT_SKIP_DAYS");
							}
							else
								READ_ERROR("CHEAT_SKIP_DAYS");
						}
						break;
					// gracz u¿y³ kodu 'warp'
					case NetChange::CHEAT_WARP_TO_BUILDING:
						{
							byte gdzie;
							if(s.Read(gdzie))
							{
								if(info.cheats)
								{
									if(u.frozen == 0)
									{
										int id;
										InsideBuilding* building;
										if(gdzie == B_INN)
											building = city_ctx->FindInn(id);
										else
											building = city_ctx->FindInsideBuilding((BUILDING)gdzie, id);
										if(building)
										{
											WarpData& warp = Add1(mp_warps);
											warp.u = &u;
											warp.where = id;
											warp.timer = 1.f;
											u.frozen = 2;
											if(IsLeader(u))
											{
												for(vector<Unit*>::iterator it = team.begin(), end = team.end(); it != end; ++it)
												{
													if((*it)->CanFollow())
													{
														(*it)->frozen = 2;
														(*it)->hero->following = &u;
													}
												}
											}

											Net_PrepareWarp(info.u->player);
										}
										else
											WARN(Format("CHEAT_WARP_TO_BUILDING, no place %d from %s.", gdzie, info.name.c_str()));
									}
									else
										NICK_WARN("CHEAT_WARP_TO_BUILDING from frozen player %s.");
								}
								else
									CHEAT_ERROR("CHEAT_WARP_TO_BUILDING");
							}
							else
								READ_ERROR("CHEAT_WARP_TO_BUILDING");
						}
						break;
					// gracz u¿y³ kodu 'spawn_unit'
					case NetChange::CHEAT_SPAWN_UNIT:
						{
							byte ile;
							char level, in_arena;
							if(ReadString1(s) && s.Read(ile) && s.Read(level) && s.Read(in_arena))
							{
								if(info.cheats)
								{
									UnitData* data = FindUnitData(BUF, false);
									if(data)
									{
										if(in_arena < -1 || in_arena > 1)
											in_arena = -1;

										LevelContext& ctx = GetContext(*info.u);

										for(byte i=0; i<ile; ++i)
										{
											Unit* u = SpawnUnitNearLocation(ctx, info.u->GetFrontPos(), *data, &info.u->pos, level);
											if(!u)
											{
												WARN(Format("CHEAT_SPAWN_UNIT: No free space for unit '%s'!", data->id));
												break;
											}
											else if(in_arena != -1)
											{
												u->in_arena = in_arena;
												at_arena.push_back(u);
											}
											if(IsOnline())
												Net_SpawnUnit(u);
										}
									}
									else
										WARN(Format("CHEAT_SPAWN_UNIT, missing base unit %s from %s.", BUF, info.name.c_str()));
								}
								else
									CHEAT_ERROR("CHEAT_SPAWN_UNIT");
							}
							else
								READ_ERROR("CHEAT_SPAWN_UNIT");
						}
						break;
					// gracz u¿y³ kodu 'modstat'
					case NetChange::CHEAT_MOD_STAT:
						{
							byte co, skill;
							char ile;
							if(s.Read(co) && s.Read(skill) && s.Read(ile))
							{
								if(info.cheats)
								{
									if(skill == 1)
									{
										if(co < (int)Skill::MAX)
										{
											int v = info.u->unmod_stats.skill[co];
											v = clamp(v+ile, 0, SkillInfo::MAX);
											if(v != info.u->unmod_stats.skill[co])
											{
												info.u->Set((Skill)co, v);
												NetChangePlayer& c = AddChange(NetChangePlayer::STAT_CHANGED, info.pc);
												c.id = (int)ChangedStatType::SKILL;
												c.a = co;
												c.ile = v;
											}
										}
										else
											WARN(Format("CHEAT_MOD_STAT from %s: invalid skill (%d,%d).", info.name.c_str(), co, ile));
									}
									else
									{
										if(co < (int)Attribute::MAX)
										{
											int v = info.u->unmod_stats.attrib[co];
											v = clamp(v+ile, 1, AttributeInfo::MAX);
											if(v != info.u->unmod_stats.attrib[co])
											{
												info.u->Set((Attribute)co, v);
												NetChangePlayer& c = AddChange(NetChangePlayer::STAT_CHANGED, info.pc);
												c.id = (int)ChangedStatType::ATTRIBUTE;
												c.a = co;
												c.ile = v;												
											}
										}
										else
											WARN(Format("CHEAT_MOD_STAT from %s: invalid attribute (%d,%d).", info.name.c_str(), co, ile));
									}
								}
								else
									CHEAT_ERROR("CHEAT_MOD_STAT");
							}
							else
								READ_ERROR("CHEAT_MOD_STAT");
						}
						break;
					// odpowiedŸ gracza na wyzwanie go na pvp
					case NetChange::PVP:
						{
							byte co;
							if(s.Read(co))
							{
								if(pvp_response.ok && pvp_response.to == info.u)
								{
									if(co == 1)
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
							else
								READ_ERROR("PVP");
						}
						break;
					// lider chce opuœciæ lokacjê
					case NetChange::LEAVE_LOCATION:
						{
							char gdzie;
							if(s.Read(gdzie))
							{
								int w = CanLeaveLocation(*info.u);
								if(w == 0)
								{
									PushNetChange(NetChange::LEAVE_LOCATION);
									if(gdzie == WHERE_OUTSIDE)
										fallback_co = FALLBACK_EXIT;
									else if(gdzie == WHERE_LEVEL_UP)
									{
										fallback_co = FALLBACK_CHANGE_LEVEL;
										fallback_1 = -1;
									}
									else if(gdzie == WHERE_LEVEL_DOWN)
									{
										fallback_co = FALLBACK_CHANGE_LEVEL;
										fallback_1 = +1;
									}
									else
									{
										fallback_co = FALLBACK_USE_PORTAL;
										fallback_1 = gdzie;
									}
									fallback_t = -1.f;
									for(vector<Unit*>::iterator it = team.begin(), end = team.end(); it != end; ++it)
										(*it)->frozen = 2;
								}
								else
								{
									NetChangePlayer& c = Add1(net_changes_player);
									c.type = NetChangePlayer::CANT_LEAVE_LOCATION;
									c.pc = info.u->player;
									c.id = w;
									GetPlayerInfo(c.pc).NeedUpdate();
								}
							}
							else
								READ_ERROR("LEAVE_LOCATION");
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
									WARN(Format("USE_DOOR, missing doors %d.", netid));
							}
							else
								READ_ERROR("USE_DOOR");
						}
						break;
					// podró¿ do innej lokacji
					case NetChange::TRAVEL:
						{
							byte loc;
							if(s.Read(loc))
							{
								if(IsLeader(info.u))
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

									NetChange& c = Add1(net_changes);
									c.type = NetChange::TRAVEL;
									c.id = loc;
								}
								else
									WARN(Format("TRAVEL from %s who is not a leader.", info.name.c_str()));
							}
							else
								READ_ERROR("TRAVEL");
						}
						break;
					// wejœcie do lokacji
					case NetChange::ENTER_LOCATION:
						if(game_state == GS_WORLDMAP && world_state == WS_MAIN && IsLeader(info.u))
						{
							EnterLocation();
							peer->DeallocatePacket(packet);
							return;
						}
						break;
					// trenowanie przez chodzenie
					case NetChange::TRAIN_MOVE:
						info.u->player->TrainMove(1.f);
						break;
					// zamykanie tekstu spotkania
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
						peer->DeallocatePacket(packet);
						return;
					// gracz u¿y³ kodu na zmianê poziomu (<>+shift+ctrl)
					case NetChange::CHEAT_CHANGE_LEVEL:
						{
							byte where;
							if(s.Read(where))
							{
								if(info.cheats)
								{
									peer->DeallocatePacket(packet);
									if(where == 0)
										ChangeLevel(-1);
									else
										ChangeLevel(+1);
									return;
								}
								else
									CHEAT_ERROR("CHEAT_CHANGE_LEVEL");
							}
							else
								READ_ERROR("CHEAT_CHANGE_LEVEL");
						}
						break;
					// gracz u¿y³ kodu na przeniesienie do schodów (<>+shift)
					case NetChange::CHEAT_WARP_TO_STAIRS:
						{
							byte where;
							if(s.Read(where))
							{
								if(info.cheats)
								{
									InsideLocation* inside = (InsideLocation*)location;
									InsideLocationLevel& lvl = inside->GetLevelData();

									if(where == 0)
									{
										INT2 tile = lvl.GetUpStairsFrontTile();
										info.u->rot = dir_to_rot(lvl.schody_gora_dir);
										WarpUnit(*info.u, VEC3(2.f*tile.x+1.f, 0.f, 2.f*tile.y+1.f));
									}
									else
									{
										INT2 tile = lvl.GetDownStairsFrontTile();
										info.u->rot = dir_to_rot(lvl.schody_dol_dir);
										WarpUnit(*info.u, VEC3(2.f*tile.x+1.f, 0.f, 2.f*tile.y+1.f));
									}
								}
								else
									CHEAT_ERROR("CHEAT_WARP_TO_STAIRS");
							}
							else
								READ_ERROR("CHEAT_WARP_TO_STAIRS");
						}
						break;
					// obs³uga kodu 'noai'
					case NetChange::CHEAT_NOAI:
						{
							byte co;
							if(s.Read(co))
							{
								if(info.cheats)
								{
									bool new_noai = (co == 1);
									if(noai != new_noai)
									{
										noai = new_noai;
										NetChange& c = Add1(net_changes);
										c.type = NetChange::CHEAT_NOAI;
										c.id = co;
									}
								}
								else
									CHEAT_ERROR("CHEAT_NOAI");
							}
							else
								READ_ERROR("CHEAT_NOAI");
						}
						break;
					// gracz odpoczywa w karczmie
					case NetChange::REST:
						{
							byte ile;
							if(s.Read(ile))
							{
								info.u->player->Rest(ile, true);
								UseDays(info.u->player, ile);
								NetChangePlayer& c = Add1(net_changes_player);
								c.type = NetChangePlayer::END_FALLBACK;
								c.pc = info.u->player;
								GetPlayerInfo(c.pc).NeedUpdate();
							}
							else
								READ_ERROR("REST");
						}
						break;
					// gracz trenuje
					case NetChange::TRAIN:
						{
							byte co, co2;
							if(s.Read(co) && s.Read(co2))
							{
								if(co == 2)
									TournamentTrain(*info.u);
								else
									Train(*info.u, co == 1, co2);
								info.u->player->Rest(10, false);
								UseDays(info.u->player, 10);
								NetChangePlayer& c = Add1(net_changes_player);
								c.type = NetChangePlayer::END_FALLBACK;
								c.pc = info.u->player;
								GetPlayerInfo(c.pc).NeedUpdate();
							}
							else
								READ_ERROR("TRAIN");
						}
						break;
					// zmiana lidera przez aktualnego lidera
					case NetChange::CHANGE_LEADER:
						{
							byte id;
							if(s.Read(id))
							{
								if(leader_id == info.id)
								{
									leader_id = id;
									leader = GetPlayerInfo(leader_id).u;

									if(leader_id == my_id)
										AddMsg(txYouAreLeader);
									else
										AddMsg(Format(txPcIsLeader, leader->player->name.c_str()));

									NetChange& c = Add1(net_changes);
									c.type = NetChange::CHANGE_LEADER;
									c.id = id;
								}
								else
									WARN(Format("CHANGE_LEADER from %s who is not a leader.", info.name.c_str()));
							}
							else
								READ_ERROR("CHANGE_LEADER");
						}
						break;
					// gracz sp³aca kredyt
					case NetChange::PAY_CREDIT:
						{
							// [MINOR] jeœli zarobi z³oto i sp³aci czêœæ kredytu mo¿e siê nie udaæ, do naprawienia w v3
							int ile;
							if(s.Read(ile))
							{
								if(info.u->gold >= ile && info.u->player->kredyt >= ile)
								{
									info.u->gold -= ile;
									PayCredit(info.u->player, ile);
								}
								else
								{
									// chce sp³aciæ jak ma za ma³o z³ota lub mniejszy kredyt
									WARN(Format("PAY_CREDIT, player %s have %d gold and %d credit, he wants to pay %d.", info.name.c_str(), info.u->gold, info.u->player->kredyt, ile));
									// aktualizuj jego iloœæ z³ota
									info.UpdateGold();
								}
							}
							else
								READ_ERROR("PAY_CREDIT");
						}
						break;
					// gracz wysy³a z³oto innemu graczowi
					case NetChange::GIVE_GOLD:
						{
							int netid, ile;
							if(s.Read(netid) && s.Read(ile))
							{
								Unit* u = FindTeamMember(netid);
								if(u && u != info.u && ile <= info.u->gold && ile > 0)
								{
									// daj z³oto
									u->gold += ile;
									info.u->gold -= ile;
									if(u->IsPlayer())
									{
										// komunikat o otrzymaniu z³ota
										if(u->player != pc)
										{
											NetChangePlayer& c = Add1(net_changes_player);
											c.type = NetChangePlayer::GOLD_RECEIVED;
											c.id = info.id;
											c.ile = ile;
											c.pc = u->player;
											GetPlayerInfo(u->player).NeedUpdateAndGold();
										}
										else
										{
											AddMultiMsg(Format(txReceivedGold, ile, info.name.c_str()));
											if(sound_volume)
												PlaySound2d(sMoneta);
										}
									}
									else if(player->IsTradingWith(u))
									{
										NetChangePlayer& c = Add1(net_changes_player);
										c.type = NetChangePlayer::UPDATE_TRADER_GOLD;
										c.pc = player;
										c.id = u->netid;
										c.ile = u->gold;
										GetPlayerInfo(player).NeedUpdateAndGold();
									}
								}
								else if(!u)
								{
									// [MINOR] jeœli w tej samej chwili wyrzucimy go z dru¿yny
									WARN(Format("GIVE_GOLD from %s, missing team member %d.", info.name.c_str(), netid));
									info.UpdateGold();
								}
								else if(u == info.u)
								{
									ERROR(Format("GIVE_GOLD from %s, want to give to himself.", info.name.c_str()));
									info.UpdateGold();
								}
								else if(ile <= 0)
								{
									ERROR(Format("GIVE_GOLD from %s, wants to give %d gold.", info.name.c_str(), ile));
									info.UpdateGold();
								}
								else
								{
									// [MINOR] jeœli ktoœ mu 'ukrad³' albo coœ siê zepsu³o i nie wie ile ma, popraw go
									WARN(Format("GIVE_GOLD from %s, want to give %d gold, have %d.", info.name.c_str(), ile, info.u->gold));
									info.UpdateGold();
								}
							}
							else
							{
								READ_ERROR("GIVE_GOLD");
								info.UpdateGold();
							}
						}
						break;
					// gracz upuszcza z³oto na ziemiê
					case NetChange::DROP_GOLD:
						{
							int ile;
							if(s.Read(ile))
							{
								if(ile > 0 && ile <= info.u->gold && info.u->IsStanding() && info.u->action == A_NONE)
								{
									info.u->gold -= ile;

									// animacja wyrzucania
									info.u->action = A_ANIMATION;
									info.u->ani->Play("wyrzuca", PLAY_ONCE|PLAY_PRIO2, 0);
									info.u->ani->frame_end_info = false;

									// stwórz przedmiot
									GroundItem* item = new GroundItem;
									item->item = &gold_item;
									item->count = ile;
									item->team_count = 0;
									item->pos = info.u->pos;
									item->pos.x -= sin(info.u->rot)*0.25f;
									item->pos.z -= cos(info.u->rot)*0.25f;
									item->rot = random(MAX_ANGLE);
									AddGroundItem(GetContext(*info.u), item);

									// wyœlij info o animacji
									if(players > 2)
									{
										NetChange& c = Add1(net_changes);
										c.type = NetChange::DROP_ITEM;
										c.unit = info.u;
									}
								}
							}
							else
								READ_ERROR("DROP_GOLD");
						}
						break;
					// gracz wk³ada z³oto do kontenera
					case NetChange::PUT_GOLD:
						{
							int ile;
							if(s.Read(ile))
							{
								if(ile > 0 && ile <= u.gold)
								{
									if(player->action == PlayerController::Action_LootChest || player->action == PlayerController::Action_LootUnit)
									{
										InsertItem(*player->chest_trade, &gold_item, ile, 0);
										u.gold -= ile;
									}
									else
										ERROR(Format("PUT GOLD from %s, count %d, player not looting.", info.name.c_str(), ile));
								}
								else
									ERROR(Format("PUT_GOLD from %s, count %d, have %d.", info.name.c_str(), ile, u.gold));
							}
							else
								READ_ERROR("PUT_GOLD");
						}
						break;
					// cheat na zmiane pozycji na mapie
					case NetChange::CHEAT_TRAVEL:
						{
							int id;
							if(s.Read(id))
							{
								if(IsLeader(u))
								{
									current_location = id;
									Location& loc = *locations[id];
									if(loc.state == LS_KNOWN)
										loc.state = LS_VISITED;
									world_pos = loc.pos;
									if(open_location != -1)
									{
										LeaveLocation();
										open_location = -1;
									}
									if(players > 2)
									{
										NetChange& c = Add1(net_changes);
										c.type = NetChange::CHEAT_TRAVEL;
										c.id = id;
									}
								}
								else
									ERROR(Format("CHEAT_TRAVEL from %s, location %d, player is not leader.", info.name.c_str(), id));
							}
							else
								READ_ERROR("CHEAT_TRAVEL");
						}
						break;
					// gracz u¿y³ kodu 'hurt'
					case NetChange::CHEAT_HURT:
						{
							int netid;
							if(s.Read(netid))
							{
								if(info.cheats)
								{
									Unit* u = FindUnit(netid);
									if(u)
										GiveDmg(GetContext(*u), NULL, 100.f, *u);
									else
										ERROR(Format("CHEAT_HURT from %s: missing unit %d.", info.name.c_str(), netid));
								}
								else
									CHEAT_ERROR("CHEAT_HURT");
							}
							else
								READ_ERROR("CHEAT_HURT");
						}
						break;
					// gracz u¿y³ kodu 'break_action'
					case NetChange::CHEAT_BREAK_ACTION:
						{
							int netid;
							if(s.Read(netid))
							{
								if(info.cheats)
								{
									Unit* u = FindUnit(netid);
									if(u)
									{
										BreakAction2(*u);
										if(u->IsPlayer() && u->player != pc)
										{
											NetChangePlayer& c = Add1(net_changes_player);
											c.type = NetChangePlayer::BREAK_ACTION;
											c.pc = u->player;
											GetPlayerInfo(c.pc).NeedUpdate();
										}
									}
									else
										ERROR(Format("CHEAT_BREAK_ACTION from %s: missing unit %d.", info.name.c_str(), netid));
								}
								else
									CHEAT_ERROR("CHEAT_BREAK_ACTION");
							}
							else
								READ_ERROR("CHEAT_BREAK_ACTION");
						}
						break;
					// gracz u¿y³ kodu 'fall'
					case NetChange::CHEAT_FALL:
						{
							int netid;
							if(s.Read(netid))
							{
								if(info.cheats)
								{
									Unit* u = FindUnit(netid);
									if(u)
										UnitFall(*u);
									else
										ERROR(Format("CHEAT_FALL from %s: missing unit %d.", info.name.c_str(), netid));
								}
								else
									CHEAT_ERROR("CHEAT_FALL");
							}
							else
								READ_ERROR("CHEAT_FALL");
						}
						break;
					// okrzyk gracza
					case NetChange::YELL:
						PlayerYell(u);
						break;
					// nieznany komunikat
					default:
						WARN(Format("Unknown change type %d.", type));
						assert(0);
						break;
					}
				}
			}
			break;
		default:
			LOG(Format("UpdateServer: Unknown packet from %s: %s.", info.name.c_str(), PacketToString(packet)));
			break;
		}
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
		//int changes = 0;
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
					net_stream.WriteCasted<byte>(u.animacja);
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
					net_stream.WriteCasted<byte>(c.id);
					net_stream.WriteCasted<byte>(c.id == 0 ? u.wyjeta : u.chowana);
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
				net_stream.WriteCasted<byte>(c.ile);
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
					net_stream.WriteCasted<byte>(c.ile);
				}
				break;
			case NetChange::HIT_SOUND:
				net_stream.Write((cstring)&c.pos, sizeof(VEC3));
				net_stream.WriteCasted<byte>(c.id);
				net_stream.WriteCasted<byte>(c.ile);
				break;
			case NetChange::SHOT_ARROW:
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
						net_stream.Write(u.IsPlayer() ? u.player->kredyt : u.hero->kredyt);
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
				break;
			case NetChange::CHEST_OPEN:
			case NetChange::CHEST_CLOSE:
			case NetChange::KICK_NPC:
			case NetChange::REMOVE_UNIT:
			case NetChange::REMOVE_TRAP:
			case NetChange::TRIGGER_TRAP:
			case NetChange::CHEAT_TRAVEL:
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
				net_stream.WriteCasted<byte>(c.unit->in_building);
				net_stream.Write((cstring)&c.unit->pos, sizeof(VEC3));
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
					for(int i=0; i<c.ile; ++i)
						WriteString1(net_stream, q->msgs[q->msgs.size()-c.ile+i]);
				}
				break;
			case NetChange::CHANGE_LEADER:
			case NetChange::ARENA_SOUND:
			case NetChange::TRAVEL:
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
				net_stream.WriteCasted<byte>(c.unit->hero->free);
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
							case NetChangePlayer::SHOW_CHOICES:
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

//=================================================================================================
#undef READ_ERROR
#define READ_ERROR(x) ERROR("Read error " x ".")
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
		switch(packet->data[0])
		{
		case ID_CONNECTION_LOST:
		case ID_DISCONNECTION_NOTIFICATION:
			{
				cstring text, text_eng;
				if(packet->data[0] == ID_CONNECTION_LOST)
				{
					text = txLostConnection;
					text_eng = "Lost connection with server.";
				}
				else
				{
					text = txYouDisconnected;
					text_eng = "You have been disconnected.";
				}
				LOG(text_eng);
				peer->DeallocatePacket(packet);
				ExitToMenu();
				GUI.SimpleDialog(text, NULL);
				return;
			}
		case ID_SAY:
			Client_Say(packet);
			break;
		case ID_WHISPER:
			Client_Whisper(packet);
			break;
		case ID_SERVER_SAY:
			Client_ServerSay(packet);
			break;
		case ID_SERVER_CLOSE:
			{
				LOG("You have been kicked.");
				peer->DeallocatePacket(packet);
				ExitToMenu();
				GUI.SimpleDialog(txYouKicked, NULL);
				return;
			}
		case ID_CHANGE_LEVEL:
			if(packet->length != 3)
				WARN(Format("Broken packet ID_CHANGE_LEVEL: %s.", PacketToString(packet)));
			else
			{
				byte loc = packet->data[1];
				byte level = packet->data[2];
				current_location = loc;
				location = locations[loc];
				dungeon_level = level;
				LOG(Format("Level change to %s (%d, level %d).", location->name.c_str(), current_location, dungeon_level));
				info_box->Show(txGeneratingLocation);
				LeaveLevel();
				net_mode = NM_TRANSFER;
				net_state = 2;
				clear_color = BLACK;
				load_screen->visible = true;
				game_gui_container->visible = false;
				game_messages->visible = false;
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
				Net_FilterClientChanges();
				LoadingStart(4);
				return;
			}
			break;
		case ID_CHANGES:
			if(packet->length < 3)
				WARN(Format("Broken packet ID_CHANGES: %s.", PacketToString(packet)));
			else
			{
				BitStream s(packet->data+1, packet->length-1, false);
				word ile;
				s.Read(ile);
				for(word i=0; i<ile; ++i)
				{
					NetChange::TYPE type;
					if(!s.ReadCasted<byte>(type))
					{
						READ_ERROR("type");
						break;
					}

					switch(type)
					{
					// zmiana pozycji/obrotu/animacji postaci
					case NetChange::UNIT_POS:
						{
							int netid;
							if(s.Read(netid))
							{
								Unit* u = FindUnit(netid);
								if(!u)
									WARN(Format("UNIT_POS, missing unit %d.", netid));
								if(u == pc->unit || !u)
								{
									if(!SkipBitstream(s, sizeof(float)*5+1))
										READ_ERROR("UNIT_POS(2)");
								}
								else
								{
									ANIMACJA ani;
									float rot;
									if(	ReadStruct(s, u->pos) &&
										s.Read(rot) &&
										s.Read(u->ani->groups[0].speed) &&
										s.ReadCasted<byte>(ani))
									{
										assert(ani < ANI_MAX);
										if(u->animacja != ANI_ODTWORZ && ani != ANI_ODTWORZ)
											u->animacja = ani;
										UpdateUnitPhysics(u, u->pos);
										WriteToInterpolator(u->interp, u->pos, rot);
									}
									else
										READ_ERROR("UNIT_POS(3)");
								}
							}
							else
								READ_ERROR("UNIT_POS");
						}
						break;
					// zmiana ekwipunku postaci (widocznego)
					case NetChange::CHANGE_EQUIPMENT:
						{
							// [int(netid)-jednostka, byte(id)-item slot, Item-przedmiot]
							int netid;
							ITEM_SLOT type;
							const Item* item;
							if(s.Read(netid) && s.ReadCasted<byte>(type) && ReadItemAndFind(s, item) != -1)
							{
								Unit* u = FindUnit(netid);
								if(u)
									u->slots[type] = item;
								else
									ERROR(Format("CHANGE_EQUIPMENT: Missing unit %d, type %d, item 0x%p (%s).", netid, type, item, item ? item->id : "NULL"));
							}
							else
								READ_ERROR("CHANGE_EQUIPMENT");
						}
						break;
					// wyjmowanie/chowanie broni przez postaæ
					case NetChange::TAKE_WEAPON:
						{
							int netid;
							byte id, co;
							if(s.Read(netid) && s.Read(id) && s.Read(co))
							{
								Unit* u = FindUnit(netid);
								if(u)
								{
									if(u != pc->unit)
									{
										if(u->ani->ani->head.n_groups > 1)
											u->ani->groups[1].speed = 1.f;
										SetUnitWeaponState(*u, (id == 0), (BRON)co);
									}
								}
								else
									WARN(Format("TAKE_WEAPON, missing unit %d.", netid));
							}
							else
								READ_ERROR("TAKE_WEAPON");
						}
						break;
					// atak przez postaæ
					case NetChange::ATTACK:
						{
							int netid;
							byte id;
							float speed;
							if(s.Read(netid) && s.Read(id) && s.Read(speed))
							{
								Unit* up = FindUnit(netid);
								if(!up)
								{
									WARN(Format("ATTACK, missing unit %d.", netid));
									break;
								}
								Unit& u = *up;
								if(&u == pc->unit)
									break;
								byte co = (id&0xF);
								int group = u.ani->ani->head.n_groups-1;
								// upewnij siê ¿e ma wyjêt¹ broñ
								u.stan_broni = BRON_WYJETA;
								switch(co)
								{
								case AID_Attack:
									if(u.action == A_ATTACK && u.etap_animacji == 0)
									{
										u.etap_animacji = 1;
										u.ani->groups[1].speed = speed;
									}
									else
									{
										if(sound_volume > 0 && u.data->sounds->sound[SOUND_ATTACK] && rand2()%4 == 0)
											PlayAttachedSound(u, u.data->sounds->sound[SOUND_ATTACK], 1.f, 10.f);
										u.action = A_ATTACK;
										u.attack_id = ((id&0xF0)>>4);
										u.attack_power = 1.f;
										u.ani->Play(NAMES::ani_attacks[u.attack_id], PLAY_PRIO1|PLAY_ONCE|PLAY_RESTORE, group);
										u.ani->groups[group].speed = speed;
										u.etap_animacji = 1;
										u.trafil = false;
									}
									break;
								case AID_PowerAttack:
									{
										if(sound_volume > 0 && u.data->sounds->sound[SOUND_ATTACK] && rand2()%4 == 0)
											PlayAttachedSound(u, u.data->sounds->sound[SOUND_ATTACK], 1.f, 10.f);
										u.action = A_ATTACK;
										u.attack_id = ((id&0xF0)>>4);
										u.attack_power = 1.f;
										u.ani->Play(NAMES::ani_attacks[u.attack_id], PLAY_PRIO1|PLAY_ONCE|PLAY_RESTORE, group);
										u.ani->groups[group].speed = speed;
										u.etap_animacji = 0;
										u.trafil = false;
									}
									break;
								case AID_Shoot:
								case AID_StartShoot:
									if(u.action == A_SHOOT && u.etap_animacji == 0)
										u.etap_animacji = 1;
									else
									{
										u.ani->Play(NAMES::ani_shoot, PLAY_PRIO1|PLAY_ONCE|PLAY_RESTORE, group);
										u.ani->groups[group].speed = speed;
										u.action = A_SHOOT;
										u.etap_animacji = (co == AID_Shoot ? 1 : 0);
										u.trafil = false;
										if(!u.bow_instance)
										{
											if(bow_instances.empty())
												u.bow_instance = new AnimeshInstance(u.GetBow().ani);
											else
											{
												u.bow_instance = bow_instances.back();
												bow_instances.pop_back();
												u.bow_instance->ani = u.GetBow().ani;
											}
										}
										u.bow_instance->Play(&u.bow_instance->ani->anims[0], PLAY_ONCE|PLAY_PRIO1|PLAY_NO_BLEND, 0);
										u.bow_instance->groups[0].speed = u.ani->groups[group].speed;
									}
									break;
								case AID_Block:
									{
										u.action = A_BLOCK;
										u.ani->Play(NAMES::ani_block, PLAY_PRIO1|PLAY_STOP_AT_END|PLAY_RESTORE, group);
										u.ani->groups[1].speed = 1.f;
										u.ani->groups[1].blend_max = speed;
										u.etap_animacji = 0;
									}
									break;
								case AID_Bash:
									{
										u.action = A_BASH;
										u.etap_animacji = 0;
										u.ani->Play(NAMES::ani_bash, PLAY_ONCE|PLAY_PRIO1|PLAY_RESTORE, group);
										u.ani->groups[1].speed = 2.f;
										u.ani->frame_end_info2 = false;
										u.trafil = false;
									}
									break;
								case AID_RunningAttack:
									{
										if(sound_volume > 0 && u.data->sounds->sound[SOUND_ATTACK] && rand2()%4 == 0)
											PlayAttachedSound(u, u.data->sounds->sound[SOUND_ATTACK], 1.f, 10.f);
										u.action = A_ATTACK;
										u.attack_id = ((id&0xF0)>>4);
										u.attack_power = 1.5f;
										u.atak_w_biegu = true;
										u.ani->Play(NAMES::ani_attacks[u.attack_id], PLAY_PRIO1|PLAY_ONCE|PLAY_RESTORE, group);
										u.ani->groups[group].speed = speed;
										u.etap_animacji = 1;
										u.trafil = false;
									}
									break;
								case AID_StopBlock:
									{
										u.action = A_NONE;
										u.ani->frame_end_info2 = false;
										u.ani->Deactivate(group);
										u.ani->groups[1].speed = 1.f;
									}
									break;
								}
							}
							else
								READ_ERROR("ATTACK");
						}
						break;
					// zmiana flag gry
					case NetChange::CHANGE_FLAGS:
						{
							byte b;
							if(s.Read(b))
							{
								bandyta = IS_SET(b, 0x01);
								atak_szalencow = IS_SET(b, 0x02);
								anyone_talking = IS_SET(b, 0x04);
							}
							else
								READ_ERROR("CHANGE_FLAGS");
						}
						break;
					// aktualizacja hp postaci
					case NetChange::UPDATE_HP:
						{
							int netid;
							float hp, hpmax;
							if(s.Read(netid) && s.Read(hp) && s.Read(hpmax))
							{
								Unit* u = FindUnit(netid);
								if(u)
								{
									if(u == pc->unit)
									{
										float hp_prev = u->hp;
										u->hp = hp;
										u->hpmax = hpmax;
										hp_prev -= u->hp;
										if(hp_prev > 0.f)
											pc->last_dmg += hp_prev;
									}
									else
									{
										u->hp = hp;
										u->hpmax = hpmax;
									}
								}
								else
									WARN(Format("UPDATE_HP, missing unit %d.", netid));
							}
							else
								READ_ERROR("UPDATE_HP");
						}
						break;
					// tworzenie krwi
					case NetChange::SPAWN_BLOOD:
						{
							byte type;
							VEC3 pos;
							if(s.Read(type) && s.Read((char*)&pos, sizeof(pos)))
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
								pe->speed_min = VEC3(-1,0,-1);
								pe->speed_max = VEC3(1,1,1);
								pe->pos_min = VEC3(-0.1f,-0.1f,-0.1f);
								pe->pos_max = VEC3(0.1f,0.1f,0.1f);
								pe->size = 0.3f;
								pe->op_size = POP_LINEAR_SHRINK;
								pe->alpha = 0.9f;
								pe->op_alpha = POP_LINEAR_SHRINK;
								pe->mode = 0;
								pe->Init();
								GetContext(pos).pes->push_back(pe);
							}
							else
								READ_ERROR("SPAWN_BLOOD");
						}
						break;
					// dŸwiêk otrzymywania obra¿eñ przez postaæ
					case NetChange::HURT_SOUND:
						{
							int netid;
							if(s.Read(netid))
							{
								if(sound_volume)
								{
									Unit* u = FindUnit(netid);
									if(u)
										PlayAttachedSound(*u, u->data->sounds->sound[SOUND_PAIN], 2.f, 15.f);
									else
										WARN(Format("HURT_SOUND, missing unit %d.", netid));
								}
							}
							else
								READ_ERROR("HURT_SOUND");
						}
						break;
					// postaæ umiera
					case NetChange::DIE:
						{
							int netid;
							if(s.Read(netid))
							{
								Unit* u = FindUnit(netid);
								if(u)
									UnitDie(*u, NULL, NULL);
								else
									WARN(Format("DIE, missing unit %d.", netid));
							}
							else
								READ_ERROR("DIE");
						}
						break;
					// postaæ upada na ziemiê ale nie umiera
					case NetChange::FALL:
						{
							int netid;
							if(s.Read(netid))
							{
								Unit* u = FindUnit(netid);
								if(u)
									UnitFall(*u);
								else
									WARN(Format("FALL, missing unit %d.", netid));
							}
							else
								READ_ERROR("FALL");
						}
						break;
					// animacja wyrzucanie przedmiotu przez postaæ
					case NetChange::DROP_ITEM:
						{
							int netid;
							if(s.Read(netid))
							{
								Unit* u = FindUnit(netid);
								if(u)
								{
									if(u != pc->unit)
									{
										u->action = A_ANIMATION;
										u->ani->Play("wyrzuca", PLAY_ONCE|PLAY_PRIO2, 0);
										u->ani->frame_end_info = false;
									}
								}
								else
									WARN(Format("DROP_ITEM, missing unit %d.", netid));
							}
							else
								READ_ERROR("DROP_ITEM");
						}
						break;
					// pojawienie siê przedmiotu na ziemi
					case NetChange::SPAWN_ITEM:
						{
							GroundItem* item = new GroundItem;
							if(ReadItem(s, *item))
								GetContext(item->pos).items->push_back(item);
							else
							{
								READ_ERROR("SPAWN_ITEM");
								delete item;
							}
						}
						break;
					// animacja podnoszenia przedmiotu przez kogoœ
					case NetChange::PICKUP_ITEM:
						{
							int netid;
							byte u_gory;
							if(s.Read(netid) && s.Read(u_gory))
							{
								Unit* u = FindUnit(netid);
								if(u)
								{
									if(u != pc->unit)
									{
										u->action = A_PICKUP;
										u->animacja = ANI_ODTWORZ;
										u->ani->Play(u_gory == 1 ? "podnosi_gora" : "podnosi", PLAY_ONCE|PLAY_PRIO2, 0);
										u->ani->frame_end_info = false;
									}
								}
								else
									WARN(Format("PICKUP_ITEM, missing unit %d.", netid));
							}
							else
								READ_ERROR("PICKUP_ITEM");
						}
						break;
					// usuwanie przedmiotu (ktoœ go podniós³)
					case NetChange::REMOVE_ITEM:
						{
							int netid;
							if(s.Read(netid))
							{
								LevelContext* ctx;
								GroundItem* item = FindItemNetid(netid, &ctx);
								if(item)
								{
									if(before_player == BP_ITEM && before_player_ptr.item == item)
										before_player = BP_NONE;
									if(picking_item_state == 1 && picking_item == item)
										picking_item_state = 2;
									else
										delete item;
									RemoveElement(ctx->items, item);
								}
								else
									WARN(Format("REMOVE_ITEM, missing item %d.", netid));
							}
							else
								READ_ERROR("REMOVE_ITEM");
						}
						break;
					// u¿ywanie miksturki przez postaæ
					case NetChange::CONSUME_ITEM:
						{
							int netid;
							byte force;
							if(s.Read(netid) && ReadString1(s) && s.Read(force))
							{
								Unit* u = FindUnit(netid);
								const Item* item = FindItem(BUF);
								if(u && item && item->type == IT_CONSUMEABLE)
								{
									if(u != pc->unit || force == 1)
										u->ConsumeItem(item->ToConsumeable(), false, false);
								}
								else
									ERROR(Format("CONSUME_ITEM, missing unit %d or invalid item %p (%s).", netid, item, item ? item->id : "NULL"));
							}
							else
								READ_ERROR("CONSUME_ITEM");
						}
						break;
					// dŸwiêk trafienia czegoœ
					case NetChange::HIT_SOUND:
						{
							VEC3 pos;
							MATERIAL_TYPE mat1, mat2;
							if(	s.Read((char*)&pos, sizeof(VEC3)) &&
								s.ReadCasted<byte>(mat1) &&
								s.ReadCasted<byte>(mat2))
							{
								if(sound_volume)
									PlaySound3d(GetMaterialSound(mat1, mat2), pos, 2.f, 10.f);
							}
							else
								READ_ERROR("HIT_SOUND");
						}
						break;
					// og³uszenie postaci
					case NetChange::STUNNED:
						{
							int netid;
							if(s.Read(netid))
							{
								Unit* u = FindUnit(netid);

								if(u)
								{
									BreakAction2(*u);

									if(u->action != A_POSITION)
										u->action = A_PAIN;
									else
										u->etap_animacji = 1;

									if(u->ani->ani->head.n_groups == 2)
									{
										u->ani->frame_end_info2 = false;
										u->ani->Play(NAMES::ani_hurt, PLAY_PRIO1|PLAY_ONCE, 1);
									}
									else
									{
										u->ani->frame_end_info = false;
										u->ani->Play(NAMES::ani_hurt, PLAY_PRIO3|PLAY_ONCE, 0);
										u->animacja = ANI_ODTWORZ;
									}
								}
								else
									WARN(Format("STUNNED, missing unit %d.", netid));
							}
							else
								READ_ERROR("STUNNED");
						}
						break;
					// pojawienie siê strza³y
					case NetChange::SHOT_ARROW:
						{
							int netid;
							VEC3 pos;
							if(s.Read(netid) && s.Read((char*)&pos, sizeof(pos)))
							{
								LevelContext& ctx = GetContext(pos);

								Bullet& b = Add1(ctx.bullets);
								b.mesh = aArrow;
								b.pos = pos;
								b.rot.z = 0.f;
								if(	s.Read(b.rot.y) &&
									s.Read(b.yspeed) &&
									s.Read(b.rot.x))
								{
									b.owner = NULL;
									b.pe = NULL;
									b.remove = false;
									b.speed = ARROW_SPEED;
									b.spell = NULL;
									b.tex = NULL;
									b.tex_size = 0.f;
									b.timer = ARROW_TIMER;
									b.owner = FindUnit(netid);
									if(!b.owner && netid != -1)
										WARN(Format("SHOT_ARROW, missing unit %d.", netid));

									TrailParticleEmitter* tpe = new TrailParticleEmitter;
									tpe->fade = 0.3f;
									tpe->color1 = VEC4(1,1,1,0.5f);
									tpe->color2 = VEC4(1,1,1,0);
									tpe->Init(50);
									ctx.tpes->push_back(tpe);
									b.trail = tpe;

									TrailParticleEmitter* tpe2 = new TrailParticleEmitter;
									tpe2->fade = 0.3f;
									tpe2->color1 = VEC4(1,1,1,0.5f);
									tpe2->color2 = VEC4(1,1,1,0);
									tpe2->Init(50);
									ctx.tpes->push_back(tpe2);
									b.trail2 = tpe2;

									if(sound_volume)
										PlaySound3d(sBow[rand2()%2], b.pos, 2.f, 8.f);
								}
								else
								{
									ctx.bullets->pop_back();
									READ_ERROR("SHOT_ARROW(2)");
								}
							}
							else
								READ_ERROR("SHOT_ARROW");
						}
						break;
					// aktualizacja kredytu dru¿yny
					case NetChange::UPDATE_CREDIT:
						{
							byte ile;
							if(s.Read(ile))
							{
								for(byte i=0; i<ile; ++i)
								{
									int netid, kredyt;
									if(s.Read(netid) && s.Read(kredyt))
									{
										Unit* u = FindUnit(netid);
										if(u)
										{
											if(u->IsPlayer())
												u->player->kredyt = kredyt;
											else
												u->hero->kredyt = kredyt;
										}
										else
											WARN(Format("UPDATE_CREDIT, missing unit %d.", netid));
									}
									else
									{
										READ_ERROR("UPDATE_CREDIT(2)");
										break;
									}
								}
							}
							else
								READ_ERROR("UPDATE_CREDIT");
						}
						break;
					// animacja idle
					case NetChange::IDLE:
						{
							int netid;
							byte id;
							if(s.Read(netid) && s.Read(id))
							{
								Unit* u = FindUnit(netid);
								if(u)
								{
									u->ani->Play(u->data->idles[id], PLAY_ONCE, 0);
									u->ani->frame_end_info = false;
									u->animacja = ANI_IDLE;
								}
								else
									WARN(Format("IDLE, missing unit %d.", netid));
							}
							else
								READ_ERROR("IDLE");
						}
						break;
					// powitanie dŸwiêkowe przez postaæ
					case NetChange::HELLO:
						{
							int netid;
							if(s.Read(netid))
							{
								Unit* u = FindUnit(netid);
								if(u)
								{
									if(sound_volume)
									{
										SOUND snd = GetTalkSound(*u);
										if(snd)
											PlayAttachedSound(*u, snd, 2.f, 5.f);
									}
								}
								else
									WARN(Format("HELLO, missing unit %d.", netid));
							}
							else
								READ_ERROR("HELLO");
						}
						break;
					// wszystkie zadania ukoñczone
					case NetChange::ALL_QUESTS_COMPLETED:
						unique_completed_show = true;
						break;
					// mówienie czegoœ przez postaæ
					case NetChange::TALK:
						{
							int netid, skip_id;
							byte co;
							if( s.Read(netid) &&
								s.Read(co) &&
								s.Read(skip_id) &&
								ReadString1(s))
							{
								Unit* u = FindUnit(netid);

								if(u)
								{
									game_gui->AddSpeechBubble(u, BUF);
									u->bubble->skip_id = skip_id;

									if(co != 0)
									{
										u->ani->Play(co == 1 ? "i_co" : "pokazuje", PLAY_ONCE|PLAY_PRIO2, 0);
										u->animacja = ANI_ODTWORZ;
										u->action = A_ANIMATION;
									}

									if(dialog_context.dialog_mode && dialog_context.talker == u)
									{
										dialog_context.dialog_s_text = BUF;
										dialog_context.dialog_text = dialog_context.dialog_s_text.c_str();
										dialog_context.dialog_wait = 1.f;
										dialog_context.skip_id = skip_id;
									}
									else if(pc->action == PlayerController::Action_Talk && pc->action_unit == u)
									{
										predialog = BUF;
										dialog_context.skip_id = skip_id;
									}
								}
								else
									WARN(Format("TALK, missing unit %d.", netid));
							}
							else
								READ_ERROR("TALK");
						}
						break;
					// zmienia stan lokacji
					case NetChange::CHANGE_LOCATION_STATE:
						{
							byte id, co;
							if(s.Read(id) && s.Read(co))
							{
								Location* loc = NULL;
								if(id < locations.size())
									loc = locations[id];
								if(loc)
								{
									if(co == 0)
										loc->state = LS_KNOWN;
									else if(co == 1)
										loc->state = LS_VISITED;
								}
								else
									WARN(Format("CHANGE_LOCATION_STATE, missing location %d.", id));
							}
							else
								READ_ERROR("CHANGE_LOCATION_STATE");
						}
						break;
					// dodaje plotkê do dziennika
					case NetChange::ADD_RUMOR:
						{
							if(ReadString1(s))
							{
								AddGameMsg3(GMS_ADDED_RUMOR);
								plotki.push_back(BUF);
								journal->NeedUpdate(Journal::Rumors);
							}
							else
								READ_ERROR("ADD_RUMOR");
						}
						break;
					// bohater mówi jak ma na imiê
					case NetChange::TELL_NAME:
						{
							int netid;
							if(s.Read(netid))
							{
								Unit* u = FindUnit(netid);
								if(u && u->IsHero())
									u->hero->know_name = true;
								else if(u)
									WARN(Format("TELL_NAME, %d is not a hero.", netid));
								else
									WARN(Format("TELL_NAME, missing unit %d.", netid));
							}
							else
								READ_ERROR("TELL_NAME");
						}
						break;
					// zmiana koloru w³osów
					case NetChange::HAIR_COLOR:
						{
							int netid;
							VEC4 hair_color;
							if(s.Read(netid) &&
								s.Read((char*)&hair_color, sizeof(hair_color)))
							{
								Unit* u = FindUnit(netid);
								if(u && u->type == Unit::HUMAN)
									u->human_data->hair_color = hair_color;
								else if(u)
									WARN(Format("HAIR_COLOR, %d is not a human.", netid));
								else
									WARN(Format("HAIR_COLOR, missing unit %d.", netid));
							}
							else
								READ_ERROR("HAIR_COLOR");
						}
						break;
					// przenosi postaæ
					case NetChange::WARP:
						{
							int netid;
							if(!s.Read(netid))
							{
								READ_ERROR("WARP");
								break;
							}
							Unit* u = FindUnit(netid);
							if(!u)
							{
								WARN(Format("WARP, missing unit %d.", netid));
								if(!SkipBitstream(s, sizeof(VEC3)+sizeof(float)+1))
									READ_ERROR("WARP(2)");
								break;
							}
							int in_building = u->in_building;
							if(	!s.ReadCasted<byte>(u->in_building) ||
								!ReadStruct(s, u->pos) ||
								!s.Read(u->rot))
							{
								READ_ERROR("WARP(3)");
								break;
							}

							u->visual_pos = u->pos;
							if(u->in_building == 0xFF)
								u->in_building = -1;
							if(u->interp)
								u->interp->Reset(u->pos, u->rot);
							
							if(in_building != u->in_building)
							{
								RemoveElement(GetContextFromInBuilding(in_building).units, u);
								GetContextFromInBuilding(u->in_building).units->push_back(u);
							}
							if(u == pc->unit)
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
								rot_buf = 0.f;
							}
						}
						break;
					// dodaje nowy przedmiot questowy
					case NetChange::REGISTER_ITEM:
						{
							QuestItemClient* item = new QuestItemClient;
							if(ReadString1(s, item->str_id))
							{
								const Item* base;
								if(item->str_id[0] == '$')
									base = FindItem(item->str_id.c_str() + 1);
								else
									base = FindItem(item->str_id.c_str());
								if(base)
								{
									item->item = CreateItemCopy(base);
									if(ReadString1(s, item->item->name) &&
										ReadString1(s, item->item->desc) &&
										s.Read(item->item->refid))
									{										
										item->item->id = item->str_id.c_str();
										quest_items.push_back(item);
									}
									else
									{
										READ_ERROR("REGISTER_ITEM(2)");
										delete item;
									}
								}
								else
								{
									ERROR(Format("REGISTER_ITEM, missing base item '%s'.", item->str_id.c_str()));
									delete item;
								}
							}
							else
							{
								READ_ERROR("REGISTER_ITEM");
								delete item;
							}
						}
						break;
					// dodano zadanie
					case NetChange::ADD_QUEST:
						{
							PlaceholderQuest* q = new PlaceholderQuest;
							quests.push_back(q);
							if(s.Read(q->refid))
							{
								if(!ReadString1(s, q->name))
								{
									READ_ERROR("ADD_QUEST(2)");
									break;
								}
								q->msgs.resize(2);
								if(!ReadString1(s, q->msgs[0]) || !ReadString1(s, q->msgs[1]))
								{
									READ_ERROR("ADD_QUEST(3)");
									q->msgs.clear();
									break;
								}
								q->state = Quest::Started;
								journal->NeedUpdate(Journal::Quests, GetQuestIndex(q));
								AddGameMsg3(GMS_JOURNAL_UPDATED);
							}
							else
							{
								delete q;
								quests.pop_back();
								READ_ERROR("ADD_QUEST");
							}
						}
						break;
					// aktualizacja zadania
					case NetChange::UPDATE_QUEST:
						{
							int refid;
							byte state;
							if(s.Read(refid) && s.Read(state))
							{
								Quest* q = FindQuest(refid);
								if(q)
								{
									q->state = (Quest::State)state;
									string& str = Add1(q->msgs);
									if(!ReadString1(s, str))
									{
										READ_ERROR("UPDATE_QUEST(2)");
										q->msgs.pop_back();
									}
									journal->NeedUpdate(Journal::Quests, GetQuestIndex(q));
									AddGameMsg3(GMS_JOURNAL_UPDATED);
								}
								else
								{
									WARN(Format("UPDATE_QUEST, missing quest %d.", refid));
									if(!SkipString1(s))
										READ_ERROR("UPDATE_QUEST(3)");
								}
							}
							else
								READ_ERROR("UPDATE_QUEST");
						}
						break;
					// zmiana nazwy przedmiotu
					case NetChange::RENAME_ITEM:
						{
							int refid;
							if(s.Read(refid) && ReadString1(s))
							{
								bool jest = false;
								for(vector<QuestItemClient*>::iterator it = quest_items.begin(), end = quest_items.end(); it!=end; ++it)
								{
									if((*it)->item->refid == refid && (*it)->str_id == BUF)
									{
										if(!ReadString1(s, (*it)->item->name))
											READ_ERROR("RENAME_ITEM(2)");
										jest = true;
										break;
									}
								}
								if(!jest)
								{
									// pomiñ
									WARN(Format("RENAME_ITEM, missing item %s.", BUF));
									if(!SkipString1(s))
										READ_ERROR("RENAME_ITEM(3)");
								}
							}
							else
								READ_ERROR("RENAME_ITEM");
						}
						break;
					// aktualizacja zadania, dodaje kilka zdañ
					case NetChange::UPDATE_QUEST_MULTI:
						{
							int refid;
							byte ile, stan;
							if(s.Read(refid) && s.Read(stan) && s.Read(ile))
							{
								Quest* q = FindQuest(refid);
								if(q)
								{
									q->state = (Quest::State)stan;
									for(byte i=0; i<ile; ++i)
									{
										string& str = Add1(q->msgs);
										if(!ReadString1(s, str))
										{
											WARN(Format("Read error UPDATE_QUEST_MULTI(2), i=%d.", i));
											q->msgs.pop_back();
											break;
										}
									}
									journal->NeedUpdate(Journal::Quests, GetQuestIndex(q));
									AddGameMsg3(GMS_JOURNAL_UPDATED);
								}
								else
								{
									WARN(Format("UPDATE_QUEST_MULTI, missing quest %d.", refid));
									if(!SkipStringArray<byte,byte>(net_stream))
										READ_ERROR("UPDATE_QUEST_MULTI(2)");
								}
							}
							else
								READ_ERROR("UPDATE_QUEST_MULTI");
						}
						break;
					// zmiana przywódcy
					case NetChange::CHANGE_LEADER:
						{
							byte id;
							if(s.Read(id))
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
									WARN(Format("CHANGE_LEADER, missing player %d.", id));
							}
							else
								READ_ERROR("CHANGE_LEADER");
						}
						break;
					// losowa liczba
					case NetChange::RANDOM_NUMBER:
						{
							byte id, co;
							if(s.Read(id) && s.Read(co))
							{
								if(id != my_id)
								{
									PlayerInfo* info = GetPlayerInfoTry(id);
									if(info)
										AddMsg(Format(txRolledNumber, info->name.c_str(), co));
									else
										WARN(Format("RANDOM_NUMBER, missing player %d.", id));
								}
							}
							else
								READ_ERROR("RANDOM_NUMBER");
						}
						break;
					// usuwanie gracza
					case NetChange::REMOVE_PLAYER:
						{
							byte id, reason;
							if(s.Read(id) && s.Read(reason))
							{
								PlayerInfo* info = GetPlayerInfoTry(id);
								if(info)
								{
									info->left = true;
									AddMsg(Format("%s %s.", info->name.c_str(), reason == 1 ? txPcWasKicked : txPcLeftGame));

									if(info->u)
									{
										RemoveElement(team, info->u);
										RemoveElement(active_team, info->u);

										if(reason == 2)
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
								else
									WARN(Format("REMOVE_PLAYER, missing player %d.", id));
							}
							else
								READ_ERROR("REMOVE_PLAYER");
						}
						break;
					// u¿ywanie u¿ywalnego obiektu
					case NetChange::USE_USEABLE:
						{
							int netid, useable_netid;
							byte co;
							if(s.Read(netid) && s.Read(useable_netid) && s.Read(co))
							{
								Unit* u = FindUnit(netid);
								Useable* use = FindUseable(useable_netid);
								if(u && use)
								{
									if(co == 1 || co == 2)
									{
										BaseUsable& base = g_base_usables[use->type];

										u->action = A_ANIMATION2;
										u->animacja = ANI_ODTWORZ;
										u->ani->Play(co == 2 ? "czyta_papiery" : base.anim, PLAY_PRIO1, 0);
										u->useable = use;
										u->target_pos = u->pos;
										u->target_pos2 = use->pos;
										if(g_base_usables[use->type].limit_rot == 4)
											u->target_pos2 -= VEC3(sin(use->rot)*1.5f,0,cos(use->rot)*1.5f);
										u->timer = 0.f;
										u->etap_animacji = 0;
										u->use_rot = lookat_angle(u->pos, u->useable->pos);
										u->used_item = base.item;
										if(u->used_item)
										{
											u->wyjeta = B_BRAK;
											u->stan_broni = BRON_SCHOWANA;
										}
										use->user = u;

										if(before_player == BP_USEABLE && before_player_ptr.useable == use)
											before_player = BP_NONE;
									}
									else if(u->player != pc)
									{
										if(co == 0)
											use->user = NULL;
										u->action = A_NONE;
										u->animacja = ANI_STOI;
										if(u->live_state == Unit::ALIVE)
											u->used_item = NULL;
									}
								}
								else if(u)
									WARN(Format("USE_USEABLE, missing object %d.", useable_netid));
								else
									WARN(Format("USE_USEABLE, missing unit %d.", netid));
							}
						}
						break;
					// wstawanie postaci po byciu pobitym
					case NetChange::STAND_UP:
						{
							int netid;
							if(s.Read(netid))
							{
								Unit* u = FindUnit(netid);
								if(u)
									UnitStandup(*u);
								else
									WARN(Format("STAND_UP, missing unit %d.", netid));
							}
							else
								READ_ERROR("STAND_UP");
						}
						break;
					// koniec gry - przegrana
					case NetChange::GAME_OVER:
						LOG("Game over: all players died.");
						SetMusic(MUSIC_CRYPT);
						CloseAllPanels();
						++death_screen;
						death_fade = 0;
						death_solo = false;
						exit_from_server = true;
						break;
					// do³¹czanie npc do dru¿yny
					case NetChange::RECRUIT_NPC:
						{
							int netid;
							byte free;
							if(s.Read(netid) && s.Read(free))
							{
								Unit* u = FindUnit(netid);
								if(u && u->IsHero())
								{
									u->hero->team_member = true;
									u->hero->free = (free == 1);
									u->hero->kredyt = 0;
									team.push_back(u);
									if(free != 1)
										active_team.push_back(u);
									// aktualizuj TeamPanel o ile otwarty
									if(team_panel->visible)
										team_panel->Changed();
								}
								else if(!u)
									WARN(Format("RECRUIT_NPC, missing unit %d.", netid));
								else
									WARN(Format("RECRUIT_NPC, %d is not a hero.", netid));
							}
							else
								READ_ERROR("RECRUIT_NPC");
						}
						break;
					// wyrzucanie npc z dru¿yny
					case NetChange::KICK_NPC:
						{
							int netid;
							if(s.Read(netid))
							{
								Unit* u = FindUnit(netid);
								if(u && u->IsHero() && u->hero->team_member)
								{
									u->hero->team_member = false;
									RemoveElement(team, u);
									if(!u->hero->free)
										RemoveElement(active_team, u);
									// aktualizuj TeamPanel o ile otwarty
									if(team_panel->visible)
										team_panel->Changed();
								}
								else if(!u)
									WARN(Format("KICK_NPC, missing unit %d.", netid));
								else if(!u->IsHero())
									WARN(Format("KICK_NPC, %d is not a hero.", netid));
								else
									WARN(Format("KICK_NPC, %d is not a team member.", netid));
							}
							else
								READ_ERROR("KICK_NPC");
						}
						break;
					// usuwanie jednostki
					case NetChange::REMOVE_UNIT:
						{
							int netid;
							if(s.Read(netid))
							{
								Unit* u = FindUnit(netid);
								if(u)
								{
									u->to_remove = true;
									to_remove.push_back(u);
								}
								else
									WARN(Format("REMOVE_UNIT, missing unit %d.", netid));
							}
							else
								READ_ERROR("REMOVE_UNIT");
						}
						break;
					// pojawianie siê nowej jednostki
					case NetChange::SPAWN_UNIT:
						{
							Unit* u = new Unit;
							if(ReadUnit(s, *u))
							{
								LevelContext& ctx = GetContext(u->pos);
								ctx.units->push_back(u);
								u->in_building = ctx.building_id;
							}
							else
								READ_ERROR("SPAWN_UNIT");
						}
						break;
					// zmiana stanu areny jednostki
					case NetChange::CHANGE_ARENA_STATE:
						{
							int netid;
							char state;
							if(s.Read(netid) && s.Read(state))
							{
								Unit* u = FindUnit(netid);
								if(u)
								{
									if(state < -1 || state > 1)
										state = -1;
									u->in_arena = state;
								}
								else
									WARN(Format("CHANGE_ARENA_STATE, missing unit %d (%d).", netid, state));
							}
							else
								READ_ERROR("CHANGE_ARENA_STATE");
						}
						break;
					// dŸwiêk areny
					case NetChange::ARENA_SOUND:
						{
							byte co;
							if(s.Read(co))
							{
								if(sound_volume && city_ctx && city_ctx->type == L_CITY && GetArena()->ctx.building_id == pc->unit->in_building)
								{
									SOUND snd;
									if(co == 0)
										snd = sArenaFight;
									else if(co == 1)
										snd = sArenaWygrana;
									else
										snd = sArenaPrzegrana;
									PlaySound2d(snd);
								}
							}
							else
								READ_ERROR("ARENA_SOUND");
						}
						break;
					// okrzyk jednostki po zobaczeniu wroga
					case NetChange::SHOUT:
						{
							int netid;
							if(s.Read(netid))
							{
								Unit* u = FindUnit(netid);
								if(u)
								{
									if(sound_volume)
										PlayAttachedSound(*u, u->data->sounds->sound[SOUND_SEE_ENEMY], 3.f, 20.f);
								}
								else
									WARN(Format("SHOUT, missing unit %d.", netid));
							}
							else
								READ_ERROR("SHOUT");
						}
						break;
					// opuszczanie lokacji
					case NetChange::LEAVE_LOCATION:
						fallback_co = FALLBACK_WAIT_FOR_WARP;
						fallback_t = -1.f;
						pc->unit->frozen = 2;
						break;
					// wyjœcie na mapê œwiata
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
									explo->tex = fireball.tex3;
									explo->owner = NULL;

									if(sound_volume)
										PlaySound3d(fireball.sound2, explo->pos, fireball.sound_dist2.x, fireball.sound_dist2.y);

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

								if(	s.ReadCasted<byte>(loc->state) &&
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
									u->etap_animacji = 0;

									if(u->ani->ani->head.n_groups == 2)
									{
										u->ani->frame_end_info2 = false;
										u->ani->Play("cast", PLAY_ONCE|PLAY_PRIO1, 1);
									}
									else
									{
										u->ani->frame_end_info = false;
										u->animacja = ANI_ODTWORZ;
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

							if(	s.Read(netid) &&
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

								if(spell.tex2)
								{
									ParticleEmitter* pe = new ParticleEmitter;
									pe->tex = spell.tex2;
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
									pe->size = spell.size2;
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
									PlaySound3d(spell.sound, pos, spell.sound_dist.x, spell.sound_dist.y);
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
									drain.pe->speed_min = VEC3(-3,0,-3);
									drain.pe->speed_max = VEC3(3,3,3);
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
								if(sound_volume && spell->sound2)
									PlaySound3d(spell->sound2, pos, spell->sound_dist2.x, spell->sound_dist2.y);

								// cz¹steczki
								if(spell->tex2)
								{
									ParticleEmitter* pe = new ParticleEmitter;
									pe->tex = spell->tex2;
									pe->emision_interval = 0.01f;
									pe->life = 0.f;
									pe->particle_life = 0.5f;
									pe->emisions = 1;
									pe->spawn_min = 8;
									pe->spawn_max = 12;
									pe->max_particles = 12;
									pe->pos = pos;
									pe->speed_min = VEC3(-1.5f,-1.5f,-1.5f);
									pe->speed_max = VEC3(1.5f,1.5f,1.5f);
									pe->pos_min = VEC3(-spell->size, -spell->size, -spell->size);
									pe->pos_max = VEC3(spell->size, spell->size, spell->size);
									pe->size = spell->size2;
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
								pe->tex = spell.tex3;
								pe->emision_interval = 0.01f;
								pe->life = 0.f;
								pe->particle_life = 0.5f;
								pe->emisions = 1;
								pe->spawn_min = 16;
								pe->spawn_max = 25;
								pe->max_particles = 25;
								pe->pos = pos;
								pe->speed_min = VEC3(-1.5f,-1.5f,-1.5f);
								pe->speed_max = VEC3(1.5f,1.5f,1.5f);
								pe->pos_min = VEC3(-spell.size, -spell.size, -spell.size);
								pe->pos_max = VEC3(spell.size, spell.size, spell.size);
								pe->size = spell.size2;
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
								pe->tex = spell.tex3;
								pe->emision_interval = 0.01f;
								pe->life = 0.f;
								pe->particle_life = 0.5f;
								pe->emisions = 1;
								pe->spawn_min = 16;
								pe->spawn_max = 25;
								pe->max_particles = 25;
								pe->pos = pos;
								pe->speed_min = VEC3(-1.5f,-1.5f,-1.5f);
								pe->speed_max = VEC3(1.5f,1.5f,1.5f);
								pe->pos_min = VEC3(-spell.size, -spell.size, -spell.size);
								pe->pos_max = VEC3(spell.size, spell.size, spell.size);
								pe->size = spell.size2;
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
								for(word i=0; i<ile; ++i)
								{
									byte x, y;
									if(s.Read(x) && s.Read(y))
										minimap_reveal.push_back(INT2(x,y));
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
					default:
						WARN(Format("Unknown change type %d.", type));
						assert(0);
						break;
					}
				}
			}
			break;
		case ID_PLAYER_UPDATE:
			{
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
					for(byte i=0; i<ile; ++i)
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

								// action
								CloseGamePanels();
								inv_trade_mine->mode = Inventory::LOOT_MY;
								inv_trade_other->mode = Inventory::LOOT_OTHER;
								if(pc->action == PlayerController::Action_LootUnit)
								{
									inventory_mode = I_LOOT_BODY;
									inv_trade_other->unit = pc->action_unit;
									inv_trade_other->items = &pc->action_unit->items;
									inv_trade_other->slots = pc->action_unit->slots;
									inv_trade_other->title = Format("%s - %s", inv_trade_other->txLooting, pc->action_unit->GetName());
								}
								else
								{
									inventory_mode = I_LOOT_CHEST;
									inv_trade_other->unit = NULL;
									inv_trade_other->items = &pc->action_chest->items;
									inv_trade_other->slots = NULL;
									inv_trade_other->title = Inventory::txLootingChest;
								}
								BuildTmpInventory(0);
								BuildTmpInventory(1);
								gp_trade->Show();
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
						case NetChangePlayer::SHOW_CHOICES:
							{
								byte ile;
								char esc;
								if(s.Read(ile) && s.Read(esc))
								{
									dialog_context.choice_selected = 0;
									dialog_context.show_choices = true;
									dialog_context.dialog_esc = esc;
									dialog_choices.resize(ile);
									for(byte i=0; i<ile; ++i)
									{
										if(!ReadString1(s, dialog_choices[i]))
										{
											READ_ERROR("SHOW_CHOICES(2)");
											break;
										}
									}
									game_gui->UpdateScrollbar(dialog_choices.size());
								}
								else
									READ_ERROR("SHOW_CHOICES");
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

									pc->action = PlayerController::Action_Trade;
									pc->action_unit = t;
									pc->chest_trade = &chest_trade;

									if(strcmp(t->data->id, "blacksmith") == 0 || strcmp(t->data->id, "q_orkowie_kowal") == 0)
										trader_buy = blacksmith_buy;
									else if(strcmp(t->data->id, "merchant") == 0 || strcmp(t->data->id, "tut_czlowiek") == 0)
										trader_buy = merchant_buy;
									else if(strcmp(t->data->id, "alchemist") == 0)
										trader_buy = alchemist_buy;
									else if(strcmp(t->data->id, "innkeeper") == 0)
										trader_buy = innkeeper_buy;
									else if(strcmp(t->data->id, "food_seller") == 0)
										trader_buy = foodseller_buy;

									CloseGamePanels();
									inventory_mode = I_TRADE;
									BuildTmpInventory(0);
									inv_trade_mine->mode = Inventory::TRADE_MY;
									BuildTmpInventory(1);
									inv_trade_other->unit = t;
									inv_trade_other->items = pc->chest_trade;
									inv_trade_other->slots = NULL;
									inv_trade_other->title = Format("%s - %s", inv_trade_other->txTrading, t->GetName());
									inv_trade_other->mode = Inventory::TRADE_OTHER;
									gp_trade->Show();
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
										ERROR(Format("ADD_ITEMS_TRADER: Missing unit %d (Item %s, count %u, team count %u).", netid, item->id, count, team_count));
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
										ERROR(Format("ADD_ITEMS_CHEST: Missing chest %d (Item %s, count %u, team count %u).", netid, item->id, count, team_count));
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
								{
									CloseGamePanels();
									pc->action_unit = &u;
									pc->chest_trade = &u.items;
									if(is_share)
									{
										pc->action = PlayerController::Action_ShareItems;
										inventory_mode = I_SHARE;
										inv_trade_mine->mode = Inventory::SHARE_MY;
										inv_trade_other->mode = Inventory::SHARE_OTHER;
										inv_trade_other->title = Format("%s - %s", Inventory::txShareItems, u.GetName());
									}
									else
									{
										pc->action = PlayerController::Action_GiveItems;
										inventory_mode = I_GIVE;
										inv_trade_mine->mode = Inventory::GIVE_MY;
										inv_trade_other->mode = Inventory::GIVE_OTHER;
										inv_trade_other->title = Format("%s - %s", Inventory::txGiveItems, u.GetName());
									}
									BuildTmpInventory(0);
									BuildTmpInventory(1);
									inv_trade_other->unit = &u;
									inv_trade_other->items = &u.items;
									inv_trade_other->slots = u.slots;
									gp_trade->Show();
								}
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
									inv_trade_mine->IsBetterItemResponse(co == 1);
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
						// informacja o zdobyciu atrbutu/umiejêtnoœci
						case NetChangePlayer::GAIN_STAT:
							{
								byte co, ile;
								if(s.Read(co) && s.Read(ile))
									ShowStatGain(co, ile);
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
										WARN(Format("UPDATE_TRADER_GOLD, not trading with %s (%d).", u->data->id, netid));
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
										ERROR(Format("UPDATE_TRADER_INVENTORY, not trading with %s (%d).", u->data->id, netid));
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
			}
			break;
		default:
			WARN(Format("UpdateClient: Unknown packet from server: %s.", PacketToString(packet)));
			break;
		}
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
			net_stream.WriteCasted<byte>(pc->unit->animacja);
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
				net_stream.WriteCasted<byte>(it->id);
				if(it->id == 0)
					net_stream.WriteCasted<byte>(pc->unit->wyjeta);
				else
					net_stream.WriteCasted<byte>(pc->unit->chowana);
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
			case NetChange::TAKE_ITEM_CREDIT:
			case NetChange::IDLE:
			case NetChange::CHOICE:
			case NetChange::ENTER_BUILDING:
			case NetChange::CHANGE_LEADER:
			case NetChange::RANDOM_NUMBER:
			case NetChange::CHEAT_GODMODE:
			case NetChange::CHEAT_INVISIBLE:
			case NetChange::CHEAT_NOCLIP:
			case NetChange::CHEAT_WARP_TO_BUILDING:
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
			case NetChange::CHEAT_ADD_GOLD:
			case NetChange::CHEAT_ADD_GOLD_TEAM:
			case NetChange::CHEAT_SKIP_DAYS:
			case NetChange::PAY_CREDIT:
			case NetChange::DROP_GOLD:
				net_stream.Write(it->id);
				break;
			case NetChange::STOP_TRADE:
			case NetChange::GET_ALL_ITEMS:
			case NetChange::EXIT_BUILDING:
			case NetChange::WARP:
			case NetChange::CHEAT_SUICIDE:
			case NetChange::STAND_UP:
			case NetChange::CHEAT_SCARE:
			case NetChange::CHEAT_CITZEN:
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
			case NetChange::CHEAT_KILL_ALL:
				net_stream.WriteCasted<byte>(it->id);
				if(it->id == 3)
				{
					int netid = (it->unit ? it->unit->netid : -1);
					net_stream.Write(netid);
				}
				break;
			case NetChange::CHEAT_KILL:
			case NetChange::CHEAT_HEAL_UNIT:
			case NetChange::CHEAT_HURT:
			case NetChange::CHEAT_BREAK_ACTION:
			case NetChange::CHEAT_FALL:
				net_stream.Write(it->unit->netid);
				break;
			case NetChange::CHEAT_ADD_ITEM:
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
			case NetChange::CHEAT_MOD_STAT:
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
void Game::Client_Say(Packet* packet)
{
	BitStream s(packet->data+1, packet->length-1, false);
	byte id;

	if(!s.Read(id) || !ReadString1(s))
		WARN(Format("Client_Say: Broken packet: %s.", PacketToString(packet)));
	else
	{
		int index = GetPlayerIndex(id);
		if(index == -1)
			WARN(Format("Client_Say: Broken packet, missing player %d: %s.", id, PacketToString(packet)));
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
void Game::Client_Whisper(Packet* packet)
{
	BitStream s(packet->data+1, packet->length-1, false);
	byte id;

	if(!s.Read(id) || !ReadString1(s))
		WARN(Format("Client_Whisper: Broken packet: %s.", PacketToString(packet)));
	else
	{
		int index = GetPlayerIndex(id);
		if(index == -1)
			WARN(Format("Client_Whisper: Broken packet, missing player %d: %s.", id, PacketToString(packet)));
		else
		{
			cstring s = Format("%s@: %s", game_players[index].name.c_str(), BUF);
			AddMsg(s);
		}
	}
}

//=================================================================================================
void Game::Client_ServerSay(Packet* packet)
{
	BitStream s(packet->data+1, packet->length-1, false);

	if(!ReadString1(s))
		WARN(Format("Client_ServerSay: Broken packet: %s.", PacketToString(packet)));
	else
		AddServerMsg(BUF);
}

//=================================================================================================
void Game::Server_Say(Packet* packet, PlayerInfo& info)
{
	BitStream s(packet->data+1, packet->length-1, false);
	byte id;

	if(!s.Read(id) || !ReadString1(s))
		WARN(Format("Server_Say: Broken packet from %s: %s.", info.name.c_str(), PacketToString(packet)));
	else
	{
		// id gracza jest ignorowane przez serwer ale mo¿na je sprawdziæ
		assert(id == info.id);

		cstring str = Format("%s: %s", info.name.c_str(), BUF);
		AddMsg(str);

		// przeœlij dalej
		if(players > 2)
			peer->Send((cstring)packet->data, packet->length, MEDIUM_PRIORITY, RELIABLE, 0, packet->systemAddress, true);

		if(game_state == GS_LEVEL)
			game_gui->AddSpeechBubble(info.u, BUF);
	}
}

//=================================================================================================
void Game::Server_Whisper(Packet* packet, PlayerInfo& info)
{
	BitStream s(packet->data+1, packet->length-1, false);
	byte id;

	if(!s.Read(id) || !ReadString1(s))
		WARN(Format("Server_Whisper: Broken packet from %s: %s.", info.name.c_str(), PacketToString(packet)));
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
				WARN(Format("Server_Whisper: Broken packet from %s to missing player %d: %s.", id, PacketToString(packet)));
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
	mp_box->Reset();
	mp_box->visible = true;
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
	mp_box->Reset();
	mp_box->visible = true;
}

//=================================================================================================
const Item* Game::FindQuestItemClient(cstring id, int refid) const
{
	assert(id);

	for(vector<QuestItemClient*>::const_iterator it = quest_items.begin(), end = quest_items.end(); it != end; ++it)
	{
		if((*it)->str_id == id && (refid == -1 || refid == (*it)->item->refid))
			return (*it)->item;
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
				s.WriteCasted<byte>(city.citzens);
				s.WriteCasted<word>(city.citzens_world);
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
	for(vector<Location*>::iterator it = locations.begin(), end = locations.end(); it != end; ++it)
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
		if(type >= L_MAX)
		{
			ERROR(Format("Broken packet ID_WORLD_DATA, unknown location type %d.", type));
			return false;
		}

		Location* loc;
		if(type == L_DUNGEON || type == L_CRYPT)
		{
			byte poziomy;
			if(!s.Read(poziomy))
			{
				READ_ERROR;
				return false;
			}

			if(poziomy == 0)
			{
				ERROR("Broken packet ID_WORLD_DATA, location with 0 levels.");
				return false;
			}
			else if(poziomy == 1)
				loc = new SingleInsideLocation;
			else
				loc = new MultiInsideLocation(poziomy);
		}
		else if(type == L_CAVE)
			loc = new CaveLocation;
		else if(type == L_FOREST || type == L_ENCOUNTER || type == L_CAMP || type == L_MOONWELL)
			loc = new OutsideLocation;
		else
		{
			byte citzens;
			word world_citzens;
			if(!s.Read(citzens) || !s.Read(world_citzens))
			{
				READ_ERROR;
				return false;
			}

			if(type == L_CITY)
			{
				City* city = new City;
				loc = city;
				city->citzens = citzens;
				city->citzens_world = world_citzens;
			}
			else
			{
				Village* village = new Village;
				loc = village;
				village->citzens = citzens;
				village->citzens_world = world_citzens;

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
	for(vector<Quest*>::iterator it = quests.begin(), end = quests.end(); it != end; ++it)
	{
		*it = new PlaceholderQuest;
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
	quest_items.resize(ile2);
	for(vector<QuestItemClient*>::iterator it = quest_items.begin(), end = quest_items.end(); it != end; ++it)
	{
		QuestItemClient* item = new QuestItemClient;
		if(ReadString1(s, item->str_id))
		{
			const Item* base;
			if(item->str_id[0] == '$')
				base = FindItem(item->str_id.c_str()+1);
			else
				base = FindItem(item->str_id.c_str());
			if(base)
			{
				item->item = CreateItemCopy(base);
				if(ReadString1(s, item->item->name) &&
					ReadString1(s, item->item->desc) &&
					s.Read(item->item->refid))
				{
					item->item->id = item->str_id.c_str();
					*it = item;
				}
				else
				{
					READ_ERROR;
					delete item;
					quest_items.erase(it, quest_items.end());
					return false;
				}
			}
			else
			{
				READ_ERROR;
				delete item;
				quest_items.erase(it, quest_items.end());
				return false;
			}
		}
		else
		{
			READ_ERROR;
			delete item;
			quest_items.erase(it, quest_items.end());
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
				u->animacja = ANI_STOI;
			}
		}
		else
		{
			if(!u->ani->groups[0].anim && !u->ani->groups[1].anim)
			{
				u->action = A_NONE;
				u->animacja = ANI_STOI;
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
						u->animacja = ANI_STOI;
					}
				}
				else
				{
					if(!u->ani->groups[0].anim && !u->ani->groups[1].anim)
					{
						u->action = A_NONE;
						u->animacja = ANI_STOI;
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
void Game::WriteToInterpolator(EntityInterpolator* e, const VEC3& pos, float rot)
{
	assert(e);

	for(int i=EntityInterpolator::MAX_ENTRIES-1; i>0; --i)
		e->entries[i] = e->entries[i-1];

	e->entries[0].pos = pos;
	e->entries[0].rot = rot;
	e->entries[0].timer = 0.f;

	e->valid_entries = min(e->valid_entries+1, EntityInterpolator::MAX_ENTRIES);
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
bool Game::CheckMoveNet(Unit* u, const VEC3& pos)
{
	assert(u);

	global_col.clear();

	const float radius = u->GetUnitRadius();
	IgnoreObjects ignore = {0};
	Unit* ignored[] = {u, NULL};
	const void* ignored_objects[2] = {NULL, NULL};
	if(u->useable)
		ignored_objects[0] = u->useable;
	ignore.ignored_units = (const Unit**)ignored;
	ignore.ignored_objects = ignored_objects;
	
	GatherCollisionObjects(GetContext(*u), global_col, pos, radius, &ignore);

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
		if(info.left_reason != 1)
		{
			LOG(Format("Player %s left game.", info.name.c_str()));
			AddMsg(Format(txPlayerLeft, info.name.c_str()));
		}

		if(info.u)
		{
			if(info.left_reason == 2 || game_state == GS_WORLDMAP)
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
	case NetChange::CHEAT_ADD_ITEM:
	case NetChange::CHEAT_ADD_GOLD:
	case NetChange::CHEAT_ADD_GOLD_TEAM:
	case NetChange::CHEAT_MOD_STAT:
	case NetChange::CHEAT_REVEAL:
	case NetChange::GAME_OVER:
	case NetChange::CHEAT_CITZEN:
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
