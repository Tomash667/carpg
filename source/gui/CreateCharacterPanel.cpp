#include "Pch.h"
#include "CreateCharacterPanel.h"

#include "Game.h"
#include "GameGui.h"
#include "GameResources.h"
#include "Language.h"
#include "Level.h"
#include "Unit.h"

#include <Camera.h>
#include <GetTextDialog.h>
#include <PickItemDialog.h>
#include <Render.h>
#include <RenderTarget.h>
#include <ResourceManager.h>
#include <Scene.h>
#include <SceneManager.h>

//-----------------------------------------------------------------------------
const int SECTION_H = 40;
const int VALUE_H = 20;
const int FLOW_BORDER = 3;
const int FLOW_PADDING = 2;

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
CreateCharacterPanel::CreateCharacterPanel(DialogInfo& info) : DialogBox(info), unit(nullptr), rtChar(nullptr), scene(nullptr), camera(nullptr)
{
	size = Int2(700, 500);
	unit = new Unit;
	unit->humanData = new Human;
	unit->player = nullptr;
	unit->ai = nullptr;
	unit->hero = nullptr;
	unit->usedItem = nullptr;
	unit->weaponState = WeaponState::Hidden;
	unit->pos = unit->visualPos = Vec3(0, 0, 0);
	unit->rot = 0.f;
	unit->fakeUnit = true;
	unit->action = A_NONE;
	unit->stats = new UnitStats;
	unit->stats->fixed = false;
	unit->stats->subprofile.value = 0;
	unit->stamina = unit->staminaMax = 100.f;
	unit->usable = nullptr;
	unit->liveState = Unit::ALIVE;

	btCancel.id = IdCancel;
	btCancel.custom = &customClose;
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

	checkbox.btSize = Int2(32, 32);
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
		s.width = 180;
		s.parent = this;
	}

	{
		Slider& s = slider[1];
		s.id = IdMustache;
		s.minv = 0;
		s.maxv = MAX_MUSTACHE - 1;
		s.val = 0;
		s.pos = Int2(20, 150);
		s.width = 180;
		s.parent = this;
	}

	{
		Slider& s = slider[2];
		s.id = IdBeard;
		s.minv = 0;
		s.maxv = MAX_BEARD - 1;
		s.val = 0;
		s.pos = Int2(20, 200);
		s.width = 180;
		s.parent = this;
	}

	{
		Slider& s = slider[3];
		s.id = IdColor;
		s.minv = 0;
		s.maxv = nHairColors - 1;
		s.val = 0;
		s.pos = Int2(20, 250);
		s.width = 180;
		s.parent = this;
	}

	{
		Slider& s = slider[4];
		s.id = IdSize;
		s.minv = 0;
		s.maxv = 100;
		s.val = 50;
		s.pos = Int2(20, 300);
		s.width = 180;
		s.parent = this;
		s.SetHold(25.f);
	}

	lbClasses.pos = Int2(16, 55);
	lbClasses.size = Int2(228, 253);
	lbClasses.SetForceImageSize(Int2(20, 20));
	lbClasses.SetItemHeight(24);
	lbClasses.eventHandler = DialogEvent(this, &CreateCharacterPanel::OnChangeClass);
	lbClasses.parent = this;

	tbClassDesc.size = Int2(340, 93);
	tbClassDesc.pos = Int2((size.x - tbClassDesc.size.x) / 2, 335);
	tbClassDesc.SetReadonly(true);
	tbClassDesc.AddScrollbar();

	tbInfo.size = Int2(340, 93);
	tbInfo.pos = Int2((size.x - tbInfo.size.x) / 2, 335);
	tbInfo.SetReadonly(true);
	tbInfo.AddScrollbar();

	flowSize = Int2(212, 253);
	flowPos = Int2(size.x - flowSize.x - 33, 55);
	flowScroll.pos = Int2(flowPos.x + flowSize.x - 1, flowPos.y);
	flowScroll.size = Int2(16, flowSize.y);
	flowScroll.total = 100;
	flowScroll.part = 10;

	tooltip.Init(TooltipController::Callback(this, &CreateCharacterPanel::GetTooltip));

	flowSkills.size = Int2(228, 253);
	flowSkills.pos = Int2(16, 55);
	flowSkills.buttonSize = Int2(16, 16);
	flowSkills.buttonTex = customBt;
	flowSkills.onButton = ButtonEvent(this, &CreateCharacterPanel::OnPickSkill);

	flowPerks.size = Int2(228, 253);
	flowPerks.pos = Int2(size.x - flowPerks.size.x - 17, 55);
	flowPerks.buttonSize = Int2(16, 16);
	flowPerks.buttonTex = customBt;
	flowPerks.onButton = ButtonEvent(this, &CreateCharacterPanel::OnPickPerk);
}

//=================================================================================================
CreateCharacterPanel::~CreateCharacterPanel()
{
	delete unit;
	delete scene;
	delete camera;
}

//=================================================================================================
void CreateCharacterPanel::LoadLanguage()
{
	txAttributes = Str("attributes");
	txRelatedAttributes = Str("relatedAttributes");

	Language::Section section = Language::GetSection("CreateCharacterPanel");
	txHardcoreMode = section.Get("hardcoreMode");
	txHardcoreDesc = section.Get("hardcoreDesc");
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
	tBox = resMgr->Load<Texture>("box.png");
	tPowerBar = resMgr->Load<Texture>("klasa_cecha.png");
	tArrowLeft = resMgr->Load<Texture>("page_prev.png");
	tArrowRight = resMgr->Load<Texture>("page_next.png");
	customClose.tex[Button::NONE] = AreaLayout(resMgr->Load<Texture>("close.png"));
	customClose.tex[Button::HOVER] = AreaLayout(resMgr->Load<Texture>("close_hover.png"));
	customClose.tex[Button::DOWN] = AreaLayout(resMgr->Load<Texture>("close_down.png"));
	customClose.tex[Button::DISABLED] = AreaLayout(resMgr->Load<Texture>("close_disabled.png"));
	customBt[0].tex[Button::NONE] = AreaLayout(resMgr->Load<Texture>("plus.png"));
	customBt[0].tex[Button::HOVER] = AreaLayout(resMgr->Load<Texture>("plus_hover.png"));
	customBt[0].tex[Button::DOWN] = AreaLayout(resMgr->Load<Texture>("plus_down.png"));
	customBt[0].tex[Button::DISABLED] = AreaLayout(resMgr->Load<Texture>("plus_disabled.png"));
	customBt[1].tex[Button::NONE] = AreaLayout(resMgr->Load<Texture>("minus.png"));
	customBt[1].tex[Button::HOVER] = AreaLayout(resMgr->Load<Texture>("minus_hover.png"));
	customBt[1].tex[Button::DOWN] = AreaLayout(resMgr->Load<Texture>("minus_down.png"));
	customBt[1].tex[Button::DISABLED] = AreaLayout(resMgr->Load<Texture>("minus_disabled.png"));

	rtChar = render->CreateRenderTarget(Int2(256, 256));

	scene = new Scene;
	scene->fogRange = Vec2(40, 80);
	scene->fogColor = Color(0.9f, 0.85f, 0.8f);
	scene->ambientColor = Color(0.5f, 0.5f, 0.5f);
	scene->lightColor = Color::White;
	scene->lightDir = Vec3(sin(PI / 2), 2.f, cos(PI / 2)).Normalize();
	scene->useLightDir = true;

	camera = new Camera;
	camera->from = Vec3(0, 2, -2.5f);
	camera->to = Vec3(0, 1, 0);
	camera->aspect = 1.f;
	camera->znear = 1.f;
	camera->zfar = 5.f;
	camera->UpdateMatrix();
}

//=================================================================================================
void CreateCharacterPanel::Draw()
{
	DrawPanel();

	// top text
	Rect rect0 = { 12 + pos.x, 12 + pos.y, pos.x + size.x - 12, 12 + pos.y + 72 };
	gui->DrawText(GameGui::fontBig, txCharacterCreation, DTF_CENTER, Color::Black, rect0);

	// arrows
	gui->DrawSprite(tArrowLeft, Int2(pos.x + size.x / 2 - 75, pos.y + 164));
	gui->DrawSprite(tArrowRight, Int2(pos.x + size.x / 2 + 75 - 16, pos.y + 164));

	// character
	gui->DrawSprite(rtChar, Int2(pos.x + (size.x - 256) / 2, pos.y + 64));

	// close button
	btCancel.Draw();

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
			Int2 fpos = flowPos + globalPos;
			gui->DrawItem(tBox, fpos, flowSize, Color::White, 8, 32);
			flowScroll.Draw();

			const Rect clipRect = Rect::Create(fpos, flowSize, FLOW_BORDER);
			Rect part = { 0, 0, 256, 32 };
			int y = clipRect.Top() - (int)flowScroll.offset;

			for(OldFlowItem& fi : flowItems)
			{
				cstring itemText = GetText(fi.group, fi.id);
				if(fi.section)
				{
					Rect rect = { clipRect.Left() + FLOW_PADDING, y, clipRect.Right() - FLOW_PADDING, y + SECTION_H };
					y += SECTION_H;
					if(!gui->DrawText(GameGui::fontBig, itemText, DTF_SINGLELINE, Color::Black, rect, &clipRect))
						break;
				}
				else
				{
					if(fi.part > 0)
					{
						const Vec2 scale(float(flowSize.x - 4) / 256, 17.f / 32);
						const Vec2 pos((float)clipRect.Left(), (float)y);
						const Matrix mat = Matrix::Transform2D(nullptr, 0.f, &scale, nullptr, 0.f, &pos);
						part.Right() = int(fi.part * 256);
						gui->DrawSprite2(tPowerBar, mat, &part, &clipRect, Color::White);
					}

					Rect rect = { clipRect.Left() + FLOW_PADDING, y, clipRect.Right() - FLOW_PADDING, y + VALUE_H };
					y += VALUE_H;
					if(!gui->DrawText(GameGui::font, itemText, DTF_SINGLELINE, Color::Black, rect, &clipRect))
						break;
				}
			}
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
			Rect r = { globalPos.x + 16, globalPos.y + 310, globalPos.x + 216, globalPos.y + 360 };
			gui->DrawText(GameGui::font, Format(txSkillPoints, cc.sp, cc.spMax), 0, Color::Black, r);

			// right text "Feats: X/Y"
			Rect r2 = { globalPos.x + size.x - 216, globalPos.y + 310, globalPos.x + size.x - 16, globalPos.y + 360 };
			gui->DrawText(GameGui::font, Format(txPerkPoints, cc.perks, cc.perksMax), DTF_RIGHT, Color::Black, r2);
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

	tooltip.Draw();
}

//=================================================================================================
void CreateCharacterPanel::Update(float dt)
{
	RenderUnit();
	UpdateUnit(dt);

	int group = -1, id = -1;

	// rotating unit
	const Int2 regionPos(pos.x + (size.x - 256) / 2 + 45, pos.y + 64);
	if(Rect::IsInside(gui->cursorPos, regionPos, Int2(166, 256)) && input->Focus() && focus)
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
			unit->rot = Clip(unit->rot - float(gui->cursorPos.x - pos.x - size.x / 2) / 16 * dt);
	}
	else
		rotating = false;

	// x button
	btCancel.mouseFocus = focus;
	btCancel.Update(dt);

	switch(mode)
	{
	case Mode::PickClass:
		{
			lbClasses.mouseFocus = focus;
			lbClasses.Update(dt);

			btNext.mouseFocus = focus;
			btNext.Update(dt);

			tbClassDesc.mouseFocus = focus;
			tbClassDesc.Update(dt);

			flowScroll.mouseFocus = focus;
			flowScroll.Update(dt);

			if(!flowScroll.clicked && Rect::IsInside(gui->cursorPos, flowPos + globalPos, flowSize))
			{
				if(focus && input->Focus())
					flowScroll.ApplyMouseWheel();

				int y = gui->cursorPos.y - globalPos.y - flowPos.y - FLOW_BORDER + (int)flowScroll.offset;
				for(OldFlowItem& fi : flowItems)
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
			else if(!flowScroll.clicked && Rect::IsInside(gui->cursorPos, flowScroll.pos + globalPos, flowScroll.size) && focus && input->Focus())
				flowScroll.ApplyMouseWheel();
		}
		break;
	case Mode::PickSkillPerk:
		{
			btBack.mouseFocus = focus;
			btBack.Update(dt);

			btNext.mouseFocus = focus;
			btNext.Update(dt);

			btRandomSet.mouseFocus = focus;
			btRandomSet.Update(dt);

			tbInfo.mouseFocus = focus;
			tbInfo.Update(dt);

			flowSkills.mouseFocus = focus;
			flowSkills.Update(dt);
			flowSkills.GetSelected(group, id);

			flowPerks.mouseFocus = focus;
			flowPerks.Update(dt);
			flowPerks.GetSelected(group, id);
		}
		break;
	case Mode::PickAppearance:
		{
			btBack.mouseFocus = focus;
			btBack.Update(dt);

			btCreate.mouseFocus = focus;
			btCreate.Update(dt);

			btRandomSet.mouseFocus = focus;
			btRandomSet.Update(dt);

			if(!Net::IsOnline())
			{
				checkbox.mouseFocus = focus;
				checkbox.Update(dt);
				if(focus && checkbox.IsInside(gui->cursorPos))
				{
					group = (int)Group::Hardcore;
					id = 0;
				}
			}

			for(int i = 0; i < 5; ++i)
			{
				slider[i].mouseFocus = focus;
				slider[i].Update(dt);
			}
		}
		break;
	}

	tooltip.UpdateTooltip(dt, group, id);

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
		}
		pos = globalPos = (gui->wndSize - size) / 2;
		btCancel.globalPos = globalPos + btCancel.pos;
		btBack.globalPos = globalPos + btBack.pos;
		btNext.globalPos = globalPos + btNext.pos;
		btCreate.globalPos = globalPos + btCreate.pos;
		btRandomSet.globalPos = globalPos + btRandomSet.pos;
		for(int i = 0; i < 5; ++i)
			slider[i].globalPos = globalPos + slider[i].pos;
		checkbox.globalPos = globalPos + checkbox.pos;
		lbClasses.Event(GuiEvent_Moved);
		tbClassDesc.Move(pos);
		tbInfo.Move(pos);
		flowScroll.globalPos = globalPos + flowScroll.pos;
		flowSkills.UpdatePos(globalPos);
		flowPerks.UpdatePos(globalPos);
		tooltip.Clear();
	}
	else if(e == GuiEvent_Close)
	{
		visible = false;
		lbClasses.Event(GuiEvent_LostFocus);
		tbClassDesc.LostFocus();
		flowScroll.LostFocus();
	}
	else if(e == GuiEvent_LostFocus)
	{
		lbClasses.Event(GuiEvent_LostFocus);
		tbClassDesc.LostFocus();
		flowScroll.LostFocus();
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
				if(resetSkillsPerks)
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
					di.order = DialogOrder::Top;
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
			if(enterName)
			{
				GetTextDialogParams params(Format("%s:", txName), playerName);
				params.event = [this](int id)
				{
					if(id == BUTTON_OK)
					{
						lastHairColorIndex = hairColorIndex;
						CloseDialog();
						event(BUTTON_OK);
					}
				};
				params.validate = [](const string& str)
				{
					return !Trimmed(str).empty();
				};
				params.limit = 16;
				params.parent = this;
				GetTextDialog::Show(params);
			}
			else
			{
				lastHairColorIndex = hairColorIndex;
				CloseDialog();
				event(BUTTON_OK);
			}
			break;
		case IdHardcore:
			game->hardcoreOption = checkbox.checked;
			game->cfg.Add("hardcoreOption", game->hardcoreOption);
			game->SaveCfg();
			break;
		case IdHair:
			unit->humanData->hair = slider[0].val - 1;
			slider[0].text = Format("%s %d/%d", txHair, slider[0].val, slider[0].maxv);
			break;
		case IdMustache:
			unit->humanData->mustache = slider[1].val - 1;
			slider[1].text = Format("%s %d/%d", txMustache, slider[1].val, slider[1].maxv);
			break;
		case IdBeard:
			unit->humanData->beard = slider[2].val - 1;
			slider[2].text = Format("%s %d/%d", txBeard, slider[2].val, slider[2].maxv);
			break;
		case IdColor:
			unit->humanData->hairColor = gHairColors[slider[3].val];
			slider[3].text = Format("%s %d/%d", txHairColor, slider[3].val, slider[3].maxv);
			break;
		case IdSize:
			unit->humanData->height = Lerp(MIN_HEIGHT, MAX_HEIGHT, float(slider[4].val) / 100);
			slider[4].text = Format("%s %d/%d", txSize, slider[4].val, slider[4].maxv);
			unit->humanData->ApplyScale(unit->meshInst);
			unit->meshInst->needUpdate = true;
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
	render->SetRenderTarget(rtChar);
	render->Clear(Color::None);

	FrustumPlanes frustum(camera->matViewProj);

	sceneMgr->SetScene(scene, camera);

	game->drawBatch.Clear();
	game->drawBatch.locPart = nullptr;
	game->drawBatch.scene = scene;
	game->drawBatch.camera = camera;
	game->drawBatch.gatherLights = false;
	game->ListDrawObjectsUnit(frustum, *unit);
	game->drawBatch.Process();
	sceneMgr->DrawSceneNodes(game->drawBatch);

	render->SetRenderTarget(nullptr);
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
			{
				anim = DA_BATTLE_MODE;
				unit->meshInst->Deactivate(1);
			}
			break;
		case DA_BATTLE_MODE:
			if(unit->weaponTaken == W_ONE_HANDED)
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
			unit->action = A_ATTACK;
			unit->animationState = AS_ATTACK_PREPARE;
			unit->act.attack.index = unit->GetRandomAttack();
			unit->act.attack.run = false;
			unit->meshInst->Play(NAMES::aniAttacks[unit->act.attack.index], PLAY_PRIO1 | PLAY_ONCE, 1);
			unit->meshInst->groups[1].speed = unit->GetAttackSpeed();
			t = 100.f;
			break;
		case DA_BLOCK:
			unit->meshInst->Play(NAMES::aniBlock, PLAY_PRIO2, 1);
			t = 1.f;
			break;
		case DA_BATTLE_MODE:
			if(unit->weaponTaken == W_ONE_HANDED)
				unit->animation = ANI_BATTLE;
			else
				unit->animation = ANI_BATTLE_BOW;
			t = 1.f;
			break;
		case DA_WALK:
			unit->animation = ANI_WALK;
			t = 2.f;
			break;
		case DA_LOOKS_AROUND:
			unit->meshInst->Play("rozglada", PLAY_PRIO2 | PLAY_ONCE, 0);
			unit->animation = ANI_IDLE;
			t = 100.f;
			break;
		case DA_HIDE_WEAPON:
			unit->SetWeaponState(false, W_ONE_HANDED, false);
			unit->animation = ANI_STAND;
			t = 100.f;
			break;
		case DA_HIDE_BOW:
			unit->SetWeaponState(false, W_BOW, false);
			unit->animation = ANI_STAND;
			t = 100.f;
			break;
		case DA_STAND:
			unit->animation = ANI_STAND;
			t = 2.f;
			break;
		case DA_SHOOT:
			unit->DoRangedAttack(true);
			t = 100.f;
			break;
		case DA_SHOW_WEAPON:
			unit->SetWeaponState(true, W_ONE_HANDED, false);
			unit->animation = ANI_BATTLE;
			t = 100.f;
			break;
		case DA_SHOW_BOW:
			unit->SetWeaponState(true, W_BOW, false);
			unit->animation = ANI_BATTLE_BOW;
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
		if(unit->meshInst->IsEnded())
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
		if(unit->weaponState == WeaponState::Hidden)
		{
			anim = DA_STAND;
			t = 1.f;
		}
		break;
	case DA_SHOOT:
		if(unit->action == A_NONE)
		{
			if(Rand() % 2 == 0)
			{
				anim = DA_SHOOT;
				anim2 = DA_STAND;
			}
			else
				anim = DA_BATTLE_MODE;
		}
		break;
	case DA_SHOW_WEAPON:
		if(unit->weaponState == WeaponState::Taken)
			anim = DA_ATTACK;
		break;
	case DA_SHOW_BOW:
		if(unit->weaponState == WeaponState::Taken)
			anim = DA_SHOOT;
		break;
	case DA_LOOKS_AROUND:
		if(unit->animation != ANI_IDLE)
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

	unit->Update(dt);
}

//=================================================================================================
void CreateCharacterPanel::Init()
{
	unit->meshInst = new MeshInstance(gameRes->aHuman);

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
	u.humanData->beard = Rand() % MAX_BEARD - 1;
	u.humanData->hair = Rand() % MAX_HAIR - 1;
	u.humanData->mustache = Rand() % MAX_MUSTACHE - 1;
	hairColorIndex = Rand() % nHairColors;
	u.humanData->hairColor = gHairColors[hairColorIndex];
	u.humanData->height = Random(0.95f, 1.05f);
	u.humanData->ApplyScale(u.meshInst);
	SetControls();
}

//=================================================================================================
void CreateCharacterPanel::Show(bool enterName)
{
	clas = Class::GetRandomPlayer();
	lbClasses.Select(lbClasses.FindIndex((int)clas));
	ClassChanged();
	RandomAppearance();

	resetSkillsPerks = true;
	this->enterName = enterName;
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
	hairColorIndex = lastHairColorIndex;
	unit->ApplyHumanData(hd);
	this->cc = cc;
	RebuildSkillsFlow();
	RebuildPerksFlow();
	UpdateInventory();

	resetSkillsPerks = false;
	enterName = false;
	mode = Mode::PickAppearance;

	SetControls();
	SetCharacter();
	gui->ShowDialog(this);
	ResetDoll(true);
}

//=================================================================================================
void CreateCharacterPanel::SetControls()
{
	slider[0].val = unit->humanData->hair + 1;
	slider[0].text = Format("%s %d/%d", txHair, slider[0].val, slider[0].maxv);
	slider[1].val = unit->humanData->mustache + 1;
	slider[1].text = Format("%s %d/%d", txMustache, slider[1].val, slider[1].maxv);
	slider[2].val = unit->humanData->beard + 1;
	slider[2].text = Format("%s %d/%d", txBeard, slider[2].val, slider[2].maxv);
	slider[3].val = hairColorIndex;
	slider[3].text = Format("%s %d/%d", txHairColor, slider[3].val, slider[3].maxv);
	slider[4].val = int((unit->humanData->height - MIN_HEIGHT) * 500);
	slider[4].text = Format("%s %d/%d", txSize, slider[4].val, slider[4].maxv);
}

//=================================================================================================
void CreateCharacterPanel::SetCharacter()
{
	anim = anim2 = DA_STAND;
	unit->animation = ANI_STAND;
	unit->currentAnimation = ANI_STAND;
	unit->meshInst->Play(NAMES::aniStand, PLAY_PRIO2 | PLAY_NO_BLEND, 0);
}

//=================================================================================================
void CreateCharacterPanel::OnChangeClass(int index)
{
	clas = reinterpret_cast<Class*>(lbClasses.GetItem()->value);
	ClassChanged();
	resetSkillsPerks = true;
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
void CreateCharacterPanel::GetTooltip(TooltipController* ptrTool, int group, int id, bool refresh)
{
	TooltipController& tool = *ptrTool;

	switch((Group)group)
	{
	case Group::Section:
	default:
		tool.anything = false;
		break;
	case Group::Attribute:
		{
			Attribute& ai = Attribute::attributes[id];
			tool.bigText = ai.name;
			tool.text = ai.desc;
			tool.smallText.clear();
			tool.anything = true;
			tool.img = nullptr;
		}
		break;
	case Group::Skill:
		{
			Skill& si = Skill::skills[id];
			tool.bigText = si.name;
			tool.text = si.desc;
			if(si.attrib2 != AttributeId::NONE)
			{
				tool.smallText = Format("%s: %s, %s", txRelatedAttributes, Attribute::attributes[(int)si.attrib].name.c_str(),
					Attribute::attributes[(int)si.attrib2].name.c_str());
			}
			else
				tool.smallText = Format("%s: %s", txRelatedAttributes, Attribute::attributes[(int)si.attrib].name.c_str());
			tool.anything = true;
			tool.img = nullptr;
		}
		break;
	case Group::Perk:
		{
			Perk* perk = (Perk*)id;
			tool.anything = true;
			tool.img = nullptr;
			tool.smallText.clear();
			tool.bigText = perk->name;
			tool.text = perk->desc;
			if(IsSet(perk->flags, Perk::Flaw))
			{
				tool.text += "\n\n";
				tool.text += txFlawExtraPerk;
			}
		}
		break;
	case Group::TakenPerk:
		{
			TakenPerk& tp = cc.takenPerks[id];
			tool.anything = true;
			tool.img = nullptr;
			tool.bigText = tp.perk->name;
			tool.text = tp.perk->desc;
			tp.GetDetails(tool.smallText);
			if(IsSet(tp.perk->flags, Perk::Flaw))
			{
				tool.text += "\n\n";
				tool.text += txFlawExtraPerk;
			}
		}
		break;
	case Group::Hardcore:
		tool.anything = true;
		tool.img = nullptr;
		tool.bigText.clear();
		tool.text = txHardcoreDesc;
		tool.smallText.clear();
		break;
	}
}

//=================================================================================================
void CreateCharacterPanel::ClassChanged()
{
	anim = DA_STAND;
	unit->data = clas->player;
	unit->animation = ANI_STAND;
	unit->SetWeaponStateInstant(WeaponState::Hidden, W_NONE);
	t = 1.f;
	tbClassDesc.Reset();
	tbClassDesc.SetText(clas->desc.c_str());
	tbClassDesc.UpdateScrollbar();

	flowItems.clear();

	int y = 0;

	StatProfile& profile = clas->player->GetStatProfile();
	unit->stats->Set(profile);
	unit->CalculateStats();

	// attributes
	flowItems.push_back(OldFlowItem((int)Group::Section, -1, y));
	y += SECTION_H;
	for(int i = 0; i < (int)AttributeId::MAX; ++i)
	{
		flowItems.push_back(OldFlowItem((int)Group::Attribute, i, 35, 70, profile.attrib[i], y));
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
				flowItems.push_back(OldFlowItem((int)Group::Section, (int)si.group, y));
				y += SECTION_H;
				group = si.group;
			}
			flowItems.push_back(OldFlowItem((int)Group::Skill, i, 0, 20, profile.skill[i], y));
			y += VALUE_H;
		}
	}
	flowScroll.total = y + FLOW_BORDER * 2;
	flowScroll.part = flowScroll.size.y;
	flowScroll.offset = 0.f;

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
	FlowItem* findItem = nullptr;
	for(FlowItem* item : flowSkills.items)
	{
		if(item->type == FlowItem::Button)
		{
			if(!cc.s[item->id].add)
			{
				item->state = (cc.sp > 0 ? Button::NONE : Button::DISABLED);
				item->texId = 0;
			}
			else
			{
				item->state = Button::NONE;
				item->texId = 1;
			}
		}
		else if(item->type == FlowItem::Item && item->id == id)
			findItem = item;
	}

	flowSkills.UpdateText(findItem, Format("%s: %d", Skill::skills[id].name.c_str(), cc.s[id].value));

	UpdateInventory();
}

//=================================================================================================
void CreateCharacterPanel::OnPickPerk(int group, int id)
{
	assert(group == (int)Group::PickPerk_AddButton || group == (int)Group::PickPerk_RemoveButton);

	if(group == (int)Group::PickPerk_AddButton)
	{
		// add perk
		Perk* perk = (Perk*)id;
		switch(perk->valueType)
		{
		case Perk::Attribute:
			PickAttribute(txPickAttribIncrease, perk);
			break;
		case Perk::Skill:
			PickSkill(txPickSkillIncrease, perk);
			break;
		default:
			AddPerk(perk);
			break;
		}
	}
	else
	{
		// remove perk
		PerkContext ctx(&cc);
		TakenPerk& taken = cc.takenPerks[id];
		taken.Remove(ctx);
		cc.takenPerks.erase(cc.takenPerks.begin() + id);
		vector<Perk*> removed;
		ctx.checkRemove = true;
		LoopAndRemove(cc.takenPerks, [&](TakenPerk& perk)
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
			for(Perk* perk : removed)
			{
				if(first)
				{
					s += " ";
					first = false;
				}
				else
					s += ", ";
				s += perk->name;
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
	SkillGroupId lastGroup = SkillGroupId::NONE;

	for(Skill& si : Skill::skills)
	{
		int i = (int)si.skillId;
		if(cc.s[i].value >= 0)
		{
			if(si.group != lastGroup)
			{
				flowSkills.Add()->Set(SkillGroup::groups[(int)si.group].name.c_str());
				lastGroup = si.group;
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
	availablePerks.clear();
	for(Perk* perk : Perk::perks)
	{
		if(ctx.HavePerk(perk))
			continue;
		TakenPerk tp;
		tp.perk = perk;
		if(tp.CanTake(ctx))
			availablePerks.push_back(perk);
	}
	takenPerks.clear();
	LocalVector<string*> strs;
	for(int i = 0; i < (int)cc.takenPerks.size(); ++i)
	{
		Perk* perk = cc.takenPerks[i].perk;
		if(perk->valueType != Perk::None)
		{
			string* s = StringPool.Get();
			*s = cc.takenPerks[i].FormatName();
			strs.push_back(s);
			takenPerks.push_back(pair<cstring, int>(s->c_str(), i));
		}
		else
			takenPerks.push_back(pair<cstring, int>(perk->name.c_str(), i));
	}

	// sort perks
	std::sort(availablePerks.begin(), availablePerks.end(), SortPerks);
	std::sort(takenPerks.begin(), takenPerks.end(), SortTakenPerks);

	// fill flow
	flowPerks.Clear();
	if(!availablePerks.empty())
	{
		flowPerks.Add()->Set(txAvailablePerks);
		for(Perk* perk : availablePerks)
		{
			bool canPick = (cc.perks == 0 && !IsSet(perk->flags, Perk::Flaw));
			flowPerks.Add()->Set((int)Group::PickPerk_AddButton, (int)perk, 0, canPick);
			flowPerks.Add()->Set(perk->name.c_str(), (int)Group::Perk, (int)perk);
		}
	}
	if(!cc.takenPerks.empty())
	{
		flowPerks.Add()->Set(txTakenPerks);
		for(auto& tp : takenPerks)
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
	resetSkillsPerks = false;
	cc.Clear(clas);
	RebuildSkillsFlow();
	RebuildPerksFlow();
	flowSkills.ResetScrollbar();
	flowPerks.ResetScrollbar();
}

//=================================================================================================
void CreateCharacterPanel::PickAttribute(cstring text, Perk* perk)
{
	pickedPerk = perk;

	PickItemDialogParams params;
	params.event = DialogEvent(this, &CreateCharacterPanel::OnPickAttributeForPerk);
	params.getTooltip = TooltipController::Callback(this, &CreateCharacterPanel::GetTooltip);
	params.parent = this;
	params.text = text;

	for(int i = 0; i < (int)AttributeId::MAX; ++i)
		params.AddItem(Format("%s: %d", Attribute::attributes[i].name.c_str(), cc.a[i].value), (int)Group::Attribute, i, cc.a[i].mod);

	pickItemDialog = PickItemDialog::Show(params);
}

//=================================================================================================
void CreateCharacterPanel::PickSkill(cstring text, Perk* perk)
{
	pickedPerk = perk;

	PickItemDialogParams params;
	params.event = DialogEvent(this, &CreateCharacterPanel::OnPickSkillForPerk);
	params.getTooltip = TooltipController::Callback(this, &CreateCharacterPanel::GetTooltip);
	params.parent = this;
	params.text = text;

	SkillGroupId lastGroup = SkillGroupId::NONE;
	for(Skill& info : Skill::skills)
	{
		int i = (int)info.skillId;
		if(cc.s[i].value > 0)
		{
			if(info.group != lastGroup)
			{
				params.AddSeparator(SkillGroup::groups[(int)info.group].name.c_str());
				lastGroup = info.group;
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

	AddPerk(pickedPerk, selected);
}

//=================================================================================================
void CreateCharacterPanel::OnPickSkillForPerk(int id)
{
	if(id == BUTTON_CANCEL)
		return;

	int group, selected;
	pickItemDialog->GetSelected(group, selected);
	AddPerk(pickedPerk, selected);
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
				item->texId = 0;
			}
			else
			{
				item->state = Button::NONE;
				item->texId = 1;
			}
		}
	}
}

//=================================================================================================
void CreateCharacterPanel::AddPerk(Perk* perk, int value)
{
	TakenPerk taken(perk, value);
	cc.takenPerks.push_back(taken);
	PerkContext ctx(&cc);
	taken.Apply(ctx);
	CheckSkillsUpdate();
	RebuildPerksFlow();
}

//=================================================================================================
void CreateCharacterPanel::CheckSkillsUpdate()
{
	if(cc.updateSkills)
	{
		UpdateSkillButtons();
		cc.updateSkills = false;
	}

	if(!cc.toUpdate.empty())
	{
		if(cc.toUpdate.size() == 1)
		{
			int id = (int)cc.toUpdate[0];
			flowSkills.UpdateText((int)Group::Skill, id, Format("%s: %d", Skill::skills[id].name.c_str(), cc.s[id].value));
		}
		else
		{
			for(SkillId s : cc.toUpdate)
			{
				int id = (int)s;
				flowSkills.UpdateText((int)Group::Skill, id, Format("%s: %d", Skill::skills[id].name.c_str(), cc.s[id].value), true);
			}
			flowSkills.UpdateText();
		}
		cc.toUpdate.clear();
	}

	UpdateInventory();
}

//=================================================================================================
void CreateCharacterPanel::UpdateInventory()
{
	array<const Item*, SLOT_MAX> oldItems = items;

	cc.GetStartingItems(items);

	if(items == oldItems)
		return;

	unit->ReplaceItems(items);

	bool reset = false;

	if((anim == DA_SHOW_BOW || anim == DA_HIDE_BOW || anim == DA_SHOOT) && !items[SLOT_BOW])
		reset = true;
	if(anim == DA_BLOCK && !items[SLOT_SHIELD])
		reset = true;

	if(reset)
	{
		anim = DA_STAND;
		unit->SetWeaponStateInstant(WeaponState::Hidden, W_NONE);
		t = 0.25f;
	}
}

//=================================================================================================
void CreateCharacterPanel::ResetDoll(bool instant)
{
	anim = DA_STAND;
	unit->meshInst->Deactivate(1);
	unit->SetWeaponStateInstant(WeaponState::Hidden, W_NONE);
	if(instant)
	{
		UpdateUnit(0.f);
		unit->SetAnimationAtEnd();
	}
	if(unit->bowInstance)
		gameLevel->FreeBowInstance(unit->bowInstance);
	unit->action = A_NONE;
}
