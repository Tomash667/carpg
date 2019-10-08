#include "Pch.h"
#include "GameCore.h"
#include "SpellLoader.h"
#include "Spell.h"
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
	TK_SPELL,
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
	K_MOVE_RANGE
};

//=================================================================================================
void SpellLoader::DoLoading()
{
	Load("spells.txt", G_TOP);
}

//=================================================================================================
void SpellLoader::Cleanup()
{
	DeleteElements(Spell::spells);
}

//=================================================================================================
void SpellLoader::InitTokenizer()
{
	t.AddKeywords(G_TOP, {
		{ "spell", TK_SPELL },
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
		{ "move_range", K_MOVE_RANGE }
		});

	t.AddKeywords(G_TYPE, {
		{ "point", Spell::Point },
		{ "ray", Spell::Ray },
		{ "target", Spell::Target },
		{ "ball", Spell::Ball }
		});

	t.AddKeywords(G_FLAG, {
		{ "explode", Spell::Explode },
		{ "poison", Spell::Poison },
		{ "raise", Spell::Raise },
		{ "jump", Spell::Jump },
		{ "drain", Spell::Drain },
		{ "hold", Spell::Hold },
		{ "triple", Spell::Triple },
		{ "heal", Spell::Heal },
		{ "non_combat", Spell::NonCombat },
		{ "cleric", Spell::Cleric }
		});
}

//=================================================================================================
void SpellLoader::LoadEntity(int top, const string& id)
{
	switch(top)
	{
	case TK_SPELL:
		ParseSpell(id);
		break;
	case TK_ALIAS:
		ParseAlias(id);
		break;
	}
}

//=================================================================================================
void SpellLoader::ParseSpell(const string& id)
{
	if(Spell::TryGet(id))
		t.Throw("Id must be unique.");

	Ptr<Spell> spell;
	spell->id = id;
	crc.Update(spell->id);
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
			spell->type = (Spell::Type)t.MustGetKeywordId(G_TYPE);
			crc.Update(spell->type);
			t.Next();
			break;
		case K_FLAGS:
			t.ParseFlags(G_FLAG, spell->flags);
			crc.Update(spell->flags);
			t.Next();
			break;
		case K_DMG:
			spell->dmg = t.MustGetInt();
			if(spell->dmg < 0)
				t.Throw("Negative damage %d.", spell->dmg);
			crc.Update(spell->dmg);
			t.Next();
			if(t.IsSymbol('+'))
			{
				t.Next();
				spell->dmg_bonus = t.MustGetInt();
				if(spell->dmg_bonus < 0)
					t.Throw("Negative bonus damage %d.", spell->dmg_bonus);
				crc.Update(spell->dmg_bonus);
				t.Next();
			}
			break;
		case K_COOLDOWN:
			t.Parse(spell->cooldown);
			if(spell->cooldown.x < 0 || spell->cooldown.x > spell->cooldown.y)
				t.Throw("Invalid cooldown {%g %g}.", spell->cooldown.x, spell->cooldown.y);
			crc.Update(spell->cooldown);
			break;
		case K_RANGE:
			spell->range = t.MustGetFloat();
			if(spell->range < 0.f)
				t.Throw("Negative range %g.", spell->range);
			crc.Update(spell->range);
			t.Next();
			break;
		case K_SPEED:
			spell->speed = t.MustGetFloat();
			if(spell->speed < 0.f)
				t.Throw("Negative speed %g.", spell->speed);
			crc.Update(spell->speed);
			t.Next();
			break;
		case K_EXPLODE_RANGE:
			spell->explode_range = t.MustGetFloat();
			if(spell->explode_range < 0.f)
				t.Throw("Negative explode range %g.", spell->explode_range);
			crc.Update(spell->explode_range);
			t.Next();
			break;
		case K_MESH:
			{
				const string& mesh_id = t.MustGetString();
				spell->mesh = res_mgr->TryGet<Mesh>(mesh_id);
				if(!spell->mesh)
					t.Throw("Missing mesh '%s'.", mesh_id.c_str());
				crc.Update(mesh_id);
				t.Next();
				spell->size = t.MustGetFloat();
				if(spell->size <= 0.f)
					t.Throw("Invalid mesh size %g.", spell->size);
				crc.Update(spell->size);
				t.Next();
			}
			break;
		case K_TEX:
			{
				const string& tex_id = t.MustGetString();
				spell->tex = res_mgr->TryGet<Texture>(tex_id);
				if(!spell->tex)
					t.Throw("Missing texture '%s'.", tex_id.c_str());
				crc.Update(tex_id);
				t.Next();
				spell->size = t.MustGetFloat();
				if(spell->size <= 0.f)
					t.Throw("Invalid texture size %g.", spell->size);
				crc.Update(spell->size);
				t.Next();
			}
			break;
		case K_TEX_PARTICLE:
			{
				const string& tex_id = t.MustGetString();
				spell->tex_particle = res_mgr->TryGet<Texture>(tex_id);
				if(!spell->tex_particle)
					t.Throw("Missing texture '%s'.", tex_id.c_str());
				crc.Update(tex_id);
				t.Next();
				spell->size_particle = t.MustGetFloat();
				if(spell->size_particle <= 0.f)
					t.Throw("Invalid particle texture size %g.", spell->size_particle);
				crc.Update(spell->size_particle);
				t.Next();
			}
			break;
		case K_TEX_EXPLODE:
			{
				const string& tex_id = t.MustGetString();
				spell->tex_explode = res_mgr->TryGet<Texture>(tex_id);
				if(!spell->tex_explode)
					t.Throw("Missing texture '%s'.", tex_id.c_str());
				crc.Update(tex_id);
				t.Next();
			}
			break;
		case K_SOUND_CAST:
			{
				const string& sound_id = t.MustGetString();
				spell->sound_cast = res_mgr->TryGet<Sound>(sound_id);
				if(!spell->sound_cast)
					t.Throw("Missing sound '%s'.", sound_id.c_str());
				crc.Update(sound_id);
				t.Next();
				spell->sound_cast_dist = t.MustGetFloat();
				if(spell->sound_cast_dist <= 0.f)
					t.Throw("Invalid cast sound distance %g.", spell->sound_cast_dist);
				crc.Update(spell->sound_cast_dist);
				t.Next();
			}
			break;
		case K_SOUND_HIT:
			{
				const string& sound_id = t.MustGetString();
				spell->sound_hit = res_mgr->TryGet<Sound>(sound_id);
				if(!spell->sound_hit)
					t.Throw("Missing sound '%s'.", sound_id.c_str());
				crc.Update(sound_id);
				t.Next();
				spell->sound_hit_dist = t.MustGetFloat();
				if(spell->sound_hit_dist <= 0.f)
					t.Throw("Invalid hit sound distance %g}.", spell->sound_hit_dist);
				crc.Update(spell->sound_hit_dist);
				t.Next();
			}
			break;
		case K_MANA:
			spell->mana = t.MustGetFloat();
			if(spell->mana < 0)
				LoadError("Nagative mana cost.");
			crc.Update(spell->mana);
			t.Next();
			break;
		case K_MOVE_RANGE:
			spell->move_range = t.MustGetFloat();
			if(spell->move_range < 0.f)
				t.Throw("Negative nove range %g.", spell->move_range);
			crc.Update(spell->move_range);
			t.Next();
			break;
		}
	}

	if(Any(spell->type, Spell::Point, Spell::Ball) && spell->speed == 0.f)
		LoadWarning("Invalid spell speed.");

	Spell::spells.push_back(spell.Pin());
}

//=================================================================================================
void SpellLoader::ParseAlias(const string& id)
{
	Spell* spell = Spell::TryGet(id);
	if(!spell)
		t.Throw("Missing spell '%s'.", id.c_str());
	t.Next();

	const string& alias_id = t.MustGetItemKeyword();
	Spell* alias = Spell::TryGet(alias_id);
	if(alias)
		t.Throw("Alias or spell already exists.");

	Spell::aliases.push_back(pair<string, Spell*>(alias_id, spell));
}

//=================================================================================================
void SpellLoader::Finalize()
{
	content.crc[(int)Content::Id::Spells] = crc.Get();

	Info("Loaded spells (%u) - crc %p.", Spell::spells.size(), content.crc[(int)Content::Id::Spells]);
}
