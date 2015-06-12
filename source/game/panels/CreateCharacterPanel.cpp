// character creation screen
#include "Pch.h"
#include "Base.h"
#include "CreateCharacterPanel.h"
#include "Game.h"
#include "Language.h"
#include "GetTextDialog.h"
#include "Language.h"
#include "PickItemDialog.h"

//-----------------------------------------------------------------------------
const int SECTION_H = 40;
const int VALUE_H = 20;

//-----------------------------------------------------------------------------
enum ButtonId
{
	IdCancel = GuiEvent_Custom,
	IdNext,
	IdBack,
	IdCreate,
	IdHardcore,
	IdHair,
	IdMustache,
	IdBeard,
	IdColor,
	IdSize
};

//=================================================================================================
CreateCharacterPanel::CreateCharacterPanel(DialogInfo& info) : Dialog(info), unit(NULL)
{
	txHardcoreMode = Str("hardcoreMode");
	txHair = Str("hair");
	txMustache = Str("mustache");
	txBeard = Str("beard");
	txHairColor = Str("hairColor");
	txSize = Str("size");
	txCharacterCreation = Str("characterCreation");
	txName = Str("name");
	txAttributes = Str("attributes");
	txRelatedAttributes = Str("relatedAttributes");
	txCreateCharWarn = Str("createCharWarn");
	txSkillPoints = Str("skillPoints");
	txPerkPoints = Str("perkPoints");
	txPickAttribIncrase = Str("pickAttribIncrase");
	txPickAttribDecrase = Str("pickAttribDecrase");
	txPickTwoSkillsDecrase = Str("pickTwoSkillsDecrase");
	txPickSkillIncrase = Str("pickSkillIncrase");
	txAvailablePerks = Str("availablePerks");
	txUnavailablePerks = Str("unavailablePerks");
	txTakenPerks = Str("takenPerks");
	txIncrasedAttrib = Str("txIncrasedAttrib");
	txIncrasedSkill = Str("txIncrasedSkill"); 
	txDecrasedAttrib = Str("txDecrasedAttrib"); 
	txDecrasedSkill = Str("txDecrasedSkill"); 
	txDecrasedSkills = Str("txDecrasedSkills"); 
	txCreateCharTooMany = Str("txCreateCharTooMany");

	size = INT2(600,500);
	unit = new Unit;
	unit->type = Unit::HUMAN;
	unit->human_data = new Human;
	unit->player = NULL;
	unit->ai = NULL;
	unit->hero = NULL;
	unit->used_item = NULL;
	unit->stan_broni = BRON_SCHOWANA;
	unit->pos = unit->visual_pos = VEC3(0,0,0);
	unit->rot = 0.f;

	btCancel.id = IdCancel;
	btCancel.custom = &custom_x;
	btCancel.size = INT2(32, 32);
	btCancel.parent = this;
	btCancel.pos = INT2(size.x - 32 - 16, 16);

	btBack.id = IdBack;
	btBack.text = Str("goBack");
	btBack.size = INT2(100, 44);
	btBack.parent = this;
	btBack.pos = INT2(16, size.y - 60);

	btNext.id = IdNext;
	btNext.text = Str("next");
	btNext.size = INT2(100, 44);
	btNext.parent = this;
	btNext.pos = INT2(size.x - 100 - 16, size.y - 60);

	btCreate.id = IdCreate;
	btCreate.text = Str("create");
	btCreate.size = INT2(100, 44);
	btCreate.parent = this;
	btCreate.pos = INT2(size.x - 100 - 16, size.y - 60);

	checkbox.bt_size = INT2(32,32);
	checkbox.checked = false;
	checkbox.id = IdHardcore;
	checkbox.parent = this;
	checkbox.text = txHardcoreMode;
	checkbox.pos = INT2(20, 350);
	checkbox.size = INT2(200,32);

	{
		Slider2& s = slider[0];
		s.id = IdHair;
		s.minv = 0;
		s.maxv = MAX_HAIR-1;
		s.val = 0;
		s.text = txHair;
		s.pos = INT2(20,100);
		s.parent = this;
	}

	{
		Slider2& s = slider[1];
		s.id = IdMustache;
		s.minv = 0;
		s.maxv = MAX_MUSTACHE-1;
		s.val = 0;
		s.text = txMustache;
		s.pos = INT2(20,150);
		s.parent = this;
	}

	{
		Slider2& s = slider[2];
		s.id = IdBeard;
		s.minv = 0;
		s.maxv = MAX_BEARD-1;
		s.val = 0;
		s.text = txBeard;
		s.pos = INT2(20,200);
		s.parent = this;
	}

	{
		Slider2& s = slider[3];
		s.id = IdColor;
		s.minv = 0;
		s.maxv = n_hair_colors-1;
		s.val = 0;
		s.text = txHairColor;
		s.pos = INT2(20,250);
		s.parent = this;
	}

	{
		Slider2& s = slider[4];
		s.id = IdSize;
		s.minv = 0;
		s.maxv = 100;
		s.val = 50;
		s.text = txSize;
		s.pos = INT2(20,300);
		s.parent = this;
		s.SetHold(true);
		s.hold_val = 25.f;
	}

	lbClasses.pos = INT2(16, 73-18);
	lbClasses.size = INT2(198, 235+18);
	lbClasses.SetForceImageSize(INT2(20, 20));
	lbClasses.SetItemHeight(24);
	lbClasses.event_handler = DialogEvent(this, &CreateCharacterPanel::OnChangeClass);
	lbClasses.parent = this;

	tbClassDesc.pos = INT2(130, 335);
	tbClassDesc.size = INT2(341, 93);
	tbClassDesc.readonly = true;
	tbClassDesc.AddScrollbar();

	tbInfo.pos = INT2(130, 335);
	tbInfo.size = INT2(341, 93);
	tbInfo.readonly = true;
	tbInfo.text = Str("createCharText");
	tbInfo.AddScrollbar();

	flow_pos = INT2(368, 73-18);
	flow_size = INT2(198, 235+18);
	flow_scroll.pos = INT2(flow_pos.x+flow_size.x+2, flow_pos.y);
	flow_scroll.size = INT2(16, flow_size.y);
	flow_scroll.total = 100;
	flow_scroll.part = 10;

	tooltip.Init(TooltipGetText(this, &CreateCharacterPanel::GetTooltip));

	flowSkills.size = INT2(198, 235+18);
	flowSkills.pos = INT2(16, 73-18);
	flowSkills.button_size = INT2(16, 16);
	flowSkills.button_tex = custom_bt;
	flowSkills.on_button = ButtonEvent(this, &CreateCharacterPanel::OnPickSkill);

	flowPerks.size = INT2(198, 235+18);
	flowPerks.pos = INT2(size.x - flowPerks.size.x - 16, 73-18);
	flowPerks.button_size = INT2(16, 16);
	flowPerks.button_tex = custom_bt;
	flowPerks.on_button = ButtonEvent(this, &CreateCharacterPanel::OnPickPerk);
}

//=================================================================================================
CreateCharacterPanel::~CreateCharacterPanel()
{
	delete unit;
}

//=================================================================================================
void CreateCharacterPanel::Draw(ControlDrawData*)
{
	// background
	GUI.DrawSpriteFull(tBackground, COLOR_RGBA(255,255,255,128));

	// panel
	GUI.DrawItem(tDialog, global_pos, size, COLOR_RGBA(255,255,255,222), 16);

	// top text
	RECT rect0 = {12+pos.x, 12+pos.y, pos.x+size.x-12, 12+pos.y+72};
	GUI.DrawText(GUI.fBig, txCharacterCreation, DT_NOCLIP|DT_CENTER, BLACK, rect0);

	// character
	GUI.DrawSprite(game->tChar, INT2(pos.x+228,pos.y+64));

	// close button
	btCancel.Draw();

	RECT rect;
	MATRIX mat;

	switch(mode)
	{
	case Mode::PickClass:
		{
			btNext.Draw();

			// class list
			lbClasses.Draw();

			// class desc
			tbClassDesc.Draw();

			// attribute/skill flow panel
			INT2 fpos = flow_pos + global_pos;
			GUI.DrawItem(GUI.tBox, fpos, flow_size, WHITE, 8, 32);
			flow_scroll.Draw();

			rect.left = fpos.x + 2;
			rect.right = rect.left + flow_size.x - 4;
			rect.top = fpos.y + 2;
			rect.bottom = rect.top + flow_size.y - 4;

			RECT r = { rect.left, rect.top, rect.right, rect.top + 20 },
				part = { 0, 0, 256, 32 };

			for(FlowItem& fi : flow_items)
			{
				r.top = fpos.y + fi.y - (int)flow_scroll.offset;
				cstring text = GetText(fi.group, fi.id);
				if(fi.section)
				{
					r.bottom = r.top + SECTION_H;
					if(!GUI.DrawText(GUI.fBig, text, DT_SINGLELINE, BLACK, r, &rect))
						break;
				}
				else
				{
					if(fi.part > 0)
					{
						D3DXMatrixTransformation2D(&mat, NULL, 0.f, &VEC2(float(flow_size.x - 4) / 256, 17.f / 32), NULL, 0.f, &VEC2(float(r.left), float(r.top)));
						part.right = int(fi.part * 256);
						GUI.DrawSprite2(game->tKlasaCecha, &mat, &part, &rect, WHITE);
					}
					r.bottom = r.top + VALUE_H;
					if(!GUI.DrawText(GUI.default_font, text, DT_SINGLELINE, BLACK, r, &rect))
						break;
				}
			}

			tooltip.Draw();
		}
		break;
	case Mode::PickSkillPerk:
		{
			btNext.Draw();
			btBack.Draw();

			flowSkills.Draw();
			flowPerks.Draw();

			tbInfo.Draw();

			// left text "Skill points: X/Y"
			RECT r = { global_pos.x + 16, global_pos.y + 310, global_pos.x + 216, global_pos.y + 360 };
			GUI.DrawText(GUI.default_font, Format(txSkillPoints, c.sp, c.sp_max), 0, BLACK, r);

			// right text "Perks: X/Y"
			RECT r2 = { global_pos.x + size.x - 216, global_pos.y + 310, global_pos.x + size.x - 16, global_pos.y + 360 };
			GUI.DrawText(GUI.default_font, Format(txPerkPoints, c.perks, c.perks_max), DT_RIGHT, BLACK, r2);

			tooltip.Draw();
		}
		break;
	case Mode::PickAppearance:
		{
			btCreate.Draw();
			btBack.Draw();

			if(!game->IsOnline())
				checkbox.Draw();

			for(int i = 0; i<5; ++i)
				slider[i].Draw();
		}
		break;
	}
}

//=================================================================================================
void CreateCharacterPanel::Update(float dt)
{
	RenderUnit();
	UpdateUnit(dt);

	// rotating unit
	if(PointInRect(GUI.cursor_pos, INT2(pos.x+228, pos.y+94), INT2(128, 256)) && Key.Focus() && focus)
	{
		bool rotate = false;
		if(rotating)
		{
			if(Key.Down(VK_LBUTTON))
				rotate = true;
			else
				rotating = false;
		}
		else if(Key.Pressed(VK_LBUTTON))
		{
			rotate = true;
			rotating = true;
		}

		if(rotate)
			unit->rot = clip(unit->rot - float(GUI.cursor_pos.x - pos.x - 228 - 64)/16*dt);
	}
	else
		rotating = false;

	// x button
	btCancel.mouse_focus = focus;
	btCancel.Update(dt);

	switch(mode)
	{
	case Mode::PickClass:
		{
			lbClasses.mouse_focus = focus;
			lbClasses.Update(dt);

			btNext.mouse_focus = focus;
			btNext.Update(dt);

			tbClassDesc.mouse_focus = focus;
			tbClassDesc.Update(dt);

			flow_scroll.mouse_focus = focus;
			flow_scroll.Update(dt);

			int group = -1, id = -1;

			if(!flow_scroll.clicked && PointInRect(GUI.cursor_pos, flow_pos + global_pos, flow_size))
			{
				int y = GUI.cursor_pos.y - global_pos.y - flow_pos.y + (int)flow_scroll.offset;
				for(FlowItem& fi : flow_items)
				{
					if(y >= fi.y && y <= fi.y + 20)
					{
						group = fi.group;
						id = fi.id;
						break;
					}
					else if(y < fi.y)
						break;
				}
			}

			tooltip.Update(dt, group, id);
		}
		break;
	case Mode::PickSkillPerk:
		{
			btBack.mouse_focus = focus;
			btBack.Update(dt);

			btNext.mouse_focus = focus;
			btNext.Update(dt);

			tbInfo.mouse_focus = focus;
			tbInfo.Update(dt);

			int group = -1, id = -1;

			flowSkills.mouse_focus = focus;
			flowSkills.Update(dt);
			flowSkills.GetSelected(group, id);

			flowPerks.mouse_focus = focus;
			flowPerks.Update(dt);
			flowPerks.GetSelected(group, id);

			tooltip.Update(dt, group, id);
		}
		break;
	case Mode::PickAppearance:
		{
			btBack.mouse_focus = focus;
			btBack.Update(dt);

			btCreate.mouse_focus = focus;
			btCreate.Update(dt);

			if(!game->IsOnline())
			{
				checkbox.mouse_focus = focus;
				checkbox.Update(dt);
			}

			for(int i = 0; i<5; ++i)
			{
				slider[i].mouse_focus = focus;
				slider[i].Update(dt);
			}
		}
		break;
	}

	if(focus && Key.Focus() && Key.PressedRelease(VK_ESCAPE))
		Event((GuiEvent)IdCancel);
}

//=================================================================================================
void CreateCharacterPanel::Event(GuiEvent e)
{
	if(e == GuiEvent_Show || e == GuiEvent_WindowResize)
	{
		if(e == GuiEvent_Show)
		{
			visible = true;
			unit->rot = 0;
			dist = -2.5f;
		}
		pos = global_pos = (GUI.wnd_size - size)/2;
		btCancel.global_pos = global_pos + btCancel.pos;
		btBack.global_pos = global_pos + btBack.pos;
		btNext.global_pos = global_pos + btNext.pos;
		btCreate.global_pos = global_pos + btCreate.pos;
		for(int i = 0; i < 5; ++i)
			slider[i].global_pos = global_pos + slider[i].pos;			
		checkbox.global_pos = global_pos + checkbox.pos;
		lbClasses.Event(GuiEvent_Moved);
		tbClassDesc.Move(pos);
		tbInfo.Move(pos);
		flow_scroll.global_pos = global_pos + flow_scroll.pos;
		flowSkills.UpdatePos(global_pos);
		flowPerks.UpdatePos(global_pos);
	}
	else if(e == GuiEvent_Close)
	{
		visible = false;
		lbClasses.Event(GuiEvent_LostFocus);
		tbClassDesc.LostFocus();
		flow_scroll.LostFocus();
	}
	else if(e == GuiEvent_LostFocus)
	{
		lbClasses.Event(GuiEvent_LostFocus);
		tbClassDesc.LostFocus();
		flow_scroll.LostFocus();
	}
	else if(e >= GuiEvent_Custom)
	{
		switch(e)
		{
		case IdCancel:
			CloseDialog();
			event(BUTTON_CANCEL);
			break;
		case IdNext:
			if(mode == Mode::PickClass)
			{
				mode = Mode::PickSkillPerk;
				if(reset_skills_perks)
					ResetSkillsPerks();
			}
			else
			{
				if(c.sp < 0 || c.perks < 0)
					GUI.SimpleDialog(txCreateCharTooMany, this);
				else if(c.sp != 0 || c.perks != 0)
				{
					DialogInfo di;
					di.event = DialogEvent(this, &CreateCharacterPanel::OnShowWarning);
					di.name = "create_char_warn";
					di.order = ORDER_TOP;
					di.parent = this;
					di.pause = false;
					di.text = txCreateCharWarn;
					di.type = DIALOG_YESNO;
					GUI.ShowDialog(di);
				}
				else
					mode = Mode::PickAppearance;
			}
			break;
		case IdBack:
			if(mode == Mode::PickSkillPerk)
				mode = Mode::PickClass;
			else
				mode = Mode::PickSkillPerk;
			break;
		case IdCreate:
			if(enter_name)
			{
				GetTextDialogParams params(Format("%s:", txName), name);
				params.event = DialogEvent(this, &CreateCharacterPanel::OnEnterName);
				params.limit = 16;
				params.parent = this;
				GetTextDialog::Show(params);
			}
			else
			{
				CloseDialog();
				event(BUTTON_OK);
			}
			break;
		case IdHardcore:
			game->hardcore_option = checkbox.checked;
			game->cfg.Add("hardcore", game->hardcore_option ? "1" : "0");
			game->SaveCfg();
			break;
		case IdHair:
			unit->human_data->hair = slider[0].val-1;
			slider[0].text = Format("%s %d/%d", txHair, slider[0].val, slider[0].maxv);
			break;
		case IdMustache:
			unit->human_data->mustache = slider[1].val-1;
			slider[1].text = Format("%s %d/%d", txMustache, slider[1].val, slider[1].maxv);
			break;
		case IdBeard:
			unit->human_data->beard = slider[2].val-1;
			slider[2].text = Format("%s %d/%d", txBeard, slider[2].val, slider[2].maxv);
			break;
		case IdColor:
			unit->human_data->hair_color = g_hair_colors[slider[3].val];
			slider[3].text = Format("%s %d/%d", txHairColor, slider[3].val, slider[3].maxv);
			break;
		case IdSize:
			height = slider[4].val;
			unit->human_data->height = lerp(0.9f,1.1f,float(slider[4].val)/100);
			slider[4].text = Format("%s %d/%d", txSize, slider[4].val, slider[4].maxv);
			unit->human_data->ApplyScale(unit->ani->ani);
			unit->ani->need_update = true;
			break;
		}
	}
}

//=================================================================================================
void CreateCharacterPanel::InitInventory()
{
	Unit& u = *unit;
	u.ClearInventory();
	game->ParseItemScript(u, u.data->items);
	anim = DA_STOI;
	t = 1.f;
}

//=================================================================================================
void CreateCharacterPanel::OnEnterName(int id)
{
	if(id == BUTTON_OK)
	{
		CloseDialog();
		event(BUTTON_OK);
	}
}

//=================================================================================================
void CreateCharacterPanel::RenderUnit()
{
	// rysuj obrazek
	HRESULT hr = game->device->TestCooperativeLevel();
	if(hr != D3D_OK)
		return;

	game->SetAlphaBlend(false);
	game->SetAlphaTest(false);
	game->SetNoCulling(false);
	game->SetNoZWrite(false);

	// ustaw render target
	SURFACE surf = NULL;
	if(game->sChar)
		V( game->device->SetRenderTarget(0, game->sChar) );
	else
	{
		V( game->tChar->GetSurfaceLevel(0, &surf) );
		V( game->device->SetRenderTarget(0, surf) );
	}

	// pocz¹tek renderowania
	V( game->device->Clear(0, NULL, D3DCLEAR_ZBUFFER | D3DCLEAR_TARGET, 0, 1.f, 0) );
	V( game->device->BeginScene() );

	static vector<Lights> lights;

	game->SetOutsideParams();

	MATRIX matView, matProj;
	D3DXMatrixLookAtLH(&matView, &VEC3(0.f,2.f,dist), &VEC3(0.f,1.f,0.f), &VEC3(0,1,0));
	D3DXMatrixPerspectiveFovLH(&matProj, PI/4, 0.5f, 1.f, 5.f);
	game->tmp_matViewProj = matView * matProj;
	D3DXMatrixInverse(&game->tmp_matViewInv, NULL, &matView);

	game->camera_frustum.Set(game->tmp_matViewProj);
	game->ListDrawObjectsUnit(NULL, game->camera_frustum, true, *unit);
	game->DrawSceneNodes(game->draw_batch.nodes, lights, true);
	game->draw_batch.Clear();

	// koniec renderowania
	V( game->device->EndScene() );

	// kopiuj jeœli jest mipmaping
	if(game->sChar)
	{
		V( game->tChar->GetSurfaceLevel(0, &surf) );
		V( game->device->StretchRect(game->sChar, NULL, surf, NULL, D3DTEXF_NONE) );
	}
	surf->Release();

	// przywróc poprzedni render target
	V( game->device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &surf) );
	V( game->device->SetRenderTarget(0, surf) );
	surf->Release();
}

//=================================================================================================
void CreateCharacterPanel::UpdateUnit(float dt)
{
	// aktualizacja postaci	
	t -= dt;
	if(t <= 0.f)
	{
		switch(anim)
		{
		case DA_ATAK:
		case DA_SCHOWAJ_BRON:
		case DA_SCHOWAJ_LUK:
		case DA_STRZAL:
		case DA_WYJMIJ_BRON:
		case DA_WYJMIJ_LUK:
		case DA_ROZGLADA:
			assert(0);
			break;
		case DA_BLOK:
			if(rand2()%2 == 0)
				anim = DA_ATAK;
			else
				anim = DA_BOJOWY;
			break;
		case DA_BOJOWY:
			if(unit->wyjeta == B_JEDNORECZNA)
			{
				int co = rand2()%(unit->HaveShield() ? 3 : 2);
				if(co == 0)
					anim = DA_ATAK;
				else if(co == 1)
					anim = DA_SCHOWAJ_BRON;
				else
					anim = DA_BLOK;
			}
			else
			{
				if(rand2()%2 == 0)
					anim = DA_STRZAL;
				else
					anim = DA_SCHOWAJ_LUK;
			}
			break;
		case DA_STOI:
		case DA_IDZIE:
			{
				int co = rand2()%(unit->HaveBow() ? 5 : 4);
				if(co == 0)
					anim = DA_ROZGLADA;
				else if(co == 1)
					anim = DA_STOI;
				else if(co == 2)
					anim = DA_IDZIE;
				else if(co == 3)
					anim = DA_WYJMIJ_BRON;
				else
					anim = DA_WYJMIJ_LUK;
			}
			break;
		default:
			assert(0);
			break;
		}
	}

	if(anim != anim2)
	{
		anim2 = anim;
		switch(anim)
		{
		case DA_ATAK:
			unit->attack_id = unit->GetRandomAttack();
			unit->ani->Play(NAMES::ani_attacks[unit->attack_id], PLAY_PRIO2|PLAY_ONCE, 0);
			unit->ani->groups[0].speed = unit->GetAttackSpeed();
			unit->etap_animacji = 0;
			t = 100.f;
			unit->ani->frame_end_info = false;
			break;
		case DA_BLOK:
			unit->ani->Play(NAMES::ani_block, PLAY_PRIO2, 0);
			t = 1.f;
			break;
		case DA_BOJOWY:
			unit->ani->Play(NAMES::ani_battle, PLAY_PRIO2, 0);
			t = 1.f;
			break;
		case DA_IDZIE:
			unit->ani->Play(NAMES::ani_move, PLAY_PRIO2, 0);
			t = 2.f;
			break;
		case DA_ROZGLADA:
			unit->ani->Play("rozglada", PLAY_PRIO2|PLAY_ONCE, 0);
			t = 100.f;
			unit->ani->frame_end_info = false;
			break;
		case DA_SCHOWAJ_BRON:
			unit->ani->Play(unit->GetTakeWeaponAnimation(true), PLAY_PRIO2|PLAY_ONCE|PLAY_BACK, 0);
			unit->ani->groups[1].speed = 1.f;
			t = 100.f;
			unit->etap_animacji = 0;
			unit->stan_broni = BRON_CHOWA;
			unit->wyjeta = B_BRAK;
			unit->chowana = B_JEDNORECZNA;
			unit->ani->frame_end_info = false;
			break;
		case DA_SCHOWAJ_LUK:
			unit->ani->Play(NAMES::ani_take_bow, PLAY_PRIO2|PLAY_ONCE|PLAY_BACK, 0);
			unit->ani->groups[1].speed = 1.f;
			t = 100.f;
			unit->etap_animacji = 0;
			unit->stan_broni = BRON_CHOWA;
			unit->wyjeta = B_BRAK;
			unit->chowana = B_LUK;
			unit->ani->frame_end_info = false;
			break;
		case DA_STOI:
			unit->ani->Play(NAMES::ani_stand, PLAY_PRIO2, 0);
			t = 2.f;
			break;
		case DA_STRZAL:
			unit->ani->Play(NAMES::ani_shoot, PLAY_PRIO2|PLAY_ONCE, 0);
			unit->ani->groups[0].speed = unit->GetBowAttackSpeed();
			unit->etap_animacji = 0;
			t = 100.f;
			if(game->bow_instances.empty())
				unit->bow_instance = new AnimeshInstance(unit->GetBow().ani);
			else
			{
				unit->bow_instance = game->bow_instances.back();
				game->bow_instances.pop_back();
				unit->bow_instance->ani = unit->GetBow().ani;
			}
			unit->bow_instance->Play(&unit->bow_instance->ani->anims[0], PLAY_ONCE|PLAY_PRIO1|PLAY_NO_BLEND, 0);
			unit->bow_instance->groups[0].speed = unit->ani->groups[0].speed;
			unit->ani->frame_end_info = false;
			break;
		case DA_WYJMIJ_BRON:
			unit->ani->Play(unit->GetTakeWeaponAnimation(true), PLAY_PRIO2|PLAY_ONCE, 0);
			unit->ani->groups[1].speed = 1.f;
			t = 100.f;
			unit->etap_animacji = 0;
			unit->stan_broni = BRON_WYJMUJE;
			unit->wyjeta = B_JEDNORECZNA;
			unit->chowana = B_BRAK;
			unit->ani->frame_end_info = false;
			break;
		case DA_WYJMIJ_LUK:
			unit->ani->Play(NAMES::ani_take_bow, PLAY_PRIO2|PLAY_ONCE, 0);
			unit->ani->groups[1].speed = 1.f;
			t = 100.f;
			unit->etap_animacji = 0;
			unit->stan_broni = BRON_WYJMUJE;
			unit->wyjeta = B_LUK;
			unit->chowana = B_BRAK;
			unit->ani->frame_end_info = false;
			break;
		default:
			assert(0);
			break;
		}
	}

	switch(anim)
	{
	case DA_ATAK:
		if(unit->ani->frame_end_info)
		{
			unit->ani->groups[0].speed = 1.f;
			if(rand2()%2==0)
			{
				anim = DA_ATAK;
				anim2 = DA_STOI;
			}
			else
				anim = DA_BOJOWY;
		}
		break;
	case DA_SCHOWAJ_BRON:
	case DA_SCHOWAJ_LUK:
		if(unit->etap_animacji == 0 && (unit->ani->GetProgress() <= unit->data->frames->t[F_TAKE_WEAPON] || unit->ani->frame_end_info))
			unit->etap_animacji = 1;
		if(unit->ani->frame_end_info)
		{
			unit->stan_broni = BRON_SCHOWANA;
			unit->chowana = B_BRAK;
			anim = DA_STOI;
			t = 1.f;
		}
		break;
	case DA_STRZAL:
		if(unit->ani->GetProgress() > 20.f/40 && unit->etap_animacji < 2)
			unit->etap_animacji = 2;
		else if(unit->ani->GetProgress() > 35.f/40)
		{
			unit->etap_animacji = 3;
			if(unit->ani->frame_end_info)
			{
				assert(unit->bow_instance);
				game->bow_instances.push_back(unit->bow_instance);
				unit->bow_instance = NULL;
				unit->ani->groups[0].speed = 1.f;
				if(rand2()%2 == 0)
				{
					anim = DA_STRZAL;
					anim2 = DA_STOI;
				}
				else
					anim = DA_BOJOWY;
				break;
			}
		}
		unit->bow_instance->groups[0].time = min(unit->ani->groups[0].time, unit->bow_instance->groups[0].anim->length);
		unit->bow_instance->need_update = true;
		break;
	case DA_WYJMIJ_BRON:
		if(unit->etap_animacji == 0 && (unit->ani->GetProgress() >= unit->data->frames->t[F_TAKE_WEAPON] || unit->ani->frame_end_info))
			unit->etap_animacji = 1;
		if(unit->ani->frame_end_info)
		{
			unit->stan_broni = BRON_WYJETA;
			anim = DA_ATAK;
		}
		break;
	case DA_WYJMIJ_LUK:
		if(unit->etap_animacji == 0 && (unit->ani->GetProgress() >= unit->data->frames->t[F_TAKE_WEAPON] || unit->ani->frame_end_info))
			unit->etap_animacji = 1;
		if(unit->ani->frame_end_info)
		{
			unit->stan_broni = BRON_WYJETA;
			anim = DA_STRZAL;
		}
		break;
	case DA_ROZGLADA:
		if(unit->ani->frame_end_info)
			anim = DA_STOI;
		break;
	case DA_BLOK:
	case DA_BOJOWY:
	case DA_STOI:
	case DA_IDZIE:
		break;
	default:
		assert(0);
		break;
	}

	unit->ani->Update(dt);
}

//=================================================================================================
void CreateCharacterPanel::Random(Class _clas)
{
	mode = Mode::PickClass;
	if(_clas == Class::RANDOM)
		clas = ClassInfo::GetRandomPlayer();
	else
		clas = _clas;
	lbClasses.Select(lbClasses.FindIndex((int)clas));
	ClassChanged();
	Unit& u = *unit;
	u.human_data->beard = rand2()%MAX_BEARD-1;
	u.human_data->hair = rand2()%MAX_HAIR-1;
	u.human_data->mustache = rand2()%MAX_MUSTACHE-1;
	int id = rand2()%n_hair_colors;
	u.human_data->hair_color = g_hair_colors[id];
	u.human_data->height = random(0.95f,1.05f);
	u.human_data->ApplyScale(game->aHumanBase);
	anim = anim2 = DA_STOI;
	u.ani->Play(NAMES::ani_stand, PLAY_PRIO2|PLAY_NO_BLEND, 0);
	slider[0].val = u.human_data->hair+1;
	slider[0].text = Format("%s %d/%d", txHair, slider[0].val, slider[0].maxv);
	slider[1].val = u.human_data->mustache+1;
	slider[1].text = Format("%s %d/%d", txMustache, slider[1].val, slider[1].maxv);
	slider[2].val = u.human_data->beard+1;
	slider[2].text = Format("%s %d/%d", txBeard, slider[2].val, slider[2].maxv);
	slider[3].val = id;
	slider[3].text = Format("%s %d/%d", txHairColor, slider[3].val, slider[3].maxv);
	slider[4].val = int((u.human_data->height-0.9f)*500);
	height = slider[4].val;
	slider[4].text = Format("%s %d/%d", txSize, slider[4].val, slider[4].maxv);
	reset_skills_perks = true;
}

//=================================================================================================
void CreateCharacterPanel::Redo(Class _clas, HumanData& hd, CreatedCharacter& cc)
{
	clas = _clas;
	ClassChanged();
	c = cc;
	RebuildSkillsFlow();
	RebuildPerksFlow();
}

//=================================================================================================
void CreateCharacterPanel::Init()
{
	unit->ani = new AnimeshInstance(game->aHumanBase);
	
	for(ClassInfo& ci : g_classes)
	{
		if(ci.IsPickable())
			lbClasses.Add(new DefaultGuiElement(ci.name.c_str(), (int)ci.class_id, ci.icon));
	}
	lbClasses.Sort();
	lbClasses.Init();
}

//=================================================================================================
void CreateCharacterPanel::OnChangeClass(int index)
{
	clas = (Class)lbClasses.GetItem()->value;
	ClassChanged();
	reset_skills_perks = true;
}

//=================================================================================================
cstring CreateCharacterPanel::GetText(int group, int id)
{
	switch((Group)group)
	{
	case Group::Section:
		if(id == -1)
			return txAttributes;
		else
			return g_skill_groups[id].name.c_str();
	case Group::Attribute:
		return Format("%s: %d", g_attributes[id].name.c_str(), unit->Get((Attribute)id));
	case Group::Skill:
		return Format("%s: %d", g_skills[id].name.c_str(), unit->Get((Skill)id));
	default:
		return "MISSING";
	}
}

//=================================================================================================
void CreateCharacterPanel::GetTooltip(TooltipController* _tool, int group, int id)
{
	TooltipController& tool = *_tool;

	switch((Group)group)
	{
	case Group::Section:
	default:
		tool.anything = false;
		break;
	case Group::Attribute:
		{
			AttributeInfo& ai = g_attributes[id];
			tool.big_text = ai.name;
			tool.text = ai.desc;
			tool.small_text.clear();
			tool.anything = true;
			tool.img = NULL;
		}
		break;
	case Group::Skill:
		{
			SkillInfo& si = g_skills[id];
			tool.big_text = si.name;
			tool.text = si.desc;
			if(si.attrib2 != Attribute::NONE)
				tool.small_text = Format("%s: %s, %s", txRelatedAttributes, g_attributes[(int)si.attrib].name.c_str(), g_attributes[(int)si.attrib2].name.c_str());
			else
				tool.small_text = Format("%s: %s", txRelatedAttributes, g_attributes[(int)si.attrib].name.c_str());
			tool.anything = true;
			tool.img = NULL;
		}
		break;
	case Group::Perk:
		{
			tool.anything = true;
			tool.img = NULL;
			tool.small_text.clear();
			PerkInfo& pi = g_perks[id];
			tool.big_text = pi.name;
			tool.text = pi.desc;
		}
		break;
	case Group::TakenPerk:
		{
			TakenPerk& taken = c.taken_perks[id];
			tool.anything = true;
			tool.img = NULL;			
			PerkInfo& pi = g_perks[(int)taken.perk];
			tool.big_text = pi.name;
			tool.text = pi.desc;

			switch(taken.perk)
			{
			case Perk::Skilled:
			case Perk::CraftingTradition:
				tool.small_text.clear();
			case Perk::Weakness:
				tool.small_text = Format("%s: %s", txDecrasedAttrib, g_attributes[taken.value].name.c_str());
				break;
			case Perk::Strength:
				tool.small_text = Format("%s: %s", txIncrasedAttrib, g_attributes[taken.value].name.c_str());
				break;			
			case Perk::SkillFocus:
				{
					int skill_p = (taken.value & 0xFF),
						skill_m1 = ((taken.value & 0xFF00) >> 8),
						skill_m2 = ((taken.value & 0xFF0000) >> 16);
					tool.small_text = Format("%s: %s\n%s: %s, %s", txIncrasedSkill, g_skills[skill_p].name.c_str(), txDecrasedSkills, 
						g_skills[skill_m1].name.c_str(), g_skills[skill_m2].name.c_str());
				}
				break;
			case Perk::Talent:
				tool.small_text = Format("%s: %s", txIncrasedSkill, g_skills[taken.value].name.c_str());
				break;
			default:
				assert(0);
				tool.small_text.clear();
				break;
			}
		}
		break;
	}
}

//=================================================================================================
void CreateCharacterPanel::ClassChanged()
{
	ClassInfo& ci = g_classes[(int)clas];
	unit->data = ci.unit_data;
	InitInventory();
	tbClassDesc.text = ci.desc;
	tbClassDesc.UpdateScrollbar();

	flow_items.clear();

	int y = 0;

	StatProfile& profile = ci.unit_data->GetStatProfile();
	profile.Set(0, unit->unmod_stats);
	unit->CalculateStats();

	// attributes
	flow_items.push_back(FlowItem((int)Group::Section, -1, y));
	y += SECTION_H;
	for(int i = 0; i<(int)Attribute::MAX; ++i)
	{
		flow_items.push_back(FlowItem((int)Group::Attribute, i, 35, 70, profile.attrib[i], y));
		y += VALUE_H;
	}

	// skills
	SkillGroup group = SkillGroup::NONE;
	for(int i = 0; i<(int)Skill::MAX; ++i)
	{
		if(profile.skill[i] >= 0)
		{
			SkillInfo& si = g_skills[i];
			if(si.group != group)
			{
				// start new section
				flow_items.push_back(FlowItem((int)Group::Section, (int)si.group, y));
				y += SECTION_H;
				group = si.group;
			}
			flow_items.push_back(FlowItem((int)Group::Skill, i, 0, 20, profile.skill[i], y));
			y += VALUE_H;
		}
	}
	flow_scroll.total = y;
	flow_scroll.part = flow_scroll.size.y;
	flow_scroll.offset = 0.f;
}

//=================================================================================================
void CreateCharacterPanel::OnPickSkill(int group, int id)
{
	assert(group == (int)Group::PickSkill_Button);

	if(!c.s[id].add)
	{
		// add
		--c.sp;
		c.s[id].value += 5;
		c.s[id].add = true;
	}
	else
	{
		// remove
		++c.sp;
		c.s[id].value -= 5;
		c.s[id].add = false;
	}

	// update buttons image / text
	FlowItem2* find_item = NULL;
	for(FlowItem2* item : flowSkills.items)
	{
		if(item->type == FlowItem2::Button)
		{
			if(!c.s[item->id].add)
			{
				item->state = (c.sp>0 ? Button::NONE : Button::DISABLED);
				item->tex_id = 0;
			}
			else
			{
				item->state = Button::NONE;
				item->tex_id = 1;
			}
		}
		else if(item->type == FlowItem2::Item && item->id == id)
			find_item = item;
	}

	flowSkills.UpdateText(find_item, Format("%s: %d", g_skills[id].name.c_str(), c.s[id].value));	
}

//=================================================================================================
void CreateCharacterPanel::OnPickPerk(int group, int id)
{
	assert(group == (int)Group::PickPerk_AddButton || group == (int)Group::PickPerk_RemoveButton);

	if(group == (int)Group::PickPerk_AddButton)
	{
		// add perk
		switch((Perk)id)
		{
		case Perk::Strength:
			PickAttribute(txPickAttribIncrase, Perk::Strength);
			break;
		case Perk::Weakness:
			PickAttribute(txPickAttribDecrase, Perk::Weakness);
			break;
		case Perk::Skilled:
			c.sp += 2;
			c.sp_max += 2;
			UpdateSkillButtons();
			AddPerk(Perk::Skilled);
			break;
		case Perk::SkillFocus:
			step = 0;
			PickSkill(txPickTwoSkillsDecrase, Perk::SkillFocus, 2);
			break;
		case Perk::Talent:
			PickSkill(txPickSkillIncrase, Perk::Talent);
			break;
		case Perk::CraftingTradition:
			UpdateSkill(Skill::CRAFTING, 10, true);
			AddPerk(Perk::CraftingTradition);
			break;
		default:
			assert(0);
			break;
		}
	}
	else
	{
		// remove perk
		TakenPerk& taken = c.taken_perks[id];
		PerkInfo& info = g_perks[(int)taken.perk];

		switch((Perk)taken.perk)
		{
		case Perk::Strength:
			c.a[taken.value].value -= 5;
			c.a[taken.value].mod = false;
			break;
		case Perk::Weakness:
			c.a[taken.value].value += 5;
			c.a[taken.value].mod = false;
			break;
		case Perk::Skilled:
			c.sp -= 2;
			c.sp_max -= 2;
			UpdateSkillButtons();
			break;
		case Perk::SkillFocus:
			break;
		case Perk::Talent:
			UpdateSkill((Skill)taken.value, -5, false);
			break;
		case Perk::CraftingTradition:
			UpdateSkill(Skill::CRAFTING, -10, false);
			break;
		default:
			assert(0);
			break;
		}

		if(!IS_SET(info.flags, PerkInfo::Free))
			++c.perks;
		if(IS_SET(info.flags, PerkInfo::Flaw))
		{
			--c.perks;
			--c.perks_max;
		}

		c.taken_perks.erase(c.taken_perks.begin() + id);
		RebuildPerksFlow();
	}
}

//=================================================================================================
void CreateCharacterPanel::RebuildSkillsFlow()
{
	flowSkills.Clear();
	SkillGroup last_group = SkillGroup::NONE;
	StatProfile& profile = g_classes[(int)clas].unit_data->GetStatProfile();

	for(SkillInfo& si : g_skills)
	{
		int i = (int)si.skill_id;
		if(c.s[i].value >= 0)
		{
			if(si.group != last_group)
			{
				flowSkills.Add()->Set(g_skill_groups[(int)si.group].name.c_str());
				last_group = si.group;
			}
			bool plus = (c.s[i].value == profile.skill[i]);
			flowSkills.Add()->Set((int)Group::PickSkill_Button, i, (plus ? 0 : 1), !plus && c.sp<=0);
			flowSkills.Add()->Set(Format("%s: %d", si.name.c_str(), c.s[i].value), (int)Group::Skill, i);
		}
	}

	flowSkills.Reposition();
}

//=================================================================================================
void CreateCharacterPanel::RebuildPerksFlow()
{
	unavailable_perks.clear();
	flowPerks.Clear();
	flowPerks.Add()->Set(txAvailablePerks);
	for(PerkInfo& perk : g_perks)
	{
		bool taken = false;
		if(!IS_SET(perk.flags, PerkInfo::Multiple))
		{
			for(TakenPerk& tp : c.taken_perks)
			{
				if(tp.perk == perk.perk_id)
				{
					taken = true;
					break;
				}
			}
		}
		if(!taken)
		{
			if(!IS_SET(perk.flags, PerkInfo::Validate) || ValidatePerk(perk.perk_id))
			{
				bool can_pick = (c.perks == 0 && !IS_SET(perk.flags, PerkInfo::Free));
				flowPerks.Add()->Set((int)Group::PickPerk_AddButton, (int)perk.perk_id, 0, can_pick);
				flowPerks.Add()->Set(perk.name.c_str(), (int)Group::Perk, (int)perk.perk_id);
			}
			else
				unavailable_perks.push_back(perk.perk_id);
		}
	}
	if(!unavailable_perks.empty())
	{
		flowPerks.Add()->Set(txUnavailablePerks);
		for(Perk p : unavailable_perks)
		{
			flowPerks.Add()->Set((int)Group::PickPerk_DisabledButton, (int)p, 0, true);
			flowPerks.Add()->Set(g_perks[(int)p].name.c_str(), (int)Group::Perk, (int)p);
		}
	}
	if(!c.taken_perks.empty())
	{
		flowPerks.Add()->Set(txTakenPerks);
		int index = 0;
		for(TakenPerk& tp : c.taken_perks)
		{
			flowPerks.Add()->Set((int)Group::PickPerk_RemoveButton, index, 1, false);
			flowPerks.Add()->Set(g_perks[(int)tp.perk].name.c_str(), (int)Group::TakenPerk, index);
			++index;
		}
	}
	flowPerks.Reposition();
}

//=================================================================================================
void CreateCharacterPanel::ResetSkillsPerks()
{
	reset_skills_perks = false;

	// todo, values
	c.sp = 2;
	c.sp_max = 2;
	c.perks = 2;
	c.perks_max = 2;

	ClassInfo& ci = g_classes[(int)clas];
	StatProfile& profile = ci.unit_data->GetStatProfile();
	for(int i = 0; i < (int)Skill::MAX; ++i)
	{
		c.s[i].value = profile.skill[i];
		c.s[i].add = false;
		c.s[i].mod = false;
	}

	for(int i = 0; i < (int)Attribute::MAX; ++i)
	{
		c.a[i].value = profile.attrib[i];
		c.a[i].mod = false;
	}

	c.taken_perks.clear();

	RebuildSkillsFlow();
	RebuildPerksFlow();
	flowSkills.ResetScrollbar();
	flowPerks.ResetScrollbar();
}

//=================================================================================================
void CreateCharacterPanel::OnShowWarning(int id)
{
	if(id == BUTTON_YES)
		mode = Mode::PickAppearance;
}

//=================================================================================================
void CreateCharacterPanel::PickAttribute(cstring text, Perk perk)
{
	picked_perk = perk;

	PickItemDialogParams params;
	params.event = DialogEvent(this, &CreateCharacterPanel::OnPickAttributeForPerk);
	params.get_tooltip = TooltipGetText(this, &CreateCharacterPanel::GetTooltip);
	params.parent = this;
	params.text = text;

	for(int i = 0; i < (int)Attribute::MAX; ++i)
		params.AddItem(Format("%s: %d", g_attributes[i].name.c_str(), c.a[i].value), (int)Group::Attribute, i, c.a[i].mod);

	pickItemDialog = PickItemDialog::Show(params);
}

//=================================================================================================
void CreateCharacterPanel::PickSkill(cstring text, Perk perk, int multiple)
{
	picked_perk = perk;

	PickItemDialogParams params;
	params.event = DialogEvent(this, &CreateCharacterPanel::OnPickSkillForPerk);
	params.get_tooltip = TooltipGetText(this, &CreateCharacterPanel::GetTooltip);
	params.parent = this;
	params.text = text;
	params.multiple = multiple;

	SkillGroup last_group = SkillGroup::NONE;
	for(SkillInfo& info : g_skills)
	{
		int i = (int)info.skill_id;
		if(c.s[i].value > 0)
		{
			if(info.group != last_group)
			{
				params.AddSeparator(g_skill_groups[(int)info.group].name.c_str());
				last_group = info.group;
			}
			params.AddItem(Format("%s: %d", g_skills[i].name.c_str(), c.s[i].value), (int)Group::Skill, i, c.s[i].mod);
		}
	}

	pickItemDialog = PickItemDialog::Show(params);
}

//=================================================================================================
void CreateCharacterPanel::OnPickAttributeForPerk(int id)
{
	if(id == BUTTON_CANCEL)
		return;

	int group, selected;
	pickItemDialog->GetSelected(group, selected);

	switch(picked_perk)
	{
	case Perk::Weakness:
		c.a[selected].value -= 5;
		break;
	case Perk::Strength:
		c.a[selected].value += 5;
		break;
	default:
		assert(0);
		break;
	}

	c.a[selected].mod = true;

	AddPerk(picked_perk, selected);
}

//=================================================================================================
void CreateCharacterPanel::OnPickSkillForPerk(int id)
{
	if(id == BUTTON_CANCEL)
	{
		if(picked_perk == Perk::SkillFocus && step == 1)
		{
			c.s[step_var].value += 5;
			c.s[step_var].mod = false;
			c.s[step_var2].value += 5;
			c.s[step_var2].mod = false;
		}
		return;
	}

	switch(picked_perk)
	{
	case Perk::SkillFocus:
		if(step == 0)
		{
			auto& items = pickItemDialog->GetSelected();
			step_var = items[0]->id;
			step_var2 = items[1]->id;
			step = 1;
			c.s[step_var].value -= 5;
			c.s[step_var].mod = true;
			c.s[step_var2].value -= 5;
			c.s[step_var2].mod = true;
			PickSkill(txPickSkillIncrase, Perk::SkillFocus);
		}
		else
		{
			int group, selected;
			pickItemDialog->GetSelected(group, selected);
			c.s[selected].value += 10;
			c.s[selected].mod = true;
			flowSkills.UpdateText((int)Group::Skill, step_var, Format("%s: %d", g_skills[step_var].name.c_str(), c.s[step_var].value), true);
			flowSkills.UpdateText((int)Group::Skill, step_var2, Format("%s: %d", g_skills[step_var2].name.c_str(), c.s[step_var2].value), true);
			flowSkills.UpdateText((int)Group::Skill, selected, Format("%s: %d", g_skills[selected].name.c_str(), c.s[selected].value), true);
			flowSkills.UpdateText();
			AddPerk(Perk::SkillFocus, selected | (step_var << 8) | (step_var2 << 16));
		}
		break;
	case Perk::Talent:
		{
			int group, selected;
			pickItemDialog->GetSelected(group, selected);
			UpdateSkill((Skill)selected, 5, true);
			AddPerk(Perk::Talent, selected);
		}
		break;
	default:
		assert(0);
		break;
	}	
}

//=================================================================================================
void CreateCharacterPanel::UpdateSkill(Skill s, int value, bool mod)
{
	int id = (int)s;
	c.s[id].value += value;
	c.s[id].mod = mod;
	flowSkills.UpdateText(flowSkills.Find((int)Group::Skill, id), Format("%s: %d", g_skills[id].name.c_str(), c.s[id].value));
}

//=================================================================================================
void CreateCharacterPanel::UpdateSkillButtons()
{
	for(FlowItem2* item : flowSkills.items)
	{
		if(item->type == FlowItem2::Button)
		{
			if(!c.s[item->id].add)
			{
				item->state = (c.sp>0 ? Button::NONE : Button::DISABLED);
				item->tex_id = 0;
			}
			else
			{
				item->state = Button::NONE;
				item->tex_id = 1;
			}
		}
	}
}

//=================================================================================================
void CreateCharacterPanel::AddPerk(Perk perk, int value)
{
	c.taken_perks.push_back(TakenPerk(perk, value));
	PerkInfo& info = g_perks[(int)perk];
	if(!IS_SET(info.flags, PerkInfo::Free))
		--c.perks;
	if(IS_SET(info.flags, PerkInfo::Flaw))
	{
		++c.perks;
		++c.perks_max;
	}
	RebuildPerksFlow();
}

//=================================================================================================
bool CreateCharacterPanel::ValidatePerk(Perk perk)
{
	switch(perk)
	{
	case Perk::CraftingTradition:
		return !c.s[(int)Skill::CRAFTING].mod;
	default:
		assert(0);
		return false;
	}
}
