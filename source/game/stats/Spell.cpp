#include "Pch.h"
#include "Base.h"
#include "Spell.h"
#include "Crc.h"

vector<Spell*> spells;
extern string g_system_dir;

//-----------------------------------------------------------------------------
Spell g_spells[] = {
	Spell(0, "magic_bolt", Spell::Point, 0, 25, 2, VEC2(0,0), 50.f, 15.f, "mp.png", "mpp.png", nullptr, 0.05f, 0.02f, "flaunch.wav", "punch1.wav", VEC2(2,8), VEC2(2,8), 0.f, nullptr),
	Spell(1, "xmagic_bolt", Spell::Point, Spell::Triple, 20, 1, VEC2(0,0), 50.f, 15.f, "xmp.png", "xmpp.png", nullptr, 0.05f, 0.02f, "flaunch.wav", "punch1.wav", VEC2(2,8), VEC2(2,8), 0.f, nullptr),
	Spell(2, "fireball", Spell::Point, Spell::Explode, 100, 5, VEC2(2.5f,5.f), 50.f, 8.f, "flare.png", "flare.png", "explosion.jpg", 0.2f, 0.075f, "rlaunch.mp3", "explode.mp3", VEC2(2,8), VEC2(4,15), 2.f, nullptr),
	Spell(3, "spit_poison", Spell::Ball, Spell::Poison, 50, 0, VEC2(5.f,10.f), 20.f, 12.f, "spit.png", "spitp.png", nullptr, 0.1f, 0.03f, "splash1.wav", "splash2.mp3", VEC2(2,8), VEC2(2,8), 0.f, nullptr),
	Spell(4, "raise", Spell::Target, Spell::Raise | Spell::NonCombat, 0, 0, VEC2(10.f, 15.f), 15.f, 0.f, nullptr, "czarna_iskra.png", nullptr, 1.f, 0.1f, "Darkness3.ogg", nullptr, VEC2(2, 5), VEC2(0, 0), 0.f, nullptr),
	Spell(5, "thunder_bolt", Spell::Ray, Spell::Jump, 150, 3, VEC2(7.5f,12.5f), 50.f, 0.f, nullptr, "iskra.png", nullptr, 1.f, 0.03f, "lighting_bolt.mp3", "zap.mp3", VEC2(4,12), VEC2(2,8), 0.f, nullptr),
	Spell(6, "drain", Spell::Ray, Spell::Drain|Spell::Hold, 50, 1, VEC2(2.5f,5.f), 5.f, 0.f, nullptr, nullptr, nullptr, 1.f, 1.f, "suck.mp3", nullptr, VEC2(2.f,5.f), VEC2(0,0), 0.f, nullptr),
	Spell(7, "drain2", Spell::Ray, Spell::Drain|Spell::Hold, 80, 1, VEC2(3.5f,6.f), 6.f, 0.f, nullptr, nullptr, nullptr, 1.f, 1.f, "suck.mp3", nullptr, VEC2(2.f,5.f), VEC2(0,0), 0.f, nullptr),
	Spell(8, "exploding_skull", Spell::Point, Spell::Explode, 120, 2, VEC2(5.f,10.f), 40.f, 7.5f, nullptr, "flare.png", "explosion.jpg", 0.13f, 0.075f, "evil_laught.mp3", "explode.mp3", VEC2(3,8), VEC2(4,12), 2.5f, "czaszka.qmsh"),
	Spell(9, "heal", Spell::Target, Spell::Heal | Spell::NonCombat, 100, 10, VEC2(10.f, 15.f), 15.f, 0.f, nullptr, "heal.png", nullptr, 1.f, 0.03f, "heal.ogg", nullptr, VEC2(2, 5), VEC2(0, 0), 0.f, nullptr),
	Spell(10, "mystic_ball", Spell::Point, Spell::Explode, 200, 0, VEC2(10.f,10.f), 50.f, 18.f, "flare2.png", "flare2.png", "rainbow.jpg", 0.2f, 0.075f, "rlaunch.mp3", "explode.mp3", VEC2(2,8), VEC2(4,15), 3.f, nullptr)
};
const uint n_spells = countof(g_spells);

//=================================================================================================
Spell* FindSpell(cstring name)
{
	assert(name);

	for(Spell& s : g_spells)
	{
		if(s.name == name)
			return &s;
	}

	assert(0);
	return &g_spells[0];
}

enum Group
{
	G_TOP,
	G_KEYWORD,
	G_TYPE,
	G_FLAG
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
		spell->id = (int)spells.size();
		spell->name = t.MustGetItemKeyword();
		crc.Update(spell->name);
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
		ERROR(Format("Failed to load spell '%s': %s", spell->name.c_str(), e.ToString()));
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

	t.AddKeyword("spell", 0, G_TOP);

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

			if(t.IsKeyword(0, G_TOP))
			{
				if(!LoadSpell(t, crc))
				{
					++errors;
					skip = true;
				}
			}
			else
			{
				int k = 0, g = G_TOP;
				ERROR(t.FormatUnexpected(Tokenizer::T_KEYWORD, &k, &g));
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
