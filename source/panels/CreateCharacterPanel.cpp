#include "Pch.h"
#include "GameCore.h"
#include "CreateCharacterPanel.h"
#include "Game.h"
#include "Language.h"
#include "GetTextDialog.h"
#include "Language.h"
#include "PickItemDialog.h"
#include "ResourceManager.h"
#include "DirectX.h"
#include "Unit.h"
#include "Render.h"
#include "RenderTarget.h"
#include "Level.h"
#include "GameGui.h"
#include "GameResources.h"

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
	IdSize,
	IdRandomSet
};

//=================================================================================================
CreateCharacterPanel::CreateCharacterPanel(DialogInfo& info) : DialogBox(info), unit(nullptr), rt_char(nullptr)
{
	size = Int2(600, 500);
	unit = new Unit;
	unit->human_data = new Human;
	unit->player = nullptr;
	unit->ai = nullptr;
	unit->hero = nullptr;
	unit->used_item = nullptr;
	unit->weapon_state = WS_HIDDEN;
	unit->pos = unit->visual_pos = Vec3(0, 0, 0);
	unit->rot = 0.f;
	unit->fake_unit = true;
	unit->action = A_NONE;
	unit->stats = new UnitStats;
	unit->stats->fixed = false;
	unit->stats->subprofile.value = 0;
	unit->stamina = unit->stamina_max = 100.f;

	btCancel.id = IdCancel;
	btCancel.custom = &custom_x;
	btCancel.size = Int2(32, 32);
	btCancel.parent = this;
	btCancel.pos = Int2(size.x - 32 - 16, 16);

	btBack.id = IdBack;
	btBack.size = Int2(100, 44);
	btBack.parent = this;
	btBack.pos = Int2(16, size.y - 60);

	btNext.id = IdNext;
	btNext.size = Int2(100, 44);
	btNext.parent = this;
	btNext.pos = Int2(size.x - 100 - 16, size.y - 60);

	btCreate.id = IdCreate;
	btCreate.size = Int2(100, 44);
	btCreate.parent = this;
	btCreate.pos = Int2(size.x - 100 - 16, size.y - 60);

	btRandomSet.id = IdRandomSet;
	btRandomSet.size = Int2(100, 44);
	btRandomSet.parent = this;
	btRandomSet.pos = Int2(size.x / 2 - 50, size.y - 60);

	checkbox.bt_size = Int2(32, 32);
	checkbox.checked = false;
	checkbox.id = IdHardcore;
	checkbox.parent = this;
	checkbox.pos = Int2(20, 350);
	checkbox.size = Int2(200, 32);

	{
		Slider& s = slider[0];
		s.id = IdHair;
		s.minv = 0;
		s.maxv = MAX_HAIR - 1;
		s.val = 0;
		s.pos = Int2(20, 100);
		s.parent = this;
	}

	{
		Slider& s = slider[1];
		s.id = IdMustache;
		s.minv = 0;
		s.maxv = MAX_MUSTACHE - 1;
		s.val = 0;
		s.pos = Int2(20, 150);
		s.parent = this;
	}

	{
		Slider& s = slider[2];
		s.id = IdBeard;
		s.minv = 0;
		s.maxv = MAX_BEARD - 1;
		s.val = 0;
		s.pos = Int2(20, 200);
		s.parent = this;
	}

	{
		Slider& s = slider[3];
		s.id = IdColor;
		s.minv = 0;
		s.maxv = n_hair_colors - 1;
		s.val = 0;
		s.pos = Int2(20, 250);
		s.parent = this;
	}

	{
		Slider& s = slider[4];
		s.id = IdSize;
		s.minv = 0;
		s.maxv = 100;
		s.val = 50;
		s.pos = Int2(20, 300);
		s.parent = this;
		s.SetHold(true);
		s.hold_val = 25.f;
	}

	lbClasses.pos = Int2(16, 73 - 18);
	lbClasses.size = Int2(198, 235 + 18);
	lbClasses.SetForceImageSize(Int2(20, 20));
	lbClasses.SetItemHeight(24);
	lbClasses.event_handler = DialogEvent(this, &CreateCharacterPanel::OnChangeClass);
	lbClasses.parent = this;

	tbClassDesc.pos = Int2(130, 335);
	tbClassDesc.size = Int2(341, 93);
	tbClassDesc.SetReadonly(true);
	tbClassDesc.AddScrollbar();

	tbInfo.pos = Int2(130, 335);
	tbInfo.size = Int2(341, 93);
	tbInfo.SetReadonly(true);
	tbInfo.AddScrollbar();

	flow_pos = Int2(368, 73 - 18);
	flow_size = Int2(198, 235 + 18);
	flow_scroll.pos = Int2(flow_pos.x + flow_size.x + 2, flow_pos.y);
	flow_scroll.size = Int2(16, flow_size.y);
	flow_scroll.total = 100;
	flow_scroll.part = 10;

	tooltip.Init(TooltipController::Callback(this, &CreateCharacterPanel::GetTooltip));

	flowSkills.size = Int2(198, 235 + 18);
	flowSkills.pos = Int2(16, 73 - 18);
	flowSkills.button_size = Int2(16, 16);
	flowSkills.button_tex = custom_bt;
	flowSkills.on_button = ButtonEvent(this, &CreateCharacterPanel::OnPickSkill);

	flowPerks.size = Int2(198, 235 + 18);
	flowPerks.pos = Int2(size.x - flowPerks.size.x - 16, 73 - 18);
	flowPerks.button_size = Int2(16, 16);
	flowPerks.button_tex = custom_bt;
	flowPerks.on_button = ButtonEvent(this, &CreateCharacterPanel::OnPickPerk);
}

//=================================================================================================
CreateCharacterPanel::~CreateCharacterPanel()
{
	delete unit;
}

//=================================================================================================
void CreateCharacterPanel::LoadLanguage()
{
	txAttributes = Str("attributes");
	txRelatedAttributes = Str("relatedAttributes");

	Language::Section section = Language::GetSection("CreateCharacterPanel");
	txHardcoreMode = section.Get("hardcoreMode");
	txHair = section.Get("hair");
	txMustache = section.Get("mustache");
	txBeard = section.Get("beard");
	txHairColor = section.Get("hairColor");
	txSize = section.Get("size");
	txCharacterCreation = section.Get("characterCreation");
	txName = section.Get("name");
	txCreateCharWarn = section.Get("createCharWarn");
	txSkillPoints = section.Get("skillPoints");
	txPerkPoints = section.Get("perkPoints");
	txPickAttribIncrease = section.Get("pickAttribIncrease");
	txPickSkillIncrease = section.Get("pickSkillIncrease");
	txAvailablePerks = section.Get("availablePerks");
	txTakenPerks = section.Get("takenPerks");
	txCreateCharTooMany = section.Get("createCharTooMany");
	txFlawExtraPerk = section.Get("flawExtraPerk");
	txPerksRemoved = section.Get("perksRemoved");
	btBack.text = section.Get("goBack");
	btNext.text = section.Get("next");
	btCreate.text = section.Get("create");
	btRandomSet.text = section.Get("randomSet");

	checkbox.text = txHardcoreMode;

	slider[0].text = txHair;
	slider[1].text = txMustache;
	slider[2].text = txBeard;
	slider[3].text = txHairColor;
	slider[4].text = txSize;

	tbInfo.SetText(section.Get("createCharText"));
}

//=================================================================================================
void CreateCharacterPanel::LoadData()
{
	tBox = res_mgr->Load<Texture>("box.png");
	tPowerBar = res_mgr->Load<Texture>("klasa_cecha.png");
	custom_x.tex[Button::NONE] = AreaLayout(res_mgr->Load<Texture>("close.png"));
	custom_x.tex[Button::HOVER] = AreaLayout(res_mgr->Load<Texture>("close_hover.png"));
	custom_x.tex[Button::DOWN] = AreaLayout(res_mgr->Load<Texture>("close_down.png"));
	custom_x.tex[Button::DISABLED] = AreaLayout(res_mgr->Load<Texture>("close_disabled.png"));
	custom_bt[0].tex[Button::NONE] = AreaLayout(res_mgr->Load<Texture>("plus.png"));
	custom_bt[0].tex[Button::HOVER] = AreaLayout(res_mgr->Load<Texture>("plus_hover.png"));
	custom_bt[0].tex[Button::DOWN] = AreaLayout(res_mgr->Load<Texture>("plus_down.png"));
	custom_bt[0].tex[Button::DISABLED] = AreaLayout(res_mgr->Load<Texture>("plus_disabled.png"));
	custom_bt[1].tex[Button::NONE] = AreaLayout(res_mgr->Load<Texture>("minus.png"));
	custom_bt[1].tex[Button::HOVER] = AreaLayout(res_mgr->Load<Texture>("minus_hover.png"));
	custom_bt[1].tex[Button::DOWN] = AreaLayout(res_mgr->Load<Texture>("minus_down.png"));
	custom_bt[1].tex[Button::DISABLED] = AreaLayout(res_mgr->Load<Texture>("minus_disabled.png"));

	rt_char = render->CreateRenderTarget(Int2(128, 256));
}

//=================================================================================================
void CreateCharacterPanel::Draw(ControlDrawData*)
{
	DrawPanel();

	// top text
	Rect rect0 = { 12 + pos.x, 12 + pos.y, pos.x + size.x - 12, 12 + pos.y + 72 };
	gui->DrawText(GameGui::font_big, txCharacterCreation, DTF_CENTER, Color::Black, rect0);

	// character
	gui->DrawSprite(rt_char->GetTexture(), Int2(pos.x + 228, pos.y + 64));

	// close button
	btCancel.Draw();

	Rect rect;
	Matrix mat;

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
			Int2 fpos = flow_pos + global_pos;
			gui->DrawItem(tBox, fpos, flow_size, Color::White, 8, 32);
			flow_scroll.Draw();

			rect = Rect::Create(fpos + Int2(2, 2), flow_size - Int2(4, 4));
			Rect r = rect, part = { 0, 0, 256, 32 };
			r.Top() += 20;

			for(OldFlowItem& fi : flow_items)
			{
				r.Top() = fpos.y + fi.y - (int)flow_scroll.offset;
				cstring item_text = GetText(fi.group, fi.id);
				if(fi.section)
				{
					r.Bottom() = r.Top() + SECTION_H;
					if(!gui->DrawText(GameGui::font_big, item_text, DTF_SINGLELINE, Color::Black, r, &rect))
						break;
				}
				else
				{
					if(fi.part > 0)
					{
						mat = Matrix::Transform2D(nullptr, 0.f, &Vec2(float(flow_size.x - 4) / 256, 17.f / 32), nullptr, 0.f, &Vec2(r.LeftTop()));
						part.Right() = int(fi.part * 256);
						gui->DrawSprite2(tPowerBar, mat, &part, &rect, Color::White);
					}
					r.Bottom() = r.Top() + VALUE_H;
					if(!gui->DrawText(GameGui::font, item_text, DTF_SINGLELINE, Color::Black, r, &rect))
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
			btRandomSet.Draw();

			flowSkills.Draw();
			flowPerks.Draw();

			tbInfo.Draw();

			// left text "Skill points: X/Y"
			Rect r = { global_pos.x + 16, global_pos.y + 310, global_pos.x + 216, global_pos.y + 360 };
			gui->DrawText(GameGui::font, Format(txSkillPoints, cc.sp, cc.sp_max), 0, Color::Black, r);

			// right text "Feats: X/Y"
			Rect r2 = { global_pos.x + size.x - 216, global_pos.y + 310, global_pos.x + size.x - 16, global_pos.y + 360 };
			gui->DrawText(GameGui::font, Format(txPerkPoints, cc.perks, cc.perks_max), DTF_RIGHT, Color::Black, r2);

			tooltip.Draw();
		}
		break;
	case Mode::PickAppearance:
		{
			btCreate.Draw();
			btRandomSet.Draw();
			btBack.Draw();

			if(!Net::IsOnline())
				checkbox.Draw();

			for(int i = 0; i < 5; ++i)
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
	if(PointInRect(gui->cursor_pos, Int2(pos.x + 228, pos.y + 94), Int2(128, 256)) && input->Focus() && focus)
	{
		bool rotate = false;
		if(rotating)
		{
			if(input->Down(Key::LeftButton))
				rotate = true;
			else
				rotating = false;
		}
		else if(input->Pressed(Key::LeftButton))
		{
			rotate = true;
			rotating = true;
		}

		if(rotate)
			unit->rot = Clip(unit->rot - float(gui->cursor_pos.x - pos.x - 228 - 64) / 16 * dt);
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

			if(!flow_scroll.clicked && PointInRect(gui->cursor_pos, flow_pos + global_pos, flow_size))
			{
				if(focus && input->Focus())
					flow_scroll.ApplyMouseWheel();

				int y = gui->cursor_pos.y - global_pos.y - flow_pos.y + (int)flow_scroll.offset;
				for(OldFlowItem& fi : flow_items)
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
			else if(!flow_scroll.clicked && PointInRect(gui->cursor_pos, flow_scroll.pos + global_pos, flow_scroll.size) && focus && input->Focus())
				flow_scroll.ApplyMouseWheel();

			tooltip.UpdateTooltip(dt, group, id);
		}
		break;
	case Mode::PickSkillPerk:
		{
			btBack.mouse_focus = focus;
			btBack.Update(dt);

			btNext.mouse_focus = focus;
			btNext.Update(dt);

			btRandomSet.mouse_focus = focus;
			btRandomSet.Update(dt);

			tbInfo.mouse_focus = focus;
			tbInfo.Update(dt);

			int group = -1, id = -1;

			flowSkills.mouse_focus = focus;
			flowSkills.Update(dt);
			flowSkills.GetSelected(group, id);

			flowPerks.mouse_focus = focus;
			flowPerks.Update(dt);
			flowPerks.GetSelected(group, id);

			tooltip.UpdateTooltip(dt, group, id);
		}
		break;
	case Mode::PickAppearance:
		{
			btBack.mouse_focus = focus;
			btBack.Update(dt);

			btCreate.mouse_focus = focus;
			btCreate.Update(dt);

			btRandomSet.mouse_focus = focus;
			btRandomSet.Update(dt);

			if(!Net::IsOnline())
			{
				checkbox.mouse_focus = focus;
				checkbox.Update(dt);
			}

			for(int i = 0; i < 5; ++i)
			{
				slider[i].mouse_focus = focus;
				slider[i].Update(dt);
			}
		}
		break;
	}

	if(focus && input->Focus() && input->PressedRelease(Key::Escape))
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
		pos = global_pos = (gui->wnd_size - size) / 2;
		btCancel.global_pos = global_pos + btCancel.pos;
		btBack.global_pos = global_pos + btBack.pos;
		btNext.global_pos = global_pos + btNext.pos;
		btCreate.global_pos = global_pos + btCreate.pos;
		btRandomSet.global_pos = global_pos + btRandomSet.pos;
		for(int i = 0; i < 5; ++i)
			slider[i].global_pos = global_pos + slider[i].pos;
		checkbox.global_pos = global_pos + checkbox.pos;
		lbClasses.Event(GuiEvent_Moved);
		tbClassDesc.Move(pos);
		tbInfo.Move(pos);
		flow_scroll.global_pos = global_pos + flow_scroll.pos;
		flowSkills.UpdatePos(global_pos);
		flowPerks.UpdatePos(global_pos);
		tooltip.Clear();
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
				if(cc.sp < 0 || cc.perks < 0)
					gui->SimpleDialog(txCreateCharTooMany, this);
				else if(cc.sp != 0 || cc.perks != 0)
				{
					DialogInfo di;
					di.event = [this](int id)
					{
						if(id == BUTTON_YES)
							mode = Mode::PickAppearance;
					};
					di.name = "create_char_warn";
					di.order = ORDER_TOP;
					di.parent = this;
					di.pause = false;
					di.text = txCreateCharWarn;
					di.type = DIALOG_YESNO;
					gui->ShowDialog(di);
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
				GetTextDialogParams params(Format("%s:", txName), player_name);
				params.event = [this](int id)
				{
					if(id == BUTTON_OK)
					{
						last_hair_color_index = hair_color_index;
						CloseDialog();
						event(BUTTON_OK);
					}
				};
				params.limit = 16;
				params.parent = this;
				GetTextDialog::Show(params);
			}
			else
			{
				last_hair_color_index = hair_color_index;
				CloseDialog();
				event(BUTTON_OK);
			}
			break;
		case IdHardcore:
			game->hardcore_option = checkbox.checked;
			game->cfg.Add("hardcore", game->hardcore_option);
			game->SaveCfg();
			break;
		case IdHair:
			unit->human_data->hair = slider[0].val - 1;
			slider[0].text = Format("%s %d/%d", txHair, slider[0].val, slider[0].maxv);
			break;
		case IdMustache:
			unit->human_data->mustache = slider[1].val - 1;
			slider[1].text = Format("%s %d/%d", txMustache, slider[1].val, slider[1].maxv);
			break;
		case IdBeard:
			unit->human_data->beard = slider[2].val - 1;
			slider[2].text = Format("%s %d/%d", txBeard, slider[2].val, slider[2].maxv);
			break;
		case IdColor:
			unit->human_data->hair_color = g_hair_colors[slider[3].val];
			slider[3].text = Format("%s %d/%d", txHairColor, slider[3].val, slider[3].maxv);
			break;
		case IdSize:
			unit->human_data->height = Lerp(0.9f, 1.1f, float(slider[4].val) / 100);
			slider[4].text = Format("%s %d/%d", txSize, slider[4].val, slider[4].maxv);
			unit->human_data->ApplyScale(unit->mesh_inst->mesh);
			unit->mesh_inst->need_update = true;
			break;
		case IdRandomSet:
			if(mode == Mode::PickAppearance)
				RandomAppearance();
			else
			{
				cc.Random(clas);
				RebuildSkillsFlow();
				RebuildPerksFlow();
				UpdateInventory();
			}
			break;
		}
	}
}

//=================================================================================================
void CreateCharacterPanel::RenderUnit()
{
	IDirect3DDevice9* device = render->GetDevice();
	HRESULT hr = device->TestCooperativeLevel();
	if(hr != D3D_OK)
		return;

	render->SetAlphaBlend(false);
	render->SetAlphaTest(false);
	render->SetNoCulling(false);
	render->SetNoZWrite(false);
	render->SetTarget(rt_char);

	// start rendering
	V(device->Clear(0, nullptr, D3DCLEAR_ZBUFFER | D3DCLEAR_TARGET, 0, 1.f, 0));
	V(device->BeginScene());

	static vector<Lights> lights;

	game_level->SetOutsideParams();

	Matrix matView, matProj;
	Vec3 from = Vec3(0.f, 2.f, dist);
	matView = Matrix::CreateLookAt(from, Vec3(0.f, 1.f, 0.f), Vec3(0, 1, 0));
	matProj = Matrix::CreatePerspectiveFieldOfView(PI / 4, 0.5f, 1.f, 5.f);
	game_level->camera.matViewProj = matView * matProj;
	game_level->camera.center = from;
	game_level->camera.matViewInv = matView.Inverse();

	game_level->camera.frustum.Set(game_level->camera.matViewProj);
	game->ListDrawObjectsUnit(game_level->camera.frustum, true, *unit);
	game->DrawSceneNodes(game->draw_batch.nodes, lights, true);
	game->draw_batch.Clear();

	// end rendering
	V(device->EndScene());

	render->SetTarget(nullptr);
}

//=================================================================================================
void CreateCharacterPanel::UpdateUnit(float dt)
{
	// update character
	t -= dt;
	if(t <= 0.f)
	{
		switch(anim)
		{
		case DA_ATTACK:
		case DA_HIDE_WEAPON:
		case DA_HIDE_BOW:
		case DA_SHOOT:
		case DA_SHOW_WEAPON:
		case DA_SHOW_BOW:
		case DA_LOOKS_AROUND:
			assert(0);
			break;
		case DA_BLOCK:
			if(Rand() % 2 == 0)
				anim = DA_ATTACK;
			else
				anim = DA_BATTLE_MODE;
			break;
		case DA_BATTLE_MODE:
			if(unit->weapon_taken == W_ONE_HANDED)
			{
				int what = Rand() % (unit->HaveShield() ? 3 : 2);
				if(what == 0)
					anim = DA_ATTACK;
				else if(what == 1)
					anim = DA_HIDE_WEAPON;
				else
					anim = DA_BLOCK;
			}
			else
			{
				if(Rand() % 2 == 0)
					anim = DA_SHOOT;
				else
					anim = DA_HIDE_BOW;
			}
			break;
		case DA_STAND:
		case DA_WALK:
			{
				int what = Rand() % (unit->HaveBow() ? 5 : 4);
				if(what == 0)
					anim = DA_LOOKS_AROUND;
				else if(what == 1)
					anim = DA_STAND;
				else if(what == 2)
					anim = DA_WALK;
				else if(what == 3)
					anim = DA_SHOW_WEAPON;
				else
					anim = DA_SHOW_BOW;
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
		case DA_ATTACK:
			unit->attack_id = unit->GetRandomAttack();
			unit->mesh_inst->Play(NAMES::ani_attacks[unit->attack_id], PLAY_PRIO2 | PLAY_ONCE, 0);
			unit->mesh_inst->groups[0].speed = unit->GetAttackSpeed();
			unit->animation_state = 0;
			t = 100.f;
			break;
		case DA_BLOCK:
			unit->mesh_inst->Play(NAMES::ani_block, PLAY_PRIO2, 0);
			t = 1.f;
			break;
		case DA_BATTLE_MODE:
			unit->mesh_inst->Play(NAMES::ani_battle, PLAY_PRIO2, 0);
			t = 1.f;
			break;
		case DA_WALK:
			unit->mesh_inst->Play(NAMES::ani_move, PLAY_PRIO2, 0);
			t = 2.f;
			break;
		case DA_LOOKS_AROUND:
			unit->mesh_inst->Play("rozglada", PLAY_PRIO2 | PLAY_ONCE, 0);
			t = 100.f;
			break;
		case DA_HIDE_WEAPON:
			unit->mesh_inst->Play(unit->GetTakeWeaponAnimation(true), PLAY_PRIO2 | PLAY_ONCE | PLAY_BACK, 0);
			unit->animation_state = 0;
			unit->weapon_state = WS_HIDING;
			unit->weapon_taken = W_NONE;
			unit->weapon_hiding = W_ONE_HANDED;
			t = 100.f;
			break;
		case DA_HIDE_BOW:
			unit->mesh_inst->Play(NAMES::ani_take_bow, PLAY_PRIO2 | PLAY_ONCE | PLAY_BACK, 0);
			unit->animation_state = 0;
			unit->weapon_state = WS_HIDING;
			unit->weapon_taken = W_NONE;
			unit->weapon_hiding = W_BOW;
			t = 100.f;
			break;
		case DA_STAND:
			unit->mesh_inst->Play(NAMES::ani_stand, PLAY_PRIO2, 0);
			t = 2.f;
			break;
		case DA_SHOOT:
			unit->mesh_inst->Play(NAMES::ani_shoot, PLAY_PRIO2 | PLAY_ONCE, 0);
			unit->mesh_inst->groups[0].speed = unit->GetBowAttackSpeed();
			unit->animation_state = 0;
			unit->bow_instance = game_level->GetBowInstance(unit->GetBow().mesh);
			unit->bow_instance->Play(&unit->bow_instance->mesh->anims[0], PLAY_ONCE | PLAY_PRIO1 | PLAY_NO_BLEND, 0);
			unit->bow_instance->groups[0].speed = unit->mesh_inst->groups[0].speed;
			unit->action = A_SHOOT;
			t = 100.f;
			break;
		case DA_SHOW_WEAPON:
			unit->mesh_inst->Play(unit->GetTakeWeaponAnimation(true), PLAY_PRIO2 | PLAY_ONCE, 0);
			unit->animation_state = 0;
			unit->weapon_state = WS_TAKING;
			unit->weapon_taken = W_ONE_HANDED;
			unit->weapon_hiding = W_NONE;
			t = 100.f;
			break;
		case DA_SHOW_BOW:
			unit->mesh_inst->Play(NAMES::ani_take_bow, PLAY_PRIO2 | PLAY_ONCE, 0);
			unit->animation_state = 0;
			unit->weapon_state = WS_TAKING;
			unit->weapon_taken = W_BOW;
			unit->weapon_hiding = W_NONE;
			t = 100.f;
			break;
		default:
			assert(0);
			break;
		}
	}

	switch(anim)
	{
	case DA_ATTACK:
		if(unit->mesh_inst->IsEnded())
		{
			if(Rand() % 2 == 0)
			{
				anim = DA_ATTACK;
				anim2 = DA_STAND;
			}
			else
				anim = DA_BATTLE_MODE;
		}
		break;
	case DA_HIDE_WEAPON:
	case DA_HIDE_BOW:
		if(unit->animation_state == 0 && (unit->mesh_inst->GetProgress() <= unit->data->frames->t[F_TAKE_WEAPON]))
			unit->animation_state = 1;
		if(unit->mesh_inst->IsEnded())
		{
			unit->weapon_state = WS_HIDDEN;
			unit->weapon_hiding = W_NONE;
			anim = DA_STAND;
			t = 1.f;
		}
		break;
	case DA_SHOOT:
		if(unit->mesh_inst->GetProgress() > 20.f / 40 && unit->animation_state < 2)
			unit->animation_state = 2;
		else if(unit->mesh_inst->GetProgress() > 35.f / 40)
		{
			unit->animation_state = 3;
			if(unit->mesh_inst->IsEnded())
			{
				game_level->FreeBowInstance(unit->bow_instance);
				unit->action = A_NONE;
				if(Rand() % 2 == 0)
				{
					anim = DA_SHOOT;
					anim2 = DA_STAND;
				}
				else
					anim = DA_BATTLE_MODE;
				break;
			}
		}
		unit->bow_instance->groups[0].time = min(unit->mesh_inst->groups[0].time, unit->bow_instance->groups[0].anim->length);
		unit->bow_instance->need_update = true;
		break;
	case DA_SHOW_WEAPON:
		if(unit->animation_state == 0 && (unit->mesh_inst->GetProgress() >= unit->data->frames->t[F_TAKE_WEAPON]))
			unit->animation_state = 1;
		if(unit->mesh_inst->IsEnded())
		{
			unit->weapon_state = WS_TAKEN;
			anim = DA_ATTACK;
		}
		break;
	case DA_SHOW_BOW:
		if(unit->animation_state == 0 && (unit->mesh_inst->GetProgress() >= unit->data->frames->t[F_TAKE_WEAPON]))
			unit->animation_state = 1;
		if(unit->mesh_inst->IsEnded())
		{
			unit->weapon_state = WS_TAKEN;
			anim = DA_SHOOT;
		}
		break;
	case DA_LOOKS_AROUND:
		if(unit->mesh_inst->IsEnded())
			anim = DA_STAND;
		break;
	case DA_BLOCK:
	case DA_BATTLE_MODE:
	case DA_STAND:
	case DA_WALK:
		break;
	default:
		assert(0);
		break;
	}

	unit->mesh_inst->Update(dt);
}

//=================================================================================================
void CreateCharacterPanel::Init()
{
	unit->mesh_inst = new MeshInstance(game_res->aHuman);

	for(Class* clas : Class::classes)
	{
		if(clas->IsPickable())
			lbClasses.Add(new DefaultGuiElement(clas->name.c_str(), reinterpret_cast<int>(clas), clas->icon));
	}
	lbClasses.Sort();
	lbClasses.Initialize();
}

//=================================================================================================
void CreateCharacterPanel::RandomAppearance()
{
	Unit& u = *unit;
	u.human_data->beard = Rand() % MAX_BEARD - 1;
	u.human_data->hair = Rand() % MAX_HAIR - 1;
	u.human_data->mustache = Rand() % MAX_MUSTACHE - 1;
	hair_color_index = Rand() % n_hair_colors;
	u.human_data->hair_color = g_hair_colors[hair_color_index];
	u.human_data->height = Random(0.95f, 1.05f);
	u.human_data->ApplyScale(game_res->aHuman);
	SetControls();
}

//=================================================================================================
void CreateCharacterPanel::Show(bool enter_name)
{
	clas = Class::GetRandomPlayer();
	lbClasses.Select(lbClasses.FindIndex((int)clas));
	ClassChanged();
	RandomAppearance();

	reset_skills_perks = true;
	this->enter_name = enter_name;
	mode = Mode::PickClass;

	SetControls();
	SetCharacter();
	gui->ShowDialog(this);
	ResetDoll(true);
	RenderUnit();
}

//=================================================================================================
void CreateCharacterPanel::ShowRedo(Class* clas, HumanData& hd, CreatedCharacter& cc)
{
	this->clas = clas;
	lbClasses.Select(lbClasses.FindIndex((int)clas));
	ClassChanged();
	hair_color_index = last_hair_color_index;
	unit->ApplyHumanData(hd);
	this->cc = cc;
	RebuildSkillsFlow();
	RebuildPerksFlow();
	UpdateInventory();

	reset_skills_perks = false;
	enter_name = false;
	mode = Mode::PickAppearance;

	SetControls();
	SetCharacter();
	gui->ShowDialog(this);
	ResetDoll(true);
}

//=================================================================================================
void CreateCharacterPanel::SetControls()
{
	slider[0].val = unit->human_data->hair + 1;
	slider[0].text = Format("%s %d/%d", txHair, slider[0].val, slider[0].maxv);
	slider[1].val = unit->human_data->mustache + 1;
	slider[1].text = Format("%s %d/%d", txMustache, slider[1].val, slider[1].maxv);
	slider[2].val = unit->human_data->beard + 1;
	slider[2].text = Format("%s %d/%d", txBeard, slider[2].val, slider[2].maxv);
	slider[3].val = hair_color_index;
	slider[3].text = Format("%s %d/%d", txHairColor, slider[3].val, slider[3].maxv);
	slider[4].val = int((unit->human_data->height - 0.9f) * 500);
	slider[4].text = Format("%s %d/%d", txSize, slider[4].val, slider[4].maxv);
}

//=================================================================================================
void CreateCharacterPanel::SetCharacter()
{
	anim = anim2 = DA_STAND;
	unit->mesh_inst->Play(NAMES::ani_stand, PLAY_PRIO2 | PLAY_NO_BLEND, 0);
}

//=================================================================================================
void CreateCharacterPanel::OnChangeClass(int index)
{
	clas = reinterpret_cast<Class*>(lbClasses.GetItem()->value);
	ClassChanged();
	reset_skills_perks = true;
	ResetDoll(false);
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
			return SkillGroup::groups[id].name.c_str();
	case Group::Attribute:
		{
			StatProfile& profile = unit->data->GetStatProfile();
			return Format("%s: %d", Attribute::attributes[id].name.c_str(), profile.attrib[id]);
		}
	case Group::Skill:
		{
			StatProfile& profile = unit->data->GetStatProfile();
			return Format("%s: %d", Skill::skills[id].name.c_str(), profile.skill[id]);
		}
	default:
		return "MISSING";
	}
}

//=================================================================================================
void CreateCharacterPanel::GetTooltip(TooltipController* ptr_tool, int group, int id, bool refresh)
{
	TooltipController& tool = *ptr_tool;

	switch((Group)group)
	{
	case Group::Section:
	default:
		tool.anything = false;
		break;
	case Group::Attribute:
		{
			Attribute& ai = Attribute::attributes[id];
			tool.big_text = ai.name;
			tool.text = ai.desc;
			tool.small_text.clear();
			tool.anything = true;
			tool.img = nullptr;
		}
		break;
	case Group::Skill:
		{
			Skill& si = Skill::skills[id];
			tool.big_text = si.name;
			tool.text = si.desc;
			if(si.attrib2 != AttributeId::NONE)
			{
				tool.small_text = Format("%s: %s, %s", txRelatedAttributes, Attribute::attributes[(int)si.attrib].name.c_str(),
					Attribute::attributes[(int)si.attrib2].name.c_str());
			}
			else
				tool.small_text = Format("%s: %s", txRelatedAttributes, Attribute::attributes[(int)si.attrib].name.c_str());
			tool.anything = true;
			tool.img = nullptr;
		}
		break;
	case Group::Perk:
		{
			tool.anything = true;
			tool.img = nullptr;
			tool.small_text.clear();
			PerkInfo& pi = PerkInfo::perks[id];
			tool.big_text = pi.name;
			tool.text = pi.desc;
			if(IsSet(pi.flags, PerkInfo::Flaw))
			{
				tool.text += "\n\n";
				tool.text += txFlawExtraPerk;
			}
		}
		break;
	case Group::TakenPerk:
		{
			TakenPerk& taken = cc.taken_perks[id];
			tool.anything = true;
			tool.img = nullptr;
			PerkInfo& pi = PerkInfo::perks[(int)taken.perk];
			tool.big_text = pi.name;
			tool.text = pi.desc;
			taken.GetDesc(tool.small_text);
			if(IsSet(pi.flags, PerkInfo::Flaw))
			{
				tool.text += "\n\n";
				tool.text += txFlawExtraPerk;
			}
		}
		break;
	}
}

//=================================================================================================
void CreateCharacterPanel::ClassChanged()
{
	unit->data = clas->player;
	anim = DA_STAND;
	t = 1.f;
	tbClassDesc.Reset();
	tbClassDesc.SetText(clas->desc.c_str());
	tbClassDesc.UpdateScrollbar();

	flow_items.clear();

	int y = 0;

	StatProfile& profile = clas->player->GetStatProfile();
	unit->stats->Set(profile);
	unit->CalculateStats();

	// attributes
	flow_items.push_back(OldFlowItem((int)Group::Section, -1, y));
	y += SECTION_H;
	for(int i = 0; i < (int)AttributeId::MAX; ++i)
	{
		flow_items.push_back(OldFlowItem((int)Group::Attribute, i, 35, 70, profile.attrib[i], y));
		y += VALUE_H;
	}

	// skills
	SkillGroupId group = SkillGroupId::NONE;
	for(int i = 0; i < (int)SkillId::MAX; ++i)
	{
		if(profile.skill[i] >= 0)
		{
			Skill& si = Skill::skills[i];
			if(si.group != group)
			{
				// start new section
				flow_items.push_back(OldFlowItem((int)Group::Section, (int)si.group, y));
				y += SECTION_H;
				group = si.group;
			}
			flow_items.push_back(OldFlowItem((int)Group::Skill, i, 0, 20, profile.skill[i], y));
			y += VALUE_H;
		}
	}
	flow_scroll.total = y;
	flow_scroll.part = flow_scroll.size.y;
	flow_scroll.offset = 0.f;

	cc.Clear(clas);
	UpdateInventory();
}

//=================================================================================================
void CreateCharacterPanel::OnPickSkill(int group, int id)
{
	assert(group == (int)Group::PickSkill_Button);

	if(!cc.s[id].add)
	{
		// add
		--cc.sp;
		cc.s[id].Add(Skill::TAG_BONUS, true);
	}
	else
	{
		// remove
		++cc.sp;
		cc.s[id].Add(-Skill::TAG_BONUS, false);
	}

	// update buttons image / text
	FlowItem* find_item = nullptr;
	for(FlowItem* item : flowSkills.items)
	{
		if(item->type == FlowItem::Button)
		{
			if(!cc.s[item->id].add)
			{
				item->state = (cc.sp > 0 ? Button::NONE : Button::DISABLED);
				item->tex_id = 0;
			}
			else
			{
				item->state = Button::NONE;
				item->tex_id = 1;
			}
		}
		else if(item->type == FlowItem::Item && item->id == id)
			find_item = item;
	}

	flowSkills.UpdateText(find_item, Format("%s: %d", Skill::skills[id].name.c_str(), cc.s[id].value));

	UpdateInventory();
}

//=================================================================================================
void CreateCharacterPanel::OnPickPerk(int group, int id)
{
	assert(group == (int)Group::PickPerk_AddButton || group == (int)Group::PickPerk_RemoveButton);

	if(group == (int)Group::PickPerk_AddButton)
	{
		// add perk
		PerkInfo& info = PerkInfo::perks[id];
		switch(info.value_type)
		{
		case PerkInfo::Attribute:
			PickAttribute(txPickAttribIncrease, (Perk)id);
			break;
		case PerkInfo::Skill:
			PickSkill(txPickSkillIncrease, (Perk)id);
			break;
		default:
			AddPerk((Perk)id);
			break;
		}
	}
	else
	{
		// remove perk
		PerkContext ctx(&cc);
		TakenPerk& taken = cc.taken_perks[id];
		taken.Remove(ctx);
		cc.taken_perks.erase(cc.taken_perks.begin() + id);
		vector<Perk> removed;
		ctx.check_remove = true;
		LoopAndRemove(cc.taken_perks, [&](TakenPerk& perk)
		{
			if(perk.CanTake(ctx))
				return false;
			removed.push_back(perk.perk);
			perk.Remove(ctx);
			return true;
		});
		if(!removed.empty())
		{
			string s = txPerksRemoved;
			bool first = true;
			for(Perk p : removed)
			{
				if(first)
				{
					s += " ";
					first = false;
				}
				else
					s += ", ";
				s += PerkInfo::perks[(int)p].name;
			}
			s += ".";
			gui->SimpleDialog(s.c_str(), this);
		}
		CheckSkillsUpdate();
		RebuildPerksFlow();
	}
}

//=================================================================================================
void CreateCharacterPanel::RebuildSkillsFlow()
{
	flowSkills.Clear();
	SkillGroupId last_group = SkillGroupId::NONE;

	for(Skill& si : Skill::skills)
	{
		int i = (int)si.skill_id;
		if(cc.s[i].value >= 0)
		{
			if(si.group != last_group)
			{
				flowSkills.Add()->Set(SkillGroup::groups[(int)si.group].name.c_str());
				last_group = si.group;
			}
			bool plus = !cc.s[i].add;
			flowSkills.Add()->Set((int)Group::PickSkill_Button, i, (plus ? 0 : 1), plus && cc.sp <= 0);
			flowSkills.Add()->Set(Format("%s: %d", si.name.c_str(), cc.s[i].value), (int)Group::Skill, i);
		}
	}

	flowSkills.Reposition();
}

//=================================================================================================
void CreateCharacterPanel::RebuildPerksFlow()
{
	PerkContext ctx(&cc);

	// group perks by availability
	available_perks.clear();
	for(PerkInfo& perk : PerkInfo::perks)
	{
		if(ctx.HavePerk(perk.perk_id))
			continue;
		TakenPerk tp;
		tp.perk = perk.perk_id;
		if(tp.CanTake(ctx))
			available_perks.push_back(perk.perk_id);
	}
	taken_perks.clear();
	LocalVector2<string*> strs;
	for(int i = 0; i < (int)cc.taken_perks.size(); ++i)
	{
		PerkInfo& perk = PerkInfo::perks[(int)cc.taken_perks[i].perk];
		if(IsSet(perk.flags, PerkInfo::RequireFormat))
		{
			string* s = StringPool.Get();
			*s = cc.taken_perks[i].FormatName();
			strs.push_back(s);
			taken_perks.push_back(pair<cstring, int>(s->c_str(), i));
		}
		else
			taken_perks.push_back(pair<cstring, int>(perk.name.c_str(), i));
	}

	// sort perks
	std::sort(available_perks.begin(), available_perks.end(), SortPerks);
	std::sort(taken_perks.begin(), taken_perks.end(), SortTakenPerks);

	// fill flow
	flowPerks.Clear();
	if(!available_perks.empty())
	{
		flowPerks.Add()->Set(txAvailablePerks);
		for(Perk perk : available_perks)
		{
			PerkInfo& info = PerkInfo::perks[(int)perk];
			bool can_pick = (cc.perks == 0 && !IsSet(info.flags, PerkInfo::Flaw));
			flowPerks.Add()->Set((int)Group::PickPerk_AddButton, (int)perk, 0, can_pick);
			flowPerks.Add()->Set(info.name.c_str(), (int)Group::Perk, (int)perk);
		}
	}
	if(!cc.taken_perks.empty())
	{
		flowPerks.Add()->Set(txTakenPerks);
		for(auto& tp : taken_perks)
		{
			flowPerks.Add()->Set((int)Group::PickPerk_RemoveButton, tp.second, 1, false);
			flowPerks.Add()->Set(tp.first, (int)Group::TakenPerk, tp.second);
		}
	}
	flowPerks.Reposition();
	StringPool.Free(strs.Get());
}

//=================================================================================================
void CreateCharacterPanel::ResetSkillsPerks()
{
	reset_skills_perks = false;
	cc.Clear(clas);
	RebuildSkillsFlow();
	RebuildPerksFlow();
	flowSkills.ResetScrollbar();
	flowPerks.ResetScrollbar();
}

//=================================================================================================
void CreateCharacterPanel::PickAttribute(cstring text, Perk perk)
{
	picked_perk = perk;

	PickItemDialogParams params;
	params.event = DialogEvent(this, &CreateCharacterPanel::OnPickAttributeForPerk);
	params.get_tooltip = TooltipController::Callback(this, &CreateCharacterPanel::GetTooltip);
	params.parent = this;
	params.text = text;

	for(int i = 0; i < (int)AttributeId::MAX; ++i)
		params.AddItem(Format("%s: %d", Attribute::attributes[i].name.c_str(), cc.a[i].value), (int)Group::Attribute, i, cc.a[i].mod);

	pickItemDialog = PickItemDialog::Show(params);
}

//=================================================================================================
void CreateCharacterPanel::PickSkill(cstring text, Perk perk)
{
	picked_perk = perk;

	PickItemDialogParams params;
	params.event = DialogEvent(this, &CreateCharacterPanel::OnPickSkillForPerk);
	params.get_tooltip = TooltipController::Callback(this, &CreateCharacterPanel::GetTooltip);
	params.parent = this;
	params.text = text;

	SkillGroupId last_group = SkillGroupId::NONE;
	for(Skill& info : Skill::skills)
	{
		int i = (int)info.skill_id;
		if(cc.s[i].value > 0)
		{
			if(info.group != last_group)
			{
				params.AddSeparator(SkillGroup::groups[(int)info.group].name.c_str());
				last_group = info.group;
			}
			params.AddItem(Format("%s: %d", Skill::skills[i].name.c_str(), cc.s[i].value), (int)Group::Skill, i, cc.s[i].mod);
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

	AddPerk(picked_perk, selected);
}

//=================================================================================================
void CreateCharacterPanel::OnPickSkillForPerk(int id)
{
	if(id == BUTTON_CANCEL)
		return;

	int group, selected;
	pickItemDialog->GetSelected(group, selected);
	AddPerk(picked_perk, selected);
	UpdateInventory();
}

//=================================================================================================
void CreateCharacterPanel::UpdateSkillButtons()
{
	for(FlowItem* item : flowSkills.items)
	{
		if(item->type == FlowItem::Button)
		{
			if(!cc.s[item->id].add)
			{
				item->state = (cc.sp > 0 ? Button::NONE : Button::DISABLED);
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
	TakenPerk taken(perk, value);
	cc.taken_perks.push_back(taken);
	PerkContext ctx(&cc);
	taken.Apply(ctx);
	CheckSkillsUpdate();
	RebuildPerksFlow();
}

//=================================================================================================
bool CreateCharacterPanel::ValidatePerk(Perk perk)
{
	switch(perk)
	{
	case Perk::VeryWealthy:
		return cc.HavePerk(Perk::Wealthy);
	default:
		assert(0);
		return false;
	}
}

//=================================================================================================
void CreateCharacterPanel::CheckSkillsUpdate()
{
	if(cc.update_skills)
	{
		UpdateSkillButtons();
		cc.update_skills = false;
	}
	if(!cc.to_update.empty())
	{
		if(cc.to_update.size() == 1)
		{
			int id = (int)cc.to_update[0];
			flowSkills.UpdateText((int)Group::Skill, id, Format("%s: %d", Skill::skills[id].name.c_str(), cc.s[id].value));
		}
		else
		{
			for(SkillId s : cc.to_update)
			{
				int id = (int)s;
				flowSkills.UpdateText((int)Group::Skill, id, Format("%s: %d", Skill::skills[id].name.c_str(), cc.s[id].value), true);
			}
			flowSkills.UpdateText();
		}
		cc.to_update.clear();
	}

	UpdateInventory();
}

//=================================================================================================
void CreateCharacterPanel::UpdateInventory()
{
	const Item* old_items[SLOT_MAX];
	for(int i = 0; i < SLOT_MAX; ++i)
		old_items[i] = items[i];

	cc.GetStartingItems(items);

	bool same = true;
	for(int i = 0; i < SLOT_MAX; ++i)
	{
		if(items[i] != old_items[i])
		{
			same = false;
			break;
		}
	}
	if(same)
		return;

	for(int i = 0; i < SLOT_MAX; ++i)
	{
		if(items[i])
			game_res->PreloadItem(items[i]);
		unit->slots[i] = items[i];
	}

	bool reset = false;

	if((anim == DA_SHOW_BOW || anim == DA_HIDE_BOW || anim == DA_SHOOT) && !items[SLOT_BOW])
		reset = true;
	if(anim == DA_BLOCK && !items[SLOT_SHIELD])
		reset = true;

	if(reset)
	{
		anim = DA_STAND;
		unit->weapon_state = WS_HIDDEN;
		unit->weapon_taken = W_NONE;
		unit->weapon_hiding = W_NONE;
		t = 0.25f;
	}
}

//=================================================================================================
void CreateCharacterPanel::ResetDoll(bool instant)
{
	anim = DA_STAND;
	unit->weapon_state = WS_HIDDEN;
	unit->weapon_taken = W_NONE;
	unit->weapon_hiding = W_NONE;
	if(instant)
	{
		UpdateUnit(0.f);
		unit->SetAnimationAtEnd();
	}
	if(unit->bow_instance)
		game_level->FreeBowInstance(unit->bow_instance);
	unit->action = A_NONE;
}
