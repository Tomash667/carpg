#include "Pch.h"
#include "GameCore.h"
#include "AbilityLoader.h"
#include "Ability.h"
#include <ResourceManager.h>
#include <Mesh.h>

enum Group
{
	G_TOP,
	G_KEYWORD,
	G_TYPE,
	G_FLAG
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
	K_TEX_PARTICLE,
	K_TEX_EXPLODE,
	K_SOUND_CAST,
	K_SOUND_HIT,
	K_MANA,
	K_MOVE_RANGE,
	K_ICON,
	K_CHARGES,
	K_RECHARGE,
	K_WIDTH,
	K_STAMINA
};

//=================================================================================================
void AbilityLoader::DoLoading()
{
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
		{ "tex_particle", K_TEX_PARTICLE },
		{ "tex_explode", K_TEX_EXPLODE },
		{ "sound_cast", K_SOUND_CAST },
		{ "sound_hit", K_SOUND_HIT },
		{ "mana", K_MANA },
		{ "move_range", K_MOVE_RANGE },
		{ "icon", K_ICON },
		{ "charges", K_CHARGES },
		{ "recharge", K_RECHARGE },
		{ "width", K_WIDTH },
		{ "stamina", K_STAMINA }
		});

	t.AddKeywords(G_TYPE, {
		{ "point", Ability::Point },
		{ "ray", Ability::Ray },
		{ "target", Ability::Target },
		{ "ball", Ability::Ball },
		{ "charge", Ability::Charge },
		{ "summon", Ability::Summon }
		});

	t.AddKeywords(G_FLAG, {
		{ "explode", Ability::Explode },
		{ "poison", Ability::Poison },
		{ "raise", Ability::Raise },
		{ "jump", Ability::Jump },
		{ "drain", Ability::Drain },
		{ "hold", Ability::Hold },
		{ "triple", Ability::Triple },
		{ "heal", Ability::Heal },
		{ "non_combat", Ability::NonCombat },
		{ "cleric", Ability::Cleric },
		{ "ignore_units", Ability::IgnoreUnits },
		{ "pick_dir", Ability::PickDir },
		{ "use_cast", Ability::UseCast }
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
	if(Ability::TryGet(id))
		t.Throw("Id must be unique.");

	Ptr<Ability> ability;
	ability->id = id;
	crc.Update(ability->id);
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
				ability->dmg_bonus = t.MustGetInt();
				if(ability->dmg_bonus < 0)
					t.Throw("Negative bonus damage %d.", ability->dmg_bonus);
				crc.Update(ability->dmg_bonus);
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
			ability->explode_range = t.MustGetFloat();
			if(ability->explode_range < 0.f)
				t.Throw("Negative explode range %g.", ability->explode_range);
			crc.Update(ability->explode_range);
			t.Next();
			break;
		case K_MESH:
			{
				const string& mesh_id = t.MustGetString();
				ability->mesh = res_mgr->TryGet<Mesh>(mesh_id);
				if(!ability->mesh)
					t.Throw("Missing mesh '%s'.", mesh_id.c_str());
				crc.Update(mesh_id);
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
				const string& tex_id = t.MustGetString();
				ability->tex = res_mgr->TryGet<Texture>(tex_id);
				if(!ability->tex)
					t.Throw("Missing texture '%s'.", tex_id.c_str());
				crc.Update(tex_id);
				t.Next();
				ability->size = t.MustGetFloat();
				if(ability->size <= 0.f)
					t.Throw("Invalid texture size %g.", ability->size);
				crc.Update(ability->size);
				t.Next();
			}
			break;
		case K_TEX_PARTICLE:
			{
				const string& tex_id = t.MustGetString();
				ability->tex_particle = res_mgr->TryGet<Texture>(tex_id);
				if(!ability->tex_particle)
					t.Throw("Missing texture '%s'.", tex_id.c_str());
				crc.Update(tex_id);
				t.Next();
				ability->size_particle = t.MustGetFloat();
				if(ability->size_particle <= 0.f)
					t.Throw("Invalid particle texture size %g.", ability->size_particle);
				crc.Update(ability->size_particle);
				t.Next();
			}
			break;
		case K_TEX_EXPLODE:
			{
				const string& tex_id = t.MustGetString();
				ability->tex_explode = res_mgr->TryGet<Texture>(tex_id);
				if(!ability->tex_explode)
					t.Throw("Missing texture '%s'.", tex_id.c_str());
				crc.Update(tex_id);
				t.Next();
			}
			break;
		case K_SOUND_CAST:
			{
				const string& sound_id = t.MustGetString();
				ability->sound_cast = res_mgr->TryGet<Sound>(sound_id);
				if(!ability->sound_cast)
					t.Throw("Missing sound '%s'.", sound_id.c_str());
				crc.Update(sound_id);
				t.Next();
				ability->sound_cast_dist = t.MustGetFloat();
				if(ability->sound_cast_dist <= 0.f)
					t.Throw("Invalid cast sound distance %g.", ability->sound_cast_dist);
				crc.Update(ability->sound_cast_dist);
				t.Next();
			}
			break;
		case K_SOUND_HIT:
			{
				const string& sound_id = t.MustGetString();
				ability->sound_hit = res_mgr->TryGet<Sound>(sound_id);
				if(!ability->sound_hit)
					t.Throw("Missing sound '%s'.", sound_id.c_str());
				crc.Update(sound_id);
				t.Next();
				ability->sound_hit_dist = t.MustGetFloat();
				if(ability->sound_hit_dist <= 0.f)
					t.Throw("Invalid hit sound distance %g}.", ability->sound_hit_dist);
				crc.Update(ability->sound_hit_dist);
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
			ability->move_range = t.MustGetFloat();
			if(ability->move_range < 0.f)
				t.Throw("Negative move range %g.", ability->move_range);
			crc.Update(ability->move_range);
			t.Next();
			break;
		case K_ICON:
			ability->tex_icon = res_mgr->TryGet<Texture>(Format("%s.png", ability->id.c_str()));
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
		}
	}

	if(Any(ability->type, Ability::Point, Ability::Ball) && ability->speed == 0.f)
		LoadWarning("Invalid ability speed.");

	if(ability->recharge == 0)
		ability->recharge = ability->cooldown.x;

	Ability::abilities.push_back(ability.Pin());
}

//=================================================================================================
void AbilityLoader::ParseAlias(const string& id)
{
	Ability* ability = Ability::TryGet(id);
	if(!ability)
		t.Throw("Missing ability '%s'.", id.c_str());
	t.Next();

	const string& alias_id = t.MustGetItemKeyword();
	Ability* alias = Ability::TryGet(alias_id);
	if(alias)
		t.Throw("Alias or ability already exists.");

	Ability::aliases.push_back(pair<string, Ability*>(alias_id, ability));
}

//=================================================================================================
void AbilityLoader::Finalize()
{
	content.crc[(int)Content::Id::Abilities] = crc.Get();

	Info("Loaded abilities (%u) - crc %p.", Ability::abilities.size(), content.crc[(int)Content::Id::Abilities]);
}
