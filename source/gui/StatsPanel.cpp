#include "Pch.h"
#include "StatsPanel.h"

#include "Game.h"
#include "GameGui.h"
#include "GameStats.h"
#include "Language.h"
#include "PlayerController.h"
#include "Unit.h"
#include "World.h"

#include <Input.h>

//-----------------------------------------------------------------------------
enum Group
{
	G_ATTRIB,
	G_STATS,
	G_SKILL,
	G_PERK,
	G_INVALID = -1
};

//-----------------------------------------------------------------------------
enum Stats
{
	STATS_DATE,
	STATS_CLASS,
	STATS_ATTACK
};

//=================================================================================================
StatsPanel::StatsPanel() : last_update(0.f)
{
	visible = false;
	tooltip.Init(TooltipController::Callback(this, &StatsPanel::GetTooltip));
}

//=================================================================================================
void StatsPanel::LoadLanguage()
{
	auto section = Language::GetSection("StatsPanel");
	txAttributes = section.Get("attributes");
	txTitle = section.Get("title");
	txClass = section.Get("class");
	txTraitsStart = section.Get("traitsStart");
	txTraitsStartMp = section.Get("traitsStartMp");
	txTraitsEnd = section.Get("traitsEnd");
	txStatsText = section.Get("statsText");
	txYearMonthDay = section.Get("yearMonthDay");
	txBase = section.Get("base");
	txRelatedAttributes = section.Get("relatedAttributes");
	txFeats = section.Get("feats");
	txTraits = section.Get("traits");
	txStats = section.Get("stats");
	txDate = section.Get("date");
	txAttack = section.Get("attack");
	txMeleeAttack = section.Get("meleeAttack");
	txRangedAttack = section.Get("rangedAttack");
	txHardcoreMode = section.Get("hardcoreMode");
}

//=================================================================================================
void StatsPanel::Draw(ControlDrawData*)
{
	GamePanel::Draw();

	Rect rect = {
		pos.x + 8,
		pos.y + 8,
		pos.x + size.x - 16,
		pos.y + size.y - 16
	};
	gui->DrawText(GameGui::font_big, txTitle, DTF_TOP | DTF_CENTER, Color::Black, rect);

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

		flowAttribs.UpdateSize(global_pos + Int2(16, 48), Int2(sizex, 200), visible);
		flowStats.UpdateSize(global_pos + Int2(16, 200 + 48 + 8), Int2(sizex, sizey - 200 - 8), visible);
		flowSkills.UpdateSize(global_pos + Int2(16 + sizex + 8, 48), Int2(sizex, sizey), visible);
		flowFeats.UpdateSize(global_pos + Int2(16 + (sizex + 8) * 2, 48), Int2(sizex, sizey), visible);
	}
	else if(e == GuiEvent_Show)
	{
		SetText();
		tooltip.Clear();
	}
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

	tooltip.UpdateTooltip(dt, group, id);

	if(focus && input->Focus() && input->PressedRelease(Key::Escape))
		Hide();
}

//=================================================================================================
void StatsPanel::SetText()
{
	// attributes
	flowAttribs.Clear();
	flowAttribs.Add()->Set(txAttributes);
	for(int i = 0; i < (int)AttributeId::MAX; ++i)
	{
		StatState state;
		int value = pc->unit->Get((AttributeId)i, &state);
		flowAttribs.Add()->Set(Format("%s: $c%c%d$c-", Attribute::attributes[i].name.c_str(), StatStateToColor(state), value), G_ATTRIB, i);
	}
	flowAttribs.Reposition();

	// stats
	flowStats.Clear();
	flowStats.Add()->Set(txTraits);
	Class* clas = pc->unit->GetClass();
	int hp = int(pc->unit->hp);
	if(hp == 0 && pc->unit->hp > 0)
		hp = 1;
	flowStats.Add()->Set(Format("%s: %s", txClass, clas->name.c_str()), G_STATS, STATS_CLASS);
	if(IsSet(clas->flags, Class::F_MP_BAR))
	{
		flowStats.Add()->Set(Format(txTraitsStartMp, hp, int(pc->unit->hpmax), int(pc->unit->mp), int(pc->unit->mpmax),
			int(pc->unit->stamina), int(pc->unit->stamina_max)), G_INVALID, -1);
	}
	else
		flowStats.Add()->Set(Format(txTraitsStart, hp, int(pc->unit->hpmax), int(pc->unit->stamina), int(pc->unit->stamina_max)), G_INVALID, -1);
	cstring meleeAttack = (pc->unit->HaveWeapon() ? Format("%d", (int)pc->unit->CalculateAttack(&pc->unit->GetWeapon())) : "-");
	cstring rangedAttack = (pc->unit->HaveBow() ? Format("%d", (int)pc->unit->CalculateAttack(&pc->unit->GetBow())) : "-");
	flowStats.Add()->Set(Format("%s: %s/%s", txAttack, meleeAttack, rangedAttack), G_STATS, STATS_ATTACK);
	cstring blockDesc = (pc->unit->HaveShield() ? Format("%d", (int)pc->unit->CalculateBlock()) : "-");
	flowStats.Add()->Set(Format(txTraitsEnd, (int)pc->unit->CalculateDefense(), blockDesc, (int)pc->unit->CalculateMobility(), pc->learning_points,
		float(pc->unit->weight) / 10, float(pc->unit->weight_max) / 10, pc->unit->gold), G_INVALID, -1);
	flowStats.Add()->Set(txStats);
	if(game->hardcore_mode)
		flowStats.Add()->Set(Format("$cr%s$c-", txHardcoreMode), G_INVALID, -1);
	flowStats.Add()->Set(Format(txDate, world->GetDate()), G_STATS, STATS_DATE);
	flowStats.Add()->Set(Format(txStatsText, game_stats->hour, game_stats->minute, game_stats->second, pc->kills, pc->knocks, pc->dmg_done, pc->dmg_taken,
		pc->arena_fights), G_INVALID, -1);
	flowStats.Reposition();

	// skills
	SkillGroupId last_group = SkillGroupId::NONE;
	flowSkills.Clear();
	for(int i = 0; i < (int)SkillId::MAX; ++i)
	{
		int value = pc->unit->Get((SkillId)i, nullptr, false);
		if(value > 0)
		{
			Skill& info = Skill::skills[i];
			if(info.group != last_group)
			{
				flowSkills.Add()->Set(SkillGroup::groups[(int)info.group].name.c_str());
				last_group = info.group;
			}
			StatState state;
			int value = pc->unit->Get((SkillId)i, &state);
			flowSkills.Add()->Set(Format("%s: $c%c%d$c-", info.name.c_str(), StatStateToColor(state), value), G_SKILL, i);
		}
	}
	flowSkills.Reposition();

	// feats
	perks.clear();
	LocalVector<string*> strs;
	for(int i = 0; i < (int)pc->perks.size(); ++i)
	{
		Perk* perk = pc->perks[i].perk;
		if(perk->value_type != Perk::None)
		{
			string* s = StringPool.Get();
			*s = pc->perks[i].FormatName();
			strs.push_back(s);
			perks.push_back(pair<cstring, int>(s->c_str(), i));
		}
		else
			perks.push_back(pair<cstring, int>(perk->name.c_str(), i));
	}
	std::sort(perks.begin(), perks.end(), SortTakenPerks);
	flowFeats.Clear();
	flowFeats.Add()->Set(txFeats);
	for(auto& perk : perks)
		flowFeats.Add()->Set(perk.first, G_PERK, perk.second);
	flowFeats.Reposition();
	StringPool.Free(strs.Get());

	last_update = 0.5f;
}

//=================================================================================================
void StatsPanel::GetTooltip(TooltipController*, int group, int id, bool refresh)
{
	tooltip.anything = true;
	tooltip.img = nullptr;

	switch(group)
	{
	case G_ATTRIB:
		{
			Attribute& ai = Attribute::attributes[id];
			AttributeId a = (AttributeId)id;
			tooltip.big_text = Format("%s: %d", ai.name.c_str(), pc->unit->Get(a));
			if(game->devmode && Net::IsLocal())
			{
				PlayerController::StatData& stat = pc->attrib[id];
				tooltip.text = Format("%s: %d\n%s\n\nExp: %d/%d (%g%%)\nTrain: %d\nAptitude: %d", txBase, pc->unit->GetBase(a),
					ai.desc.c_str(), stat.points, stat.next, float(stat.points) * 100 / stat.next, stat.train, stat.apt);
			}
			else
				tooltip.text = Format("%s: %d\n%s", txBase, pc->unit->GetBase(a), ai.desc.c_str());
			tooltip.small_text.clear();
		}
		break;
	case G_STATS:
		switch(id)
		{
		case STATS_DATE:
			{
				tooltip.big_text.clear();
				tooltip.text = Format(txYearMonthDay, world->GetYear(), world->GetMonth() + 1, world->GetDay() + 1);
				tooltip.small_text.clear();
			}
			break;
		case STATS_CLASS:
			{
				Class* clas = pc->unit->GetClass();
				tooltip.big_text = clas->name;
				tooltip.text = clas->desc;
				if(game->devmode)
				{
					if(Net::IsLocal())
						tooltip.small_text = Format("Level: %d\nTrain level: %d\nExp: %d/%d", pc->unit->level, pc->exp_level, pc->exp, pc->exp_need);
					else
						tooltip.small_text = Format("Level: %d", pc->unit->level);
				}
				else
					tooltip.small_text.clear();
			}
			break;
		case STATS_ATTACK:
			{
				cstring meleeAttack = (pc->unit->HaveWeapon() ? Format("%d", (int)pc->unit->CalculateAttack(&pc->unit->GetWeapon())) : "--");
				cstring rangedAttack = (pc->unit->HaveBow() ? Format("%d", (int)pc->unit->CalculateAttack(&pc->unit->GetBow())) : "--");
				tooltip.big_text.clear();
				tooltip.text = Format("%s: %s\n%s: %s", txMeleeAttack, meleeAttack, txRangedAttack, rangedAttack);
				tooltip.small_text.clear();
			}
			break;
		default:
			assert(0);
			tooltip.anything = false;
			break;
		}
		break;
	case G_SKILL:
		{
			Skill& si = Skill::skills[id];
			SkillId s = (SkillId)id;
			tooltip.big_text = Format("%s: %d", si.name.c_str(), pc->unit->Get(s));
			if(game->devmode && Net::IsLocal())
			{
				PlayerController::StatData& stat = pc->skill[id];
				tooltip.text = Format("%s: %d\n%s\n\nExp: %d/%d (%g%%)\nTrain: %d\nAptitude: %d", txBase, pc->unit->GetBase(s),
					si.desc.c_str(), stat.points, stat.next, float(stat.points) * 100 / stat.next, stat.train, pc->GetAptitude(s));
			}
			else
				tooltip.text = Format("%s: %d\n%s", txBase, pc->unit->GetBase(s), si.desc.c_str());
			if(si.attrib2 != AttributeId::NONE)
				tooltip.small_text = Format("%s: %s, %s", txRelatedAttributes, Attribute::attributes[(int)si.attrib].name.c_str(), Attribute::attributes[(int)si.attrib2].name.c_str());
			else
				tooltip.small_text = Format("%s: %s", txRelatedAttributes, Attribute::attributes[(int)si.attrib].name.c_str());
		}
		break;
	case G_PERK:
		{
			TakenPerk& tp = pc->perks[id];
			tooltip.img = nullptr;
			tooltip.big_text = tp.perk->name;
			tooltip.text = tp.perk->desc;
			tp.GetDetails(tooltip.small_text);
		}
		break;
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

//=================================================================================================
char StatsPanel::StatStateToColor(StatState s)
{
	switch(s)
	{
	default:
	case StatState::NORMAL:
		return 'k';
	case StatState::POSITIVE:
		return 'g';
	case StatState::POSITIVE_MIXED:
		return '0';
	case StatState::MIXED:
		return 'y';
	case StatState::NEGATIVE_MIXED:
		return '1';
	case StatState::NEGATIVE:
		return 'r';
	}
}
