#include "Pch.h"
#include "GameCore.h"
#include "PerkLoader.h"
#include "Perk.h"
#include "Attribute.h"
#include "Skill.h"
#include "Effect.h"

enum Group
{
	G_TOP,
	G_KEYWORD,
	G_FLAG,
	G_ATTRIBUTE,
	G_SKILL,
	G_EFFECT,
	G_SPECIAL_REQUIRED,
	G_SPECIAL_EFFECT
};

enum Keyword
{
	K_FLAGS,
	K_REQUIRED,
	K_EFFECT,
	K_COST,
	K_PARENT
};

enum SpecialRequired
{
	SR_NO_PERK,
	SR_CAN_MOD
};

enum SpecialEffect
{
	SE_APTITUDE,
	SE_PICKED_ATTRIBUTE,
	SE_PICKED_SKILL,
	SE_SKILL_POINT,
	SE_GOLD
};

//=================================================================================================
void PerkLoader::DoLoading()
{
	Load("perks.txt", G_TOP);
}

//=================================================================================================
void PerkLoader::Cleanup()
{
	DeleteElements(Perk::perks);
}

//=================================================================================================
void PerkLoader::InitTokenizer()
{
	t.AddKeyword("perk", 0, G_TOP);

	t.AddKeywords(G_KEYWORD, {
		{ "flags", K_FLAGS },
		{ "required", K_REQUIRED },
		{ "effect", K_EFFECT },
		{ "cost", K_COST },
		{ "parent", K_PARENT }
		});

	t.AddKeywords(G_FLAG, {
		{ "flaw", Perk::Flaw },
		{ "start", Perk::Start },
		{ "history", Perk::History }
		});

	for(int i = 0; i < (int)AttributeId::MAX; ++i)
		t.AddKeyword(Attribute::attributes[i].id, i, G_ATTRIBUTE);

	for(int i = 0; i < (int)SkillId::MAX; ++i)
		t.AddKeyword(Skill::skills[i].id, i, G_SKILL);

	for(int i = 0; i < (int)EffectId::Max; ++i)
		t.AddKeyword(EffectInfo::effects[i].id, i, G_EFFECT);

	t.AddKeywords(G_SPECIAL_REQUIRED, {
		{ "no_perk", SR_NO_PERK },
		{ "can_mod", SR_CAN_MOD }
		});

	t.AddKeywords(G_SPECIAL_EFFECT, {
		{ "aptitude", SE_APTITUDE },
		{ "picked_attribute", SE_PICKED_ATTRIBUTE },
		{ "picked_skill", SE_PICKED_SKILL },
		{ "skill_point", SE_SKILL_POINT },
		{ "gold", SE_GOLD }
		});
}

//=================================================================================================
void PerkLoader::LoadEntity(int, const string& id)
{
	const int hash = Hash(id);
	Perk* existing_perk = Perk::Get(hash);
	if(existing_perk && existing_perk->defined)
	{
		if(existing_perk->id == id)
			t.Throw("Id must be unique.");
		else
			t.Throw("Id hash collision.");
	}

	Ptr<Perk> perk;
	perk->hash = hash;
	perk->id = id;
	perk->defined = true;
	crc.Update(id);
	t.Next();

	t.AssertSymbol('{');
	t.Next();

	while(!t.IsSymbol('}'))
	{
		const Keyword k = (Keyword)t.MustGetKeywordId(G_KEYWORD);
		t.Next();

		switch(k)
		{
		case K_FLAGS:
			t.ParseFlags(G_FLAG, perk->flags);
			crc.Update(perk->flags);
			t.Next();
			break;
		case K_REQUIRED:
			t.AssertSymbol('{');
			t.Next();
			while(!t.IsSymbol('}'))
			{
				if(t.IsKeyword(0, G_TOP))
				{
					t.Next();
					const int perk_hash = ParsePerkId()->hash;
					perk->required.push_back({ Perk::Required::HAVE_PERK, perk_hash, 0 });
					crc.Update(Perk::Required::HAVE_PERK);
					crc.Update(perk_hash);
				}
				else if(t.IsKeywordGroup(G_SPECIAL_REQUIRED))
				{
					SpecialRequired special = (SpecialRequired)t.GetKeywordId(G_SPECIAL_REQUIRED);
					t.Next();
					if(special == SR_NO_PERK)
					{
						const int perk_hash = ParsePerkId()->hash;
						perk->required.push_back({ Perk::Required::HAVE_NO_PERK, perk_hash, 0 });
						crc.Update(Perk::Required::HAVE_NO_PERK);
						crc.Update(perk_hash);
					}
					else
					{
						const int subtype = t.GetKeywordId(G_ATTRIBUTE);
						t.Next();
						perk->required.push_back({ Perk::Required::CAN_MOD, subtype, 0 });
						crc.Update(Perk::Required::CAN_MOD);
						crc.Update(subtype);
					}
				}
				else if(t.IsKeywordGroup(G_ATTRIBUTE))
				{
					const int subtype = t.GetKeywordId(G_ATTRIBUTE);
					t.Next();
					const int value = t.MustGetInt();
					t.Next();
					perk->required.push_back({ Perk::Required::HAVE_ATTRIBUTE, subtype, value });
					crc.Update(Perk::Required::HAVE_ATTRIBUTE);
					crc.Update(subtype);
					crc.Update(value);
				}
				else if(t.IsKeywordGroup(G_SKILL))
				{
					const int subtype = t.GetKeywordId(G_SKILL);
					t.Next();
					const int value = t.MustGetInt();
					t.Next();
					perk->required.push_back({ Perk::Required::HAVE_SKILL, subtype, value });
					crc.Update(Perk::Required::HAVE_SKILL);
					crc.Update(subtype);
					crc.Update(value);
				}
				else
					t.Unexpected();
			}
			t.Next();
			break;
		case K_EFFECT:
			t.AssertSymbol('{');
			t.Next();
			while(!t.IsSymbol('}'))
			{
				if(t.IsKeywordGroup(G_ATTRIBUTE))
				{
					const int subtype = t.GetKeywordId(G_ATTRIBUTE);
					t.Next();
					const int value = t.MustGetInt();
					t.Next();
					perk->effects.push_back({ Perk::Effect::ATTRIBUTE, subtype, value });
					crc.Update(Perk::Effect::ATTRIBUTE);
					crc.Update(subtype);
					crc.Update(value);
				}
				else if(t.IsKeywordGroup(G_SKILL))
				{
					const int subtype = t.GetKeywordId(G_SKILL);
					t.Next();
					const int value = t.MustGetInt();
					t.Next();
					perk->effects.push_back({ Perk::Effect::SKILL, subtype, value });
					crc.Update(Perk::Effect::SKILL);
					crc.Update(subtype);
					crc.Update(value);
				}
				else if(t.IsKeywordGroup(G_EFFECT))
				{
					Perk::Effect e;
					e.type = Perk::Effect::EFFECT;
					e.subtype = t.GetKeywordId(G_EFFECT);
					t.Next();
					e.valuef = t.MustGetFloat();
					t.Next();
					perk->effects.push_back(e);
					crc.Update(Perk::Effect::EFFECT);
					crc.Update(e.subtype);
					crc.Update(e.valuef);
				}
				else if(t.IsKeywordGroup(G_SPECIAL_EFFECT))
				{
					Perk::Effect e;
					SpecialEffect special = (SpecialEffect)t.GetKeywordId(G_SPECIAL_EFFECT);
					switch(special)
					{
					case SE_APTITUDE:
						e.type = Perk::Effect::APTITUDE;
						e.subtype = 0;
						break;
					case SE_PICKED_ATTRIBUTE:
						if(perk->value_type != Perk::None)
							t.Throw("Pickable value type already set.");
						perk->value_type = Perk::Attribute;
						e.type = Perk::Effect::ATTRIBUTE;
						e.subtype = (int)AttributeId::NONE;
						break;
					case SE_PICKED_SKILL:
						if(perk->value_type != Perk::None)
							t.Throw("Pickable value type already set.");
						perk->value_type = Perk::Skill;
						e.type = Perk::Effect::SKILL;
						e.subtype = (int)SkillId::NONE;
						break;
					case SE_SKILL_POINT:
						e.type = Perk::Effect::SKILL_POINT;
						e.subtype = 0;
						break;
					case SE_GOLD:
						e.type = Perk::Effect::GOLD;
						e.subtype = 0;
						break;
					}
					t.Next();
					e.value = t.MustGetInt();
					t.Next();
					perk->effects.push_back(e);
					crc.Update(e.type);
					crc.Update(e.subtype);
					crc.Update(e.value);
				}
				else
					t.Unexpected();
			}
			t.Next();
			break;
		case K_COST:
			perk->cost = t.MustGetUint();
			crc.Update(perk->cost);
			t.Next();
			break;
		case K_PARENT:
			{
				Perk* parent = ParsePerkId();
				assert(parent->parent == 0); // YAGNI
				perk->parent = parent->hash;
				crc.Update(parent->hash);
			}
			break;
		}
	}

	if(existing_perk)
	{
		RemoveElement(Perk::perks, existing_perk);
		delete existing_perk;
	}
	Perk::hash_perks[hash] = perk;
	Perk::perks.push_back(perk.Pin());
}

//=================================================================================================
Perk* PerkLoader::ParsePerkId()
{
	const string& id = t.MustGetItemKeyword();
	int hash = Hash(id);
	Perk* perk = Perk::Get(hash);
	if(!perk)
	{
		// forward declaration
		perk = new Perk;
		perk->hash = hash;
		perk->id = id;
		Perk::hash_perks[hash] = perk;
		Perk::perks.push_back(perk);
	}
	t.Next();
	return perk;
}

//=================================================================================================
void PerkLoader::Finalize()
{
	for(Perk* perk : Perk::perks)
	{
		if(!perk->defined)
			LoadError("Perk '%s' not defined.", perk->id.c_str());
	}

	content.crc[(int)Content::Id::Perks] = crc.Get();

	Info("Loaded perks (%u) - crc %p.", Perk::perks.size(), crc.Get());
}
