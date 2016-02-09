#include "Pch.h"
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
	INT2 pos, size;
	bool corridor;
	int connected[2];
};

RoomInfo t_rooms[] = {
	INT2(0,19), INT2(6,3), false, {1,-1},
	INT2(0,14), INT2(8,5), false, {0, 2},
	INT2(8,14), INT2(5,5), false, {1, 3},
	INT2(13,13), INT2(7,7), false, {2, 4},
	INT2(13,4), INT2(7,9), false, {3, 5},
	INT2(14,0), INT2(5,5), false, {4, 6},
	INT2(8,0), INT2(6,5), false, {5, 7},
	INT2(10,4), INT2(3,4), true, {6, 8},
	INT2(2,8), INT2(11,6), false, {7, 9},
	INT2(2,1), INT2(7,7), false, {8,-1}
};

void Game::StartTutorial()
{
	HumanData hd;
	hd.Get(*create_character->unit->human_data);
	NewGameCommon(create_character->clas, create_character->name.c_str(), hd, create_character->cc, true);
	in_tutorial = true;
	tut_state = 0;
	ttexts.clear();
	tut_shield = nullptr;
	tut_shield2 = nullptr;
	contest_state = CONTEST_NOT_DONE;

	// ekwipunek
	pc->unit->ClearInventory();
	pc->unit->slots[SLOT_ARMOR] = FindItem("al_clothes");
	pc->unit->weight += pc->unit->slots[SLOT_ARMOR]->weight;
	pc->unit->gold = 10;
	notes.push_back(txTutNote);

	INT2 start_tile;

	// startowa lokacja
	SingleInsideLocation* loc = new SingleInsideLocation;
	loc->target = TUTORIAL_FORT;
	loc->name = txTutLoc;
	loc->type = L_DUNGEON;
	locations.push_back(loc);
	current_location = 0;
	open_location = 0;
	location = loc;
	city_ctx = nullptr;
	local_ctx_valid = true;
	InsideLocationLevel& lvl = loc->GetLevelData();
	lvl.w = lvl.h = 22;
	dungeon_level = 0;
	loc->SetActiveLevel(0);
	BaseLocation& base = g_base_locations[TUTORIAL_FORT];
	ApplyContext(loc, local_ctx);
	SetDungeonParamsAndTextures(base);

	// pokoje
	lvl.rooms.resize(countof(t_rooms));
	for(int i=0; i<countof(t_rooms); ++i)
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
	lvl.map = new Pole[22*22];
	for(int y=0; y<22; ++y)
	{
		for(int x=0; x<22; ++x)
		{
			Pole& p = lvl.map[x+y*22];
			p.flags = 0;
			switch(mapa_t[x+y*22])
			{
			case ' ':
				p.type = PUSTE;
				break;
			case '#':
				p.type = SCIANA;
				break;
			case '/':
				p.type = SCHODY_GORA;
				lvl.staircase_up = INT2(x,y);
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
					TutorialText& tt = Add1(ttexts);
					char c = mapa_t3[x+y*22];
					assert(in_range(c, '0', '9'));
					tt.id = int(c-'0');
					tt.state = (tt.id == 0 ? 1 : 0);
					tt.text = txTut[tt.id];
					tt.pos = VEC3(2.f*x+1,0,2.f*y+1);
				}
				break;
			case 'S':
				{
					p.type = PUSTE;
					char c = mapa_t3[x+y*22];
					assert(in_range(c, '0', '9'));
					switch((int)(c-'0'))
					{
					/*0 - start
					1 - skrzynia z broni¹,pancerzem,tarcz¹,z³otem [w dó³]
					2 - manekin obrócony w lewo
					3 - goblin obrócony w dó³
					4 - skrzynia z ³ukiem,mikstórk¹,z³otem [w dó³]
					5 - tarcza strzelniacza [w prawo]
					6 - cz³owiek [w dó³]*/
					case 0:
						start_tile = INT2(x,y);
						break;
					case 1:
						{
							Obj* o = FindObject("chest");
							Chest* c = (Chest*)SpawnObject(local_ctx, o, VEC3(2.f*x+1,0,2.f*y+o->size.y), PI);
							c->AddItem(FindItem("sword_long"));
							c->AddItem(FindItem("shield_wood"));
							c->AddItem(FindItem("al_leather"));
							c->AddItem(gold_item_ptr, random(75,100));
							c->handler = &tut_chest_handler;
						}
						break;
					case 2:
						tut_dummy = VEC3(2.f*x+1,0,2.f*y+1);
						SpawnObject(local_ctx, FindObject("melee_target"), tut_dummy, PI/2);
						break;
					case 3:
						{
							Unit* u = SpawnUnitNearLocation(local_ctx, VEC3(2.f*x+1,0,2.f*y+1), *FindUnitData("tut_goblin"), nullptr, 1);
							u->rot = PI;
							u->event_handler = &tut_unit_handler;
						}
						break;
					case 4:
						{
							Obj* o = FindObject("chest");
							Chest* c = (Chest*)SpawnObject(local_ctx, o, VEC3(2.f*x+1,0,2.f*y+o->size.y), PI);
							c->AddItem(FindItem("bow_short"));
							c->AddItem(FindItem("p_hp"));
							c->AddItem(gold_item_ptr, random(75,100));
							c->handler = &tut_chest_handler2;
						}
						break;
					case 5:
						{
							Object* o = SpawnObject(local_ctx, FindObject("bow_target"), VEC3(2.f*x+1,0,2.f*y+1), -PI/2);
							if(tut_shield)
								tut_shield2 = o;
							else
								tut_shield = o;
						}
						break;
					case 6:
						{
							Unit* u = SpawnUnitNearLocation(local_ctx, VEC3(2.f*x+1,0,2.f*y+1), *FindUnitData("tut_czlowiek"), nullptr, 1);
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

			char c = mapa_t2[x+y*22];
			if(c == ' ')
				p.room = -1;
			else if(in_range(c, '0', '9'))
				p.room = int(c-'0');
			else
				assert(0);
		}
	}

	// obiekty
	ustaw_flagi(lvl.map, 22);
	GenerateDungeonObjects2();
	GenerateDungeonObjects();

	// drzwi
	for(vector<Door*>::iterator it = local_ctx.doors->begin(), end = local_ctx.doors->end(); it != end; ++it)
	{
		char c = mapa_t3[(*it)->pt(22)];
		assert(in_range(c, '0', '9'));
		(*it)->locked = LOCK_SAMOUCZEK+int(c-'0');
	}

	// przedmioty na handel
	GenerateMerchantItems(chest_merchant, 500);

	// go!
	SpawnDungeonColliders();
	CreateDungeonMinimap();
	AddPlayerTeam(VEC3(2.f*start_tile.x+1,0,2.f*start_tile.y+1), 0, false, true);
	location_event_handler = nullptr;
	SetMusic();
	main_menu->visible = false;
	game_gui->visible = true;
	world_map->visible = false;
	clear_color = clear_color2;
	cam.Reset();
}

/*
tut_state:
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

void Game::UpdateTutorial()
{
	// atakowanie manekina
	if(pc->unit->action == A_ATTACK && pc->unit->animation_state == 1 && !pc->unit->hitted && pc->unit->ani->GetProgress2() >= pc->unit->GetAttackFrame(1) && distance(pc->unit->pos, tut_dummy) < 5.f)
	{
		Animesh::Point* hitbox, *point;
		hitbox = pc->unit->GetWeapon().mesh->FindPoint("hit");
		point = pc->unit->ani->ani->GetPoint(NAMES::point_weapon);

		BOX box1, box2;

		// oblicz macierz hitbox
		MATRIX m1, m2, m3;

		// transformacja postaci
		D3DXMatrixTranslation(&m1, pc->unit->pos);
		D3DXMatrixRotationY(&m2, pc->unit->rot);
		D3DXMatrixMultiply(&m1, &m2, &m1); // m1 (World) = Rot * Pos

		// transformacja punktu broni
		D3DXMatrixMultiply(&m2, &point->mat, &pc->unit->ani->mat_bones[point->bone]); // m2 = PointMatrix * BoneMatrix
		D3DXMatrixMultiply(&m3, &m2, &m1); // m3 = PointMatrix * BoneMatrix * UnitRot * UnitPos

		// transformacja hitboxa broni
		D3DXMatrixMultiply(&m1, &hitbox->mat, &m3); // m1 = BoxMatrix * PointMatrix * BoneMatrix * UnitRot * UnitPos

		// przy okazji stwórz obrócony BOX
		OOBBOX obox1, obox2;
		D3DXVec3TransformCoord(&obox1.pos, &VEC3(0,0,0), &m1);
		obox1.size = hitbox->size/2;
		obox1.rot = m1;

		// stwórz obrócony box
		obox2.pos = tut_dummy;
		obox2.pos.y += 1.f;
		obox2.size = VEC3(0.6f,2.f,0.6f);
		D3DXMatrixIdentity(&obox2.rot);

		VEC3 hitpoint;

		// sprawdŸ czy jest kolizja
		if(OrientedBoxToOrientedBox(obox1, obox2, &hitpoint))
		{
			pc->unit->hitted = true;
			ParticleEmitter* pe = new ParticleEmitter;
			pe->tex = tIskra;
			pe->emision_interval = 0.01f;
			pe->life = 5.f;
			pe->particle_life = 0.5f;
			pe->emisions = 1;
			pe->spawn_min = 10;
			pe->spawn_max = 15;
			pe->max_particles = 15;
			pe->pos = hitpoint;
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
			local_ctx.pes->push_back(pe);
			// jest kolizja
			if(sound_volume)
				PlaySound3d(GetMaterialSound(MAT_IRON, MAT_ROCK), hitpoint, 2.f, 10.f);
			if(tut_state == 5)
			{
				Train(*pc->unit, true, (int)Skill::ONE_HANDED_WEAPON, 1);
				tut_state = 6;
				int activate = 4;
				for(vector<TutorialText>::iterator it = ttexts.begin(), end = ttexts.end(); it != end; ++it)
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

	for(vector<TutorialText>::iterator it = ttexts.begin(), end = ttexts.end(); it != end; ++it)
	{
		if(it->state == 1 && distance(it->pos, pc->unit->pos) < 3.f)
		{
			DialogInfo info;
			info.event = nullptr;
			info.name = "tut";
			info.order = ORDER_TOP;
			info.parent = nullptr;
			info.pause = true;
			info.text = it->text;
			info.type = DIALOG_OK;
			GUI.ShowDialog(info);

			it->state = 2;

			int activate = -1,
				unlock = -1;

			switch(it->id)
			{
			case 0:
				activate = 1;
				if(tut_state < 1)
					tut_state = 1;
				break;
			case 1:
				unlock = 0;
				activate = 2;
				if(tut_state < 2)
					tut_state = 2;
				break;
			case 2:
				if(tut_state < 3)
					tut_state = 3;
				break;
			case 3:
				if(tut_state < 5)
					tut_state = 5;
				break;
			case 4:
				if(tut_state < 7)
					tut_state = 7;
				unlock = 2;
				break;
			case 5:
				if(tut_state < 9)
					tut_state = 9;
				unlock = 4;
				activate = 6;
				break;
			case 6:
				if(tut_state < 10)
					tut_state = 10;
				break;
			case 7:
				if(tut_state < 12)
					tut_state = 12;
				break;
			case 8:
				if(tut_state < 14)
					tut_state = 14;
				break;
			}

			if(activate != -1)
			{
				for(vector<TutorialText>::iterator it = ttexts.begin(), end = ttexts.end(); it != end; ++it)
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
				for(vector<Door*>::iterator it = local_ctx.doors->begin(), end = local_ctx.doors->end(); it != end; ++it)
				{
					if((*it)->locked == LOCK_SAMOUCZEK+unlock)
					{
						(*it)->locked = LOCK_NONE;
						break;
					}
				}
			}

			break;
		}
	}
}

void Game::EndOfTutorial(int)
{
	GUI.GetDialog("tut_end")->visible = false;
	ClearGame();
	StartNewGame();
}

void Game::TutEvent(int id)
{
	int activate = -1,
		unlock = -1;

	switch(id)
	{
	case 0:
		// otwarto skrzyniê
		if(tut_state < 4)
		{
			unlock = 1;
			activate = 3;
			tut_state = 4;
		}
		break;
	case 1:
		// otwarto drug¹ skrzyniê
		if(tut_state < 11)
		{
			unlock = 5;
			activate = 7;
			tut_state = 11;
		}
		break;
	case 2:
		// zabito goblina
		if(tut_state < 8)
		{
			unlock = 3;
			activate = 5;
			tut_state = 8;
		}
		break;
	case 3:
		{
			DialogInfo info;
			info.event = DialogEvent(this, &Game::EndOfTutorial);
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
		for(vector<TutorialText>::iterator it = ttexts.begin(), end = ttexts.end(); it != end; ++it)
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
		for(vector<Door*>::iterator it = local_ctx.doors->begin(), end = local_ctx.doors->end(); it != end; ++it)
		{
			if((*it)->locked == LOCK_SAMOUCZEK+unlock)
			{
				(*it)->locked = LOCK_NONE;
				break;
			}
		}
	}
}
