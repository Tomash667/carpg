#include "Pch.h"
#include "Base.h"
#include "Spell.h"

//-----------------------------------------------------------------------------
Spell g_spells[] = {
	Spell(0, "magic_bolt", Spell::Point, 0, 25, 2, VEC2(0,0), 50.f, 15.f, "mp.png", "mpp.png", NULL, 0.05f, 0.02f, "flaunch.wav", "punch1.wav", VEC2(2,8), VEC2(2,8), 0.f, NULL),
	Spell(1, "xmagic_bolt", Spell::Point, Spell::Triple, 20, 1, VEC2(0,0), 50.f, 15.f, "xmp.png", "xmpp.png", NULL, 0.05f, 0.02f, "flaunch.wav", "punch1.wav", VEC2(2,8), VEC2(2,8), 0.f, NULL),
	Spell(2, "fireball", Spell::Point, Spell::Explode, 100, 5, VEC2(2.5f,5.f), 50.f, 8.f, "flare.png", "flare.png", "explosion.jpg", 0.2f, 0.075f, "rlaunch.mp3", "explode.mp3", VEC2(2,8), VEC2(4,15), 2.f, NULL),
	Spell(3, "spit_poison", Spell::Ball, Spell::Poison, 50, 0, VEC2(5.f,10.f), 20.f, 12.f, "spit.png", "spitp.png", NULL, 0.1f, 0.03f, "splash1.wav", "splash2.mp3", VEC2(2,8), VEC2(2,8), 0.f, NULL),
	Spell(4, "raise", Spell::Target, Spell::Raise|Spell::NonCombat, 0, 0, VEC2(10.f,15.f), 15.f, 0.f, NULL, NULL, "czarna_iskra.png", 1.f, 0.1f, "Darkness3.ogg", NULL, VEC2(2,5), VEC2(0,0), 0.f, NULL),
	Spell(5, "thunder_bolt", Spell::Ray, Spell::Jump, 150, 3, VEC2(7.5f,12.5f), 50.f, 0.f, NULL, "iskra.png", NULL, 1.f, 0.03f, "lighting_bolt.mp3", "zap.mp3", VEC2(4,12), VEC2(2,8), 0.f, NULL),
	Spell(6, "drain", Spell::Ray, Spell::Drain|Spell::Hold, 50, 1, VEC2(2.5f,5.f), 5.f, 0.f, NULL, NULL, NULL, 1.f, 1.f, "suck.mp3", NULL, VEC2(2.f,5.f), VEC2(0,0), 0.f, NULL),
	Spell(7, "drain2", Spell::Ray, Spell::Drain|Spell::Hold, 80, 1, VEC2(3.5f,6.f), 6.f, 0.f, NULL, NULL, NULL, 1.f, 1.f, "suck.mp3", NULL, VEC2(2.f,5.f), VEC2(0,0), 0.f, NULL),
	Spell(8, "exploding_skull", Spell::Point, Spell::Explode, 120, 2, VEC2(5.f,10.f), 40.f, 7.5f, NULL, "flare.png", "explosion.jpg", 0.13f, 0.075f, "evil_laught.mp3", "explode.mp3", VEC2(3,8), VEC2(4,12), 2.5f, "czaszka.qmsh"),
	Spell(9, "heal", Spell::Target, Spell::Heal|Spell::NonCombat, 100, 10, VEC2(10.f,15.f), 15.f, 0.f, NULL, NULL, "heal.png", 1.f, 0.03f, "heal.ogg", NULL, VEC2(2,5), VEC2(0,0), 0.f, NULL),
	Spell(10, "mistic_ball", Spell::Point, Spell::Explode, 200, 0, VEC2(10.f,10.f), 50.f, 18.f, "flare2.png", "flare2.png", "rainbow.jpg", 0.2f, 0.075f, "rlaunch.mp3", "explode.mp3", VEC2(2,8), VEC2(4,15), 3.f, NULL)
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
	K_TEX_OTHER,
	K_SOUND_CAST,
	K_SOUND_HIT
};

//=================================================================================================
void LoadSpells(uint& out_crc)
{
	out_crc = 0;

	Tokenizer t;

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
		{ "tex_other", K_TEX_OTHER },
		{ "sound_cast", K_SOUND_CAST },
		{ "sound_hit", K_SOUND_HIT }
	});

	t.AddKeywords(G_TYPE, {
		{ "point", Spell::Point },
		{ "ray", Spell::Ray },
		{ "target", Spell::Target },
		{ "ball", Spell::Ball }
	});

	t.AddKeywords(G_FLAGS, {
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
}
