#include "Pch.h"
#include "Base.h"
#include "PlayerController.h"
#include "Unit.h"
#include "Game.h"
#include "SaveState.h"

//=================================================================================================
PlayerController::~PlayerController()
{
	if(dialog_ctx && !dialog_ctx->is_local)
		delete dialog_ctx;
}

//=================================================================================================
float PlayerController::CalculateAttack() const
{
	BRON b;

	switch(unit->stan_broni)
	{
	case BRON_CHOWA:
		b = unit->chowana;
		break;
	case BRON_SCHOWANA:
		if(ostatnia == B_BRAK)
		{
			if(unit->HaveWeapon())
				b = B_JEDNORECZNA;
			else if(unit->HaveBow())
				b = B_LUK;
			else
				b = B_BRAK;
		}
		else
			b = ostatnia;
		break;
	case BRON_WYJMUJE:
	case BRON_WYJETA:
		b = unit->wyjeta;
		break;
	default:
		assert(0);
		break;
	}

	if(b == B_JEDNORECZNA)
	{
		if(!unit->HaveWeapon())
		{
			if(!unit->HaveBow())
				b = B_BRAK;
			else
				b = B_LUK;
		}
	}
	else if(b == B_LUK)
	{
		if(!unit->HaveBow())
		{
			if(!unit->HaveWeapon())
				b = B_BRAK;
			else
				b = B_JEDNORECZNA;
		}
	}

	if(b == B_JEDNORECZNA)
		return unit->CalculateAttack(&unit->GetWeapon());
	else if(b == B_LUK)
		return unit->CalculateAttack(&unit->GetBow());
	else
		return 0.5f * unit->attrib[(int)Attribute::STR] + 0.5f * unit->CalculateDexterity();
}

//=================================================================================================
float PlayerController::CalculateLevel(int attributes, int skills, int flags)
{
	// atrybuty
	const float c_attrib_mod[] = {
		-4.f, //10-19
		-2.f, //20-29
		-1.f, //30-39
		0.f, // 40-49
		1.f, // 50-59
		2.f, // 60-69
		5.f, // 70-79
		10.f, //80-89
		15.f, //90-99
		20.f  //100
	};
	float attrib_level = 0.f;
	uint count = 0;
	for(int i=0; i<(int)Attribute::MAX; ++i)
	{
		if(IS_SET(attributes, BIT(i)))
		{
			int n = unit->attrib[i]/10-1;
			int k = unit->attrib[i] % 10;
			float v;
			if(k == 0)
				v = c_attrib_mod[n];
			else
				v = lerp(c_attrib_mod[n], c_attrib_mod[n+1], float(k)/9);
			if(IS_SET(flags, USE_WEAPON_ATTRIB_MOD))
			{
				if(i == (int)Attribute::STR || i == (int)Attribute::DEX)
				{
					const WeaponTypeInfo& info = weapon_type_info[unit->GetWeapon().weapon_type];
					if(i == (int)Attribute::STR)
						v *= info.str2dmg*2;
					else
						v *= info.dex2dmg*2;
					v = clamp(v, -4.f, 20.f);
				}
			}
			attrib_level += v;
			++count;
		}
	}
	attrib_level /= count;
	attrib_level = clamp(attrib_level, 0.f, 20.f);

	// umiejêtnoœci
	const float c_skill_mod[] = {
		0.f,  //  0-9
		1.f,  // 10-19
		2.f,  // 20-29
		4.f,  // 30-39
		8.f,  // 40-49
		10.f, // 50-59
		12.f, // 60-69
		14.f, // 70-79
		16.f, // 80-89
		18.f, // 90-99
		20.f  // 100
	};
	float skill_level = 0.f;
	count = 0;
	for(int i = 0; i<(int)Skill::MAX; ++i)
	{
		if(IS_SET(skills, BIT(i)))
		{
			int n = unit->skill[i]/10;
			int k = unit->skill[i]%10;
			float v;
			if(k == 0)
				v = c_skill_mod[n];
			else
				v = lerp(c_skill_mod[n], c_skill_mod[n+1], float(k)/9);
			skill_level += v;
			++count;
		}
	}
	skill_level /= count;
	skill_level = clamp(skill_level, 0.f, 20.f);

	// ekwipunek
	count = 0;
	float item_level = 0;
	if(IS_SET(flags, USE_WEAPON))
	{
		item_level += unit->GetWeapon().level;
		++count;
	}
	if(IS_SET(flags, USE_ARMOR))
	{
		item_level += unit->GetArmor().level;
		++count;
	}
	if(IS_SET(flags, USE_BOW))
	{
		item_level += unit->GetBow().level;
		++count;
	}
	if(IS_SET(flags, USE_SHIELD))
	{
		item_level += unit->GetShield().level;
		++count;
	}
	item_level /= count;
	item_level = clamp(item_level, 0.f, 20.f);

	return (attrib_level + skill_level + item_level)/3;
}

//=================================================================================================
void PlayerController::Train2(TrainWhat what, float value, float source_lvl, float precalclvl)
{
	const int b_str = BIT((int)Attribute::STR);
	const int b_con = BIT((int)Attribute::CON);
	const int b_dex = BIT((int)Attribute::DEX);

	switch(what)
	{
	case Train_Hit:
		{
			// gracz kogoœ uderzy³
			// szkol w sile, zrêcznoœci, walce broni¹
			float mylvl = CalculateLevel(b_str | b_dex, BIT((int)Skill::WEAPON), USE_WEAPON);
			float points = value * (source_lvl / mylvl);
			const WeaponTypeInfo& info = weapon_type_info[unit->GetWeapon().weapon_type];
			Train(Attribute::STR, int(points * info.str2dmg), T_OFENSE);
			Train(Attribute::DEX, int(points * info.dex2dmg), T_OFENSE);
			Train(Skill::WEAPON, (int)points*2, T_OFENSE);
		}
		break;
	case Train_Hurt:
		{
			// gracz zosta³ trafiony przez coœ
			Skill s;
			int f;
			if(unit->HaveArmor())
			{
				if(unit->GetArmor().IsHeavy())
					s = Skill::HEAVY_ARMOR;
				else
					s = Skill::LIGHT_ARMOR;
				f = USE_ARMOR;
			}
			else
			{
				s = Skill::NONE;
				f = 0;
			}
			float mylvl = CalculateLevel(b_str | b_con | b_dex, s != Skill::NONE ? BIT((int)s) : 0, f);
			float points = value * (source_lvl / mylvl);
			if(s == Skill::HEAVY_ARMOR)
			{
				Train(Attribute::STR, int(points / 5), T_DEFENSE);
				Train(Attribute::DEX, int(points / 10), T_DEFENSE);
			}
			else
			{
				Train(Attribute::STR, int(points / 10), T_DEFENSE);
				Train(Attribute::DEX, int(points / 5), T_DEFENSE);
			}
			Train(Attribute::CON, int(points), T_DEFENSE);
			if(s != Skill::NONE)
				Train(s, (int)points*2, T_DEFENSE);
		}
		break;
	case Train_Block:
		{
			// gracz zablokowa³ cios tarcz¹
			float mylvl = CalculateLevel(b_str | b_dex, BIT((int)Skill::SHIELD), USE_SHIELD);
			float points = value * (source_lvl / mylvl);
			Train(Attribute::STR, int(points / 2), T_DEFENSE);
			Train(Attribute::DEX, int(points / 4), T_DEFENSE);
			Train(Skill::SHIELD, (int)points*2, T_DEFENSE);
		}
		break;
	case Train_Bash:
		{
			// gracz waln¹³ kogoœ tarcz¹
			float mylvl = CalculateLevel(b_str | b_dex, BIT((int)Skill::SHIELD), USE_SHIELD);
			float points = value * (source_lvl / mylvl);
			Train(Attribute::STR, int(points / 2), T_OFENSE);
			Train(Attribute::DEX, int(points / 4), T_OFENSE);
			Train(Skill::SHIELD, (int)points*2, T_OFENSE);
		}
		break;
	case Train_Shot:
		{
			// gracz strzeli³ do kogoœ z ³uku
			float points = value * (source_lvl / precalclvl);
			Train(Attribute::STR, int(points / 4), T_OFENSE);
			Train(Attribute::DEX, int(points), T_OFENSE);
			Train(Skill::BOW, (int)points*2, T_OFENSE);
		}
		break;
	default:
		assert(0);
		break;
	}
}

//=================================================================================================
void PlayerController::Init(Unit& _unit)
{
	unit = &_unit;
	move_tick = 0.f;
	ostatnia = B_BRAK;
	po_akcja = PO_BRAK;
	last_dmg_poison = last_dmg = dmgc = poison_dmgc = 0.f;
	idle_timer = random(1.f, 2.f);
	kredyt = 0;
	na_kredycie = false;
	godmode = false;
	noclip = false;
	action = Action_None;
	free_days = 0;
	kills = 0;
	dmg_done = 0;
	dmg_taken = 0;
	knocks = 0;
	arena_fights = 0;

	for(int i = 0; i<(int)Skill::MAX; ++i)
	{
		sp[i] = 0;
		sn[i] = GetRequiredSkillPoints(_unit.skill[i]);
		for(int j=0; j<T_MAX; ++j)
			spg[i][j] = 0;
	}
	for(int i = 0; i<(int)Attribute::MAX; ++i)
	{
		ap[i] = 0;
		an[i] = GetRequiredAttributePoints(_unit.attrib[i]);
		for(int j=0; j<T_MAX; ++j)
			apg[i][j] = 0;
	}

// 	chain = 0;
// 	chain_timer = 0.f;
}

//=================================================================================================
void PlayerController::Update(float dt)
{
	dmgc += last_dmg;
	dmgc *= (1.f - dt*2);
	if(last_dmg == 0.f && dmgc < 0.1f)
		dmgc = 0.f;

	poison_dmgc += (last_dmg_poison - poison_dmgc) * dt;
	if(last_dmg_poison == 0.f && poison_dmgc < 0.1f)
		poison_dmgc = 0.f;
}

//=================================================================================================
int SkillToGain(Skill s)
{
	switch(s)
	{
	case Skill::WEAPON:
		return GAIN_STAT_WEP;
	case Skill::SHIELD:
		return GAIN_STAT_SHI;
	case Skill::BOW:
		return GAIN_STAT_BOW;
	case Skill::LIGHT_ARMOR:
		return GAIN_STAT_LAR;
	case Skill::HEAVY_ARMOR:
		return GAIN_STAT_HAR;
	default:
		assert(0);
		return GAIN_STAT_MAX;
	}
}

//=================================================================================================
void PlayerController::Train(Skill skill, int ile, TRAIN type)
{
	int s = (int)skill;
	int zdobyto = 0;
	sp[s] += ile;
	while(sp[s] >= sn[s])
	{
		sp[s] -= sn[s];
		if(unit->skill[s] != 100)
		{
			++unit->skill[s];
			sn[s] = GetRequiredSkillPoints(unit->skill[s]);
			unit->CalculateLevel();

			++zdobyto;
		}
		else
			break;
	}
	spg[s][type] += ile;

	if(zdobyto)
	{
		Game& game = Game::Get();
		if(this == game.pc)
		{
			if(SHOW_HERO_GAIN)
				game.ShowStatGain(SkillToGain(skill), zdobyto);
		}
		else if(game.IsOnline())
		{
			PlayerInfo& info = game.GetPlayerInfo(id);
			if(SHOW_HERO_GAIN)
			{
				NetChangePlayer& c = Add1(game.net_changes_player);
				c.type = NetChangePlayer::GAIN_STAT;
				c.id = SkillToGain(skill);
				c.ile = zdobyto;
				c.pc = this;
				info.NeedUpdate();
			}
			info.update_flags |= PlayerInfo::UF_SKILLS;
		}
	}
}

//=================================================================================================
int AttributeToGain(Attribute a)
{
	switch(a)
	{
	case Attribute::STR:
		return GAIN_STAT_STR;
	case Attribute::CON:
		return GAIN_STAT_END;
	case Attribute::DEX:
		return GAIN_STAT_DEX;
	default:
		assert(0);
		return GAIN_STAT_MAX;
	}
}

//=================================================================================================
void PlayerController::Train(Attribute attrib, int ile, TRAIN type)
{
	int a = (int)attrib;
	ap[a] += ile;
	int zdobyto = 0;
	while(ap[a] >= an[a])
	{
		ap[a] -= an[a];
		if(unit->attrib[a] != 100)
		{
			++unit->attrib[a];
			an[a] = GetRequiredAttributePoints(unit->attrib[a]);

			++zdobyto;
		}
		else
			break;
	}
	apg[a][type] += ile;

	if(zdobyto)
	{
		if(attrib == Attribute::STR)
		{
			unit->CalculateLoad();
			unit->RecalculateHp();
		}
		else if(attrib == Attribute::CON)
			unit->RecalculateHp();
		unit->CalculateLevel();

		Game& game = Game::Get();
		if(this == game.pc && SHOW_HERO_GAIN)
			game.ShowStatGain(AttributeToGain(attrib), zdobyto);
		if(game.IsOnline())
		{
			if(this != game.pc)
			{
				PlayerInfo& info = game.GetPlayerInfo(id);
				info.update_flags |= PlayerInfo::UF_ATTRIB;
				if(SHOW_HERO_GAIN)
				{
					NetChangePlayer& c = Add1(game.net_changes_player);
					c.type = NetChangePlayer::GAIN_STAT;
					c.pc = this;
					c.id = AttributeToGain(attrib);
					c.ile = zdobyto;
					info.NeedUpdate();
				}
			}
			if(attrib != Attribute::DEX)
			{
				NetChange& c = Add1(game.net_changes);
				c.type = NetChange::UPDATE_HP;
				c.unit = unit;
			}
		}
	}
}

//=================================================================================================
void PlayerController::TrainMove(float dt)
{
	move_tick += dt;
	if(move_tick >= 1.f)
	{
		move_tick -= 1.f;

		int dex, str;

		switch(unit->GetLoadState())
		{
		case 0: // brak
			dex = 50;
			str = 5;
			break;
		case 1: // lekkie
			dex = 37;
			str = 15;
			break;
		case 2: // œrednie
			dex = 25;
			str = 50;
			break;
		case 3: // ciê¿kie
			dex = 7;
			str = 75;
			break;
		case 4: // maksymalne
		default:
			str = 100;
			dex = 2;
			break;
		}

		Train(Attribute::STR, str, T_WALK);
		Train(Attribute::DEX, dex, T_WALK);

		if(unit->HaveArmor())
			Train(unit->GetArmor().GetSkill(), unit->GetArmor().IsHeavy() ? str : dex, T_WALK);
	}
}

//=================================================================================================
void PlayerController::TravelTick()
{
	Rest(false);
	TrainMove(1.f);

	// up³yw czasu efektów
	uint index = 0;
	for(vector<Effect>::iterator it = unit->effects.begin(), end = unit->effects.end(); it != end; ++it, ++index)
	{
		if((it->time -= 1.f) <= 0.f)
			_to_remove.push_back(index);
	}

	while(!_to_remove.empty())
	{
		index = _to_remove.back();
		_to_remove.pop_back();
		if(index == unit->effects.size()-1)
			unit->effects.pop_back();
		else
		{
			std::iter_swap(unit->effects.begin()+index, unit->effects.end()-1);
			unit->effects.pop_back();
		}
	}
}

//=================================================================================================
// wywo³ywane podczas podró¿y
void PlayerController::Rest(bool resting)
{
	if(unit->hp != unit->hpmax)
	{
		float heal = 0.5f * unit->attrib[(int)Attribute::CON];
		if(resting)
			heal *= 2;
		float reg;
		if(unit->FindEffect(E_NATURAL, &reg))
			heal *= reg;

		heal = min(heal, unit->hpmax-unit->hp);
		unit->hp += heal;

		Train(Attribute::CON, int(heal), T_WALK);
	}

	last_dmg = 0;
	last_dmg_poison = 0;
	dmgc = 0;
	poison_dmgc = 0;
}

//=================================================================================================
void PlayerController::Rest(int days, bool resting)
{
	// up³yw czasu efektów
	int best_nat;
	unit->EndEffects(days, &best_nat);
	
	// regeneracja hp
	if(unit->hp != unit->hpmax)
	{
		float heal = 0.5f * unit->attrib[(int)Attribute::CON];
		if(resting)
			heal *= 2;
		if(best_nat)
		{
			if(best_nat != days)
				heal = heal*best_nat*2 + heal*(days-best_nat);
			else
				heal *= 2*days;
		}
		else
			heal *= days;

		heal = min(heal, unit->hpmax-unit->hp);
		unit->hp += heal;

		Game& game = Game::Get();
		if(game.IsOnline())
		{
			NetChange& c = Add1(game.net_changes);
			c.type = NetChange::UPDATE_HP;
			c.unit = unit;
		}

		Train(Attribute::CON, int(heal), T_WALK);
	}

	last_dmg = 0;
	last_dmg_poison = 0;
	dmgc = 0;
	poison_dmgc = 0;
}

//=================================================================================================
void PlayerController::Save(HANDLE file)
{
	WriteFile(file, &clas, sizeof(clas), &tmp, NULL);
	byte len = (byte)name.length();
	WriteFile(file, &len, sizeof(len), &tmp, NULL);
	WriteFile(file, name.c_str(), len, &tmp, NULL);
	WriteFile(file, &move_tick, sizeof(move_tick), &tmp, NULL);
	WriteFile(file, &last_dmg, sizeof(last_dmg), &tmp, NULL);
	WriteFile(file, &last_dmg_poison, sizeof(last_dmg_poison), &tmp, NULL);
	WriteFile(file, &dmgc, sizeof(dmgc), &tmp, NULL);
	WriteFile(file, &poison_dmgc, sizeof(poison_dmgc), &tmp, NULL);
	WriteFile(file, &idle_timer, sizeof(idle_timer), &tmp, NULL);
	WriteFile(file, sp, sizeof(sp), &tmp, NULL);
	WriteFile(file, sn, sizeof(sn), &tmp, NULL);
	WriteFile(file, ap, sizeof(ap), &tmp, NULL);
	WriteFile(file, an, sizeof(an), &tmp, NULL);
	WriteFile(file, spg, sizeof(spg), &tmp, NULL);
	WriteFile(file, apg, sizeof(apg), &tmp, NULL);
	WriteFile(file, &klawisz, sizeof(klawisz), &tmp, NULL);
	WriteFile(file, &po_akcja, sizeof(po_akcja), &tmp, NULL);
	WriteFile(file, &po_akcja_idx, sizeof(po_akcja_idx), &tmp, NULL);
	WriteFile(file, &ostatnia, sizeof(ostatnia), &tmp, NULL);
	WriteFile(file, &kredyt, sizeof(kredyt), &tmp, NULL);
	WriteFile(file, &godmode, sizeof(godmode), &tmp, NULL);
	WriteFile(file, &noclip, sizeof(noclip), &tmp, NULL);
	WriteFile(file, &id, sizeof(id), &tmp, NULL);
	WriteFile(file, &free_days, sizeof(free_days), &tmp, NULL);
	WriteFile(file, &kills, sizeof(kills), &tmp, NULL);
	WriteFile(file, &knocks, sizeof(knocks), &tmp, NULL);
	WriteFile(file, &dmg_done, sizeof(dmg_done), &tmp, NULL);
	WriteFile(file, &dmg_taken, sizeof(dmg_taken), &tmp, NULL);
	WriteFile(file, &arena_fights, sizeof(arena_fights), &tmp, NULL);
}

//=================================================================================================
void PlayerController::Load(HANDLE file)
{
	ReadFile(file, &clas, sizeof(clas), &tmp, NULL);
	byte len;
	ReadFile(file, &len, sizeof(len), &tmp, NULL);
	BUF[len] = 0;
	ReadFile(file, BUF, len, &tmp, NULL);
	name = BUF;
	if(LOAD_VERSION < V_0_2_10)
	{
		float old_weight;
		ReadFile(file, &old_weight, sizeof(old_weight), &tmp, NULL);
	}
	ReadFile(file, &move_tick, sizeof(move_tick), &tmp, NULL);
	ReadFile(file, &last_dmg, sizeof(last_dmg), &tmp, NULL);
	ReadFile(file, &last_dmg_poison, sizeof(last_dmg_poison), &tmp, NULL);
	ReadFile(file, &dmgc, sizeof(dmgc), &tmp, NULL);
	ReadFile(file, &poison_dmgc, sizeof(poison_dmgc), &tmp, NULL);
	ReadFile(file, &idle_timer, sizeof(idle_timer), &tmp, NULL);
	ReadFile(file, sp, sizeof(sp), &tmp, NULL);
	ReadFile(file, sn, sizeof(sn), &tmp, NULL);
	ReadFile(file, ap, sizeof(ap), &tmp, NULL);
	ReadFile(file, an, sizeof(an), &tmp, NULL);
	ReadFile(file, spg, sizeof(spg), &tmp, NULL);
	ReadFile(file, apg, sizeof(apg), &tmp, NULL);
	ReadFile(file, &klawisz, sizeof(klawisz), &tmp, NULL);
	ReadFile(file, &po_akcja, sizeof(po_akcja), &tmp, NULL);
	ReadFile(file, &po_akcja_idx, sizeof(po_akcja_idx), &tmp, NULL);
	ReadFile(file, &ostatnia, sizeof(ostatnia), &tmp, NULL);
	if(LOAD_VERSION == V_0_2)
	{
		bool resting;
		ReadFile(file, &resting, sizeof(resting), &tmp, NULL);
	}
	if(LOAD_VERSION < V_0_2_20)
	{
		// stary raise_timer, teraz jest w Unit
		float raise_timer;
		ReadFile(file, &raise_timer, sizeof(raise_timer), &tmp, NULL);
	}
	ReadFile(file, &kredyt, sizeof(kredyt), &tmp, NULL);
	ReadFile(file, &godmode, sizeof(godmode), &tmp, NULL);
	ReadFile(file, &noclip, sizeof(noclip), &tmp, NULL);
	ReadFile(file, &id, sizeof(id), &tmp, NULL);
	ReadFile(file, &free_days, sizeof(free_days), &tmp, NULL);
	if(LOAD_VERSION == V_0_2)
		kills = 0;
	else
		ReadFile(file, &kills, sizeof(kills), &tmp, NULL);
	if(LOAD_VERSION < V_0_2_10)
	{
		knocks = 0;
		dmg_done = 0;
		dmg_taken = 0;
		arena_fights = 0;
	}
	else
	{
		ReadFile(file, &knocks, sizeof(knocks), &tmp, NULL);
		ReadFile(file, &dmg_done, sizeof(dmg_done), &tmp, NULL);
		ReadFile(file, &dmg_taken, sizeof(dmg_taken), &tmp, NULL);
		ReadFile(file, &arena_fights, sizeof(arena_fights), &tmp, NULL);
	}

	action = Action_None;
}

//=================================================================================================
int PlayerController::GetRequiredAttributePoints(int level)
{
	return 6*(level-40)*(level-40);
}

//=================================================================================================
int PlayerController::GetRequiredSkillPoints(int level)
{
	return 4*(level+1)*(level+2);
}
