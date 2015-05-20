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
#define INDEX_SKILL 1
#define INDEX_DATA 2

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

	int new_index = INDEX_INVALID, new_index2 = INDEX_INVALID;

	if(mouse_focus)
		GUI.Intersect(flow.hitboxes, GUI.cursor_pos, &new_index, &new_index2);

	UpdateBoxIndex(dt, new_index, new_index2);
}

//=================================================================================================
void StatsPanel::SetText()
{
	{
		string& s = tAttribs->text;
		s.clear();
		for(int i=0; i<(int)Attribute::MAX; ++i)
		{
			s += Format("$g+0,%d;%s: ", i, g_attributes[i].name.c_str());
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
			if(i == (int)Attribute::DEX)
			{
				int dex = (int)pc->unit->CalculateDexterity();
				if(dex != pc->unit->GetAttribute((int)Attribute::DEX))
					s += Format(" (%d)", dex);
			}
			if(mod != 0)
				s += "$c-";
			s += "$g-\n";
		}
	}

	{
		string& s = tSkills->text;
		s.clear();
		for(int i=0; i<(int)Skill::MAX; ++i)
		{
			s += Format("$g+1,%d;%s: ", i, g_skills[i].name.c_str());
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
			s += "$g-\n";
		}
	}

	int hp = int(pc->unit->hp);
	if(hp == 0 && pc->unit->hp > 0)
		hp = 1;
	cstring meleeAttack = (pc->unit->HaveWeapon() ? Format("%d", (int)pc->unit->CalculateAttack(&pc->unit->GetWeapon())) : "-");
	cstring rangedAttack = (pc->unit->HaveBow() ? Format("%d", (int)pc->unit->CalculateAttack(&pc->unit->GetBow())) : "-");
	tTraits->text = Format(txTraitsText, g_classes[(int)pc->clas].name.c_str(), hp, int(pc->unit->hpmax), meleeAttack, rangedAttack,
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
	else if(last_index == INDEX_ATTRIB)
	{
		// atrybut
		const AttributeInfo& info = g_attributes[last_index2];
		box_text = Format("%s: %d\n%s: %d", info.name.c_str(), pc->unit->GetAttribute(last_index2), txBase, pc->unit->GetBaseAttribute(last_index2));
		box_text_small = info.desc;
	}
	else if(last_index == INDEX_SKILL)
	{
		// skill
		const SkillInfo& info = g_skills[last_index2];
		box_text = Format("%s: %d\n%s: %d", info.name.c_str(), pc->unit->GetSkill(last_index2), txBase, pc->unit->GetBaseSkill(last_index2));
		box_text_small = info.desc;
	}
}
