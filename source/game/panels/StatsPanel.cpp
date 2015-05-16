#include "Pch.h"
#include "Base.h"
#include "StatsPanel.h"
#include "Unit.h"
#include "PlayerController.h"
#include "KeyStates.h"
#include "Game.h"
#include "Language.h"

//-----------------------------------------------------------------------------
#define INDEX_ATTRIB 0
#define INDEX_SKILL A_MAX
#define INDEX_DATA A_MAX+S_MAX

//=================================================================================================
StatsPanel::StatsPanel(const INT2& _pos, const INT2& _size)
{
	min_size = INT2(32,32);
	pos = global_pos = _pos;
	size = _size;

	txStatsPanel = Str("statsPanel");
	txTraitsText = Str("traitsText");
	txStatsText = Str("statsText");
	txYearMonthDay = Str("yearMonthDay");
	txBase = Str("base");

	flow.pos = INT2(8,40);
	flow.global_pos = global_pos + flow.pos;
	flow.size = size - INT2(44,48);
	flow.parent = this;
	flow.moved = 0;

	flow.Add(new StaticText(Str("attributes"), GUI.fBig));

	tAttribs = new StaticText("", GUI.default_font);
	flow.Add(tAttribs);

	flow.Add(new StaticText(Str("skills"), GUI.fBig));

	tSkills = new StaticText("", GUI.default_font);
	flow.Add(tSkills);

	flow.Add(new StaticText(Str("traits"), GUI.fBig));

	tTraits = new StaticText("", GUI.default_font);
	flow.Add(tTraits);

	flow.Add(new StaticText(Str("stats"), GUI.fBig));

	tStats = new StaticText("", GUI.default_font);
	flow.Add(tStats);

	scrollbar.pos = INT2(size.x-28,48);
	scrollbar.size = INT2(16,size.y-60);
	scrollbar.global_pos = global_pos + scrollbar.pos;
	scrollbar.total = 100;
	scrollbar.offset = 0;
	scrollbar.part = 100;
}

//=================================================================================================
StatsPanel::~StatsPanel()
{
	flow.DeleteItems();
}

//=================================================================================================
void StatsPanel::Draw(ControlDrawData*)
{
	GamePanel::Draw();
	scrollbar.Draw();

	SetText();

	RECT rect = {
		pos.x+8,
		pos.y+8,
		pos.x+size.x-16,
		pos.y+size.y-16
	};
	GUI.DrawText(GUI.fBig, txStatsPanel, DT_TOP|DT_CENTER, BLACK, rect);

	flow.Draw();

	DrawBox();
}

//=================================================================================================
void StatsPanel::Event(GuiEvent e)
{
	GamePanel::Event(e);

	if(e == GuiEvent_Moved)
	{
		flow.global_pos = global_pos + flow.pos;
		scrollbar.global_pos = global_pos + scrollbar.pos;
	}
	else if(e == GuiEvent_Resize)
	{
		flow.size = size - INT2(44,48);
		scrollbar.pos = INT2(size.x-28, 48);
		scrollbar.global_pos = global_pos + scrollbar.pos;
		scrollbar.size = INT2(16, size.y-60);
		if(scrollbar.offset+scrollbar.part > scrollbar.total)
			scrollbar.offset = float(scrollbar.total-scrollbar.part);
		if(scrollbar.offset < 0)
			scrollbar.offset = 0;
		flow.moved = int(scrollbar.offset);
	}
	else if(e == GuiEvent_Show)
		SetText();
	else if(e == GuiEvent_LostFocus)
		scrollbar.LostFocus();
}

//=================================================================================================
void StatsPanel::Update(float dt)
{
	GamePanel::Update(dt);
	if(mouse_focus && Key.Focus() && IsInside(GUI.cursor_pos))
		scrollbar.ApplyMouseWheel();
	if(focus)
		scrollbar.Update(dt);
	flow.moved = int(scrollbar.offset);

	int new_index = INDEX_INVALID;

	if(mouse_focus)
		GUI.Intersect(flow.hitboxes, GUI.cursor_pos, &new_index);

	UpdateBoxIndex(dt, new_index);
}

//=================================================================================================
void StatsPanel::SetText()
{
	{
		string& s = tAttribs->text;
		s.clear();
		for(int i=0; i<A_MAX; ++i)
		{
			s += Format("$h+%s: ", g_attribute_info[i].name);
			int mod = pc->unit->GetAttributeState(i);
			if(mod != 0)
			{
				if(mod == 1)
					s += "$cg";
				else if(mod == 2)
					s += "$cr";
				else
					s += "$cy";
			}
			s += Format("%d", pc->unit->GetAttribute(i));
			if(i == A_DEX)
			{
				int dex = (int)pc->unit->CalculateDexterity();
				if(dex != pc->unit->GetAttribute(A_DEX))
					s += Format(" (%d)", dex);
			}
			if(mod != 0)
				s += "$c-";
			s += "$h-\n";
		}
	}

	{
		string& s = tSkills->text;
		s.clear();
		for(int i=0; i<S_MAX; ++i)
		{
			s += Format("$h+%s: ", skill_infos[i].name);
			int mod = pc->unit->GetSkillState(i);
			if(mod != 0)
			{
				if(mod == 1)
					s += "$cg";
				else if(mod == 2)
					s += "$cr";
				else
					s += "$cy";
			}
			s += Format("%d", pc->unit->GetSkill(i));
			if(mod != 0)
				s += "$c-";
			s += "$h-\n";
		}
	}

	int hp = int(pc->unit->hp);
	if(hp == 0 && pc->unit->hp > 0)
		hp = 1;
	cstring meleeAttack = (pc->unit->HaveWeapon() ? Format("%d", (int)pc->unit->CalculateAttack(&pc->unit->GetWeapon())) : "-");
	cstring rangedAttack = (pc->unit->HaveBow() ? Format("%d", (int)pc->unit->CalculateAttack(&pc->unit->GetBow())) : "-");
	tTraits->text = Format(txTraitsText, g_classes[pc->clas].name, hp, int(pc->unit->hpmax), meleeAttack, rangedAttack,
		(int)pc->unit->CalculateDefense(), float(pc->unit->weight)/10, float(pc->unit->weight_max)/10, pc->unit->gold);

	Game& game = Game::Get();
	tStats->text = Format(txStatsText, game.year, game.month+1, game.day+1, game.gt_hour, game.gt_minute, game.gt_second, pc->kills, pc->knocks, pc->dmg_done, pc->dmg_taken, pc->arena_fights);

	flow.Calculate();

	scrollbar.total = flow.total_size;
	scrollbar.part = flow.size.y;
	if(scrollbar.offset+scrollbar.part > scrollbar.total)
		scrollbar.offset = float(scrollbar.total-scrollbar.part);
	if(scrollbar.offset < 0)
		scrollbar.offset = 0;
	flow.moved = int(scrollbar.offset);
}

//=================================================================================================
void StatsPanel::FormatBox()
{
	box_img = NULL;

	if(last_index == INDEX_DATA)
	{
		Game& game = Game::Get();
		box_text = Format(txYearMonthDay, game.year, game.month+1, game.day+1);
		box_text_small.clear();
	}
	else if(last_index < INDEX_SKILL)
	{
		// atrybut
		const AttributeInfo& info = g_attribute_info[last_index];
		box_text = Format("%s: %d\n%s: %d", info.name, pc->unit->GetAttribute(last_index), txBase, pc->unit->GetBaseAttribute(last_index));
		box_text_small = info.desc;
	}
	else
	{
		// skill
		int skill = last_index-INDEX_SKILL;
		const SkillInfo& info = skill_infos[skill];
		box_text = Format("%s: %d\n%s: %d", info.name, pc->unit->GetSkill(skill), txBase, pc->unit->GetBaseSkill(skill));
		box_text_small = info.desc;
	}
}
