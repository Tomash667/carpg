#include "Pch.h"
#include "Base.h"
#include "StatsPanel.h"
#include "Unit.h"
#include "PlayerController.h"
#include "KeyStates.h"
#include "Game.h"
#include "Language.h"

//-----------------------------------------------------------------------------
enum Group
{
	G_ATTRIB,
	G_STATS,
	G_SKILL,
	G_PERK,
	G_INVALID = -1
};

//=================================================================================================
StatsPanel::StatsPanel() : last_update(0.f)
{
	visible = false;

	txAttributes = Str("attributes");
	txStatsPanel = Str("statsPanel");
	txTraitsText = Str("traitsText");
	txStatsText = Str("statsText");
	txYearMonthDay = Str("yearMonthDay");
	txBase = Str("base");
	txRelatedAttributes = Str("relatedAttributes");
	txFeats = Str("feats");
	txTraits = Str("traits");
	txStats = Str("stats");
	txStatsDate = Str("statsDate");

	tooltip.Init(TooltipGetText(this, &StatsPanel::GetTooltip));
}

//=================================================================================================
void StatsPanel::Draw(ControlDrawData*)
{
	GamePanel::Draw();

	RECT rect = {
		pos.x+8,
		pos.y+8,
		pos.x+size.x-16,
		pos.y+size.y-16
	};
	GUI.DrawText(GUI.fBig, txStatsPanel, DT_TOP|DT_CENTER, BLACK, rect);

	flowAttribs.Draw();
	flowStats.Draw();
	flowSkills.Draw();
	flowFeats.Draw();

	tooltip.Draw();
}

//=================================================================================================
void StatsPanel::Event(GuiEvent e)
{
	GamePanel::Event(e);

	if(e == GuiEvent_Moved || e == GuiEvent_Resize)
	{
		int sizex = (size.x - 48) / 3;
		int sizey = size.y - 64;

		flowAttribs.UpdateSize(global_pos + INT2(16, 48), INT2(sizex, 200), visible);
		flowStats.UpdateSize(global_pos + INT2(16, 200 + 48 + 8), INT2(sizex, sizey - 200 - 8), visible);
		flowSkills.UpdateSize(global_pos + INT2(16 + sizex + 8, 48), INT2(sizex, sizey), visible);
		flowFeats.UpdateSize(global_pos + INT2(16 + (sizex + 8) * 2, 48), INT2(sizex, sizey), visible);
	}
	else if(e == GuiEvent_Show)
		SetText();
}

//=================================================================================================
void StatsPanel::Update(float dt)
{
	GamePanel::Update(dt);

	last_update -= dt;
	if(last_update <= 0.f)
		SetText();

	int group = -1, id = -1;

	flowAttribs.mouse_focus = focus;
	flowAttribs.Update(dt);
	flowAttribs.GetSelected(group, id);

	flowStats.mouse_focus = focus;
	flowStats.Update(dt);
	flowStats.GetSelected(group, id);

	flowSkills.mouse_focus = focus;
	flowSkills.Update(dt);
	flowSkills.GetSelected(group, id);

	flowFeats.mouse_focus = focus;
	flowFeats.Update(dt);
	flowFeats.GetSelected(group, id);

	tooltip.Update(dt, group, id);

	if(focus && Key.Focus() && Key.PressedRelease(VK_ESCAPE))
		Hide();
}

//=================================================================================================
void StatsPanel::SetText()
{
	Game& game = Game::Get();

	// attributes
	flowAttribs.Clear();
	flowAttribs.Add()->Set(txAttributes);
	for(int i = 0; i < (int)Attribute::MAX; ++i)
		flowAttribs.Add()->Set(Format("%s: $c%c%d$c-", g_attributes[i].name.c_str(), StatStateToColor(pc->attrib_state[i]), pc->unit->Get((Attribute)i)), G_ATTRIB, i);
	flowAttribs.Reposition();

	// stats
	flowStats.Clear();
	flowStats.Add()->Set(txTraits);
	int hp = int(pc->unit->hp);
	if(hp == 0 && pc->unit->hp > 0)
		hp = 1;
	cstring meleeAttack = (pc->unit->HaveWeapon() ? Format("%d", (int)pc->unit->CalculateAttack(&pc->unit->GetWeapon())) : "-");
	cstring rangedAttack = (pc->unit->HaveBow() ? Format("%d", (int)pc->unit->CalculateAttack(&pc->unit->GetBow())) : "-");
	flowStats.Add()->Set(Format(txTraitsText, g_classes[(int)pc->clas].name.c_str(), hp, int(pc->unit->hpmax), meleeAttack, rangedAttack,
		(int)pc->unit->CalculateDefense(), float(pc->unit->weight) / 10, float(pc->unit->weight_max) / 10, pc->unit->gold), G_INVALID, -1);
	flowStats.Add()->Set(txStats);
	flowStats.Add()->Set(Format(txStatsDate, game.year, game.month + 1, game.day + 1), G_STATS, 0);
	flowStats.Add()->Set(Format(txStatsText, game.gt_hour, game.gt_minute, game.gt_second, pc->kills, pc->knocks, pc->dmg_done, pc->dmg_taken, pc->arena_fights), G_INVALID, -1);
	flowStats.Reposition();

	// skills
	SkillGroup last_group = SkillGroup::NONE;
	flowSkills.Clear();
	for(int i = 0; i < (int)Skill::MAX; ++i)
	{
		if(pc->unit->Get((Skill)i) > 0)
		{
			SkillInfo& info = g_skills[i];
			if(info.group != last_group)
			{
				flowSkills.Add()->Set(g_skill_groups[(int)info.group].name.c_str());
				last_group = info.group;
			}
			flowSkills.Add()->Set(Format("%s: $c%c%d$c-", info.name.c_str(), StatStateToColor(pc->skill_state[i]), pc->unit->Get((Skill)i)), G_SKILL, i);
		}
	}
	flowSkills.Reposition();

	// feats
	flowFeats.Clear();
	flowFeats.Add()->Set(txFeats);
	flowFeats.Reposition();

	last_update = 0.5f;
}

//=================================================================================================
void StatsPanel::GetTooltip(TooltipController*, int group, int id)
{
	tooltip.anything = true;
	tooltip.img = NULL;
	
	switch(group)
	{
	case G_ATTRIB:
		{
			AttributeInfo& ai = g_attributes[id];
			Attribute a = (Attribute)id;
			tooltip.big_text = Format("%s: %d", ai.name.c_str(), pc->unit->Get(a));
			tooltip.text = Format("%s: %d/%d\n%s", txBase, pc->unit->GetUnmod(a), pc->GetBase(a), ai.desc.c_str());
			tooltip.small_text.clear();

		}
		break;
	case G_STATS:
		// date
		{
			Game& game = Game::Get();
			tooltip.big_text.clear();
			tooltip.text = Format(txYearMonthDay, game.year, game.month + 1, game.day + 1);
			tooltip.small_text.clear();
		}
		break;
	case G_SKILL:
		{
			SkillInfo& si = g_skills[id];
			Skill s = (Skill)id;
			tooltip.big_text = Format("%s: %d", si.name.c_str(), pc->unit->Get(s));
			tooltip.text = Format("%s: %d/%d\n%s", txBase, pc->unit->GetUnmod(s), pc->GetBase(s), si.desc.c_str());
			if(si.attrib2 != Attribute::NONE)
				tooltip.small_text = Format("%s: %s, %s", txRelatedAttributes, g_attributes[(int)si.attrib].name.c_str(), g_attributes[(int)si.attrib2].name.c_str());
			else
				tooltip.small_text = Format("%s: %s", txRelatedAttributes, g_attributes[(int)si.attrib].name.c_str());
		}
		break;
	case G_PERK:
	case G_INVALID:
	default:
		tooltip.anything = false;
		break;
	}
}

//=================================================================================================
void StatsPanel::Show()
{
	visible = true;
	Event(GuiEvent_Show);
	GainFocus();
}

//=================================================================================================
void StatsPanel::Hide()
{
	LostFocus();
	visible = false;
}
