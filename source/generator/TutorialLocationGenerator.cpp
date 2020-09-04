#include "Pch.h"
#include "TutorialLocationGenerator.h"

#include "AIController.h"
#include "Game.h"
#include "InsideLocation.h"
#include "InsideLocationLevel.h"
#include "Level.h"
#include "QuestManager.h"
#include "Quest_Tutorial.h"

struct RoomInfo
{
	Int2 pos, size;
	bool corridor;
	int connected;
};

RoomInfo t_rooms[] = {
	Int2(0,19), Int2(6,3), false, -1,
	Int2(0,14), Int2(8,5), false, 0,
	Int2(8,14), Int2(5,5), false, 1,
	Int2(13,13), Int2(7,7), false, 2,
	Int2(13,4), Int2(7,9), false, 3,
	Int2(14,0), Int2(5,5), false, 4,
	Int2(8,0), Int2(6,5), false, 5,
	Int2(10,4), Int2(3,4), true, 6,
	Int2(2,8), Int2(11,6), false, 7,
	Int2(2,1), Int2(7,7), false, 8,
};

char mapa_t[] = {
	"$$$$$$$$###########$$$"
	"$   ### #  S  #   #  $"
	"$ ###/###  T  + T #  $"
	"$ #     #     #   #  $"
	"$ #  S  ###+####+### $"
	"$ #     # # ##     # $"
	"$ #  T  # # ##     # $"
	"$ #     # # ##     # $"
	"$ ###+##### ##  S  # $"
	"$ #          #     # $"
	"$ # S      T #     # $"
	"$ # S        #     # $"
	"$ #          #     # $"
	"$ ##############+### $"
	"##############  T  # $"
	"#      #  S  #     # $"
	"#     T+  T  + TS  # $"
	"#      #     #     # $"
	"#    #########     # $"
	"# T  #       ####### $"
	"# S  #               $"
	"######$$$$$$$$$$$$$$$$"
};

/*
$ - blokada

S:
0 - start
1 - skrzynia z broni¹,pancerzem,tarcz¹,z³otem [w dó³]
2 - manekin obrócony w lewo
3 - goblin obrócony w dó³
4 - skrzynia z ³ukiem,mikstórk¹,z³otem [w dó³]
5 - tarcza strzelniacza [w prawo]
6 - cz³owiek [w dó³]

+:
0 - po tekœcie 1
1 - po ograbieniu skrzyni i tekœcie 2
2 - po tekœcie 4 [który wyœwietla siê po uderzeniu manekina]
3 - po zabiciu goblina
4 - po tekœcie 5
5 - po tekœcie 6 i ograbieniu skrzyni 4
6 - po tekœcie 7 i trafieniu w 5
*/

char mapa_t2[] = {
	"        66666655555   "
	"  99999966666655555   "
	"  99999966666655555   "
	"  99999966666655555   "
	"  999999967774444444  "
	"  9999999 7774444444  "
	"  9999999 7774444444  "
	"  9999999 7774444444  "
	"  888888888884444444  "
	"  888888888884444444  "
	"  888888888884444444  "
	"  888888888884444444  "
	"  888888888884444444  "
	"  888888888883333333  "
	"11111111222223333333  "
	"11111111222223333333  "
	"11111111222223333333  "
	"11111111222223333333  "
	"11111111222223333333  "
	"000000       3333333  "
	"000000                "
	"000000                "
};

char mapa_t3[] = {
	"        ###########   "
	"    ### #  4  #   #   "
	"  ###/###  6  4 5 #   "
	"  #     #     #   #   "
	"  #  6  ###5####3###  "
	"  #     # # ##     #  "
	"  #  8  # # ##     #  "
	"  #     # # ##     #  "
	"  ###6##### ##  3  #  "
	"  #          #     #  "
	"  # 5      7 #     #  "
	"  # 5        #     #  "
	"  #          #     #  "
	"  ##############2###  "
	"##############  4  #  "
	"#      #  1  #     #  "
	"#     10  2  1 32  #  "
	"#      #     #     #  "
	"#    #########     #  "
	"# 0  #       #######  "
	"# 0  #                "
	"######                "
};

void TutorialLocationGenerator::OnEnter()
{
	InsideLocationLevel& lvl = GetLevelData();
	Quest_Tutorial& quest = *quest_mgr->quest_tutorial;
	Int2 start_tile;

	lvl.w = lvl.h = 22;
	inside->SetActiveLevel(dungeon_level);
	game_level->lvl = &inside->GetLevelData();
	game_level->Apply();
	game->SetDungeonParamsAndTextures(g_base_locations[TUTORIAL_FORT]);

	// rooms
	lvl.rooms.resize(countof(t_rooms));
	for(uint i = 0; i < countof(t_rooms); ++i)
	{
		Room* room = Room::Get();
		RoomInfo& info = t_rooms[i];
		lvl.rooms[i] = room;
		room->index = i;
		room->target = (info.corridor ? RoomTarget::Corridor : RoomTarget::None);
		room->type = nullptr;
		room->pos = info.pos;
		room->size = info.size;
		room->connected.clear();
		if(info.connected != -1)
		{
			Room* room2 = lvl.rooms[info.connected];
			room->connected.push_back(room2);
			room2->connected.push_back(room);
		}
	}

	// map
	lvl.map = new Tile[22 * 22];
	for(int y = 0; y < 22; ++y)
	{
		for(int x = 0; x < 22; ++x)
		{
			Tile& p = lvl.map[x + y * 22];
			p.flags = 0;
			switch(mapa_t[x + y * 22])
			{
			case ' ':
				p.type = EMPTY;
				break;
			case '#':
				p.type = WALL;
				break;
			case '/':
				p.type = ENTRY_PREV;
				lvl.prevEntryPt = Int2(x, y);
				lvl.prevEntryDir = GDIR_UP;
				lvl.prevEntryType = ENTRY_STAIRS_UP;
				break;
			case '+':
				p.type = DOORS;
				break;
			case '$':
				p.type = BLOCKADE;
				break;
			case 'T':
				{
					p.type = EMPTY;
					Quest_Tutorial::Text& tt = Add1(quest.texts);
					char c = mapa_t3[x + y * 22];
					assert(InRange(c, '0', '9'));
					tt.id = int(c - '0');
					tt.state = (tt.id == 0 ? 1 : 0);
					tt.text = quest.txTut[tt.id];
					tt.pos = Vec3(2.f*x + 1, 0, 2.f*y + 1);
				}
				break;
			case 'S':
				{
					p.type = EMPTY;
					char c = mapa_t3[x + y * 22];
					assert(InRange(c, '0', '9'));
					switch((int)(c - '0'))
					{
					/*0 - start
					1 - skrzynia z broni¹,pancerzem,tarcz¹,z³otem [w dó³]
					2 - manekin obrócony w lewo
					3 - goblin obrócony w dó³
					4 - skrzynia z ³ukiem,mikstórk¹,z³otem [w dó³]
					5 - tarcza strzelniacza [w prawo]
					6 - cz³owiek [w dó³]*/
					case 0:
						start_tile = Int2(x, y);
						break;
					case 1:
						{
							BaseObject* o = BaseObject::Get("chest");
							Chest* chest = game_level->SpawnObjectEntity(lvl, o, Vec3(2.f*x + 1, 0, 2.f*y + o->size.y), PI);
							chest->AddItem(Item::Get("sword_long"));
							chest->AddItem(Item::Get("shield_wood"));
							chest->AddItem(Item::Get("al_leather"));
							chest->AddItem(Item::gold, Random(75, 100));
							chest->handler = &quest;
							quest.chests[0] = chest;
						}
						break;
					case 2:
						quest.dummy = Vec3(2.f*x + 1, 0, 2.f*y + 1);
						game_level->SpawnObjectEntity(lvl, BaseObject::Get("melee_target"), quest.dummy, PI / 2);
						break;
					case 3:
						{
							Unit* u = game_level->SpawnUnitNearLocation(lvl, Vec3(2.f*x + 1, 0, 2.f*y + 1), *UnitData::Get("tut_goblin"), nullptr, 1);
							u->SetRot(PI);
							u->event_handler = &quest;
						}
						break;
					case 4:
						{
							BaseObject* o = BaseObject::Get("chest");
							Chest* chest = game_level->SpawnObjectEntity(lvl, o, Vec3(2.f*x + 1, 0, 2.f*y + o->size.y), PI);
							chest->AddItem(Item::Get("bow_short"));
							chest->AddItem(Item::Get("p_hp"));
							chest->AddItem(Item::gold, Random(75, 100));
							chest->handler = &quest;
							quest.chests[1] = chest;
						}
						break;
					case 5:
						game_level->SpawnObjectEntity(lvl, BaseObject::Get("bow_target"), Vec3(2.f*x + 1, 0, 2.f*y + 1), -PI / 2);
						break;
					case 6:
						{
							Unit* u = game_level->SpawnUnitNearLocation(lvl, Vec3(2.f*x + 1, 0, 2.f*y + 1), *UnitData::Get("tut_czlowiek"), nullptr, 1);
							u->SetRot(PI);
						}
						break;
					default:
						assert(0);
						break;
					}
				}
				break;
			default:
				assert(0);
				break;
			}

			char c = mapa_t2[x + y * 22];
			if(c == ' ')
				p.room = -1;
			else if(InRange(c, '0', '9'))
				p.room = int(c - '0');
			else
				assert(0);
		}
	}

	// objects
	Tile::SetupFlags(lvl.map, Int2(22, 22), ENTRY_STAIRS_UP, ENTRY_STAIRS_DOWN);
	GenerateDungeonObjects();

	// doors
	for(Door* door : lvl.doors)
	{
		char c = mapa_t3[door->pt(22)];
		assert(InRange(c, '0', '9'));
		door->locked = LOCK_TUTORIAL + int(c - '0');
	}

	game_level->SpawnDungeonColliders();
	CreateMinimap();
	game_level->AddPlayerTeam(Vec3(2.f*start_tile.x + 1, 0, 2.f*start_tile.y + 1), 0);
}
