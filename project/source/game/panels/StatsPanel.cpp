#include "Pch.h"
#include "Core.h"
#include "StatsPanel.h"
#include "Unit.h"
#include "PlayerController.h"
#include "KeyStates.h"
#include "Game.h"
#include "GameStats.h"
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

	txAttributes = Str("attributes");
	txStatsPanel = Str("statsPanel");
	txClass = Str("class");
	txStatsText = Str("statsText");
	txYearMonthDay = Str("yearMonthDay");
	txBase = Str("base");
	txRelatedAttributes = Str("relatedAttributes");
	txFeats = Str("feats");
	txTraits = Str("traits");
	txStats = Str("stats");
	txStatsDate = Str("statsDate");
	txHealth = Str("health");
	txStamina = Str("stamina");
	txAttack = Str("attack");
	txDefense = Str("defense");
	txMeleeAttack = Str("meleeAttack");
	txRangedAttack = Str("rangedAttack");
	txMobility = Str("mobility");
	txCarryShort = Str("carryShort");
	txGold = Str("gold");
	txBlock = Str("block");

	tooltip.Init(TooltipGetText(this, &StatsPanel::GetTooltip));
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
	GUI.DrawText(GUI.fBig, txStatsPanel, DT_TOP | DT_CENTER, BLACK, rect);

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
	{
		StatState state;
		int value = pc->unit->Get((Attribute)i, state);
		flowAttribs.Add()->Set(Format("%s: $c%c%d$c-", AttributeInfo::attributes[i].name.c_str(), StatStateToColor(state), value), G_ATTRIB, i);
	}
	flowAttribs.Reposition();

	// stats
	flowStats.Clear();
	flowStats.Add()->Set(txTraits);
	flowStats.Add()->Set(Format("%s: %s", txClass, ClassInfo::classes[(int)pc->clas].name.c_str()), G_STATS, STATS_CLASS);
	if(game.devmode)
		flowStats.Add()->Set(Format("Level: %g (%d)", pc->level, pc->unit->level), G_INVALID, -1);
	int hp = int(pc->unit->hp);
	if(hp == 0 && pc->unit->hp > 0)
		hp = 1;
	flowStats.Add()->Set(Format("%s: %d/%d", txHealth, hp, int(pc->unit->hpmax)), G_INVALID, -1);
	flowStats.Add()->Set(Format("%s: %d/%d", txStamina, int(pc->unit->stamina), int(pc->unit->stamina_max)), G_INVALID, -1);
	cstring meleeAttack = (pc->unit->HaveWeapon() ? Format("%d", (int)pc->unit->CalculateAttack(&pc->unit->GetWeapon())) : "--");
	cstring rangedAttack = (pc->unit->HaveBow() ? Format("%d", (int)pc->unit->CalculateAttack(&pc->unit->GetBow())) : "--");
	flowStats.Add()->Set(Format("%s: %s/%s", txAttack, meleeAttack, rangedAttack), G_STATS, STATS_ATTACK);
	flowStats.Add()->Set(Format("%s: %d", txDefense, (int)pc->unit->CalculateDefense()), G_INVALID, -1);
	cstring block = (pc->unit->HaveShield() ? Format("%d", (int)pc->unit->CalculateBlock()) : "--");
	flowStats.Add()->Set(Format("%s: %s", txBlock, block), G_INVALID, -1);
	flowStats.Add()->Set(Format("%s: %d", txMobility, (int)pc->unit->CalculateMobility()), G_INVALID, -1);
	flowStats.Add()->Set(Format(txCarryShort, float(pc->unit->weight) / 10, float(pc->unit->weight_max) / 10), G_INVALID, -1);
	flowStats.Add()->Set(Format(txGold, pc->unit->gold), G_INVALID, -1);
	flowStats.Add()->Set(txStats);
	flowStats.Add()->Set(Format(txStatsDate, game.year, game.month + 1, game.day + 1), G_STATS, STATS_DATE);
	GameStats& game_stats = GameStats::Get();
	flowStats.Add()->Set(Format(txStatsText, game_stats.hour, game_stats.minute, game_stats.second, pc->kills, pc->knocks, pc->dmg_done, pc->dmg_taken, pc->arena_fights),
		G_INVALID, -1);
	flowStats.Reposition();

	// skills
	SkillGroup last_group = SkillGroup::NONE;
	flowSkills.Clear();
	for(int i = 0; i < (int)Skill::MAX; ++i)
	{
		Skill skill = (Skill)i;
		if(pc->unit->GetBase(skill) > 0)
		{
			SkillInfo& info = SkillInfo::skills[i];
			if(info.group != last_group)
			{
				flowSkills.Add()->Set(SkillGroupInfo::groups[(int)info.group].name.c_str());
				last_group = info.group;
			}
			StatState state;
			int value = pc->unit->Get(skill, state);
			flowSkills.Add()->Set(Format("%s: $c%c%d$c-", info.name.c_str(), StatStateToColor(state), value), G_SKILL, i);
		}
	}
	flowSkills.Reposition();

	// feats
	perks.clear();
	LocalVector2<string*> strs;
	for(int i = 0; i < (int)pc->unit->statsx->perks.size(); ++i)
	{
		auto& taken_perk = pc->unit->statsx->perks[i];
		if(taken_perk.hidden)
			continue;
		PerkInfo& perk = PerkInfo::perks[(int)taken_perk.perk];
		if(IS_SET(perk.flags, PerkInfo::RequireFormat))
		{
			string* s = StringPool.Get();
			*s = taken_perk.FormatName();
			strs.push_back(s);
			perks.push_back(std::pair<cstring, int>(s->c_str(), i));
		}
		else
			perks.push_back(std::pair<cstring, int>(perk.name.c_str(), i));
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
void StatsPanel::GetTooltip(TooltipController*, int group, int id)
{
	tooltip.anything = true;
	tooltip.img = nullptr;

	switch(group)
	{
	case G_ATTRIB:
		{
			AttributeInfo& ai = AttributeInfo::attributes[id];
			Attribute a = (Attribute)id;
			tooltip.big_text = Format("%s: %d", ai.name.c_str(), pc->unit->Get(a));
			if(!Game::Get().devmode || Net::IsClient())
				tooltip.text = Format("%s: %d\n%s", txBase, pc->unit->GetBase(a), ai.desc.c_str());
			else
			{
				tooltip.text = Format("%s: %d (%d)\n%s\n\nAptitude: %+d\nTrain: %d/%d (%g%%)", txBase, pc->unit->GetBase(a), pc->unit->statsx->attrib_base[id],
					ai.desc.c_str(), pc->unit->GetAptitude(a), pc->attrib_pts[id].value, pc->attrib_pts[id].next, pc->attrib_pts[id].Percentage());
			}
			tooltip.small_text.clear();
		}
		break;
	case G_STATS:
		switch(id)
		{
		case STATS_DATE:
			{
				Game& game = Game::Get();
				tooltip.big_text.clear();
				tooltip.text = Format(txYearMonthDay, game.year, game.month + 1, game.day + 1);
				tooltip.small_text.clear();
			}
			break;
		case STATS_CLASS:
			{
				ClassInfo& info = ClassInfo::classes[(int)pc->clas];
				tooltip.big_text = info.name;
				tooltip.text = info.desc;
				tooltip.small_text.clear();
			}
			break;
		case STATS_ATTACK:
			{
				tooltip.big_text.clear();
				tooltip.small_text.clear();
				cstring meleeAttack, rangedAttack;
				if(Game::Get().devmode)
				{
					meleeAttack = (pc->unit->HaveWeapon() ? Format("%d (crit %d%% x%g, power x%g)",
						(int)pc->unit->CalculateAttack(&pc->unit->GetWeapon()),
						pc->unit->GetCriticalChance(&pc->unit->GetWeapon(), false, 0.f),
						FLT100(pc->unit->GetCriticalDamage(&pc->unit->GetWeapon())),
						FLT100(pc->unit->GetPowerAttackMod())
					) : "--");
					rangedAttack = (pc->unit->HaveBow() ? Format("%d (crit %d%% x%g)",
						(int)pc->unit->CalculateAttack(&pc->unit->GetBow()),
						pc->unit->GetCriticalChance(&pc->unit->GetBow(), false, 0.f),
						FLT100(pc->unit->GetCriticalDamage(&pc->unit->GetBow()))
					) : "--");
				}
				else
				{
					meleeAttack = (pc->unit->HaveWeapon() ? Format("%d", (int)pc->unit->CalculateAttack(&pc->unit->GetWeapon())) : "--");
					rangedAttack = (pc->unit->HaveBow() ? Format("%d", (int)pc->unit->CalculateAttack(&pc->unit->GetBow())) : "--");
				}
				tooltip.text = Format("%s: %s\n%s: %s", txMeleeAttack, meleeAttack, txRangedAttack, rangedAttack);
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
			SkillInfo& si = SkillInfo::skills[id];
			Skill s = (Skill)id;
			tooltip.big_text = Format("%s: %d", si.name.c_str(), pc->unit->Get(s));
			if(!Game::Get().devmode || Net::IsClient())
				tooltip.text = Format("%s: %d\n%s", txBase, pc->unit->GetBase(s), si.desc.c_str());
			else
			{
				tooltip.text = Format("%s: %d (%d)\n%s\n\nAptitude: %+d\nTrain: %d/%d (%g%%)", txBase, pc->unit->GetBase(s), pc->unit->statsx->skill_base[id],
					si.desc.c_str(), pc->unit->GetAptitude(s), pc->skill_pts[id].value, pc->skill_pts[id].next, pc->skill_pts[id].Percentage());
			}
			if(si.attrib2 != Attribute::NONE)
			{
				tooltip.small_text = Format("%s: %s, %s", txRelatedAttributes, AttributeInfo::attributes[(int)si.attrib].name.c_str(),
					AttributeInfo::attributes[(int)si.attrib2].name.c_str());
			}
			else
				tooltip.small_text = Format("%s: %s", txRelatedAttributes, AttributeInfo::attributes[(int)si.attrib].name.c_str());
		}
		break;
	case G_PERK:
		{
			TakenPerk& perk = pc->unit->statsx->perks[id];
			PerkInfo& pi = PerkInfo::perks[(int)perk.perk];
			tooltip.img = nullptr;
			tooltip.big_text = pi.name;
			tooltip.text = pi.desc;
			perk.GetDesc(tooltip.small_text);
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
