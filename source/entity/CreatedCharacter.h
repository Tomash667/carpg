// character stats used for character creation
#pragma once

//-----------------------------------------------------------------------------
#include "StatProfile.h"
#include "Perk.h"
#include "Attribute.h"
#include "Skill.h"
#include "Class.h"
#include "HumanData.h"

//-----------------------------------------------------------------------------
struct CreatedCharacter
{
	struct AttributeData
	{
		int value;
		bool mod;

		void Mod(int v, bool _mod)
		{
			assert(_mod ? !mod : mod);
			value += v;
			mod = _mod;
		}
	};

	struct SkillData
	{
		int value, base;
		bool add;
		bool mod;

		void Add(int v, bool _add)
		{
			assert(_add ? !add : add);
			value += v;
			add = _add;
		}

		void Mod(int v, bool _mod)
		{
			assert(_mod ? !mod : mod);
			value += v;
			mod = _mod;
		}
	};

	Class* clas;
	vector<TakenPerk> takenPerks;
	AttributeData a[(int)AttributeId::MAX];
	SkillData s[(int)SkillId::MAX];
	int sp, spMax, perks, perksMax;
	bool updateSkills;
	vector<SkillId> toUpdate;
	SubprofileInfo lastSub;

	CreatedCharacter() { lastSub.value = 0; }
	void Clear(Class* clas);
	void Random(Class* clas);
	void Write(BitStreamWriter& f) const;
	// 0 - ok, 1 - read error, 2 - value error, 3 - validation error
	int Read(BitStreamReader& f);
	bool HavePerk(Perk* perk) const;
	void GetStartingItems(array<const Item*, SLOT_MAX>& items);
	int GetItemLevel(int level, bool poor);
	int Get(AttributeId attrib) const { return a[(int)attrib].value; }
	int Get(SkillId sk) const { return s[(int)sk].value; }
};

//-----------------------------------------------------------------------------
void WriteCharacterData(BitStreamWriter& f, Class* clas, const HumanData& hd, const CreatedCharacter& cc);
// 0 - ok, 1 - read error, 2 - value error, 3 - validation error
int ReadCharacterData(BitStreamReader& f, Class*& clas, HumanData& hd, CreatedCharacter& cc);
