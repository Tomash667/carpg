#include "Pch.h"
#include "Base.h"
#include "Spell.h"
#include "Crc.h"

extern string g_system_dir;
vector<Spell*> spells;
vector<std::pair<string, Spell*>> spell_alias;

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
	K_SOUND_HIT
};

//=================================================================================================
bool LoadSpell(Tokenizer& t, CRC32& crc)
{
	Spell* spell = new Spell;

	try
	{
		t.Next();
		spell->id = t.MustGetItemKeyword();
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
				spell->flags = ReadFlags(t, G_FLAG);
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
				spell->range = t.MustGetNumberFloat();
				if(spell->range < 0.f)
					t.Throw("Negative range %g.", spell->range);
				crc.Update(spell->range);
				t.Next();
				break;
			case K_SPEED:
				spell->speed = t.MustGetNumberFloat();
				if(spell->speed < 0.f)
					t.Throw("Negative speed %g.", spell->speed);
				crc.Update(spell->speed);
				t.Next();
				break;
			case K_EXPLODE_RANGE:
				spell->explode_range = t.MustGetNumberFloat();
				if(spell->explode_range < 0.f)
					t.Throw("Negative explode range %g.", spell->explode_range);
				crc.Update(spell->explode_range);
				t.Next();
				break;
			case K_MESH:
				spell->mesh_id = t.MustGetString();
				if(spell->mesh_id.empty())
					t.Throw("Empty mesh.");
				crc.Update(spell->mesh_id);
				t.Next();
				spell->size = t.MustGetNumberFloat();
				if(spell->size <= 0.f)
					t.Throw("Invalid mesh size %g.", spell->size);
				crc.Update(spell->size);
				t.Next();
				break;
			case K_TEX:
				spell->tex_id = t.MustGetString();
				if(spell->tex_id.empty())
					t.Throw("Empty texture.");
				crc.Update(spell->tex_id);
				t.Next();
				spell->size = t.MustGetNumberFloat();
				if(spell->size <= 0.f)
					t.Throw("Invalid texture size %g.", spell->size);
				crc.Update(spell->size);
				t.Next();
				break;
			case K_TEX_PARTICLE:
				spell->tex_particle_id = t.MustGetString();
				if(spell->tex_particle_id.empty())
					t.Throw("Empty particle texture.");
				crc.Update(spell->tex_particle_id);
				t.Next();
				spell->size_particle = t.MustGetNumberFloat();
				if(spell->size_particle <= 0.f)
					t.Throw("Invalid particle texture size %g.", spell->size_particle);
				crc.Update(spell->size_particle);
				t.Next();
				break;
			case K_TEX_EXPLODE:
				spell->tex_explode_id = t.MustGetString();
				if(spell->tex_explode_id.empty())
					t.Throw("Empty explode texture.");
				crc.Update(spell->tex_explode_id);
				t.Next();
				break;
			case K_SOUND_CAST:
				spell->sound_cast_id = t.MustGetString();
				if(spell->sound_cast_id.empty())
					t.Throw("Empty cast sound.");
				crc.Update(spell->sound_cast_id);
				t.Next();
				t.Parse(spell->sound_cast_dist);
				if(spell->sound_cast_dist.x < 0 || spell->sound_cast_dist.x > spell->sound_cast_dist.y)
					t.Throw("Invalid cast sound distance {%g %g}.", spell->sound_cast_dist.x, spell->sound_cast_dist.y);
				crc.Update(spell->sound_cast_dist);
				break;
			case K_SOUND_HIT:
				spell->sound_hit_id = t.MustGetString();
				if(spell->sound_hit_id.empty())
					t.Throw("Empty hit sound.");
				crc.Update(spell->sound_hit_id);
				t.Next();
				t.Parse(spell->sound_hit_dist);
				if(spell->sound_hit_dist.x < 0 || spell->sound_hit_dist.x > spell->sound_hit_dist.y)
					t.Throw("Invalid hit sound distance {%g %g}.", spell->sound_hit_dist.x, spell->sound_hit_dist.y);
				crc.Update(spell->sound_hit_dist);
				break;
			}
		}

		for(Spell* s : spells)
		{
			if(s->id == spell->id)
				t.Throw("Spell with this id already exists.");
		}

		spells.push_back(spell);
		return true;
	}
	catch(const Tokenizer::Exception& e)
	{
		ERROR(Format("Failed to load spell '%s': %s", spell->id.c_str(), e.ToString()));
		delete spell;
		return false;
	}
}

//=================================================================================================
void LoadSpells(uint& out_crc)
{
	Tokenizer t;
	if(!t.FromFile(Format("%s/spells.txt", g_system_dir.c_str())))
		throw "Failed to open spells.txt.";

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
		{ "sound_hit", K_SOUND_HIT }
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
		{ "non_combat", Spell::NonCombat }
	});

	CRC32 crc;
	int errors = 0;

	try
	{
		t.Next();

		while(!t.IsEof())
		{
			bool skip = false;

			if(t.IsKeywordGroup(G_TOP))
			{
				TopKeyword top = (TopKeyword)t.GetKeywordId(G_TOP);
				if(top == TK_SPELL)
				{
					if(!LoadSpell(t, crc))
					{
						++errors;
						skip = true;
					}
				}
				else
				{
					try
					{
						t.Next();

						const string& spell_id = t.MustGetItemKeyword();
						Spell* spell = FindSpell(spell_id.c_str());
						if(!spell)
							t.Throw("Missing spell '%s'.", spell_id.c_str());
						t.Next();

						const string& alias_id = t.MustGetItemKeyword();
						Spell* alias = FindSpell(alias_id.c_str());
						if(alias)
							t.Throw("Alias or spell already exists.");

						spell_alias.push_back(std::pair<string, Spell*>(alias_id, spell));
					}
					catch(const Tokenizer::Exception& e)
					{
						ERROR(Format("Failed to load spell alias: %s", e.ToString()));
						++errors;
						skip = true;
					}
				}
			}
			else
			{
				int g = G_TOP;
				ERROR(t.FormatUnexpected(Tokenizer::T_KEYWORD_GROUP, &g));
				++errors;
				skip = true;
			}

			if(skip)
				t.SkipToKeywordGroup(G_TOP);
			else
				t.Next();
		}
	}
	catch(const Tokenizer::Exception& e)
	{
		ERROR(Format("Failed to load spells: %s", e.ToString()));
		++errors;
	}

	if(errors > 0)
		throw Format("Failed to load spells (%d errors), check log for details.", errors);

	out_crc = crc.Get();
}

//=================================================================================================
void CleanupSpells()
{
	DeleteElements(spells);
}
