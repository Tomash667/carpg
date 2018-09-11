#include "Pch.h"
#include "GameCore.h"
#include "Quest_Tutorial.h"
#include "HumanData.h"
#include "QuestManager.h"
#include "Quest_Contest.h"
#include "CreateCharacterPanel.h"
#include "GameGui.h"
#include "Journal.h"
#include "LoadScreen.h"
#include "MainMenu.h"
#include "WorldMapGui.h"
#include "Language.h"
#include "SingleInsideLocation.h"
#include "World.h"
#include "Level.h"
#include "LocationGeneratorFactory.h"
#include "SoundManager.h"
#include "Game.h"


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


struct RoomInfo
{
	Int2 pos, size;
	bool corridor;
	int connected[2];
};

RoomInfo t_rooms[] = {
	Int2(0,19), Int2(6,3), false, {1,-1},
	Int2(0,14), Int2(8,5), false, {0, 2},
	Int2(8,14), Int2(5,5), false, {1, 3},
	Int2(13,13), Int2(7,7), false, {2, 4},
	Int2(13,4), Int2(7,9), false, {3, 5},
	Int2(14,0), Int2(5,5), false, {4, 6},
	Int2(8,0), Int2(6,5), false, {5, 7},
	Int2(10,4), Int2(3,4), true, {6, 8},
	Int2(2,8), Int2(11,6), false, {7, 9},
	Int2(2,1), Int2(7,7), false, {8,-1}
};


//=================================================================================================
void Quest_Tutorial::InitOnce()
{
	LoadArray(txTut, "tut");
	txTutNote = Str("tutNote");
	txTutLoc = Str("tutLoc");
}

//=================================================================================================
void Quest_Tutorial::Start()
{
	Game& game = Game::Get();

	game.LoadingStart(1);

	HumanData hd;
	hd.Get(*game.create_character->unit->human_data);
	game.NewGameCommon(game.create_character->clas, game.create_character->name.c_str(), hd, game.create_character->cc, true);
	game.in_tutorial = true;
	state = 0;
	texts.clear();
	shield = nullptr;
	shield2 = nullptr;
	QM.quest_contest->state = Quest_Contest::CONTEST_NOT_DONE;
	game.pc_data.autowalk = false;

	// ekwipunek
	game.pc->unit->ClearInventory();
	auto item = Item::Get("al_clothes");
	game.PreloadItem(item);
	game.pc->unit->slots[SLOT_ARMOR] = item;
	game.pc->unit->weight += game.pc->unit->slots[SLOT_ARMOR]->weight;
	game.pc->unit->gold = 10;
	game.game_gui->journal->GetNotes().push_back(txTutNote);

	Int2 start_tile;

	// startowa lokacja
	SingleInsideLocation* loc = new SingleInsideLocation;
	loc->target = TUTORIAL_FORT;
	loc->name = txTutLoc;
	loc->type = L_DUNGEON;
	loc->image = LI_DUNGEON;
	W.StartInLocation(loc);
	L.city_ctx = nullptr;
	InsideLocationLevel& lvl = loc->GetLevelData();
	lvl.w = lvl.h = 22;
	L.dungeon_level = 0;
	loc->SetActiveLevel(0);
	BaseLocation& base = g_base_locations[TUTORIAL_FORT];
	L.ApplyContext(loc, L.local_ctx);
	game.SetDungeonParamsAndTextures(base);

	// pokoje
	lvl.rooms.resize(countof(t_rooms));
	for(uint i = 0; i < countof(t_rooms); ++i)
	{
		RoomInfo& info = t_rooms[i];
		Room& r = lvl.rooms[i];
		r.target = (info.corridor ? RoomTarget::Corridor : RoomTarget::None);
		r.pos = info.pos;
		r.size = info.size;
		r.connected.push_back(info.connected[0]);
		if(info.connected[1] != -1)
			r.connected.push_back(info.connected[1]);
	}

	// map
	lvl.map = new Pole[22 * 22];
	for(int y = 0; y < 22; ++y)
	{
		for(int x = 0; x < 22; ++x)
		{
			Pole& p = lvl.map[x + y * 22];
			p.flags = 0;
			switch(mapa_t[x + y * 22])
			{
			case ' ':
				p.type = PUSTE;
				break;
			case '#':
				p.type = SCIANA;
				break;
			case '/':
				p.type = SCHODY_GORA;
				lvl.staircase_up = Int2(x, y);
				lvl.staircase_up_dir = 2;
				break;
			case '+':
				p.type = DRZWI;
				break;
			case '$':
				p.type = BLOKADA;
				break;
			case 'T':
				{
					p.type = PUSTE;
					Text& tt = Add1(texts);
					char c = mapa_t3[x + y * 22];
					assert(InRange(c, '0', '9'));
					tt.id = int(c - '0');
					tt.state = (tt.id == 0 ? 1 : 0);
					tt.text = txTut[tt.id];
					tt.pos = Vec3(2.f*x + 1, 0, 2.f*y + 1);
				}
				break;
			case 'S':
				{
					p.type = PUSTE;
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
							Chest* chest = L.SpawnObjectEntity(L.local_ctx, o, Vec3(2.f*x + 1, 0, 2.f*y + o->size.y), PI);
							chest->AddItem(Item::Get("sword_long"));
							chest->AddItem(Item::Get("shield_wood"));
							chest->AddItem(Item::Get("al_leather"));
							chest->AddItem(Item::gold, Random(75, 100));
							chest->handler = this;
							chests[0] = chest;
						}
						break;
					case 2:
						dummy = Vec3(2.f*x + 1, 0, 2.f*y + 1);
						L.SpawnObjectEntity(L.local_ctx, BaseObject::Get("melee_target"), dummy, PI / 2);
						break;
					case 3:
						{
							Unit* u = L.SpawnUnitNearLocation(L.local_ctx, Vec3(2.f*x + 1, 0, 2.f*y + 1), *UnitData::Get("tut_goblin"), nullptr, 1);
							u->rot = PI;
							u->event_handler = this;
						}
						break;
					case 4:
						{
							BaseObject* o = BaseObject::Get("chest");
							Chest* chest = L.SpawnObjectEntity(L.local_ctx, o, Vec3(2.f*x + 1, 0, 2.f*y + o->size.y), PI);
							chest->AddItem(Item::Get("bow_short"));
							chest->AddItem(Item::Get("p_hp"));
							chest->AddItem(Item::gold, Random(75, 100));
							chest->handler = this;
							chests[1] = chest;
						}
						break;
					case 5:
						{
							Object* o = L.SpawnObjectEntity(L.local_ctx, BaseObject::Get("bow_target"), Vec3(2.f*x + 1, 0, 2.f*y + 1), -PI / 2);
							if(shield)
								shield2 = o;
							else
								shield = o;
						}
						break;
					case 6:
						{
							Unit* u = L.SpawnUnitNearLocation(L.local_ctx, Vec3(2.f*x + 1, 0, 2.f*y + 1), *UnitData::Get("tut_czlowiek"), nullptr, 1);
							u->rot = PI;
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

	// obiekty
	Pole::SetupFlags(lvl.map, Int2(22, 22));
	game.GenerateDungeonObjects2();
	game.GenerateDungeonObjects();

	// drzwi
	for(vector<Door*>::iterator it = L.local_ctx.doors->begin(), end = L.local_ctx.doors->end(); it != end; ++it)
	{
		char c = mapa_t3[(*it)->pt(22)];
		assert(InRange(c, '0', '9'));
		(*it)->locked = LOCK_TUTORIAL + int(c - '0');
	}

	// przedmioty na handel
	game.GenerateMerchantItems(game.chest_merchant, 500);

	// go!
	game.LoadResources("", false);
	game.SpawnDungeonColliders();
	game.loc_gen_factory->Get(L.location)->CreateMinimap();
	game.AddPlayerTeam(Vec3(2.f*start_tile.x + 1, 0, 2.f*start_tile.y + 1), 0, false, true);
	L.event_handler = nullptr;
	game.SetMusic();
	game.load_screen->visible = false;
	game.main_menu->visible = false;
	game.game_gui->visible = true;
	game.world_map->visible = false;
	game.clear_color = game.clear_color2;
	game.cam.Reset();
}

/*
state:
0 - pocz¹tek
1 - pierwszy tekst
2 - tekst przed drzwiami
3 - tekst przed skrzyni¹
4 - otwarto skrzyniê
5 - tekst przed manekinem
6 - uderzono manekin
7 - tekst przed goblinem
8 - zabito goblina
9 - tekst o dzienniku
10 - tekst o skrzyni z ³ukiem
11 - otwarto skrzyniê
12 - tekst o ³uku
13 - trafiono z ³uku
14 - tekst o gadaniu
*/

//=================================================================================================
void Quest_Tutorial::Update()
{
	Game& game = Game::Get();
	PlayerController* pc = game.pc;

	// atakowanie manekina
	if(pc->unit->action == A_ATTACK && pc->unit->animation_state == 1 && !pc->unit->hitted && pc->unit->mesh_inst->GetProgress2() >= pc->unit->GetAttackFrame(1)
		&& Vec3::Distance(pc->unit->pos, dummy) < 5.f)
	{
		Mesh::Point* hitbox, *point;
		hitbox = pc->unit->GetWeapon().mesh->FindPoint("hit");
		point = pc->unit->mesh_inst->mesh->GetPoint(NAMES::point_weapon);

		Obbox obox1, obox2;

		// calculate hitbox matrix
		Matrix m_unit = Matrix::RotationY(pc->unit->rot) * Matrix::Translation(pc->unit->pos);
		Matrix m_weapon = point->mat * pc->unit->mesh_inst->mat_bones[point->bone] * m_unit;
		Matrix m_hitbox = hitbox->mat * m_weapon;

		// create weapon hitbox oriented bounding box
		obox1.pos = Vec3::TransformZero(m_hitbox);
		obox1.size = hitbox->size / 2;
		obox1.rot = m_hitbox;

		// create dummy oriented bounding box
		obox2.pos = dummy;
		obox2.pos.y += 1.f;
		obox2.size = Vec3(0.6f, 2.f, 0.6f);
		obox2.rot = Matrix::IdentityMatrix;

		Vec3 hitpoint;

		// sprawdŸ czy jest kolizja
		if(OrientedBoxToOrientedBox(obox1, obox2, &hitpoint))
		{
			pc->unit->hitted = true;
			ParticleEmitter* pe = new ParticleEmitter;
			pe->tex = game.tIskra;
			pe->emision_interval = 0.01f;
			pe->life = 5.f;
			pe->particle_life = 0.5f;
			pe->emisions = 1;
			pe->spawn_min = 10;
			pe->spawn_max = 15;
			pe->max_particles = 15;
			pe->pos = hitpoint;
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
			L.local_ctx.pes->push_back(pe);
			// jest kolizja
			game.sound_mgr->PlaySound3d(game.GetMaterialSound(MAT_IRON, MAT_ROCK), hitpoint, 2.f, 10.f);
			if(state == 5)
			{
				game.Train(*pc->unit, true, (int)SkillId::ONE_HANDED_WEAPON, 1);
				state = 6;
				int activate = 4;
				for(vector<Text>::iterator it = texts.begin(), end = texts.end(); it != end; ++it)
				{
					if(it->id == activate)
					{
						it->state = 1;
						break;
					}
				}
			}
		}
	}

	// check tutorial texts
	for(Text& text : texts)
	{
		if(text.state != 1 || Vec3::Distance(text.pos, pc->unit->pos) > 3.f)
			continue;

		DialogInfo info;
		info.event = nullptr;
		info.name = "tut";
		info.order = ORDER_TOP;
		info.parent = nullptr;
		info.pause = true;
		info.text = text.text;
		info.type = DIALOG_OK;
		GUI.ShowDialog(info);

		text.state = 2;

		int activate = -1,
			unlock = -1;

		switch(text.id)
		{
		case 0:
			activate = 1;
			if(state < 1)
				state = 1;
			break;
		case 1:
			unlock = 0;
			activate = 2;
			if(state < 2)
				state = 2;
			break;
		case 2:
			if(state < 3)
				state = 3;
			break;
		case 3:
			if(state < 5)
				state = 5;
			break;
		case 4:
			if(state < 7)
				state = 7;
			unlock = 2;
			break;
		case 5:
			if(state < 9)
				state = 9;
			unlock = 4;
			activate = 6;
			break;
		case 6:
			if(state < 10)
				state = 10;
			break;
		case 7:
			if(state < 12)
				state = 12;
			break;
		case 8:
			if(state < 14)
				state = 14;
			break;
		}

		if(activate != -1)
		{
			for(Text& to_activate : texts)
			{
				if(to_activate.id == activate)
				{
					to_activate.state = 1;
					break;
				}
			}
		}

		if(unlock != -1)
		{
			for(Door* door : *L.local_ctx.doors)
			{
				if(door->locked == LOCK_TUTORIAL + unlock)
				{
					door->locked = LOCK_NONE;
					break;
				}
			}
		}

		break;
	}
}

//=================================================================================================
void Quest_Tutorial::Finish(int)
{
	Game& game = Game::Get();
	GUI.GetDialog("tut_end")->visible = false;
	game.finished_tutorial = true;
	game.ClearGame();
	game.StartNewGame();
}

//=================================================================================================
void Quest_Tutorial::OnEvent(Event event)
{
	int activate = -1,
		unlock = -1;

	switch(event)
	{
	case OpenedChest1:
		if(state < 4)
		{
			unlock = 1;
			activate = 3;
			state = 4;
		}
		break;
	case OpenedChest2:
		if(state < 11)
		{
			unlock = 5;
			activate = 7;
			state = 11;
		}
		break;
	case KilledGoblin:
		if(state < 8)
		{
			unlock = 3;
			activate = 5;
			state = 8;
		}
		break;
	case Exit:
		{
			DialogInfo info;
			info.event = DialogEvent(this, &Quest_Tutorial::Finish);
			info.name = "tut_end";
			info.order = ORDER_TOP;
			info.parent = nullptr;
			info.pause = true;
			info.text = txTut[9];
			info.type = DIALOG_OK;
			GUI.ShowDialog(info);
		}
		break;
	default:
		assert(0);
		break;
	}

	if(activate != -1)
	{
		for(vector<Text>::iterator it = texts.begin(), end = texts.end(); it != end; ++it)
		{
			if(it->id == activate)
			{
				it->state = 1;
				break;
			}
		}
	}

	if(unlock != -1)
	{
		for(vector<Door*>::iterator it = L.local_ctx.doors->begin(), end = L.local_ctx.doors->end(); it != end; ++it)
		{
			if((*it)->locked == LOCK_TUTORIAL + unlock)
			{
				(*it)->locked = LOCK_NONE;
				break;
			}
		}
	}
}

//=================================================================================================
void Quest_Tutorial::HandleChestEvent(ChestEventHandler::Event event, Chest* chest)
{
	OnEvent(chest == chests[0] ? OpenedChest1 : OpenedChest2);
}

//=================================================================================================
void Quest_Tutorial::HandleUnitEvent(UnitEventHandler::TYPE event, Unit* unit)
{
	OnEvent(KilledGoblin);
}

//=================================================================================================
void Quest_Tutorial::HandleBulletCollision(void* ptr)
{
	if((ptr == shield || ptr == shield2) && state == 12)
	{
		Game& game = Game::Get();
		game.Train(*game.pc->unit, true, (int)SkillId::BOW, 1);
		state = 13;
		int unlock = 6;
		int activate = 8;
		for(vector<Text>::iterator it = texts.begin(), end = texts.end(); it != end; ++it)
		{
			if(it->id == activate)
			{
				it->state = 1;
				break;
			}
		}
		for(vector<Door*>::iterator it = L.local_ctx.doors->begin(), end = L.local_ctx.doors->end(); it != end; ++it)
		{
			if((*it)->locked == LOCK_TUTORIAL + unlock)
			{
				(*it)->locked = LOCK_NONE;
				break;
			}
		}
	}
}
