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
inline bool ReadItemSimple(BitStream& stream, const Item*& item)
{
	if(!ReadString1(stream))
		return false;

	if(BUF[0] == '$')
		item = FindItem(BUF+1);
	else
		item = FindItem(BUF);

	return (item != nullptr);
}

//=================================================================================================
inline void WriteBaseItem(BitStream& stream, const Item& item)
{
	WriteString1(stream, item.id);
	if(item.id[0] == '$')
		stream.Write(item.refid);
}

//=================================================================================================
inline void WriteBaseItem(BitStream& stream, const Item* item)
{
	if(item)
		WriteBaseItem(stream, *item);
	else
		stream.WriteCasted<byte>(0);
}

//=================================================================================================
inline void WriteItemList(BitStream& stream, vector<ItemSlot>& items)
{
	stream.Write(items.size());
	for(ItemSlot& slot : items)
	{
		WriteBaseItem(stream, *slot.item);
		stream.Write(slot.count);
	}
}

//=================================================================================================
inline void WriteItemListTeam(BitStream& stream, vector<ItemSlot>& items)
{
	stream.Write(items.size());
	for(ItemSlot& slot : items)
	{
		WriteBaseItem(stream, *slot.item);
		stream.Write(slot.count);
		stream.Write(slot.team_count);
	}
}

//=================================================================================================
bool Game::ReadItemList(BitStream& stream, vector<ItemSlot>& items)
{
	const int MIN_SIZE = 5;

	uint count;
	if(!stream.Read(count)
		|| !EnsureSize(stream, count * MIN_SIZE))
		return false;

	items.resize(count);
	for(ItemSlot& slot : items)
	{
		if(ReadItemAndFind(stream, slot.item) < 1
			|| !stream.Read(slot.count))
			return false;
		slot.team_count = 0;
	}

	return true;
}

//=================================================================================================
bool Game::ReadItemListTeam(BitStream& stream, vector<ItemSlot>& items)
{
	const int MIN_SIZE = 9;

	uint count;
	if(!stream.Read(count)
		|| !EnsureSize(stream, count * MIN_SIZE))
		return false;

	items.resize(count);
	for(ItemSlot& slot : items)
	{
		if(ReadItemAndFind(stream, slot.item) < 1
			|| !stream.Read(slot.count)
			|| !stream.Read(slot.team_count))
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
	DEBUG_DO(peer->SetTimeoutTime(60*60*1000, UNASSIGNED_SYSTEM_ADDRESS));

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

	DEBUG_DO(peer->SetTimeoutTime(60*60*1000, UNASSIGNED_SYSTEM_ADDRESS));
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
void Game::PrepareLevelData(BitStream& stream)
{
	stream.WriteCasted<byte>(ID_LEVEL_DATA);
	WriteBool(stream, mp_load);

	if(location->outside)
	{
		// outside location
		OutsideLocation* outside = (OutsideLocation*)location;
		stream.Write((cstring)outside->tiles, sizeof(TerrainTile)*outside->size*outside->size);
		stream.Write((cstring)outside->h, sizeof(float)*(outside->size + 1)*(outside->size + 1));
		if(location->type == L_CITY || location->type == L_VILLAGE)
		{
			City* city = (City*)location;
			WriteBool(stream, city->have_exit);
			stream.WriteCasted<byte>(city->entry_points.size());
			for(EntryPoint& entry_point : city->entry_points)
			{
				stream.Write(entry_point.exit_area);
				stream.Write(entry_point.exit_y);
			}
			stream.WriteCasted<byte>(city->buildings.size());
			for(CityBuilding& building : city->buildings)
			{
				stream.WriteCasted<byte>(building.type);
				stream.Write(building.pt);
				stream.WriteCasted<byte>(building.rot);
			}
			stream.WriteCasted<byte>(city->inside_buildings.size());
			for(InsideBuilding* inside_building : city->inside_buildings)
			{
				InsideBuilding& ib = *inside_building;
				stream.Write(ib.level_shift);
				stream.WriteCasted<byte>(ib.type);
				// useable objects
				stream.WriteCasted<byte>(ib.useables.size());
				for(Useable* useable : ib.useables)
					useable->Write(stream);
				// units
				stream.WriteCasted<byte>(ib.units.size());
				for(Unit* unit : ib.units)
					WriteUnit(stream, *unit);
				// doors
				stream.WriteCasted<byte>(ib.doors.size());
				for(Door* door : ib.doors)
					WriteDoor(stream, *door);
				// ground items
				stream.WriteCasted<byte>(ib.items.size());
				for(GroundItem* item : ib.items)
					WriteItem(stream, *item);
				// bloods
				stream.WriteCasted<word>(ib.bloods.size());
				for(Blood& blood : ib.bloods)
					blood.Write(stream);
				// objects
				stream.WriteCasted<byte>(ib.objects.size());
				for(Object& object : ib.objects)
					object.Write(stream);
				// lights
				stream.WriteCasted<byte>(ib.lights.size());
				for(Light& light : ib.lights)
					light.Write(stream);
				// other
				stream.Write(ib.xsphere_pos);
				stream.Write(ib.enter_area);
				stream.Write(ib.exit_area);
				stream.Write(ib.top);
				stream.Write(ib.xsphere_radius);
				stream.WriteCasted<byte>(ib.type);
				stream.Write(ib.enter_y);
			}
		}
		stream.Write(light_angle);
	}
	else
	{
		// inside location
		InsideLocation* inside = (InsideLocation*)location;
		InsideLocationLevel& lvl = inside->GetLevelData();
		stream.WriteCasted<byte>(inside->target);
		WriteBool(stream, inside->from_portal);
		// map
		stream.WriteCasted<byte>(lvl.w);
		stream.Write((cstring)lvl.map, sizeof(Pole)*lvl.w*lvl.h);
		// lights
		stream.WriteCasted<byte>(lvl.lights.size());
		for(Light& light : lvl.lights)
			light.Write(stream);
		// rooms
		stream.WriteCasted<byte>(lvl.rooms.size());
		for(Room& room : lvl.rooms)
			room.Write(stream);
		// traps
		stream.WriteCasted<byte>(lvl.traps.size());
		for(Trap* trap : lvl.traps)
			WriteTrap(stream, *trap);
		// doors
		stream.WriteCasted<byte>(lvl.doors.size());
		for(Door* door : lvl.doors)
			WriteDoor(stream, *door);
		// stairs
		stream.Write(lvl.staircase_up);
		stream.Write(lvl.staircase_down);
		stream.WriteCasted<byte>(lvl.staircase_up_dir);
		stream.WriteCasted<byte>(lvl.staircase_down_dir);
		WriteBool(stream, lvl.staircase_down_in_wall);
	}

	// useable objects
	stream.WriteCasted<byte>(local_ctx.useables->size());
	for(Useable* useable : *local_ctx.useables)
		useable->Write(stream);
	// units
	stream.WriteCasted<byte>(local_ctx.units->size());
	for(Unit* unit : *local_ctx.units)
		WriteUnit(stream, *unit);
	// ground items
	stream.WriteCasted<byte>(local_ctx.items->size());
	for(GroundItem* item : *local_ctx.items)
		WriteItem(stream, *item);
	// bloods
	stream.WriteCasted<word>(local_ctx.bloods->size());
	for(Blood& blood : *local_ctx.bloods)
		blood.Write(stream);
	// objects
	stream.WriteCasted<word>(local_ctx.objects->size());
	for(Object& object : *local_ctx.objects)
		object.Write(stream);
	// chests
	stream.WriteCasted<byte>(local_ctx.chests->size());
	for(Chest* chest : *local_ctx.chests)
		WriteChest(stream, *chest);

	location->WritePortals(stream);

	// saved bullets, spells, explosions etc
	if(mp_load)
	{
		// bullets
		stream.WriteCasted<byte>(local_ctx.bullets->size());
		for(Bullet& bullet : *local_ctx.bullets)
		{
			stream.Write(bullet.pos);
			stream.Write(bullet.rot);
			stream.Write(bullet.speed);
			stream.Write(bullet.yspeed);
			stream.Write(bullet.timer);
			stream.Write(bullet.owner ? bullet.owner->netid : -1);
			if(bullet.spell)
				WriteString1(stream, bullet.spell->id);
			else
				stream.Write0();
		}

		// explosions
		stream.WriteCasted<byte>(local_ctx.explos->size());
		for(Explo* explo : *local_ctx.explos)
		{
			WriteString1(stream, explo->tex->filename);
			stream.Write(explo->pos);
			stream.Write(explo->size);
			stream.Write(explo->sizemax);
		}

		// electros
		stream.WriteCasted<byte>(local_ctx.electros->size());
		for(Electro* electro : *local_ctx.electros)
		{
			stream.Write(electro->netid);
			stream.WriteCasted<byte>(electro->lines.size());
			for(ElectroLine& line : electro->lines)
			{
				stream.Write(line.pts.front());
				stream.Write(line.pts.back());
				stream.Write(line.t);
			}
		}
	}

	stream.WriteCasted<byte>(GetLocationMusic());
	stream.WriteCasted<byte>(0xFF);
}

//=================================================================================================
void Game::WriteUnit(BitStream& stream, Unit& unit)
{
	// main
	WriteString1(stream, unit.data->id);
	stream.Write(unit.netid);

	// human data
	if(unit.type == Unit::HUMAN)
	{
		stream.WriteCasted<byte>(unit.human_data->hair);
		stream.WriteCasted<byte>(unit.human_data->beard);
		stream.WriteCasted<byte>(unit.human_data->mustache);
		stream.Write(unit.human_data->hair_color);
		stream.Write(unit.human_data->height);
	}

	// items
	if(unit.type != Unit::ANIMAL)
	{
		byte zero = 0;
		if(unit.HaveWeapon())
			WriteString1(stream, unit.GetWeapon().id);
		else
			stream.Write(zero);
		if(unit.HaveBow())
			WriteString1(stream, unit.GetBow().id);
		else
			stream.Write(zero);
		if(unit.HaveShield())
			WriteString1(stream, unit.GetShield().id);
		else
			stream.Write(zero);
		if(unit.HaveArmor())
			WriteString1(stream, unit.GetArmor().id);
		else
			stream.Write(zero);
	}
	stream.WriteCasted<byte>(unit.live_state);
	stream.Write(unit.pos);
	stream.Write(unit.rot);
	stream.Write(unit.hp);
	stream.Write(unit.hpmax);
	stream.Write(unit.netid);
	stream.WriteCasted<char>(unit.in_arena);

	// hero/player data
	byte b;
	if(unit.IsHero())
		b = 1;
	else if(unit.IsPlayer())
		b = 2;
	else
		b = 0;
	stream.Write(b);
	if(unit.IsHero())
	{
		WriteString1(stream, unit.hero->name);
		stream.WriteCasted<byte>(unit.hero->clas);
		b = 0;
		if(unit.hero->know_name)
			b |= 0x01;
		if(unit.hero->team_member)
			b |= 0x02;
		stream.Write(b);
		stream.Write(unit.hero->credit);
	}
	else if(unit.IsPlayer())
	{
		WriteString1(stream, unit.player->name);
		stream.WriteCasted<byte>(unit.player->clas);
		stream.WriteCasted<byte>(unit.player->id);
		stream.Write(unit.player->credit);
		stream.Write(unit.player->free_days);
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
		stream.Write(b);
	}

	// loaded data
	if(mp_load)
	{
		stream.Write(unit.netid);
		unit.ani->Write(stream);
		stream.WriteCasted<byte>(unit.animation);
		stream.WriteCasted<byte>(unit.current_animation);
		stream.WriteCasted<byte>(unit.animation_state);
		stream.WriteCasted<byte>(unit.attack_id);
		stream.WriteCasted<byte>(unit.action);
		stream.WriteCasted<byte>(unit.weapon_taken);
		stream.WriteCasted<byte>(unit.weapon_hiding);
		stream.WriteCasted<byte>(unit.weapon_state);
		stream.Write(unit.target_pos);
		stream.Write(unit.target_pos2);
		stream.Write(unit.timer);
		if(unit.used_item)
			WriteString1(stream, unit.used_item->id);
		else
			stream.WriteCasted<byte>(0);
		stream.Write(unit.useable ? unit.useable->netid : -1);
	}
}

//=================================================================================================
void Game::WriteDoor(BitStream& stream, Door& door)
{
	stream.Write(door.pos);
	stream.Write(door.rot);
	stream.Write(door.pt);
	stream.WriteCasted<byte>(door.locked);
	stream.WriteCasted<byte>(door.state);
	stream.Write(door.netid);
	WriteBool(stream, door.door2);
}

//=================================================================================================
void Game::WriteItem(BitStream& stream, GroundItem& item)
{
	stream.Write(item.netid);
	stream.Write(item.pos);
	stream.Write(item.rot);
	stream.Write(item.count);
	stream.Write(item.team_count);
	WriteString1(stream, item.item->id);
	if(item.item->IsQuest())
		stream.Write(item.item->refid);
}

//=================================================================================================
void Game::WriteChest(BitStream& stream, Chest& chest)
{
	stream.Write(chest.pos);
	stream.Write(chest.rot);
	stream.Write(chest.netid);
}

//=================================================================================================
void Game::WriteTrap(BitStream& stream, Trap& trap)
{
	stream.WriteCasted<byte>(trap.base->type);
	stream.WriteCasted<byte>(trap.dir);
	stream.Write(trap.netid);
	stream.Write(trap.tile);
	stream.Write(trap.pos);
	stream.Write(trap.obj.rot.y);

	if(mp_load)
	{
		stream.WriteCasted<byte>(trap.state);
		stream.Write(trap.time);
	}
}

//=================================================================================================
bool Game::ReadLevelData(BitStream& stream)
{
	cam.Reset();
	player_rot_buf = 0.f;
	show_mp_panel = true;
	boss_level_mp = false;
	open_location = 0;

	if(!ReadBool(stream, mp_load))
	{
		ERROR("Read level: Broken packet for loading info.");
		return false;
	}

	if(!location->outside)
	{
		InsideLocation* inside = (InsideLocation*)location;
		inside->SetActiveLevel(dungeon_level);
	}
	ApplyContext(location, local_ctx);
	local_ctx_valid = true;
	city_ctx = nullptr;

	if(location->outside)
	{
		// outside location
		SetOutsideParams();

		OutsideLocation* outside = (OutsideLocation*)location;
		int size11 = outside->size*outside->size;
		int size22 = outside->size + 1;
		size22 *= size22;
		if(!outside->tiles)
			outside->tiles = new TerrainTile[size11];
		if(!outside->h)
			outside->h = new float[size22];
		if(!stream.Read((char*)outside->tiles, sizeof(TerrainTile)*size11)
			|| !stream.Read((char*)outside->h, sizeof(float)*size22))
		{
			ERROR("Read level: Broken packet for terrain.");
			return false;
		}
		ApplyTiles(outside->h, outside->tiles);
		SpawnTerrainCollider();
		if(location->type == L_CITY || location->type == L_VILLAGE)
		{
			City* city = (City*)location;
			city_ctx = city;

			// entry points
			const int ENTRY_POINT_MIN_SIZE = 20;
			byte count;
			if(!ReadBool(stream, city->have_exit)
				|| !stream.Read(count)
				|| !EnsureSize(stream, count * ENTRY_POINT_MIN_SIZE))
			{
				ERROR("Read level: Broken packet for city.");
				return false;
			}
			city->entry_points.resize(count);
			for(EntryPoint& entry : city->entry_points)
			{
				if(!stream.Read(entry.exit_area)
					|| !stream.Read(entry.exit_y))
				{
					ERROR("Read level: Broken packet for entry points.");
					return false;
				}
			}

			// buildings
			const int BUILDING_MIN_SIZE = 10;
			if(!stream.Read(count)
				|| !EnsureSize(stream, BUILDING_MIN_SIZE * count))
			{
				ERROR("Read level: Broken packet for buildings count.");
				return false;
			}
			city->buildings.resize(count);
			for(CityBuilding& building : city->buildings)
			{
				if(!stream.ReadCasted<byte>(building.type)
					|| !stream.Read(building.pt)
					|| !stream.ReadCasted<byte>(building.rot))
				{
					ERROR("Read level: Broken packet for buildings.");
					return false;
				}
				if(building.type >= B_MAX)
				{
					ERROR(Format("Read level: Invalid building type %d.", building.type));
					return false;
				}
			}

			// inside buildings
			const int INSIDE_BUILDING_MIN_SIZE = 73;
			if(!stream.Read(count)
				|| !EnsureSize(stream, INSIDE_BUILDING_MIN_SIZE * count))
			{
				ERROR("Read level: Broken packet for inside buildings count.");
				return false;
			}
			city->inside_buildings.resize(count);
			int index = 0;
			for(InsideBuilding*& ib_ptr : city->inside_buildings)
			{
				ib_ptr = new InsideBuilding;
				InsideBuilding& ib = *ib_ptr;
				ApplyContext(ib_ptr, ib_ptr->ctx);
				ib.ctx.building_id = index;
				if(!stream.Read(ib.level_shift)
					|| !stream.ReadCasted<byte>(ib.type))
				{
					ERROR(Format("Read level: Broken packet for %d inside building.", index));
					return false;
				}
				if(ib.type >= B_MAX
					|| !buildings[ib.type].inside_mesh)
				{
					ERROR(Format("Read level: Invalid building type %d for building %d.", ib.type, index));
					return false;
				}
				ib.offset = VEC2(512.f*ib.level_shift.x + 256.f, 512.f*ib.level_shift.y + 256.f);
				ProcessBuildingObjects(ib.ctx, city_ctx, &ib, buildings[ib.type].inside_mesh, nullptr, 0, 0, VEC3(ib.offset.x, 0, ib.offset.y), ib.type, nullptr, true);

				// useable objects
				if(!stream.Read(count)
					|| !EnsureSize(stream, Useable::MIN_SIZE * count))
				{
					ERROR(Format("Read level: Broken packet for useable object in %d inside building.", index));
					return false;
				}
				ib.useables.resize(count);
				for(Useable*& useable : ib.useables)
				{
					useable = new Useable;
					if(!useable->Read(stream))
					{
						ERROR(Format("Read level: Broken packet for useable object in %d inside building.", index));
						return false;
					}
				}

				// units
				if(!stream.Read(count)
					|| !EnsureSize(stream, Unit::MIN_SIZE * count))
				{
					ERROR(Format("Read level: Broken packet for unit count in %d inside building.", index));
					return false;
				}
				ib.units.resize(count);
				for(Unit*& unit : ib.units)
				{
					unit = new Unit;
					if(!ReadUnit(stream, *unit))
					{
						ERROR(Format("Read level: Broken packet for unit in %d inside building.", index));
						return false;
					}
					unit->in_building = index;
				}

				// doors
				if(!stream.Read(count)
					|| !EnsureSize(stream, count * Door::MIN_SIZE))
				{
					ERROR(Format("Read level: Broken packet for door count in %d inside building.", index));
					return false;
				}
				ib.doors.resize(count);
				for(Door*& door : ib.doors)
				{
					door = new Door;
					if(!ReadDoor(stream, *door))
					{
						ERROR(Format("Read level: Broken packet for door in %d inside building.", index));
						return false;
					}
				}

				// ground items
				if(!stream.Read(count)
					|| !EnsureSize(stream, count * GroundItem::MIN_SIZE))
				{
					ERROR(Format("Read level: Broken packet for ground item count in %d inside building.", index));
					return false;
				}
				ib.items.resize(count);
				for(GroundItem*& item : ib.items)
				{
					item = new GroundItem;
					if(!ReadItem(stream, *item))
					{
						ERROR(Format("Read level: Broken packet for ground item in %d inside building.", index));
						return false;
					}
				}

				// bloods
				word count2;
				if(!stream.Read(count2)
					|| !EnsureSize(stream, count2 * Blood::MIN_SIZE))
				{
					ERROR(Format("Read level: Broken packet for blood count in %d inside building.", index));
					return false;
				}
				ib.bloods.resize(count2);
				for(Blood& blood : ib.bloods)
				{
					if(!blood.Read(stream))
					{
						ERROR(Format("Read level: Broken packet for blood in %d inside building.", index));
						return false;
					}
				}

				// objects
				if(!stream.Read(count)
					|| !EnsureSize(stream, count * Object::MIN_SIZE))
				{
					ERROR(Format("Read level: Broken packet for object count in %d inside building.", index));
					return false;
				}
				ib.objects.resize(count);
				for(Object& object : ib.objects)
				{
					if(!object.Read(stream))
					{
						ERROR(Format("Read level: Broken packet for object in %d inside building.", index));
						return false;
					}
				}

				// lights
				if(!stream.Read(count)
					|| !EnsureSize(stream, count * Light::MIN_SIZE))
				{
					ERROR(Format("Read level: Broken packet for light count in %d inside building.", index));
					return false;
				}
				ib.lights.resize(count);
				for(Light& light : ib.lights)
				{
					if(!light.Read(stream))
					{
						ERROR(Format("Read level: Broken packet for light in %d inside building.", index));
						return false;
					}
				}

				// other
				if(!stream.Read(ib.xsphere_pos)
					|| !stream.Read(ib.enter_area)
					|| !stream.Read(ib.exit_area)
					|| !stream.Read(ib.top)
					|| !stream.Read(ib.xsphere_radius)
					|| !stream.ReadCasted<byte>(ib.type)
					|| !stream.Read(ib.enter_y))
				{
					ERROR(Format("Read level: Broken packet for inside building %d other data.", index));
					return false;
				}

				++index;
			}

			SpawnCityPhysics();
			RespawnBuildingPhysics();
			CreateCityMinimap();
		}
		else
			CreateForestMinimap();
		if(!stream.Read(light_angle))
		{
			ERROR("Read level: Broken packet for light angle.");
			return false;
		}
		SpawnOutsideBariers();
	}
	else
	{
		// inside location
		InsideLocation* inside = (InsideLocation*)location;
		InsideLocationLevel& lvl = inside->GetLevelData();
		if(!stream.ReadCasted<byte>(inside->target)
			|| !ReadBool(stream, inside->from_portal)
			|| !stream.ReadCasted<byte>(lvl.w)
			|| !EnsureSize(stream, lvl.w * lvl.w * sizeof(Pole)))
		{
			ERROR("Read level: Broken packet for inside location.");
			return false;
		}

		// map
		lvl.h = lvl.w;
		if(!lvl.map)
			lvl.map = new Pole[lvl.w * lvl.h];
		if(!stream.Read((char*)lvl.map, lvl.w * lvl.h * sizeof(Pole)))
		{
			ERROR("Read level: Broken packet for inside location map.");
			return false;
		}

		// lights
		byte count;
		if(!stream.Read(count)
			|| !EnsureSize(stream, count * Light::MIN_SIZE))
		{
			ERROR("Read level: Broken packet for inside location light count.");
			return false;
		}
		lvl.lights.resize(count);
		for(Light& light : lvl.lights)
		{
			if(!light.Read(stream))
			{
				ERROR("Read level: Broken packet for inside location light.");
				return false;
			}
		}

		// rooms
		if(!stream.Read(count)
			|| !EnsureSize(stream, count * Room::MIN_SIZE))
		{
			ERROR("Read level: Broken packet for inside location room count.");
			return false;
		}
		lvl.rooms.resize(count);
		for(Room& room : lvl.rooms)
		{
			if(!room.Read(stream))
			{
				ERROR("Read level: Broken packet for inside location room.");
				return false;
			}
		}

		// traps
		if(!stream.Read(count)
			|| !EnsureSize(stream, count * Trap::MIN_SIZE))
		{
			ERROR("Read level: Broken packet for inside location trap count.");
			return false;
		}
		lvl.traps.resize(count);
		for(Trap*& trap : lvl.traps)
		{
			trap = new Trap;
			if(!ReadTrap(stream, *trap))
			{
				ERROR("Read level: Broken packet for inside location trap.");
				return false;
			}
		}

		// doors
		if(!stream.Read(count)
			|| !EnsureSize(stream, count * Door::MIN_SIZE))
		{
			ERROR("Read level: Broken packet for inside location door count.");
			return false;
		}
		lvl.doors.resize(count);
		for(Door*& door : lvl.doors)
		{
			door = new Door;
			if(!ReadDoor(stream, *door))
			{
				ERROR("Read level: Broken packet for inside location door.");
				return false;
			}
		}

		// stairs
		if(!stream.Read(lvl.staircase_up)
			|| !stream.Read(lvl.staircase_down)
			|| !stream.ReadCasted<byte>(lvl.staircase_up_dir)
			|| !stream.ReadCasted<byte>(lvl.staircase_down_dir)
			|| !ReadBool(stream, lvl.staircase_down_in_wall))
		{
			ERROR("Read level: Broken packet for stairs.");
			return false;
		}

		BaseLocation& base = g_base_locations[inside->target];
		SetDungeonParamsAndTextures(base);

		SpawnDungeonColliders();
		CreateDungeonMinimap();
	}

	// useable objects
	byte count;
	if(!stream.Read(count)
		|| !EnsureSize(stream, count * Useable::MIN_SIZE))
	{
		ERROR("Read level: Broken useable object count.");
		return false;
	}
	local_ctx.useables->resize(count);
	for(Useable*& useable : *local_ctx.useables)
	{
		useable = new Useable;
		if(!useable->Read(stream))
		{
			ERROR("Read level: Broken useable object.");
			return false;
		}
	}

	// units
	if(!stream.Read(count)
		|| !EnsureSize(stream, count * Unit::MIN_SIZE))
	{
		ERROR("Read level: Broken unit count.");
		return false;
	}
	local_ctx.units->resize(count);
	for(Unit*& unit : *local_ctx.units)
	{
		unit = new Unit;
		if(!ReadUnit(stream, *unit))
		{
			ERROR("Read level: Broken unit.");
			return false;
		}
	}

	// ground items
	if(!stream.Read(count)
		|| !EnsureSize(stream, count * GroundItem::MIN_SIZE))
	{
		ERROR("Read level: Broken ground item count.");
		return false;
	}
	local_ctx.items->resize(count);
	for(GroundItem*& item : *local_ctx.items)
	{
		item = new GroundItem;
		if(!ReadItem(stream, *item))
		{
			ERROR("Read level: Broken ground item.");
			return false;
		}
	}

	// bloods
	word count2;
	if(!stream.Read(count2)
		|| !EnsureSize(stream, count2 * Blood::MIN_SIZE))
	{
		ERROR("Read level: Broken blood count.");
		return false;
	}
	local_ctx.bloods->resize(count2);
	for(Blood& blood : *local_ctx.bloods)
	{
		if(!blood.Read(stream))
		{
			ERROR("Read level: Broken blood.");
			return false;
		}
	}

	// objects
	if(!stream.Read(count2)
		|| !EnsureSize(stream, count2 * Object::MIN_SIZE))
	{
		ERROR("Read level: Broken object count.");
		return false;
	}
	local_ctx.objects->resize(count2);
	for(Object& object : *local_ctx.objects)
	{
		if(!object.Read(stream))
		{
			ERROR("Read level: Broken object.");
			return false;
		}
	}

	// chests
	if(!stream.Read(count)
		|| !EnsureSize(stream, count * Chest::MIN_SIZE))
	{
		ERROR("Read level: Broken chest count.");
		return false;
	}
	local_ctx.chests->resize(count);
	for(Chest*& chest : *local_ctx.chests)
	{
		chest = new Chest;
		if(!ReadChest(stream, *chest))
		{
			ERROR("Read level: Broken chest.");
			return false;
		}
	}

	// portals
	if(!location->ReadPortals(stream, dungeon_level))
	{
		ERROR("Read level: Broken portals.");
		return false;
	}

	// multiplayer data
	if(mp_load)
	{
		// bullets
		if(!stream.Read(count)
			|| !EnsureSize(stream, count * Bullet::MIN_SIZE))
		{
			ERROR("Read level: Broken bullet count.");
			return false;
		}
		local_ctx.bullets->resize(count);
		for(Bullet& bullet : *local_ctx.bullets)
		{
			int netid;
			if(!stream.Read(bullet.pos)
				|| !stream.Read(bullet.rot)
				|| !stream.Read(bullet.speed)
				|| !stream.Read(bullet.yspeed)
				|| !stream.Read(bullet.timer)
				|| !stream.Read(netid)
				|| !ReadString1(stream))
			{
				ERROR("Read level: Broken bullet.");
				return false;
			}
			if(BUF[0] == 0)
			{
				bullet.spell = nullptr;
				bullet.mesh = aArrow;
				bullet.pe = nullptr;
				bullet.remove = false;
				bullet.tex = nullptr;
				bullet.tex_size = 0.f;

				TrailParticleEmitter* tpe = new TrailParticleEmitter;
				tpe->fade = 0.3f;
				tpe->color1 = VEC4(1,1,1,0.5f);
				tpe->color2 = VEC4(1,1,1,0);
				tpe->Init(50);
				local_ctx.tpes->push_back(tpe);
				bullet.trail = tpe;

				TrailParticleEmitter* tpe2 = new TrailParticleEmitter;
				tpe2->fade = 0.3f;
				tpe2->color1 = VEC4(1,1,1,0.5f);
				tpe2->color2 = VEC4(1,1,1,0);
				tpe2->Init(50);
				local_ctx.tpes->push_back(tpe2);
				bullet.trail2 = tpe2;
			}
			else
			{
				Spell* spell_ptr = FindSpell(BUF);
				if(!spell_ptr)
				{
					ERROR(Format("Read level: Missing spell '%s'.", BUF));
					return false;
				}

				Spell& spell = *spell_ptr;
				bullet.spell = &spell;				
				bullet.mesh = spell.mesh;
				bullet.tex = spell.tex;
				bullet.tex_size = spell.size;
				bullet.remove = false;
				bullet.trail = nullptr;
				bullet.trail2 = nullptr;
				bullet.pe = nullptr;

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
					pe->pos = bullet.pos;
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
					bullet.pe = pe;
				}
			}

			if(netid != -1)
			{
				bullet.owner = FindUnit(netid);
				if(!bullet.owner)
				{
					ERROR(Format("Read level: Missing bullet owner %d.", netid));
					return false;
				}
			}
			else
				bullet.owner = nullptr;
		}

		// explosions
		if(!stream.Read(count)
			|| !EnsureSize(stream, count * Explo::MIN_SIZE))
		{
			ERROR("Read level: Broken explosion count.");
			return false;
		}
		local_ctx.explos->resize(count);
		for(Explo*& explo : *local_ctx.explos)
		{
			explo = new Explo;
			if(!ReadString1(stream)
				|| !stream.Read(explo->pos)
				|| !stream.Read(explo->size)
				|| !stream.Read(explo->sizemax))
			{
				ERROR("Read level: Broken explosion.");
				return false;
			}

			explo->tex = resMgr.GetLoadedTexture(BUF);
		}

		// electro
		if(!stream.Read(count)
			|| !EnsureSize(stream, count * Electro::MIN_SIZE))
		{
			ERROR("Read level: Broken electro count.");
			return false;
		}
		local_ctx.electros->resize(count);
		Spell* electro_spell = FindSpell("thunder_bolt");
		for(Electro*& electro : *local_ctx.electros)
		{
			electro = new Electro;
			electro->spell = electro_spell;
			electro->valid = true;
			if(!stream.Read(electro->netid)
				|| !stream.Read(count)
				|| !EnsureSize(stream, count * Electro::LINE_MIN_SIZE))
			{
				ERROR("Read level: Broken electro.");
				return false;
			}
			electro->lines.resize(count);
			VEC3 from, to;
			float t;
			for(byte i=0; i<count; ++i)
			{
				stream.Read(from);
				stream.Read(to);
				stream.Read(t);
				electro->AddLine(from, to);
				electro->lines.back().t = t;
			}
		}
	}

	// music
	MusicType music;
	if(!stream.ReadCasted<byte>(music))
	{
		ERROR("Read level: Broken music.");
		return false;
	}
	if(!nomusic)
		LoadMusic(music, false);

	// checksum
	byte check;
	if(!stream.Read(check) || check != 0xFF)
	{
		ERROR("Read level: Broken checksum.");
		return false;
	}

	RespawnObjectColliders();
	local_ctx_valid = true;
	if(!boss_level_mp)
		SetMusic(music);

	InitQuadTree();

	return true;
}

//=================================================================================================
bool Game::ReadUnit(BitStream& stream, Unit& unit)
{
	// main
	if(!ReadString1(stream)
		|| !stream.Read(unit.netid))
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

	// human data
	if(unit.type == Unit::HUMAN)
	{
		unit.human_data = new Human;
		if(!stream.ReadCasted<byte>(unit.human_data->hair)
			|| !stream.ReadCasted<byte>(unit.human_data->beard)
			|| !stream.ReadCasted<byte>(unit.human_data->mustache)
			|| !stream.Read(unit.human_data->hair_color)
			|| !stream.Read(unit.human_data->height))
			return false;
		if(unit.human_data->hair == 0xFF)
			unit.human_data->hair = -1;
		if(unit.human_data->beard == 0xFF)
			unit.human_data->beard = -1;
		if(unit.human_data->mustache == 0xFF)
			unit.human_data->mustache = -1;
		if(unit.human_data->hair < -1
			|| unit.human_data->hair >= MAX_HAIR
			|| unit.human_data->beard < -1
			|| unit.human_data->beard >= MAX_BEARD
			|| unit.human_data->mustache < -1
			|| unit.human_data->mustache >= MAX_MUSTACHE
			|| !in_range(unit.human_data->height, 0.85f, 1.15f))
		{
			ERROR(Format("Invalid human data (hair:%d, beard:%d, mustache:%d, height:%g).", unit.human_data->hair, unit.human_data->beard,
				unit.human_data->mustache, unit.human_data->height));
			return false;
		}
		unit.ani = new AnimeshInstance(aHumanBase);
		unit.human_data->ApplyScale(aHumanBase);
	}
	else
	{
		unit.ani = new AnimeshInstance(unit.data->mesh);
		unit.human_data = nullptr;
	}

	// equipped items
	if(unit.type != Unit::ANIMAL)
	{
		for(int i=0; i<SLOT_MAX; ++i)
		{
			if(!ReadString1(stream))
				return false;
			if(!BUF[0])
				unit.slots[i] = nullptr;
			else
			{
				const Item* item = FindItem(BUF);
				if(item && ItemTypeToSlot(item->type) == (ITEM_SLOT)i)
					unit.slots[i] = item;
				else
				{
					if(item)
						ERROR(Format("Invalid slot type (%d != %d).", ItemTypeToSlot(item->type), i));
					return false;
				}
			}
		}
	}
	else
	{
		for(int i=0; i<SLOT_MAX; ++i)
			unit.slots[i] = nullptr;
	}

	// variables
	if(!stream.ReadCasted<byte>(unit.live_state)
		|| !stream.Read(unit.pos)
		|| !stream.Read(unit.rot)
		|| !stream.Read(unit.hp)
		|| !stream.Read(unit.hpmax)
		|| !stream.Read(unit.netid)
		|| !stream.ReadCasted<char>(unit.in_arena))
		return false;
	if(unit.live_state >= Unit::LIVESTATE_MAX)
	{
		ERROR(Format("Invalid live state %d.", unit.live_state));
		return false;
	}

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
	unit.SetAnimationAtEnd();
	unit.ani->ptr = &unit;

	// hero/player data
	byte type;
	if(!stream.Read(type))
		return false;
	if(type == 1)
	{
		// hero
		byte flags;
		unit.ai = (AIController*)1; // (X_X)
		unit.player = nullptr;
		unit.hero = new HeroData;
		unit.hero->unit = &unit;
		if(!ReadString1(stream, unit.hero->name)
			|| !stream.ReadCasted<byte>(unit.hero->clas)
			|| !stream.Read(flags)
			|| !stream.Read(unit.hero->credit))
			return false;
		if(unit.hero->clas >= Class::MAX)
		{
			ERROR(Format("Invalid hero class %d.", unit.hero->clas));
			return false;
		}
		unit.hero->know_name = IS_SET(flags, 0x01);
		unit.hero->team_member = IS_SET(flags, 0x02);
	}
	else if(type == 2)
	{
		// player
		unit.ai = nullptr;
		unit.hero = nullptr;
		unit.player = new PlayerController;
		unit.player->unit = &unit;
		if(!ReadString1(stream, unit.player->name) ||
			!stream.ReadCasted<byte>(unit.player->clas) ||
			!stream.ReadCasted<byte>(unit.player->id) ||
			!stream.Read(unit.player->credit) ||
			!stream.Read(unit.player->free_days))
			return false;
		if(unit.player->credit < 0)
		{
			ERROR(Format("Invalid player %d credit %d.", unit.player->id, unit.player->credit));
			return false;
		}
		if(unit.player->free_days < 0)
		{
			ERROR(Format("Invalid player %d free days %d.", unit.player->id, unit.player->free_days));
			return false;
		}
		if(!ClassInfo::IsPickable(unit.player->clas))
		{
			ERROR(Format("Invalid player %d class %d.", unit.player->id, unit.player->clas));
			return false;
		}
		PlayerInfo* info = GetPlayerInfoTry(unit.player->id);
		if(!info)
		{
			ERROR(Format("Invalid player id %d.", unit.player->id));
			return false;
		}
		info->u = &unit;
	}
	else
	{
		// ai
		unit.ai = (AIController*)1; // (X_X)
		unit.hero = nullptr;
		unit.player = nullptr;
	}

	// ai variables
	if(unit.IsAI())
	{
		if(!stream.ReadCasted<byte>(unit.ai_mode))
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
	unit.useable = nullptr;
	unit.used_item = nullptr;
	unit.bow_instance = nullptr;
	unit.ai = nullptr;
	unit.animation = ANI_STAND;
	unit.current_animation = ANI_STAND;
	unit.timer = 0.f;
	unit.to_remove = false;
	unit.bubble = nullptr;
	unit.interp = interpolators.Get();
	unit.interp->Reset(unit.pos, unit.rot);
	unit.visual_pos = unit.pos;
	unit.animation_state = 0;

	if(mp_load)
	{
		// get current state in multiplayer
		int useable_netid;
		if(!stream.Read(unit.netid)
			|| !unit.ani->Read(stream)
			|| !stream.ReadCasted<byte>(unit.animation)
			|| !stream.ReadCasted<byte>(unit.current_animation)
			|| !stream.ReadCasted<byte>(unit.animation_state)
			|| !stream.ReadCasted<byte>(unit.attack_id)
			|| !stream.ReadCasted<byte>(unit.action)
			|| !stream.ReadCasted<byte>(unit.weapon_taken)
			|| !stream.ReadCasted<byte>(unit.weapon_hiding)
			|| !stream.ReadCasted<byte>(unit.weapon_state)
			|| !stream.Read(unit.target_pos)
			|| !stream.Read(unit.target_pos2)
			|| !stream.Read(unit.timer)
			|| !ReadString1(stream)
			|| !stream.Read(useable_netid))
			return false;
		else
		{
			// used item
			if(BUF[0])
			{
				unit.used_item = FindItem(BUF);
				if(!unit.used_item)
				{
					ERROR(Format("Missing used item '%s'.", BUF));
					return false;
				}
			}
			else
				unit.used_item = nullptr;

			// useable
			if(useable_netid == -1)
				unit.useable = nullptr;
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
				{
					ERROR(Format("Missing useable %d.", useable_netid));
					return false;
				}
			}

			// bow animesh instance
			if(unit.action == A_SHOOT)
			{
				unit.bow_instance = GetBowInstance(unit.GetBow().mesh);
				unit.bow_instance->Play(&unit.bow_instance->ani->anims[0], PLAY_ONCE|PLAY_PRIO1|PLAY_NO_BLEND, 0);
				unit.bow_instance->groups[0].speed = unit.ani->groups[1].speed;
				unit.bow_instance->groups[0].time = unit.ani->groups[1].time;
			}
		}		
	}

	// physics
	btCapsuleShape* caps = new btCapsuleShape(unit.GetUnitRadius(), max(MIN_H, unit.GetUnitHeight()));
	unit.cobj = new btCollisionObject;
	unit.cobj->setCollisionShape(caps);
	unit.cobj->setUserPointer(this);
	unit.cobj->setCollisionFlags(CG_UNIT);
	phy_world->addCollisionObject(unit.cobj);
	UpdateUnitPhysics(unit, unit.IsAlive() ? unit.pos : VEC3(1000,1000,1000));

	// boss music
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
bool Game::ReadDoor(BitStream& stream, Door& door)
{
	if(!stream.Read(door.pos)
		|| !stream.Read(door.rot)
		|| !stream.Read(door.pt)
		|| !stream.ReadCasted<byte>(door.locked)
		|| !stream.ReadCasted<byte>(door.state)
		|| !stream.Read(door.netid)
		|| !ReadBool(stream, door.door2))
		return false;

	if(door.state >= Door::Max)
	{
		ERROR(Format("Invalid door state %d.", door.state));
		return false;
	}

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
bool Game::ReadItem(BitStream& stream, GroundItem& item)
{
	if(!stream.Read(item.netid)
		|| !stream.Read(item.pos)
		|| !stream.Read(item.rot)
		|| !stream.Read(item.count)
		|| !stream.Read(item.team_count)
		|| ReadItemAndFind(stream, item.item) <= 0)
		return false;
	else
		return true;
}

//=================================================================================================
bool Game::ReadChest(BitStream& stream, Chest& chest)
{
	if(!stream.Read(chest.pos)
		|| !stream.Read(chest.rot)
		|| !stream.Read(chest.netid))
		return false;
	chest.ani = new AnimeshInstance(aSkrzynia);
	return true;
}

//=================================================================================================
bool Game::ReadTrap(BitStream& stream, Trap& trap)
{
	TRAP_TYPE type;
	if(!stream.ReadCasted<byte>(type)
		|| !stream.ReadCasted<byte>(trap.dir)
		|| !stream.Read(trap.netid)
		|| !stream.Read(trap.tile)
		|| !stream.Read(trap.pos)
		|| !stream.Read(trap.obj.rot.y))
		return false;
	trap.base = &g_traps[type];

	trap.state = 0;
	trap.obj.base = nullptr;
	trap.obj.mesh = trap.base->mesh;
	trap.obj.pos = trap.pos;
	trap.obj.scale = 1.f;
	trap.obj.rot.x = trap.obj.rot.z = 0;
	trap.trigger = false;
	trap.hitted = nullptr;

	if(mp_load)
	{
		if(!stream.ReadCasted<byte>(trap.state)
			|| !stream.Read(trap.time))
			return false;
	}

	if(type == TRAP_ARROW || type == TRAP_POISON)
		trap.obj.rot = VEC3(0,0,0);
	else if(type == TRAP_SPEAR)
	{
		trap.obj2.base = nullptr;
		trap.obj2.mesh = trap.base->mesh2;
		trap.obj2.pos = trap.obj.pos;
		trap.obj2.rot = trap.obj.rot;
		trap.obj2.scale = 1.f;
		trap.obj2.pos.y -= 2.f;
		trap.hitted = nullptr;
	}
	else
		trap.obj.base = &obj_alpha;

	return true;
}

//=================================================================================================
void Game::SendPlayerData(int index)
{
	PlayerInfo& info = game_players[index];
	Unit& unit = *info.u;
	BitStream& stream = net_stream2;

	stream.Reset();
	stream.Write(ID_PLAYER_DATA2);
	stream.Write(unit.netid);

	// items
	for(int i=0; i<SLOT_MAX; ++i)
	{
		if(unit.slots[i])
			WriteString1(stream, unit.slots[i]->id);
		else
			stream.WriteCasted<byte>(0);
	}
	WriteItemListTeam(stream, unit.items);

	// data
	unit.stats.Write(stream);
	unit.unmod_stats.Write(stream);
	stream.Write(unit.gold);
	unit.player->Write(stream);

	// other team members
	stream.WriteCasted<byte>(team.size()-1);
	for(Unit* other_unit : team)
	{
		if(other_unit != &unit)
			stream.Write(other_unit->netid);
	}
	stream.WriteCasted<byte>(leader_id);

	// multiplayer load data
	if(mp_load)
	{
		int flags = 0;
		if(unit.run_attack)
			flags |= 0x01;
		if(unit.used_item_is_team)
			flags |= 0x02;
		stream.Write(unit.attack_power);
		stream.Write(unit.raise_timer);
		stream.WriteCasted<byte>(flags);
	}

	stream.WriteCasted<byte>(0xFF);

	peer->Send(&stream, HIGH_PRIORITY, RELIABLE, 0, info.adr, false);
}

//=================================================================================================
bool Game::ReadPlayerData(BitStream& stream)
{
	int netid;
	if(!stream.Read(netid))
	{
		ERROR("Read player data: Broken packet.");
		return false;
	}

	Unit* unit = FindUnit(netid);
	if(!unit)
	{
		ERROR(Format("Read player data: Missing unit %d.", netid));
		return false;
	}
	game_players[0].u = unit;
	pc = unit->player;
	pc->player_info = &game_players[0];
	game_players[0].pc = pc;

	// items
	for(int i=0; i<SLOT_MAX; ++i)
	{
		if(!ReadString1(stream))
		{
			ERROR(Format("Read player data: Broken item %d.", i));
			return false;
		}
		if(BUF[0])
		{
			unit->slots[i] = FindItem(BUF);
			if(!unit->slots[i])
				return false;
		}
		else
			unit->slots[i] = nullptr;
	}
	if(!ReadItemListTeam(stream, unit->items))
	{
		ERROR("Read player data: Broken item list.");
		return false;
	}

	int credit = unit->player->credit,
		free_days = unit->player->free_days;

	unit->player->Init(*unit, true);

	if(!unit->stats.Read(stream) ||
		!unit->unmod_stats.Read(stream) ||
		!stream.Read(unit->gold) ||
		!pc->Read(stream))
	{
		ERROR("Read player data: Broken stats.");
		return false;
	}

	unit->look_target = nullptr;
	unit->prev_speed = 0.f;
	unit->run_attack = false;

	unit->weight = 0;
	unit->CalculateLoad();
	unit->RecalculateWeight();

	unit->player->credit = credit;
	unit->player->free_days = free_days;
	unit->player->is_local = true;

	// other team members
	team.clear();
	active_team.clear();
	team.push_back(unit);
	active_team.push_back(unit);
	byte count;
	if(!stream.Read(count)
		|| !EnsureSize(stream, sizeof(int) * count))
	{
		ERROR("Read player data: Broken team members.");
		return false;
	}
	for(byte i=0; i<count; ++i)
	{
		stream.Read(netid);
		Unit* team_member = FindUnit(netid);
		if(!team_member)
		{
			ERROR(Format("Read player data: Missing team member %d.", netid));
			return false;
		}
		team.push_back(team_member);
		if(team_member->IsPlayer() || !team_member->hero->free)
			active_team.push_back(team_member);
	}
	if(!stream.ReadCasted<byte>(leader_id))
	{
		ERROR("Read player data: Broken team leader.");
		return false;
	}
	PlayerInfo* leader_info = GetPlayerInfoTry(leader_id);
	if(!leader_info)
	{
		ERROR(Format("Read player data: Missing player %d.", leader_id));
		return false;
	}
	leader = leader_info->u;

	dialog_context.pc = unit->player;
	pc->noclip = noclip;
	pc->godmode = godmode;
	pc->unit->invisible = invisible;

	// multiplayer load data
	if(mp_load)
	{
		byte flags;
		if(!stream.Read(unit->attack_power)
			|| !stream.Read(unit->raise_timer)
			|| !stream.ReadCasted<byte>(flags))
		{
			ERROR("Read player data: Broken multiplaye data.");
			return false;
		}
		unit->run_attack = IS_SET(flags, 0x01);
		unit->used_item_is_team = IS_SET(flags, 0x02);

		mp_load = false;
	}

	// checksum
	byte check;
	if(!stream.Read(check) || check != 0xFF)
	{
		ERROR("Read player data: Broken checksum.");
		return false;
	}

	return true;
}

//=================================================================================================
Unit* Game::FindUnit(int netid)
{
	if(netid == -1)
		return nullptr;

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

	return nullptr;
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
			StreamError();
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

		// changes
		WriteServerChanges(net_stream);
		net_changes.clear();
		assert(net_talk.empty());
		peer->Send(&net_stream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, UNASSIGNED_SYSTEM_ADDRESS, true);

#ifdef _DEBUG
		int _net_player_updates = (int)net_changes_player.size();
#endif

		for(PlayerInfo& info : game_players)
		{
			if(info.id == my_id || info.left)
				continue;

			// update stats
			if(info.u->player->stat_flags != 0)
			{
				NetChangePlayer& c = Add1(net_changes_player);
				c.pc = info.u->player;
				c.id = c.pc->stat_flags;
				c.type = NetChangePlayer::PLAYER_STATS;
				info.NeedUpdate();
				c.pc->stat_flags = 0;
				DEBUG_DO(++_net_player_updates);
			}

			// update bufs
			int buffs = info.u->GetBuffs();
			if(buffs != info.buffs)
			{
				info.buffs = buffs;
				info.update_flags |= PlayerInfo::UF_BUFFS;
			}

			// write & send updates
			if(info.update_flags)
			{
				net_stream.Reset();
				int changes = WriteServerChangesForPlayer(net_stream, info);
				DEBUG_DO(_net_player_updates -= changes);
				peer->Send(&net_stream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, info.adr, false);
				info.update_flags = 0;
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
bool Game::ProcessControlMessageServer(BitStream& stream, PlayerInfo& info)
{
	bool move_info;
	if(!ReadBool(stream, move_info))
	{
		ERROR(Format("UpdateServer: Broken packet ID_CONTROL from %s.", info.name.c_str()));
		StreamError();
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

			if(!stream.Read(new_pos)
				|| !stream.Read(rot)
				|| !stream.Read(unit.ani->groups[0].speed))
			{
				ERROR(Format("UpdateServer: Broken packet ID_CONTROL(2) from %s.", info.name.c_str()));
				StreamError();
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
			if(!Skip(stream, sizeof(VEC3)+sizeof(float)*2))
			{
				ERROR(Format("UpdateServer: Broken packet ID_CONTROL(3) from %s.", info.name.c_str()));
				StreamError();
				return true;
			}
		}

		// animation
		Animation ani;
		if(!stream.ReadCasted<byte>(ani))
		{
			ERROR(Format("UpdateServer: Broken packet ID_CONTROL(4) from %s.", info.name.c_str()));
			StreamError();
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
		StreamError();
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
			StreamError();
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
					StreamError();
				}
				else if(i_index >= 0)
				{
					// equipping item
					if(uint(i_index) >= unit.items.size())
					{
						ERROR(Format("Update server: CHANGE_EQUIPMENT from %s, invalid index %d.", info.name.c_str(), i_index));
						StreamError();
						break;
					}

					ItemSlot& slot = unit.items[i_index];
					if(!slot.item->IsWearableByHuman())
					{
						ERROR(Format("Update server: CHANGE_EQUIPMENT from %s, item at index %d (%s) is not wearable.",
							info.name.c_str(), i_index, slot.item->id.c_str()));
						StreamError();
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
						StreamError();
					}
					else if(!unit.slots[slot])
					{
						ERROR(Format("Update server: CHANGE_EQUIPMENT from %s, empty slot type %d.", info.name.c_str(), slot));
						StreamError();
					}
					else
					{
						unit.AddItem(unit.slots[slot], 1u, false);
						unit.weight -= unit.slots[slot]->weight;
						unit.slots[slot] = nullptr;

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
					StreamError();
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
				float attack_speed;

				if(!stream.Read(typeflags)
					|| !stream.Read(attack_speed))
				{
					ERROR(Format("Update server: Broken ATTACK from %s.", info.name.c_str()));
					StreamError();
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
							unit.ani->groups[1].speed = attack_speed;
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
							unit.ani->groups[1].speed = attack_speed;
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
							unit.ani->groups[1].speed = attack_speed;
							unit.action = A_SHOOT;
							unit.animation_state = (type == AID_Shoot ? 1 : 0);
							unit.hitted = false;
							if(!unit.bow_instance)
								unit.bow_instance = GetBowInstance(unit.GetBow().mesh);
							unit.bow_instance->Play(&unit.bow_instance->ani->anims[0], PLAY_ONCE|PLAY_PRIO1|PLAY_NO_BLEND, 0);
							unit.bow_instance->groups[0].speed = unit.ani->groups[1].speed;
						}
						if(type == AID_Shoot)
						{
							if(!stream.Read(info.yspeed))
							{
								ERROR(Format("Update server: Broken ATTACK(2) from %s.", info.name.c_str()));
								StreamError();
							}
						}
						break;
					case AID_Block:
						{
							unit.action = A_BLOCK;
							unit.ani->Play(NAMES::ani_block, PLAY_PRIO1|PLAY_STOP_AT_END|PLAY_RESTORE, 1);
							unit.ani->groups[1].speed = 1.f;
							unit.ani->groups[1].blend_max = attack_speed;
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
							unit.ani->groups[1].speed = attack_speed;
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
						c.f[1] = attack_speed;
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
					StreamError();
				}
				else if(count == 0)
				{
					ERROR(Format("Update server: DROP_ITEM from %s, count %d.", info.name.c_str(), count));
					StreamError();
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
							StreamError();
							break;
						}

						ItemSlot& sl = unit.items[i_index];
						if(count > (int)sl.count)
						{
							ERROR(Format("Update server: DROP_ITEM from %s, index %d (count %d) have only %d count.", info.name.c_str(), i_index,
								count, sl.count));
							StreamError();
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
							StreamError();
							break;
						}

						const Item*& slot = unit.slots[slot_type];
						if(!slot)
						{
							ERROR(Format("Update server: DROP_ITEM from %s, empty slot %d.", info.name.c_str(), slot_type));
							StreamError();
							break;
						}

						unit.weight -= slot->weight*count;
						item = new GroundItem;
						item->item = slot;
						item->count = 1;
						item->team_count = 0;
						slot = nullptr;

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
					StreamError();
					break;
				}

				LevelContext* ctx;
				GroundItem* item = FindItemNetid(netid, &ctx);
				if(!item)
				{
					ERROR(Format("Update server: PICKUP_ITEM from %s, missing item %d.", info.name.c_str(), netid));
					StreamError();
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
					StreamError();
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
					StreamError();
					break;
				}

				Unit* looted_unit = FindUnit(netid);
				if(!looted_unit)
				{
					ERROR(Format("Update server: LOOT_UNIT from %s, missing unit %d.", info.name.c_str(), netid));
					StreamError();
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
					StreamError();
					break;
				}

				Chest* chest = FindChest(netid);
				if(!chest)
				{
					ERROR(Format("Update server: LOOT_CHEST from %s, missing chest %d.", info.name.c_str(), netid));
					StreamError();
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
					StreamError();
					break;
				}

				if(!player.IsTrading())
				{
					ERROR(Format("Update server: GET_ITEM, player %s is not trading.", info.name.c_str()));
					StreamError();
					break;
				}

				if(i_index >= 0)
				{
					// getting not equipped item
					if(i_index >= (int)player.chest_trade->size())
					{
						ERROR(Format("Update server: GET_ITEM from %s, invalid index %d.", info.name.c_str(), i_index));
						StreamError();
						break;
					}

					ItemSlot& slot = player.chest_trade->at(i_index);
					if(count < 1 || count >(int)slot.count)
					{
						ERROR(Format("Update server: GET_ITEM from %s, invalid item count %d (have %d).", info.name.c_str(), count, slot.count));
						StreamError();
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
								player.action_unit->used_item = nullptr;
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
						StreamError();
						break;
					}

					// get equipped item from unit
					const Item*& slot = player.action_unit->slots[type];
					AddItem(unit, slot, 1u, 1u, false);
					if(player.action == PlayerController::Action_LootUnit && type == IT_WEAPON && slot == player.action_unit->used_item)
					{
						player.action_unit->used_item = nullptr;
						// removed item from hand, send info to other players
						if(players > 2)
						{
							NetChange& c = Add1(net_changes);
							c.type = NetChange::REMOVE_USED_ITEM;
							c.unit = player.action_unit;
						}
					}
					player.action_unit->weight -= slot->weight;
					slot = nullptr;

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
					StreamError();
					break;
				}

				if(!player.IsTrading())
				{
					ERROR(Format("Update server: PUT_ITEM, player %s is not trading.", info.name.c_str()));
					StreamError();
					break;
				}

				if(i_index >= 0)
				{
					// put not equipped item
					if(i_index >= (int)unit.items.size())
					{
						ERROR(Format("Update server: PUT_ITEM from %s, invalid index %d.", info.name.c_str(), i_index));
						StreamError();
						break;
					}

					ItemSlot& slot = unit.items[i_index];
					if(count < 1 || count > slot.count)
					{
						ERROR(Format("Update server: PUT_ITEM from %s, invalid count %u (have %u).", info.name.c_str(), count, slot.count));
						StreamError();
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
						StreamError();
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
					slot = nullptr;
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
					StreamError();
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
							slots[i] = nullptr;
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
						unit.AddItem(gold_item_ptr, slot.count, slot.team_count);
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
				StreamError();
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
				player.action_unit->look_target = nullptr;
			}
			player.action = PlayerController::Action_None;
			if(player.dialog_ctx->next_talker)
			{
				Unit* t = player.dialog_ctx->next_talker;
				player.dialog_ctx->next_talker = nullptr;
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
					StreamError();
					break;
				}

				if(index < 0 || index >= (int)unit.items.size())
				{
					ERROR(Format("Update server: TAKE_ITEM_CREDIT from %s, invalid index %d.", info.name.c_str(), index));
					StreamError();
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
					StreamError();
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
					StreamError();
				}
				else if(index >= unit.data->idles->size())
				{
					ERROR(Format("Update server: IDLE from %s, invalid animation index %u.", info.name.c_str(), index));
					StreamError();
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
					StreamError();
					break;
				}

				Unit* talk_to = FindUnit(netid);
				if(!talk_to)
				{
					ERROR(Format("Update server: TALK from %s, missing unit %d.", info.name.c_str(), netid));
					StreamError();
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
					StreamError();
				}
				else if(player.dialog_ctx->show_choices && choice < player.dialog_ctx->choices.size())
					player.dialog_ctx->choice_selected = choice;
				else
				{
					ERROR(Format("Update server: CHOICE from %s, not in dialog or invalid choice %u.", info.name.c_str(), choice));
					StreamError();
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
					StreamError();
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
					StreamError();
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
					StreamError();
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
				StreamError();
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
					StreamError();
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
					StreamError();
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
					StreamError();
					break;
				}

				Useable* useable = FindUseable(useable_netid);
				if(!useable)
				{
					ERROR(Format("Update server: USE_USEABLE from %s, missing useable %d.", info.name.c_str(), useable_netid));
					StreamError();
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
						useable->user = nullptr;
						if(unit.live_state == Unit::ALIVE)
							unit.used_item = nullptr;

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
						StreamError();
					}
				}
			}
			break;
		// player used cheat 'suicide'
		case NetChange::CHEAT_SUICIDE:
			if(info.devmode)
				GiveDmg(GetContext(unit), nullptr, unit.hpmax, unit);
			else
			{
				ERROR(Format("Update server: Player %s used CHEAT_SUICIDE without devmode.", info.name.c_str()));
				StreamError();
			}
			break;
		// player used cheat 'godmode' 
		case NetChange::CHEAT_GODMODE:
			{
				bool state;
				if(!ReadBool(stream, state))
				{
					ERROR(Format("Update server: Broken CHEAT_GODMODE from %s.", info.name.c_str()));
					StreamError();
				}
				else if(info.devmode)
					player.godmode = state;
				else
				{
					ERROR(Format("Update server: Player %s used CHEAT_GODMODE without devmode.", info.name.c_str()));
					StreamError();
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
					StreamError();
				}
				else if(info.devmode)
					player.noclip = state;
				else
				{
					ERROR(Format("Update server: Player %s used CHEAT_NOCLIP without devmode.", info.name.c_str()));
					StreamError();
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
					StreamError();
				}
				else if(info.devmode)
					unit.invisible = state;
				else
				{
					ERROR(Format("Update server: Player %s used CHEAT_INVISIBLE without devmode.", info.name.c_str()));
					StreamError();
				}
			}
			break;
		// player used cheat 'scare'
		case NetChange::CHEAT_SCARE:
			if(info.devmode)
			{
				for(AIController* ai : ais)
				{
					if(IsEnemy(*ai->unit, unit) && distance(ai->unit->pos, unit.pos) < ALERT_RANGE.x && CanSee(*ai->unit, unit))
						ai->morale = -10;
				}
			}
			else
			{
				ERROR(Format("Update server: Player %s used CHEAT_SCARE without devmode.", info.name.c_str()));
				StreamError();
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
					StreamError();
					break;
				}

				if(!info.devmode)
				{
					ERROR(Format("Update server: Player %s used CHEAT_KILLALL without devmode.", info.name.c_str()));
					StreamError();
					break;
				}

				Unit* ignored;
				if(ignored_netid == -1)
					ignored = nullptr;
				else
				{
					ignored = FindUnit(ignored_netid);
					if(!ignored)
					{
						ERROR(Format("Update server: CHEAT_KILLALL from %s, missing unit %d.", info.name.c_str(), ignored_netid));
						StreamError();
						break;
					}
				}

				if(!Cheat_KillAll(type, unit, ignored))
				{
					ERROR(Format("Update server: CHEAT_KILLALL from %s, invalid type %u.", info.name.c_str(), type));
					StreamError();
				}
			}
			break;
		// client checks if item is better for npc
		case NetChange::IS_BETTER_ITEM:
			{
				int i_index;
				if(!stream.Read(i_index))
				{
					ERROR(Format("Update server: Broken IS_BETTER_ITEM from %s.", info.name.c_str()));
					StreamError();
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
						StreamError();
					}
				}
				else
				{
					ERROR(Format("Update server: IS_BETTER_ITEM from %s, player is not giving items.", info.name.c_str()));
					StreamError();
				}
			}
			break;
		// player used cheat 'citizen'
		case NetChange::CHEAT_CITIZEN:
			if(info.devmode)
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
				ERROR(Format("Update server: Player %s used CHEAT_CITIZEN without devmode.", info.name.c_str()));
				StreamError();
			}
			break;
		// player used cheat 'heal'
		case NetChange::CHEAT_HEAL:
			if(info.devmode)
			{
				unit.hp = unit.hpmax;
				NetChange& c = Add1(net_changes);
				c.type = NetChange::UPDATE_HP;
				c.unit = &unit;
			}
			else
			{
				ERROR(Format("Update server: Player %s used CHEAT_HEAL without devmode.", info.name.c_str()));
				StreamError();
			}
			break;
		// player used cheat 'kill'
		case NetChange::CHEAT_KILL:
			{
				int netid;
				if(!stream.Read(netid))
				{
					ERROR(Format("Update server: Broken CHEAT_KILL from %s.", info.name.c_str()));
					StreamError();
				}
				else if(!info.devmode)
				{
					ERROR(Format("Update server: Player %s used CHEAT_KILL without devmode.", info.name.c_str()));
					StreamError();
				}
				else
				{
					Unit* target = FindUnit(netid);
					if(!target)
					{
						ERROR(Format("Update server: CHEAT_KILL from %s, missing unit %d.", info.name.c_str(), netid));
						StreamError();
					}
					else
						GiveDmg(GetContext(*target), nullptr, target->hpmax, *target);
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
					StreamError();
				}
				else if(!info.devmode)
				{
					ERROR(Format("Update server: Player %s used CHEAT_HEALUNIT without devmode.", info.name.c_str()));
					StreamError();
				}
				else
				{
					Unit* target = FindUnit(netid);
					if(!target)
					{
						ERROR(Format("Update server: CHEAT_HEALUNIT from %s, missing unit %d.", info.name.c_str(), netid));
						StreamError();
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
			if(info.devmode)
				Cheat_Reveal();
			else
			{
				ERROR(Format("Update server: Player %s used CHEAT_REVEAL without devmode.", info.name.c_str()));
				StreamError();
			}
			break;
		// player used cheat 'goto_map'
		case NetChange::CHEAT_GOTO_MAP:
			if(info.devmode)
			{
				ExitToMap();
				StreamEnd();
				return false;
			}
			else
			{
				ERROR(Format("Update server: Player %s used CHEAT_GOTO_MAP without devmode.", info.name.c_str()));
				StreamError();
			}
			break;
		// player used cheat 'show_minimap'
		case NetChange::CHEAT_SHOW_MINIMAP:
			if(info.devmode)
				Cheat_ShowMinimap();
			else
			{
				ERROR(Format("Update server: Player %s used CHEAT_SHOW_MINIMAP without devmode.", info.name.c_str()));
				StreamError();
			}
			break;
		// player used cheat 'addgold'
		case NetChange::CHEAT_ADDGOLD:
			{
				int count;
				if(!stream.Read(count))
				{
					ERROR(Format("Update server: Broken CHEAT_ADDGOLD from %s.", info.name.c_str()));
					StreamError();
				}
				else if(!info.devmode)
				{
					ERROR(Format("Update server: Player %s used CHEAT_ADDGOLD without devmode.", info.name.c_str()));
					StreamError();
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
					StreamError();
				}
				else if(!info.devmode)
				{
					ERROR(Format("Update server: Player %s used CHEAT_ADDGOLD_TEAM without devmode.", info.name.c_str()));
					StreamError();
				}
				else if(count <= 0)
				{
					ERROR(Format("Update server: CHEAT_ADDGOLD_TEAM from %s, invalid count %d.", info.name.c_str(), count));
					StreamError();
				}
				else
					AddGold(count);
			}
			break;
		// player used cheat 'additem' or 'addteam'
		case NetChange::CHEAT_ADDITEM:
			{
				int count;
				bool is_team;
				if(!ReadString1(stream)
					|| !stream.Read(count)
					|| !ReadBool(stream, is_team))
				{
					ERROR(Format("Update server: Broken CHEAT_ADDITEM from %s.", info.name.c_str()));
					StreamError();
				}
				else if(!info.devmode)
				{
					ERROR(Format("Update server: Player %s used CHEAT_ADDITEM without devmode.", info.name.c_str()));
					StreamError();
				}
				else
				{
					const Item* item = FindItem(BUF);
					if(item && count)
						AddItem(*info.u, item, count, is_team);
					else
					{
						ERROR(Format("Update server: CHEAT_ADDITEM from %s, missing item %s or invalid count %u.", info.name.c_str(), BUF, count));
						StreamError();
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
					StreamError();
				}
				else if(!info.devmode)
				{
					ERROR(Format("Update server: Player %s used CHEAT_SKIP_DAYS without devmode.", info.name.c_str()));
					StreamError();
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
					StreamError();
				}
				else if(!info.devmode)
				{
					ERROR(Format("Update server: Player %s used CHEAT_WARP without devmode.", info.name.c_str()));
					StreamError();
				}
				else if(unit.frozen != 0)
				{
					ERROR(Format("Update server: CHEAT_WARP from %s, unit is frozen.", info.name.c_str()));
					StreamError();
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
						StreamError();
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
					StreamError();
				}
				else if(!info.devmode)
				{
					ERROR(Format("Update server: Player %s used CHEAT_SPAWN_UNIT without devmode.", info.name.c_str()));
					StreamError();
				}
				else
				{
					UnitData* data = FindUnitData(BUF, false);
					if(!data)
					{
						ERROR(Format("Update server: CHEAT_SPAWN_UNIT from %s, invalid unit id %s.", info.name.c_str(), BUF));
						StreamError();
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
					StreamError();
				}
				else if(!info.devmode)
				{
					ERROR(Format("Update server: Player %s used %s without devmode.", info.name.c_str(), name));
					StreamError();
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
						StreamError();
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
						StreamError();
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
					StreamError();
				}
				else if(pvp_response.ok && pvp_response.to == info.u)
				{
					if(accepted)
					{
						StartPvp(pvp_response.from->player, pvp_response.to);
						pvp_response.ok = false;
					}
					else
					{
						if(pvp_response.from->player == pc)
						{
							AddMsg(Format(txPvpRefuse, info.name.c_str()));
							pvp_response.ok = false;
						}
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
							RemoveElement(GUI.created_dialogs, dialog_pvp);
							delete dialog_pvp;
							dialog_pvp = nullptr;
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
					StreamError();
				}
				else if(!IsLeader(unit))
				{
					ERROR(Format("Update server: LEAVE_LOCATION from %s, player is not leader.", info.name.c_str()));
					StreamError();
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
								StreamError();
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
					StreamError();
					break;
				}

				Door* door = FindDoor(netid);
				if(!door)
				{
					ERROR(Format("Update server: USE_DOOR from %s, missing door %d.", info.name.c_str(), netid));
					StreamError();
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
		// leader wants to travel to location
		case NetChange::TRAVEL:
			{
				byte loc;
				if(!stream.Read(loc))
				{
					ERROR(Format("Update server: Broken TRAVEL from %s.", info.name.c_str()));
					StreamError();
				}
				else if(!IsLeader(unit))
				{
					ERROR(Format("Update server: LEAVE_LOCATION from %s, player is not leader.", info.name.c_str()));
					StreamError();
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
				{
					StreamEnd();
					return false;
				}
			}
			else
			{
				ERROR(Format("Update server: ENTER_LOCATION from %s, not leader or not on map.", info.name.c_str()));
				StreamError();
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
				RemoveElement(GUI.created_dialogs, dialog_enc);
				delete dialog_enc;
				dialog_enc = nullptr;
			}
			world_state = WS_TRAVEL;
			PushNetChange(NetChange::CLOSE_ENCOUNTER);
			Event_RandomEncounter(0);
			StreamEnd();
			return false;
		// player used cheat to change level (<>+shift+ctrl)
		case NetChange::CHEAT_CHANGE_LEVEL:
			{
				bool is_down;
				if(!ReadBool(stream, is_down))
				{
					ERROR(Format("Update server: Broken CHEAT_CHANGE_LEVEL from %s.", info.name.c_str()));
					StreamError();
				}
				else if(!info.devmode)
				{
					ERROR(Format("Update server: Player %s used CHEAT_CHANGE_LEVEL without devmode.", info.name.c_str()));
					StreamError();
				}
				else if(location->outside)
				{
					ERROR(Format("Update server:CHEAT_CHANGE_LEVEL from %s, outside location.", info.name.c_str()));
					StreamError();
				}
				else
				{
					ChangeLevel(is_down ? +1 : -1);
					StreamEnd();
					return false;
				}
			}
			break;
		// player used cheat to warp to stairs (<>+shift)
		case NetChange::CHEAT_WARP_TO_STAIRS:
			{
				bool is_down;
				if(!ReadBool(stream, is_down))
				{
					ERROR(Format("Update server: Broken CHEAT_CHANGE_LEVEL from %s.", info.name.c_str()));
					StreamError();
				}
				else if(!info.devmode)
				{
					ERROR(Format("Update server: Player %s used CHEAT_CHANGE_LEVEL without devmode.", info.name.c_str()));
					StreamError();
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
					StreamError();
				}
				else if(info.devmode)
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
					ERROR(Format("Update server: Player %s used CHEAT_NOAI without devmode.", info.name.c_str()));
					StreamError();
				}
			}
			break;
		// player rest in inn
		case NetChange::REST:
			{
				byte days;
				if(!stream.Read(days))
				{
					ERROR(Format("Update server: Broken REST from %s.", info.name.c_str()));
					StreamError();
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
				if(!stream.Read(type)
					|| !stream.Read(stat_type))
				{
					ERROR(Format("Update server: Broken TRAIN from %s.", info.name.c_str()));
					StreamError();
				}
				else
				{
					if(type == 2)
						TournamentTrain(unit);
					else
					{
						cstring error = nullptr;
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
							StreamError();
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
					StreamError();
				}
				else if(leader_id != info.id)
				{
					ERROR(Format("Update server: CHANGE_LEADER from %s, player is not leader.", info.name.c_str()));
					StreamError();
				}
				else
				{
					PlayerInfo* new_leader = GetPlayerInfoTry(id);
					if(!new_leader)
					{
						ERROR(Format("Update server: CHANGE_LEADER from %s, invalid player id %u.", id));
						StreamError();
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
					StreamError();
				}
				else if(unit.gold < count || player.credit < count || count < 0)
				{
					ERROR(Format("Update server: PAY_CREDIT from %s, invalid count %d (gold %d, credit %d).",
						info.name.c_str(), count, unit.gold, player.credit));
					StreamError();
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
					StreamError();
					break;
				}

				Unit* target = FindTeamMember(netid);
				if(!target)
				{
					ERROR(Format("Update server: GIVE_GOLD from %s, missing unit %d.", info.name.c_str(), netid));
					StreamError();
				}
				else if(target == &unit)
				{
					ERROR(Format("Update server: GIVE_GOLD from %s, wants to give gold to himself.", info.name.c_str()));
					StreamError();
				}
				else if(count > unit.gold || count < 0)
				{
					ERROR(Format("Update server: GIVE_GOLD from %s, invalid count %d (have %d).", info.name.c_str(), count, unit.gold));
					StreamError();
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
								PlaySound2d(sCoins);
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
					StreamError();
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
					item->item = gold_item_ptr;
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
					StreamError();
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
					StreamError();
				}
				else if(count < 0 || count > unit.gold)
				{
					ERROR(Format("Update server: PUT_GOLD from %s, invalid count %d (have %d).", info.name.c_str(), count, unit.gold));
					StreamError();
				}
				else if(player.action != PlayerController::Action_LootChest && player.action != PlayerController::Action_LootUnit)
				{
					ERROR(Format("Update server: PUT_GOLD from %s, player is not trading.", info.name.c_str()));
					StreamError();
				}
				else
				{
					InsertItem(*player.chest_trade, gold_item_ptr, count, 0);
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
					StreamError();
				}
				else if(!info.devmode)
				{
					ERROR(Format("Update server: Player %s used CHEAT_TRAVEL without devmode.", info.name.c_str()));
					StreamError();
				}
				else if(!IsLeader(unit))
				{
					ERROR(Format("Update server: CHEAT_TRAVEL from %s, player is not leader.", info.name.c_str()));
					StreamError();
				}
				else if(location_index >= locations.size() || !locations[location_index])
				{
					ERROR(Format("Update server: CHEAT_TRAVEL from %s, invalid location index %u.", info.name.c_str(), location_index));
					StreamError();
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
					StreamError();
				}
				else if(!info.devmode)
				{
					ERROR(Format("Update server: Player %s used CHEAT_HURT without devmode.", info.name.c_str()));
					StreamError();
				}
				else
				{
					Unit* target = FindUnit(netid);
					if(target)
						GiveDmg(GetContext(*target), nullptr, 100.f, *target);
					else
					{
						ERROR(Format("Update server: CHEAT_HURT from %s, missing unit %d.", info.name.c_str(), netid));
						StreamError();
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
					StreamError();
				}
				else if(!info.devmode)
				{
					ERROR(Format("Update server: Player %s used CHEAT_BREAK_ACTION without devmode.", info.name.c_str()));
					StreamError();
				}
				else
				{
					Unit* target = FindUnit(netid);
					if(target)
						BreakAction(*target, false, true);
					else
					{
						ERROR(Format("Update server: CHEAT_BREAK_ACTION from %s, missing unit %d.", info.name.c_str(), netid));
						StreamError();
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
					StreamError();
				}
				else if(!info.devmode)
				{
					ERROR(Format("Update server: Player %s used CHEAT_FALL without devmode.", info.name.c_str()));
					StreamError();
				}
				else
				{
					Unit* target = FindUnit(netid);
					if(target)
						UnitFall(*target);
					else
					{
						ERROR(Format("Update server: CHEAT_FALL from %s, missing unit %d.", info.name.c_str(), netid));
						StreamError();
					}
				}
			}
			break;
		// player yell to move ai
		case NetChange::YELL:
			PlayerYell(unit);
			break;
		// client failed to read packet and cancel action
		/*case NetChange::CANCEL_ACTION:
			{
				byte action_id;
				if(!stream.Read(action_id))
				{
					ERROR(Format("Update server: Broken CANCEL_ACTION from %s.", info.name.c_str()));
					StreamError();
					break;
				}

				switch(action_id)
				{
				case NetChangePlayer::LOOT:
				case NetChangePlayer::START_DIALOG:
				case NetChangePlayer::SHOW_DIALOG_CHOICES:
				case NetChangePlayer::START_TRADE:
				case NetChangePlayer::ADD_ITEMS_TRADER:
				case NetChangePlayer::ADD_ITEMS_CHEST:
				case NetChangePlayer::REMOVE_ITEMS_TRADER:
				case NetChangePlayer::START_SHARE:
				case NetChangePlayer::START_GIVE:
				case NetChangePlayer::UPDATE_TRADER_INVENTORY:
				}
			}
			break;*/
		// invalid change
		default:
			ERROR(Format("Update server: Invalid change type %u from %s.", type, info.name.c_str()));
			StreamError();
			break;
		}

		byte checksum = 0;
		if(!stream.Read(checksum) || checksum != 0xFF)
		{
			ERROR(Format("Update server: Invalid checksum from %s (%u).", info.name.c_str(), change_i));
			StreamError();
			return true;
		}
	}

	return true;
}

//=================================================================================================
void Game::WriteServerChanges(BitStream& stream)
{
	// count
	stream.WriteCasted<word>(net_changes.size());
	if(net_changes.size() >= 0xFFFF)
		ERROR(Format("Too many changes %d!", net_changes.size()));

	// changes
	for(NetChange& c : net_changes)
	{
		stream.WriteCasted<byte>(c.type);

		switch(c.type)
		{
		case NetChange::UNIT_POS:
			{
				Unit& unit = *c.unit;
				stream.Write(unit.netid);
				stream.Write(unit.pos);
				stream.Write(unit.rot);
				stream.Write(unit.ani->groups[0].speed);
				stream.WriteCasted<byte>(unit.animation);
			}
			break;
		case NetChange::CHANGE_EQUIPMENT:
			stream.Write(c.unit->netid);
			stream.WriteCasted<byte>(c.id);
			WriteBaseItem(stream, c.unit->slots[c.id]);
			break;
		case NetChange::TAKE_WEAPON:
			{
				Unit& u = *c.unit;
				stream.Write(u.netid);
				WriteBool(stream, c.id != 0);
				stream.WriteCasted<byte>(c.id == 0 ? u.weapon_taken : u.weapon_hiding);
			}
			break;
		case NetChange::ATTACK:
			{
				Unit&u = *c.unit;
				stream.Write(u.netid);
				byte b = (byte)c.id;
				b |= ((u.attack_id&0xF)<<4);
				stream.Write(b);
				stream.Write(c.f[1]);
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
				stream.Write(b);
			}
			break;
		case NetChange::UPDATE_HP:
			stream.Write(c.unit->netid);
			stream.Write(c.unit->hp);
			stream.Write(c.unit->hpmax);
			break;
		case NetChange::SPAWN_BLOOD:
			stream.WriteCasted<byte>(c.id);
			stream.Write(c.pos);
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
		case NetChange::CREATE_DRAIN:
		case NetChange::HERO_LEAVE:
		case NetChange::REMOVE_USED_ITEM:
		case NetChange::USEABLE_SOUND:
		case NetChange::BREAK_ACTION:
			stream.Write(c.unit->netid);
			break;
		case NetChange::CAST_SPELL:
			stream.Write(c.unit->netid);
			stream.WriteCasted<byte>(c.id);
			break;
		case NetChange::PICKUP_ITEM:
			stream.Write(c.unit->netid);
			WriteBool(stream, c.ile != 0);
			break;
		case NetChange::SPAWN_ITEM:
			WriteItem(stream, *c.item);
			break;
		case NetChange::REMOVE_ITEM:
			stream.Write(c.id);
			break;
		case NetChange::CONSUME_ITEM:
			{
				stream.Write(c.unit->netid);
				const Item* item = (const Item*)c.id;
				WriteString1(stream, item->id);
				WriteBool(stream, c.ile != 0);
			}
			break;
		case NetChange::HIT_SOUND:
			stream.Write(c.pos);
			stream.WriteCasted<byte>(c.id);
			stream.WriteCasted<byte>(c.ile);
			break;
		case NetChange::SHOOT_ARROW:
			{
				int netid = (c.unit ? c.unit->netid : -1);
				stream.Write(netid);
				stream.Write(c.pos);
				stream.Write(c.vec3);
				stream.Write(c.extra_f);
			}
			break;
		case NetChange::UPDATE_CREDIT:
			{
				byte ile = (byte)active_team.size();
				stream.Write(ile);
				for(vector<Unit*>::iterator it2 = active_team.begin(), end2 = active_team.end(); it2 != end2; ++it2)
				{
					Unit& u = **it2;
					stream.Write(u.netid);
					stream.Write(u.IsPlayer() ? u.player->credit : u.hero->credit);
				}
			}
			break;
		case NetChange::UPDATE_FREE_DAYS:
			{
				byte count = 0;
				uint pos = PatchByte(stream);
				for(PlayerInfo& info : game_players)
				{
					if(!info.left)
					{
						stream.Write(info.u->netid);
						stream.Write(info.u->player->free_days);
						++count;
					}
				}
				PatchByteApply(stream, pos, count);
			}
			break;
		case NetChange::IDLE:
			stream.Write(c.unit->netid);
			stream.WriteCasted<byte>(c.id);
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
			stream.Write(c.id);
			break;
		case NetChange::TALK:
			stream.Write(c.unit->netid);
			stream.WriteCasted<byte>(c.id);
			stream.Write(c.ile);
			WriteString1(stream, *c.str);
			StringPool.Free(c.str);
			RemoveElement(net_talk, c.str);
			break;
		case NetChange::CHANGE_LOCATION_STATE:
			stream.WriteCasted<byte>(c.id);
			stream.WriteCasted<byte>(c.ile);
			break;
		case NetChange::ADD_RUMOR:
			WriteString1(stream, rumors[c.id]);
			break;
		case NetChange::HAIR_COLOR:
			stream.Write(c.unit->netid);
			stream.Write(c.unit->human_data->hair_color);
			break;
		case NetChange::WARP:
			stream.Write(c.unit->netid);
			stream.WriteCasted<char>(c.unit->in_building);
			stream.Write(c.unit->pos);
			stream.Write(c.unit->rot);
			break;
		case NetChange::REGISTER_ITEM:
			{
				WriteString1(stream, c.base_item->id);
				WriteString1(stream, c.item2->id);
				WriteString1(stream, c.item2->name);
				WriteString1(stream, c.item2->desc);
				stream.Write(c.item2->refid);
			}
			break;
		case NetChange::ADD_QUEST:
		case NetChange::ADD_QUEST_MAIN:
			{
				Quest* q = FindQuest(c.id, false);
				stream.Write(q->refid);
				WriteString1(stream, q->name);
				WriteString2(stream, q->msgs[0]);
				WriteString2(stream, q->msgs[1]);
			}
			break;
		case NetChange::UPDATE_QUEST:
			{
				Quest* q = FindQuest(c.id, false);
				stream.Write(q->refid);
				stream.WriteCasted<byte>(q->state);
				WriteString2(stream, q->msgs.back());
			}
			break;
		case NetChange::RENAME_ITEM:
			{
				const Item* item = c.base_item;
				stream.Write(item->refid);
				WriteString1(stream, item->id);
				WriteString1(stream, item->name);
			}
			break;
		case NetChange::UPDATE_QUEST_MULTI:
			{
				Quest* q = FindQuest(c.id, false);
				stream.Write(q->refid);
				stream.WriteCasted<byte>(q->state);
				stream.WriteCasted<byte>(c.ile);
				for(int i = 0; i<c.ile; ++i)
					WriteString2(stream, q->msgs[q->msgs.size()-c.ile+i]);
			}
			break;
		case NetChange::CHANGE_LEADER:
		case NetChange::ARENA_SOUND:
		case NetChange::TRAVEL:
		case NetChange::CHEAT_TRAVEL:
		case NetChange::REMOVE_CAMP:
			stream.WriteCasted<byte>(c.id);
			break;
		case NetChange::PAUSED:
		case NetChange::CHEAT_NOAI:
			WriteBool(stream, c.id != 0);
			break;
		case NetChange::RANDOM_NUMBER:
			stream.WriteCasted<byte>(c.unit->player->id);
			stream.WriteCasted<byte>(c.id);
			break;
		case NetChange::REMOVE_PLAYER:
			stream.WriteCasted<byte>(c.id);
			stream.WriteCasted<byte>(c.ile);
			break;
		case NetChange::USE_USEABLE:
			stream.Write(c.unit->netid);
			stream.Write(c.id);
			stream.WriteCasted<byte>(c.ile);
			break;
		case NetChange::RECRUIT_NPC:
			stream.Write(c.unit->netid);
			WriteBool(stream, c.unit->hero->free);
			break;
		case NetChange::SPAWN_UNIT:
			WriteUnit(stream, *c.unit);
			break;
		case NetChange::CHANGE_ARENA_STATE:
			stream.Write(c.unit->netid);
			stream.WriteCasted<char>(c.unit->in_arena);
			break;
		case NetChange::WORLD_TIME:
			stream.Write(worldtime);
			stream.WriteCasted<byte>(day);
			stream.WriteCasted<byte>(month);
			stream.WriteCasted<byte>(year);
			break;
		case NetChange::USE_DOOR:
			stream.Write(c.id);
			WriteBool(stream, c.ile != 0);
			break;
		case NetChange::CREATE_EXPLOSION:
			WriteString1(stream, c.spell->id);
			stream.Write(c.pos);
			break;
		case NetChange::ENCOUNTER:
			WriteString1(stream, *c.str);
			StringPool.Free(c.str);
			break;
		case NetChange::ADD_LOCATION:
			{
				Location& loc = *locations[c.id];
				stream.WriteCasted<byte>(c.id);
				stream.WriteCasted<byte>(loc.type);
				if(loc.type == L_DUNGEON || loc.type == L_CRYPT)
					stream.WriteCasted<byte>(loc.GetLastLevel()+1);
				stream.WriteCasted<byte>(loc.state);
				stream.Write(loc.pos.x);
				stream.Write(loc.pos.y);
				WriteString1(stream, loc.name);
			}
			break;
		case NetChange::CHANGE_AI_MODE:
			{
				stream.Write(c.unit->netid);
				byte mode = 0;
				if(c.unit->dont_attack)
					mode |= 0x01;
				if(c.unit->assist)
					mode |= 0x02;
				if(c.unit->ai->state != AIController::Idle)
					mode |= 0x04;
				if(c.unit->attack_team)
					mode |= 0x08;
				stream.Write(mode);
			}
			break;
		case NetChange::CHANGE_UNIT_BASE:
			stream.Write(c.unit->netid);
			WriteString1(stream, c.unit->data->id);
			break;
		case NetChange::CREATE_SPELL_BALL:
			WriteString1(stream, c.spell->id);
			stream.Write(c.pos);
			stream.Write(c.f[0]);
			stream.Write(c.f[1]);
			stream.Write(c.extra_netid);
			break;
		case NetChange::SPELL_SOUND:
			WriteString1(stream, c.spell->id);
			stream.Write(c.pos);
			break;
		case NetChange::CREATE_ELECTRO:
			stream.Write(c.e_id);
			stream.Write(c.pos);
			stream.Write(c.vec3);
			break;
		case NetChange::UPDATE_ELECTRO:
			stream.Write(c.e_id);
			stream.Write(c.pos);
			break;
		case NetChange::ELECTRO_HIT:
		case NetChange::RAISE_EFFECT:
		case NetChange::HEAL_EFFECT:
			stream.Write(c.pos);
			break;
		case NetChange::REVEAL_MINIMAP:
			stream.WriteCasted<word>(minimap_reveal_mp.size());
			for(vector<INT2>::iterator it2 = minimap_reveal_mp.begin(), end2 = minimap_reveal_mp.end(); it2 != end2; ++it2)
			{
				stream.WriteCasted<byte>(it2->x);
				stream.WriteCasted<byte>(it2->y);
			}
			minimap_reveal_mp.clear();
			break;
		case NetChange::CHANGE_MP_VARS:
			WriteNetVars(stream);
			break;
		case NetChange::SECRET_TEXT:
			WriteString1(stream, GetSecretNote()->desc);
			break;
		case NetChange::UPDATE_MAP_POS:
			stream.Write(world_pos);
			break;
		case NetChange::GAME_STATS:
			stream.Write(total_kills);
			break;
		default:
			ERROR(Format("Update server: Unknown change %d.", c.type));
			assert(0);
			break;
		}

		stream.WriteCasted<byte>(0xFF);
	}
}

//=================================================================================================
int Game::WriteServerChangesForPlayer(BitStream& stream, PlayerInfo& info)
{
	int changes = 0;
	PlayerController& player = *info.pc;

	stream.WriteCasted<byte>(ID_PLAYER_UPDATE);
	stream.WriteCasted<byte>(info.update_flags);
	if(IS_SET(info.update_flags, PlayerInfo::UF_POISON_DAMAGE))
		stream.Write(player.last_dmg_poison);
	if(IS_SET(info.update_flags, PlayerInfo::UF_NET_CHANGES))
	{
		uint pos = PatchByte(stream);
		for(NetChangePlayer& c : net_changes_player)
		{
			if(c.pc != &player)
				continue;

			++changes;
			stream.WriteCasted<byte>(c.type);

			switch(c.type)
			{
			case NetChangePlayer::PICKUP:
				stream.Write(c.id);
				stream.Write(c.ile);
				break;
			case NetChangePlayer::LOOT:
				WriteBool(stream, c.id != 0);
				if(c.id != 0)
					WriteItemListTeam(stream, *player.chest_trade);
				break;
			case NetChangePlayer::START_SHARE:
			case NetChangePlayer::START_GIVE:
				{
					Unit& u = *player.action_unit;
					stream.Write(u.weight);
					stream.Write(u.weight_max);
					stream.Write(u.gold);
					u.stats.Write(stream);
					WriteItemListTeam(stream, u.items);
				}
				break;
			case NetChangePlayer::GOLD_MSG:
				WriteBool(stream, c.id != 0);
				stream.Write(c.ile);
				break;
			case NetChangePlayer::START_DIALOG:
				stream.Write(c.id);
				break;
			case NetChangePlayer::SHOW_DIALOG_CHOICES:
				{
					DialogContext& ctx = *player.dialog_ctx;
					stream.WriteCasted<byte>(ctx.choices.size());
					stream.WriteCasted<char>(ctx.dialog_esc);
					for(DialogChoice& choice : ctx.choices)
						WriteString1(stream, choice.msg);
				}
				break;
			case NetChangePlayer::END_DIALOG:
			case NetChangePlayer::USE_USEABLE:
			case NetChangePlayer::PREPARE_WARP:
			case NetChangePlayer::ENTER_ARENA:
			case NetChangePlayer::START_ARENA_COMBAT:
			case NetChangePlayer::EXIT_ARENA:
			case NetChangePlayer::END_FALLBACK:
			case NetChangePlayer::ADDED_ITEM_MSG:
				break;
			case NetChangePlayer::START_TRADE:
				stream.Write(c.id);
				WriteItemList(stream, *player.chest_trade);
				break;
			case NetChangePlayer::SET_FROZEN:
			case NetChangePlayer::DEVMODE:
			case NetChangePlayer::PVP:
			case NetChangePlayer::NO_PVP:
			case NetChangePlayer::CANT_LEAVE_LOCATION:
			case NetChangePlayer::REST:
				stream.WriteCasted<byte>(c.id);
				break;
			case NetChangePlayer::IS_BETTER_ITEM:
				WriteBool(stream, c.id != 0);
				break;
			case NetChangePlayer::REMOVE_QUEST_ITEM:
			case NetChangePlayer::LOOK_AT:
				stream.Write(c.id);
				break;
			case NetChangePlayer::ADD_ITEMS:
				{
					stream.Write(c.id);
					stream.Write(c.ile);
					WriteString1(stream, c.item->id);
					if(c.item->id[0] == '$')
						stream.Write(c.item->refid);
				}
				break;
			case NetChangePlayer::TRAIN:
				stream.WriteCasted<byte>(c.id);
				stream.WriteCasted<byte>(c.ile);
				break;
			case NetChangePlayer::UNSTUCK:
				stream.Write(c.pos);
				break;
			case NetChangePlayer::GOLD_RECEIVED:
				stream.WriteCasted<byte>(c.id);
				stream.Write(c.ile);
				break;
			case NetChangePlayer::GAIN_STAT:
				WriteBool(stream, c.id != 0);
				stream.WriteCasted<byte>(c.a);
				stream.WriteCasted<byte>(c.ile);
				break;
			case NetChangePlayer::ADD_ITEMS_TRADER:
				stream.Write(c.id);
				stream.Write(c.ile);
				stream.Write(c.a);
				WriteBaseItem(stream, c.item);
				break;
			case NetChangePlayer::ADD_ITEMS_CHEST:
				stream.Write(c.id);
				stream.Write(c.ile);
				stream.Write(c.a);
				WriteBaseItem(stream, c.item);
				break;
			case NetChangePlayer::REMOVE_ITEMS:
				stream.Write(c.id);
				stream.Write(c.ile);
				break;
			case NetChangePlayer::REMOVE_ITEMS_TRADER:
				stream.Write(c.id);
				stream.Write(c.ile);
				stream.Write(c.a);
				break;
			case NetChangePlayer::UPDATE_TRADER_GOLD:
				stream.Write(c.id);
				stream.Write(c.ile);
				break;
			case NetChangePlayer::UPDATE_TRADER_INVENTORY:
				stream.Write(c.unit->netid);
				WriteItemListTeam(stream, c.unit->items);
				break;
			case NetChangePlayer::PLAYER_STATS:
				stream.WriteCasted<byte>(c.id);
				if(IS_SET(c.id, STAT_KILLS))
					stream.Write(player.kills);
				if(IS_SET(c.id, STAT_DMG_DONE))
					stream.Write(player.dmg_done);
				if(IS_SET(c.id, STAT_DMG_TAKEN))
					stream.Write(player.dmg_taken);
				if(IS_SET(c.id, STAT_KNOCKS))
					stream.Write(player.knocks);
				if(IS_SET(c.id, STAT_ARENA_FIGHTS))
					stream.Write(player.arena_fights);
				break;
			case NetChangePlayer::ADDED_ITEMS_MSG:
				stream.WriteCasted<byte>(c.ile);
				break;
			case NetChangePlayer::STAT_CHANGED:
				stream.WriteCasted<byte>(c.id);
				stream.WriteCasted<byte>(c.a);
				stream.Write(c.ile);
				break;
			case NetChangePlayer::ADD_PERK:
				stream.WriteCasted<byte>(c.id);
				stream.Write(c.ile);
				break;
			default:
				ERROR(Format("Update server: Unknown player %s change %d.", info.name.c_str(), c.type));
				assert(0);
				break;
			}

			stream.WriteCasted<byte>(0xFF);
		}
		if(changes > 0xFF)
			ERROR(Format("Update server: Too many changes for player %s.", info.name.c_str()));
		PatchByteApply(stream, pos, changes);
	}
	if(IS_SET(info.update_flags, PlayerInfo::UF_GOLD))
		stream.Write(info.u->gold);
	if(IS_SET(info.update_flags, PlayerInfo::UF_ALCOHOL))
		stream.Write(info.u->alcohol);
	if(IS_SET(info.update_flags, PlayerInfo::UF_BUFFS))
		stream.WriteCasted<byte>(info.buffs);

	return changes;
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
				GUI.SimpleDialog(text, nullptr);
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
				GUI.SimpleDialog(txYouKicked, nullptr);
				return;
			}
		case ID_CHANGE_LEVEL:
			{
				byte loc, level;
				if(!stream.Read(loc)
					|| !stream.Read(level))
				{
					ERROR("Update client: Broken ID_CHANGE_LEVEL.");
					StreamError();
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
							RemoveElement(GUI.created_dialogs, dialog_pvp);
							delete dialog_pvp;
							dialog_pvp = nullptr;
						}
						pvp_response.ok = false;
					}
					if(dialog_enc)
					{
						GUI.CloseDialog(dialog_enc);
						RemoveElement(GUI.created_dialogs, dialog_enc);
						delete dialog_enc;
						dialog_enc = nullptr;
					}
					peer->DeallocatePacket(packet);
					StreamError();
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
			StreamError();
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
			WriteBool(net_stream, true);
			net_stream.Write((cstring)&pc->unit->pos, sizeof(VEC3));
			net_stream.Write(pc->unit->rot);
			net_stream.Write(pc->unit->ani->groups[0].speed);
			net_stream.WriteCasted<byte>(pc->unit->animation);
		}
		else
			WriteBool(net_stream, false);
		// changes
		WriteClientChanges(net_stream);
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
		StreamError();
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
			StreamError();
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
				float rot, ani_speed;
				Animation ani;

				if(!stream.Read(netid)
					|| !stream.Read(pos)
					|| !stream.Read(rot)
					|| !stream.Read(ani_speed)
					|| !stream.ReadCasted<byte>(ani))
				{
					ERROR("Update client: Broken UNIT_POS.");
					StreamError();
					break;
				}

				Unit* unit = FindUnit(netid);
				if(!unit)
				{
					ERROR(Format("Update client: UNIT_POS, missing unit %d.", netid));
					StreamError();
				}
				else if(unit != pc->unit)
				{
					unit->pos = pos;
					unit->ani->groups[0].speed = ani_speed;
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
				if(!stream.Read(netid)
					|| !stream.ReadCasted<byte>(type)
					|| ReadItemAndFind(stream, item) < 0)
				{
					ERROR("Update client: Broken CHANGE_EQUIPMENT.");
					StreamError();
				}
				else if(!IsValid(type))
				{
					ERROR(Format("Update client: CHANGE_EQUIPMENT, invalid slot %d.", type));
					StreamError();
				}
				else
				{
					Unit* target = FindUnit(netid);
					if(!target)
					{
						ERROR(Format("Update client: CHANGE_EQUIPMENT, missing unit %d.", netid));
						StreamError();
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
					StreamError();
				}
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
					{
						ERROR(Format("Update client: TAKE_WEAPON, missing unit %d.", netid));
						StreamError();
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
				float attack_speed;
				if(!stream.Read(netid)
					|| !stream.Read(typeflags)
					|| !stream.Read(attack_speed))
				{
					ERROR("Update client: Broken ATTACK.");
					StreamError();
					break;
				}

				Unit* unit_ptr = FindUnit(netid);
				if(!unit_ptr)
				{
					ERROR(Format("Update client: ATTACK, missing unit %d.", netid));
					StreamError();
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
						unit.ani->groups[1].speed = attack_speed;
					}
					else
					{
						if(sound_volume > 0 && unit.data->sounds->sound[SOUND_ATTACK] && rand2()%4 == 0)
							PlayAttachedSound(unit, unit.data->sounds->sound[SOUND_ATTACK], 1.f, 10.f);
						unit.action = A_ATTACK;
						unit.attack_id = ((typeflags & 0xF0)>>4);
						unit.attack_power = 1.f;
						unit.ani->Play(NAMES::ani_attacks[unit.attack_id], PLAY_PRIO1|PLAY_ONCE|PLAY_RESTORE, group);
						unit.ani->groups[group].speed = attack_speed;
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
						unit.ani->groups[group].speed = attack_speed;
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
						unit.ani->groups[group].speed = attack_speed;
						unit.action = A_SHOOT;
						unit.animation_state = (type == AID_Shoot ? 1 : 0);
						unit.hitted = false;
						if(!unit.bow_instance)
							unit.bow_instance = GetBowInstance(unit.GetBow().mesh);
						unit.bow_instance->Play(&unit.bow_instance->ani->anims[0], PLAY_ONCE|PLAY_PRIO1|PLAY_NO_BLEND, 0);
						unit.bow_instance->groups[0].speed = unit.ani->groups[group].speed;
					}
					break;
				case AID_Block:
					{
						unit.action = A_BLOCK;
						unit.ani->Play(NAMES::ani_block, PLAY_PRIO1|PLAY_STOP_AT_END|PLAY_RESTORE, group);
						unit.ani->groups[1].speed = 1.f;
						unit.ani->groups[1].blend_max = attack_speed;
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
						unit.ani->groups[group].speed = attack_speed;
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
					StreamError();
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
					StreamError();
				}
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
					{
						ERROR(Format("Update client: UPDATE_HP, missing unit %d.", netid));
						StreamError();
					}
					else if(unit == pc->unit)
					{
						// handling of previous hp
						float hp_dif = hp - unit->hp - hpmax + unit->hpmax;
						unit->hp = hp;
						unit->hpmax = hpmax;
						if(hp_dif < 0.f)
							pc->last_dmg += -hp_dif;
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
					|| !stream.Read(pos))
				{
					ERROR("Update client: Broken SPAWN_BLOOD.");
					StreamError();
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
					StreamError();
				}
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
					{
						ERROR(Format("Update client: HURT_SOUND, missing unit %d.", netid));
						StreamError();
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
					StreamError();
				}
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
					{
						ERROR(Format("Update client: DIE, missing unit %d.", netid));
						StreamError();
					}
					else
						UnitDie(*unit, nullptr, nullptr);
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
					StreamError();
				}
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
					{
						ERROR(Format("Update client: FALL, missing unit %d.", netid));
						StreamError();
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
					StreamError();
				}
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
					{
						ERROR(Format("Update client: DROP_ITEM, missing unit %d.", netid));
						StreamError();
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
					StreamError();
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
					StreamError();
				}
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
					{
						ERROR(Format("Update client: PICKUP_ITEM, missing unit %d.", netid));
						StreamError();
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
					StreamError();
				}
				else
				{
					LevelContext* ctx;
					GroundItem* item = FindItemNetid(netid, &ctx);
					if(!item)
					{
						ERROR(Format("Update client: REMOVE_ITEM, missing ground item %d.", netid));
						StreamError();
					}
					else
					{
						RemoveElement(ctx->items, item);
						if(before_player == BP_ITEM && before_player_ptr.item == item)
							before_player = BP_NONE;
						if(picking_item_state == 1 && picking_item == item)
							picking_item_state = 2;
						else
							delete item;
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
				if(!stream.Read(netid)
					|| ReadItemAndFind(stream, item) <= 0
					|| !ReadBool(stream, force))
				{
					ERROR("Update client: Broken CONSUME_ITEM.");
					StreamError();
				}
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
					{
						ERROR(Format("Update client: CONSUME_ITEM, missing unit %d.", netid));
						StreamError();
					}
					else if(item->type != IT_CONSUMEABLE)
					{
						ERROR(Format("Update client: CONSUME_ITEM, %s is not consumeable.", item->id.c_str()));
						StreamError();
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
				if(!stream.Read(pos)
					|| !stream.ReadCasted<byte>(mat1)
					|| !stream.ReadCasted<byte>(mat2))
				{
					ERROR("Update client: Broken HIT_SOUND.");
					StreamError();
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
					StreamError();
				}
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
					{
						ERROR(Format("Update client: STUNNED, missing unit %d.", netid));
						StreamError();
					}
					else
					{
						BreakAction(*unit);

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
				float rotX, rotY, speedY, speed;
				if(!stream.Read(netid)
					|| !stream.Read(pos)
					|| !stream.Read(rotY)
					|| !stream.Read(speedY)
					|| !stream.Read(rotX)
					|| !stream.Read(speed))
				{
					ERROR("Update client: Broken SHOOT_ARROW.");
					StreamError();
				}
				else
				{
					Unit* owner;
					if(netid == -1)
						owner = nullptr;
					else
					{
						owner = FindUnit(netid);
						if(!owner)
						{
							ERROR(Format("Update client: SHOOT_ARROW, missing unit %d.", netid));
							StreamError();
							break;
						}
					}

					LevelContext& ctx = GetContext(pos);

					Bullet& b = Add1(ctx.bullets);
					b.mesh = aArrow;
					b.pos = pos;
					b.rot = VEC3(rotX, rotY, 0);
					b.yspeed = speedY;
					b.owner = nullptr;
					b.pe = nullptr;
					b.remove = false;
					b.speed = speed;
					b.spell = nullptr;
					b.tex = nullptr;
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
					StreamError();
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
							StreamError();
						}
						else
						{
							Unit* unit = FindUnit(netid);
							if(!unit)
							{
								ERROR(Format("Update client: UPDATE_CREDIT, missing unit %d.", netid));
								StreamError();
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
					StreamError();
				}
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
					{
						ERROR(Format("Update client: IDLE, missing unit %d.", netid));
						StreamError();
					}
					else if(animation_index >= unit->data->idles->size())
					{
						ERROR(Format("Update client: IDLE, invalid animation index %u (count %u).", animation_index, unit->data->idles->size()));
						StreamError();
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
					StreamError();
				}
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
					{
						ERROR(Format("Update client: BROKEN, missing unit %d.", netid));
						StreamError();
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
					|| !ReadString1(stream))
				{
					ERROR("Update client: Broken TALK.");
					StreamError();
				}
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
					{
						ERROR(Format("Update client: TALK, missing unit %d.", netid));
						StreamError();
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
					StreamError();
				}
				else
				{
					Location* loc = nullptr;
					if(location_index < locations.size())
						loc = locations[location_index];
					if(!loc)
					{
						ERROR(Format("Update client: CHANGE_LOCATION_STATE, missing location %u.", location_index));
						StreamError();
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
				StreamError();
			}
			else
			{
				AddGameMsg3(GMS_ADDED_RUMOR);
				rumors.push_back(BUF);
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
					StreamError();
				}
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
					{
						ERROR(Format("Update client: TELL_NAME, missing unit %d.", netid));
						StreamError();
					}
					else if(!unit->IsHero())
					{
						ERROR(Format("Update client: TELL_NAME, unit %d (%s) is not a hero.", netid, unit->data->id.c_str()));
						StreamError();
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
					|| !stream.Read(hair_color))
				{
					ERROR("Update client: Broken HAIR_COLOR.");
					StreamError();
				}
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
					{
						ERROR(Format("Update client: HAIR_COLOR, missing unit %d.", netid));
						StreamError();
					}
					else if(unit->type != Unit::HUMAN)
					{
						ERROR(Format("Update client: HAIR_COLOR, unit %d (%s) is not human.", netid, unit->data->id.c_str()));
						StreamError();
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
					|| !stream.Read(pos)
					|| !stream.Read(rot))
				{
					ERROR("Update client: Broken WARP.");
					StreamError();
					break;
				}
				
				Unit* unit = FindUnit(netid);
				if(!unit)
				{
					ERROR(Format("Update client: WARP, missing unit %d.", netid));
					StreamError();
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
				StreamError();
			}
			else
			{
				const Item* base;
				base = FindItem(BUF);
				if(!base)
				{
					ERROR(Format("Update client: REGISTER_ITEM, missing base item %s.", BUF));
					StreamError();
					if(!SkipString1(stream) // id
						|| !SkipString1(stream) // name
						|| !SkipString1(stream) // desc
						|| !Skip(stream, sizeof(int))) // refid
					{
						ERROR("Update client: Broken REGISTER_ITEM(2).");
					}
				}
				else
				{
					Item* item = CreateItemCopy(base);
					if(!ReadString1(stream, item->id)
						|| !ReadString1(stream, item->name)
						|| !ReadString1(stream, item->desc)
						|| !stream.Read(item->refid))
					{
						ERROR("Update client: Broken REGISTER_ITEM(3).");
						StreamError();
					}
					else
						quest_items.push_back(item);
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
					StreamError();
					break;
				}

				PlaceholderQuest* quest = new PlaceholderQuest;
				quest->quest_index = quests.size();
				quest->name = BUF;
				quest->refid = refid;
				quest->msgs.resize(2);

				if(!ReadString2(stream, quest->msgs[0])
					|| !ReadString2(stream, quest->msgs[1]))
				{
					ERROR(Format("Update client: Broken %s(2).", name));
					StreamError();
					delete quest;
					break;
				}

				quest->state = Quest::Started;
				game_gui->journal->NeedUpdate(Journal::Quests, quest->quest_index);
				if(type == NetChange::ADD_QUEST)
					AddGameMsg3(GMS_JOURNAL_UPDATED);
				else
					GUI.SimpleDialog(txQuest[270], nullptr);
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
					|| !ReadString2(stream, g_tmp_string))
				{
					ERROR("Update client: Broken UPDATE_QUEST.");
					StreamError();
					break;
				}

				Quest* quest = FindQuest(refid, false);
				if(!quest)
				{
					ERROR(Format("Update client: UPDATE_QUEST, missing quest %d.", refid));
					StreamError();
				}
				else
				{
					quest->state = (Quest::State)state;
					quest->msgs.push_back(g_tmp_string);
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
					StreamError();
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
								StreamError();
							}
							found = true;
							break;
						}
					}
					if(!found)
					{
						ERROR(Format("Update client: RENAME_ITEM, missing quest item %d.", refid));
						StreamError();
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
					StreamError();
					break;
				}

				Quest* quest = FindQuest(refid, false);
				if(!quest)
				{
					ERROR(Format("Update client: UPDATE_QUEST_MULTI, missing quest %d.", refid));
					StreamError();
					if(!SkipStringArray<byte, byte>(stream))
						ERROR("Update client: Broken UPDATE_QUEST_MULTI(2).");
				}
				else
				{
					quest->state = (Quest::State)state;
					for(byte i = 0; i<count; ++i)
					{
						if(!ReadString2(stream, Add1(quest->msgs)))
						{
							ERROR(Format("Update client: Broken UPDATE_QUEST_MULTI(3) on index %u.", i));
							StreamError();
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
					StreamError();
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
						StreamError();
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
					StreamError();
				}
				else if(player_id != my_id)
				{
					PlayerInfo* info = GetPlayerInfoTry(player_id);
					if(info)
						AddMsg(Format(txRolledNumber, info->name.c_str(), number));
					else
					{
						ERROR(Format("Update client: RANDOM_NUMBER, missing player %u.", player_id));
						StreamError();
					}
				}
			}
			break;
		// remove player from game
		case NetChange::REMOVE_PLAYER:
			{
				byte player_id, reason;
				if(!stream.Read(player_id)
					|| !stream.Read(reason))
				{
					ERROR("Update client: Broken REMOVE_PLAYER.");
					StreamError();
				}
				else
				{
					PlayerInfo* info = GetPlayerInfoTry(player_id);
					if(!info)
					{
						ERROR(Format("Update client: REMOVE_PLAYER, missing player %u.", player_id));
						StreamError();
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
								info->u = nullptr;
							}
							else
							{
								info->u->to_remove = true;
								to_remove.push_back(info->u);

								if(info->u->useable)
									info->u->useable->user = nullptr;
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
					StreamError();
					break;
				}

				Unit* unit = FindUnit(netid);
				if(!unit)
				{
					ERROR(Format("Update client: USE_USEABLE, missing unit %d.", netid));
					StreamError();
					break;
				}

				Useable* useable = FindUseable(useable_netid);
				if(!useable)
				{
					ERROR(Format("Update client: USE_USEABLE, missing useable %d.", useable_netid));
					StreamError();
					break;
				}
				
				if(state == 1 || state == 2)
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
						useable->user = nullptr;
					unit->action = A_NONE;
					unit->animation = ANI_STAND;
					if(unit->live_state == Unit::ALIVE)
						unit->used_item = nullptr;
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
					StreamError();
				}
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
					{
						ERROR(Format("Update client: STAND_UP, missing unit %d.", netid));
						StreamError();
					}
					else
						UnitStandup(*unit);
				}
			}
			break;
		// game over
		case NetChange::GAME_OVER:
			LOG("Update client: Game over - all players died.");
			SetMusic(MusicType::Death);
			CloseAllPanels(true);
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
					StreamError();
				}
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
					{
						ERROR(Format("Update client: RECRUIT_NPC, missing unit %d.", netid));
						StreamError();
					}
					else if(!unit->IsHero())
					{
						ERROR(Format("Update client: RECRUIT_NPC, unit %d (%s) is not a hero.", netid, unit->data->id.c_str()));
						StreamError();
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
					StreamError();
				}
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
					{
						ERROR(Format("Update client: KICK_NPC, missing unit %d.", netid));
						StreamError();
					}
					else if(!unit->IsHero() || !unit->hero->team_member)
					{
						ERROR(Format("Update client: KICK_NPC, unit %d (%s) is not a team member.", netid, unit->data->id.c_str()));
						StreamError();
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
		// remove unit from game
		case NetChange::REMOVE_UNIT:
			{
				int netid;
				if(!stream.Read(netid))
				{
					ERROR("Update client: Broken REMOVE_UNIT.");
					StreamError();
				}
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
					{
						ERROR(Format("Update client: REMOVE_UNIT, missing unit %d.", netid));
						StreamError();
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
					StreamError();
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
					StreamError();
				}
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
					{
						ERROR(Format("Update client: CHANGE_ARENA_STATE, missing unit %d.", netid));
						StreamError();
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
					StreamError();
				}
				else if(sound_volume && city_ctx && city_ctx->type == L_CITY && GetArena()->ctx.building_id == pc->unit->in_building)
				{
					SOUND snd;
					if(type == 0)
						snd = sArenaFight;
					else if(type == 1)
						snd = sArenaWin;
					else
						snd = sArenaLost;
					PlaySound2d(snd);
				}
			}
			break;
		// unit shout after seeing enemy
		case NetChange::SHOUT:
			{
				int netid;
				if(!stream.Read(netid))
				{
					ERROR("Update client: Broken SHOUT.");
					StreamError();
				}
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
					{
						ERROR(Format("Update client: SHOUT, missing unit %d.", netid));
						StreamError();
					}
					else if(sound_volume)
						PlayAttachedSound(*unit, unit->data->sounds->sound[SOUND_SEE_ENEMY], 3.f, 20.f);
				}
			}
			break;
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
		// leader wants to travel to location
		case NetChange::TRAVEL:
			{
				byte loc;
				if(!stream.Read(loc))
				{
					ERROR("Update client: Broken TRAVEL.");
					StreamError();
				}
				else
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

					// leave current location
					if(open_location != -1)
					{
						LeaveLocation();
						open_location = -1;
					}
				}
			}
			break;
		// change world time
		case NetChange::WORLD_TIME:
			{
				int new_worldtime;
				byte new_day, new_month, new_year;
				if(!stream.Read(new_worldtime)
					|| !stream.Read(new_day)
					|| !stream.Read(new_month)
					|| !stream.Read(new_year))
				{
					ERROR("Update client: Broken WORLD_TIME.");
					StreamError();
				}
				else
				{
					worldtime = new_worldtime;
					day = new_day;
					month = new_month;
					year = new_year;
				}
			}
			break;
		// someone open/close door
		case NetChange::USE_DOOR:
			{
				int netid;
				bool is_closing;
				if(!stream.Read(netid)
					|| !ReadBool(stream, is_closing))
				{
					ERROR("Update client: Broken USE_DOOR.");
					StreamError();
				}
				else
				{
					Door* door = FindDoor(netid);
					if(!door)
					{
						ERROR(Format("Update client: USE_DOOR, missing door %d.", netid));
						StreamError();
					}
					else
					{
						bool ok = true;
						if(is_closing)
						{
							// closing door
							if(door->state == Door::Open)
							{
								door->state = Door::Closing;
								door->ani->Play(&door->ani->ani->anims[0], PLAY_ONCE | PLAY_STOP_AT_END | PLAY_NO_BLEND | PLAY_BACK, 0);
								door->ani->frame_end_info = false;
							}
							else if(door->state == Door::Opening)
							{
								door->state = Door::Closing2;
								door->ani->Play(&door->ani->ani->anims[0], PLAY_ONCE | PLAY_STOP_AT_END | PLAY_BACK, 0);
								door->ani->frame_end_info = false;
							}
							else if(door->state == Door::Opening2)
							{
								door->state = Door::Closing;
								door->ani->Play(&door->ani->ani->anims[0], PLAY_ONCE | PLAY_STOP_AT_END | PLAY_BACK, 0);
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
								door->ani->Play(&door->ani->ani->anims[0], PLAY_ONCE | PLAY_STOP_AT_END | PLAY_NO_BLEND, 0);
								door->ani->frame_end_info = false;
							}
							else if(door->state == Door::Closing)
							{
								door->locked = LOCK_NONE;
								door->state = Door::Opening2;
								door->ani->Play(&door->ani->ani->anims[0], PLAY_ONCE | PLAY_STOP_AT_END, 0);
								door->ani->frame_end_info = false;
							}
							else if(door->state == Door::Closing2)
							{
								door->locked = LOCK_NONE;
								door->state = Door::Opening;
								door->ani->Play(&door->ani->ani->anims[0], PLAY_ONCE | PLAY_STOP_AT_END, 0);
								door->ani->frame_end_info = false;
							}
							else
								ok = false;
						}

						if(ok && sound_volume && rand2() == 0)
						{
							SOUND snd;
							if(is_closing && rand2() % 2 == 0)
								snd = sDoorClose;
							else
								snd = sDoor[rand2() % 3];
							VEC3 pos = door->pos;
							pos.y += 1.5f;
							PlaySound3d(snd, pos, 2.f, 5.f);
						}
					}
				}
			}
			break;
		// chest opening animation
		case NetChange::CHEST_OPEN:
			{
				int netid;
				if(!stream.Read(netid))
				{
					ERROR("Update client: Broken CHEST_OPEN.");
					StreamError();
				}
				else
				{
					Chest* chest = FindChest(netid);
					if(!chest)
					{
						ERROR(Format("Update client: CHEST_OPEN, missing chest %d.", netid));
						StreamError();
					}
					else
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
			}
			break;
		// chest closing animation
		case NetChange::CHEST_CLOSE:
			{
				int netid;
				if(!stream.Read(netid))
				{
					ERROR("Update client: Broken CHEST_CLOSE.");
					StreamError();
				}
				else
				{
					Chest* chest = FindChest(netid);
					if(!chest)
					{
						ERROR(Format("Update client: CHEST_CLOSE, missing chest %d.", netid));
						StreamError();
					}
					else
					{
						chest->ani->Play(&chest->ani->ani->anims[0], PLAY_PRIO1 | PLAY_ONCE | PLAY_STOP_AT_END | PLAY_BACK, 0);
						if(sound_volume)
						{
							VEC3 pos = chest->pos;
							pos.y += 0.5f;
							PlaySound3d(sChestClose, pos, 2.f, 5.f);
						}
					}
				}
			}
			break;
		// create explosion effect
		case NetChange::CREATE_EXPLOSION:
			{
				VEC3 pos;
				if(!ReadString1(stream)
					|| !stream.Read(pos))
				{
					ERROR("Update client: Broken CREATE_EXPLOSION.");
					StreamError();
					break;
				}

				Spell* spell_ptr = FindSpell(BUF);
				if(!spell_ptr)
				{
					ERROR(Format("Update client: CREATE_EXPLOSION, missing spell '%s'.", BUF));
					StreamError();
					break;
				}

				Spell& spell = *spell_ptr;
				if(!IS_SET(spell.flags, Spell::Explode))
				{
					ERROR(Format("Update client: CREATE_EXPLOSION, spell '%s' is not explosion.", BUF));
					StreamError();
					break;
				}

				Explo* explo = new Explo;
				explo->pos = pos;
				explo->size = 0.f;
				explo->sizemax = 2.f;
				explo->tex = spell.tex_explode;
				explo->owner = nullptr;

				if(sound_volume)
					PlaySound3d(spell.sound_hit, explo->pos, spell.sound_hit_dist.x, spell.sound_hit_dist.y);

				GetContext(pos).explos->push_back(explo);
			}
			break;
		// remove trap
		case NetChange::REMOVE_TRAP:
			{
				int netid;
				if(!stream.Read(netid))
				{
					ERROR("Update client: Broken REMOVE_TRAP.");
					StreamError();
				}
				else if(!RemoveTrap(netid))
				{
					ERROR(Format("Update client: REMOVE_TRAP, missing trap %d.", netid));
					StreamError();
				}
			}
			break;
		// trigger trap
		case NetChange::TRIGGER_TRAP:
			{
				int netid;
				if(!stream.Read(netid))
				{
					ERROR("Update client: Broken TRIGGER_TRAP.");
					StreamError();
				}
				else
				{
					Trap* trap = FindTrap(netid);
					if(trap)
						trap->trigger = true;
					else
					{
						ERROR(Format("Update client: TRIGGER_TRAP, missing trap %d.", netid));
						StreamError();
					}
				}
			}
			break;
		// play evil sound
		case NetChange::EVIL_SOUND:
			if(sound_volume)
				PlaySound2d(sEvil);
			break;
		// start encounter on world map
		case NetChange::ENCOUNTER:
			if(!ReadString1(stream))
			{
				ERROR("Update client: Broken ENCOUNTER.");
				StreamError();
			}
			else
			{
				DialogInfo info;
				info.event = DialogEvent(this, &Game::Event_StartEncounter);
				info.name = "encounter";
				info.order = ORDER_TOP;
				info.parent = nullptr;
				info.pause = true;
				info.type = DIALOG_OK;
				info.text = BUF;

				dialog_enc = GUI.ShowDialog(info);
				if(!IsLeader())
					dialog_enc->bts[0].state = Button::DISABLED;
				world_state = WS_ENCOUNTER;
			}
			break;
		// close encounter message box
		case NetChange::CLOSE_ENCOUNTER:
			if(dialog_enc)
			{
				GUI.CloseDialog(dialog_enc);
				RemoveElement(GUI.created_dialogs, dialog_enc);
				delete dialog_enc;
				dialog_enc = nullptr;
			}
			world_state = WS_TRAVEL;
			break;
		// close portal in location
		case NetChange::CLOSE_PORTAL:
			if(location->portal)
			{
				delete location->portal;
				location->portal = nullptr;
			}
			else
			{
				ERROR("Update client: CLOSE_PORTAL, missing portal.");
				StreamError();
			}
			break;
		// clean altar in evil quest
		case NetChange::CLEAN_ALTAR:
			{
				// change object
				Obj* o = FindObject("bloody_altar");
				int index = 0;
				for(vector<Object>::iterator it = local_ctx.objects->begin(), end = local_ctx.objects->end(); it != end; ++it, ++index)
				{
					if(it->base == o)
						break;
				}
				Object& obj = local_ctx.objects->at(index);
				obj.base = FindObject("altar");
				obj.mesh = obj.base->mesh;

				// remove particles
				float best_dist = 999.f;
				ParticleEmitter* pe = nullptr;
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
		// add new location
		case NetChange::ADD_LOCATION:
			{
				byte location_index;
				LOCATION type;
				if(!stream.Read(location_index)
					|| !stream.ReadCasted<byte>(type))
				{
					ERROR("Update client: Broken ADD_LOCATION.");
					StreamError();
					break;
				}

				Location* loc;
				if(type == L_DUNGEON || type == L_CRYPT)
				{
					byte levels;
					if(!stream.Read(levels))
					{
						ERROR("Update client: Broken ADD_LOCATION(2).");
						StreamError();
						break;
					}
					if(levels == 1)
						loc = new SingleInsideLocation;
					else
						loc = new MultiInsideLocation(levels);
				}
				else if(type == L_CAVE)
					loc = new CaveLocation;
				else
					loc = new OutsideLocation;
				loc->type = type;

				if(!stream.ReadCasted<byte>(loc->state)
					|| !stream.Read(loc->pos)
					|| !ReadString1(stream, loc->name))
				{
					ERROR("Update client: Broken ADD_LOCATION(3).");
					StreamError();
					delete loc;
					break;
				}
				
				if(location_index >= locations.size())
					locations.resize(location_index + 1, nullptr);
				locations[location_index] = loc;
			}
			break;
		// remove camp
		case NetChange::REMOVE_CAMP:
			{
				byte camp_index;
				if(!stream.Read(camp_index))
				{
					ERROR("Update client: Broken REMOVE_CAMP.");
					StreamError();
				}
				else if(camp_index >= locations.size() || !locations[camp_index] || locations[camp_index]->type != L_CAMP)
				{
					ERROR(Format("Update client: REMOVE_CAMP, invalid location %u.", camp_index));
					StreamError();
				}
				else
				{
					delete locations[camp_index];
					if(camp_index == locations.size()-1)
						locations.pop_back();
					else
						locations[camp_index] = nullptr;
				}
			}
			break;
		// change unit ai mode
		case NetChange::CHANGE_AI_MODE:
			{
				int netid;
				byte mode;
				if(!stream.Read(netid)
					|| !stream.Read(mode))
				{
					ERROR("Update client: Broken CHANGE_AI_MODE.");
					StreamError();
				}
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
					{
						ERROR(Format("Update client: CHANGE_AI_MODE, missing unit %d.", netid));
						StreamError();
					}
					else
						unit->ai_mode = mode;
				}
			}
			break;
		// change unit base type
		case NetChange::CHANGE_UNIT_BASE:
			{
				int netid;
				if(!stream.Read(netid)
					|| !ReadString1(stream))
				{
					ERROR("Update client: Broken CHANGE_UNIT_BASE.");
					StreamError();
				}
				else
				{
					Unit* unit = FindUnit(netid);
					UnitData* ud = FindUnitData(BUF, false);
					if(unit && ud)
						unit->data = ud;
					else if(!unit)
					{
						ERROR(Format("Update client: CHANGE_UNIT_BASE, missing unit %d.", netid));
						StreamError();
					}
					else
					{
						ERROR(Format("Update client: CHANGE_UNIT_BASE, missing base unit '%s'.", BUF));
						StreamError();
					}
				}
			}
			break;
		// unit casts spell
		case NetChange::CAST_SPELL:
			{
				int netid;
				byte attack_id;
				if(!stream.Read(netid)
					|| !stream.Read(attack_id))
				{
					ERROR("Update client: Broken CAST_SPELL.");
					StreamError();
				}
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
					{
						ERROR(Format("Update client: CAST_SPELL, missing unit %d.", netid));
						StreamError();
					}
					else
					{
						unit->action = A_CAST;
						unit->attack_id = attack_id;
						unit->animation_state = 0;

						if(unit->ani->ani->head.n_groups == 2)
						{
							unit->ani->frame_end_info2 = false;
							unit->ani->Play("cast", PLAY_ONCE|PLAY_PRIO1, 1);
						}
						else
						{
							unit->ani->frame_end_info = false;
							unit->animation = ANI_PLAY;
							unit->ani->Play("cast", PLAY_ONCE|PLAY_PRIO1, 0);
						}
					}
				}
			}
			break;
		// create ball - spell effect
		case NetChange::CREATE_SPELL_BALL:
			{
				int netid;
				VEC3 pos;
				float rotY, speedY;

				if(!ReadString1(stream)
					|| !stream.Read(pos)
					|| !stream.Read(rotY)
					|| !stream.Read(speedY)
					|| !stream.Read(netid))
				{
					ERROR("Update client: Broken CREATE_SPELL_BALL.");
					StreamError();
					break;
				}

				Spell* spell_ptr = FindSpell(BUF);
				if(!spell_ptr)
				{
					ERROR(Format("Update client: CREATE_SPELL_BALL, missing spell '%s'.", BUF));
					StreamError();
					break;
				}

				Unit* unit;
				if(netid == -1)
					unit = nullptr;
				else
				{
					unit = FindUnit(netid);
					if(!unit)
					{
						ERROR(Format("Update client: CREATE_SPELL_BALL, missing unit %d.", netid));
						StreamError();
					}
				}

				Spell& spell = *spell_ptr;
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
				b.trail = nullptr;
				b.trail2 = nullptr;
				b.pe = nullptr;
				b.spell = &spell;
				b.start_pos = b.pos;
				b.owner = unit;
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
			break;
		// play spell sound
		case NetChange::SPELL_SOUND:
			{
				VEC3 pos;
				if(!ReadString1(stream)
					|| !stream.Read(pos))
				{
					ERROR("Update client: Broken SPELL_SOUND.");
					StreamError();
					break;
				}

				Spell* spell_ptr = FindSpell(BUF);
				if(!spell_ptr)
				{
					ERROR(Format("Update client: SPELL_SOUND, missing spell '%s'.", BUF));
					StreamError();
					break;
				}
				
				if(sound_volume)
				{
					Spell& spell = *spell_ptr;
					PlaySound3d(spell.sound_cast, pos, spell.sound_cast_dist.x, spell.sound_cast_dist.y);
				}
			}
			break;
		// drain blood effect
		case NetChange::CREATE_DRAIN:
			{
				int netid;
				if(!stream.Read(netid))
				{
					ERROR("Update client: Broken CREATE_DRAIN.");
					StreamError();
				}
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
					{
						ERROR(Format("Update client: CREATE_DRAIN, missing unit %d.", netid));
						StreamError();
					}
					else
					{
						LevelContext& ctx = GetContext(*unit);
						if(ctx.pes->empty())
						{
							ERROR("Update client: CREATE_DRAIN, missing blood.");
							StreamError();
						}
						else
						{
							Drain& drain = Add1(ctx.drains);
							drain.from = nullptr;
							drain.to = unit;
							drain.pe = ctx.pes->back();
							drain.t = 0.f;
							drain.pe->manual_delete = 1;
							drain.pe->speed_min = VEC3(-3, 0, -3);
							drain.pe->speed_max = VEC3(3, 3, 3);
						}
					}
				}
			}
			break;
		// create electro effect
		case NetChange::CREATE_ELECTRO:
			{
				int netid;
				VEC3 p1, p2;
				if(!stream.Read(netid)
					|| !stream.Read(p1)
					|| !stream.Read(p2))
				{
					ERROR("Update client: Broken CREATE_ELECTRO.");
					StreamError();
				}
				else
				{
					Electro* e = new Electro;
					e->spell = FindSpell("thunder_bolt");
					e->start_pos = p1;
					e->netid = netid;
					e->AddLine(p1, p2);
					e->valid = true;
					GetContext(p1).electros->push_back(e);
				}
			}
			break;
		// update electro effect
		case NetChange::UPDATE_ELECTRO:
			{
				int netid;
				VEC3 pos;
				if(!stream.Read(netid)
					|| !stream.Read(pos))
				{
					ERROR("Update client: Broken UPDATE_ELECTRO.");
					StreamError();
				}
				else
				{
					Electro* e = FindElectro(netid);
					if(!e)
					{
						ERROR(Format("Update client: UPDATE_ELECTRO, missing electro %d.", netid));
						StreamError();
					}
					else
					{
						VEC3 from = e->lines.back().pts.back();
						e->AddLine(from, pos);
					}
				}
			}
			break;
		// electro hit effect
		case NetChange::ELECTRO_HIT:
			{
				VEC3 pos;
				if(!stream.Read(pos))
				{
					ERROR("Update client: Broken ELECTRO_HIT.");
					StreamError();
				}
				else
				{
					Spell* spell = FindSpell("thunder_bolt");

					// sound
					if(sound_volume && spell->sound_hit)
						PlaySound3d(spell->sound_hit, pos, spell->sound_hit_dist.x, spell->sound_hit_dist.y);

					// particles
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
			}
			break;
		// raise spell effect
		case NetChange::RAISE_EFFECT:
			{
				VEC3 pos;
				if(!stream.Read(pos))
				{
					ERROR("Update client: Broken RAISE_EFFECT.");
					StreamError();
				}
				else
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
			}
			break;
		// heal spell effect
		case NetChange::HEAL_EFFECT:
			{
				VEC3 pos;
				if(!stream.Read(pos))
				{
					ERROR("Update client: Broken HEAL_EFFECT.");
					StreamError();
				}
				else
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
			}
			break;
		// someone used cheat 'show_minimap'
		case NetChange::CHEAT_SHOW_MINIMAP:
			Cheat_ShowMinimap();
			break;
		// revealing minimap
		case NetChange::REVEAL_MINIMAP:
			{
				word count;
				if(!stream.Read(count)
					|| !EnsureSize(stream, count*sizeof(byte)*2))
				{
					ERROR("Update client: Broken REVEAL_MINIMAP.");
					StreamError();
				}
				else
				{
					for(word i = 0; i<count; ++i)
					{
						byte x, y;
						stream.Read(x);
						stream.Read(y);
						minimap_reveal.push_back(INT2(x, y));
					}
				}
			}
			break;
		// 'noai' cheat change
		case NetChange::CHEAT_NOAI:
			{
				bool state;
				if(!ReadBool(stream, state))
				{
					ERROR("Update client: Broken CHEAT_NOAI.");
					StreamError();
				}
				else
					noai = state;
			}
			break;
		// end of game, time run out
		case NetChange::END_OF_GAME:
			LOG("Update client: Game over - time run out.");
			CloseAllPanels(true);
			koniec_gry = true;
			death_fade = 0.f;
			exit_from_server = true;
			break;
		// update players free days
		case NetChange::UPDATE_FREE_DAYS:
			{
				byte count;
				if(!stream.Read(count)
					|| !EnsureSize(stream, sizeof(int) * 2 * count))
				{
					ERROR("Update client: Broken UPDATE_FREE_DAYS.");
					StreamError();
				}
				else
				{
					for(byte i = 0; i < count; ++i)
					{
						int netid, days;
						stream.Read(netid);
						stream.Read(days);

						Unit* unit = FindUnit(netid);
						if(!unit || !unit->IsPlayer())
						{
							ERROR(Format("Update client: UPDATE_FREE_DAYS, missing unit %d or is not a player (0x%p).", netid, unit));
							StreamError();
							Skip(stream, sizeof(int) * 2 * (count - i - 1));
							break;
						}

						unit->player->free_days = days;
					}
				}
			}
			break;
		// multiplayer vars changed
		case NetChange::CHANGE_MP_VARS:
			if(!ReadNetVars(stream))
			{
				ERROR("Update client: Broken CHANGE_MP_VARS.");
				StreamError();
			}
			break;
		// game saved notification
		case NetChange::GAME_SAVED:
			AddMultiMsg(txGameSaved);
			break;
		// ai left team due too many team members
		case NetChange::HERO_LEAVE:
			{
				int netid;
				if(!stream.Read(netid))
				{
					ERROR("Update client: Broken HERO_LEAVE.");
					StreamError();
				}
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
					{
						ERROR(Format("Update client: HERO_LEAVE, missing unit %d.", netid));
						StreamError();
					}
					else
						AddMultiMsg(Format(txMpNPCLeft, unit->GetName()));
				}
			}
			break;
		// game paused/resumed
		case NetChange::PAUSED:
			{
				bool is_paused;
				if(!ReadBool(stream, is_paused))
				{
					ERROR("Update client: Broken PAUSED.");
					StreamError();
				}
				else
				{
					paused = is_paused;
					AddMultiMsg(paused ? txGamePaused : txGameResumed);
				}
			}
			break;
		// secret letter text update
		case NetChange::SECRET_TEXT:
			if(!ReadString1(stream))
			{
				ERROR("Update client: Broken SECRET_TEXT.");
				StreamError();
			}
			else
				GetSecretNote()->desc = BUF;
			break;
		// update position on world map
		case NetChange::UPDATE_MAP_POS:
			{
				VEC2 pos;
				if(!stream.Read(pos))
				{
					ERROR("Update client: Broken UPDATE_MAP_POS.");
					StreamError();
				}
				else
					world_pos = pos;
			}
			break;
		// player used cheat for fast travel on map
		case NetChange::CHEAT_TRAVEL:
			{
				byte location_index;
				if(!stream.Read(location_index))
				{
					ERROR("Update client: Broken CHEAT_TRAVEL.");
					StreamError();
				}
				else if(location_index >= locations.size() || !locations[location_index])
				{
					ERROR(Format("Update client: CHEAT_TRAVEL, invalid location index %u.", location_index));
					StreamError();
				}
				else
				{
					current_location = location_index;
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
			}
			break;
		// remove used item from unit hand
		case NetChange::REMOVE_USED_ITEM:
			{
				int netid;
				if(!stream.Read(netid))
				{
					ERROR("Update client: Broken REMOVE_USED_ITEM.");
					StreamError();
				}
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
					{
						ERROR(Format("Update client: REMOVE_USED_ITEM, missing unit %d.", netid));
						StreamError();
					}
					else
						unit->used_item = nullptr;
				}
			}
			break;
		// game stats showed at end of game
		case NetChange::GAME_STATS:
			if(!stream.Read(total_kills))
			{
				ERROR("Update client: Broken GAME_STATS.");
				StreamError();
			}
			break;
		// play useable object sound for unit
		case NetChange::USEABLE_SOUND:
			{
				int netid;
				if(!stream.Read(netid))
				{
					ERROR("Update client: Broken USEABLE_SOUND.");
					StreamError();
				}
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
					{
						ERROR(Format("Update client: USEABLE_SOUND, missing unit %d.", netid));
						StreamError();
					}
					else if(!unit->useable)
					{
						ERROR(Format("Update client: USEABLE_SOUND, unit %d (%s) don't use useable.", netid, unit->data->id.c_str()));
						StreamError();
					}
					else if(sound_volume && unit != pc->unit)
						PlaySound3d(unit->useable->GetBase()->sound, unit->GetCenter(), 2.f, 5.f);
				}
			}
			break;
		// show text when trying to enter academy
		case NetChange::ACADEMY_TEXT:
			ShowAcademyText();
			break;
		// break unit action
		case NetChange::BREAK_ACTION:
			{
				int netid;
				if(!stream.Read(netid))
				{
					ERROR("Update client: Broken BREAK_ACTION.");
					StreamError();
				}
				else
				{
					Unit* unit = FindUnit(netid);
					if(unit)
						BreakAction(*unit);
					else
					{
						ERROR(Format("Update client: BREAK_ACTION, missing unit %d.", netid));
						StreamError();
					}
				}
			}
			break;
		// invalid change
		default:
			WARN(Format("Update client: Unknown change type %d.", type));
			StreamError();
			break;
		}

		byte checksum = 0;
		if(!stream.Read(checksum) || checksum != 0xFF)
		{
			ERROR(Format("Update client: Invalid checksum (%u).", change_i));
			StreamError();
			return true;
		}
	}

	return true;
}

//=================================================================================================
bool Game::ProcessControlMessageClientForMe(BitStream& stream)
{
	// flags
	byte flags;
	if(!stream.Read(flags))
	{
		ERROR("Update single client: Broken ID_PLAYER_UPDATE.");
		StreamError();
		return true;
	}

	// last damage from poison
	if(IS_SET(flags, PlayerInfo::UF_POISON_DAMAGE))
	{
		if(!stream.Read(pc->last_dmg_poison))
		{
			ERROR("Update single client: Broken ID_PLAYER_UPDATE at UF_POISON_DAMAGE.");
			StreamError();
			return true;
		}
	}

	// changes
	if(IS_SET(flags, PlayerInfo::UF_NET_CHANGES))
	{
		byte changes;
		if(!stream.Read(changes))
		{
			ERROR("Update single client: Broken ID_PLAYER_UPDATE at UF_NET_CHANGES.");
			StreamError();
			return true;
		}

		for(byte change_i = 0; change_i<changes; ++change_i)
		{
			NetChangePlayer::TYPE type;
			if(!stream.ReadCasted<byte>(type))
			{
				ERROR("Update single client: Broken ID_PLAYER_UPDATE at UF_NET_CHANGES(2).");
				StreamError();
				return true;
			}

			switch(type)
			{
			// item picked up
			case NetChangePlayer::PICKUP:
				{
					int count, team_count;
					if(!stream.Read(count)
						|| !stream.Read(team_count))
					{
						ERROR("Update single client: Broken PICKUP.");
						StreamError();
					}
					else
					{
						AddItem(*pc->unit, picking_item->item, (uint)count, (uint)team_count);
						if(picking_item->item->type == IT_GOLD && sound_volume)
							PlaySound2d(sCoins);
						if(picking_item_state == 2)
							delete picking_item;
						picking_item_state = 0;
					}
				}
				break;
			// response to looting
			case NetChangePlayer::LOOT:
				{
					bool can_loot;
					if(!ReadBool(stream, can_loot))
					{
						/*NetChange& c = Add1(net_changes);
						c.type = NetChange::CANCEL_ACTION;
						c.id = type;*/
						ERROR("Update single client: Broken LOOT.");
						StreamError();
						break;
					}

					if(!can_loot)
					{
						AddGameMsg3(GMS_IS_LOOTED);
						pc->action = PlayerController::Action_None;
						break;
					}

					// read items
					if(!ReadItemListTeam(stream, *pc->chest_trade))
					{
						/*NetChange& c = Add1(net_changes);
						c.type = NetChange::CANCEL_ACTION;
						c.id = type;*/
						ERROR("Update single client: Broken LOOT(2).");
						StreamError();
						break;
					}

					// start trade
					if(pc->action == PlayerController::Action_LootUnit)
						StartTrade(I_LOOT_BODY, *pc->action_unit);
					else
						StartTrade(I_LOOT_CHEST, pc->action_chest->items);
				}
				break;
			// message about gained gold
			case NetChangePlayer::GOLD_MSG:
				{
					bool default_msg;
					int count;
					if(!ReadBool(stream, default_msg)
						|| !stream.Read(count))
					{
						ERROR("Update single client: Broken GOLD_MSG.");
						StreamError();
					}
					else
					{
						if(default_msg)
							AddGameMsg(Format(txGoldPlus, count), 3.f);
						else
							AddGameMsg(Format(txQuestCompletedGold, count), 4.f);
					}
				}
				break;
			// start dialog with unit or is busy
			case NetChangePlayer::START_DIALOG:
				{
					int netid;
					if(!stream.Read(netid))
					{
						/*NetChange& c = Add1(net_changes);
						c.type = NetChange::CANCEL_ACTION;
						c.id = type;*/
						ERROR("Update single client: Broken START_DIALOG.");
						StreamError();
					}
					else if(netid == -1)
					{
						// unit is busy
						pc->action = PlayerController::Action_None;
						AddGameMsg3(GMS_UNIT_BUSY);
					}
					else
					{
						// start dialog
						Unit* unit = FindUnit(netid);
						if(!unit)
						{
							/*NetChange& c = Add1(net_changes);
							c.type = NetChange::CANCEL_ACTION;
							c.id = type;*/
							ERROR(Format("Update single client: START_DIALOG, missing unit %d.", netid));
							StreamError();
						}
						else
						{
							pc->action = PlayerController::Action_Talk;
							pc->action_unit = unit;
							StartDialog(dialog_context, unit);
							if(!predialog.empty())
							{
								dialog_context.dialog_s_text = predialog;
								dialog_context.dialog_text = dialog_context.dialog_s_text.c_str();
								dialog_context.dialog_wait = 1.f;
								predialog.clear();
							}
							else if(unit->bubble)
							{
								dialog_context.dialog_s_text = unit->bubble->text;
								dialog_context.dialog_text = dialog_context.dialog_s_text.c_str();
								dialog_context.dialog_wait = 1.f;
								dialog_context.skip_id = unit->bubble->skip_id;
							}
							before_player = BP_NONE;
						}
					}
				}
				break;
			// show dialog choices
			case NetChangePlayer::SHOW_DIALOG_CHOICES:
				{
					byte count;
					char escape;
					if(!stream.Read(count)
						|| !stream.Read(escape)
						|| !EnsureSize(stream, count))
					{
						/*NetChange& c = Add1(net_changes);
						c.type = NetChange::CANCEL_ACTION;
						c.id = type;*/
						ERROR("Update single client: Broken SHOW_DIALOG_CHOICES.");
						StreamError();
					}
					else
					{
						dialog_context.choice_selected = 0;
						dialog_context.show_choices = true;
						dialog_context.dialog_esc = escape;
						dialog_choices.resize(count);
						for(byte i = 0; i<count; ++i)
						{
							if(!ReadString1(stream, dialog_choices[i]))
							{
								/*NetChange& c = Add1(net_changes);
								c.type = NetChange::CANCEL_ACTION;
								c.id = type;*/
								ERROR(Format("Update single client: Broken SHOW_DIALOG_CHOICES at %u index.", i));
								StreamError();
								break;
							}
						}
						game_gui->UpdateScrollbar(dialog_choices.size());
					}
				}
				break;
			// end of dialog
			case NetChangePlayer::END_DIALOG:
				dialog_context.dialog_mode = false;
				if(pc->action == PlayerController::Action_Talk)
					pc->action = PlayerController::Action_None;
				pc->unit->look_target = nullptr;
				break;
			// start trade
			case NetChangePlayer::START_TRADE:
				{
					int netid;
					if(!stream.Read(netid)
						|| !ReadItemList(stream, chest_trade))
					{
						/*NetChange& c = Add1(net_changes);
						c.type = NetChange::CANCEL_ACTION;
						c.id = type;*/
						ERROR("Update single client: Broken START_TRADE.");
						StreamError();
						break;
					}

					Unit* trader = FindUnit(netid);
					if(!trader)
					{
						/*NetChange& c = Add1(net_changes);
						c.type = NetChange::CANCEL_ACTION;
						c.id = type;*/
						ERROR(Format("Update single client: START_TRADE, missing unit %d.", netid));
						StreamError();
						break;
					}

					const string& id = trader->data->id;

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

					StartTrade(I_TRADE, chest_trade, trader);
				}
				break;
			// add multiple same items to inventory
			case NetChangePlayer::ADD_ITEMS:
				{
					int team_count, count;
					const Item* item;
					if(!stream.Read(team_count)
						|| !stream.Read(count)
						|| ReadItemAndFind(stream, item) <= 0)
					{
						ERROR("Update single client: Broken ADD_ITEMS.");
						StreamError();
					}
					else if(count <= 0 || team_count > count)
					{
						ERROR(Format("Update single client: ADD_ITEMS, invalid count %d or team count %d.", count, team_count));
						StreamError();
					}
					else
						AddItem(*pc->unit, item, (uint)count, (uint)team_count);
				}
				break;
			// add items to trader which is trading with player
			case NetChangePlayer::ADD_ITEMS_TRADER:
				{
					int netid, count, team_count;
					const Item* item;
					if(!stream.Read(netid)
						|| !stream.Read(count)
						|| !stream.Read(team_count)
						|| ReadItemAndFind(stream, item) <= 0)
					{
						/*NetChange& c = Add1(net_changes);
						c.type = NetChange::CANCEL_ACTION;
						c.id = type;*/
						ERROR("Update single client: Broken ADD_ITEMS_TRADER.");
						StreamError();
					}
					else if(count <= 0 || team_count > count)
					{
						/*NetChange& c = Add1(net_changes);
						c.type = NetChange::CANCEL_ACTION;
						c.id = type;*/
						ERROR(Format("Update single client: ADD_ITEMS_TRADER, invalid count %d or team count %d.", count, team_count));
						StreamError();
					}
					else
					{
						Unit* unit = FindUnit(netid);
						if(!unit)
						{
							/*NetChange& c = Add1(net_changes);
							c.type = NetChange::CANCEL_ACTION;
							c.id = type;*/
							ERROR(Format("Update single client: ADD_ITEMS_TRADER, missing unit %d.", netid));
							StreamError();
						}
						else if(!pc->IsTradingWith(unit))
						{
							/*NetChange& c = Add1(net_changes);
							c.type = NetChange::CANCEL_ACTION;
							c.id = type;*/
							ERROR(Format("Update single client: ADD_ITEMS_TRADER, unit %d (%s) is not trading with player.", netid, unit->data->id.c_str()));
							StreamError();
						}
						else
							AddItem(*unit, item, (uint)count, (uint)team_count);
					}
				}
				break;
			// add items to chest which is open by player
			case NetChangePlayer::ADD_ITEMS_CHEST:
				{
					int netid, count, team_count;
					const Item* item;
					if(!stream.Read(netid)
						|| !stream.Read(count)
						|| !stream.Read(team_count)
						|| ReadItemAndFind(stream, item) <= 0)
					{
						/*NetChange& c = Add1(net_changes);
						c.type = NetChange::CANCEL_ACTION;
						c.id = type;*/
						ERROR("Update single client: Broken ADD_ITEMS_CHEST.");
						StreamError();
					}
					else if(count <= 0 || team_count > count)
					{
						/*NetChange& c = Add1(net_changes);
						c.type = NetChange::CANCEL_ACTION;
						c.id = type;*/
						ERROR(Format("Update single client: ADD_ITEMS_CHEST, invalid count %d or team count %d.", count, team_count));
						StreamError();
					}
					else
					{
						Chest* chest = FindChest(netid);
						if(!chest)
						{
							/*NetChange& c = Add1(net_changes);
							c.type = NetChange::CANCEL_ACTION;
							c.id = type;*/
							ERROR(Format("Update single client: ADD_ITEMS_CHEST, missing chest %d.", netid));
							StreamError();
						}
						else if(pc->action != PlayerController::Action_LootChest || pc->action_chest != chest)
						{
							/*NetChange& c = Add1(net_changes);
							c.type = NetChange::CANCEL_ACTION;
							c.id = type;*/
							ERROR(Format("Update single client: ADD_ITEMS_CHEST, chest %d is not opened by player.", netid));
							StreamError();
						}
						else
							AddItem(*chest, item, (uint)count, (uint)team_count);
					}
				}
				break;
			// remove items from inventory
			case NetChangePlayer::REMOVE_ITEMS:
				{
					int i_index, count;
					if(!stream.Read(i_index)
						|| !stream.Read(count))
					{
						ERROR("Update single client: Broken REMOVE_ITEMS.");
						StreamError();
					}
					else if(count <= 0)
					{
						ERROR(Format("Update single client: REMOVE_ITEMS, invalid count %d.", count));
						StreamError();
					}
					else
						RemoveItem(*pc->unit, i_index, count);
				}
				break;
			// remove items from traded inventory which is trading with player
			case NetChangePlayer::REMOVE_ITEMS_TRADER:
				{
					int netid, i_index, count;
					if(!stream.Read(netid)
						|| !stream.Read(count)
						|| !stream.Read(i_index))
					{
						/*NetChange& c = Add1(net_changes);
						c.type = NetChange::CANCEL_ACTION;
						c.id = type;*/
						ERROR("Update single client: Broken REMOVE_ITEMS_TRADER.");
						StreamError();
					}
					else if(count <= 0)
					{
						/*NetChange& c = Add1(net_changes);
						c.type = NetChange::CANCEL_ACTION;
						c.id = type;*/
						ERROR(Format("Update single client: REMOVE_ITEMS_TRADE, invalid count %d.", count));
						StreamError();
					}
					else
					{
						Unit* unit = FindUnit(netid);
						if(!unit)
						{
							/*NetChange& c = Add1(net_changes);
							c.type = NetChange::CANCEL_ACTION;
							c.id = type;*/
							ERROR(Format("Update single client: REMOVE_ITEMS_TRADER, missing unit %d.", netid));
							StreamError();
						}
						else if(!pc->IsTradingWith(unit))
						{
							/*NetChange& c = Add1(net_changes);
							c.type = NetChange::CANCEL_ACTION;
							c.id = type;*/
							ERROR(Format("Update single client: REMOVE_ITEMS_TRADER, unit %d (%s) is not trading with player.", netid, unit->data->id.c_str()));
							StreamError();
						}
						else
							RemoveItem(*unit, i_index, count);
					}
				}
				break;
			// change player frozen state
			case NetChangePlayer::SET_FROZEN:
				if(!stream.ReadCasted<byte>(pc->unit->frozen))
				{
					ERROR("Update single client: Broken SET_FROZEN.");
					StreamError();
				}
				break;
			// remove quest item from inventory
			case NetChangePlayer::REMOVE_QUEST_ITEM:
				{
					int refid;
					if(!stream.Read(refid))
					{
						ERROR("Update single client: Broken REMOVE_QUEST_ITEM.");
						StreamError();
					}
					else
						pc->unit->RemoveQuestItem(refid);
				}
				break;
			// someone else is using useable
			case NetChangePlayer::USE_USEABLE:
				AddGameMsg3(GMS_USED);
				break;
			// change development mode for player
			case NetChangePlayer::DEVMODE:
				{
					bool allowed;
					if(!ReadBool(stream, allowed))
					{
						ERROR("Update single client: Broken CHEATS.");
						StreamError();
					}
					else if(devmode != allowed)
					{
						AddMsg(allowed ? txDevmodeOn : txDevmodeOff);
						devmode = allowed;
					}
				}
				break;
			// start sharing/giving items
			case NetChangePlayer::START_SHARE:
			case NetChangePlayer::START_GIVE:
				{
					cstring name = (type == NetChangePlayer::START_SHARE ? "START_SHARE" : "START_GIVE");
					if(pc->action_unit && pc->action_unit->IsTeamMember())
					{
						Unit& unit = *pc->action_unit;
						if(!stream.Read(unit.weight)
							|| !stream.Read(unit.weight_max)
							|| !stream.Read(unit.gold)
							|| !unit.stats.Read(stream)
							|| !ReadItemListTeam(stream, unit.items))
						{
							/*NetChange& c = Add1(net_changes);
							c.type = NetChange::CANCEL_ACTION;
							c.id = type;*/
							ERROR(Format("Update single client: Broken %s.", name));
							StreamError();
						}
						else
							StartTrade(type == NetChangePlayer::START_SHARE ? I_SHARE : I_GIVE, unit);
					}
					else
					{
						/*NetChange& c = Add1(net_changes);
						c.type = NetChange::CANCEL_ACTION;
						c.id = type;*/
						ERROR(Format("Update single client: %s, player is not talking with team member.", name));
						StreamError();
						// try to skip
						UnitStats stats;
						vector<ItemSlot> items;
						if(!Skip(stream, sizeof(int)*3)
							|| !stats.Read(stream)
							|| !ReadItemListTeam(stream, items))
						{
							ERROR(Format("Update single client: Broken %s(2).", name));
						}
					}
				}
				break;
			// response to is IS_BETTER_ITEM
			case NetChangePlayer::IS_BETTER_ITEM:
				{
					bool is_better;
					if(!ReadBool(stream, is_better))
					{
						ERROR("Update single client: Broken IS_BETTER_ITEM.");
						StreamError();
					}
					else
						game_gui->inv_trade_mine->IsBetterItemResponse(is_better);
				}
				break;
			// question about pvp
			case NetChangePlayer::PVP:
				{
					byte player_id;
					if(!stream.Read(player_id))
					{
						ERROR("Update single client: Broken PVP.");
						StreamError();
					}
					else
					{
						PlayerInfo* info = GetPlayerInfoTry(player_id);
						if(!info)
						{
							ERROR(Format("Update single client: PVP, invalid player id %u.", player_id));
							StreamError();
						}
						else
						{
							pvp_unit = info->u;
							DialogInfo info;
							info.event = DialogEvent(this, &Game::Event_Pvp);
							info.name = "pvp";
							info.order = ORDER_TOP;
							info.parent = nullptr;
							info.pause = false;
							info.text = Format(txPvp, pvp_unit->player->name.c_str());
							info.type = DIALOG_YESNO;
							dialog_pvp = GUI.ShowDialog(info);

							pvp_response.ok = true;
							pvp_response.timer = 0.f;
							pvp_response.to = pc->unit;
						}
					}
				}
				break;
			// preparing to warp
			case NetChangePlayer::PREPARE_WARP:
				fallback_co = FALLBACK_WAIT_FOR_WARP;
				fallback_t = -1.f;
				pc->unit->frozen = 2;
				break;
			// entering arena
			case NetChangePlayer::ENTER_ARENA:
				fallback_co = FALLBACK_ARENA;
				fallback_t = -1.f;
				pc->unit->frozen = 2;
				break;
			// start of arena combat
			case NetChangePlayer::START_ARENA_COMBAT:
				pc->unit->frozen = 0;
				break;
			// exit from arena
			case NetChangePlayer::EXIT_ARENA:
				fallback_co = FALLBACK_ARENA_EXIT;
				fallback_t = -1.f;
				pc->unit->frozen = 2;
				break;
			// player refused to pvp
			case NetChangePlayer::NO_PVP:
				{
					byte player_id;
					if(!stream.Read(player_id))
					{
						ERROR("Update single client: Broken NO_PVP.");
						StreamError();
					}
					else
					{
						PlayerInfo* info = GetPlayerInfoTry(player_id);
						if(!info)
						{
							ERROR(Format("Update single client: NO_PVP, invalid player id %u.", player_id));
							StreamError();
						}
						else
							AddMsg(Format(txPvpRefuse, info->name.c_str()));
					}
				}
				break;
			// can't leave location message
			case NetChangePlayer::CANT_LEAVE_LOCATION:
				{
					byte reason;
					if(!stream.Read(reason))
					{
						ERROR("Update single client: Broken CANT_LEAVE_LOCATION.");
						StreamError();
					}
					else if(reason >= 2)
					{
						ERROR(Format("Update single client: CANT_LEAVE_LOCATION, invalid reason %u.", reason));
						StreamError();
					}
					else
						AddGameMsg3(reason == 1 ? GMS_GATHER_TEAM : GMS_NOT_IN_COMBAT);
				}
				break;
			// force player to look at unit
			case NetChangePlayer::LOOK_AT:
				{
					int netid;
					if(!stream.Read(netid))
					{
						ERROR("Update single client: Broken LOOK_AT.");
						StreamError();
					}
					else
					{
						if(netid == -1)
							pc->unit->look_target = nullptr;
						else
						{
							Unit* unit = FindUnit(netid);
							if(!unit)
							{
								ERROR(Format("Update single client: LOOK_AT, missing unit %d.", netid));
								StreamError();
							}
							else
								pc->unit->look_target = unit;
						}
					}
				}
				break;
			// end of fallback
			case NetChangePlayer::END_FALLBACK:
				if(fallback_co == FALLBACK_CLIENT)
					fallback_co = FALLBACK_CLIENT2;
				break;
			// response to rest in inn
			case NetChangePlayer::REST:
				{
					byte days;
					if(!stream.Read(days))
					{
						ERROR("Update single client: Broken REST.");
						StreamError();
					}
					else
					{
						fallback_co = FALLBACK_REST;
						fallback_t = -1.f;
						fallback_1 = days;
						pc->unit->frozen = 2;
					}
				}
				break;
			// response to training
			case NetChangePlayer::TRAIN:
				{
					byte type, stat_type;
					if(!stream.Read(type)
						|| !stream.Read(stat_type))
					{
						ERROR("Update single client: Broken TRAIN.");
						StreamError();
					}
					else
					{
						fallback_co = FALLBACK_TRAIN;
						fallback_t = -1.f;
						fallback_1 = type;
						fallback_2 = stat_type;
						pc->unit->frozen = 2;
					}
				}
				break;
			// warped player to not stuck position
			case NetChangePlayer::UNSTUCK:
				{
					VEC3 new_pos;
					if(!stream.Read(new_pos))
					{
						ERROR("Update single client: Broken UNSTUCK.");
						StreamError();
					}
					else
					{
						pc->unit->pos = new_pos;
						interpolate_timer = 0.1f;
					}
				}
				break;
			// message about receiving gold from another player
			case NetChangePlayer::GOLD_RECEIVED:
				{
					byte player_id;
					int count;
					if(!stream.Read(player_id)
						|| !stream.Read(count))
					{
						ERROR("Update single client: Broken GOLD_RECEIVED.");
						StreamError();
					}
					else if(count <= 0)
					{
						ERROR(Format("Update single client: GOLD_RECEIVED, invalid count %d.", count));
						StreamError();
					}
					{
						PlayerInfo* info = GetPlayerInfoTry(player_id);
						if(!info)
						{
							ERROR(Format("Update single client: GOLD_RECEIVED, invalid player id %u.", player_id));
							StreamError();
						}
						else
						{
							AddMultiMsg(Format(txReceivedGold, count, info->name.c_str()));
							if(sound_volume)
								PlaySound2d(sCoins);
						}
					}
				}
				break;
			// message about gaining attribute/skill
			case NetChangePlayer::GAIN_STAT:
				{
					bool is_skill;
					byte what, value;
					if(!ReadBool(stream, is_skill)
						|| !stream.Read(what)
						|| !stream.Read(value))
					{
						ERROR("Update single client: Broken GAIN_STAT.");
						StreamError();
					}
					else
						ShowStatGain(is_skill, what, value);
				}
				break;
			// update trader gold
			case NetChangePlayer::UPDATE_TRADER_GOLD:
				{
					int netid, count;
					if(!stream.Read(netid)
						|| !stream.Read(count))
					{
						ERROR("Update single client: Broken UPDATE_TRADER_GOLD.");
						StreamError();
					}
					else if(count < 0)
					{
						ERROR(Format("Update single client: UPDATE_TRADER_GOLD, invalid count %d.", count));
						StreamError();
					}
					else
					{
						Unit* unit = FindUnit(netid);
						if(!unit)
						{
							ERROR(Format("Update single client: UPDATE_TRADER_GOLD, missing unit %d.", netid));
							StreamError();
						}
						else if(!pc->IsTradingWith(unit))
						{
							ERROR(Format("Update single client: UPDATE_TRADER_GOLD, unit %d (%s) is not trading with player.", netid, unit->data->id.c_str()));
							StreamError();
						}
						else
							unit->gold = count;
					}
				}
				break;
			// update trader inventory after getting item
			case NetChangePlayer::UPDATE_TRADER_INVENTORY:
				{
					int netid;
					if(!stream.Read(netid))
					{
						/*NetChange& c = Add1(net_changes);
						c.type = NetChange::CANCEL_ACTION;
						c.id = type;*/
						ERROR("Update single client: Broken UPDATE_TRADER_INVENTORY.");
						StreamError();
					}
					else
					{
						Unit* unit = FindUnit(netid);
						bool ok = true;
						if(!unit)
						{
							/*NetChange& c = Add1(net_changes);
							c.type = NetChange::CANCEL_ACTION;
							c.id = type;*/
							ERROR(Format("Update single client: UPDATE_TRADER_INVENTORY, missing unit %d.", netid));
							StreamError();
							ok = false;
						}
						else if(!pc->IsTradingWith(unit))
						{
							/*NetChange& c = Add1(net_changes);
							c.type = NetChange::CANCEL_ACTION;
							c.id = type;*/
							ERROR(Format("Update single client: UPDATE_TRADER_INVENTORY, unit %d (%s) is not trading with player.", netid, unit->data->id.c_str()));
							StreamError();
							ok = false;
						}
						if(ok)
						{
							if(!ReadItemListTeam(stream, unit->items))
							{
								/*NetChange& c = Add1(net_changes);
								c.type = NetChange::CANCEL_ACTION;
								c.id = type;*/
								ERROR("Update single client: Broken UPDATE_TRADER_INVENTORY(2).");
								StreamError();
							}
						}
						else
						{
							vector<ItemSlot> items;
							if(!ReadItemListTeam(stream, items))
								ERROR("Update single client: Broken UPDATE_TRADER_INVENTORY(3).");
						}
					}
				}
				break;
			// update player statistics
			case NetChangePlayer::PLAYER_STATS:
				{
					byte flags;
					if(!stream.Read(flags))
					{
						ERROR("Update single client: Broken PLAYER_STATS.");
						StreamError();
					}
					else if(flags == 0 || (flags & ~STAT_MAX) != 0)
					{
						ERROR(Format("Update single client: PLAYER_STATS, invalid flags %u.", flags));
						StreamError();
					}
					else
					{
						int set_flags = count_bits(flags);
						// read to buffer
						if(!stream.Read(BUF, sizeof(int)*set_flags))
						{
							ERROR("Update single client: Broken PLAYER_STATS(2).");
							StreamError();
						}
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
			// message about gaining item
			case NetChangePlayer::ADDED_ITEM_MSG:
				AddGameMsg3(GMS_ADDED_ITEM);
				break;
			// message about gaining multiple items
			case NetChangePlayer::ADDED_ITEMS_MSG:
				{
					byte count;
					if(!stream.Read(count))
					{
						ERROR("Update single client: Broken ADDED_ITEMS_MSG.");
						StreamError();
					}
					else if(count <= 1)
					{
						ERROR(Format("Update single client: ADDED_ITEMS_MSG, invalid count %u.", count));
						StreamError();
					}
					else
						AddGameMsg(Format(txGmsAddedItems, (int)count), 3.f);
				}
				break;
			// player stat changed
			case NetChangePlayer::STAT_CHANGED:
				{
					byte type, what;
					int value;
					if(!stream.Read(type)
						|| !stream.Read(what)
						|| !stream.Read(value))
					{
						ERROR("Update single client: Broken STAT_CHANGED.");
						StreamError();
					}
					else
					{
						switch((ChangedStatType)type)
						{
						case ChangedStatType::ATTRIBUTE:
							if(what >= (byte)Attribute::MAX)
							{
								ERROR(Format("Update single client: STAT_CHANGED, invalid attribute %u.", what));
								StreamError();
							}
							else
								pc->unit->Set((Attribute)what, value);
							break;
						case ChangedStatType::SKILL:
							if(what >= (byte)Skill::MAX)
							{
								ERROR(Format("Update single client: STAT_CHANGED, invalid skill %u.", what));
								StreamError();
							}
							else
								pc->unit->Set((Skill)what, value);
							break;
						case ChangedStatType::BASE_ATTRIBUTE:
							if(what >= (byte)Attribute::MAX)
							{
								ERROR(Format("Update single client: STAT_CHANGED, invalid base attribute %u.", what));
								StreamError();
							}
							else
								pc->SetBase((Attribute)what, value);
							break;
						case ChangedStatType::BASE_SKILL:
							if(what >= (byte)Skill::MAX)
							{
								ERROR(Format("Update single client: STAT_CHANGED, invalid base skill %u.", what));
								StreamError();
							}
							else
								pc->SetBase((Skill)what, value);
							break;
						default:
							ERROR(Format("Update single client: STAT_CHANGED, invalid change type %u.", type));
							StreamError();
							break;
						}
					}
				}
				break;
			// add perk to player
			case NetChangePlayer::ADD_PERK:
				{
					byte id;
					int value;
					if(!stream.Read(id)
						|| !stream.Read(value))
					{
						ERROR("Update single client: Broken ADD_PERK.");
						StreamError();
					}
					else
						pc->perks.push_back(TakenPerk((Perk)id, value));
				}
				break;
			default:
				WARN(Format("Update single client: Unknown player change type %d.", type));
				StreamError();
				break;
			}

			byte checksum = 0;
			if(!stream.Read(checksum) || checksum != 0xFF)
			{
				ERROR(Format("Update single client: Invalid checksum (%u).", change_i));
				StreamError();
				return true;
			}
		}
	}
	if(pc)
	{
		// gold
		if(IS_SET(flags, PlayerInfo::UF_GOLD))
		{
			if(!stream.Read(pc->unit->gold))
			{
				ERROR("Update single client: Broken ID_PLAYER_UPDATE at UF_GOLD.");
				StreamError();
				return true;
			}
		}

		// alcohol
		if(IS_SET(flags, PlayerInfo::UF_ALCOHOL))
		{
			if(!stream.Read(pc->unit->alcohol))
			{
				ERROR("Update single client: Broken ID_PLAYER_UPDATE at UF_GOLD.");
				StreamError();
				return true;
			}
		}

		// buffs
		if(IS_SET(flags, PlayerInfo::UF_BUFFS))
		{
			if(!stream.ReadCasted<byte>(pc->player_info->buffs))
			{
				ERROR("Update single client: Broken ID_PLAYER_UPDATE at UF_BUFFS.");
				StreamError();
				return true;
			}
		}
	}

	return true;
}

//=================================================================================================
void Game::WriteClientChanges(BitStream& stream)
{
	// count
	stream.WriteCasted<byte>(net_changes.size());
	if(net_changes.size() > 255)
		ERROR(Format("Update client: Too many changes %u!", net_changes.size()));

	// changes
	for(NetChange& c : net_changes)
	{
		stream.WriteCasted<byte>(c.type);

		switch(c.type)
		{
		case NetChange::CHANGE_EQUIPMENT:
		case NetChange::IS_BETTER_ITEM:
		case NetChange::CONSUME_ITEM:
			stream.Write(c.id);
			break;
		case NetChange::TAKE_WEAPON:
			WriteBool(stream, c.id != 0);
			stream.WriteCasted<byte>(c.id == 0 ? pc->unit->weapon_taken : pc->unit->weapon_hiding);
			break;
		case NetChange::ATTACK:
			{
				byte b = (byte)c.id;
				b |= ((pc->unit->attack_id & 0xF)<<4);
				stream.Write(b);
				stream.Write(c.f[1]);
				if(c.id == 2)
					stream.Write(PlayerAngleY()*12);
			}
			break;
		case NetChange::DROP_ITEM:
			stream.Write(c.id);
			stream.Write(c.ile);
			break;
		case NetChange::IDLE:
		case NetChange::CHOICE:
		case NetChange::ENTER_BUILDING:
		case NetChange::CHANGE_LEADER:
		case NetChange::RANDOM_NUMBER:
		case NetChange::CHEAT_WARP:
		case NetChange::TRAVEL:
		case NetChange::CHEAT_TRAVEL:
		case NetChange::REST:
		//case NetChange::CANCEL_ACTION:
			stream.WriteCasted<byte>(c.id);
			break;
		case NetChange::CHEAT_WARP_TO_STAIRS:
		case NetChange::CHEAT_CHANGE_LEVEL:
		case NetChange::CHEAT_NOAI:
		case NetChange::PVP:
		case NetChange::CHEAT_GODMODE:
		case NetChange::CHEAT_INVISIBLE:
		case NetChange::CHEAT_NOCLIP:
			WriteBool(stream, c.id != 0);
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
			stream.Write(c.id);
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
			WriteString1(stream, notes.back());
			break;
		case NetChange::USE_USEABLE:
			stream.Write(c.id);
			stream.WriteCasted<byte>(c.ile);
			break;
		case NetChange::CHEAT_KILLALL:
			stream.Write(c.unit ? c.unit->netid : -1);
			stream.WriteCasted<byte>(c.id);
			break;
		case NetChange::CHEAT_KILL:
		case NetChange::CHEAT_HEALUNIT:
		case NetChange::CHEAT_HURT:
		case NetChange::CHEAT_BREAK_ACTION:
		case NetChange::CHEAT_FALL:
			stream.Write(c.unit->netid);
			break;
		case NetChange::CHEAT_ADDITEM:
			WriteString1(stream, c.base_item->id);
			stream.Write(c.ile);
			WriteBool(stream, c.id != 0);
			break;
		case NetChange::CHEAT_SPAWN_UNIT:
			WriteString1(stream, c.base_unit->id);
			stream.WriteCasted<byte>(c.ile);
			stream.WriteCasted<char>(c.id);
			stream.WriteCasted<char>(c.i);
			break;
		case NetChange::CHEAT_SETSTAT:
		case NetChange::CHEAT_MODSTAT:
			stream.WriteCasted<byte>(c.id);
			WriteBool(stream, c.ile != 0);
			stream.WriteCasted<char>(c.i);
			break;
		case NetChange::LEAVE_LOCATION:
			stream.WriteCasted<char>(c.id);
			break;
		case NetChange::USE_DOOR:
			stream.Write(c.id);
			WriteBool(stream, c.ile != 0);
			break;
		case NetChange::TRAIN:
			stream.WriteCasted<byte>(c.id);
			stream.WriteCasted<byte>(c.ile);
			break;
		case NetChange::GIVE_GOLD:
		case NetChange::GET_ITEM:
		case NetChange::PUT_ITEM:
			stream.Write(c.id);
			stream.Write(c.ile);
			break;
		case NetChange::PUT_GOLD:
			stream.Write(c.ile);
			break;
		default:
			ERROR(Format("UpdateClient: Unknown change %d.", c.type));
			assert(0);
			break;
		}

		stream.WriteCasted<byte>(0xFF);
	}
}

//=================================================================================================
void Game::Client_Say(BitStream& stream)
{
	byte id;

	if(!stream.Read(id)
		|| !ReadString1(stream))
	{
		ERROR("Client_Say: Broken packet.");
		StreamError();
	}
	else
	{
		int index = GetPlayerIndex(id);
		if(index == -1)
		{
			ERROR(Format("Client_Say: Broken packet, missing player %d.", id));
			StreamError();
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

	if(!stream.Read(id)
		|| !ReadString1(stream))
	{
		ERROR("Client_Whisper: Broken packet.");
		StreamError();
	}
	else
	{
		int index = GetPlayerIndex(id);
		if(index == -1)
		{
			ERROR(Format("Client_Whisper: Broken packet, missing player %d.", id));
			StreamError();
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
		StreamError();
	}
	else
		AddServerMsg(BUF);
}

//=================================================================================================
void Game::Server_Say(BitStream& stream, PlayerInfo& info, Packet* packet)
{
	byte id;

	if(!stream.Read(id)
		|| !ReadString1(stream))
	{
		ERROR(Format("Server_Say: Broken packet from %s: %s.", info.name.c_str()));
		StreamError();
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

	if(!stream.Read(id)
		|| !ReadString1(stream))
	{
		ERROR(Format("Server_Whisper: Broken packet from %s.", info.name.c_str()));
		StreamError();
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
				StreamError();
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

	return nullptr;
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
	devmode = default_devmode;
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
			sp.devmode = old->devmode;
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
	
	return nullptr;
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

	return nullptr;
}

//=================================================================================================
// -2 read error, -1 not found, 0 empty, 1 found
int Game::ReadItemAndFind(BitStream& s, const Item*& item) const
{
	item = nullptr;

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
			WARN(Format("Missing quest item '%s' (%d).", BUF, quest_refid));
			return -1;
		}
		else
			return 1;
	}
	else
	{
		item = FindItem(BUF);
		if(!item)
		{
			WARN(Format("Missing item '%s'.", BUF));
			return -1;
		}
		else
			return 1;
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

	return nullptr;
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

	return nullptr;
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

	return nullptr;
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

	return nullptr;
}

//=================================================================================================
void Game::ReequipItemsMP(Unit& unit)
{
	if(players > 1)
	{
		const Item* prev_slots[SLOT_MAX];

		for(int i=0; i<SLOT_MAX; ++i)
			prev_slots[i] = unit.slots[i];

		unit.ReequipItems();

		for(int i=0; i<SLOT_MAX; ++i)
		{
			if(unit.slots[i] != prev_slots[i])
			{
				NetChange& c = Add1(net_changes);
				c.type = NetChange::CHANGE_EQUIPMENT;
				c.unit = &unit;
				c.id = i;
			}
		}
	}
	else
		unit.ReequipItems();
}

//=================================================================================================
void Game::UseDays(PlayerController* player, int count)
{
	assert(player && count > 0);

	if(player->free_days >= count)
		player->free_days -= count;
	else
	{
		count -= player->free_days;
		player->free_days = 0;

		for(vector<PlayerInfo>::iterator it = game_players.begin(), end = game_players.end(); it != end; ++it)
		{
			if(!it->left && it->u->player != player)
				it->u->player->free_days += count;
		}

		WorldProgress(count, WPM_NORMAL);
	}

	PushNetChange(NetChange::UPDATE_FREE_DAYS);
}

//=================================================================================================
PlayerInfo* Game::FindOldPlayer(cstring nick)
{
	assert(nick);

	for(PlayerInfo& info : old_players)
	{
		if(info.name == nick)
			return &info;
	}

	return nullptr;
}

//=================================================================================================
void Game::PrepareWorldData(BitStream& stream)
{
	LOG("Preparing world data.");

	stream.Reset();
	stream.WriteCasted<byte>(ID_WORLD_DATA);

	// locations
	stream.WriteCasted<byte>(locations.size());
	for(Location* loc_ptr : locations)
	{
		if(!loc_ptr)
		{
			stream.WriteCasted<byte>(L_NULL);
			continue;
		}

		Location& loc = *loc_ptr;
		stream.WriteCasted<byte>(loc.type);
		if(loc.type == L_DUNGEON || loc.type == L_CRYPT)
			stream.WriteCasted<byte>(loc.GetLastLevel()+1);
		else if(loc.type == L_CITY || loc.type == L_VILLAGE)
		{
			City& city = (City&)loc;
			stream.WriteCasted<byte>(city.citizens);
			stream.WriteCasted<word>(city.citizens_world);
			if(loc.type == L_VILLAGE)
			{
				Village& village = (Village&)city;
				stream.WriteCasted<byte>(village.v_buildings[0]);
				stream.WriteCasted<byte>(village.v_buildings[1]);
			}
		}
		stream.WriteCasted<byte>(loc.state);
		stream.Write(loc.pos);
		WriteString1(stream, loc.name);
	}
	stream.WriteCasted<byte>(current_location);

	// quests
	stream.WriteCasted<word>(quests.size());
	for(Quest* quest : quests)
	{
		stream.Write(quest->refid);
		stream.WriteCasted<byte>(quest->state);
		WriteString1(stream, quest->name);
		WriteStringArray<byte,word>(stream, quest->msgs);
	}

	// rumors
	WriteStringArray<byte,word>(stream, rumors);

	// time
	stream.WriteCasted<byte>(year);
	stream.WriteCasted<byte>(month);
	stream.WriteCasted<byte>(day);
	stream.Write(worldtime);
	stream.Write(gt_hour);
	stream.WriteCasted<byte>(gt_minute);
	stream.WriteCasted<byte>(gt_second);

	// mp vars
	WriteNetVars(stream);

	// quest items
	stream.WriteCasted<word>(net_changes.size());
	for(NetChange& c : net_changes)
	{
		assert(c.type == NetChange::REGISTER_ITEM);
		WriteString1(stream, c.base_item->id);
		WriteString1(stream, c.item2->id);
		WriteString1(stream, c.item2->name);
		WriteString1(stream, c.item2->desc);
		stream.Write(c.item2->refid);
	}
	net_changes.clear();
	if(!net_talk.empty())
		StringPool.Free(net_talk);

	// secret note text
	WriteString1(stream, GetSecretNote()->desc);

	// position on world map
	if(world_state == WS_TRAVEL)
	{
		WriteBool(stream, true);
		stream.Write(picked_location);
		stream.Write(travel_day);
		stream.Write(travel_time);
		stream.Write(travel_start);
		stream.Write(world_pos);
	}
	else
		WriteBool(stream, false);

	stream.WriteCasted<byte>(0xFF);
}

//=================================================================================================
bool Game::ReadWorldData(BitStream& stream)
{
	// count of locations
	byte count;
	if(!stream.Read(count)
		|| !EnsureSize(stream, count))
	{
		ERROR("Read world: Broken packet.");
		return false;
	}

	// locations
	locations.resize(count);
	uint index = 0;
	for(Location*& loc : locations)
	{
		LOCATION type;

		if(!stream.ReadCasted<byte>(type))
		{
			ERROR(Format("Read world: Broken packet for location %u.", index));
			return false;
		}

		if(type == L_NULL)
		{
			loc = nullptr;
			++index;
			continue;
		}

		switch(type)
		{
		case L_DUNGEON:
		case L_CRYPT:
			{
				byte levels;
				if(!stream.Read(levels))
				{
					ERROR(Format("Read world: Broken packet for dungeon location %u.", index));
					return false;
				}
				else if(levels == 0)
				{
					ERROR(Format("Read world: Location %d with 0 levels.", index));
					return false;
				}

				if(levels == 1)
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
				if(!stream.Read(citizens)
					|| !stream.Read(world_citizens))
				{
					ERROR(Format("Read world: Broken packet for city location %u.", index));
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

					if(!stream.ReadCasted<byte>(village->v_buildings[0])
						|| !stream.ReadCasted<byte>(village->v_buildings[1]))
					{
						ERROR(Format("Read world: Broken packet for village location %u.", index));
						return false;
					}

					if(village->v_buildings[0] >= B_MAX)
					{
						ERROR(Format("Read world: Unknown building type %d for village location %u.", village->v_buildings[0], index));
						return false;
					}

					if(village->v_buildings[1] > B_MAX)
					{
						ERROR(Format("Read world: Unknown second building type %d for village location %u.", village->v_buildings[1], index));
						return false;
					}
				}
			}
			break;
		default:
			ERROR(Format("Read world: Unknown location type %d for location %u.", type, index));
			return false;
		}

		// location data
		if(!stream.ReadCasted<byte>(loc->state)
			|| !stream.Read(loc->pos)
			|| !ReadString1(stream, loc->name))
		{
			ERROR(Format("Read world: Broken packet(2) for location %u.", index));
			return false;
		}
		loc->type = type;		
		if(loc->state > LS_CLEARED)
		{
			ERROR(Format("Read world: Invalid state %d for location %u.", loc->state, index));
			return false;
		}

		++index;
	}

	// current location
	if(!stream.ReadCasted<byte>(current_location))
	{
		ERROR("Read world: Broken packet for current location.");
		return false;
	}
	if(current_location >= (int)locations.size() || !locations[current_location])
	{
		ERROR(Format("Read world: Invalid location %d.", current_location));
		return false;
	}
	world_pos = locations[current_location]->pos;
	locations[current_location]->state = LS_VISITED;

	// quests
	const int QUEST_MIN_SIZE = sizeof(int) + sizeof(byte) * 3;
	word quest_count;
	if(!stream.Read(quest_count)
		|| !EnsureSize(stream, QUEST_MIN_SIZE * quest_count))
	{
		ERROR("Read world: Broken packet for quests.");
		return false;
	}
	quests.resize(quest_count);
	index = 0;
	for(Quest*& quest : quests)
	{
		quest = new PlaceholderQuest;
		quest->quest_index = index;
		if(	!stream.Read(quest->refid) ||
			!stream.ReadCasted<byte>(quest->state) ||
			!ReadString1(stream, quest->name) ||
			!ReadStringArray<byte,word>(stream, quest->msgs))
		{
			ERROR(Format("Read world: Broken packet for quest %d.", index));
			return false;
		}
	}

	// rumors
	if(!ReadStringArray<byte,word>(stream, rumors))
	{
		ERROR("Read world: Broken packet for rumors.");
		return false;
	}

	// time
	if(	!stream.ReadCasted<byte>(year) ||
		!stream.ReadCasted<byte>(month) ||
		!stream.ReadCasted<byte>(day) ||
		!stream.Read(worldtime) ||
		!stream.Read(gt_hour) ||
		!stream.ReadCasted<byte>(gt_minute) ||
		!stream.ReadCasted<byte>(gt_second))
	{
		ERROR("Read world: Broken packet for time.");
		return false;
	}

	// mp vars
	if(!ReadNetVars(stream))
	{
		ERROR("Read world: Broken packet for mp vars.");
		return false;
	}

	// questowe przedmioty
	const int QUEST_ITEM_MIN_SIZE = 7;
	word quest_items_count;
	if(!stream.Read(quest_items_count)
		|| !EnsureSize(stream, QUEST_ITEM_MIN_SIZE * quest_items_count))
	{
		ERROR("Read world: Broken packet for quest items.");
		return false;
	}
	quest_items.reserve(quest_items_count);
	for(word i = 0; i < quest_items_count; ++i)
	{
		const Item* base_item;
		if(!ReadItemSimple(stream, base_item))
		{
			ERROR(Format("Read world: Broken packet for quest item %u.", i));
			return false;
		}
		else
		{
			Item* item = CreateItemCopy(base_item);
			if(!ReadString1(stream, item->id)
				|| !ReadString1(stream, item->name)
				|| !ReadString1(stream, item->desc)
				|| !stream.Read(item->refid))
			{
				ERROR(Format("Read world: Broken packet for quest item %u (2).", i));
				delete item;
				return false;
			}
			else
				quest_items.push_back(item);
		}
	}

	// secret note text
	if(!ReadString1(stream, GetSecretNote()->desc))
	{
		ERROR("Read world: Broken packet for secret note text.");
		return false;
	}

	// position on world map
	bool in_travel;
	if(!ReadBool(stream, in_travel))
	{
		ERROR("Read world: Broken packet for in travel data.");
		return false;
	}
	if(in_travel)
	{
		world_state = WS_TRAVEL;
		if(!stream.Read(picked_location) ||
			!stream.Read(travel_day) ||
			!stream.Read(travel_time) ||
			!stream.Read(travel_start) ||
			!stream.Read(world_pos))
		{
			ERROR("Read world: Broken packet for in travel data (2).");
			return false;
		}
	}

	// checksum
	byte check;
	if(!stream.Read(check) || check != 0xFF)
	{
		ERROR("Read world: Broken checksum.");
		return false;
	}

	// load music
	if(!nomusic)
	{
		LoadMusic(MusicType::Boss, false);
		LoadMusic(MusicType::Death, false);
		LoadMusic(MusicType::Travel, false);
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
	for(Unit* unit : *local_ctx.units)
	{
		if(unit != pc->unit)
			UpdateInterpolator(unit->interp, dt, unit->visual_pos, unit->rot);
		if(unit->ani->ani->head.n_groups == 1)
		{
			if(!unit->ani->groups[0].anim)
			{
				unit->action = A_NONE;
				unit->animation = ANI_STAND;
			}
		}
		else
		{
			if(!unit->ani->groups[0].anim && !unit->ani->groups[1].anim)
			{
				unit->action = A_NONE;
				unit->animation = ANI_STAND;
			}
		}
	}
	if(city_ctx)
	{
		for(InsideBuilding* inside : city_ctx->inside_buildings)
		{
			for(Unit* unit : inside->units)
			{
				if(unit != pc->unit)
					UpdateInterpolator(unit->interp, dt, unit->visual_pos, unit->rot);
				if(unit->ani->ani->head.n_groups == 1)
				{
					if(!unit->ani->groups[0].anim)
					{
						unit->action = A_NONE;
						unit->animation = ANI_STAND;
					}
				}
				else
				{
					if(!unit->ani->groups[0].anim && !unit->ani->groups[1].anim)
					{
						unit->action = A_NONE;
						unit->animation = ANI_STAND;
					}
				}
			}
		}
	}
}

//=================================================================================================
void Game::InterpolatePlayers(float dt)
{
	for(PlayerInfo& info : game_players)
	{
		if(info.id != my_id && !info.left)
			UpdateInterpolator(info.u->interp, dt, info.u->visual_pos, info.u->rot);
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
void Game::WritePlayerStartData(BitStream& stream, PlayerInfo& info)
{
	// flags
	byte flags = 0;
	if(info.devmode)
		flags |= 0x01;
	if(mp_load)
	{
		if(info.u->invisible)
			flags |= 0x02;
		if(info.u->player->noclip)
			flags |= 0x04;
		if(info.u->player->godmode)
			flags |= 0x08;
	}
	if(noai)
		flags |= 0x10;
	stream.Write(flags);

	// notes
	WriteStringArray<word,word>(stream, info.notes);

	stream.WriteCasted<byte>(0xFF);
}

//=================================================================================================
bool Game::ReadPlayerStartData(BitStream& stream)
{
	byte flags;

	if(	!stream.Read(flags) ||
		!ReadStringArray<word,word>(stream, notes))
		return false;

	if(IS_SET(flags, 0x01))
		devmode = true;
	if(IS_SET(flags, 0x02))
		invisible = true;
	if(IS_SET(flags, 0x04))
		noclip = true;
	if(IS_SET(flags, 0x08))
		godmode = true;
	if(IS_SET(flags, 0x10))
		noai = true;

	// checksum
	byte check;
	if(!stream.Read(check) || check != 0xFF)
	{
		ERROR("Read player start data: Broken checksum.");
		return false;
	}

	return true;
}

//=================================================================================================
bool Game::CheckMoveNet(Unit& unit, const VEC3& pos)
{
	global_col.clear();

	const float radius = unit.GetUnitRadius();
	IgnoreObjects ignore = {0};
	Unit* ignored[] = { &unit, nullptr };
	const void* ignored_objects[2] = {nullptr, nullptr};
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
Unit* Game::FindTeamMember(int netid)
{
	for(Unit* unit : active_team)
	{
		if(unit->netid == netid)
			return unit;
	}
	return nullptr;
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
	for(int player_id : players_left)
	{
		// order of changes is importat here
		PlayerInfo& info = GetPlayerInfo(player_id);
		NetChange& c = Add1(net_changes);
		c.type = NetChange::REMOVE_PLAYER;
		c.id = player_id;
		c.ile = info.left_reason;

		--players;
		if(info.left_reason != PlayerInfo::LEFT_KICK)
		{
			LOG(Format("Player %s left game.", info.name.c_str()));
			AddMsg(Format(txPlayerLeft, info.name.c_str()));
		}

		if(info.u)
		{
			Unit* unit = info.u;

			if(before_player_ptr.unit == unit)
				before_player = BP_NONE;
			if(info.left_reason == PlayerInfo::LEFT_LOADING || game_state == GS_WORLDMAP)
			{
				if(open_location != -1)
					RemoveElement(GetContext(*unit).units, unit);
				RemoveElement(team, unit);
				RemoveElement(active_team, unit);
				if(unit->interp)
					interpolators.Free(unit->interp);
				if(unit->cobj)
					delete unit->cobj->getCollisionShape();
				delete unit;
				info.u = nullptr;
			}
			else
			{
				if(unit->useable)
					unit->useable->user = nullptr;
				switch(unit->player->action)
				{
				case PlayerController::Action_LootChest:
					{
						// close chest
						unit->player->action_chest->looted = false;
						unit->player->action_chest->ani->Play(&unit->player->action_chest->ani->ani->anims[0],
							PLAY_PRIO1|PLAY_ONCE|PLAY_STOP_AT_END|PLAY_BACK, 0);
						if(sound_volume)
						{
							VEC3 pos = unit->player->action_chest->pos;
							pos.y += 0.5f;
							PlaySound3d(sChestClose, pos, 2.f, 5.f);
						}
						NetChange& c = Add1(net_changes);
						c.type = NetChange::CHEST_CLOSE;
						c.id = unit->player->action_chest->netid;
					}
					break;
				case PlayerController::Action_LootUnit:
					unit->player->action_unit->busy = Unit::Busy_No;
					break;
				case PlayerController::Action_Trade:
				case PlayerController::Action_Talk:
				case PlayerController::Action_GiveItems:
				case PlayerController::Action_ShareItems:
					unit->player->action_unit->busy = Unit::Busy_No;
					unit->player->action_unit->look_target = nullptr;
					break;
				}

				if(contest_state >= CONTEST_STARTING)
					RemoveElementTry(contest_units, info.u);
				if(!arena_free)
					RemoveElementTry(at_arena, info.u);
				if(tournament_state >= TOURNAMENT_IN_PROGRESS)
				{
					RemoveElementTry(tournament_units, info.u);
					for(vector<std::pair<Unit*, Unit*> >::iterator it = tournament_pairs.begin(), end = tournament_pairs.end(); it != end; ++it)
					{
						if(it->first == info.u)
						{
							it->first = nullptr;
							break;
						}
						else if(it->second == info.u)
						{
							it->second = nullptr;
							break;
						}
					}
					if(tournament_skipped_unit == info.u)
						tournament_skipped_unit = nullptr;
					if(tournament_other_fighter == info.u)
						tournament_skipped_unit = nullptr;
				}

				RemoveElement(team, unit);
				RemoveElement(active_team, unit);
				to_remove.push_back(unit);
				unit->to_remove = true;
				info.u = nullptr;
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

			AddMsg(txYouAreLeader);
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
			c.str = nullptr;
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
	case NetChangePlayer::DEVMODE:
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
	RemoveElementOrder(team, info.u);
	RemoveElementOrder(active_team, info.u);
	if(leader == info.u)
		leader_id = -1;
	if(mp_load)
		RemoveElement(GetContext(info.u->pos).units, info.u);
	delete info.u;
	--players;
	peer->CloseConnection(info.adr, true, 0, IMMEDIATE_PRIORITY);
}

//=================================================================================================
BitStream& Game::StreamStart(Packet* packet, StreamLogType type)
{
	assert(packet);
	assert(!current_packet);
	if(current_packet)
	{
		ERROR("Unclosed stream.");
		StreamError();
	}

	current_packet = packet;
	current_stream.~BitStream();
	new((void*)&current_stream)BitStream(packet->data, packet->length, false);
	ErrorHandler::Get().StreamStart(current_packet, (int)type);

	return current_stream;
}

//=================================================================================================
void Game::StreamEnd()
{
	if(!current_packet)
		return;

	ErrorHandler::Get().StreamEnd(true);
	current_packet = nullptr;
}

//=================================================================================================
void Game::StreamError()
{
	if(!current_packet)
		return;

	ErrorHandler::Get().StreamEnd(false);
	current_packet = nullptr;
}
