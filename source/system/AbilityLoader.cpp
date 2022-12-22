#include "Pch.h"
#include "AbilityLoader.h"

#include "Ability.h"
#include "BaseTrap.h"
#include "GameResources.h"
#include "UnitData.h"

#include <Mesh.h>
#include <ResourceManager.h>

enum Group
{
	G_TOP,
	G_KEYWORD,
	G_TYPE,
	G_FLAG,
	G_EFFECT
};

enum TopKeyword
{
	TK_ABILITY,
	TK_ALIAS
};

enum Keyword
{
	K_TYPE,
	K_FLAGS,
	K_DMG,
	K_COOLDOWN,
	K_RANGE,
	K_SPEED,
	K_EXPLODE_RANGE,
	K_MESH,
	K_TEX,
	K_TEX_EXPLODE,
	K_SOUND_CAST,
	K_SOUND_HIT,
	K_MANA,
	K_MOVE_RANGE,
	K_ICON,
	K_CHARGES,
	K_RECHARGE,
	K_WIDTH,
	K_STAMINA,
	K_UNIT,
	K_EFFECT,
	K_LEARNING_POINTS,
	K_SKILL,
	K_LEVEL,
	K_ANIMATION,
	K_COUNT,
	K_TIME,
	K_COLOR,
	K_TRAP_ID,
	K_CAST_TIME,
	K_PARTICLE_EFFECT
};

//=================================================================================================
void AbilityLoader::DoLoading()
{
	tPlaceholder = gameRes->CreatePlaceholderTexture(Int2(128, 128));
	Load("abilities.txt", G_TOP);
}

//=================================================================================================
void AbilityLoader::Cleanup()
{
	DeleteElements(Ability::abilities);
}

//=================================================================================================
void AbilityLoader::InitTokenizer()
{
	t.AddKeywords(G_TOP, {
		{ "ability", TK_ABILITY },
		{ "alias", TK_ALIAS }
		});

	t.AddKeywords(G_KEYWORD, {
		{ "type", K_TYPE },
		{ "flags", K_FLAGS },
		{ "dmg", K_DMG },
		{ "cooldown", K_COOLDOWN },
		{ "range", K_RANGE },
		{ "speed", K_SPEED },
		{ "explode_range", K_EXPLODE_RANGE },
		{ "mesh", K_MESH },
		{ "tex", K_TEX },
		{ "tex_explode", K_TEX_EXPLODE },
		{ "sound_cast", K_SOUND_CAST },
		{ "sound_hit", K_SOUND_HIT },
		{ "mana", K_MANA },
		{ "move_range", K_MOVE_RANGE },
		{ "icon", K_ICON },
		{ "charges", K_CHARGES },
		{ "recharge", K_RECHARGE },
		{ "width", K_WIDTH },
		{ "stamina", K_STAMINA },
		{ "unit", K_UNIT },
		{ "effect", K_EFFECT },
		{ "learning_points", K_LEARNING_POINTS },
		{ "skill", K_SKILL },
		{ "level", K_LEVEL },
		{ "animation", K_ANIMATION },
		{ "count", K_COUNT },
		{ "time", K_TIME },
		{ "color", K_COLOR },
		{ "trap_id", K_TRAP_ID },
		{ "cast_time", K_CAST_TIME },
		//{ "particle_effect", K_PARTICLE_EFFECT }
		});
	FIXME;

	t.AddKeywords(G_TYPE, {
		{ "point", Ability::Point },
		{ "ray", Ability::Ray },
		{ "target", Ability::Target },
		{ "ball", Ability::Ball },
		{ "charge", Ability::Charge },
		{ "summon", Ability::Summon },
		{ "aggro", Ability::Aggro },
		{ "summon_away", Ability::SummonAway },
		{ "ranged_attack", Ability::RangedAttack },
		{ "trap", Ability::Trap }
		});

	t.AddKeywords(G_EFFECT, {
		{ "heal", Ability::Heal },
		{ "raise", Ability::Raise },
		{ "drain", Ability::Drain },
		{ "electro", Ability::Electro },
		{ "stun", Ability::Stun },
		{ "rooted", Ability::Rooted }
		});

	t.AddKeywords(G_FLAG, {
		{ "explode", Ability::Explode },
		{ "poison", Ability::Poison },
		{ "triple", Ability::Triple },
		{ "non_combat", Ability::NonCombat },
		{ "cleric", Ability::Cleric },
		{ "ignore_units", Ability::IgnoreUnits },
		{ "pick_dir", Ability::PickDir },
		{ "use_cast", Ability::UseCast },
		{ "mage", Ability::Mage },
		{ "strength", Ability::Strength },
		{ "boss50hp", Ability::Boss50Hp },
		{ "default_attack", Ability::DefaultAttack },
		{ "use_kneel", Ability::UseKneel }
		});
}

//=================================================================================================
void AbilityLoader::LoadEntity(int top, const string& id)
{
	switch(top)
	{
	case TK_ABILITY:
		ParseAbility(id);
		break;
	case TK_ALIAS:
		ParseAlias(id);
		break;
	}
}

//=================================================================================================
void AbilityLoader::ParseAbility(const string& id)
{
	int hash = Hash(id);
	Ability* existingAbility = Ability::Get(hash);
	if(existingAbility)
	{
		if(existingAbility->id == id)
			t.Throw("Id must be unique.");
		else
			t.Throw("Id hash collision.");
	}

	Ptr<Ability> ability;
	ability->hash = hash;
	ability->id = id;
	crc.Update(id);
	t.Next();

	t.AssertSymbol('{');
	t.Next();

	while(!t.IsSymbol('}'))
	{
		Keyword k = (Keyword)t.MustGetKeywordId(G_KEYWORD);
		t.Next();

		switch(k)
		{
		case K_TYPE:
			ability->type = (Ability::Type)t.MustGetKeywordId(G_TYPE);
			crc.Update(ability->type);
			t.Next();
			break;
		case K_FLAGS:
			t.ParseFlags(G_FLAG, ability->flags);
			crc.Update(ability->flags);
			t.Next();
			break;
		case K_DMG:
			ability->dmg = t.MustGetInt();
			if(ability->dmg < 0)
				t.Throw("Negative damage %d.", ability->dmg);
			crc.Update(ability->dmg);
			t.Next();
			if(t.IsSymbol('+'))
			{
				t.Next();
				ability->dmgBonus = t.MustGetInt();
				if(ability->dmgBonus < 0)
					t.Throw("Negative bonus damage %d.", ability->dmgBonus);
				crc.Update(ability->dmgBonus);
				t.Next();
			}
			break;
		case K_COOLDOWN:
			t.Parse(ability->cooldown);
			if(ability->cooldown.x < 0 || ability->cooldown.x > ability->cooldown.y)
				t.Throw("Invalid cooldown {%g %g}.", ability->cooldown.x, ability->cooldown.y);
			crc.Update(ability->cooldown);
			break;
		case K_RANGE:
			ability->range = t.MustGetFloat();
			if(ability->range < 0.f)
				t.Throw("Negative range %g.", ability->range);
			crc.Update(ability->range);
			t.Next();
			break;
		case K_SPEED:
			ability->speed = t.MustGetFloat();
			if(ability->speed < 0.f)
				t.Throw("Negative speed %g.", ability->speed);
			crc.Update(ability->speed);
			t.Next();
			break;
		case K_EXPLODE_RANGE:
			ability->explodeRange = t.MustGetFloat();
			if(ability->explodeRange < 0.f)
				t.Throw("Negative explode range %g.", ability->explodeRange);
			crc.Update(ability->explodeRange);
			t.Next();
			break;
		case K_MESH:
			{
				const string& meshId = t.MustGetString();
				ability->mesh = resMgr->TryGet<Mesh>(meshId);
				if(!ability->mesh)
					t.Throw("Missing mesh '%s'.", meshId.c_str());
				crc.Update(meshId);
				t.Next();
				ability->size = t.MustGetFloat();
				if(ability->size <= 0.f)
					t.Throw("Invalid mesh size %g.", ability->size);
				crc.Update(ability->size);
				t.Next();
			}
			break;
		case K_TEX:
			{
				const string& texId = t.MustGetString();
				ability->tex = resMgr->TryGet<Texture>(texId);
				if(!ability->tex)
					t.Throw("Missing texture '%s'.", texId.c_str());
				crc.Update(texId);
				t.Next();
				ability->size = t.MustGetFloat();
				if(ability->size <= 0.f)
					t.Throw("Invalid texture size %g.", ability->size);
				crc.Update(ability->size);
				t.Next();
			}
			break;
		case K_TEX_EXPLODE:
			{
				const string& texId = t.MustGetString();
				ability->texExplode.diffuse = resMgr->TryGet<Texture>(texId);
				if(!ability->texExplode.diffuse)
					t.Throw("Missing texture '%s'.", texId.c_str());
				crc.Update(texId);
				t.Next();
			}
			break;
		case K_SOUND_CAST:
			{
				const string& soundId = t.MustGetString();
				ability->soundCast = resMgr->TryGet<Sound>(soundId);
				if(!ability->soundCast)
					t.Throw("Missing sound '%s'.", soundId.c_str());
				crc.Update(soundId);
				t.Next();
				ability->soundCastDist = t.MustGetFloat();
				if(ability->soundCastDist <= 0.f)
					t.Throw("Invalid cast sound distance %g.", ability->soundCastDist);
				crc.Update(ability->soundCastDist);
				t.Next();
			}
			break;
		case K_SOUND_HIT:
			{
				const string& soundId = t.MustGetString();
				ability->soundHit = resMgr->TryGet<Sound>(soundId);
				if(!ability->soundHit)
					t.Throw("Missing sound '%s'.", soundId.c_str());
				crc.Update(soundId);
				t.Next();
				ability->soundHitDist = t.MustGetFloat();
				if(ability->soundHitDist <= 0.f)
					t.Throw("Invalid hit sound distance %g}.", ability->soundHitDist);
				crc.Update(ability->soundHitDist);
				t.Next();
			}
			break;
		case K_MANA:
			ability->mana = t.MustGetFloat();
			if(ability->mana < 0)
				LoadError("Nagative mana cost.");
			crc.Update(ability->mana);
			t.Next();
			break;
		case K_MOVE_RANGE:
			ability->moveRange = t.MustGetFloat();
			if(ability->moveRange < 0.f)
				t.Throw("Negative move range %g.", ability->moveRange);
			crc.Update(ability->moveRange);
			t.Next();
			break;
		case K_ICON:
			ability->texIcon = resMgr->TryGet<Texture>(Format("%s.png", ability->id.c_str()));
			break;
		case K_CHARGES:
			ability->charges = t.MustGetInt();
			if(ability->charges < 1)
				t.Throw("Invalid charges count %d.", ability->charges);
			crc.Update(ability->charges);
			t.Next();
			break;
		case K_RECHARGE:
			ability->recharge = t.MustGetFloat();
			if(ability->recharge <= 0.f)
				t.Throw("Invalid recharge value %g.", ability->recharge);
			crc.Update(ability->recharge);
			t.Next();
			break;
		case K_WIDTH:
			ability->width = t.MustGetFloat();
			crc.Update(ability->width);
			t.Next();
			break;
		case K_STAMINA:
			ability->stamina = t.MustGetFloat();
			if(ability->stamina < 0.f)
				t.Throw("Invalid stamina value %g.", ability->stamina);
			crc.Update(ability->stamina);
			t.Next();
			break;
		case K_UNIT:
			ability->unitId = t.MustGetItem();
			crc.Update(ability->unitId);
			t.Next();
			break;
		case K_EFFECT:
			ability->effect = (Ability::Effect)t.MustGetKeywordId(G_EFFECT);
			crc.Update(ability->effect);
			t.Next();
			break;
		case K_LEARNING_POINTS:
			ability->learningPoints = t.MustGetInt();
			crc.Update(ability->learningPoints);
			t.Next();
			break;
		case K_SKILL:
			ability->skill = t.MustGetInt();
			crc.Update(ability->skill);
			t.Next();
			break;
		case K_LEVEL:
			ability->level = t.MustGetInt();
			crc.Update(ability->level);
			t.Next();
			break;
		case K_ANIMATION:
			ability->animation = t.MustGetString();
			crc.Update(ability->animation);
			t.Next();
			break;
		case K_COUNT:
			ability->count = t.MustGetInt();
			crc.Update(ability->count);
			t.Next();
			break;
		case K_TIME:
			ability->time = t.MustGetFloat();
			if(ability->time < 0.f)
				t.Throw("Invalid time value.");
			crc.Update(ability->time);
			t.Next();
			break;
		case K_COLOR:
			t.Parse(ability->color);
			crc.Update(ability->color);
			break;
		case K_TRAP_ID:
			{
				const string& id = t.MustGetItem();
				ability->trap = BaseTrap::Get(id);
				if(!ability->trap)
					t.Throw("Missing trap '%s'.", id.c_str());
				crc.Update(id);
				t.Next();
			}
			break;
		case K_CAST_TIME:
			ability->castTime = t.MustGetFloat();
			if(ability->castTime < 0.f)
				t.Throw("Invalid cast time value.");
			crc.Update(ability->castTime);
			t.Next();
			break;
		}
	}

	if(Any(ability->type, Ability::Point, Ability::Ball) && ability->speed == 0.f)
		LoadWarning("Invalid ability speed.");
	if(IsSet(ability->flags, Ability::Explode) && !ability->texExplode.diffuse)
		LoadError("Missing explode texture.");

	if(ability->recharge == 0)
		ability->recharge = ability->cooldown.x;
	if(!ability->texIcon)
		ability->texIcon = tPlaceholder;

	Ability::hashAbilities[hash] = ability;
	Ability::abilities.push_back(ability.Pin());
}

//=================================================================================================
void AbilityLoader::ParseAlias(const string& id)
{
	Ability* ability = Ability::Get(id);
	if(!ability)
		t.Throw("Missing ability '%s'.", id.c_str());
	t.Next();

	const string& aliasId = t.MustGetItemKeyword();
	int hash = Hash(aliasId);
	if(Ability::Get(hash))
		t.Throw("Alias or ability already exists.");

	Ability::hashAbilities[hash] = ability;
}

//=================================================================================================
void AbilityLoader::Finalize()
{
	content.crc[(int)Content::Id::Abilities] = crc.Get();

	Info("Loaded abilities (%u) - crc %p.", Ability::abilities.size(), content.crc[(int)Content::Id::Abilities]);
}

//=================================================================================================
void AbilityLoader::ApplyUnits()
{
	for(Ability* ability : Ability::abilities)
	{
		if(ability->unitId.empty())
			continue;
		ability->unit = UnitData::TryGet(ability->unitId);
		if(!ability->unit)
		{
			SetLocalId(ability->id);
			LoadError("Missing unit '%s'.", ability->unitId.c_str());
		}
	}
}
