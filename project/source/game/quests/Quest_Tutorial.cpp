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
#include "ParticleSystem.h"
#include "Game.h"
#include "GlobalGui.h"

//=================================================================================================
void Quest_Tutorial::LoadLanguage()
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
	hd.Get(*game.gui->create_character->unit->human_data);
	game.NewGameCommon(game.gui->create_character->clas, game.gui->create_character->name.c_str(), hd, game.gui->create_character->cc, true);
	in_tutorial = true;
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
	game.gui->journal->GetNotes().push_back(txTutNote);

	// startowa lokacja
	SingleInsideLocation* loc = new SingleInsideLocation;
	loc->target = TUTORIAL_FORT;
	loc->name = txTutLoc;
	loc->type = L_DUNGEON;
	loc->image = LI_DUNGEON;
	W.StartInLocation(loc);
	L.dungeon_level = 0;

	game.loc_gen_factory->Get(L.location)->OnEnter();

	// go!
	game.LoadResources("", false);
	L.event_handler = nullptr;
	game.SetMusic();
	game.gui->load_screen->visible = false;
	game.gui->main_menu->visible = false;
	game.gui->game_gui->visible = true;
	game.gui->world_map->visible = false;
	game.clear_color = game.clear_color2;
	game.cam.Reset();
}

/*
state:
0 - pocz�tek
1 - pierwszy tekst
2 - tekst przed drzwiami
3 - tekst przed skrzyni�
4 - otwarto skrzyni�
5 - tekst przed manekinem
6 - uderzono manekin
7 - tekst przed goblinem
8 - zabito goblina
9 - tekst o dzienniku
10 - tekst o skrzyni z �ukiem
11 - otwarto skrzyni�
12 - tekst o �uku
13 - trafiono z �uku
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

		// sprawd� czy jest kolizja
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
				pc->Train(true, (int)SkillId::ONE_HANDED_WEAPON, TrainMode::Tutorial);
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
	finished_tutorial = true;
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
		Game::Get().pc->Train(true, (int)SkillId::BOW, TrainMode::Tutorial);
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
