#include "Pch.h"
#include "GameCore.h"
#include "Game.h"
#include "GameStats.h"
#include "BitStreamFunc.h"
#include "Version.h"
#include "Inventory.h"
#include "Journal.h"
#include "TeamPanel.h"
#include "ErrorHandler.h"
#include "Content.h"
#include "QuestManager.h"
#include "Quest.h"
#include "City.h"
#include "InsideLocation.h"
#include "MultiInsideLocation.h"
#include "CaveLocation.h"
#include "ServerPanel.h"
#include "InfoBox.h"
#include "LoadScreen.h"
#include "GameGui.h"
#include "WorldMapGui.h"
#include "MpBox.h"
#include "AIController.h"
#include "Spell.h"
#include "Team.h"
#include "Action.h"
#include "SoundManager.h"
#include "ScriptManager.h"
#include "Portal.h"

vector<NetChange> Net::changes;
Net::Mode Net::mode;
extern bool merchant_buy[];
extern bool blacksmith_buy[];
extern bool alchemist_buy[];
extern bool innkeeper_buy[];
extern bool foodseller_buy[];
static Unit* SUMMONER_PLACEHOLDER = (Unit*)0xFA4E1111;

//=================================================================================================
inline bool ReadItemSimple(BitStreamReader& f, const Item*& item)
{
	const string& item_id = f.ReadString1();
	if(!f)
		return false;

	if(item_id[0] == '$')
		item = Item::TryGet(item_id.c_str() + 1);
	else
		item = Item::TryGet(item_id);

	return (item != nullptr);
}

//=================================================================================================
inline void WriteBaseItem(BitStreamWriter& f, const Item& item)
{
	f << item.id;
	if(item.id[0] == '$')
		f << item.refid;
}

//=================================================================================================
inline void WriteBaseItem(BitStreamWriter& f, const Item* item)
{
	if(item)
		WriteBaseItem(f, *item);
	else
		f.Write0();
}

//=================================================================================================
inline void WriteItemList(BitStreamWriter& f, vector<ItemSlot>& items)
{
	f << items.size();
	for(ItemSlot& slot : items)
	{
		WriteBaseItem(f, *slot.item);
		f << slot.count;
	}
}

//=================================================================================================
inline void WriteItemListTeam(BitStreamWriter& f, vector<ItemSlot>& items)
{
	f << items.size();
	for(ItemSlot& slot : items)
	{
		WriteBaseItem(f, *slot.item);
		f << slot.count;
		f << slot.team_count;
	}
}

//=================================================================================================
bool Game::ReadItemList(BitStreamReader& f, vector<ItemSlot>& items)
{
	const int MIN_SIZE = 5;

	uint count = f.Read<uint>();
	if(!f.Ensure(count * MIN_SIZE))
		return false;

	items.resize(count);
	for(ItemSlot& slot : items)
	{
		if(ReadItemAndFind(f, slot.item) < 1)
			return false;
		f >> slot.count;
		if(!f)
			return false;
		PreloadItem(slot.item);
		slot.team_count = 0;
	}

	return true;
}

//=================================================================================================
bool Game::ReadItemListTeam(BitStreamReader& f, vector<ItemSlot>& items, bool skip)
{
	const int MIN_SIZE = 9;

	uint count;
	f >> count;
	if(!f.Ensure(count * MIN_SIZE))
		return false;

	items.resize(count);
	for(ItemSlot& slot : items)
	{
		if(ReadItemAndFind(f, slot.item) < 1)
			return false;
		f >> slot.count;
		f >> slot.team_count;
		if(!f)
			return false;
		if(!skip)
			PreloadItem(slot.item);
	}

	return true;
}

//=================================================================================================
void Game::InitServer()
{
	Info("Creating server (port %d)...", mp_port);

	if(!peer)
		peer = SLNet::RakPeerInterface::GetInstance();

	SocketDescriptor sd(mp_port, 0);
	sd.socketFamily = AF_INET;
	StartupResult r = peer->Startup(max_players + 4, &sd, 1);
	if(r != RAKNET_STARTED)
	{
		Error("Failed to create server: RakNet error %d.", r);
		throw Format(txCreateServerFailed, r);
	}

	if(!server_pswd.empty())
	{
		Info("Set server password.");
		peer->SetIncomingPassword(server_pswd.c_str(), server_pswd.length());
	}

	peer->SetMaximumIncomingConnections((word)max_players + 4);
	DEBUG_DO(peer->SetTimeoutTime(60 * 60 * 1000, UNASSIGNED_SYSTEM_ADDRESS));

	Info("Server created. Waiting for connection.");

	Net::SetMode(Net::Mode::Server);
	sv_startup = false;
	Info("sv_online = true");
}

//=================================================================================================
void Game::InitClient()
{
	Info("Initlializing client...");

	if(!peer)
		peer = SLNet::RakPeerInterface::GetInstance();

	SLNet::SocketDescriptor sd;
	sd.socketFamily = AF_INET;
	StartupResult r = peer->Startup(1, &sd, 1);
	if(r != RAKNET_STARTED)
	{
		Error("Failed to create client: RakNet error %d.", r);
		throw Format(txInitConnectionFailed, r);
	}
	peer->SetMaximumIncomingConnections(0);

	Net::SetMode(Net::Mode::Client);
	Info("sv_online = true");

	DEBUG_DO(peer->SetTimeoutTime(60 * 60 * 1000, UNASSIGNED_SYSTEM_ADDRESS));
}

//=================================================================================================
void Game::UpdateServerInfo()
{
	// 0 char C
	// 1 char A
	// 2-5 int - wersja
	// 6 byte - gracze
	// 7 byte - max graczy
	// 8 byte - flagi (0x01 - has�o, 0x02 - wczytana gra)
	// 9+ byte - nazwa
	server_info.Reset();
	BitStreamWriter f(server_info);
	f.WriteCasted<byte>('C');
	f.WriteCasted<byte>('A');
	f << VERSION;
	f.WriteCasted<byte>(players);
	f.WriteCasted<byte>(max_players);
	byte flags = 0;
	if(!server_pswd.empty())
		flags |= 0x01;
	if(mp_load)
		flags |= 0x02;
	f.WriteCasted<byte>(flags);
	f << server_name;

	peer->SetOfflinePingResponse((cstring)server_info.GetData(), server_info.GetNumberOfBytesUsed());
}

//=================================================================================================
int Game::FindPlayerIndex(cstring nick, bool not_left)
{
	assert(nick);

	int index = 0;
	for(auto player : game_players)
	{
		if(player->name == nick)
		{
			if(not_left && player->left != PlayerInfo::LEFT_NO)
				return -1;
			return index;
		}
		++index;
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
		AddGameMsg(msg, 2.f + float(strlen(msg)) / 10);
		AddMultiMsg(s);
	}
}

//=================================================================================================
void Game::KickPlayer(int index)
{
	PlayerInfo& info = *game_players[index];

	// wy�lij informacje o kicku
	packet_data.resize(2);
	packet_data[0] = ID_SERVER_CLOSE;
	packet_data[1] = 1;
	peer->Send((cstring)&packet_data[0], 2, MEDIUM_PRIORITY, RELIABLE, 0, info.adr, false);
	StreamWrite(packet_data, Stream_None, info.adr);

	info.state = PlayerInfo::REMOVING;

	if(server_panel->visible)
	{
		AddMsg(Format(txPlayerKicked, info.name.c_str()));
		Info("Player %s was kicked.", info.name.c_str());

		if(players > 2)
			AddLobbyUpdate(Int2(Lobby_KickPlayer, info.id));

		// puki co tylko dla lobby
		CheckReady();

		UpdateServerInfo();
	}
	else
	{
		info.left = PlayerInfo::LEFT_KICK;
		players_left = true;
	}
}

//=================================================================================================
int Game::GetPlayerIndex(int id)
{
	assert(InRange(id, 0, 255));
	int index = 0;
	for(auto player : game_players)
	{
		if(player->id == id)
			return index;
		++index;
	}
	return -1;
}

//=================================================================================================
int Game::FindPlayerIndex(const SystemAddress& adr)
{
	assert(adr != UNASSIGNED_SYSTEM_ADDRESS);
	int index = 0;
	for(auto player : game_players)
	{
		if(player->adr == adr)
			return index;
		++index;
	}
	return -1;
}

//=================================================================================================
void Game::PrepareLevelData(BitStream& stream, bool loaded_resources)
{
	BitStreamWriter f(stream);
	f << ID_LEVEL_DATA;
	f << mp_load;
	f << loaded_resources;

	if(location->outside)
	{
		// outside location
		OutsideLocation* outside = (OutsideLocation*)location;
		f.Write((cstring)outside->tiles, sizeof(TerrainTile)*outside->size*outside->size);
		f.Write((cstring)outside->h, sizeof(float)*(outside->size + 1)*(outside->size + 1));
		if(location->type == L_CITY)
		{
			City* city = (City*)location;
			f.WriteCasted<byte>(city->flags);
			f.WriteCasted<byte>(city->entry_points.size());
			for(EntryPoint& entry_point : city->entry_points)
			{
				f << entry_point.exit_area;
				f << entry_point.exit_y;
			}
			f.WriteCasted<byte>(city->buildings.size());
			for(CityBuilding& building : city->buildings)
			{
				f << building.type->id;
				f << building.pt;
				f.WriteCasted<byte>(building.rot);
			}
			f.WriteCasted<byte>(city->inside_buildings.size());
			for(InsideBuilding* inside_building : city->inside_buildings)
			{
				InsideBuilding& ib = *inside_building;
				f << ib.level_shift;
				f << ib.type->id;
				// usable objects
				f.WriteCasted<byte>(ib.usables.size());
				for(Usable* usable : ib.usables)
					usable->Write(f);
				// units
				f.WriteCasted<byte>(ib.units.size());
				for(Unit* unit : ib.units)
					WriteUnit(f, *unit);
				// doors
				f.WriteCasted<byte>(ib.doors.size());
				for(Door* door : ib.doors)
					WriteDoor(f, *door);
				// ground items
				f.WriteCasted<byte>(ib.items.size());
				for(GroundItem* item : ib.items)
					WriteItem(f, *item);
				// bloods
				f.WriteCasted<word>(ib.bloods.size());
				for(Blood& blood : ib.bloods)
					blood.Write(f);
				// objects
				f.WriteCasted<byte>(ib.objects.size());
				for(Object* object : ib.objects)
					object->Write(f);
				// lights
				f.WriteCasted<byte>(ib.lights.size());
				for(Light& light : ib.lights)
					light.Write(f);
				// other
				f << ib.xsphere_pos;
				f << ib.enter_area;
				f << ib.exit_area;
				f << ib.top;
				f << ib.xsphere_radius;
				f << ib.enter_y;
			}
		}
		f << light_angle;
	}
	else
	{
		// inside location
		InsideLocation* inside = (InsideLocation*)location;
		InsideLocationLevel& lvl = inside->GetLevelData();
		f.WriteCasted<byte>(inside->target);
		f << inside->from_portal;
		// map
		f.WriteCasted<byte>(lvl.w);
		f.Write((cstring)lvl.map, sizeof(Pole)*lvl.w*lvl.h);
		// lights
		f.WriteCasted<byte>(lvl.lights.size());
		for(Light& light : lvl.lights)
			light.Write(f);
		// rooms
		f.WriteCasted<byte>(lvl.rooms.size());
		for(Room& room : lvl.rooms)
			room.Write(f);
		// traps
		f.WriteCasted<byte>(lvl.traps.size());
		for(Trap* trap : lvl.traps)
			WriteTrap(f, *trap);
		// doors
		f.WriteCasted<byte>(lvl.doors.size());
		for(Door* door : lvl.doors)
			WriteDoor(f, *door);
		// stairs
		f << lvl.staircase_up;
		f << lvl.staircase_down;
		f.WriteCasted<byte>(lvl.staircase_up_dir);
		f.WriteCasted<byte>(lvl.staircase_down_dir);
		f << lvl.staircase_down_in_wall;
	}

	// usable objects
	f.WriteCasted<byte>(local_ctx.usables->size());
	for(Usable* usable : *local_ctx.usables)
		usable->Write(f);
	// units
	f.WriteCasted<byte>(local_ctx.units->size());
	for(Unit* unit : *local_ctx.units)
		WriteUnit(f, *unit);
	// ground items
	f.WriteCasted<byte>(local_ctx.items->size());
	for(GroundItem* item : *local_ctx.items)
		WriteItem(f, *item);
	// bloods
	f.WriteCasted<word>(local_ctx.bloods->size());
	for(Blood& blood : *local_ctx.bloods)
		blood.Write(f);
	// objects
	f.WriteCasted<word>(local_ctx.objects->size());
	for(Object* object : *local_ctx.objects)
		object->Write(f);
	// chests
	f.WriteCasted<byte>(local_ctx.chests->size());
	for(Chest* chest : *local_ctx.chests)
		WriteChest(f, *chest);

	location->WritePortals(f);

	// items preload
	f << items_load.size();
	for(const Item* item : items_load)
	{
		f << item->id;
		if(item->IsQuest())
			f << item->refid;
	}

	// saved bullets, spells, explosions etc
	if(mp_load)
	{
		// bullets
		f.WriteCasted<byte>(local_ctx.bullets->size());
		for(Bullet& bullet : *local_ctx.bullets)
		{
			f << bullet.pos;
			f << bullet.rot;
			f << bullet.speed;
			f << bullet.yspeed;
			f << bullet.timer;
			f << (bullet.owner ? bullet.owner->netid : -1);
			if(bullet.spell)
				f << bullet.spell->id;
			else
				f.Write0();
		}

		// explosions
		f.WriteCasted<byte>(local_ctx.explos->size());
		for(Explo* explo : *local_ctx.explos)
		{
			f << explo->tex->filename;
			f << explo->pos;
			f << explo->size;
			f << explo->sizemax;
		}

		// electros
		f.WriteCasted<byte>(local_ctx.electros->size());
		for(Electro* electro : *local_ctx.electros)
		{
			f << electro->netid;
			f.WriteCasted<byte>(electro->lines.size());
			for(ElectroLine& line : electro->lines)
			{
				f << line.pts.front();
				f << line.pts.back();
				f << line.t;
			}
		}
	}

	f.WriteCasted<byte>(GetLocationMusic());
	f.WriteCasted<byte>(0xFF);
}

//=================================================================================================
void Game::WriteUnit(BitStreamWriter& f, Unit& unit)
{
	// main
	f << unit.data->id;
	f << unit.netid;

	// human data
	if(unit.data->type == UNIT_TYPE::HUMAN)
	{
		f.WriteCasted<byte>(unit.human_data->hair);
		f.WriteCasted<byte>(unit.human_data->beard);
		f.WriteCasted<byte>(unit.human_data->mustache);
		f << unit.human_data->hair_color;
		f << unit.human_data->height;
	}

	// items
	if(unit.data->type != UNIT_TYPE::ANIMAL)
	{
		if(unit.HaveWeapon())
			f << unit.GetWeapon().id;
		else
			f.Write0();
		if(unit.HaveBow())
			f << unit.GetBow().id;
		else
			f.Write0();
		if(unit.HaveShield())
			f << unit.GetShield().id;
		else
			f.Write0();
		if(unit.HaveArmor())
			f << unit.GetArmor().id;
		else
			f.Write0();
	}
	f.WriteCasted<byte>(unit.live_state);
	f << unit.pos;
	f << unit.rot;
	f << unit.hp;
	f << unit.hpmax;
	f << unit.netid;
	f.WriteCasted<char>(unit.in_arena);
	f << (unit.summoner != nullptr);

	// hero/player data
	byte b;
	if(unit.IsHero())
		b = 1;
	else if(unit.IsPlayer())
		b = 2;
	else
		b = 0;
	f << b;
	if(unit.IsHero())
	{
		f << unit.hero->name;
		b = 0;
		if(unit.hero->know_name)
			b |= 0x01;
		if(unit.hero->team_member)
			b |= 0x02;
		f << b;
		f << unit.hero->credit;
	}
	else if(unit.IsPlayer())
	{
		f << unit.player->name;
		f.WriteCasted<byte>(unit.player->id);
		f << unit.player->credit;
		f << unit.player->free_days;
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
		f << b;
	}

	// loaded data
	if(mp_load)
	{
		f << unit.netid;
		unit.mesh_inst->Write(f);
		f.WriteCasted<byte>(unit.animation);
		f.WriteCasted<byte>(unit.current_animation);
		f.WriteCasted<byte>(unit.animation_state);
		f.WriteCasted<byte>(unit.attack_id);
		f.WriteCasted<byte>(unit.action);
		f.WriteCasted<byte>(unit.weapon_taken);
		f.WriteCasted<byte>(unit.weapon_hiding);
		f.WriteCasted<byte>(unit.weapon_state);
		f << unit.target_pos;
		f << unit.target_pos2;
		f << unit.timer;
		if(unit.used_item)
			f << unit.used_item->id;
		else
			f.Write0();
		f << (unit.usable ? unit.usable->netid : -1);
	}
}

//=================================================================================================
void Game::WriteDoor(BitStreamWriter& f, Door& door)
{
	f << door.pos;
	f << door.rot;
	f << door.pt;
	f.WriteCasted<byte>(door.locked);
	f.WriteCasted<byte>(door.state);
	f << door.netid;
	f << door.door2;
}

//=================================================================================================
void Game::WriteItem(BitStreamWriter& f, GroundItem& item)
{
	f << item.netid;
	f << item.pos;
	f << item.rot;
	f << item.count;
	f << item.team_count;
	f << item.item->id;
	if(item.item->IsQuest())
		f << item.item->refid;
}

//=================================================================================================
void Game::WriteChest(BitStreamWriter& f, Chest& chest)
{
	f << chest.pos;
	f << chest.rot;
	f << chest.netid;
}

//=================================================================================================
void Game::WriteTrap(BitStreamWriter& f, Trap& trap)
{
	f.WriteCasted<byte>(trap.base->type);
	f.WriteCasted<byte>(trap.dir);
	f << trap.netid;
	f << trap.tile;
	f << trap.pos;
	f << trap.obj.rot.y;

	if(mp_load)
	{
		f.WriteCasted<byte>(trap.state);
		f << trap.time;
	}
}

//=================================================================================================
bool Game::ReadLevelData(BitStreamReader& f)
{
	cam.Reset();
	pc_data.rot_buf = 0.f;
	show_mp_panel = true;
	boss_level_mp = false;
	open_location = 0;

	bool loaded_resources;
	f >> mp_load;
	f >> loaded_resources;
	if(!f)
	{
		Error("Read level: Broken packet for loading info.");
		return false;
	}

	if(!location->outside)
	{
		InsideLocation* inside = (InsideLocation*)location;
		inside->SetActiveLevel(dungeon_level);
	}
	ApplyContext(location, local_ctx);
	RequireLoadingResources(location, &loaded_resources);
	local_ctx_valid = true;
	city_ctx = nullptr;

	if(location->outside)
	{
		// outside location
		SetOutsideParams();
		SetTerrainTextures();

		OutsideLocation* outside = (OutsideLocation*)location;
		int size11 = outside->size*outside->size;
		int size22 = outside->size + 1;
		size22 *= size22;
		if(!outside->tiles)
			outside->tiles = new TerrainTile[size11];
		if(!outside->h)
			outside->h = new float[size22];
		f.Read((char*)outside->tiles, sizeof(TerrainTile)*size11);
		f.Read((char*)outside->h, sizeof(float)*size22);
		if(!f)
		{
			Error("Read level: Broken packet for terrain.");
			return false;
		}
		ApplyTiles(outside->h, outside->tiles);
		SpawnTerrainCollider();
		if(location->type == L_CITY)
		{
			City* city = (City*)location;
			city_ctx = city;

			// entry points
			const int ENTRY_POINT_MIN_SIZE = 20;
			byte count;
			f.ReadCasted<byte>(city->flags);
			f >> count;
			if(!f.Ensure(count * ENTRY_POINT_MIN_SIZE))
			{
				Error("Read level: Broken packet for city.");
				return false;
			}
			city->entry_points.resize(count);
			for(EntryPoint& entry : city->entry_points)
			{
				f.Read(entry.exit_area);
				f.Read(entry.exit_y);
			}
			if(!f)
			{
				Error("Read level: Broken packet for entry points.");
				return false;
			}

			// buildings
			const int BUILDING_MIN_SIZE = 10;
			f >> count;
			if(f.Ensure(BUILDING_MIN_SIZE * count))
			{
				Error("Read level: Broken packet for buildings count.");
				return false;
			}
			city->buildings.resize(count);
			for(CityBuilding& building : city->buildings)
			{
				const string& building_id = f.ReadString1();
				f >> building.pt;
				f.ReadCasted<byte>(building.rot);
				if(!f)
				{
					Error("Read level: Broken packet for buildings.");
					return false;
				}
				building.type =  Building::Get(building_id);
				if(!building.type)
				{
					Error("Read level: Invalid building id '%s'.", building_id.c_str());
					return false;
				}
			}

			// inside buildings
			const int INSIDE_BUILDING_MIN_SIZE = 73;
			f >> count;
			if(!f.Ensure(INSIDE_BUILDING_MIN_SIZE * count))
			{
				Error("Read level: Broken packet for inside buildings count.");
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
				f >> ib.level_shift;
				const string& building_id = f.ReadString1();
				if(!f)
				{
					Error("Read level: Broken packet for %d inside building.", index);
					return false;
				}
				ib.type = Building::Get(building_id);
				if(!ib.type || !ib.type->inside_mesh)
				{
					Error("Read level: Invalid building id '%s' for building %d.", building_id.c_str(), index);
					return false;
				}
				ib.offset = Vec2(512.f*ib.level_shift.x + 256.f, 512.f*ib.level_shift.y + 256.f);
				ProcessBuildingObjects(ib.ctx, city_ctx, &ib, ib.type->inside_mesh, nullptr, 0, 0, Vec3(ib.offset.x, 0, ib.offset.y), ib.type, nullptr, true);

				// usable objects
				f >> count;
				if(f.Ensure(Usable::MIN_SIZE * count))
				{
					Error("Read level: Broken packet for usable object in %d inside building.", index);
					return false;
				}
				ib.usables.resize(count);
				for(Usable*& usable : ib.usables)
				{
					usable = new Usable;
					if(!usable->Read(f))
					{
						Error("Read level: Broken packet for usable object in %d inside building.", index);
						return false;
					}
				}

				// units
				f >> count;
				if(!f.Ensure(Unit::MIN_SIZE * count))
				{
					Error("Read level: Broken packet for unit count in %d inside building.", index);
					return false;
				}
				ib.units.resize(count);
				for(Unit*& unit : ib.units)
				{
					unit = new Unit;
					if(!ReadUnit(f, *unit))
					{
						Error("Read level: Broken packet for unit in %d inside building.", index);
						return false;
					}
					unit->in_building = index;
				}

				// doors
				f >> count;
				if(!f.Ensure(count * Door::MIN_SIZE))
				{
					Error("Read level: Broken packet for door count in %d inside building.", index);
					return false;
				}
				ib.doors.resize(count);
				for(Door*& door : ib.doors)
				{
					door = new Door;
					if(!ReadDoor(f, *door))
					{
						Error("Read level: Broken packet for door in %d inside building.", index);
						return false;
					}
				}

				// ground items
				f >> count;
				if(!f.Ensure(count * GroundItem::MIN_SIZE))
				{
					Error("Read level: Broken packet for ground item count in %d inside building.", index);
					return false;
				}
				ib.items.resize(count);
				for(GroundItem*& item : ib.items)
				{
					item = new GroundItem;
					if(!ReadItem(f, *item))
					{
						Error("Read level: Broken packet for ground item in %d inside building.", index);
						return false;
					}
				}

				// bloods
				word count2;
				f >> count2;
				if(!f.Ensure(count2 * Blood::MIN_SIZE))
				{
					Error("Read level: Broken packet for blood count in %d inside building.", index);
					return false;
				}
				ib.bloods.resize(count2);
				for(Blood& blood : ib.bloods)
					blood.Read(f);
				if(!f)
				{
					Error("Read level: Broken packet for blood in %d inside building.", index);
					return false;
				}

				// objects
				f >> count;
				if(!f.Ensure(count * Object::MIN_SIZE))
				{
					Error("Read level: Broken packet for object count in %d inside building.", index);
					return false;
				}
				ib.objects.resize(count);
				for(Object*& object : ib.objects)
				{
					object = new Object;
					if(!object->Read(f))
					{
						Error("Read level: Broken packet for object in %d inside building.", index);
						return false;
					}
				}

				// lights
				f >> count;
				if(!f.Ensure(count * Light::MIN_SIZE))
				{
					Error("Read level: Broken packet for light count in %d inside building.", index);
					return false;
				}
				ib.lights.resize(count);
				for(Light& light : ib.lights)
					light.Read(f);
				if(!f)
				{
					Error("Read level: Broken packet for light in %d inside building.", index);
					return false;
				}

				// other
				f >> ib.xsphere_pos;
				f >> ib.enter_area;
				f >> ib.exit_area;
				f >> ib.top;
				f >> ib.xsphere_radius;
				f >> ib.enter_y;
				if(!f)
				{
					Error("Read level: Broken packet for inside building %d other data.", index);
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
		f >> light_angle;
		if(!f)
		{
			Error("Read level: Broken packet for light angle.");
			return false;
		}
		SpawnOutsideBariers();
	}
	else
	{
		// inside location
		InsideLocation* inside = (InsideLocation*)location;
		InsideLocationLevel& lvl = inside->GetLevelData();
		f.ReadCasted<byte>(inside->target);
		f >> inside->from_portal;
		f.ReadCasted<byte>(lvl.w);
		if(!f.Ensure(lvl.w * lvl.w * sizeof(Pole)))
		{
			Error("Read level: Broken packet for inside location.");
			return false;
		}

		// map
		lvl.h = lvl.w;
		if(!lvl.map)
			lvl.map = new Pole[lvl.w * lvl.h];

		// lights
		byte count;
		f >> count;
		if(!f.Ensure(count * Light::MIN_SIZE))
		{
			Error("Read level: Broken packet for inside location light count.");
			return false;
		}
		lvl.lights.resize(count);
		for(Light& light : lvl.lights)
			light.Read(f);
		if(!f)
		{
			Error("Read level: Broken packet for inside location light.");
			return false;
		}

		// rooms
		f >> count;
		if(!f.Ensure(count * Room::MIN_SIZE))
		{
			Error("Read level: Broken packet for inside location room count.");
			return false;
		}
		lvl.rooms.resize(count);
		for(Room& room : lvl.rooms)
			room.Read(f);
		if(!f)
		{
			Error("Read level: Broken packet for inside location room.");
			return false;
		}

		// traps
		f >> count;
		if(!f.Ensure(count * Trap::MIN_SIZE))
		{
			Error("Read level: Broken packet for inside location trap count.");
			return false;
		}
		lvl.traps.resize(count);
		for(Trap*& trap : lvl.traps)
		{
			trap = new Trap;
			if(!ReadTrap(f, *trap))
			{
				Error("Read level: Broken packet for inside location trap.");
				return false;
			}
		}

		// doors
		f >> count;
		if(!f.Ensure(count * Door::MIN_SIZE))
		{
			Error("Read level: Broken packet for inside location door count.");
			return false;
		}
		lvl.doors.resize(count);
		for(Door*& door : lvl.doors)
		{
			door = new Door;
			if(!ReadDoor(f, *door))
			{
				Error("Read level: Broken packet for inside location door.");
				return false;
			}
		}

		// stairs
		f >> lvl.staircase_up;
		f >> lvl.staircase_down;
		f.ReadCasted<byte>(lvl.staircase_up_dir);
		f.ReadCasted<byte>(lvl.staircase_down_dir);
		f >> lvl.staircase_down_in_wall;
		if(!f)
		{
			Error("Read level: Broken packet for stairs.");
			return false;
		}

		BaseLocation& base = g_base_locations[inside->target];
		SetDungeonParamsAndTextures(base);

		SpawnDungeonColliders();
		CreateDungeonMinimap();
	}

	// usable objects
	byte count;
	f >> count;

	if(!f.Ensure(count * Usable::MIN_SIZE))
	{
		Error("Read level: Broken usable object count.");
		return false;
	}
	local_ctx.usables->resize(count);
	for(Usable*& usable : *local_ctx.usables)
	{
		usable = new Usable;
		if(!usable->Read(f))
		{
			Error("Read level: Broken usable object.");
			return false;
		}
	}

	// units
	f >> count;
	if(!f.Ensure(count * Unit::MIN_SIZE))
	{
		Error("Read level: Broken unit count.");
		return false;
	}
	local_ctx.units->resize(count);
	for(Unit*& unit : *local_ctx.units)
	{
		unit = new Unit;
		if(!ReadUnit(f, *unit))
		{
			Error("Read level: Broken unit.");
			return false;
		}
	}

	// ground items
	f >> count;
	if(!f.Ensure(count * GroundItem::MIN_SIZE))
	{
		Error("Read level: Broken ground item count.");
		return false;
	}
	local_ctx.items->resize(count);
	for(GroundItem*& item : *local_ctx.items)
	{
		item = new GroundItem;
		if(!ReadItem(f, *item))
		{
			Error("Read level: Broken ground item.");
			return false;
		}
	}

	// bloods
	word count2;
	f >> count2;
	if(!f.Ensure(count2 * Blood::MIN_SIZE))
	{
		Error("Read level: Broken blood count.");
		return false;
	}
	local_ctx.bloods->resize(count2);
	for(Blood& blood : *local_ctx.bloods)
		blood.Read(f);
	if(!f)
	{
		Error("Read level: Broken blood.");
		return false;
	}

	// objects
	f >> count2;
	if(!f.Ensure(count2 * Object::MIN_SIZE))
	{
		Error("Read level: Broken object count.");
		return false;
	}
	local_ctx.objects->resize(count2);
	for(Object*& object : *local_ctx.objects)
	{
		object = new Object;
		if(!object->Read(f))
		{
			Error("Read level: Broken object.");
			return false;
		}
	}

	// chests
	f >> count;
	if(!f.Ensure(count * Chest::MIN_SIZE))
	{
		Error("Read level: Broken chest count.");
		return false;
	}
	local_ctx.chests->resize(count);
	for(Chest*& chest : *local_ctx.chests)
	{
		chest = new Chest;
		if(!ReadChest(f, *chest))
		{
			Error("Read level: Broken chest.");
			return false;
		}
	}

	// portals
	if(!location->ReadPortals(f, dungeon_level))
	{
		Error("Read level: Broken portals.");
		return false;
	}

	// items to preload
	uint items_load_count = f.Read<uint>();
	if(!f.Ensure(items_load_count * 2))
	{
		Error("Read level: Broken items preload count.");
		return false;
	}
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
			const Item* item = FindQuestItemClient(item_id.c_str(), refid);
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

	// multiplayer data
	if(mp_load)
	{
		// bullets
		f >> count;
		if(!f.Ensure(count * Bullet::MIN_SIZE))
		{
			Error("Read level: Broken bullet count.");
			return false;
		}
		local_ctx.bullets->resize(count);
		for(Bullet& bullet : *local_ctx.bullets)
		{
			f >> bullet.pos;
			f >> bullet.rot;
			f >> bullet.speed;
			f >> bullet.yspeed;
			f >> bullet.timer;
			int netid = f.Read<int>();
			const string& spell_id = f.ReadString1();
			if(!f)
			{
				Error("Read level: Broken bullet.");
				return false;
			}
			if(spell_id.empty())
			{
				bullet.spell = nullptr;
				bullet.mesh = aArrow;
				bullet.pe = nullptr;
				bullet.remove = false;
				bullet.tex = nullptr;
				bullet.tex_size = 0.f;

				TrailParticleEmitter* tpe = new TrailParticleEmitter;
				tpe->fade = 0.3f;
				tpe->color1 = Vec4(1, 1, 1, 0.5f);
				tpe->color2 = Vec4(1, 1, 1, 0);
				tpe->Init(50);
				local_ctx.tpes->push_back(tpe);
				bullet.trail = tpe;

				TrailParticleEmitter* tpe2 = new TrailParticleEmitter;
				tpe2->fade = 0.3f;
				tpe2->color1 = Vec4(1, 1, 1, 0.5f);
				tpe2->color2 = Vec4(1, 1, 1, 0);
				tpe2->Init(50);
				local_ctx.tpes->push_back(tpe2);
				bullet.trail2 = tpe2;
			}
			else
			{
				Spell* spell_ptr = FindSpell(spell_id.c_str());
				if(!spell_ptr)
				{
					Error("Read level: Missing spell '%s'.", spell_id.c_str());
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
					pe->speed_min = Vec3(-1, -1, -1);
					pe->speed_max = Vec3(1, 1, 1);
					pe->pos_min = Vec3(-spell.size, -spell.size, -spell.size);
					pe->pos_max = Vec3(spell.size, spell.size, spell.size);
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
					Error("Read level: Missing bullet owner %d.", netid);
					return false;
				}
			}
			else
				bullet.owner = nullptr;
		}

		// explosions
		f >> count;
		if(!f.Ensure(count * Explo::MIN_SIZE))
		{
			Error("Read level: Broken explosion count.");
			return false;
		}
		local_ctx.explos->resize(count);
		for(Explo*& explo : *local_ctx.explos)
		{
			explo = new Explo;
			const string& tex_id = f.ReadString1();
			f >> explo->pos;
			f >> explo->size;
			f >> explo->sizemax;
			if(!f)
			{
				Error("Read level: Broken explosion.");
				return false;
			}
			explo->tex = ResourceManager::Get<Texture>().GetLoaded(tex_id);
		}

		// electro effects
		f >> count;
		if(!f.Ensure(count * Electro::MIN_SIZE))
		{
			Error("Read level: Broken electro count.");
			return false;
		}
		local_ctx.electros->resize(count);
		Spell* electro_spell = FindSpell("thunder_bolt");
		for(Electro*& electro : *local_ctx.electros)
		{
			electro = new Electro;
			electro->spell = electro_spell;
			electro->valid = true;
			f >> electro->netid;
			f >> count;
			if(!f.Ensure(count * Electro::LINE_MIN_SIZE))
			{
				Error("Read level: Broken electro.");
				return false;
			}
			electro->lines.resize(count);
			Vec3 from, to;
			float t;
			for(byte i = 0; i < count; ++i)
			{
				f >> from;
				f >> to;
				f >> t;
				electro->AddLine(from, to);
				electro->lines.back().t = t;
			}
		}
	}

	// music
	MusicType music;
	f.ReadCasted<byte>(music);
	if(!f)
	{
		Error("Read level: Broken music.");
		return false;
	}
	if(!sound_mgr->IsMusicDisabled())
		LoadMusic(music, false);

	// checksum
	byte check;
	f >> check;
	if(!f || check != 0xFF)
	{
		Error("Read level: Broken checksum.");
		return false;
	}

	RespawnObjectColliders();
	local_ctx_valid = true;
	if(!boss_level_mp)
		SetMusic(music);

	InitQuadTree();
	CalculateQuadtree();

	return true;
}

//=================================================================================================
bool Game::ReadUnit(BitStreamReader& f, Unit& unit)
{
	// main
	const string& id = f.ReadString1();
	f >> unit.netid;
	if(!f)
		return false;
	unit.data = UnitData::TryGet(id);
	if(!unit.data)
	{
		Error("Missing base unit id '%s'!", id.c_str());
		return false;
	}

	// human data
	if(unit.data->type == UNIT_TYPE::HUMAN)
	{
		unit.human_data = new Human;
		f.ReadCasted<byte>(unit.human_data->hair);
		f.ReadCasted<byte>(unit.human_data->beard);
		f.ReadCasted<byte>(unit.human_data->mustache);
		f >> unit.human_data->hair_color;
		f >> unit.human_data->height;
		if(!f)
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
			|| !InRange(unit.human_data->height, 0.85f, 1.15f))
		{
			Error("Invalid human data (hair:%d, beard:%d, mustache:%d, height:%g).", unit.human_data->hair, unit.human_data->beard,
				unit.human_data->mustache, unit.human_data->height);
			return false;
		}
	}
	else
		unit.human_data = nullptr;

	// equipped items
	if(unit.data->type != UNIT_TYPE::ANIMAL)
	{
		for(int i = 0; i < SLOT_MAX; ++i)
		{
			const string& item_id = f.ReadString1();
			if(!f)
				return false;
			if(item_id.empty())
				unit.slots[i] = nullptr;
			else
			{
				const Item* item = Item::TryGet(item_id);
				if(item && ItemTypeToSlot(item->type) == (ITEM_SLOT)i)
				{
					PreloadItem(item);
					unit.slots[i] = item;
				}
				else
				{
					if(item)
						Error("Invalid slot type (%d != %d).", ItemTypeToSlot(item->type), i);
					return false;
				}
			}
		}
	}
	else
	{
		for(int i = 0; i < SLOT_MAX; ++i)
			unit.slots[i] = nullptr;
	}

	// variables
	f.ReadCasted<byte>(unit.live_state);
	f >> unit.pos;
	f >> unit.rot;
	f >> unit.hp;
	f >> unit.hpmax;
	f >> unit.netid;
	f.ReadCasted<char>(unit.in_arena);
	bool summoner = f.Read<bool>();
	if(!f)
		return false;
	if(unit.live_state >= Unit::LIVESTATE_MAX)
	{
		Error("Invalid live state %d.", unit.live_state);
		return false;
	}
	unit.summoner = (summoner ? SUMMONER_PLACEHOLDER : nullptr);

	// hero/player data
	byte type;
	f >> type;
	if(!f)
		return false;
	if(type == 1)
	{
		// hero
		byte flags;
		unit.ai = (AIController*)1; // (X_X)
		unit.player = nullptr;
		unit.hero = new HeroData;
		unit.hero->unit = &unit;
		f >> unit.hero->name;
		f >> flags;
		f >> unit.hero->credit;
		if(!f)
			return false;
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
		f >> unit.player->name;
		f.ReadCasted<byte>(unit.player->id);
		f >> unit.player->credit;
		f >> unit.player->free_days;
		if(!f)
			return false;
		if(unit.player->credit < 0)
		{
			Error("Invalid player %d credit %d.", unit.player->id, unit.player->credit);
			return false;
		}
		if(unit.player->free_days < 0)
		{
			Error("Invalid player %d free days %d.", unit.player->id, unit.player->free_days);
			return false;
		}
		PlayerInfo* info = GetPlayerInfoTry(unit.player->id);
		if(!info)
		{
			Error("Invalid player id %d.", unit.player->id);
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
		f.ReadCasted<byte>(unit.ai_mode);
		if(!f)
			return false;
	}

	// mesh
	unit.CreateMesh(mp_load ? Unit::CREATE_MESH::PRELOAD : Unit::CREATE_MESH::NORMAL);

	unit.action = A_NONE;
	unit.weapon_taken = W_NONE;
	unit.weapon_hiding = W_NONE;
	unit.weapon_state = WS_HIDDEN;
	unit.talking = false;
	unit.busy = Unit::Busy_No;
	unit.in_building = -1;
	unit.frozen = FROZEN::NO;
	unit.usable = nullptr;
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
		f >> unit.netid;
		if(!unit.mesh_inst->Read(f))
			return false;
		f.ReadCasted<byte>(unit.animation);
		f.ReadCasted<byte>(unit.current_animation);
		f.ReadCasted<byte>(unit.animation_state);
		f.ReadCasted<byte>(unit.attack_id);
		f.ReadCasted<byte>(unit.action);
		f.ReadCasted<byte>(unit.weapon_taken);
		f.ReadCasted<byte>(unit.weapon_hiding);
		f.ReadCasted<byte>(unit.weapon_state);
		f >> unit.target_pos;
		f >> unit.target_pos2;
		f >> unit.timer;
		const string& used_item = f.ReadString1();
		int usable_netid = f.Read<int>();
		if(!f)
			return false;

		// used item
		if(!used_item.empty())
		{
			unit.used_item = Item::TryGet(used_item);
			if(!unit.used_item)
			{
				Error("Missing used item '%s'.", used_item.c_str());
				return false;
			}
		}
		else
			unit.used_item = nullptr;

		// usable
		if(usable_netid == -1)
			unit.usable = nullptr;
		else
		{
			unit.usable = FindUsable(usable_netid);
			if(unit.usable)
			{
				unit.use_rot = Vec3::LookAtAngle(unit.pos, unit.usable->pos);
				unit.usable->user = &unit;
			}
			else
			{
				Error("Missing usable %d.", usable_netid);
				return false;
			}
		}

		// bow animesh instance
		if(unit.action == A_SHOOT)
		{
			unit.bow_instance = GetBowInstance(unit.GetBow().mesh);
			unit.bow_instance->Play(&unit.bow_instance->mesh->anims[0], PLAY_ONCE | PLAY_PRIO1 | PLAY_NO_BLEND, 0);
			unit.bow_instance->groups[0].speed = unit.mesh_inst->groups[1].speed;
			unit.bow_instance->groups[0].time = unit.mesh_inst->groups[1].time;
		}
	}

	// physics
	CreateUnitPhysics(unit, true);

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
bool Game::ReadDoor(BitStreamReader& f, Door& door)
{
	f >> door.pos;
	f >> door.rot;
	f >> door.pt;
	f.ReadCasted<byte>(door.locked);
	f.ReadCasted<byte>(door.state);
	f >> door.netid;
	f >> door.door2;
	if(!f)
		return false;

	if(door.state >= Door::Max)
	{
		Error("Invalid door state %d.", door.state);
		return false;
	}

	door.mesh_inst = new MeshInstance(door.door2 ? aDoor2 : aDoor);
	door.mesh_inst->groups[0].speed = 2.f;
	door.phy = new btCollisionObject;
	door.phy->setCollisionShape(shape_door);
	door.phy->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_DOOR);

	btTransform& tr = door.phy->getWorldTransform();
	Vec3 pos = door.pos;
	pos.y += 1.319f;
	tr.setOrigin(ToVector3(pos));
	tr.setRotation(btQuaternion(door.rot, 0, 0));
	phy_world->addCollisionObject(door.phy, CG_DOOR);

	if(door.state == Door::Open)
	{
		btVector3& pos = door.phy->getWorldTransform().getOrigin();
		pos.setY(pos.y() - 100.f);
		door.mesh_inst->SetToEnd(door.mesh_inst->mesh->anims[0].name.c_str());
	}

	return true;
}

//=================================================================================================
bool Game::ReadItem(BitStreamReader& f, GroundItem& item)
{
	f >> item.netid;
	f >> item.pos;
	f >> item.rot;
	f >> item.count;
	f >> item.team_count;
	return f.IsOk() && ReadItemAndFind(f, item.item) > 0;
}

//=================================================================================================
bool Game::ReadChest(BitStreamReader& f, Chest& chest)
{
	f >> chest.pos;
	f >> chest.rot;
	f >> chest.netid;
	if(!f)
		return false;
	chest.mesh_inst = new MeshInstance(aChest);
	return true;
}

//=================================================================================================
bool Game::ReadTrap(BitStreamReader& f, Trap& trap)
{
	TRAP_TYPE type;
	f.ReadCasted<byte>(type);
	f.ReadCasted<byte>(trap.dir);
	f >> trap.netid;
	f >> trap.tile;
	f >> trap.pos;
	f >> trap.obj.rot.y;
	if(!f)
		return false;
	trap.base = &BaseTrap::traps[type];

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
		f.ReadCasted<byte>(trap.state);
		f >> trap.time;
		if(!f)
			return false;
	}

	if(type == TRAP_ARROW || type == TRAP_POISON)
		trap.obj.rot = Vec3(0, 0, 0);
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
	PlayerInfo& info = *game_players[index];
	Unit& unit = *info.u;
	net_stream2.Reset();
	BitStreamWriter f(net_stream2);

	f << ID_PLAYER_DATA;
	f << unit.netid;

	// items
	for(int i = 0; i < SLOT_MAX; ++i)
	{
		if(unit.slots[i])
			f << unit.slots[i]->id;
		else
			f.Write0();
	}
	WriteItemListTeam(f, unit.items);

	// data
	unit.stats.Write(f);
	unit.unmod_stats.Write(f);
	f << unit.gold;
	f << unit.stamina;
	unit.player->Write(f);

	// other team members
	f.WriteCasted<byte>(Team.members.size() - 1);
	for(Unit* other_unit : Team.members)
	{
		if(other_unit != &unit)
			f << other_unit->netid;
	}
	f.WriteCasted<byte>(leader_id);

	// multiplayer load data
	if(mp_load)
	{
		int flags = 0;
		if(unit.run_attack)
			flags |= 0x01;
		if(unit.used_item_is_team)
			flags |= 0x02;
		f << unit.attack_power;
		f << unit.raise_timer;
		f.WriteCasted<byte>(flags);
	}

	f.WriteCasted<byte>(0xFF);

	peer->Send(&net_stream2, HIGH_PRIORITY, RELIABLE, 0, info.adr, false);
	StreamWrite(net_stream2, Stream_TransferServer, info.adr);
}

//=================================================================================================
bool Game::ReadPlayerData(BitStreamReader& f)
{
	int netid = f.Read<int>();
	if(!f)
	{
		Error("Read player data: Broken packet.");
		return false;
	}

	Unit* unit = FindUnit(netid);
	if(!unit)
	{
		Error("Read player data: Missing unit %d.", netid);
		return false;
	}
	game_players[0]->u = unit;
	pc = unit->player;
	pc->player_info = game_players[0];
	game_players[0]->pc = pc;
	game_gui->Setup();

	// items
	for(int i = 0; i < SLOT_MAX; ++i)
	{
		const string& item_id = f.ReadString1();
		if(!f)
		{
			Error("Read player data: Broken item %d.", i);
			return false;
		}
		if(!item_id.empty())
		{
			unit->slots[i] = Item::TryGet(item_id);
			if(!unit->slots[i])
				return false;
		}
		else
			unit->slots[i] = nullptr;
	}
	if(!ReadItemListTeam(f, unit->items))
	{
		Error("Read player data: Broken item list.");
		return false;
	}

	int credit = unit->player->credit,
		free_days = unit->player->free_days;

	unit->player->Init(*unit, true);

	unit->stats.Read(f);
	unit->unmod_stats.Read(f);
	f >> unit->gold;
	f >> unit->stamina;
	if(!f || !pc->Read(f))
	{
		Error("Read player data: Broken stats.");
		return false;
	}

	unit->look_target = nullptr;
	unit->prev_speed = 0.f;
	unit->run_attack = false;

	unit->weight = 0;
	unit->CalculateLoad();
	unit->RecalculateWeight();
	unit->stamina_max = unit->CalculateMaxStamina();

	unit->player->credit = credit;
	unit->player->free_days = free_days;
	unit->player->is_local = true;

	// other team members
	Team.members.clear();
	Team.active_members.clear();
	Team.members.push_back(unit);
	Team.active_members.push_back(unit);
	byte count;
	f >> count;
	if(!f.Ensure(sizeof(int) * count))
	{
		Error("Read player data: Broken team members.");
		return false;
	}
	for(byte i = 0; i < count; ++i)
	{
		f >> netid;
		Unit* team_member = FindUnit(netid);
		if(!team_member)
		{
			Error("Read player data: Missing team member %d.", netid);
			return false;
		}
		Team.members.push_back(team_member);
		if(team_member->IsPlayer() || !team_member->hero->free)
			Team.active_members.push_back(team_member);
	}
	f.ReadCasted<byte>(leader_id);
	if(!f)
	{
		Error("Read player data: Broken team leader.");
		return false;
	}
	PlayerInfo* leader_info = GetPlayerInfoTry(leader_id);
	if(!leader_info)
	{
		Error("Read player data: Missing player %d.", leader_id);
		return false;
	}
	Team.leader = leader_info->u;

	dialog_context.pc = unit->player;
	pc->noclip = noclip;
	pc->godmode = godmode;
	pc->unit->invisible = invisible;

	// multiplayer load data
	if(mp_load)
	{
		byte flags;
		f >> unit->attack_power;
		f >> unit->raise_timer;
		f >> flags;
		if(!f)
		{
			Error("Read player data: Broken multiplaye data.");
			return false;
		}
		unit->run_attack = IS_SET(flags, 0x01);
		unit->used_item_is_team = IS_SET(flags, 0x02);
	}

	// checksum
	byte check;
	f >> check;
	if(!f || check != 0xFF)
	{
		Error("Read player data: Broken checksum.");
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
Unit* Game::FindUnit(delegate<bool(Unit*)> pred)
{
	for(auto u : *local_ctx.units)
	{
		if(pred(u))
			return u;
	}

	if(city_ctx)
	{
		for(auto inside : city_ctx->inside_buildings)
		{
			for(auto u : inside->units)
			{
				if(pred(u))
					return u;
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
	for(packet = peer->Receive(); packet; peer->DeallocatePacket(packet), packet = peer->Receive())
	{
		BitStream& stream = StreamStart(packet, Stream_UpdateGameServer);
		BitStreamReader reader(stream);
		int player_index = FindPlayerIndex(packet->systemAddress);
		if(player_index == -1)
		{
		ignore_him:
			Info("Ignoring packet from %s.", packet->systemAddress.ToString());
			StreamEnd();
			continue;
		}

		PlayerInfo& info = *game_players[player_index];
		if(info.left)
			goto ignore_him;

		byte msg_id;
		reader >> msg_id;

		switch(msg_id)
		{
		case ID_CONNECTION_LOST:
		case ID_DISCONNECTION_NOTIFICATION:
			Info(msg_id == ID_CONNECTION_LOST ? "Lost connection with player %s." : "Player %s has disconnected.", info.name.c_str());
			--players;
			players_left = true;
			info.left = (msg_id == ID_CONNECTION_LOST ? PlayerInfo::LEFT_DISCONNECTED : PlayerInfo::LEFT_QUIT);
			break;
		case ID_SAY:
			Server_Say(stream, info, packet);
			break;
		case ID_WHISPER:
			Server_Whisper(reader, info, packet);
			break;
		case ID_CONTROL:
			if(!ProcessControlMessageServer(reader, info))
			{
				peer->DeallocatePacket(packet);
				return;
			}
			break;
		default:
			Warn("UpdateServer: Unknown packet from %s: %u.", info.name.c_str(), msg_id);
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
			Net::PushChange(NetChange::CHANGE_FLAGS);

		update_timer = 0;
		net_stream.Reset();
		BitStreamWriter f(net_stream);
		f << ID_CHANGES;

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

		// wy�lij odkryte kawa�ki minimapy
		if(!minimap_reveal_mp.empty())
		{
			if(game_state == GS_LEVEL)
				Net::PushChange(NetChange::REVEAL_MINIMAP);
			else
				minimap_reveal_mp.clear();
		}

		// changes
		WriteServerChanges(f);
		Net::changes.clear();
		assert(net_talk.empty());
		peer->Send(&net_stream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
		StreamWrite(net_stream, Stream_UpdateGameServer, UNASSIGNED_SYSTEM_ADDRESS);

		for(auto pinfo : game_players)
		{
			auto& info = *pinfo;
			if(info.id == my_id || info.left)
				continue;

			// update stats
			if(info.u->player->stat_flags != 0)
			{
				NetChangePlayer& c = Add1(info.changes);
				c.id = info.u->player->stat_flags;
				c.type = NetChangePlayer::PLAYER_STATS;
				info.u->player->stat_flags = 0;
			}

			// update bufs
			int buffs = info.u->GetBuffs();
			if(buffs != info.buffs)
			{
				info.buffs = buffs;
				info.update_flags |= PlayerInfo::UF_BUFFS;
			}

			// write & send updates
			if(!info.changes.empty() || info.update_flags)
			{
				net_stream.Reset();
				BitStreamWriter f(net_stream);
				WriteServerChangesForPlayer(f, info);
				peer->Send(&net_stream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, info.adr, false);
				StreamWrite(net_stream, Stream_UpdateGameServer, info.adr);
				info.update_flags = 0;
				info.changes.clear();
			}
		}
	}
}

//=================================================================================================
bool Game::ProcessControlMessageServer(BitStreamReader& f, PlayerInfo& info)
{
	bool move_info;
	f >> move_info;
	if(!f)
	{
		StreamError("UpdateServer: Broken packet ID_CONTROL from %s.", info.name.c_str());
		return true;
	}

	Unit& unit = *info.u;
	PlayerController& player = *info.u->player;

	// movement/animation info
	if(move_info)
	{
		if(!info.warping && game_state == GS_LEVEL)
		{
			Vec3 new_pos;
			float rot;

			f >> new_pos;
			f >> rot;
			f >> unit.mesh_inst->groups[0].speed;
			if(!f)
			{
				StreamError("UpdateServer: Broken packet ID_CONTROL(2) from %s.", info.name.c_str());
				return true;
			}

			if(Vec3::Distance(unit.pos, new_pos) >= 10.f)
			{
				// too big change in distance, warp unit to old position
				Warn("UpdateServer: Invalid unit movement from %s ((%g,%g,%g) -> (%g,%g,%g)).", info.name.c_str(), unit.pos.x, unit.pos.y, unit.pos.z,
					new_pos.x, new_pos.y, new_pos.z);
				WarpUnit(unit, unit.pos);
				unit.interp->Add(unit.pos, rot);
			}
			else
			{
				if(player.noclip || unit.usable || CheckMoveNet(unit, new_pos))
				{
					// update position
					if(!unit.pos.Equal(new_pos) && !location->outside)
					{
						// reveal minimap
						Int2 new_tile(int(new_pos.x / 2), int(new_pos.z / 2));
						if(Int2(int(unit.pos.x / 2), int(unit.pos.z / 2)) != new_tile)
							DungeonReveal(new_tile);
					}
					unit.pos = new_pos;
					UpdateUnitPhysics(unit, unit.pos);
					unit.interp->Add(unit.pos, rot);
					unit.changed = true;
				}
				else
				{
					// player is now stuck inside something, unstuck him
					unit.interp->Add(unit.pos, rot);
					NetChangePlayer& c = Add1(info.changes);
					c.type = NetChangePlayer::UNSTUCK;
					c.pos = unit.pos;
				}
			}
		}
		else
		{
			// player is warping or not in level, skip movement
			f.Skip(sizeof(Vec3) + sizeof(float) * 2);
			if(!f)
			{
				StreamError("UpdateServer: Broken packet ID_CONTROL(3) from %s.", info.name.c_str());
				return true;
			}
		}

		// animation
		Animation ani;
		f.ReadCasted<byte>(ani);
		if(!f)
		{
			StreamError("UpdateServer: Broken packet ID_CONTROL(4) from %s.", info.name.c_str());
			return true;
		}
		if(unit.animation != ANI_PLAY && ani != ANI_PLAY)
			unit.animation = ani;
	}

	// count of changes
	byte changes;
	f >> changes;
	if(!f)
	{
		StreamError("UpdateServer: Broken packet ID_CONTROL(5) from %s.", info.name.c_str());
		return true;
	}

	// process changes
	for(byte change_i = 0; change_i < changes; ++change_i)
	{
		// change type
		NetChange::TYPE type;
		f.ReadCasted<byte>(type);
		if(!f)
		{
			StreamError("UpdateServer: Broken packet ID_CONTROL(6) from %s.", info.name.c_str());
			return true;
		}

		switch(type)
		{
		// player change equipped items
		case NetChange::CHANGE_EQUIPMENT:
			{
				int i_index;
				f >> i_index;
				if(!f)
					StreamError("Update server: Broken CHANGE_EQUIPMENT from %s.", info.name.c_str());
				else if(i_index >= 0)
				{
					// equipping item
					if(uint(i_index) >= unit.items.size())
					{
						StreamError("Update server: CHANGE_EQUIPMENT from %s, invalid index %d.", info.name.c_str(), i_index);
						break;
					}

					ItemSlot& slot = unit.items[i_index];
					if(!slot.item->IsWearableByHuman())
					{
						StreamError("Update server: CHANGE_EQUIPMENT from %s, item at index %d (%s) is not wearable.",
							info.name.c_str(), i_index, slot.item->id.c_str());
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
						unit.items.erase(unit.items.begin() + i_index);
					}

					// send to other players
					if(players > 2)
					{
						NetChange& c = Add1(Net::changes);
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
						StreamError("Update server: CHANGE_EQUIPMENT from %s, invalid slot type %d.", info.name.c_str(), slot);
					else if(!unit.slots[slot])
						StreamError("Update server: CHANGE_EQUIPMENT from %s, empty slot type %d.", info.name.c_str(), slot);
					else
					{
						unit.AddItem(unit.slots[slot], 1u, false);
						unit.weight -= unit.slots[slot]->weight;
						unit.slots[slot] = nullptr;

						// send to other players
						if(players > 2)
						{
							NetChange& c = Add1(Net::changes);
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
				f >> hide;
				f.ReadCasted<byte>(weapon_type);
				if(!f)
				{
					StreamError("Update server: Broken TAKE_WEAPON from %s.", info.name.c_str());
				}
				else
				{
					unit.mesh_inst->groups[1].speed = 1.f;
					SetUnitWeaponState(unit, !hide, weapon_type);
					// send to other players
					if(players > 2)
					{
						NetChange& c = Add1(Net::changes);
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
				f >> typeflags;
				f >> attack_speed;
				if(!f)
				{
					StreamError("Update server: Broken ATTACK from %s.", info.name.c_str());
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
							unit.attack_power = unit.mesh_inst->groups[1].time / unit.GetAttackFrame(0);
							unit.animation_state = 1;
							unit.mesh_inst->groups[1].speed = unit.attack_power + unit.GetAttackSpeed();
							unit.attack_power += 1.f;
						}
						else
						{
							if(unit.data->sounds->sound[SOUND_ATTACK] && Rand() % 4 == 0)
								PlayAttachedSound(unit, unit.data->sounds->sound[SOUND_ATTACK]->sound, 1.f, 10.f);
							unit.action = A_ATTACK;
							unit.attack_id = ((typeflags & 0xF0) >> 4);
							unit.attack_power = 1.f;
							unit.mesh_inst->Play(NAMES::ani_attacks[unit.attack_id], PLAY_PRIO1 | PLAY_ONCE | PLAY_RESTORE, 1);
							unit.mesh_inst->groups[1].speed = attack_speed;
							unit.animation_state = 1;
							unit.hitted = false;
						}
						unit.player->Train(TrainWhat::AttackStart, 0.f, 0);
						break;
					case AID_PrepareAttack:
						{
							if(unit.data->sounds->sound[SOUND_ATTACK] && Rand() % 4 == 0)
								PlayAttachedSound(unit, unit.data->sounds->sound[SOUND_ATTACK]->sound, 1.f, 10.f);
							unit.action = A_ATTACK;
							unit.attack_id = ((typeflags & 0xF0) >> 4);
							unit.attack_power = 1.f;
							unit.mesh_inst->Play(NAMES::ani_attacks[unit.attack_id], PLAY_PRIO1 | PLAY_ONCE | PLAY_RESTORE, 1);
							unit.mesh_inst->groups[1].speed = attack_speed;
							unit.animation_state = 0;
							unit.hitted = false;
							unit.RemoveStamina(unit.GetWeapon().GetInfo().stamina);
							unit.timer = 0.f;
						}
						break;
					case AID_Shoot:
					case AID_StartShoot:
						if(unit.action == A_SHOOT && unit.animation_state == 0)
							unit.animation_state = 1;
						else
						{
							unit.mesh_inst->Play(NAMES::ani_shoot, PLAY_PRIO1 | PLAY_ONCE | PLAY_RESTORE, 1);
							unit.mesh_inst->groups[1].speed = attack_speed;
							unit.action = A_SHOOT;
							unit.animation_state = (type == AID_Shoot ? 1 : 0);
							unit.hitted = false;
							if(!unit.bow_instance)
								unit.bow_instance = GetBowInstance(unit.GetBow().mesh);
							unit.bow_instance->Play(&unit.bow_instance->mesh->anims[0], PLAY_ONCE | PLAY_PRIO1 | PLAY_NO_BLEND, 0);
							unit.bow_instance->groups[0].speed = unit.mesh_inst->groups[1].speed;
							unit.RemoveStamina(Unit::STAMINA_BOW_ATTACK);
						}
						if(type == AID_Shoot)
						{
							f >> info.yspeed;
							if(!f)
								StreamError("Update server: Broken ATTACK(2) from %s.", info.name.c_str());
						}
						break;
					case AID_Block:
						{
							unit.action = A_BLOCK;
							unit.mesh_inst->Play(NAMES::ani_block, PLAY_PRIO1 | PLAY_STOP_AT_END | PLAY_RESTORE, 1);
							unit.mesh_inst->groups[1].speed = 1.f;
							unit.mesh_inst->groups[1].blend_max = attack_speed;
							unit.animation_state = 0;
						}
						break;
					case AID_Bash:
						{
							unit.action = A_BASH;
							unit.animation_state = 0;
							unit.mesh_inst->Play(NAMES::ani_bash, PLAY_ONCE | PLAY_PRIO1 | PLAY_RESTORE, 1);
							unit.mesh_inst->groups[1].speed = 2.f;
							unit.mesh_inst->frame_end_info2 = false;
							unit.hitted = false;
							unit.player->Train(TrainWhat::BashStart, 0.f, 0);
							unit.RemoveStamina(Unit::STAMINA_BASH_ATTACK);
						}
						break;
					case AID_RunningAttack:
						{
							if(unit.data->sounds->sound[SOUND_ATTACK] && Rand() % 4 == 0)
								PlayAttachedSound(unit, unit.data->sounds->sound[SOUND_ATTACK]->sound, 1.f, 10.f);
							unit.action = A_ATTACK;
							unit.attack_id = ((typeflags & 0xF0) >> 4);
							unit.attack_power = 1.5f;
							unit.run_attack = true;
							unit.mesh_inst->Play(NAMES::ani_attacks[unit.attack_id], PLAY_PRIO1 | PLAY_ONCE | PLAY_RESTORE, 1);
							unit.mesh_inst->groups[1].speed = attack_speed;
							unit.animation_state = 1;
							unit.hitted = false;
							unit.RemoveStamina(unit.GetWeapon().GetInfo().stamina * 1.5f);
						}
						break;
					case AID_StopBlock:
						{
							unit.action = A_NONE;
							unit.mesh_inst->frame_end_info2 = false;
							unit.mesh_inst->Deactivate(1);
						}
						break;
					}

					// send to other players
					if(players > 2)
					{
						NetChange& c = Add1(Net::changes);
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
				f >> i_index;
				f >> count;
				if(!f)
					StreamError("Update server: Broken DROP_ITEM from %s.", info.name.c_str());
				else if(count == 0)
					StreamError("Update server: DROP_ITEM from %s, count %d.", info.name.c_str(), count);
				else
				{
					GroundItem* item;

					if(i_index >= 0)
					{
						// dropping unequipped item
						if(i_index >= (int)unit.items.size())
						{
							StreamError("Update server: DROP_ITEM from %s, invalid index %d (count %d).", info.name.c_str(), i_index, count);
							break;
						}

						ItemSlot& sl = unit.items[i_index];
						if(count > (int)sl.count)
						{
							StreamError("Update server: DROP_ITEM from %s, index %d (count %d) have only %d count.", info.name.c_str(), i_index,
								count, sl.count);
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
							unit.items.erase(unit.items.begin() + i_index);
					}
					else
					{
						// dropping equipped item
						ITEM_SLOT slot_type = IIndexToSlot(i_index);
						if(!IsValid(slot_type))
						{
							StreamError("Update server: DROP_ITEM from %s, invalid slot %d.", info.name.c_str(), slot_type);
							break;
						}

						const Item*& slot = unit.slots[slot_type];
						if(!slot)
						{
							StreamError("Update server: DROP_ITEM from %s, empty slot %d.", info.name.c_str(), slot_type);
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
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::CHANGE_EQUIPMENT;
							c.unit = &unit;
							c.id = slot_type;
						}
					}

					unit.action = A_ANIMATION;
					unit.mesh_inst->Play("wyrzuca", PLAY_ONCE | PLAY_PRIO2, 0);
					unit.mesh_inst->groups[0].speed = 1.f;
					unit.mesh_inst->frame_end_info = false;
					item->pos = unit.pos;
					item->pos.x -= sin(unit.rot)*0.25f;
					item->pos.z -= cos(unit.rot)*0.25f;
					item->rot = Random(MAX_ANGLE);
					if(!CheckMoonStone(item, unit))
						AddGroundItem(GetContext(unit), item);

					// send to other players
					if(players > 2)
					{
						NetChange& c = Add1(Net::changes);
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
				f >> netid;
				if(!f)
				{
					StreamError("Update server: Broken PICKUP_ITEM from %s.", info.name.c_str());
					break;
				}

				LevelContext* ctx;
				GroundItem* item = FindItemNetid(netid, &ctx);
				if(!item)
				{
					StreamError("Update server: PICKUP_ITEM from %s, missing item %d.", info.name.c_str(), netid);
					break;
				}

				// add item
				unit.AddItem(item->item, item->count, item->team_count);

				// start animation
				bool up_animation = (item->pos.y > unit.pos.y + 0.5f);
				unit.action = A_PICKUP;
				unit.animation = ANI_PLAY;
				unit.mesh_inst->Play(up_animation ? "podnosi_gora" : "podnosi", PLAY_ONCE | PLAY_PRIO2, 0);
				unit.mesh_inst->groups[0].speed = 1.f;
				unit.mesh_inst->frame_end_info = false;

				// send pickup acceptation
				NetChangePlayer& c = Add1(info.changes);
				c.type = NetChangePlayer::PICKUP;
				c.id = item->count;
				c.ile = item->team_count;

				// send remove item to all players
				NetChange& c2 = Add1(Net::changes);
				c2.type = NetChange::REMOVE_ITEM;
				c2.id = item->netid;

				// send info to other players about picking item
				if(players > 2)
				{
					NetChange& c3 = Add1(Net::changes);
					c3.type = NetChange::PICKUP_ITEM;
					c3.unit = &unit;
					c3.ile = (up_animation ? 1 : 0);
				}

				// remove item
				if(pc_data.before_player == BP_ITEM && pc_data.before_player_ptr.item == item)
					pc_data.before_player = BP_NONE;
				DeleteElement(*ctx->items, item);
			}
			break;
		// player consume item
		case NetChange::CONSUME_ITEM:
			{
				int index;
				f >> index;
				if(!f)
					StreamError("Update server: Broken CONSUME_ITEM from %s.", info.name.c_str());
				else
					unit.ConsumeItem(index);
			}
			break;
		// player wants to loot unit
		case NetChange::LOOT_UNIT:
			{
				int netid;
				f >> netid;
				if(!f)
				{
					StreamError("Update server: Broken LOOT_UNIT from %s.", info.name.c_str());
					break;
				}

				Unit* looted_unit = FindUnit(netid);
				if(!looted_unit)
				{
					StreamError("Update server: LOOT_UNIT from %s, missing unit %d.", info.name.c_str(), netid);
					break;
				}

				NetChangePlayer& c = Add1(info.changes);
				c.type = NetChangePlayer::LOOT;
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
			}
			break;
		// player wants to loot chest
		case NetChange::LOOT_CHEST:
			{
				int netid;
				f >> netid;
				if(!f)
				{
					StreamError("Update server: Broken LOOT_CHEST from %s.", info.name.c_str());
					break;
				}

				Chest* chest = FindChest(netid);
				if(!chest)
				{
					StreamError("Update server: LOOT_CHEST from %s, missing chest %d.", info.name.c_str(), netid);
					break;
				}

				NetChangePlayer& c = Add1(info.changes);
				c.type = NetChangePlayer::LOOT;
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
					NetChange& c2 = Add1(Net::changes);
					c2.type = NetChange::CHEST_OPEN;
					c2.id = chest->netid;

					// animation / sound
					chest->mesh_inst->Play(&chest->mesh_inst->mesh->anims[0], PLAY_PRIO1 | PLAY_ONCE | PLAY_STOP_AT_END, 0);
					sound_mgr->PlaySound3d(sChestOpen, chest->GetCenter(), 2.f, 5.f);

					// event handler
					if(chest->handler)
						chest->handler->HandleChestEvent(ChestEventHandler::Opened);
				}
			}
			break;
		// player gets item from unit or container
		case NetChange::GET_ITEM:
			{
				int i_index, count;
				f >> i_index;
				f >> count;
				if(!f)
				{
					StreamError("Update server: Broken GET_ITEM from %s.", info.name.c_str());
					break;
				}

				if(!player.IsTrading())
				{
					StreamError("Update server: GET_ITEM, player %s is not trading.", info.name.c_str());
					break;
				}

				if(i_index >= 0)
				{
					// getting not equipped item
					if(i_index >= (int)player.chest_trade->size())
					{
						StreamError("Update server: GET_ITEM from %s, invalid index %d.", info.name.c_str(), i_index);
						break;
					}

					ItemSlot& slot = player.chest_trade->at(i_index);
					if(count < 1 || count >(int)slot.count)
					{
						StreamError("Update server: GET_ITEM from %s, invalid item count %d (have %d).", info.name.c_str(), count, slot.count);
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
							player.chest_trade->erase(player.chest_trade->begin() + i_index);
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
						else if(player.action == PlayerController::Action_ShareItems && slot.item->type == IT_CONSUMABLE
							&& slot.item->ToConsumable().effect == E_HEAL)
							player.action_unit->ai->have_potion = 1;
						if(player.action != PlayerController::Action_LootChest && player.action != PlayerController::Action_LootContainer)
						{
							player.action_unit->weight -= slot.item->weight*count;
							if(player.action == PlayerController::Action_LootUnit && slot.item == player.action_unit->used_item)
							{
								player.action_unit->used_item = nullptr;
								// removed item from hand, send info to other players
								if(players > 2)
								{
									NetChange& c = Add1(Net::changes);
									c.type = NetChange::REMOVE_USED_ITEM;
									c.unit = player.action_unit;
								}
							}
						}
						slot.count -= count;
						if(slot.count == 0)
							player.chest_trade->erase(player.chest_trade->begin() + i_index);
						else
							slot.team_count -= team_count;
					}
				}
				else
				{
					// getting equipped item
					ITEM_SLOT type = IIndexToSlot(i_index);
					if(player.action == PlayerController::Action_LootChest || player.action == PlayerController::Action_LootContainer
						|| type < SLOT_WEAPON || type >= SLOT_MAX || !player.action_unit->slots[type])
					{
						StreamError("Update server: GET_ITEM from %s, invalid or empty slot %d.", info.name.c_str(), type);
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
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::REMOVE_USED_ITEM;
							c.unit = player.action_unit;
						}
					}
					player.action_unit->weight -= slot->weight;
					slot = nullptr;

					// send info about changing equipment of looted unit
					if(players > 2)
					{
						NetChange& c = Add1(Net::changes);
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
				f >> i_index;
				f >> count;
				if(!f)
				{
					StreamError("Update server: Broken PUT_ITEM from %s.", info.name.c_str());
					break;
				}

				if(!player.IsTrading())
				{
					StreamError("Update server: PUT_ITEM, player %s is not trading.", info.name.c_str());
					break;
				}

				if(i_index >= 0)
				{
					// put not equipped item
					if(i_index >= (int)unit.items.size())
					{
						StreamError("Update server: PUT_ITEM from %s, invalid index %d.", info.name.c_str(), i_index);
						break;
					}

					ItemSlot& slot = unit.items[i_index];
					if(count < 1 || count > slot.count)
					{
						StreamError("Update server: PUT_ITEM from %s, invalid count %u (have %u).", info.name.c_str(), count, slot.count);
						break;
					}

					uint team_count = min(count, slot.team_count);

					// add item
					if(player.action == PlayerController::Action_LootChest)
						AddItem(*player.action_chest, slot.item, count, team_count, false);
					else if(player.action == PlayerController::Action_LootContainer)
						player.action_container->container->AddItem(slot.item, count, team_count);
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
							if(slot.item->type == IT_CONSUMABLE && slot.item->ToConsumable().effect == E_HEAL)
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
							if(slot.item->type == IT_CONSUMABLE && slot.item->ToConsumable().effect == E_HEAL)
								t->ai->have_potion = 2;
						}
						AddItem(*t, slot.item, count, add_as_team, false);
						if(player.action == PlayerController::Action_ShareItems || player.action == PlayerController::Action_GiveItems)
						{
							if(slot.item->type == IT_CONSUMABLE && t->ai->have_potion == 0)
								t->ai->have_potion = 1;
							if(player.action == PlayerController::Action_GiveItems)
							{
								UpdateUnitInventory(*t);
								NetChangePlayer& c = Add1(info.changes);
								c.type = NetChangePlayer::UPDATE_TRADER_INVENTORY;
								c.unit = t;
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
						StreamError("Update server: PUT_ITEM from %s, invalid or empty slot %d.", info.name.c_str(), type);
						break;
					}

					const Item*& slot = unit.slots[type];
					int price = GetItemPrice(slot, unit, false);
					// add new item
					if(player.action == PlayerController::Action_LootChest)
						AddItem(*player.action_chest, slot, 1u, 0u, false);
					else if(player.action == PlayerController::Action_LootContainer)
						player.action_container->container->AddItem(slot, 1u, 0u);
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
							NetChangePlayer& c = Add1(info.changes);
							c.type = NetChangePlayer::UPDATE_TRADER_INVENTORY;
							c.unit = player.action_unit;
						}
					}
					// remove equipped
					unit.weight -= slot->weight;
					slot = nullptr;
					// send info about changing equipment
					if(players > 2)
					{
						NetChange& c = Add1(Net::changes);
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
					StreamError("Update server: GET_ALL_ITEM, player %s is not trading.", info.name.c_str());
					break;
				}

				bool changes = false;

				// slots
				if(player.action != PlayerController::Action_LootChest && player.action != PlayerController::Action_LootContainer)
				{
					const Item** slots = player.action_unit->slots;
					for(int i = 0; i < SLOT_MAX; ++i)
					{
						if(slots[i])
						{
							InsertItemBare(unit.items, slots[i]);
							slots[i] = nullptr;
							changes = true;

							// send info about changing equipment
							if(players > 2)
							{
								NetChange& c = Add1(Net::changes);
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
				StreamError("Update server: STOP_TRADE, player %s is not trading.", info.name.c_str());
				break;
			}

			if(player.action == PlayerController::Action_LootChest)
			{
				player.action_chest->looted = false;
				player.action_chest->mesh_inst->Play(&player.action_chest->mesh_inst->mesh->anims[0], PLAY_PRIO1 | PLAY_ONCE | PLAY_STOP_AT_END | PLAY_BACK, 0);
				sound_mgr->PlaySound3d(sChestClose, player.action_chest->GetCenter(), 2.f, 5.f);
				NetChange& c = Add1(Net::changes);
				c.type = NetChange::CHEST_CLOSE;
				c.id = player.action_chest->netid;
			}
			else if(player.action == PlayerController::Action_LootContainer)
			{
				player.unit->UseUsable(nullptr);

				NetChange& c = Add1(Net::changes);
				c.type = NetChange::USE_USABLE;
				c.unit = info.u;
				c.id = player.action_container->netid;
				c.ile = USE_USABLE_END;
			}
			else
			{
				player.action_unit->busy = Unit::Busy_No;
				player.action_unit->look_target = nullptr;
			}
			player.action = PlayerController::Action_None;
			break;
		// player takes item for credit
		case NetChange::TAKE_ITEM_CREDIT:
			{
				int index;
				f >> index;
				if(!f)
				{
					StreamError("Update server: Broken TAKE_ITEM_CREDIT from %s.", info.name.c_str());
					break;
				}

				if(index < 0 || index >= (int)unit.items.size())
				{
					StreamError("Update server: TAKE_ITEM_CREDIT from %s, invalid index %d.", info.name.c_str(), index);
					break;
				}

				ItemSlot& slot = unit.items[index];
				if(slot.item->IsWearableByHuman() && slot.team_count != 0)
				{
					slot.team_count = 0;
					player.credit += slot.item->value / 2;
					CheckCredit(true);
				}
				else
				{
					StreamError("Update server: TAKE_ITEM_CREDIT from %s, item %s (count %u, team count %u).", info.name.c_str(), slot.item->id.c_str(),
						slot.count, slot.team_count);
				}
			}
			break;
		// unit plays idle animation
		case NetChange::IDLE:
			{
				byte index;
				f >> index;
				if(!f)
					StreamError("Update server: Broken IDLE from %s.", info.name.c_str());
				else if(index >= unit.data->idles->anims.size())
					StreamError("Update server: IDLE from %s, invalid animation index %u.", info.name.c_str(), index);
				else
				{
					unit.mesh_inst->Play(unit.data->idles->anims[index].c_str(), PLAY_ONCE, 0);
					unit.mesh_inst->groups[0].speed = 1.f;
					unit.mesh_inst->frame_end_info = false;
					unit.animation = ANI_IDLE;
					// send info to other players
					if(players > 2)
					{
						NetChange& c = Add1(Net::changes);
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
				f >> netid;
				if(!f)
				{
					StreamError("Update server: Broken TALK from %s.", info.name.c_str());
					break;
				}

				Unit* talk_to = FindUnit(netid);
				if(!talk_to)
				{
					StreamError("Update server: TALK from %s, missing unit %d.", info.name.c_str(), netid);
					break;
				}

				NetChangePlayer& c = Add1(info.changes);
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
					StartDialog(*player.dialog_ctx, talk_to);
				}
			}
			break;
		// player selected dialog choice
		case NetChange::CHOICE:
			{
				byte choice;
				f >> choice;
				if(!f)
					StreamError("Update server: Broken CHOICE from %s.", info.name.c_str());
				else if(player.dialog_ctx->show_choices && choice < player.dialog_ctx->choices.size())
					player.dialog_ctx->choice_selected = choice;
				else
					StreamError("Update server: CHOICE from %s, not in dialog or invalid choice %u.", info.name.c_str(), choice);
			}
			break;
		// pomijanie dialogu przez gracza
		case NetChange::SKIP_DIALOG:
			{
				int skip_id;
				f >> skip_id;
				if(!f)
					StreamError("Update server: Broken SKIP_DIALOG from %s.", info.name.c_str());
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
				f >> building_index;
				if(!f)
					StreamError("Update server: Broken ENTER_BUILDING from %s.", info.name.c_str());
				else if(city_ctx && building_index < city_ctx->inside_buildings.size())
				{
					WarpData& warp = Add1(mp_warps);
					warp.u = &unit;
					warp.where = building_index;
					warp.timer = 1.f;
					unit.frozen = FROZEN::YES;
				}
				else
					StreamError("Update server: ENTER_BUILDING from %s, invalid building index %u.", info.name.c_str(), building_index);
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
				unit.frozen = FROZEN::YES;
			}
			else
				StreamError("Update server: EXIT_BUILDING from %s, unit not in building.", info.name.c_str());
			break;
		// notify about warping
		case NetChange::WARP:
			info.warping = false;
			break;
		// player added note to journal
		case NetChange::ADD_NOTE:
			{
				string& str = Add1(info.notes);
				f.ReadString1(str);
				if(!f)
				{
					info.notes.pop_back();
					StreamError("Update server: Broken ADD_NOTE from %s.", info.name.c_str());
				}
			}
			break;
		// get Random number for player
		case NetChange::RANDOM_NUMBER:
			{
				byte number;
				f >> number;
				if(!f)
					StreamError("Update server: Broken RANDOM_NUMBER from %s.", info.name.c_str());
				else
				{
					AddMsg(Format(txRolledNumber, info.name.c_str(), number));
					// send to other players
					if(players > 2)
					{
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::RANDOM_NUMBER;
						c.unit = info.u;
						c.id = number;
					}
				}
			}
			break;
		// player wants to start/stop using usable
		case NetChange::USE_USABLE:
			{
				int usable_netid;
				USE_USABLE_STATE state;
				f >> usable_netid;
				f.ReadCasted<byte>(state);
				if(!f)
				{
					StreamError("Update server: Broken USE_USABLE from %s.", info.name.c_str());
					break;
				}

				Usable* usable = FindUsable(usable_netid);
				if(!usable)
				{
					StreamError("Update server: USE_USABLE from %s, missing usable %d.", info.name.c_str(), usable_netid);
					break;
				}

				if(state == USE_USABLE_START)
				{
					// use usable
					if(usable->user)
					{
						// someone else is already using this
						NetChangePlayer& c = Add1(info.changes);
						c.type = NetChangePlayer::USE_USABLE;
						break;
					}
					else
					{
						BaseUsable& base = *usable->base;
						if(!IS_SET(base.use_flags, BaseUsable::CONTAINER))
						{
							unit.action = A_ANIMATION2;
							unit.animation = ANI_PLAY;
							unit.mesh_inst->Play(base.anim.c_str(), PLAY_PRIO1, 0);
							unit.mesh_inst->groups[0].speed = 1.f;
							unit.target_pos = unit.pos;
							unit.target_pos2 = usable->pos;
							if(usable->base->limit_rot == 4)
								unit.target_pos2 -= Vec3(sin(usable->rot)*1.5f, 0, cos(usable->rot)*1.5f);
							unit.timer = 0.f;
							unit.animation_state = AS_ANIMATION2_MOVE_TO_OBJECT;
							unit.use_rot = Vec3::LookAtAngle(unit.pos, usable->pos);
							unit.used_item = base.item;
							if(unit.used_item)
							{
								unit.weapon_taken = W_NONE;
								unit.weapon_state = WS_HIDDEN;
							}
						}
						else
						{
							// start looting container
							NetChangePlayer& c = Add1(info.changes);
							c.type = NetChangePlayer::LOOT;
							c.id = 1;

							player.action = PlayerController::Action_LootContainer;
							player.action_container = usable;
							player.chest_trade = &usable->container->items;
						}

						unit.UseUsable(usable);
					}
				}
				else
				{
					// stop using usable
					if(usable->user == &unit)
					{
						if(state == USE_USABLE_STOP)
						{
							BaseUsable& base = *usable->base;
							if(!IS_SET(base.use_flags, BaseUsable::CONTAINER))
							{
								unit.action = A_NONE;
								unit.animation = ANI_STAND;
								if(unit.live_state == Unit::ALIVE)
									unit.used_item = nullptr;
							}
						}
						else if(state == USE_USABLE_END)
							unit.UseUsable(nullptr);
					}
				}

				// send info to players
				NetChange& c = Add1(Net::changes);
				c.type = NetChange::USE_USABLE;
				c.unit = info.u;
				c.id = usable_netid;
				c.ile = state;
			}
			break;
		// player used cheat 'suicide'
		case NetChange::CHEAT_SUICIDE:
			if(info.devmode)
				GiveDmg(GetContext(unit), nullptr, unit.hpmax, unit);
			else
				StreamError("Update server: Player %s used CHEAT_SUICIDE without devmode.", info.name.c_str());
			break;
		// player used cheat 'godmode'
		case NetChange::CHEAT_GODMODE:
			{
				bool state;
				f >> state;
				if(!f)
					StreamError("Update server: Broken CHEAT_GODMODE from %s.", info.name.c_str());
				else if(info.devmode)
					player.godmode = state;
				else
					StreamError("Update server: Player %s used CHEAT_GODMODE without devmode.", info.name.c_str());
			}
			break;
		// player stands up
		case NetChange::STAND_UP:
			UnitStandup(unit);
			// send to other players
			if(players > 2)
			{
				NetChange& c = Add1(Net::changes);
				c.type = NetChange::STAND_UP;
				c.unit = &unit;
			}
			break;
		// player used cheat 'noclip'
		case NetChange::CHEAT_NOCLIP:
			{
				bool state;
				f >> state;
				if(!f)
					StreamError("Update server: Broken CHEAT_NOCLIP from %s.", info.name.c_str());
				else if(info.devmode)
					player.noclip = state;
				else
					StreamError("Update server: Player %s used CHEAT_NOCLIP without devmode.", info.name.c_str());
			}
			break;
		// player used cheat 'invisible'
		case NetChange::CHEAT_INVISIBLE:
			{
				bool state;
				f >> state;
				if(!f)
					StreamError("Update server: Broken CHEAT_INVISIBLE from %s.", info.name.c_str());
				else if(info.devmode)
					unit.invisible = state;
				else
					StreamError("Update server: Player %s used CHEAT_INVISIBLE without devmode.", info.name.c_str());
			}
			break;
		// player used cheat 'scare'
		case NetChange::CHEAT_SCARE:
			if(info.devmode)
			{
				for(AIController* ai : ais)
				{
					if(IsEnemy(*ai->unit, unit) && Vec3::Distance(ai->unit->pos, unit.pos) < ALERT_RANGE.x && CanSee(*ai->unit, unit))
					{
						ai->morale = -10;
						ai->target_last_pos = unit.pos;
					}
				}
			}
			else
				StreamError("Update server: Player %s used CHEAT_SCARE without devmode.", info.name.c_str());
			break;
		// player used cheat 'killall'
		case NetChange::CHEAT_KILLALL:
			{
				int ignored_netid;
				byte type;
				f >> ignored_netid;
				f >> type;
				if(!f)
				{
					StreamError("Update server: Broken CHEAT_KILLALL from %s.", info.name.c_str());
					break;
				}

				if(!info.devmode)
				{
					StreamError("Update server: Player %s used CHEAT_KILLALL without devmode.", info.name.c_str());
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
						StreamError("Update server: CHEAT_KILLALL from %s, missing unit %d.", info.name.c_str(), ignored_netid);
						break;
					}
				}

				if(!Cheat_KillAll(type, unit, ignored))
					StreamError("Update server: CHEAT_KILLALL from %s, invalid type %u.", info.name.c_str(), type);
			}
			break;
		// client checks if item is better for npc
		case NetChange::IS_BETTER_ITEM:
			{
				int i_index;
				f >> i_index;
				if(!f)
				{
					StreamError("Update server: Broken IS_BETTER_ITEM from %s.", info.name.c_str());
					break;
				}

				NetChangePlayer& c = Add1(info.changes);
				c.type = NetChangePlayer::IS_BETTER_ITEM;
				c.id = 0;

				if(player.action == PlayerController::Action_GiveItems)
				{
					const Item* item = unit.GetIIndexItem(i_index);
					if(item)
					{
						if(IsBetterItem(*player.action_unit, item))
							c.id = 1;
					}
					else
						StreamError("Update server: IS_BETTER_ITEM from %s, invalid i_index %d.", info.name.c_str(), i_index);
				}
				else
					StreamError("Update server: IS_BETTER_ITEM from %s, player is not giving items.", info.name.c_str());
			}
			break;
		// player used cheat 'citizen'
		case NetChange::CHEAT_CITIZEN:
			if(info.devmode)
			{
				if(Team.is_bandit || Team.crazies_attack)
				{
					Team.is_bandit = false;
					Team.crazies_attack = false;
					Net::PushChange(NetChange::CHANGE_FLAGS);
				}
			}
			else
				StreamError("Update server: Player %s used CHEAT_CITIZEN without devmode.", info.name.c_str());
			break;
		// player used cheat 'heal'
		case NetChange::CHEAT_HEAL:
			if(info.devmode)
			{
				if(unit.hp != unit.hpmax)
				{
					unit.hp = unit.hpmax;
					NetChange& c = Add1(Net::changes);
					c.type = NetChange::UPDATE_HP;
					c.unit = &unit;
				}
				if(unit.stamina != unit.stamina_max)
				{
					unit.stamina = unit.stamina_max;
					info.update_flags |= PlayerInfo::UF_STAMINA;
				}
				unit.RemovePoison();
				unit.RemoveEffect(EffectId::Stun);
			}
			else
				StreamError("Update server: Player %s used CHEAT_HEAL without devmode.", info.name.c_str());
			break;
		// player used cheat 'kill'
		case NetChange::CHEAT_KILL:
			{
				int netid;
				f >> netid;
				if(!f)
					StreamError("Update server: Broken CHEAT_KILL from %s.", info.name.c_str());
				else if(!info.devmode)
					StreamError("Update server: Player %s used CHEAT_KILL without devmode.", info.name.c_str());
				else
				{
					Unit* target = FindUnit(netid);
					if(!target)
						StreamError("Update server: CHEAT_KILL from %s, missing unit %d.", info.name.c_str(), netid);
					else
						GiveDmg(GetContext(*target), nullptr, target->hpmax, *target);
				}
			}
			break;
		// player used cheat 'heal_unit'
		case NetChange::CHEAT_HEAL_UNIT:
			{
				int netid;
				f >> netid;
				if(!f)
					StreamError("Update server: Broken CHEAT_HEAL_UNIT from %s.", info.name.c_str());
				else if(!info.devmode)
					StreamError("Update server: Player %s used CHEAT_HEAL_UNIT without devmode.", info.name.c_str());
				else
				{
					Unit* target = FindUnit(netid);
					if(!target)
						StreamError("Update server: CHEAT_HEAL_UNIT from %s, missing unit %d.", info.name.c_str(), netid);
					else
					{
						if(target->hp != target->hpmax)
						{
							target->hp = target->hpmax;
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::UPDATE_HP;
							c.unit = target;
						}
						if(target->stamina != target->stamina_max)
						{
							target->stamina = target->stamina_max;
							if(target->player && !target->player->is_local)
								target->player->player_info->update_flags |= PlayerInfo::UF_STAMINA;
						}
						target->RemovePoison();
						target->RemoveEffect(EffectId::Stun);
					}
				}
			}
			break;
		// player used cheat 'reveal'
		case NetChange::CHEAT_REVEAL:
			if(info.devmode)
				Cheat_Reveal();
			else
				StreamError("Update server: Player %s used CHEAT_REVEAL without devmode.", info.name.c_str());
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
				StreamError("Update server: Player %s used CHEAT_GOTO_MAP without devmode.", info.name.c_str());
			break;
		// player used cheat 'show_minimap'
		case NetChange::CHEAT_SHOW_MINIMAP:
			if(info.devmode)
				Cheat_ShowMinimap();
			else
				StreamError("Update server: Player %s used CHEAT_SHOW_MINIMAP without devmode.", info.name.c_str());
			break;
		// player used cheat 'add_gold' or 'add_team_gold'
		case NetChange::CHEAT_ADD_GOLD:
			{
				bool is_team;
				int count;
				f >> is_team;
				f >> count;
				if(!f)
					StreamError("Update server: Broken CHEAT_ADD_GOLD from %s.", info.name.c_str());
				else if(!info.devmode)
					StreamError("Update server: Player %s used CHEAT_ADD_GOLD without devmode.", info.name.c_str());
				else
				{
					if(is_team)
					{
						if(count <= 0)
							StreamError("Update server: CHEAT_ADD_GOLD from %s, invalid count %d.", info.name.c_str(), count);
						else
							AddGold(count);
					}
					else
					{
						unit.gold = max(unit.gold + count, 0);
						info.UpdateGold();
					}
				}
			}
			break;
		// player used cheat 'add_item' or 'add_team_item'
		case NetChange::CHEAT_ADD_ITEM:
			{
				int count;
				bool is_team;
				const string& item_id = f.ReadString1();
				f >> count;
				f >> is_team;
				if(!f)
					StreamError("Update server: Broken CHEAT_ADD_ITEM from %s.", info.name.c_str());
				else if(!info.devmode)
					StreamError("Update server: Player %s used CHEAT_ADD_ITEM without devmode.", info.name.c_str());
				else
				{
					const Item* item = Item::TryGet(item_id);
					if(item && count)
					{
						PreloadItem(item);
						AddItem(*info.u, item, count, is_team);
					}
					else
						StreamError("Update server: CHEAT_ADD_ITEM from %s, missing item %s or invalid count %u.", info.name.c_str(), item_id.c_str(), count);
				}
			}
			break;
		// player used cheat 'skip_days'
		case NetChange::CHEAT_SKIP_DAYS:
			{
				int count;
				f >> count;
				if(!f)
					StreamError("Update server: Broken CHEAT_SKIP_DAYS from %s.", info.name.c_str());
				else if(!info.devmode)
					StreamError("Update server: Player %s used CHEAT_SKIP_DAYS without devmode.", info.name.c_str());
				else
					WorldProgress(count, WPM_SKIP);
			}
			break;
		// player used cheat 'warp'
		case NetChange::CHEAT_WARP:
			{
				byte building_index;
				f >> building_index;
				if(!f)
					StreamError("Update server: Broken CHEAT_WARP from %s.", info.name.c_str());
				else if(!info.devmode)
					StreamError("Update server: Player %s used CHEAT_WARP without devmode.", info.name.c_str());
				else if(unit.frozen != FROZEN::NO)
					StreamError("Update server: CHEAT_WARP from %s, unit is frozen.", info.name.c_str());
				else if(!city_ctx || building_index >= city_ctx->inside_buildings.size())
					StreamError("Update server: CHEAT_WARP from %s, invalid inside building index %u.", info.name.c_str(), building_index);
				else
				{
					WarpData& warp = Add1(mp_warps);
					warp.u = &unit;
					warp.where = building_index;
					warp.timer = 1.f;
					unit.frozen = (unit.usable ? FROZEN::YES_NO_ANIM : FROZEN::YES);
					Net_PrepareWarp(info.u->player);
				}
			}
			break;
		// player used cheat 'spawn_unit'
		case NetChange::CHEAT_SPAWN_UNIT:
			{
				byte count;
				char level, in_arena;
				const string& unit_id = f.ReadString1();
				f >> count;
				f >> level;
				f >> in_arena;
				if(!f)
					StreamError("Update server: Broken CHEAT_SPAWN_UNIT from %s.", info.name.c_str());
				else if(!info.devmode)
					StreamError("Update server: Player %s used CHEAT_SPAWN_UNIT without devmode.", info.name.c_str());
				else
				{
					UnitData* data = UnitData::TryGet(unit_id);
					if(!data)
						StreamError("Update server: CHEAT_SPAWN_UNIT from %s, invalid unit id %s.", info.name.c_str(), unit_id.c_str());
					else
					{
						if(in_arena < -1 || in_arena > 1)
							in_arena = -1;

						LevelContext& ctx = GetContext(*info.u);
						Vec3 pos = info.u->GetFrontPos();

						for(byte i = 0; i < count; ++i)
						{
							Unit* spawned = SpawnUnitNearLocation(ctx, pos, *data, &unit.pos, level);
							if(!spawned)
							{
								Warn("Update server: CHEAT_SPAWN_UNIT from %s, no free space for unit.", info.name.c_str());
								break;
							}
							else if(in_arena != -1)
							{
								spawned->in_arena = in_arena;
								at_arena.push_back(spawned);
							}
							if(Net::IsOnline())
								Net_SpawnUnit(spawned);
						}
					}
				}
			}
			break;
		// player used cheat 'set_stat' or 'mod_stat'
		case NetChange::CHEAT_SET_STAT:
		case NetChange::CHEAT_MOD_STAT:
			{
				cstring name = (type == NetChange::CHEAT_SET_STAT ? "CHEAT_SET_STAT" : "CHEAT_MOD_STAT");

				byte what;
				bool is_skill;
				char value;
				f >> what;
				f >> is_skill;
				f >> value;
				if(!f)
					StreamError("Update server: Broken %s from %s.", name, info.name.c_str());
				else if(!info.devmode)
					StreamError("Update server: Player %s used %s without devmode.", info.name.c_str(), name);
				else if(is_skill)
				{
					if(what < (int)SkillId::MAX)
					{
						int num = +value;
						if(type == NetChange::CHEAT_MOD_STAT)
							num += info.u->unmod_stats.skill[what];
						int v = Clamp(num, Skill::MIN, Skill::MAX);
						if(v != info.u->unmod_stats.skill[what])
						{
							info.u->Set((SkillId)what, v);
							NetChangePlayer& c = AddChange(NetChangePlayer::STAT_CHANGED, info.pc);
							c.id = (int)ChangedStatType::SKILL;
							c.a = what;
							c.ile = v;
						}
					}
					else
						StreamError("Update server: %s from %s, invalid skill %d.", name, info.name.c_str(), what);
				}
				else
				{
					if(what < (int)AttributeId::MAX)
					{
						int num = +value;
						if(type == NetChange::CHEAT_MOD_STAT)
							num += info.u->unmod_stats.attrib[what];
						int v = Clamp(num, Attribute::MIN, Attribute::MAX);
						if(v != info.u->unmod_stats.attrib[what])
						{
							info.u->Set((AttributeId)what, v);
							NetChangePlayer& c = AddChange(NetChangePlayer::STAT_CHANGED, info.pc);
							c.id = (int)ChangedStatType::ATTRIBUTE;
							c.a = what;
							c.ile = v;
						}
					}
					else
						StreamError("Update server: %s from %s, invalid attribute %d.", name, info.name.c_str(), what);
				}
			}
			break;
		// response to pvp request
		case NetChange::PVP:
			{
				bool accepted;
				f >> accepted;
				if(!f)
					StreamError("Update server: Broken PVP from %s.", info.name.c_str());
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
							NetChangePlayer& c = Add1(pvp_response.from->player->player_info->changes);
							c.type = NetChangePlayer::NO_PVP;
							c.id = pvp_response.to->player->id;
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
				f >> type;
				if(!f)
					StreamError("Update server: Broken LEAVE_LOCATION from %s.", info.name.c_str());
				else if(!Team.IsLeader(unit))
					StreamError("Update server: LEAVE_LOCATION from %s, player is not leader.", info.name.c_str());
				else
				{
					CanLeaveLocationResult result = CanLeaveLocation(*info.u);
					if(result == CanLeaveLocationResult::Yes)
					{
						Net::PushChange(NetChange::LEAVE_LOCATION);
						if(type == WHERE_OUTSIDE)
							fallback_co = FALLBACK::EXIT;
						else if(type == WHERE_LEVEL_UP)
						{
							fallback_co = FALLBACK::CHANGE_LEVEL;
							fallback_1 = -1;
						}
						else if(type == WHERE_LEVEL_DOWN)
						{
							fallback_co = FALLBACK::CHANGE_LEVEL;
							fallback_1 = +1;
						}
						else
						{
							if(location->TryGetPortal(type))
							{
								fallback_co = FALLBACK::USE_PORTAL;
								fallback_1 = type;
							}
							else
							{
								StreamError("Update server: LEAVE_LOCATION from %s, invalid type %d.", type);
								break;
							}
						}

						fallback_t = -1.f;
						for(Unit* team_member : Team.members)
							team_member->frozen = FROZEN::YES;
					}
					else
					{
						// can't leave
						NetChangePlayer& c = Add1(info.changes);
						c.type = NetChangePlayer::CANT_LEAVE_LOCATION;
						c.id = (int)result;
					}
				}
			}
			break;
		// player open/close door
		case NetChange::USE_DOOR:
			{
				int netid;
				bool is_closing;
				f >> netid;
				f >> is_closing;
				if(!f)
				{
					StreamError("Update server: Broken USE_DOOR from %s.", info.name.c_str());
					break;
				}

				Door* door = FindDoor(netid);
				if(!door)
				{
					StreamError("Update server: USE_DOOR from %s, missing door %d.", info.name.c_str(), netid);
					break;
				}

				bool ok = true;
				if(is_closing)
				{
					// closing door
					if(door->state == Door::Open)
					{
						door->state = Door::Closing;
						door->mesh_inst->Play(&door->mesh_inst->mesh->anims[0], PLAY_ONCE | PLAY_STOP_AT_END | PLAY_NO_BLEND | PLAY_BACK, 0);
						door->mesh_inst->frame_end_info = false;
					}
					else if(door->state == Door::Opening)
					{
						door->state = Door::Closing2;
						door->mesh_inst->Play(&door->mesh_inst->mesh->anims[0], PLAY_ONCE | PLAY_STOP_AT_END | PLAY_BACK, 0);
						door->mesh_inst->frame_end_info = false;
					}
					else if(door->state == Door::Opening2)
					{
						door->state = Door::Closing;
						door->mesh_inst->Play(&door->mesh_inst->mesh->anims[0], PLAY_ONCE | PLAY_STOP_AT_END | PLAY_BACK, 0);
						door->mesh_inst->frame_end_info = false;
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
						door->mesh_inst->Play(&door->mesh_inst->mesh->anims[0], PLAY_ONCE | PLAY_STOP_AT_END | PLAY_NO_BLEND, 0);
						door->mesh_inst->frame_end_info = false;
					}
					else if(door->state == Door::Closing)
					{
						door->locked = LOCK_NONE;
						door->state = Door::Opening2;
						door->mesh_inst->Play(&door->mesh_inst->mesh->anims[0], PLAY_ONCE | PLAY_STOP_AT_END, 0);
						door->mesh_inst->frame_end_info = false;
					}
					else if(door->state == Door::Closing2)
					{
						door->locked = LOCK_NONE;
						door->state = Door::Opening;
						door->mesh_inst->Play(&door->mesh_inst->mesh->anims[0], PLAY_ONCE | PLAY_STOP_AT_END, 0);
						door->mesh_inst->frame_end_info = false;
					}
					else
						ok = false;
				}

				if(ok && Rand() == 0)
				{
					SOUND snd;
					if(is_closing && Rand() % 2 == 0)
						snd = sDoorClose;
					else
						snd = sDoor[Rand() % 3];
					sound_mgr->PlaySound3d(snd, door->GetCenter(), 2.f, 5.f);
				}

				// send to other players
				NetChange& c = Add1(Net::changes);
				c.type = NetChange::USE_DOOR;
				c.id = netid;
				c.ile = (is_closing ? 1 : 0);
			}
			break;
		// leader wants to travel to location
		case NetChange::TRAVEL:
			{
				byte loc;
				f >> loc;
				if(!f)
					StreamError("Update server: Broken TRAVEL from %s.", info.name.c_str());
				else if(!Team.IsLeader(unit))
					StreamError("Update server: LEAVE_LOCATION from %s, player is not leader.", info.name.c_str());
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
					world_dir = Angle(world_pos.x, -world_pos.y, l.pos.x, -l.pos.y);
					travel_time2 = 0.f;

					// leave current location
					if(open_location != -1)
					{
						LeaveLocation();
						open_location = -1;
					}

					// send info to players
					NetChange& c = Add1(Net::changes);
					c.type = NetChange::TRAVEL;
					c.id = loc;
				}
			}
			break;
		// leader finished travel
		case NetChange::END_TRAVEL:
			if(world_state == WS_TRAVEL)
			{
				world_state = WS_MAIN;
				current_location = picked_location;
				Location& loc = *locations[current_location];
				if(loc.state == LS_KNOWN)
					SetLocationVisited(loc);
				world_pos = loc.pos;
				Net::PushChange(NetChange::END_TRAVEL);
			}
			break;
		// enter current location
		case NetChange::ENTER_LOCATION:
			if(game_state == GS_WORLDMAP && world_state == WS_MAIN && Team.IsLeader(info.u))
			{
				if(EnterLocation())
				{
					StreamEnd();
					return false;
				}
			}
			else
				StreamError("Update server: ENTER_LOCATION from %s, not leader or not on map.", info.name.c_str());
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
			Net::PushChange(NetChange::CLOSE_ENCOUNTER);
			Event_RandomEncounter(0);
			StreamEnd();
			return false;
		// player used cheat to change level (<>+shift+ctrl)
		case NetChange::CHEAT_CHANGE_LEVEL:
			{
				bool is_down;
				f >> is_down;
				if(!f)
					StreamError("Update server: Broken CHEAT_CHANGE_LEVEL from %s.", info.name.c_str());
				else if(!info.devmode)
					StreamError("Update server: Player %s used CHEAT_CHANGE_LEVEL without devmode.", info.name.c_str());
				else if(location->outside)
					StreamError("Update server:CHEAT_CHANGE_LEVEL from %s, outside location.", info.name.c_str());
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
				f >> is_down;
				if(!f)
					StreamError("Update server: Broken CHEAT_CHANGE_LEVEL from %s.", info.name.c_str());
				else if(!info.devmode)
					StreamError("Update server: Player %s used CHEAT_CHANGE_LEVEL without devmode.", info.name.c_str());
				else
				{
					InsideLocation* inside = (InsideLocation*)location;
					InsideLocationLevel& lvl = inside->GetLevelData();

					if(!is_down)
					{
						Int2 tile = lvl.GetUpStairsFrontTile();
						unit.rot = dir_to_rot(lvl.staircase_up_dir);
						WarpUnit(unit, Vec3(2.f*tile.x + 1.f, 0.f, 2.f*tile.y + 1.f));
					}
					else
					{
						Int2 tile = lvl.GetDownStairsFrontTile();
						unit.rot = dir_to_rot(lvl.staircase_down_dir);
						WarpUnit(unit, Vec3(2.f*tile.x + 1.f, 0.f, 2.f*tile.y + 1.f));
					}
				}
			}
			break;
		// player used cheat 'noai'
		case NetChange::CHEAT_NOAI:
			{
				bool state;
				f >> state;
				if(!f)
					StreamError("Update server: Broken CHEAT_NOAI from %s.", info.name.c_str());
				else if(info.devmode)
				{
					if(noai != state)
					{
						noai = state;
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::CHEAT_NOAI;
						c.id = (state ? 1 : 0);
					}
				}
				else
					StreamError("Update server: Player %s used CHEAT_NOAI without devmode.", info.name.c_str());
			}
			break;
		// player rest in inn
		case NetChange::REST:
			{
				byte days;
				f >> days;
				if(!f)
					StreamError("Update server: Broken REST from %s.", info.name.c_str());
				else
				{
					player.Rest(days, true);
					UseDays(&player, days);
					NetChangePlayer& c = Add1(info.changes);
					c.type = NetChangePlayer::END_FALLBACK;
				}
			}
			break;
		// player trains
		case NetChange::TRAIN:
			{
				byte type, stat_type;
				f >> type;
				f >> stat_type;
				if(!f)
					StreamError("Update server: Broken TRAIN from %s.", info.name.c_str());
				else
				{
					if(type == 2)
						TournamentTrain(unit);
					else
					{
						cstring error = nullptr;
						if(type == 0)
						{
							if(stat_type >= (byte)AttributeId::MAX)
								error = "attribute";
						}
						else
						{
							if(stat_type >= (byte)SkillId::MAX)
								error = "skill";
						}
						if(error)
						{
							StreamError("Update server: TRAIN from %s, invalid %d %u.", info.name.c_str(), error, stat_type);
							break;
						}
						Train(unit, type == 1, stat_type);
					}
					player.Rest(10, false);
					UseDays(&player, 10);
					NetChangePlayer& c = Add1(info.changes);
					c.type = NetChangePlayer::END_FALLBACK;
				}
			}
			break;
		// player wants to change leader
		case NetChange::CHANGE_LEADER:
			{
				byte id;
				f >> id;
				if(!f)
					StreamError("Update server: Broken CHANGE_LEADER from %s.", info.name.c_str());
				else if(leader_id != info.id)
					StreamError("Update server: CHANGE_LEADER from %s, player is not leader.", info.name.c_str());
				else
				{
					PlayerInfo* new_leader = GetPlayerInfoTry(id);
					if(!new_leader)
					{
						StreamError("Update server: CHANGE_LEADER from %s, invalid player id %u.", id);
						break;
					}

					leader_id = id;
					Team.leader = new_leader->u;

					if(leader_id == my_id)
						AddMsg(txYouAreLeader);
					else
						AddMsg(Format(txPcIsLeader, Team.leader->player->name.c_str()));

					NetChange& c = Add1(Net::changes);
					c.type = NetChange::CHANGE_LEADER;
					c.id = id;
				}
			}
			break;
		// player pays credit
		case NetChange::PAY_CREDIT:
			{
				int count;
				f >> count;
				if(!f)
					StreamError("Update server: Broken PAY_CREDIT from %s.", info.name.c_str());
				else if(unit.gold < count || player.credit < count || count < 0)
				{
					StreamError("Update server: PAY_CREDIT from %s, invalid count %d (gold %d, credit %d).",
						info.name.c_str(), count, unit.gold, player.credit);
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
				f >> netid;
				f >> count;
				if(!f)
				{
					StreamError("Update server: Broken GIVE_GOLD from %s.", info.name.c_str());
					break;
				}

				Unit* target = Team.FindActiveTeamMember(netid);
				if(!target)
					StreamError("Update server: GIVE_GOLD from %s, missing unit %d.", info.name.c_str(), netid);
				else if(target == &unit)
					StreamError("Update server: GIVE_GOLD from %s, wants to give gold to himself.", info.name.c_str());
				else if(count > unit.gold || count < 0)
					StreamError("Update server: GIVE_GOLD from %s, invalid count %d (have %d).", info.name.c_str(), count, unit.gold);
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
							NetChangePlayer& c = Add1(target->player->player_info->changes);
							c.type = NetChangePlayer::GOLD_RECEIVED;
							c.id = info.id;
							c.ile = count;
							target->player->player_info->UpdateGold();
						}
						else
						{
							AddMultiMsg(Format(txReceivedGold, count, info.name.c_str()));
							sound_mgr->PlaySound2d(sCoins);
						}
					}
					else if(player.IsTradingWith(target))
					{
						// message about changing trader gold
						NetChangePlayer& c = Add1(info.changes);
						c.type = NetChangePlayer::UPDATE_TRADER_GOLD;
						c.id = target->netid;
						c.ile = target->gold;
						info.UpdateGold();
					}
				}
			}
			break;
		// player drops gold on group
		case NetChange::DROP_GOLD:
			{
				int count;
				f >> count;
				if(!f)
					StreamError("Update server: Broken DROP_GOLD from %s.", info.name.c_str());
				else if(count > 0 && count <= unit.gold && unit.IsStanding() && unit.action == A_NONE)
				{
					unit.gold -= count;

					// animation
					unit.action = A_ANIMATION;
					unit.mesh_inst->Play("wyrzuca", PLAY_ONCE | PLAY_PRIO2, 0);
					unit.mesh_inst->groups[0].speed = 1.f;
					unit.mesh_inst->frame_end_info = false;

					// create item
					GroundItem* item = new GroundItem;
					item->item = gold_item_ptr;
					item->count = count;
					item->team_count = 0;
					item->pos = unit.pos;
					item->pos.x -= sin(unit.rot)*0.25f;
					item->pos.z -= cos(unit.rot)*0.25f;
					item->rot = Random(MAX_ANGLE);
					AddGroundItem(GetContext(*info.u), item);

					// send info to other players
					if(players > 2)
					{
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::DROP_ITEM;
						c.unit = &unit;
					}
				}
				else
					StreamError("Update server: DROP_GOLD from %s, invalid count %d or busy.", info.name.c_str());
			}
			break;
		// player puts gold into container
		case NetChange::PUT_GOLD:
			{
				int count;
				f >> count;
				if(!f)
					StreamError("Update server: Broken PUT_GOLD from %s.", info.name.c_str());
				else if(count < 0 || count > unit.gold)
					StreamError("Update server: PUT_GOLD from %s, invalid count %d (have %d).", info.name.c_str(), count, unit.gold);
				else if(player.action != PlayerController::Action_LootChest && player.action != PlayerController::Action_LootUnit
					&& player.action != PlayerController::Action_LootContainer)
				{
					StreamError("Update server: PUT_GOLD from %s, player is not trading.", info.name.c_str());
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
				f >> location_index;
				if(!f)
					StreamError("Update server: Broken CHEAT_TRAVEL from %s.", info.name.c_str());
				else if(!info.devmode)
					StreamError("Update server: Player %s used CHEAT_TRAVEL without devmode.", info.name.c_str());
				else if(!Team.IsLeader(unit))
					StreamError("Update server: CHEAT_TRAVEL from %s, player is not leader.", info.name.c_str());
				else if(location_index >= locations.size() || !locations[location_index])
					StreamError("Update server: CHEAT_TRAVEL from %s, invalid location index %u.", info.name.c_str(), location_index);
				else
				{
					current_location = location_index;
					Location& loc = *locations[location_index];
					if(loc.state == LS_KNOWN)
						SetLocationVisited(loc);
					world_pos = loc.pos;
					if(open_location != -1)
					{
						LeaveLocation(false, false);
						open_location = -1;
					}
					// inform other players
					if(players > 2)
					{
						NetChange& c = Add1(Net::changes);
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
				f >> netid;
				if(!f)
					StreamError("Update server: Broken CHEAT_HURT from %s.", info.name.c_str());
				else if(!info.devmode)
					StreamError("Update server: Player %s used CHEAT_HURT without devmode.", info.name.c_str());
				else
				{
					Unit* target = FindUnit(netid);
					if(target)
						GiveDmg(GetContext(*target), nullptr, 100.f, *target);
					else
						StreamError("Update server: CHEAT_HURT from %s, missing unit %d.", info.name.c_str(), netid);
				}
			}
			break;
		// player used cheat 'break_action'
		case NetChange::CHEAT_BREAK_ACTION:
			{
				int netid;
				f >> netid;
				if(!f)
					StreamError("Update server: Broken CHEAT_BREAK_ACTION from %s.", info.name.c_str());
				else if(!info.devmode)
					StreamError("Update server: Player %s used CHEAT_BREAK_ACTION without devmode.", info.name.c_str());
				else
				{
					Unit* target = FindUnit(netid);
					if(target)
						BreakUnitAction(*target, BREAK_ACTION_MODE::NORMAL, true);
					else
						StreamError("Update server: CHEAT_BREAK_ACTION from %s, missing unit %d.", info.name.c_str(), netid);
				}
			}
			break;
		// player used cheat 'fall'
		case NetChange::CHEAT_FALL:
			{
				int netid;
				f >> netid;
				if(!f)
					StreamError("Update server: Broken CHEAT_FALL from %s.", info.name.c_str());
				else if(!info.devmode)
					StreamError("Update server: Player %s used CHEAT_FALL without devmode.", info.name.c_str());
				else
				{
					Unit* target = FindUnit(netid);
					if(target)
						UnitFall(*target);
					else
						StreamError("Update server: CHEAT_FALL from %s, missing unit %d.", info.name.c_str(), netid);
				}
			}
			break;
		// player yell to move ai
		case NetChange::YELL:
			PlayerYell(unit);
			break;
		// player used cheat 'stun'
		case NetChange::CHEAT_STUN:
			{
				int netid;
				float length;
				f >> netid;
				f >> length;
				if(!f)
					StreamError("Update server: Broken CHEAT_STUN from %s.", info.name.c_str());
				else if(!info.devmode)
					StreamError("Update server: Player %s used CHEAT_STUN without devmode.", info.name.c_str());
				else
				{
					Unit* target = FindUnit(netid);
					if(target && length > 0)
						target->ApplyStun(length);
					else
						StreamError("Update server: CHEAT_STUN from %s, missing unit %d.", info.name.c_str(), netid);
				}
			}
			break;
		// player used action
		case NetChange::PLAYER_ACTION:
			{
				Vec3 pos;
				f >> pos;
				if(!f)
					StreamError("Update server: Broken PLAYER_ACTION from %s.", info.name.c_str());
				else
					UseAction(info.pc, false, &pos);
			}
			break;
		// player used cheat 'refresh_cooldown'
		case NetChange::CHEAT_REFRESH_COOLDOWN:
			if(!info.devmode)
				StreamError("Update server: Player %s used CHEAT_REFRESH_COOLDOWN without devmode.", info.name.c_str());
			else
				info.pc->RefreshCooldown();
			break;
		// client fallback ended
		case NetChange::END_FALLBACK:
			info.u->frozen = FROZEN::NO;
			break;
		// run script
		case NetChange::RUN_SCRIPT:
			{
				LocalString code;
				int target_netid;
				f.ReadString4(*code);
				f >> target_netid;
				if(!f)
				{
					StreamError("Update server: Broken RUN_SCRIPT from %s.", info.name.c_str());
					break;
				}

				if(!info.devmode)
				{
					StreamError("Update server: Player %s used RUN_SCRIPT without devmode.", info.name.c_str());
					break;
				}

				Unit* target = FindUnit(target_netid);
				if(!target && target_netid != -1)
				{
					StreamError("Update server: RUN_SCRIPT, invalid target %d from %s.", target_netid, info.name.c_str());
					break;
				}

				string& output = script_mgr->OpenOutput();
				script_mgr->SetContext(info.pc, target);
				script_mgr->RunScript(code->c_str());

				NetChangePlayer& c = Add1(info.changes);
				c.type = NetChangePlayer::RUN_SCRIPT_RESULT;
				if(output.empty())
					c.str = nullptr;
				else
				{
					c.str = code.Pin();
					*c.str = output;
				}

				script_mgr->CloseOutput();
			}
			break;
		// player set next action
		case NetChange::SET_NEXT_ACTION:
			{
				f.ReadCasted<byte>(info.pc->next_action);
				if(!f)
				{
					StreamError("Update server: Broken SET_NEXT_ACTION from '%s'.", info.name.c_str());
					info.pc->next_action = NA_NONE;
					break;
				}
				switch(info.pc->next_action)
				{
				case NA_NONE:
					break;
				case NA_REMOVE:
				case NA_DROP:
					f.ReadCasted<byte>(info.pc->next_action_data.slot);
					if(!f)
					{
						StreamError("Update server: Broken SET_NEXT_ACTION(2) from '%s'.", info.name.c_str());
						info.pc->next_action = NA_NONE;
					}
					else if(!IsValid(info.pc->next_action_data.slot) || !info.pc->unit->slots[info.pc->next_action_data.slot])
					{
						StreamError("Update server: SET_NEXT_ACTION, invalid slot %d from '%s'.", info.pc->next_action_data.slot, info.name.c_str());
						info.pc->next_action = NA_NONE;
					}
					break;
				case NA_EQUIP:
				case NA_CONSUME:
					{
						f >> info.pc->next_action_data.index;
						const string& item_id = f.ReadString1();
						if(!f)
						{
							StreamError("Update server: Broken SET_NEXT_ACTION(3) from '%s'.", info.name.c_str());
							info.pc->next_action = NA_NONE;
						}
						else if(info.pc->next_action_data.index < 0 || (uint)info.pc->next_action_data.index >= info.u->items.size())
						{
							StreamError("Update server: SET_NEXT_ACTION, invalid index %d from '%s'.", info.pc->next_action_data.index, info.name.c_str());
							info.pc->next_action = NA_NONE;
						}
						else
						{
							info.pc->next_action_data.item = Item::TryGet(item_id);
							if(!info.pc->next_action_data.item)
							{
								StreamError("Update server: SET_NEXT_ACTION, invalid item '%s' from '%s'.", item_id.c_str(), info.name.c_str());
								info.pc->next_action = NA_NONE;
							}
						}
					}
					break;
				case NA_USE:
					{
						int netid;
						f >> netid;
						if(!f)
						{
							StreamError("Update server: Broken SET_NEXT_ACTION(4) from '%s'.", info.name.c_str());
							info.pc->next_action = NA_NONE;
						}
						else
						{
							info.pc->next_action_data.usable = FindUsable(netid);
							if(!info.pc->next_action_data.usable)
							{
								StreamError("Update server: SET_NEXT_ACTION, invalid usable %d from '%s'.", netid, info.name.c_str());
								info.pc->next_action = NA_NONE;
							}
						}
					}
					break;
				default:
					StreamError("Update server: SET_NEXT_ACTION, invalid action %d from '%s'.", info.pc->next_action, info.name.c_str());
					info.pc->next_action = NA_NONE;
					break;
				}
			}
			break;
		// player toggle always run - notify to save it
		case NetChange::CHANGE_ALWAYS_RUN:
			{
				bool always_run;
				f >> always_run;
				if(!f)
					StreamError("Update server: Broken CHANGE_ALWAYS_RUN from %s.", info.name.c_str());
				else
					info.pc->always_run = always_run;
			}
			break;
		// invalid change
		default:
			StreamError("Update server: Invalid change type %u from %s.", type, info.name.c_str());
			break;
		}

		byte checksum;
		f >> checksum;
		if(!f || checksum != 0xFF)
		{
			StreamError("Update server: Invalid checksum from %s (%u).", info.name.c_str(), change_i);
			return true;
		}
	}

	return true;
}

//=================================================================================================
void Game::WriteServerChanges(BitStreamWriter& f)
{
	// count
	f.WriteCasted<word>(Net::changes.size());
	if(Net::changes.size() >= 0xFFFF)
		Error("Too many changes %d!", Net::changes.size());

	// changes
	for(NetChange& c : Net::changes)
	{
		f.WriteCasted<byte>(c.type);

		switch(c.type)
		{
		case NetChange::UNIT_POS:
			{
				Unit& unit = *c.unit;
				f << unit.netid;
				f << unit.pos;
				f << unit.rot;
				f << unit.mesh_inst->groups[0].speed;
				f.WriteCasted<byte>(unit.animation);
			}
			break;
		case NetChange::CHANGE_EQUIPMENT:
			f << c.unit->netid;
			f.WriteCasted<byte>(c.id);
			WriteBaseItem(f, c.unit->slots[c.id]);
			break;
		case NetChange::TAKE_WEAPON:
			{
				Unit& u = *c.unit;
				f << u.netid;
				f << (c.id != 0);
				f.WriteCasted<byte>(c.id == 0 ? u.weapon_taken : u.weapon_hiding);
			}
			break;
		case NetChange::ATTACK:
			{
				Unit&u = *c.unit;
				f << u.netid;
				byte b = (byte)c.id;
				b |= ((u.attack_id & 0xF) << 4);
				f << b;
				f << c.f[1];
			}
			break;
		case NetChange::CHANGE_FLAGS:
			{
				byte b = 0;
				if(Team.is_bandit)
					b |= 0x01;
				if(Team.crazies_attack)
					b |= 0x02;
				if(anyone_talking)
					b |= 0x04;
				f << b;
			}
			break;
		case NetChange::UPDATE_HP:
			f << c.unit->netid;
			f << c.unit->hp;
			f << c.unit->hpmax;
			break;
		case NetChange::SPAWN_BLOOD:
			f.WriteCasted<byte>(c.id);
			f << c.pos;
			break;
		case NetChange::HURT_SOUND:
		case NetChange::DIE:
		case NetChange::FALL:
		case NetChange::DROP_ITEM:
		case NetChange::STUNNED:
		case NetChange::HELLO:
		case NetChange::STAND_UP:
		case NetChange::SHOUT:
		case NetChange::CREATE_DRAIN:
		case NetChange::HERO_LEAVE:
		case NetChange::REMOVE_USED_ITEM:
		case NetChange::USABLE_SOUND:
		case NetChange::BREAK_ACTION:
		case NetChange::PLAYER_ACTION:
			f << c.unit->netid;
			break;
		case NetChange::TELL_NAME:
			f << c.unit->netid;
			f << (c.id == 1);
			if(c.id == 1)
				f << c.unit->hero->name;
			break;
		case NetChange::CAST_SPELL:
			f << c.unit->netid;
			f.WriteCasted<byte>(c.id);
			break;
		case NetChange::PICKUP_ITEM:
			f << c.unit->netid;
			f << (c.ile != 0);
			break;
		case NetChange::SPAWN_ITEM:
			WriteItem(f, *c.item);
			break;
		case NetChange::REMOVE_ITEM:
			f << c.id;
			break;
		case NetChange::CONSUME_ITEM:
			{
				const Item* item = (const Item*)c.id;
				f << c.unit->netid;
				f << item->id;
				f << (c.ile != 0);
			}
			break;
		case NetChange::HIT_SOUND:
			f << c.pos;
			f.WriteCasted<byte>(c.id);
			f.WriteCasted<byte>(c.ile);
			break;
		case NetChange::SHOOT_ARROW:
			{
				f << (c.unit ? c.unit->netid : -1);
				f << c.pos;
				f << c.vec3;
				f << c.extra_f;
			}
			break;
		case NetChange::UPDATE_CREDIT:
			{
				f << (byte)Team.GetActiveTeamSize();
				for(Unit* unit : Team.active_members)
				{
					f << unit->netid;
					f << unit->GetCredit();
				}
			}
			break;
		case NetChange::UPDATE_FREE_DAYS:
			{
				byte count = 0;
				uint pos = f.BeginPatch(count);
				for(PlayerInfo* info : game_players)
				{
					if(info->left == PlayerInfo::LEFT_NO)
					{
						f << info->u->netid;
						f << info->u->player->free_days;
						++count;
					}
				}
				f.Patch(pos, count);
			}
			break;
		case NetChange::IDLE:
			f << c.unit->netid;
			f.WriteCasted<byte>(c.id);
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
		case NetChange::END_TRAVEL:
			break;
		case NetChange::CHEST_OPEN:
		case NetChange::CHEST_CLOSE:
		case NetChange::KICK_NPC:
		case NetChange::REMOVE_UNIT:
		case NetChange::REMOVE_TRAP:
		case NetChange::TRIGGER_TRAP:
			f << c.id;
			break;
		case NetChange::TALK:
			f << c.unit->netid;
			f << (byte)c.id;
			f << c.ile;
			f << *c.str;
			StringPool.Free(c.str);
			RemoveElement(net_talk, c.str);
			break;
		case NetChange::TALK_POS:
			f << c.pos;
			f << *c.str;
			StringPool.Free(c.str);
			RemoveElement(net_talk, c.str);
			break;
		case NetChange::CHANGE_LOCATION_STATE:
			f.WriteCasted<byte>(c.id);
			f.WriteCasted<byte>(c.ile);
			break;
		case NetChange::ADD_RUMOR:
			f << game_gui->journal->GetRumors()[c.id];
			break;
		case NetChange::HAIR_COLOR:
			f << c.unit->netid;
			f << c.unit->human_data->hair_color;
			break;
		case NetChange::WARP:
			f << c.unit->netid;
			f.WriteCasted<char>(c.unit->in_building);
			f << c.unit->pos;
			f << c.unit->rot;
			break;
		case NetChange::REGISTER_ITEM:
			{
				f << c.base_item->id;
				f << c.item2->id;
				f << c.item2->name;
				f << c.item2->desc;
				f << c.item2->refid;
			}
			break;
		case NetChange::ADD_QUEST:
			{
				Quest* q = QuestManager::Get().FindQuest(c.id, false);
				f << q->refid;
				f << q->name;
				f.WriteString2(q->msgs[0]);
				f.WriteString2(q->msgs[1]);
			}
			break;
		case NetChange::UPDATE_QUEST:
			{
				Quest* q = QuestManager::Get().FindQuest(c.id, false);
				f << q->refid;
				f.WriteCasted<byte>(q->state);
				f.WriteString2(q->msgs.back());
			}
			break;
		case NetChange::RENAME_ITEM:
			{
				const Item* item = c.base_item;
				f << item->refid;
				f << item->id;
				f << item->name;
			}
			break;
		case NetChange::UPDATE_QUEST_MULTI:
			{
				Quest* q = QuestManager::Get().FindQuest(c.id, false);
				f << q->refid;
				f.WriteCasted<byte>(q->state);
				f.WriteCasted<byte>(c.ile);
				for(int i = 0; i < c.ile; ++i)
					f.WriteString2(q->msgs[q->msgs.size() - c.ile + i]);
			}
			break;
		case NetChange::CHANGE_LEADER:
		case NetChange::ARENA_SOUND:
		case NetChange::TRAVEL:
		case NetChange::CHEAT_TRAVEL:
		case NetChange::REMOVE_CAMP:
			f.WriteCasted<byte>(c.id);
			break;
		case NetChange::PAUSED:
		case NetChange::CHEAT_NOAI:
			f << (c.id != 0);
			break;
		case NetChange::RANDOM_NUMBER:
			f.WriteCasted<byte>(c.unit->player->id);
			f.WriteCasted<byte>(c.id);
			break;
		case NetChange::REMOVE_PLAYER:
			f.WriteCasted<byte>(c.id);
			f.WriteCasted<byte>(c.ile);
			break;
		case NetChange::USE_USABLE:
			f << c.unit->netid;
			f << c.id;
			f.WriteCasted<byte>(c.ile);
			break;
		case NetChange::RECRUIT_NPC:
			f << c.unit->netid;
			f << c.unit->hero->free;
			break;
		case NetChange::SPAWN_UNIT:
			WriteUnit(f, *c.unit);
			break;
		case NetChange::CHANGE_ARENA_STATE:
			f << c.unit->netid;
			f.WriteCasted<char>(c.unit->in_arena);
			break;
		case NetChange::WORLD_TIME:
			f << worldtime;
			f.WriteCasted<byte>(day);
			f.WriteCasted<byte>(month);
			f.WriteCasted<byte>(year);
			break;
		case NetChange::USE_DOOR:
			f << c.id;
			f << (c.ile != 0);
			break;
		case NetChange::CREATE_EXPLOSION:
			f << c.spell->id;
			f << c.pos;
			break;
		case NetChange::ENCOUNTER:
			f << *c.str;
			StringPool.Free(c.str);
			break;
		case NetChange::ADD_LOCATION:
			{
				Location& loc = *locations[c.id];
				f.WriteCasted<byte>(c.id);
				f.WriteCasted<byte>(loc.type);
				if(loc.type == L_DUNGEON || loc.type == L_CRYPT)
					f.WriteCasted<byte>(loc.GetLastLevel() + 1);
				f.WriteCasted<byte>(loc.state);
				f << loc.pos;
				f << loc.name;
				f.WriteCasted<byte>(loc.image);
			}
			break;
		case NetChange::CHANGE_AI_MODE:
			{
				f << c.unit->netid;
				byte mode = 0;
				if(c.unit->dont_attack)
					mode |= 0x01;
				if(c.unit->assist)
					mode |= 0x02;
				if(c.unit->ai->state != AIController::Idle)
					mode |= 0x04;
				if(c.unit->attack_team)
					mode |= 0x08;
				f << mode;
			}
			break;
		case NetChange::CHANGE_UNIT_BASE:
			f << c.unit->netid;
			f << c.unit->data->id;
			break;
		case NetChange::CREATE_SPELL_BALL:
			f << c.spell->id;
			f << c.pos;
			f << c.f[0];
			f << c.f[1];
			f << c.extra_netid;
			break;
		case NetChange::SPELL_SOUND:
			f << c.spell->id;
			f << c.pos;
			break;
		case NetChange::CREATE_ELECTRO:
			f << c.e_id;
			f << c.pos;
			f << c.vec3;
			break;
		case NetChange::UPDATE_ELECTRO:
			f << c.e_id;
			f << c.pos;
			break;
		case NetChange::ELECTRO_HIT:
		case NetChange::RAISE_EFFECT:
		case NetChange::HEAL_EFFECT:
			f << c.pos;
			break;
		case NetChange::REVEAL_MINIMAP:
			f.WriteCasted<word>(minimap_reveal_mp.size());
			for(vector<Int2>::iterator it2 = minimap_reveal_mp.begin(), end2 = minimap_reveal_mp.end(); it2 != end2; ++it2)
			{
				f.WriteCasted<byte>(it2->x);
				f.WriteCasted<byte>(it2->y);
			}
			minimap_reveal_mp.clear();
			break;
		case NetChange::CHANGE_MP_VARS:
			WriteNetVars(f);
			break;
		case NetChange::SECRET_TEXT:
			f << GetSecretNote()->desc;
			break;
		case NetChange::UPDATE_MAP_POS:
			f << world_pos;
			break;
		case NetChange::GAME_STATS:
			f << total_kills;
			break;
		case NetChange::STUN:
			f << c.unit->netid;
			f << c.f[0];
			break;
		default:
			Error("Update server: Unknown change %d.", c.type);
			assert(0);
			break;
		}

		f.WriteCasted<byte>(0xFF);
	}
}

//=================================================================================================
void Game::WriteServerChangesForPlayer(BitStreamWriter& f, PlayerInfo& info)
{
	PlayerController& player = *info.pc;

	if(!info.changes.empty())
		info.update_flags |= PlayerInfo::UF_NET_CHANGES;

	f.WriteCasted<byte>(ID_PLAYER_CHANGES);
	f.WriteCasted<byte>(info.update_flags);
	if(IS_SET(info.update_flags, PlayerInfo::UF_POISON_DAMAGE))
		f << player.last_dmg_poison;
	if(IS_SET(info.update_flags, PlayerInfo::UF_NET_CHANGES))
	{
		f.WriteCasted<byte>(info.changes.size());
		if(info.changes.size() > 0xFF)
			Error("Update server: Too many changes for player %s.", info.name.c_str());

		for(NetChangePlayer& c : info.changes)
		{
			f.WriteCasted<byte>(c.type);

			switch(c.type)
			{
			case NetChangePlayer::PICKUP:
				f << c.id;
				f << c.ile;
				break;
			case NetChangePlayer::LOOT:
				f << (c.id != 0);
				if(c.id != 0)
					WriteItemListTeam(f, *player.chest_trade);
				break;
			case NetChangePlayer::START_SHARE:
			case NetChangePlayer::START_GIVE:
				{
					Unit& u = *player.action_unit;
					f << u.weight;
					f << u.weight_max;
					f << u.gold;
					u.stats.Write(f);
					WriteItemListTeam(f, u.items);
				}
				break;
			case NetChangePlayer::GOLD_MSG:
				f << (c.id != 0);
				f << c.ile;
				break;
			case NetChangePlayer::START_DIALOG:
				f << c.id;
				break;
			case NetChangePlayer::SHOW_DIALOG_CHOICES:
				{
					DialogContext& ctx = *player.dialog_ctx;
					f.WriteCasted<byte>(ctx.choices.size());
					f.WriteCasted<char>(ctx.dialog_esc);
					for(DialogChoice& choice : ctx.choices)
						f << choice.msg;
				}
				break;
			case NetChangePlayer::END_DIALOG:
			case NetChangePlayer::USE_USABLE:
			case NetChangePlayer::PREPARE_WARP:
			case NetChangePlayer::ENTER_ARENA:
			case NetChangePlayer::START_ARENA_COMBAT:
			case NetChangePlayer::EXIT_ARENA:
			case NetChangePlayer::END_FALLBACK:
				break;
			case NetChangePlayer::START_TRADE:
				f << c.id;
				WriteItemList(f, *player.chest_trade);
				break;
			case NetChangePlayer::SET_FROZEN:
			case NetChangePlayer::DEVMODE:
			case NetChangePlayer::PVP:
			case NetChangePlayer::NO_PVP:
			case NetChangePlayer::CANT_LEAVE_LOCATION:
			case NetChangePlayer::REST:
				f.WriteCasted<byte>(c.id);
				break;
			case NetChangePlayer::IS_BETTER_ITEM:
				f << (c.id != 0);
				break;
			case NetChangePlayer::REMOVE_QUEST_ITEM:
			case NetChangePlayer::LOOK_AT:
				f << c.id;
				break;
			case NetChangePlayer::ADD_ITEMS:
				{
					f << c.id;
					f << c.ile;
					f << c.item->id;
					if(c.item->id[0] == '$')
						f << c.item->refid;
				}
				break;
			case NetChangePlayer::TRAIN:
				f.WriteCasted<byte>(c.id);
				f.WriteCasted<byte>(c.ile);
				break;
			case NetChangePlayer::UNSTUCK:
				f << c.pos;
				break;
			case NetChangePlayer::GOLD_RECEIVED:
				f.WriteCasted<byte>(c.id);
				f << c.ile;
				break;
			case NetChangePlayer::GAIN_STAT:
				f << (c.id != 0);
				f.WriteCasted<byte>(c.a);
				f.WriteCasted<byte>(c.ile);
				break;
			case NetChangePlayer::ADD_ITEMS_TRADER:
				f << c.id;
				f << c.ile;
				f << c.a;
				WriteBaseItem(f, c.item);
				break;
			case NetChangePlayer::ADD_ITEMS_CHEST:
				f << c.id;
				f << c.ile;
				f << c.a;
				WriteBaseItem(f, c.item);
				break;
			case NetChangePlayer::REMOVE_ITEMS:
				f << c.id;
				f << c.ile;
				break;
			case NetChangePlayer::REMOVE_ITEMS_TRADER:
				f << c.id;
				f << c.ile;
				f << c.a;
				break;
			case NetChangePlayer::UPDATE_TRADER_GOLD:
				f << c.id;
				f << c.ile;
				break;
			case NetChangePlayer::UPDATE_TRADER_INVENTORY:
				f << c.unit->netid;
				WriteItemListTeam(f, c.unit->items);
				break;
			case NetChangePlayer::PLAYER_STATS:
				f.WriteCasted<byte>(c.id);
				if(IS_SET(c.id, STAT_KILLS))
					f << player.kills;
				if(IS_SET(c.id, STAT_DMG_DONE))
					f << player.dmg_done;
				if(IS_SET(c.id, STAT_DMG_TAKEN))
					f << player.dmg_taken;
				if(IS_SET(c.id, STAT_KNOCKS))
					f << player.knocks;
				if(IS_SET(c.id, STAT_ARENA_FIGHTS))
					f << player.arena_fights;
				break;
			case NetChangePlayer::ADDED_ITEMS_MSG:
				f.WriteCasted<byte>(c.ile);
				break;
			case NetChangePlayer::STAT_CHANGED:
				f.WriteCasted<byte>(c.id);
				f.WriteCasted<byte>(c.a);
				f << c.ile;
				break;
			case NetChangePlayer::ADD_PERK:
				f.WriteCasted<byte>(c.id);
				f << c.ile;
				break;
			case NetChangePlayer::GAME_MESSAGE:
				f << c.id;
				break;
			case NetChangePlayer::RUN_SCRIPT_RESULT:
				if(c.str)
				{
					f.WriteString4(*c.str);
					StringPool.Free(c.str);
				}
				else
					f << 0u;
				break;
			default:
				Error("Update server: Unknown player %s change %d.", info.name.c_str(), c.type);
				assert(0);
				break;
			}

			f.WriteCasted<byte>(0xFF);
		}
	}
	if(IS_SET(info.update_flags, PlayerInfo::UF_GOLD))
		f << info.u->gold;
	if(IS_SET(info.update_flags, PlayerInfo::UF_ALCOHOL))
		f << info.u->alcohol;
	if(IS_SET(info.update_flags, PlayerInfo::UF_BUFFS))
		f.WriteCasted<byte>(info.buffs);
	if(IS_SET(info.update_flags, PlayerInfo::UF_STAMINA))
		f << info.u->stamina;
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
				pc->unit->visual_pos = Vec3::Lerp(pc->unit->visual_pos, pc->unit->pos, (0.1f - interpolate_timer) * 10);
			else
				pc->unit->visual_pos = pc->unit->pos;
		}

		// interpolacja pozycji/obrotu postaci
		InterpolateUnits(dt);
	}

	bool exit_from_server = false;

	Packet* packet;
	for(packet = peer->Receive(); packet; peer->DeallocatePacket(packet), packet = peer->Receive())
	{
		BitStream& stream = StreamStart(packet, Stream_UpdateGameClient);
		BitStreamReader reader(stream);
		byte msg_id;
		reader >> msg_id;

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
				Info(text_eng);
				StreamEnd();
				peer->DeallocatePacket(packet);
				ExitToMenu();
				GUI.SimpleDialog(text, nullptr);
				return;
			}
		case ID_SAY:
			Client_Say(reader);
			break;
		case ID_WHISPER:
			Client_Whisper(reader);
			break;
		case ID_SERVER_SAY:
			Client_ServerSay(reader);
			break;
		case ID_SERVER_CLOSE:
			{
				byte reason = (packet->length == 2 ? packet->data[1] : 0);
				cstring reason_text, reason_text_int;
				if(reason == 1)
				{
					reason_text = "You have been kicked out.";
					reason_text_int = txYouKicked;
				}
				else
				{
					reason_text = "Server was closed.";
					reason_text_int = txServerClosed;
				}
				Info("Update client: %s", reason_text);
				StreamEnd();
				peer->DeallocatePacket(packet);
				ExitToMenu();
				GUI.SimpleDialog(reason_text_int, nullptr);
				return;
			}
		case ID_CHANGE_LEVEL:
			{
				byte loc, level;
				reader >> loc;
				reader >> level;
				if(!reader)
					StreamError("Update client: Broken ID_CHANGE_LEVEL.");
				else
				{
					current_location = loc;
					location = locations[loc];
					dungeon_level = level;
					Info("Update client: Level change to %s (%d, level %d).", location->name.c_str(), current_location, dungeon_level);
					info_box->Show(txGeneratingLocation);
					LeaveLevel();
					net_mode = NM_TRANSFER;
					net_state = NetState::Client_ChangingLevel;
					clear_color = Color::Black;
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
			if(!ProcessControlMessageClient(reader, exit_from_server))
			{
				peer->DeallocatePacket(packet);
				return;
			}
			break;
		case ID_PLAYER_CHANGES:
			if(!ProcessControlMessageClientForMe(reader))
			{
				peer->DeallocatePacket(packet);
				return;
			}
			break;
		default:
			Warn("UpdateClient: Unknown packet from server: %u.", msg_id);
			StreamError();
			break;
		}

		StreamEnd();
	}

	// wy�li moj� pozycj�/akcj�
	update_timer += dt;
	if(update_timer >= TICK)
	{
		update_timer = 0;
		net_stream.Reset();
		BitStreamWriter f(net_stream);
		f << ID_CONTROL;
		if(game_state == GS_LEVEL)
		{
			f << true;
			f << pc->unit->pos;
			f << pc->unit->rot;
			f << pc->unit->mesh_inst->groups[0].speed;
			f.WriteCasted<byte>(pc->unit->animation);
		}
		else
			f << false;
		WriteClientChanges(f);
		Net::changes.clear();
		peer->Send(&net_stream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, server, false);
		StreamWrite(net_stream, Stream_UpdateGameClient, server);
	}

	if(exit_from_server)
		peer->Shutdown(1000);
}

//=================================================================================================
bool Game::ProcessControlMessageClient(BitStreamReader& f, bool& exit_from_server)
{
	// read count
	word changes;
	f >> changes;
	if(!f)
	{
		StreamError("Update client: Broken ID_CHANGES.");
		return true;
	}

	// read changes
	for(word change_i = 0; change_i < changes; ++change_i)
	{
		// read type
		NetChange::TYPE type;
		f.ReadCasted<byte>(type);
		if(!f)
		{
			StreamError("Update client: Broken ID_CHANGES(2).");
			return true;
		}

		// process
		switch(type)
		{
		// unit position/rotation/animation
		case NetChange::UNIT_POS:
			{
				int netid;
				Vec3 pos;
				float rot, ani_speed;
				Animation ani;
				f >> netid;
				f >> pos;
				f >> rot;
				f >> ani_speed;
				f.ReadCasted<byte>(ani);
				if(!f)
				{
					StreamError("Update client: Broken UNIT_POS.");
					break;
				}

				Unit* unit = FindUnit(netid);
				if(!unit)
					StreamError("Update client: UNIT_POS, missing unit %d.", netid);
				else if(unit != pc->unit)
				{
					unit->pos = pos;
					unit->mesh_inst->groups[0].speed = ani_speed;
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
				f >> netid;
				f.ReadCasted<byte>(type);
				if(!f || ReadItemAndFind(f, item) < 0)
					StreamError("Update client: Broken CHANGE_EQUIPMENT.");
				else if(!IsValid(type))
					StreamError("Update client: CHANGE_EQUIPMENT, invalid slot %d.", type);
				else
				{
					Unit* target = FindUnit(netid);
					if(!target)
						StreamError("Update client: CHANGE_EQUIPMENT, missing unit %d.", netid);
					else
					{
						if(item)
							PreloadItem(item);
						target->slots[type] = item;
					}
				}
			}
			break;
		// unit take/hide weapon
		case NetChange::TAKE_WEAPON:
			{
				int netid;
				bool hide;
				WeaponType type;
				f >> netid;
				f >> hide;
				f.ReadCasted<byte>(type);
				if(!f)
					StreamError("Update client: Broken TAKE_WEAPON.");
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
						StreamError("Update client: TAKE_WEAPON, missing unit %d.", netid);
					else if(unit != pc->unit)
					{
						if(unit->mesh_inst->mesh->head.n_groups > 1)
							unit->mesh_inst->groups[1].speed = 1.f;
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
				f >> netid;
				f >> typeflags;
				f >> attack_speed;
				if(!f)
				{
					StreamError("Update client: Broken ATTACK.");
					break;
				}

				Unit* unit_ptr = FindUnit(netid);
				if(!unit_ptr)
				{
					StreamError("Update client: ATTACK, missing unit %d.", netid);
					break;
				}

				// don't start attack if this is local unit
				if(unit_ptr == pc->unit)
					break;

				Unit& unit = *unit_ptr;
				byte type = (typeflags & 0xF);
				int group = unit.mesh_inst->mesh->head.n_groups - 1;
				unit.weapon_state = WS_TAKEN;

				switch(type)
				{
				case AID_Attack:
					if(unit.action == A_ATTACK && unit.animation_state == 0)
					{
						unit.animation_state = 1;
						unit.mesh_inst->groups[1].speed = attack_speed;
					}
					else
					{
						if(unit.data->sounds->sound[SOUND_ATTACK] && Rand() % 4 == 0)
							PlayAttachedSound(unit, unit.data->sounds->sound[SOUND_ATTACK]->sound, 1.f, 10.f);
						unit.action = A_ATTACK;
						unit.attack_id = ((typeflags & 0xF0) >> 4);
						unit.attack_power = 1.f;
						unit.mesh_inst->Play(NAMES::ani_attacks[unit.attack_id], PLAY_PRIO1 | PLAY_ONCE | PLAY_RESTORE, group);
						unit.mesh_inst->groups[group].speed = attack_speed;
						unit.animation_state = 1;
						unit.hitted = false;
					}
					break;
				case AID_PrepareAttack:
					{
						if(unit.data->sounds->sound[SOUND_ATTACK] && Rand() % 4 == 0)
							PlayAttachedSound(unit, unit.data->sounds->sound[SOUND_ATTACK]->sound, 1.f, 10.f);
						unit.action = A_ATTACK;
						unit.attack_id = ((typeflags & 0xF0) >> 4);
						unit.attack_power = 1.f;
						unit.mesh_inst->Play(NAMES::ani_attacks[unit.attack_id], PLAY_PRIO1 | PLAY_ONCE | PLAY_RESTORE, group);
						unit.mesh_inst->groups[group].speed = attack_speed;
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
						unit.mesh_inst->Play(NAMES::ani_shoot, PLAY_PRIO1 | PLAY_ONCE | PLAY_RESTORE, group);
						unit.mesh_inst->groups[group].speed = attack_speed;
						unit.action = A_SHOOT;
						unit.animation_state = (type == AID_Shoot ? 1 : 0);
						unit.hitted = false;
						if(!unit.bow_instance)
							unit.bow_instance = GetBowInstance(unit.GetBow().mesh);
						unit.bow_instance->Play(&unit.bow_instance->mesh->anims[0], PLAY_ONCE | PLAY_PRIO1 | PLAY_NO_BLEND, 0);
						unit.bow_instance->groups[0].speed = unit.mesh_inst->groups[group].speed;
					}
					break;
				case AID_Block:
					{
						unit.action = A_BLOCK;
						unit.mesh_inst->Play(NAMES::ani_block, PLAY_PRIO1 | PLAY_STOP_AT_END | PLAY_RESTORE, group);
						unit.mesh_inst->groups[1].speed = 1.f;
						unit.mesh_inst->groups[1].blend_max = attack_speed;
						unit.animation_state = 0;
					}
					break;
				case AID_Bash:
					{
						unit.action = A_BASH;
						unit.animation_state = 0;
						unit.mesh_inst->Play(NAMES::ani_bash, PLAY_ONCE | PLAY_PRIO1 | PLAY_RESTORE, group);
						unit.mesh_inst->groups[1].speed = 2.f;
						unit.mesh_inst->frame_end_info2 = false;
						unit.hitted = false;
					}
					break;
				case AID_RunningAttack:
					{
						if(unit.data->sounds->sound[SOUND_ATTACK] && Rand() % 4 == 0)
							PlayAttachedSound(unit, unit.data->sounds->sound[SOUND_ATTACK]->sound, 1.f, 10.f);
						unit.action = A_ATTACK;
						unit.attack_id = ((typeflags & 0xF0) >> 4);
						unit.attack_power = 1.5f;
						unit.run_attack = true;
						unit.mesh_inst->Play(NAMES::ani_attacks[unit.attack_id], PLAY_PRIO1 | PLAY_ONCE | PLAY_RESTORE, group);
						unit.mesh_inst->groups[group].speed = attack_speed;
						unit.animation_state = 1;
						unit.hitted = false;
					}
					break;
				case AID_StopBlock:
					{
						unit.action = A_NONE;
						unit.mesh_inst->frame_end_info2 = false;
						unit.mesh_inst->Deactivate(group);
						unit.mesh_inst->groups[1].speed = 1.f;
					}
					break;
				}
			}
			break;
		// change of game flags
		case NetChange::CHANGE_FLAGS:
			{
				byte flags;
				f >> flags;
				if(!f)
					StreamError("Update client: Broken CHANGE_FLAGS.");
				else
				{
					Team.is_bandit = IS_SET(flags, 0x01);
					Team.crazies_attack = IS_SET(flags, 0x02);
					anyone_talking = IS_SET(flags, 0x04);
				}
			}
			break;
		// update unit hp
		case NetChange::UPDATE_HP:
			{
				int netid;
				float hp, hpmax;
				f >> netid;
				f >> hp;
				f >> hpmax;
				if(!f)
					StreamError("Update client: Broken UPDATE_HP.");
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
						StreamError("Update client: UPDATE_HP, missing unit %d.", netid);
					else if(unit == pc->unit)
					{
						// handling of previous hp
						float hp_dif = hp - unit->hp - hpmax + unit->hpmax;
						if(hp_dif < 0.f)
						{
							float old_ratio = unit->hp / unit->hpmax;
							float new_ratio = hp / hpmax;
							if(old_ratio > new_ratio)
								pc->last_dmg += -hp_dif;
						}
						unit->hp = hp;
						unit->hpmax = hpmax;
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
				Vec3 pos;
				f >> type;
				f >> pos;
				if(!f)
					StreamError("Update client: Broken SPAWN_BLOOD.");
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
					pe->speed_min = Vec3(-1, 0, -1);
					pe->speed_max = Vec3(1, 1, 1);
					pe->pos_min = Vec3(-0.1f, -0.1f, -0.1f);
					pe->pos_max = Vec3(0.1f, 0.1f, 0.1f);
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
				f >> netid;
				if(!f)
					StreamError("Update client: Broken HURT_SOUND.");
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
						StreamError("Update client: HURT_SOUND, missing unit %d.", netid);
					else if(unit->data->sounds->sound[SOUND_PAIN])
						PlayAttachedSound(*unit, unit->data->sounds->sound[SOUND_PAIN]->sound, 2.f, 15.f);
				}
			}
			break;
		// unit dies
		case NetChange::DIE:
			{
				int netid;
				f >> netid;
				if(!f)
					StreamError("Update client: Broken DIE.");
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
						StreamError("Update client: DIE, missing unit %d.", netid);
					else
						UnitDie(*unit, nullptr, nullptr);
				}
			}
			break;
		// unit falls on ground
		case NetChange::FALL:
			{
				int netid;
				f >> netid;
				if(!f)
					StreamError("Update client: Broken FALL.");
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
						StreamError("Update client: FALL, missing unit %d.", netid);
					else
						UnitFall(*unit);
				}
			}
			break;
		// unit drops item animation
		case NetChange::DROP_ITEM:
			{
				int netid;
				f >> netid;
				if(!f)
					StreamError("Update client: Broken DROP_ITEM.");
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
						StreamError("Update client: DROP_ITEM, missing unit %d.", netid);
					else if(unit != pc->unit)
					{
						unit->action = A_ANIMATION;
						unit->mesh_inst->Play("wyrzuca", PLAY_ONCE | PLAY_PRIO2, 0);
						unit->mesh_inst->groups[0].speed = 1.f;
						unit->mesh_inst->frame_end_info = false;
					}
				}
			}
			break;
		// spawn item on ground
		case NetChange::SPAWN_ITEM:
			{
				GroundItem* item = new GroundItem;
				if(!ReadItem(f, *item))
				{
					StreamError("Update client: Broken SPAWN_ITEM.");
					delete item;
				}
				else
				{
					PreloadItem(item->item);
					GetContext(item->pos).items->push_back(item);
				}
			}
			break;
		// unit picks up item
		case NetChange::PICKUP_ITEM:
			{
				int netid;
				bool up_animation;
				f >> netid;
				f >> up_animation;
				if(!f)
					StreamError("Update client: Broken PICKUP_ITEM.");
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
						StreamError("Update client: PICKUP_ITEM, missing unit %d.", netid);
					else if(unit != pc->unit)
					{
						unit->action = A_PICKUP;
						unit->animation = ANI_PLAY;
						unit->mesh_inst->Play(up_animation ? "podnosi_gora" : "podnosi", PLAY_ONCE | PLAY_PRIO2, 0);
						unit->mesh_inst->groups[0].speed = 1.f;
						unit->mesh_inst->frame_end_info = false;
					}
				}
			}
			break;
		// remove ground item (picked by someone)
		case NetChange::REMOVE_ITEM:
			{
				int netid;
				f >> netid;
				if(!f)
					StreamError("Update client: Broken REMOVE_ITEM.");
				else
				{
					LevelContext* ctx;
					GroundItem* item = FindItemNetid(netid, &ctx);
					if(!item)
						StreamError("Update client: REMOVE_ITEM, missing ground item %d.", netid);
					else
					{
						RemoveElement(ctx->items, item);
						if(pc_data.before_player == BP_ITEM && pc_data.before_player_ptr.item == item)
							pc_data.before_player = BP_NONE;
						if(pc_data.picking_item_state == 1 && pc_data.picking_item == item)
							pc_data.picking_item_state = 2;
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
				f >> netid;
				bool ok = (ReadItemAndFind(f, item) <= 0);
				f >> force;
				if(!f || !ok)
					StreamError("Update client: Broken CONSUME_ITEM.");
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
						StreamError("Update client: CONSUME_ITEM, missing unit %d.", netid);
					else if(item->type != IT_CONSUMABLE)
						StreamError("Update client: CONSUME_ITEM, %s is not consumable.", item->id.c_str());
					else if(unit != pc->unit || force)
					{
						PreloadItem(item);
						unit->ConsumeItem(item->ToConsumable(), false, false);
					}
				}
			}
			break;
		// play hit sound
		case NetChange::HIT_SOUND:
			{
				Vec3 pos;
				MATERIAL_TYPE mat1, mat2;
				f >> pos;
				f.ReadCasted<byte>(mat1);
				f.ReadCasted<byte>(mat2);
				if(!f)
					StreamError("Update client: Broken HIT_SOUND.");
				else
					sound_mgr->PlaySound3d(GetMaterialSound(mat1, mat2), pos, 2.f, 10.f);
			}
			break;
		// unit get stunned
		case NetChange::STUNNED:
			{
				int netid;
				f >> netid;
				if(!f)
					StreamError("Update client: Broken STUNNED.");
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
						StreamError("Update client: STUNNED, missing unit %d.", netid);
					else
					{
						BreakUnitAction(*unit);

						if(unit->action != A_POSITION)
							unit->action = A_PAIN;
						else
							unit->animation_state = 1;

						if(unit->mesh_inst->mesh->head.n_groups == 2)
						{
							unit->mesh_inst->frame_end_info2 = false;
							unit->mesh_inst->Play(NAMES::ani_hurt, PLAY_PRIO1 | PLAY_ONCE, 1);
						}
						else
						{
							unit->mesh_inst->frame_end_info = false;
							unit->mesh_inst->Play(NAMES::ani_hurt, PLAY_PRIO3 | PLAY_ONCE, 0);
							unit->mesh_inst->groups[0].speed = 1.f;
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
				Vec3 pos;
				float rotX, rotY, speedY, speed;
				f >> netid;
				f >> pos;
				f >> rotY;
				f >> speedY;
				f >> rotX;
				f >> speed;
				if(!f)
					StreamError("Update client: Broken SHOOT_ARROW.");
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
							StreamError("Update client: SHOOT_ARROW, missing unit %d.", netid);
							break;
						}
					}

					LevelContext& ctx = GetContext(pos);

					Bullet& b = Add1(ctx.bullets);
					b.mesh = aArrow;
					b.pos = pos;
					b.start_pos = pos;
					b.rot = Vec3(rotX, rotY, 0);
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
					tpe->color1 = Vec4(1, 1, 1, 0.5f);
					tpe->color2 = Vec4(1, 1, 1, 0);
					tpe->Init(50);
					ctx.tpes->push_back(tpe);
					b.trail = tpe;

					TrailParticleEmitter* tpe2 = new TrailParticleEmitter;
					tpe2->fade = 0.3f;
					tpe2->color1 = Vec4(1, 1, 1, 0.5f);
					tpe2->color2 = Vec4(1, 1, 1, 0);
					tpe2->Init(50);
					ctx.tpes->push_back(tpe2);
					b.trail2 = tpe2;

					sound_mgr->PlaySound3d(sBow[Rand() % 2], b.pos, 2.f, 8.f);
				}
			}
			break;
		// update team credit
		case NetChange::UPDATE_CREDIT:
			{
				byte count;
				f >> count;
				if(!f)
					StreamError("Update client: Broken UPDATE_CREDIT.");
				else
				{
					for(byte i = 0; i < count; ++i)
					{
						int netid, credit;
						f >> netid;
						f >> credit;
						if(!f)
							StreamError("Update client: Broken UPDATE_CREDIT(2).");
						else
						{
							Unit* unit = FindUnit(netid);
							if(!unit)
								StreamError("Update client: UPDATE_CREDIT, missing unit %d.", netid);
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
				f >> netid;
				f >> animation_index;
				if(!f)
					StreamError("Update client: Broken IDLE.");
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
						StreamError("Update client: IDLE, missing unit %d.", netid);
					else if(animation_index >= unit->data->idles->anims.size())
						StreamError("Update client: IDLE, invalid animation index %u (count %u).", animation_index, unit->data->idles->anims.size());
					else
					{
						unit->mesh_inst->Play(unit->data->idles->anims[animation_index].c_str(), PLAY_ONCE, 0);
						unit->mesh_inst->groups[0].speed = 1.f;
						unit->mesh_inst->frame_end_info = false;
						unit->animation = ANI_IDLE;
					}
				}
			}
			break;
		// play unit hello sound
		case NetChange::HELLO:
			{
				int netid;
				f >> netid;
				if(!f)
					StreamError("Update client: Broken HELLO.");
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
						StreamError("Update client: BROKEN, missing unit %d.", netid);
					else
					{
						SOUND snd = GetTalkSound(*unit);
						if(snd)
							PlayAttachedSound(*unit, snd, 2.f, 5.f);
					}
				}
			}
			break;
		// info about completing all unique quests
		case NetChange::ALL_QUESTS_COMPLETED:
			QuestManager::Get().unique_completed_show = true;
			break;
		// unit talks
		case NetChange::TALK:
			{
				int netid, skip_id;
				byte animation;
				f >> netid;
				f >> animation;
				f >> skip_id;
				const string& text = f.ReadString1();
				if(!f)
					StreamError("Update client: Broken TALK.");
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
						StreamError("Update client: TALK, missing unit %d.", netid);
					else
					{
						game_gui->AddSpeechBubble(unit, text.c_str());
						unit->bubble->skip_id = skip_id;

						if(animation != 0)
						{
							unit->mesh_inst->Play(animation == 1 ? "i_co" : "pokazuje", PLAY_ONCE | PLAY_PRIO2, 0);
							unit->mesh_inst->groups[0].speed = 1.f;
							unit->animation = ANI_PLAY;
							unit->action = A_ANIMATION;
						}

						if(dialog_context.dialog_mode && dialog_context.talker == unit)
						{
							dialog_context.dialog_s_text = text;
							dialog_context.dialog_text = dialog_context.dialog_s_text.c_str();
							dialog_context.dialog_wait = 1.f;
							dialog_context.skip_id = skip_id;
						}
						else if(pc->action == PlayerController::Action_Talk && pc->action_unit == unit)
						{
							predialog = text;
							dialog_context.skip_id = skip_id;
						}
					}
				}
			}
			break;
		// show talk text at position
		case NetChange::TALK_POS:
			{
				Vec3 pos;
				f >> pos;
				const string& text = f.ReadString1();
				if(!f)
					StreamError("Update client: Broken TALK_POS.");
				else
					game_gui->AddSpeechBubble(pos, text.c_str());
			}
			break;
		// change location state
		case NetChange::CHANGE_LOCATION_STATE:
			{
				byte location_index, state;
				f >> location_index;
				f >> state;
				if(!f)
					StreamError("Update client: Broken CHANGE_LOCATION_STATE.");
				else
				{
					Location* loc = nullptr;
					if(location_index < locations.size())
						loc = locations[location_index];
					if(!loc)
						StreamError("Update client: CHANGE_LOCATION_STATE, missing location %u.", location_index);
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
			{
				const string& text = f.ReadString1();
				if(!f)
					StreamError("Update client: Broken ADD_RUMOR.");
				else
				{
					AddGameMsg3(GMS_ADDED_RUMOR);
					game_gui->journal->GetRumors().push_back(text);
					game_gui->journal->NeedUpdate(Journal::Rumors);
				}
			}
			break;
		// hero tells his name
		case NetChange::TELL_NAME:
			{
				int netid;
				bool set_name;
				f >> netid;
				f >> set_name;
				if(set_name)
					f.ReadString1();
				if(!f)
					StreamError("Update client: Broken TELL_NAME.");
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
						StreamError("Update client: TELL_NAME, missing unit %d.", netid);
					else if(!unit->IsHero())
						StreamError("Update client: TELL_NAME, unit %d (%s) is not a hero.", netid, unit->data->id.c_str());
					else
					{
						unit->hero->know_name = true;
						if(set_name)
							unit->hero->name = f.GetStringBuffer();
					}
				}
			}
			break;
		// change unit hair color
		case NetChange::HAIR_COLOR:
			{
				int netid;
				Vec4 hair_color;
				f >> netid;
				f >> hair_color;
				if(!f)
					StreamError("Update client: Broken HAIR_COLOR.");
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
						StreamError("Update client: HAIR_COLOR, missing unit %d.", netid);
					else if(unit->data->type != UNIT_TYPE::HUMAN)
						StreamError("Update client: HAIR_COLOR, unit %d (%s) is not human.", netid, unit->data->id.c_str());
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
				Vec3 pos;
				float rot;
				f >> netid;
				f >> in_building;
				f >> pos;
				f >> rot;

				if(!f)
				{
					StreamError("Update client: Broken WARP.");
					break;
				}

				Unit* unit = FindUnit(netid);
				if(!unit)
				{
					StreamError("Update client: WARP, missing unit %d.", netid);
					break;
				}

				BreakUnitAction(*unit, BREAK_ACTION_MODE::INSTANT, false, true);

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
					if(fallback_co == FALLBACK::WAIT_FOR_WARP)
						fallback_co = FALLBACK::NONE;
					else if(fallback_co == FALLBACK::ARENA)
					{
						pc->unit->frozen = FROZEN::ROTATE;
						fallback_co = FALLBACK::NONE;
					}
					else if(fallback_co == FALLBACK::ARENA_EXIT)
					{
						pc->unit->frozen = FROZEN::NO;
						fallback_co = FALLBACK::NONE;

						if(pc->unit->hp <= 0.f)
						{
							pc->unit->HealPoison();
							pc->unit->live_state = Unit::ALIVE;
							pc->unit->mesh_inst->Play("wstaje2", PLAY_ONCE | PLAY_PRIO3, 0);
							pc->unit->mesh_inst->groups[0].speed = 1.f;
							pc->unit->action = A_ANIMATION;
							pc->unit->animation = ANI_PLAY;
						}
					}
					Net::PushChange(NetChange::WARP);
					interpolate_timer = 0.f;
					pc_data.rot_buf = 0.f;
					cam.Reset();
					pc_data.rot_buf = 0.f;
				}
			}
			break;
		// register new item
		case NetChange::REGISTER_ITEM:
			{
				const string& item_id = f.ReadString1();
				if(!f)
					StreamError("Update client: Broken REGISTER_ITEM.");
				else
				{
					const Item* base;
					base = Item::TryGet(item_id);
					if(!base)
					{
						StreamError("Update client: REGISTER_ITEM, missing base item %s.", item_id.c_str());
						f.SkipString1(); // id
						f.SkipString1(); // name
						f.SkipString1(); // desc
						f.Skip<int>(); // ref
						if(!f)
							Error("Update client: Broken REGISTER_ITEM(2).");
					}
					else
					{
						PreloadItem(base);
						Item* item = CreateItemCopy(base);
						f >> item->id;
						f >> item->name;
						f >> item->desc;
						f >> item->refid;
						if(!f)
							StreamError("Update client: Broken REGISTER_ITEM(3).");
						else
							quest_items.push_back(item);
					}
				}
			}
			break;
		// added quest
		case NetChange::ADD_QUEST:
			{
				int refid;
				f >> refid;
				const string& quest_name = f.ReadString1();
				if(!f)
				{
					StreamError("Update client: Broken ADD_QUEST.");
					break;
				}

				QuestManager& quest_manager = QuestManager::Get();

				PlaceholderQuest* quest = new PlaceholderQuest;
				quest->quest_index = quest_manager.quests.size();
				quest->name = quest_name;
				quest->refid = refid;
				quest->msgs.resize(2);

				f.ReadString2(quest->msgs[0]);
				f.ReadString2(quest->msgs[1]);
				if(!f)
				{
					StreamError("Update client: Broken ADD_QUEST(2).");
					delete quest;
					break;
				}

				quest->state = Quest::Started;
				game_gui->journal->NeedUpdate(Journal::Quests, quest->quest_index);
				AddGameMsg3(GMS_JOURNAL_UPDATED);
				quest_manager.quests.push_back(quest);
			}
			break;
		// update quest
		case NetChange::UPDATE_QUEST:
			{
				int refid;
				byte state;
				f >> refid;
				f >> state;
				const string& msg = f.ReadString2();
				if(!f)
				{
					StreamError("Update client: Broken UPDATE_QUEST.");
					break;
				}

				Quest* quest = QuestManager::Get().FindQuest(refid, false);
				if(!quest)
					StreamError("Update client: UPDATE_QUEST, missing quest %d.", refid);
				else
				{
					quest->state = (Quest::State)state;
					quest->msgs.push_back(msg);
					game_gui->journal->NeedUpdate(Journal::Quests, quest->quest_index);
					AddGameMsg3(GMS_JOURNAL_UPDATED);
				}
			}
			break;
		// item rename
		case NetChange::RENAME_ITEM:
			{
				int refid;
				f >> refid;
				const string& item_id = f.ReadString1();
				if(!f)
					StreamError("Update client: Broken RENAME_ITEM.");
				else
				{
					bool found = false;
					for(Item* item : quest_items)
					{
						if(item->refid == refid && item->id == item_id)
						{
							f >> item->name;
							if(!f)
								StreamError("Update client: Broken RENAME_ITEM(2).");
							found = true;
							break;
						}
					}
					if(!found)
					{
						StreamError("Update client: RENAME_ITEM, missing quest item %d.", refid);
						f.SkipString1();
					}
				}
			}
			break;
		// update quest with multiple texts
		case NetChange::UPDATE_QUEST_MULTI:
			{
				int refid;
				byte state, count;
				f >> refid;
				f >> state;
				f >> count;
				if(!f)
				{
					StreamError("Update client: Broken UPDATE_QUEST_MULTI.");
					break;
				}

				Quest* quest = QuestManager::Get().FindQuest(refid, false);
				if(!quest)
				{
					StreamError("Update client: UPDATE_QUEST_MULTI, missing quest %d.", refid);
					f.SkipStringArray<byte, word>();
					if(!f)
						Error("Update client: Broken UPDATE_QUEST_MULTI(2).");
				}
				else
				{
					quest->state = (Quest::State)state;
					for(byte i = 0; i < count; ++i)
					{
						f.ReadString2(Add1(quest->msgs));
						if(!f)
						{
							StreamError("Update client: Broken UPDATE_QUEST_MULTI(3) on index %u.", i);
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
				f >> id;
				if(!f)
					StreamError("Update client: Broken CHANGE_LEADER.");
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
						Team.leader = info->u;

						if(dialog_enc)
							dialog_enc->bts[0].state = (IsLeader() ? Button::NONE : Button::DISABLED);

						ActivateChangeLeaderButton(IsLeader());
					}
					else
						StreamError("Update client: CHANGE_LEADER, missing player %u.", id);
				}
			}
			break;
		// player get Random number
		case NetChange::RANDOM_NUMBER:
			{
				byte player_id, number;
				f >> player_id;
				f >> number;
				if(!f)
					StreamError("Update client: Broken RANDOM_NUMBER.");
				else if(player_id != my_id)
				{
					PlayerInfo* info = GetPlayerInfoTry(player_id);
					if(info)
						AddMsg(Format(txRolledNumber, info->name.c_str(), number));
					else
						StreamError("Update client: RANDOM_NUMBER, missing player %u.", player_id);
				}
			}
			break;
		// remove player from game
		case NetChange::REMOVE_PLAYER:
			{
				byte player_id;
				PlayerInfo::LeftReason reason;
				f >> player_id;
				f.ReadCasted<byte>(reason);
				if(!f)
					StreamError("Update client: Broken REMOVE_PLAYER.");
				else
				{
					PlayerInfo* info = GetPlayerInfoTry(player_id);
					if(!info)
						StreamError("Update client: REMOVE_PLAYER, missing player %u.", player_id);
					else if(player_id != my_id)
					{
						info->left = reason;
						RemovePlayer(*info);
						game_players.erase(game_players.begin() + GetPlayerIndex(info->id));
						delete info;
					}
				}
			}
			break;
		// unit uses usable object
		case NetChange::USE_USABLE:
			{
				int netid, usable_netid;
				USE_USABLE_STATE state;
				f >> netid;
				f >> usable_netid;
				f.ReadCasted<byte>(state);
				if(!f)
				{
					StreamError("Update client: Broken USE_USABLE.");
					break;
				}

				Unit* unit = FindUnit(netid);
				if(!unit)
				{
					StreamError("Update client: USE_USABLE, missing unit %d.", netid);
					break;
				}

				Usable* usable = FindUsable(usable_netid);
				if(!usable)
				{
					StreamError("Update client: USE_USABLE, missing usable %d.", usable_netid);
					break;
				}

				BaseUsable& base = *usable->base;
				if(state == USE_USABLE_START || state == USE_USABLE_START_SPECIAL)
				{
					if(!IS_SET(base.use_flags, BaseUsable::CONTAINER))
					{
						unit->action = A_ANIMATION2;
						unit->animation = ANI_PLAY;
						unit->mesh_inst->Play(state == USE_USABLE_START_SPECIAL ? "czyta_papiery" : base.anim.c_str(), PLAY_PRIO1, 0);
						unit->mesh_inst->groups[0].speed = 1.f;
						unit->target_pos = unit->pos;
						unit->target_pos2 = usable->pos;
						if(base.limit_rot == 4)
							unit->target_pos2 -= Vec3(sin(usable->rot)*1.5f, 0, cos(usable->rot)*1.5f);
						unit->timer = 0.f;
						unit->animation_state = AS_ANIMATION2_MOVE_TO_OBJECT;
						unit->use_rot = Vec3::LookAtAngle(unit->pos, usable->pos);
						unit->used_item = base.item;
						if(unit->used_item)
						{
							PreloadItem(unit->used_item);
							unit->weapon_taken = W_NONE;
							unit->weapon_state = WS_HIDDEN;
						}
					}
					else
						unit->action = A_NONE;

					unit->UseUsable(usable);
					if(pc_data.before_player == BP_USABLE && pc_data.before_player_ptr.usable == usable)
						pc_data.before_player = BP_NONE;
				}
				else
				{
					if(unit->player != pc && !IS_SET(base.use_flags, BaseUsable::CONTAINER))
					{
						unit->action = A_NONE;
						unit->animation = ANI_STAND;
						if(unit->live_state == Unit::ALIVE)
							unit->used_item = nullptr;
					}

					if(usable->user != unit)
					{
						StreamError("Update client: USE_USABLE, unit %d not using %d.", netid, usable_netid);
						unit->usable = nullptr;
					}
					else if(state == USE_USABLE_END)
						unit->UseUsable(nullptr);
				}
			}
			break;
		// unit stands up
		case NetChange::STAND_UP:
			{
				int netid;
				f >> netid;
				if(!f)
					StreamError("Update client: Broken STAND_UP.");
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
						StreamError("Update client: STAND_UP, missing unit %d.", netid);
					else
						UnitStandup(*unit);
				}
			}
			break;
		// game over
		case NetChange::GAME_OVER:
			Info("Update client: Game over - all players died.");
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
				f >> netid;
				f >> free;
				if(!f)
					StreamError("Update client: Broken RECRUIT_NPC.");
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
						StreamError("Update client: RECRUIT_NPC, missing unit %d.", netid);
					else if(!unit->IsHero())
						StreamError("Update client: RECRUIT_NPC, unit %d (%s) is not a hero.", netid, unit->data->id.c_str());
					else
					{
						unit->hero->team_member = true;
						unit->hero->free = free;
						unit->hero->credit = 0;
						Team.members.push_back(unit);
						if(!free)
							Team.active_members.push_back(unit);
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
				f >> netid;
				if(!f)
					StreamError("Update client: Broken KICK_NPC.");
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
						StreamError("Update client: KICK_NPC, missing unit %d.", netid);
					else if(!unit->IsHero() || !unit->hero->team_member)
						StreamError("Update client: KICK_NPC, unit %d (%s) is not a team member.", netid, unit->data->id.c_str());
					else
					{
						unit->hero->team_member = false;
						RemoveElement(Team.members, unit);
						if(!unit->hero->free)
							RemoveElement(Team.active_members, unit);
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
				f >> netid;
				if(!f)
					StreamError("Update client: Broken REMOVE_UNIT.");
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
						StreamError("Update client: REMOVE_UNIT, missing unit %d.", netid);
					else
						RemoveUnit(unit);
				}
			}
			break;
		// spawn new unit
		case NetChange::SPAWN_UNIT:
			{
				Unit* unit = new Unit;
				if(!ReadUnit(f, *unit))
				{
					StreamError("Update client: Broken SPAWN_UNIT.");
					delete unit;
				}
				else
				{
					LevelContext& ctx = GetContext(unit->pos);
					ctx.units->push_back(unit);
					unit->in_building = ctx.building_id;
					if(unit->summoner != nullptr)
						SpawnUnitEffect(*unit);
				}
			}
			break;
		// change unit arena state
		case NetChange::CHANGE_ARENA_STATE:
			{
				int netid;
				char state;
				f >> netid;
				f >> state;
				if(!f)
					StreamError("Update client: Broken CHANGE_ARENA_STATE.");
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
						StreamError("Update client: CHANGE_ARENA_STATE, missing unit %d.", netid);
					else
					{
						if(state < -1 || state > 1)
							state = -1;
						unit->in_arena = state;
						if(unit == pc->unit && state >= 0)
							pc->RefreshCooldown();
					}
				}
			}
			break;
		// plays arena sound
		case NetChange::ARENA_SOUND:
			{
				byte type;
				f >> type;
				if(!f)
					StreamError("Update client: Broken ARENA_SOUND.");
				else if(city_ctx && IS_SET(city_ctx->flags, City::HaveArena) && GetArena()->ctx.building_id == pc->unit->in_building)
				{
					SOUND snd;
					if(type == 0)
						snd = sArenaFight;
					else if(type == 1)
						snd = sArenaWin;
					else
						snd = sArenaLost;
					sound_mgr->PlaySound2d(snd);
				}
			}
			break;
		// unit shout after seeing enemy
		case NetChange::SHOUT:
			{
				int netid;
				f >> netid;
				if(!f)
					StreamError("Update client: Broken SHOUT.");
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
						StreamError("Update client: SHOUT, missing unit %d.", netid);
					else if(unit->data->sounds->sound[SOUND_SEE_ENEMY])
						PlayAttachedSound(*unit, unit->data->sounds->sound[SOUND_SEE_ENEMY]->sound, 3.f, 20.f);
				}
			}
			break;
		// leaving notification
		case NetChange::LEAVE_LOCATION:
			fallback_co = FALLBACK::WAIT_FOR_WARP;
			fallback_t = -1.f;
			pc->unit->frozen = FROZEN::YES;
			break;
		// exit to map
		case NetChange::EXIT_TO_MAP:
			ExitToMap();
			break;
		// leader wants to travel to location
		case NetChange::TRAVEL:
			{
				byte loc;
				f >> loc;
				if(!f)
					StreamError("Update client: Broken TRAVEL.");
				else
				{
					world_state = WS_TRAVEL;
					current_location = -1;
					travel_time = 0.f;
					travel_day = 0;
					travel_start = world_pos;
					picked_location = loc;
					Location& l = *locations[picked_location];
					world_dir = Angle(world_pos.x, -world_pos.y, l.pos.x, -l.pos.y);
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
		// leader finished travel
		case NetChange::END_TRAVEL:
			if(world_state == WS_TRAVEL)
			{
				world_state = WS_MAIN;
				current_location = picked_location;
				Location& loc = *locations[current_location];
				if(loc.state == LS_KNOWN)
					SetLocationVisited(loc);
				world_pos = loc.pos;
			}
			break;
		// change world time
		case NetChange::WORLD_TIME:
			{
				int new_worldtime;
				byte new_day, new_month, new_year;
				f >> new_worldtime;
				f >> new_day;
				f >> new_month;
				f >> new_year;
				if(!f)
					StreamError("Update client: Broken WORLD_TIME.");
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
				f >> netid;
				f >> is_closing;
				if(!f)
					StreamError("Update client: Broken USE_DOOR.");
				else
				{
					Door* door = FindDoor(netid);
					if(!door)
						StreamError("Update client: USE_DOOR, missing door %d.", netid);
					else
					{
						bool ok = true;
						if(is_closing)
						{
							// closing door
							if(door->state == Door::Open)
							{
								door->state = Door::Closing;
								door->mesh_inst->Play(&door->mesh_inst->mesh->anims[0], PLAY_ONCE | PLAY_STOP_AT_END | PLAY_NO_BLEND | PLAY_BACK, 0);
								door->mesh_inst->frame_end_info = false;
							}
							else if(door->state == Door::Opening)
							{
								door->state = Door::Closing2;
								door->mesh_inst->Play(&door->mesh_inst->mesh->anims[0], PLAY_ONCE | PLAY_STOP_AT_END | PLAY_BACK, 0);
								door->mesh_inst->frame_end_info = false;
							}
							else if(door->state == Door::Opening2)
							{
								door->state = Door::Closing;
								door->mesh_inst->Play(&door->mesh_inst->mesh->anims[0], PLAY_ONCE | PLAY_STOP_AT_END | PLAY_BACK, 0);
								door->mesh_inst->frame_end_info = false;
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
								door->mesh_inst->Play(&door->mesh_inst->mesh->anims[0], PLAY_ONCE | PLAY_STOP_AT_END | PLAY_NO_BLEND, 0);
								door->mesh_inst->frame_end_info = false;
							}
							else if(door->state == Door::Closing)
							{
								door->locked = LOCK_NONE;
								door->state = Door::Opening2;
								door->mesh_inst->Play(&door->mesh_inst->mesh->anims[0], PLAY_ONCE | PLAY_STOP_AT_END, 0);
								door->mesh_inst->frame_end_info = false;
							}
							else if(door->state == Door::Closing2)
							{
								door->locked = LOCK_NONE;
								door->state = Door::Opening;
								door->mesh_inst->Play(&door->mesh_inst->mesh->anims[0], PLAY_ONCE | PLAY_STOP_AT_END, 0);
								door->mesh_inst->frame_end_info = false;
							}
							else
								ok = false;
						}

						if(ok && Rand() == 0)
						{
							SOUND snd;
							if(is_closing && Rand() % 2 == 0)
								snd = sDoorClose;
							else
								snd = sDoor[Rand() % 3];
							sound_mgr->PlaySound3d(snd, door->GetCenter(), 2.f, 5.f);
						}
					}
				}
			}
			break;
		// chest opening animation
		case NetChange::CHEST_OPEN:
			{
				int netid;
				f >> netid;
				if(!f)
					StreamError("Update client: Broken CHEST_OPEN.");
				else
				{
					Chest* chest = FindChest(netid);
					if(!chest)
						StreamError("Update client: CHEST_OPEN, missing chest %d.", netid);
					else
					{
						chest->mesh_inst->Play(&chest->mesh_inst->mesh->anims[0], PLAY_PRIO1 | PLAY_ONCE | PLAY_STOP_AT_END, 0);
						sound_mgr->PlaySound3d(sChestOpen, chest->GetCenter(), 2.f, 5.f);
					}
				}
			}
			break;
		// chest closing animation
		case NetChange::CHEST_CLOSE:
			{
				int netid;
				f >> netid;
				if(!f)
					StreamError("Update client: Broken CHEST_CLOSE.");
				else
				{
					Chest* chest = FindChest(netid);
					if(!chest)
						StreamError("Update client: CHEST_CLOSE, missing chest %d.", netid);
					else
					{
						chest->mesh_inst->Play(&chest->mesh_inst->mesh->anims[0], PLAY_PRIO1 | PLAY_ONCE | PLAY_STOP_AT_END | PLAY_BACK, 0);
						sound_mgr->PlaySound3d(sChestClose, chest->GetCenter(), 2.f, 5.f);
					}
				}
			}
			break;
		// create explosion effect
		case NetChange::CREATE_EXPLOSION:
			{
				Vec3 pos;
				const string& spell_id = f.ReadString1();
				f >> pos;
				if(!f)
				{
					StreamError("Update client: Broken CREATE_EXPLOSION.");
					break;
				}

				Spell* spell_ptr = FindSpell(spell_id.c_str());
				if(!spell_ptr)
				{
					StreamError("Update client: CREATE_EXPLOSION, missing spell '%s'.", spell_id.c_str());
					break;
				}

				Spell& spell = *spell_ptr;
				if(!IS_SET(spell.flags, Spell::Explode))
				{
					StreamError("Update client: CREATE_EXPLOSION, spell '%s' is not explosion.", spell_id.c_str());
					break;
				}

				Explo* explo = new Explo;
				explo->pos = pos;
				explo->size = 0.f;
				explo->sizemax = 2.f;
				explo->tex = spell.tex_explode;
				explo->owner = nullptr;

				sound_mgr->PlaySound3d(spell.sound_hit, explo->pos, spell.sound_hit_dist.x, spell.sound_hit_dist.y);

				GetContext(pos).explos->push_back(explo);
			}
			break;
		// remove trap
		case NetChange::REMOVE_TRAP:
			{
				int netid;
				f >> netid;
				if(!f)
					StreamError("Update client: Broken REMOVE_TRAP.");
				else if(!RemoveTrap(netid))
					StreamError("Update client: REMOVE_TRAP, missing trap %d.", netid);
			}
			break;
		// trigger trap
		case NetChange::TRIGGER_TRAP:
			{
				int netid;
				f >> netid;
				if(!f)
					StreamError("Update client: Broken TRIGGER_TRAP.");
				else
				{
					Trap* trap = FindTrap(netid);
					if(trap)
						trap->trigger = true;
					else
						StreamError("Update client: TRIGGER_TRAP, missing trap %d.", netid);
				}
			}
			break;
		// play evil sound
		case NetChange::EVIL_SOUND:
			sound_mgr->PlaySound2d(sEvil);
			break;
		// start encounter on world map
		case NetChange::ENCOUNTER:
			{
				const string& text = f.ReadString1();
				if(!f)
					StreamError("Update client: Broken ENCOUNTER.");
				else
				{
					DialogInfo info;
					info.event = DialogEvent(this, &Game::Event_StartEncounter);
					info.name = "encounter";
					info.order = ORDER_TOP;
					info.parent = nullptr;
					info.pause = true;
					info.type = DIALOG_OK;
					info.text = text;

					dialog_enc = GUI.ShowDialog(info);
					if(!IsLeader())
						dialog_enc->bts[0].state = Button::DISABLED;
					assert(world_state == WS_TRAVEL);
					world_state = WS_ENCOUNTER;
				}
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
				StreamError("Update client: CLOSE_PORTAL, missing portal.");
			break;
		// clean altar in evil quest
		case NetChange::CLEAN_ALTAR:
			{
				// change object
				BaseObject* base_obj = BaseObject::Get("bloody_altar");
				Object* obj = local_ctx.FindObject(base_obj);
				obj->base = BaseObject::Get("altar");
				obj->mesh = obj->base->mesh;
				ResourceManager::Get<Mesh>().Load(obj->mesh);

				// remove particles
				float best_dist = 999.f;
				ParticleEmitter* pe = nullptr;
				for(vector<ParticleEmitter*>::iterator it = local_ctx.pes->begin(), end = local_ctx.pes->end(); it != end; ++it)
				{
					if((*it)->tex == tKrew[BLOOD_RED])
					{
						float dist = Vec3::Distance((*it)->pos, obj->pos);
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
				f >> location_index;
				f.ReadCasted<byte>(type);
				if(!f)
				{
					StreamError("Update client: Broken ADD_LOCATION.");
					break;
				}

				Location* loc;
				if(type == L_DUNGEON || type == L_CRYPT)
				{
					byte levels;
					f >> levels;
					if(!f)
					{
						StreamError("Update client: Broken ADD_LOCATION(2).");
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

				f.ReadCasted<byte>(loc->state);
				f >> loc->pos;
				f >> loc->name;
				f.ReadCasted<byte>(loc->image);
				if(!f)
				{
					StreamError("Update client: Broken ADD_LOCATION(3).");
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
				f >> camp_index;
				if(!f)
					StreamError("Update client: Broken REMOVE_CAMP.");
				else if(camp_index >= locations.size() || !locations[camp_index] || locations[camp_index]->type != L_CAMP)
					StreamError("Update client: REMOVE_CAMP, invalid location %u.", camp_index);
				else
				{
					delete locations[camp_index];
					if(camp_index == locations.size() - 1)
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
				f >> netid;
				f >> mode;
				if(!f)
					StreamError("Update client: Broken CHANGE_AI_MODE.");
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
						StreamError("Update client: CHANGE_AI_MODE, missing unit %d.", netid);
					else
						unit->ai_mode = mode;
				}
			}
			break;
		// change unit base type
		case NetChange::CHANGE_UNIT_BASE:
			{
				int netid;
				f >> netid;
				const string& unit_id = f.ReadString1();
				if(!f)
					StreamError("Update client: Broken CHANGE_UNIT_BASE.");
				else
				{
					Unit* unit = FindUnit(netid);
					UnitData* ud = UnitData::TryGet(unit_id);
					if(unit && ud)
						unit->data = ud;
					else if(!unit)
						StreamError("Update client: CHANGE_UNIT_BASE, missing unit %d.", netid);
					else
						StreamError("Update client: CHANGE_UNIT_BASE, missing base unit '%s'.", unit_id.c_str());
				}
			}
			break;
		// unit casts spell
		case NetChange::CAST_SPELL:
			{
				int netid;
				byte attack_id;
				f >> netid;
				f >> attack_id;
				if(!f)
					StreamError("Update client: Broken CAST_SPELL.");
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
						StreamError("Update client: CAST_SPELL, missing unit %d.", netid);
					else
					{
						unit->action = A_CAST;
						unit->attack_id = attack_id;
						unit->animation_state = 0;

						if(unit->mesh_inst->mesh->head.n_groups == 2)
						{
							unit->mesh_inst->frame_end_info2 = false;
							unit->mesh_inst->Play("cast", PLAY_ONCE | PLAY_PRIO1, 1);
						}
						else
						{
							unit->mesh_inst->frame_end_info = false;
							unit->animation = ANI_PLAY;
							unit->mesh_inst->Play("cast", PLAY_ONCE | PLAY_PRIO1, 0);
							unit->mesh_inst->groups[0].speed = 1.f;
						}
					}
				}
			}
			break;
		// create ball - spell effect
		case NetChange::CREATE_SPELL_BALL:
			{
				int netid;
				Vec3 pos;
				float rotY, speedY;
				const string& spell_id = f.ReadString1();
				f >> pos;
				f >> rotY;
				f >> speedY;
				f >> netid;
				if(!f)
				{
					StreamError("Update client: Broken CREATE_SPELL_BALL.");
					break;
				}

				Spell* spell_ptr = FindSpell(spell_id.c_str());
				if(!spell_ptr)
				{
					StreamError("Update client: CREATE_SPELL_BALL, missing spell '%s'.", spell_id.c_str());
					break;
				}

				Unit* unit = nullptr;
				if(netid != -1)
				{
					unit = FindUnit(netid);
					if(!unit)
						StreamError("Update client: CREATE_SPELL_BALL, missing unit %d.", netid);
				}

				Spell& spell = *spell_ptr;
				LevelContext& ctx = GetContext(pos);

				Bullet& b = Add1(ctx.bullets);

				b.pos = pos;
				b.rot = Vec3(0, rotY, 0);
				b.mesh = spell.mesh;
				b.tex = spell.tex;
				b.tex_size = spell.size;
				b.speed = spell.speed;
				b.timer = spell.range / (spell.speed - 1);
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
					pe->speed_min = Vec3(-1, -1, -1);
					pe->speed_max = Vec3(1, 1, 1);
					pe->pos_min = Vec3(-spell.size, -spell.size, -spell.size);
					pe->pos_max = Vec3(spell.size, spell.size, spell.size);
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
				Vec3 pos;
				const string& spell_id = f.ReadString1();
				f >> pos;
				if(!f)
				{
					StreamError("Update client: Broken SPELL_SOUND.");
					break;
				}

				Spell* spell_ptr = FindSpell(spell_id.c_str());
				if(!spell_ptr)
				{
					StreamError("Update client: SPELL_SOUND, missing spell '%s'.", spell_id.c_str());
					break;
				}

				Spell& spell = *spell_ptr;
				sound_mgr->PlaySound3d(spell.sound_cast, pos, spell.sound_cast_dist.x, spell.sound_cast_dist.y);
			}
			break;
		// drain blood effect
		case NetChange::CREATE_DRAIN:
			{
				int netid;
				f >> netid;
				if(!f)
					StreamError("Update client: Broken CREATE_DRAIN.");
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
						StreamError("Update client: CREATE_DRAIN, missing unit %d.", netid);
					else
					{
						LevelContext& ctx = GetContext(*unit);
						if(ctx.pes->empty())
							StreamError("Update client: CREATE_DRAIN, missing blood.");
						else
						{
							Drain& drain = Add1(ctx.drains);
							drain.from = nullptr;
							drain.to = unit;
							drain.pe = ctx.pes->back();
							drain.t = 0.f;
							drain.pe->manual_delete = 1;
							drain.pe->speed_min = Vec3(-3, 0, -3);
							drain.pe->speed_max = Vec3(3, 3, 3);
						}
					}
				}
			}
			break;
		// create electro effect
		case NetChange::CREATE_ELECTRO:
			{
				int netid;
				Vec3 p1, p2;
				f >> netid;
				f >> p1;
				f >> p2;
				if(!f)
					StreamError("Update client: Broken CREATE_ELECTRO.");
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
				Vec3 pos;
				f >> netid;
				f >> pos;
				if(!f)
					StreamError("Update client: Broken UPDATE_ELECTRO.");
				else
				{
					Electro* e = FindElectro(netid);
					if(!e)
						StreamError("Update client: UPDATE_ELECTRO, missing electro %d.", netid);
					else
					{
						Vec3 from = e->lines.back().pts.back();
						e->AddLine(from, pos);
					}
				}
			}
			break;
		// electro hit effect
		case NetChange::ELECTRO_HIT:
			{
				Vec3 pos;
				f >> pos;
				if(!f)
					StreamError("Update client: Broken ELECTRO_HIT.");
				else
				{
					Spell* spell = FindSpell("thunder_bolt");

					// sound
					if(spell->sound_hit)
						sound_mgr->PlaySound3d(spell->sound_hit, pos, spell->sound_hit_dist.x, spell->sound_hit_dist.y);

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
						pe->speed_min = Vec3(-1.5f, -1.5f, -1.5f);
						pe->speed_max = Vec3(1.5f, 1.5f, 1.5f);
						pe->pos_min = Vec3(-spell->size, -spell->size, -spell->size);
						pe->pos_max = Vec3(spell->size, spell->size, spell->size);
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
				Vec3 pos;
				f >> pos;
				if(!f)
					StreamError("Update client: Broken RAISE_EFFECT.");
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
					pe->speed_min = Vec3(-1.5f, -1.5f, -1.5f);
					pe->speed_max = Vec3(1.5f, 1.5f, 1.5f);
					pe->pos_min = Vec3(-spell.size, -spell.size, -spell.size);
					pe->pos_max = Vec3(spell.size, spell.size, spell.size);
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
				Vec3 pos;
				f >> pos;
				if(!f)
					StreamError("Update client: Broken HEAL_EFFECT.");
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
					pe->speed_min = Vec3(-1.5f, -1.5f, -1.5f);
					pe->speed_max = Vec3(1.5f, 1.5f, 1.5f);
					pe->pos_min = Vec3(-spell.size, -spell.size, -spell.size);
					pe->pos_max = Vec3(spell.size, spell.size, spell.size);
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
				f >> count;
				if(!f.Ensure(count * sizeof(byte) * 2))
					StreamError("Update client: Broken REVEAL_MINIMAP.");
				else
				{
					for(word i = 0; i < count; ++i)
					{
						byte x, y;
						f.Read(x);
						f.Read(y);
						minimap_reveal.push_back(Int2(x, y));
					}
				}
			}
			break;
		// 'noai' cheat change
		case NetChange::CHEAT_NOAI:
			{
				bool state;
				f >> state;
				if(!f)
					StreamError("Update client: Broken CHEAT_NOAI.");
				else
					noai = state;
			}
			break;
		// end of game, time run out
		case NetChange::END_OF_GAME:
			Info("Update client: Game over - time run out.");
			CloseAllPanels(true);
			koniec_gry = true;
			death_fade = 0.f;
			exit_from_server = true;
			break;
		// update players free days
		case NetChange::UPDATE_FREE_DAYS:
			{
				byte count;
				f >> count;
				if(!f.Ensure(sizeof(int) * 2 * count))
					StreamError("Update client: Broken UPDATE_FREE_DAYS.");
				else
				{
					for(byte i = 0; i < count; ++i)
					{
						int netid, days;
						f >> netid;
						f >> days;

						Unit* unit = FindUnit(netid);
						if(!unit || !unit->IsPlayer())
						{
							StreamError("Update client: UPDATE_FREE_DAYS, missing unit %d or is not a player (0x%p).", netid, unit);
							f.Skip(sizeof(int) * 2 * (count - i - 1));
							break;
						}

						unit->player->free_days = days;
					}
				}
			}
			break;
		// multiplayer vars changed
		case NetChange::CHANGE_MP_VARS:
			ReadNetVars(f);
			if(!f)
				StreamError("Update client: Broken CHANGE_MP_VARS.");
			break;
		// game saved notification
		case NetChange::GAME_SAVED:
			AddMultiMsg(txGameSaved);
			AddGameMsg3(GMS_GAME_SAVED);
			break;
		// ai left team due too many team members
		case NetChange::HERO_LEAVE:
			{
				int netid;
				f >> netid;
				if(!f)
					StreamError("Update client: Broken HERO_LEAVE.");
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
						StreamError("Update client: HERO_LEAVE, missing unit %d.", netid);
					else
						AddMultiMsg(Format(txMpNPCLeft, unit->GetName()));
				}
			}
			break;
		// game paused/resumed
		case NetChange::PAUSED:
			{
				bool is_paused;
				f >> is_paused;
				if(!f)
					StreamError("Update client: Broken PAUSED.");
				else
				{
					paused = is_paused;
					AddMultiMsg(paused ? txGamePaused : txGameResumed);
				}
			}
			break;
		// secret letter text update
		case NetChange::SECRET_TEXT:
			f >> GetSecretNote()->desc;
			if(!f)
				StreamError("Update client: Broken SECRET_TEXT.");
			break;
		// update position on world map
		case NetChange::UPDATE_MAP_POS:
			{
				Vec2 pos;
				f >> pos;
				if(!f)
					StreamError("Update client: Broken UPDATE_MAP_POS.");
				else
					world_pos = pos;
			}
			break;
		// player used cheat for fast travel on map
		case NetChange::CHEAT_TRAVEL:
			{
				byte location_index;
				f >> location_index;
				if(!f)
					StreamError("Update client: Broken CHEAT_TRAVEL.");
				else if(location_index >= locations.size() || !locations[location_index])
					StreamError("Update client: CHEAT_TRAVEL, invalid location index %u.", location_index);
				else
				{
					current_location = location_index;
					Location& loc = *locations[current_location];
					if(loc.state == LS_KNOWN)
						loc.state = LS_VISITED;
					world_pos = loc.pos;
					if(open_location != -1)
					{
						LeaveLocation(false, false);
						open_location = -1;
					}
				}
			}
			break;
		// remove used item from unit hand
		case NetChange::REMOVE_USED_ITEM:
			{
				int netid;
				f >> netid;
				if(!f)
					StreamError("Update client: Broken REMOVE_USED_ITEM.");
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
						StreamError("Update client: REMOVE_USED_ITEM, missing unit %d.", netid);
					else
						unit->used_item = nullptr;
				}
			}
			break;
		// game stats showed at end of game
		case NetChange::GAME_STATS:
			f >> total_kills;
			if(!f)
				StreamError("Update client: Broken GAME_STATS.");
			break;
		// play usable object sound for unit
		case NetChange::USABLE_SOUND:
			{
				int netid;
				f >> netid;
				if(!f)
					StreamError("Update client: Broken USABLE_SOUND.");
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
						StreamError("Update client: USABLE_SOUND, missing unit %d.", netid);
					else if(!unit->usable)
						StreamError("Update client: USABLE_SOUND, unit %d (%s) don't use usable.", netid, unit->data->id.c_str());
					else if(unit != pc->unit)
						sound_mgr->PlaySound3d(unit->usable->base->sound->sound, unit->GetCenter(), 2.f, 5.f);
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
				f >> netid;
				if(!f)
					StreamError("Update client: Broken BREAK_ACTION.");
				else
				{
					Unit* unit = FindUnit(netid);
					if(unit)
						BreakUnitAction(*unit);
					else
						StreamError("Update client: BREAK_ACTION, missing unit %d.", netid);
				}
			}
			break;
		// player used action
		case NetChange::PLAYER_ACTION:
			{
				int netid;
				f >> netid;
				if(!f)
					StreamError("Update client: Broken PLAYER_ACTION.");
				else
				{
					Unit* unit = FindUnit(netid);
					if(unit && unit->player)
					{
						if(unit->player != pc)
							UseAction(unit->player, true);
					}
					else
						StreamError("Update client: PLAYER_ACTION, invalid player unit %d.", netid);
				}
			}
			break;
		// unit stun - not shield bash
		case NetChange::STUN:
			{
				int netid;
				float length;
				f >> netid;
				f >> length;
				if(!f)
					StreamError("Update client: Broken STUN.");
				else
				{
					Unit* unit = FindUnit(netid);
					if(!unit)
						StreamError("Update client: STUN, missing unit %d.", netid);
					else
					{
						if(length > 0)
							unit->ApplyStun(length);
						else
							unit->RemoveEffect(EffectId::Stun);
					}
				}
			}
			break;
		// invalid change
		default:
			Warn("Update client: Unknown change type %d.", type);
			StreamError();
			break;
		}

		byte checksum;
		f >> checksum;
		if(!f || checksum != 0xFF)
		{
			StreamError("Update client: Invalid checksum (%u).", change_i);
			return true;
		}
	}

	return true;
}

//=================================================================================================
bool Game::ProcessControlMessageClientForMe(BitStreamReader& f)
{
	// flags
	byte flags;
	f >> flags;
	if(!f)
	{
		StreamError("Update single client: Broken ID_PLAYER_CHANGES.");
		return true;
	}

	// last damage from poison
	if(IS_SET(flags, PlayerInfo::UF_POISON_DAMAGE))
	{
		f >> pc->last_dmg_poison;
		if(!f)
		{
			StreamError("Update single client: Broken ID_PLAYER_CHANGES at UF_POISON_DAMAGE.");
			return true;
		}
	}

	// changes
	if(IS_SET(flags, PlayerInfo::UF_NET_CHANGES))
	{
		byte changes;
		f >> changes;
		if(!f)
		{
			StreamError("Update single client: Broken ID_PLAYER_CHANGES at UF_NET_CHANGES.");
			return true;
		}

		for(byte change_i = 0; change_i < changes; ++change_i)
		{
			NetChangePlayer::TYPE type;
			f.ReadCasted<byte>(type);
			if(!f)
			{
				StreamError("Update single client: Broken ID_PLAYER_CHANGES at UF_NET_CHANGES(2).");
				return true;
			}

			switch(type)
			{
			// item picked up
			case NetChangePlayer::PICKUP:
				{
					int count, team_count;
					f >> count;
					f >> team_count;
					if(!f)
						StreamError("Update single client: Broken PICKUP.");
					else
					{
						AddItem(*pc->unit, pc_data.picking_item->item, (uint)count, (uint)team_count);
						if(pc_data.picking_item->item->type == IT_GOLD)
							sound_mgr->PlaySound2d(sCoins);
						if(pc_data.picking_item_state == 2)
							delete pc_data.picking_item;
						pc_data.picking_item_state = 0;
					}
				}
				break;
			// response to looting
			case NetChangePlayer::LOOT:
				{
					bool can_loot;
					f >> can_loot;
					if(!f)
					{
						StreamError("Update single client: Broken LOOT.");
						break;
					}

					if(!can_loot)
					{
						AddGameMsg3(GMS_IS_LOOTED);
						pc->action = PlayerController::Action_None;
						break;
					}

					// read items
					if(!ReadItemListTeam(f, *pc->chest_trade))
					{
						StreamError("Update single client: Broken LOOT(2).");
						break;
					}

					// start trade
					if(pc->action == PlayerController::Action_LootUnit)
						StartTrade(I_LOOT_BODY, *pc->action_unit);
					else if(pc->action == PlayerController::Action_LootContainer)
						StartTrade(I_LOOT_CONTAINER, pc->action_container->container->items);
					else
						StartTrade(I_LOOT_CHEST, pc->action_chest->items);
				}
				break;
			// message about gained gold
			case NetChangePlayer::GOLD_MSG:
				{
					bool default_msg;
					int count;
					f >> default_msg;
					f >> count;
					if(!f)
						StreamError("Update single client: Broken GOLD_MSG.");
					else
					{
						if(default_msg)
							AddGameMsg(Format(txGoldPlus, count), 3.f);
						else
							AddGameMsg(Format(txQuestCompletedGold, count), 4.f);
						sound_mgr->PlaySound2d(sCoins);
					}
				}
				break;
			// start dialog with unit or is busy
			case NetChangePlayer::START_DIALOG:
				{
					int netid;
					f >> netid;
					if(!f)
						StreamError("Update single client: Broken START_DIALOG.");
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
							StreamError("Update single client: START_DIALOG, missing unit %d.", netid);
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
							pc_data.before_player = BP_NONE;
						}
					}
				}
				break;
			// show dialog choices
			case NetChangePlayer::SHOW_DIALOG_CHOICES:
				{
					byte count;
					char escape;
					f >> count;
					f >> escape;
					if(!f.Ensure(count))
						StreamError("Update single client: Broken SHOW_DIALOG_CHOICES.");
					else
					{
						dialog_context.choice_selected = 0;
						dialog_context.show_choices = true;
						dialog_context.dialog_esc = escape;
						dialog_choices.resize(count);
						for(byte i = 0; i < count; ++i)
						{
							f >> dialog_choices[i];
							if(!f)
							{
								StreamError("Update single client: Broken SHOW_DIALOG_CHOICES at %u index.", i);
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
					f >> netid;
					if(!ReadItemList(f, chest_trade))
					{
						StreamError("Update single client: Broken START_TRADE.");
						break;
					}

					Unit* trader = FindUnit(netid);
					if(!trader)
					{
						StreamError("Update single client: START_TRADE, missing unit %d.", netid);
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
					f >> team_count;
					f >> count;
					if(ReadItemAndFind(f, item) <= 0)
						StreamError("Update single client: Broken ADD_ITEMS.");
					else if(count <= 0 || team_count > count)
						StreamError("Update single client: ADD_ITEMS, invalid count %d or team count %d.", count, team_count);
					else
					{
						PreloadItem(item);
						AddItem(*pc->unit, item, (uint)count, (uint)team_count);
					}
				}
				break;
			// add items to trader which is trading with player
			case NetChangePlayer::ADD_ITEMS_TRADER:
				{
					int netid, count, team_count;
					const Item* item;
					f >> netid;
					f >> count;
					f >> team_count;
					if(ReadItemAndFind(f, item) <= 0)
						StreamError("Update single client: Broken ADD_ITEMS_TRADER.");
					else if(count <= 0 || team_count > count)
						StreamError("Update single client: ADD_ITEMS_TRADER, invalid count %d or team count %d.", count, team_count);
					else
					{
						Unit* unit = FindUnit(netid);
						if(!unit)
							StreamError("Update single client: ADD_ITEMS_TRADER, missing unit %d.", netid);
						else if(!pc->IsTradingWith(unit))
							StreamError("Update single client: ADD_ITEMS_TRADER, unit %d (%s) is not trading with player.", netid, unit->data->id.c_str());
						else
						{
							PreloadItem(item);
							AddItem(*unit, item, (uint)count, (uint)team_count);
						}
					}
				}
				break;
			// add items to chest which is open by player
			case NetChangePlayer::ADD_ITEMS_CHEST:
				{
					int netid, count, team_count;
					const Item* item;
					f >> netid;
					f >> count;
					f >> team_count;
					if(ReadItemAndFind(f, item) <= 0)
						StreamError("Update single client: Broken ADD_ITEMS_CHEST.");
					else if(count <= 0 || team_count > count)
						StreamError("Update single client: ADD_ITEMS_CHEST, invalid count %d or team count %d.", count, team_count);
					else
					{
						Chest* chest = FindChest(netid);
						if(!chest)
							StreamError("Update single client: ADD_ITEMS_CHEST, missing chest %d.", netid);
						else if(pc->action != PlayerController::Action_LootChest || pc->action_chest != chest)
							StreamError("Update single client: ADD_ITEMS_CHEST, chest %d is not opened by player.", netid);
						else
						{
							PreloadItem(item);
							AddItem(*chest, item, (uint)count, (uint)team_count);
						}
					}
				}
				break;
			// remove items from inventory
			case NetChangePlayer::REMOVE_ITEMS:
				{
					int i_index, count;
					f >> i_index;
					f >> count;
					if(!f)
						StreamError("Update single client: Broken REMOVE_ITEMS.");
					else if(count <= 0)
						StreamError("Update single client: REMOVE_ITEMS, invalid count %d.", count);
					else
						RemoveItem(*pc->unit, i_index, count);
				}
				break;
			// remove items from traded inventory which is trading with player
			case NetChangePlayer::REMOVE_ITEMS_TRADER:
				{
					int netid, i_index, count;
					f >> netid;
					f >> count;
					f >> i_index;
					if(!f)
						StreamError("Update single client: Broken REMOVE_ITEMS_TRADER.");
					else if(count <= 0)
						StreamError("Update single client: REMOVE_ITEMS_TRADE, invalid count %d.", count);
					else
					{
						Unit* unit = FindUnit(netid);
						if(!unit)
							StreamError("Update single client: REMOVE_ITEMS_TRADER, missing unit %d.", netid);
						else if(!pc->IsTradingWith(unit))
							StreamError("Update single client: REMOVE_ITEMS_TRADER, unit %d (%s) is not trading with player.", netid, unit->data->id.c_str());
						else
							RemoveItem(*unit, i_index, count);
					}
				}
				break;
			// change player frozen state
			case NetChangePlayer::SET_FROZEN:
				f.ReadCasted<byte>(pc->unit->frozen);
				if(!f)
					StreamError("Update single client: Broken SET_FROZEN.");
				break;
			// remove quest item from inventory
			case NetChangePlayer::REMOVE_QUEST_ITEM:
				{
					int refid;
					f >> refid;
					if(!f)
						StreamError("Update single client: Broken REMOVE_QUEST_ITEM.");
					else
						pc->unit->RemoveQuestItem(refid);
				}
				break;
			// someone else is using usable
			case NetChangePlayer::USE_USABLE:
				AddGameMsg3(GMS_USED);
				if(pc->action == PlayerController::Action_LootContainer)
					pc->action = PlayerController::Action_None;
				if(pc->unit->action == A_PREPARE)
					pc->unit->action = A_NONE;
				break;
			// change development mode for player
			case NetChangePlayer::DEVMODE:
				{
					bool allowed;
					f >> allowed;
					if(!f)
						StreamError("Update single client: Broken CHEATS.");
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
						f >> unit.weight;
						f >> unit.weight_max;
						f >> unit.gold;
						unit.stats.Read(f);
						if(!ReadItemListTeam(f, unit.items))
							StreamError("Update single client: Broken %s.", name);
						else
							StartTrade(type == NetChangePlayer::START_SHARE ? I_SHARE : I_GIVE, unit);
					}
					else
					{
						StreamError("Update single client: %s, player is not talking with team member.", name);
						// try to skip
						UnitStats stats;
						vector<ItemSlot> items;
						f.Skip(sizeof(int) * 3);
						stats.Read(f);
						if(!ReadItemListTeam(f, items, true))
							Error("Update single client: Broken %s(2).", name);
					}
				}
				break;
			// response to is IS_BETTER_ITEM
			case NetChangePlayer::IS_BETTER_ITEM:
				{
					bool is_better;
					f >> is_better;
					if(!f)
						StreamError("Update single client: Broken IS_BETTER_ITEM.");
					else
						game_gui->inv_trade_mine->IsBetterItemResponse(is_better);
				}
				break;
			// question about pvp
			case NetChangePlayer::PVP:
				{
					byte player_id;
					f >> player_id;
					if(!f)
						StreamError("Update single client: Broken PVP.");
					else
					{
						PlayerInfo* info = GetPlayerInfoTry(player_id);
						if(!info)
							StreamError("Update single client: PVP, invalid player id %u.", player_id);
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
				fallback_co = FALLBACK::WAIT_FOR_WARP;
				fallback_t = -1.f;
				pc->unit->frozen = (pc->unit->usable ? FROZEN::YES_NO_ANIM : FROZEN::YES);
				break;
			// entering arena
			case NetChangePlayer::ENTER_ARENA:
				fallback_co = FALLBACK::ARENA;
				fallback_t = -1.f;
				pc->unit->frozen = FROZEN::YES;
				break;
			// start of arena combat
			case NetChangePlayer::START_ARENA_COMBAT:
				pc->unit->frozen = FROZEN::NO;
				break;
			// exit from arena
			case NetChangePlayer::EXIT_ARENA:
				fallback_co = FALLBACK::ARENA_EXIT;
				fallback_t = -1.f;
				pc->unit->frozen = FROZEN::YES;
				break;
			// player refused to pvp
			case NetChangePlayer::NO_PVP:
				{
					byte player_id;
					f >> player_id;
					if(!f)
						StreamError("Update single client: Broken NO_PVP.");
					else
					{
						PlayerInfo* info = GetPlayerInfoTry(player_id);
						if(!info)
							StreamError("Update single client: NO_PVP, invalid player id %u.", player_id);
						else
							AddMsg(Format(txPvpRefuse, info->name.c_str()));
					}
				}
				break;
			// can't leave location message
			case NetChangePlayer::CANT_LEAVE_LOCATION:
				{
					CanLeaveLocationResult reason;
					f.ReadCasted<byte>(reason);
					if(!f)
						StreamError("Update single client: Broken CANT_LEAVE_LOCATION.");
					else if(reason != CanLeaveLocationResult::InCombat && reason != CanLeaveLocationResult::TeamTooFar)
						StreamError("Update single client: CANT_LEAVE_LOCATION, invalid reason %u.", (byte)reason);
					else
						AddGameMsg3(reason == CanLeaveLocationResult::TeamTooFar ? GMS_GATHER_TEAM : GMS_NOT_IN_COMBAT);
				}
				break;
			// force player to look at unit
			case NetChangePlayer::LOOK_AT:
				{
					int netid;
					f >> netid;
					if(!f)
						StreamError("Update single client: Broken LOOK_AT.");
					else
					{
						if(netid == -1)
							pc->unit->look_target = nullptr;
						else
						{
							Unit* unit = FindUnit(netid);
							if(!unit)
								StreamError("Update single client: LOOK_AT, missing unit %d.", netid);
							else
								pc->unit->look_target = unit;
						}
					}
				}
				break;
			// end of fallback
			case NetChangePlayer::END_FALLBACK:
				if(fallback_co == FALLBACK::CLIENT)
					fallback_co = FALLBACK::CLIENT2;
				break;
			// response to rest in inn
			case NetChangePlayer::REST:
				{
					byte days;
					f >> days;
					if(!f)
						StreamError("Update single client: Broken REST.");
					else
					{
						fallback_co = FALLBACK::REST;
						fallback_t = -1.f;
						fallback_1 = days;
						pc->unit->frozen = FROZEN::YES;
					}
				}
				break;
			// response to training
			case NetChangePlayer::TRAIN:
				{
					byte type, stat_type;
					f >> type;
					f >> stat_type;
					if(!f)
						StreamError("Update single client: Broken TRAIN.");
					else
					{
						fallback_co = FALLBACK::TRAIN;
						fallback_t = -1.f;
						fallback_1 = type;
						fallback_2 = stat_type;
						pc->unit->frozen = FROZEN::YES;
					}
				}
				break;
			// warped player to not stuck position
			case NetChangePlayer::UNSTUCK:
				{
					Vec3 new_pos;
					f >> new_pos;
					if(!f)
						StreamError("Update single client: Broken UNSTUCK.");
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
					f >> player_id;
					f >> count;
					if(!f)
						StreamError("Update single client: Broken GOLD_RECEIVED.");
					else if(count <= 0)
						StreamError("Update single client: GOLD_RECEIVED, invalid count %d.", count);
					else
					{
						PlayerInfo* info = GetPlayerInfoTry(player_id);
						if(!info)
							StreamError("Update single client: GOLD_RECEIVED, invalid player id %u.", player_id);
						else
						{
							AddMultiMsg(Format(txReceivedGold, count, info->name.c_str()));
							sound_mgr->PlaySound2d(sCoins);
						}
					}
				}
				break;
			// message about gaining attribute/skill
			case NetChangePlayer::GAIN_STAT:
				{
					bool is_skill;
					byte what, value;
					f >> is_skill;
					f >> what;
					f >> value;
					if(!f)
						StreamError("Update single client: Broken GAIN_STAT.");
					else
						ShowStatGain(is_skill, what, value);
				}
				break;
			// update trader gold
			case NetChangePlayer::UPDATE_TRADER_GOLD:
				{
					int netid, count;
					f >> netid;
					f >> count;
					if(!f)
						StreamError("Update single client: Broken UPDATE_TRADER_GOLD.");
					else if(count < 0)
						StreamError("Update single client: UPDATE_TRADER_GOLD, invalid count %d.", count);
					else
					{
						Unit* unit = FindUnit(netid);
						if(!unit)
							StreamError("Update single client: UPDATE_TRADER_GOLD, missing unit %d.", netid);
						else if(!pc->IsTradingWith(unit))
							StreamError("Update single client: UPDATE_TRADER_GOLD, unit %d (%s) is not trading with player.", netid, unit->data->id.c_str());
						else
							unit->gold = count;
					}
				}
				break;
			// update trader inventory after getting item
			case NetChangePlayer::UPDATE_TRADER_INVENTORY:
				{
					int netid;
					f >> netid;
					if(!f)
						StreamError("Update single client: Broken UPDATE_TRADER_INVENTORY.");
					else
					{
						Unit* unit = FindUnit(netid);
						bool ok = true;
						if(!unit)
						{
							StreamError("Update single client: UPDATE_TRADER_INVENTORY, missing unit %d.", netid);
							ok = false;
						}
						else if(!pc->IsTradingWith(unit))
						{
							StreamError("Update single client: UPDATE_TRADER_INVENTORY, unit %d (%s) is not trading with player.",
								netid, unit->data->id.c_str());
							ok = false;
						}
						if(ok)
						{
							if(!ReadItemListTeam(f, unit->items))
								StreamError("Update single client: Broken UPDATE_TRADER_INVENTORY(2).");
						}
						else
						{
							vector<ItemSlot> items;
							if(!ReadItemListTeam(f, items, true))
								Error("Update single client: Broken UPDATE_TRADER_INVENTORY(3).");
						}
					}
				}
				break;
			// update player statistics
			case NetChangePlayer::PLAYER_STATS:
				{
					byte flags;
					f >> flags;
					if(!f)
						StreamError("Update single client: Broken PLAYER_STATS.");
					else if(flags == 0 || (flags & ~STAT_MAX) != 0)
						StreamError("Update single client: PLAYER_STATS, invalid flags %u.", flags);
					else
					{
						int set_flags = CountBits(flags);
						// read to buffer
						f.Read(BUF, sizeof(int)*set_flags);
						if(!f)
							StreamError("Update single client: Broken PLAYER_STATS(2).");
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
			// message about gaining multiple items
			case NetChangePlayer::ADDED_ITEMS_MSG:
				{
					byte count;
					f >> count;
					if(!f)
						StreamError("Update single client: Broken ADDED_ITEMS_MSG.");
					else if(count <= 1)
						StreamError("Update single client: ADDED_ITEMS_MSG, invalid count %u.", count);
					else
						AddGameMsg(Format(txGmsAddedItems, (int)count), 3.f);
				}
				break;
			// player stat changed
			case NetChangePlayer::STAT_CHANGED:
				{
					byte type, what;
					int value;
					f >> type;
					f >> what;
					f >> value;
					if(!f)
						StreamError("Update single client: Broken STAT_CHANGED.");
					else
					{
						switch((ChangedStatType)type)
						{
						case ChangedStatType::ATTRIBUTE:
							if(what >= (byte)AttributeId::MAX)
								StreamError("Update single client: STAT_CHANGED, invalid attribute %u.", what);
							else if(pc)
								pc->unit->Set((AttributeId)what, value);
							break;
						case ChangedStatType::SKILL:
							if(what >= (byte)SkillId::MAX)
								StreamError("Update single client: STAT_CHANGED, invalid skill %u.", what);
							else if(pc)
								pc->unit->Set((SkillId)what, value);
							break;
						case ChangedStatType::BASE_ATTRIBUTE:
							if(what >= (byte)AttributeId::MAX)
								StreamError("Update single client: STAT_CHANGED, invalid base attribute %u.", what);
							else if(pc)
								pc->SetBase((AttributeId)what, value);
							break;
						case ChangedStatType::BASE_SKILL:
							if(what >= (byte)SkillId::MAX)
								StreamError("Update single client: STAT_CHANGED, invalid base skill %u.", what);
							else if(pc)
								pc->SetBase((SkillId)what, value);
							break;
						default:
							StreamError("Update single client: STAT_CHANGED, invalid change type %u.", type);
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
					f >> id;
					f >> value;
					if(!f)
						StreamError("Update single client: Broken ADD_PERK.");
					else
						pc->perks.push_back(TakenPerk((Perk)id, value));
				}
				break;
			// show game message
			case NetChangePlayer::GAME_MESSAGE:
				{
					int gm_id;
					f >> gm_id;
					if(!f)
						StreamError("Update single client: Broken GAME_MESSAGE.");
					else
						AddGameMsg3((GMS)gm_id);
				}
				break;
			// run script result
			case NetChangePlayer::RUN_SCRIPT_RESULT:
				{
					string* output = StringPool.Get();
					f.ReadString4(*output);
					if(!f)
						StreamError("Update single client: Broken RUN_SCRIPT_RESULT.");
					else
						AddConsoleMsg(output->c_str());
					StringPool.Free(output);
				}
				break;
			default:
				Warn("Update single client: Unknown player change type %d.", type);
				StreamError();
				break;
			}

			byte checksum;
			f >> checksum;
			if(!f || checksum != 0xFF)
			{
				StreamError("Update single client: Invalid checksum (%u).", change_i);
				return true;
			}
		}
	}

	// gold
	if(IS_SET(flags, PlayerInfo::UF_GOLD))
	{
		if(!pc)
			f.Skip(sizeof(pc->unit->gold));
		else
		{
			f >> pc->unit->gold;
			if(!f)
			{
				StreamError("Update single client: Broken ID_PLAYER_CHANGES at UF_GOLD.");
				return true;
			}
		}
	}

	// alcohol
	if(IS_SET(flags, PlayerInfo::UF_ALCOHOL))
	{
		if(!pc)
			f.Skip(sizeof(pc->unit->alcohol));
		else
		{
			f >> pc->unit->alcohol;
			if(!f)
			{
				StreamError("Update single client: Broken ID_PLAYER_CHANGES at UF_GOLD.");
				return true;
			}
		}
	}

	// buffs
	if(IS_SET(flags, PlayerInfo::UF_BUFFS))
	{
		PlayerInfo* player_info = pc ? pc->player_info : &GetPlayerInfo(my_id);
		f.ReadCasted<byte>(player_info->buffs);
		if(!f)
		{
			StreamError("Update single client: Broken ID_PLAYER_CHANGES at UF_BUFFS.");
			return true;
		}
	}

	// stamina
	if(IS_SET(flags, PlayerInfo::UF_STAMINA))
	{
		if(!pc)
			f.Skip(sizeof(pc->unit->stamina));
		else
		{
			f.Read(pc->unit->stamina);
			if(!f)
			{
				StreamError("Update single client: Broken ID_PLAYER_CHANGES at UF_STAMINA.");
				return true;
			}
		}
	}

	return true;
}

//=================================================================================================
void Game::WriteClientChanges(BitStreamWriter& f)
{
	// count
	f.WriteCasted<byte>(Net::changes.size());
	if(Net::changes.size() > 255)
		Error("Update client: Too many changes %u!", Net::changes.size());

	// changes
	for(NetChange& c : Net::changes)
	{
		f.WriteCasted<byte>(c.type);

		switch(c.type)
		{
		case NetChange::CHANGE_EQUIPMENT:
		case NetChange::IS_BETTER_ITEM:
		case NetChange::CONSUME_ITEM:
			f << c.id;
			break;
		case NetChange::TAKE_WEAPON:
			f << (c.id != 0);
			f.WriteCasted<byte>(c.id == 0 ? pc->unit->weapon_taken : pc->unit->weapon_hiding);
			break;
		case NetChange::ATTACK:
			{
				byte b = (byte)c.id;
				b |= ((pc->unit->attack_id & 0xF) << 4);
				f << b;
				f << c.f[1];
				if(c.id == 2)
					f << PlayerAngleY() * 12;
			}
			break;
		case NetChange::DROP_ITEM:
			f << c.id;
			f << c.ile;
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
			f.WriteCasted<byte>(c.id);
			break;
		case NetChange::CHEAT_WARP_TO_STAIRS:
		case NetChange::CHEAT_CHANGE_LEVEL:
		case NetChange::CHEAT_NOAI:
		case NetChange::PVP:
		case NetChange::CHEAT_GODMODE:
		case NetChange::CHEAT_INVISIBLE:
		case NetChange::CHEAT_NOCLIP:
		case NetChange::CHANGE_ALWAYS_RUN:
			f << (c.id != 0);
			break;
		case NetChange::PICKUP_ITEM:
		case NetChange::LOOT_UNIT:
		case NetChange::TALK:
		case NetChange::LOOT_CHEST:
		case NetChange::SKIP_DIALOG:
		case NetChange::CHEAT_SKIP_DAYS:
		case NetChange::PAY_CREDIT:
		case NetChange::DROP_GOLD:
		case NetChange::TAKE_ITEM_CREDIT:
			f << c.id;
			break;
		case NetChange::CHEAT_ADD_GOLD:
			f << (c.id == 1);
			f << c.ile;
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
		case NetChange::CHEAT_REFRESH_COOLDOWN:
		case NetChange::END_FALLBACK:
		case NetChange::END_TRAVEL:
			break;
		case NetChange::ADD_NOTE:
			f << game_gui->journal->GetNotes().back();
			break;
		case NetChange::USE_USABLE:
			f << c.id;
			f.WriteCasted<byte>(c.ile);
			break;
		case NetChange::CHEAT_KILLALL:
			f << (c.unit ? c.unit->netid : -1);
			f.WriteCasted<byte>(c.id);
			break;
		case NetChange::CHEAT_KILL:
		case NetChange::CHEAT_HEAL_UNIT:
		case NetChange::CHEAT_HURT:
		case NetChange::CHEAT_BREAK_ACTION:
		case NetChange::CHEAT_FALL:
			f << c.unit->netid;
			break;
		case NetChange::CHEAT_ADD_ITEM:
			f << c.base_item->id;
			f << c.ile;
			f << (c.id != 0);
			break;
		case NetChange::CHEAT_SPAWN_UNIT:
			f << c.base_unit->id;
			f.WriteCasted<byte>(c.ile);
			f.WriteCasted<char>(c.id);
			f.WriteCasted<char>(c.i);
			break;
		case NetChange::CHEAT_SET_STAT:
		case NetChange::CHEAT_MOD_STAT:
			f.WriteCasted<byte>(c.id);
			f << (c.ile != 0);
			f.WriteCasted<char>(c.i);
			break;
		case NetChange::LEAVE_LOCATION:
			f.WriteCasted<char>(c.id);
			break;
		case NetChange::USE_DOOR:
			f << c.id;
			f << (c.ile != 0);
			break;
		case NetChange::TRAIN:
			f.WriteCasted<byte>(c.id);
			f.WriteCasted<byte>(c.ile);
			break;
		case NetChange::GIVE_GOLD:
		case NetChange::GET_ITEM:
		case NetChange::PUT_ITEM:
			f << c.id;
			f << c.ile;
			break;
		case NetChange::PUT_GOLD:
			f << c.ile;
			break;
		case NetChange::PLAYER_ACTION:
			f << c.pos;
			break;
		case NetChange::CHEAT_STUN:
			f << c.unit->netid;
			f << c.f[0];
			break;
		case NetChange::RUN_SCRIPT:
			f.WriteString4(*c.str);
			StringPool.Free(c.str);
			f << c.id;
			break;
		case NetChange::SET_NEXT_ACTION:
			f.WriteCasted<byte>(pc->next_action);
			switch(pc->next_action)
			{
			case NA_NONE:
				break;
			case NA_REMOVE:
			case NA_DROP:
				f.WriteCasted<byte>(pc->next_action_data.slot);
				break;
			case NA_EQUIP:
			case NA_CONSUME:
				f << pc->next_action_data.index;
				f << pc->next_action_data.item->id;
				break;
			case NA_USE:
				f << pc->next_action_data.usable->netid;
				break;
			default:
				assert(0);
				break;
			}
			break;
		default:
			Error("UpdateClient: Unknown change %d.", c.type);
			assert(0);
			break;
		}

		f.WriteCasted<byte>(0xFF);
	}
}

//=================================================================================================
void Game::Client_Say(BitStreamReader& f)
{
	byte id;

	f >> id;
	const string& text = f.ReadString1();
	if(!f)
		StreamError("Client_Say: Broken packet.");
	else
	{
		int index = GetPlayerIndex(id);
		if(index == -1)
			StreamError("Client_Say: Broken packet, missing player %d.", id);
		else
		{
			PlayerInfo& info = *game_players[index];
			cstring s = Format("%s: %s", info.name.c_str(), text.c_str());
			AddMsg(s);
			if(game_state == GS_LEVEL)
				game_gui->AddSpeechBubble(info.u, text.c_str());
		}
	}
}

//=================================================================================================
void Game::Client_Whisper(BitStreamReader& f)
{
	byte id;

	f >> id;
	const string& text = f.ReadString1();
	if(!f)
		StreamError("Client_Whisper: Broken packet.");
	else
	{
		int index = GetPlayerIndex(id);
		if(index == -1)
			StreamError("Client_Whisper: Broken packet, missing player %d.", id);
		else
		{
			cstring s = Format("%s@: %s", game_players[index]->name.c_str(), text.c_str());
			AddMsg(s);
		}
	}
}

//=================================================================================================
void Game::Client_ServerSay(BitStreamReader& f)
{
	const string& text = f.ReadString1();
	if(!f)
		StreamError("Client_ServerSay: Broken packet.");
	else
		AddServerMsg(text.c_str());
}

//=================================================================================================
void Game::Server_Say(BitStream& stream, PlayerInfo& info, Packet* packet)
{
	byte id;
	BitStreamReader f(stream);
	f >> id;
	const string& text = f.ReadString1();
	if(!f)
		StreamError("Server_Say: Broken packet from %s: %s.", info.name.c_str());
	else
	{
		// id gracza jest ignorowane przez serwer ale mo�na je sprawdzi�
		assert(id == info.id);

		cstring str = Format("%s: %s", info.name.c_str(), text.c_str());
		AddMsg(str);

		// prze�lij dalej
		if (players > 2)
		{
			peer->Send(&stream, MEDIUM_PRIORITY, RELIABLE, 0, packet->systemAddress, true);
			StreamWrite(stream, Stream_Chat, packet->systemAddress);
		}

		if(game_state == GS_LEVEL)
			game_gui->AddSpeechBubble(info.u, text.c_str());
	}
}

//=================================================================================================
void Game::Server_Whisper(BitStreamReader& f, PlayerInfo& info, Packet* packet)
{
	byte id;
	f >> id;
	const string& text = f.ReadString1();
	if(!f)
		StreamError("Server_Whisper: Broken packet from %s.", info.name.c_str());
	else
	{
		if(id == my_id)
		{
			// wiadomo�� do mnie
			cstring str = Format("%s@: %s", info.name.c_str(), text.c_str());
			AddMsg(str);
		}
		else
		{
			// wiadomo�� do kogo� innego
			int index = GetPlayerIndex(id);
			if(index == -1)
				StreamError("Server_Whisper: Broken packet from %s to missing player %d.", info.name.c_str(), id);
			else
			{
				PlayerInfo& info2 = *game_players[index];
				packet->data[1] = (byte)info.id;
				peer->Send((cstring)packet->data, packet->length, MEDIUM_PRIORITY, RELIABLE, 0, info2.adr, false);
				StreamWrite(packet, Stream_Chat, info2.adr);
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
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::UNIT_POS;
			c.unit = *it;
			(*it)->changed = false;
		}
		if((*it)->IsAI() && (*it)->ai->change_ai_mode)
		{
			NetChange& c = Add1(Net::changes);
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
		if((it->timer -= dt * 2) <= 0.f)
		{
			UnitWarpData& uwd = Add1(unit_warp_data);
			uwd.unit = it->u;
			uwd.where = it->where;

			for(Unit* unit : Team.members)
			{
				if(unit->IsHero() && unit->hero->following == it->u)
				{
					UnitWarpData& uwd = Add1(unit_warp_data);
					uwd.unit = unit;
					uwd.where = it->where;
				}
			}

			NetChangePlayer& c = Add1(it->u->player->player_info->changes);
			c.type = NetChangePlayer::SET_FROZEN;
			c.id = 0;

			it->u->frozen = FROZEN::NO;

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
	Net::changes.clear();
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
	DeleteElements(game_players);
	my_id = 0;
	leader_id = 0;
	last_id = 0;
	paused = false;
	hardcore_mode = false;

	auto info = new PlayerInfo;
	game_players.push_back(info);
	server_panel->grid.AddItem();

	PlayerInfo& sp = *info;
	sp.name = player_name;
	sp.id = 0;
	sp.state = PlayerInfo::IN_LOBBY;
	sp.left = PlayerInfo::LEFT_NO;

	if(!mp_load)
	{
		Unit::netid_counter = 0;
		GroundItem::netid_counter = 0;
		Chest::netid_counter = 0;
		skip_id_counter = 0;
		Usable::netid_counter = 0;
		Trap::netid_counter = 0;
		Door::netid_counter = 0;
		Electro::netid_counter = 0;

		server_panel->CheckAutopick();
	}
	else
	{
		// search for saved character
		PlayerInfo* old = FindOldPlayer(player_name.c_str());
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
		Net::changes.clear(); // przy wczytywaniu jest czyszczone przed wczytaniem i w net_changes s� zapisane quest_items
	if(!net_talk.empty())
		StringPool.Free(net_talk);
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
Usable* Game::FindUsable(int netid)
{
	for(vector<Usable*>::iterator it = local_ctx.usables->begin(), end = local_ctx.usables->end(); it != end; ++it)
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
				for(vector<Usable*>::iterator it2 = (*it)->usables.begin(), end2 = (*it)->usables.end(); it2 != end2; ++it2)
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
int Game::ReadItemAndFind(BitStreamReader& f, const Item*& item) const
{
	item = nullptr;

	const string& item_id = f.ReadString1();
	if(!f)
		return -2;

	if(item_id.empty())
		return 0;

	if(item_id[0] == '$')
	{
		int quest_refid = f.Read<int>();
		if(!f)
			return -2;

		item = FindQuestItemClient(item_id.c_str(), quest_refid);
		if(!item)
		{
			Warn("Missing quest item '%s' (%d).", item_id.c_str(), quest_refid);
			return -1;
		}
		else
			return 1;
	}
	else
	{
		item = Item::TryGet(item_id);
		if(!item)
		{
			Warn("Missing item '%s'.", item_id.c_str());
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

		for(int i = 0; i < SLOT_MAX; ++i)
			prev_slots[i] = unit.slots[i];

		unit.ReequipItems();

		for(int i = 0; i < SLOT_MAX; ++i)
		{
			if(unit.slots[i] != prev_slots[i])
			{
				NetChange& c = Add1(Net::changes);
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

		for(auto info : game_players)
		{
			if(info->left == PlayerInfo::LEFT_NO && info->pc != player)
				info->pc->free_days += count;
		}

		WorldProgress(count, WPM_NORMAL);
	}

	Net::PushChange(NetChange::UPDATE_FREE_DAYS);
}

//=================================================================================================
PlayerInfo* Game::FindOldPlayer(cstring nick)
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
void Game::PrepareWorldData(BitStreamWriter& f)
{
	Info("Preparing world data.");

	f << ID_WORLD_DATA;

	// locations
	f.WriteCasted<byte>(locations.size());
	for(Location* loc_ptr : locations)
	{
		if(!loc_ptr)
		{
			f.WriteCasted<byte>(L_NULL);
			continue;
		}

		Location& loc = *loc_ptr;
		f.WriteCasted<byte>(loc.type);
		if(loc.type == L_DUNGEON || loc.type == L_CRYPT)
			f.WriteCasted<byte>(loc.GetLastLevel() + 1);
		else if(loc.type == L_CITY)
		{
			City& city = (City&)loc;
			f.WriteCasted<byte>(city.citizens);
			f.WriteCasted<word>(city.citizens_world);
			f.WriteCasted<byte>(city.settlement_type);
		}
		f.WriteCasted<byte>(loc.state);
		f << loc.pos;
		f << loc.name;
		f.WriteCasted<byte>(loc.image);
	}
	f.WriteCasted<byte>(current_location);

	// quests
	QuestManager::Get().Write(f);

	// rumors
	f.WriteStringArray<byte, word>(game_gui->journal->GetRumors());

	// time
	f.WriteCasted<byte>(year);
	f.WriteCasted<byte>(month);
	f.WriteCasted<byte>(day);
	f << worldtime;

	// stats
	GameStats::Get().Write(f);

	// mp vars
	WriteNetVars(f);

	// quest items
	f.WriteCasted<word>(Net::changes.size());
	for(NetChange& c : Net::changes)
	{
		assert(c.type == NetChange::REGISTER_ITEM);
		f << c.base_item->id;
		f << c.item2->id;
		f << c.item2->name;
		f << c.item2->desc;
		f << c.item2->refid;
	}
	Net::changes.clear();
	if(!net_talk.empty())
		StringPool.Free(net_talk);

	// secret note text
	f << GetSecretNote()->desc;

	// position on world map
	if(world_state == WS_TRAVEL)
	{
		f << true;
		f << picked_location;
		f << travel_day;
		f << travel_time;
		f << travel_start;
		f << world_pos;
	}
	else
		f << false;

	f.WriteCasted<byte>(0xFF);
}

//=================================================================================================
bool Game::ReadWorldData(BitStreamReader& f)
{
	// count of locations
	byte count;
	f >> count;
	if(!f.Ensure(count))
	{
		Error("Read world: Broken packet.");
		return false;
	}

	// locations
	locations.resize(count);
	uint index = 0;
	for(Location*& loc : locations)
	{
		LOCATION type;
		f.ReadCasted<byte>(type);
		if(!f)
		{
			Error("Read world: Broken packet for location %u.", index);
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
				f >> levels;
				if(!f)
				{
					Error("Read world: Broken packet for dungeon location %u.", index);
					return false;
				}
				else if(levels == 0)
				{
					Error("Read world: Location %d with 0 levels.", index);
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
			{
				byte citizens;
				word world_citizens;
				byte type;
				f >> citizens;
				f >> world_citizens;
				f >> type;
				if(!f)
				{
					Error("Read world: Broken packet for city location %u.", index);
					return false;
				}

				City* city = new City;
				loc = city;
				city->citizens = citizens;
				city->citizens_world = world_citizens;
				city->settlement_type = (City::SettlementType)type;
			}
			break;
		default:
			Error("Read world: Unknown location type %d for location %u.", type, index);
			return false;
		}

		// location data
		f.ReadCasted<byte>(loc->state);
		f >> loc->pos;
		f >> loc->name;
		f.ReadCasted<byte>(loc->image);
		if(!f)
		{
			Error("Read world: Broken packet(2) for location %u.", index);
			return false;
		}
		loc->type = type;
		if(loc->state > LS_CLEARED)
		{
			Error("Read world: Invalid state %d for location %u.", loc->state, index);
			return false;
		}

		++index;
	}

	// current location
	f.ReadCasted<byte>(current_location);
	if(!f)
	{
		Error("Read world: Broken packet for current location.");
		return false;
	}
	if(current_location >= (int)locations.size() || !locations[current_location])
	{
		Error("Read world: Invalid location %d.", current_location);
		return false;
	}
	location = locations[current_location];
	world_pos = location->pos;
	location->state = LS_VISITED;

	// quests
	if(!QuestManager::Get().Read(f))
		return false;

	// rumors
	f.ReadStringArray<byte, word>(game_gui->journal->GetRumors());
	if(!f)
	{
		Error("Read world: Broken packet for rumors.");
		return false;
	}

	// time
	f.ReadCasted<byte>(year);
	f.ReadCasted<byte>(month);
	f.ReadCasted<byte>(day);
	f >> worldtime;
	GameStats::Get().Read(f);
	if(!f)
	{
		Error("Read world: Broken packet for time.");
		return false;
	}

	// mp vars
	ReadNetVars(f);
	if(!f)
	{
		Error("Read world: Broken packet for mp vars.");
		return false;
	}

	// questowe przedmioty
	const int QUEST_ITEM_MIN_SIZE = 7;
	word quest_items_count;
	f >> quest_items_count;
	if(f.Ensure(QUEST_ITEM_MIN_SIZE * quest_items_count))
	{
		Error("Read world: Broken packet for quest items.");
		return false;
	}
	quest_items.reserve(quest_items_count);
	for(word i = 0; i < quest_items_count; ++i)
	{
		const Item* base_item;
		if(!ReadItemSimple(f, base_item))
		{
			Error("Read world: Broken packet for quest item %u.", i);
			return false;
		}
		else
		{
			Item* item = CreateItemCopy(base_item);
			f >> item->id;
			f >> item->name;
			f >> item->desc;
			f >> item->refid;
			if(!f)
			{
				Error("Read world: Broken packet for quest item %u (2).", i);
				delete item;
				return false;
			}
			else
				quest_items.push_back(item);
		}
	}

	// secret note text
	f >> GetSecretNote()->desc;
	if(!f)
	{
		Error("Read world: Broken packet for secret note text.");
		return false;
	}

	// position on world map
	bool in_travel;
	f >> in_travel;
	if(!f)
	{
		Error("Read world: Broken packet for in travel data.");
		return false;
	}
	if(in_travel)
	{
		world_state = WS_TRAVEL;
		f >> picked_location;
		f >> travel_day;
		f >> travel_time;
		f >> travel_start;
		f >> world_pos;
		if(!f)
		{
			Error("Read world: Broken packet for in travel data (2).");
			return false;
		}
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
	if(!sound_mgr->IsMusicDisabled())
	{
		LoadMusic(MusicType::Boss, false);
		LoadMusic(MusicType::Death, false);
		LoadMusic(MusicType::Travel, false);
	}

	return true;
}

//=================================================================================================
void Game::WriteNetVars(BitStreamWriter& f)
{
	f << mp_use_interp;
	f << mp_interp;
}

//=================================================================================================
void Game::ReadNetVars(BitStreamReader& f)
{
	f >> mp_use_interp;
	f >> mp_interp;
}

//=================================================================================================
void Game::InterpolateUnits(float dt)
{
	for(Unit* unit : *local_ctx.units)
	{
		if(unit != pc->unit)
			UpdateInterpolator(unit->interp, dt, unit->visual_pos, unit->rot);
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
	if(city_ctx)
	{
		for(InsideBuilding* inside : city_ctx->inside_buildings)
		{
			for(Unit* unit : inside->units)
			{
				if(unit != pc->unit)
					UpdateInterpolator(unit->interp, dt, unit->visual_pos, unit->rot);
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
}

//=================================================================================================
void Game::InterpolatePlayers(float dt)
{
	for(auto info : game_players)
	{
		if(info->id != my_id && info->left == PlayerInfo::LEFT_NO)
			UpdateInterpolator(info->u->interp, dt, info->u->visual_pos, info->u->rot);
	}
}

//=================================================================================================
void EntityInterpolator::Reset(const Vec3& pos, float rot)
{
	valid_entries = 1;
	entries[0].pos = pos;
	entries[0].rot = rot;
	entries[0].timer = 0.f;
}

//=================================================================================================
void EntityInterpolator::Add(const Vec3& pos, float rot)
{
	for(int i = MAX_ENTRIES - 1; i > 0; --i)
		entries[i] = entries[i - 1];

	entries[0].pos = pos;
	entries[0].rot = rot;
	entries[0].timer = 0.f;

	valid_entries = min(valid_entries + 1, MAX_ENTRIES);
}

//=================================================================================================
void Game::UpdateInterpolator(EntityInterpolator* e, float dt, Vec3& pos, float& rot)
{
	assert(e);

	for(int i = 0; i < EntityInterpolator::MAX_ENTRIES; ++i)
		e->entries[i].timer += dt;

	if(mp_use_interp)
	{
		if(e->entries[0].timer > mp_interp)
		{
			// nie ma nowszej klatki
			// extrapolation ? nie dzi�...
			pos = e->entries[0].pos;
			rot = e->entries[0].rot;
		}
		else
		{
			// znajd� odpowiednie klatki
			for(int i = 0; i < e->valid_entries; ++i)
			{
				if(Equal(e->entries[i].timer, mp_interp))
				{
					// r�wne trafienie w klatke
					pos = e->entries[i].pos;
					rot = e->entries[i].rot;
					return;
				}
				else if(e->entries[i].timer > mp_interp)
				{
					// interpolacja pomi�dzy dwoma klatkami ([i-1],[i])
					EntityInterpolator::Entry& e1 = e->entries[i - 1];
					EntityInterpolator::Entry& e2 = e->entries[i];
					float t = (mp_interp - e1.timer) / (e2.timer - e1.timer);
					pos = Vec3::Lerp(e1.pos, e2.pos, t);
					rot = Clip(Slerp(e1.rot, e2.rot, t));
					return;
				}
			}

			// brak ruchu do tej pory
		}
	}
	else
	{
		// nie u�ywa interpolacji
		pos = e->entries[0].pos;
		rot = e->entries[0].rot;
	}

	assert(rot >= 0.f && rot < PI * 2);
}

//=================================================================================================
void Game::WritePlayerStartData(BitStreamWriter& f, PlayerInfo& info)
{
	f << ID_PLAYER_START_DATA;

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
	f << flags;

	// notes
	f.WriteStringArray<word, word>(info.notes);

	f.WriteCasted<byte>(0xFF);
}

//=================================================================================================
bool Game::ReadPlayerStartData(BitStreamReader& f)
{
	byte flags;
	f >> flags;
	f.ReadStringArray<word, word>(game_gui->journal->GetNotes());
	if(!f)
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
	f >> check;
	if(!f || check != 0xFF)
	{
		Error("Read player start data: Broken checksum.");
		return false;
	}

	return true;
}

//=================================================================================================
bool Game::CheckMoveNet(Unit& unit, const Vec3& pos)
{
	global_col.clear();

	const float radius = unit.GetUnitRadius();
	IgnoreObjects ignore = { 0 };
	Unit* ignored[] = { &unit, nullptr };
	const void* ignored_objects[2] = { nullptr, nullptr };
	if(unit.usable)
		ignored_objects[0] = unit.usable;
	ignore.ignored_units = (const Unit**)ignored;
	ignore.ignored_objects = ignored_objects;

	GatherCollisionObjects(GetContext(unit), global_col, pos, radius, &ignore);

	if(global_col.empty())
		return true;

	return !Collide(global_col, pos, radius);
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
	if(!players_left)
		return;

	LoopAndRemove(game_players, [this](PlayerInfo* pinfo)
	{
		auto& info = *pinfo;
		if(info.left == PlayerInfo::LEFT_NO)
			return false;

		// order of changes is important here
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::REMOVE_PLAYER;
		c.id = info.id;
		c.ile = (int)info.left;

		RemovePlayer(info);

		if(leader_id == c.id)
		{
			leader_id = my_id;
			Team.leader = pc->unit;
			NetChange& c2 = Add1(Net::changes);
			c2.type = NetChange::CHANGE_LEADER;
			c2.id = my_id;

			if(dialog_enc)
				dialog_enc->bts[0].state = Button::NONE;

			AddMsg(txYouAreLeader);
		}

		CheckCredit();
		delete pinfo;

		return true;
	});

	players_left = false;
}

//=================================================================================================
void Game::RemovePlayer(PlayerInfo& info)
{
	switch(info.left)
	{
	case PlayerInfo::LEFT_TIMEOUT:
		{
			Info("Player %s kicked due to timeout.", info.name.c_str());
			AddMsg(Format(txPlayerKicked, info.name.c_str()));
		}
		break;
	case PlayerInfo::LEFT_KICK:
		{
			Info("Player %s kicked from server.", info.name.c_str());
			AddMsg(Format(txPlayerKicked, info.name.c_str()));
		}
		break;
	case PlayerInfo::LEFT_DISCONNECTED:
		{
			Info("Player %s disconnected from server.", info.name.c_str());
			AddMsg(Format(txPlayerDisconnected, info.name.c_str()));
		}
		break;
	case PlayerInfo::LEFT_QUIT:
		{
			Info("Player %s quit game.", info.name.c_str());
			AddMsg(Format(txPlayerQuit, info.name.c_str()));
		}
		break;
	default:
		assert(0);
		break;
	}

	if(!info.u)
		return;

	Unit* unit = info.u;
	RemoveElement(Team.members, unit);
	RemoveElement(Team.active_members, unit);
	if(game_state == GS_WORLDMAP)
	{
		if(Net::IsLocal() && open_location == -1)
			DeleteUnit(unit);
	}
	else
	{
		to_remove.push_back(unit);
		unit->to_remove = true;
	}
	info.u = nullptr;
}

//=================================================================================================
bool Game::FilterOut(NetChange& c)
{
	switch(c.type)
	{
	case NetChange::CHANGE_EQUIPMENT:
		return Net::IsServer();
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
		if(Net::IsServer() && c.str)
		{
			StringPool.Free(c.str);
			RemoveElement(net_talk, c.str);
			c.str = nullptr;
		}
		return true;
	case NetChange::RUN_SCRIPT:
		StringPool.Free(c.str);
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
	case NetChangePlayer::ADDED_ITEMS_MSG:
	case NetChangePlayer::GAME_MESSAGE:
	case NetChangePlayer::RUN_SCRIPT_RESULT:
		return false;
	default:
		return true;
		break;
	}
}

//=================================================================================================
void Game::Net_FilterClientChanges()
{
	for(vector<NetChange>::iterator it = Net::changes.begin(), end = Net::changes.end(); it != end;)
	{
		if(FilterOut(*it))
		{
			if(it + 1 == end)
			{
				Net::changes.pop_back();
				break;
			}
			else
			{
				std::iter_swap(it, end - 1);
				Net::changes.pop_back();
				end = Net::changes.end();
			}
		}
		else
			++it;
	}
}

//=================================================================================================
void Game::Net_FilterServerChanges()
{
	for(vector<NetChange>::iterator it = Net::changes.begin(), end = Net::changes.end(); it != end;)
	{
		if(FilterOut(*it))
		{
			if(it + 1 == end)
			{
				Net::changes.pop_back();
				break;
			}
			else
			{
				std::iter_swap(it, end - 1);
				Net::changes.pop_back();
				end = Net::changes.end();
			}
		}
		else
			++it;
	}

	for(PlayerInfo* info : game_players)
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
void Game::ClosePeer(bool wait)
{
	assert(peer);

	Info("Peer shutdown.");
	peer->Shutdown(wait ? I_SHUTDOWN : 0);
	Info("sv_online = false");
	if(Net::IsClient())
		was_client = true;
	Net::changes.clear();
	Net::SetMode(Net::Mode::Singleplayer);
}

//=================================================================================================
BitStream& Game::StreamStart(Packet* packet, StreamLogType type)
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

//=================================================================================================
void Game::StreamWrite(const void* data, uint size, StreamLogType type, const SystemAddress& adr)
{
	ErrorHandler::Get().StreamWrite(data, size, type, adr);
}

//=================================================================================================
PlayerInfo& Game::GetPlayerInfo(int id)
{
	for(auto info : game_players)
	{
		if(info->id == id)
			return *info;
	}
	assert(0);
	return *game_players[0];
}

//=================================================================================================
PlayerInfo* Game::GetPlayerInfoTry(int id)
{
	for(auto info : game_players)
	{
		if(info->id == id)
			return info;
	}
	return nullptr;
}
