#pragma once

#include "StatProfile.h"

struct StatsX
{
	struct Entry
	{
		StatProfile* profile;
		union Seed
		{
			struct
			{
				byte subprofile;
				byte variant;
				byte level;
			};
			uint value;
		} seed;
		StatsX* stats;

		bool operator () (const Entry& e1, const Entry& e2) const
		{
			if(e1.profile > e2.profile)
				return true;
			else if(e1.profile == e2.profile)
				return e1.seed.value > e2.seed.value;
			else
				return false;
		}
	};

	StatProfile* profile;
	SubProfile* subprofile;
	Skill weapon, armor;
	int attrib[(int)Attribute::MAX];
	int attrib_base[(int)Attribute::MAX];
	int attrib_apt[(int)Attribute::MAX];
	int skill[(int)Skill::MAX];
	int skill_base[(int)Skill::MAX];
	int skill_apt[(int)Skill::MAX];
	float skill_tag[(int)Skill::MAX];
	Entry::Seed seed;
	bool unique;

	void ApplyBase(StatProfile* profile);
	void Upgrade();
	float CalculateLevel();
	void Save(FileWriter& f);
	void Write(BitStream& stream);
	bool Read(BitStream& stream);
	StatsX* GetLevelUp();
	int Get(Attribute a) const
	{
		return attrib[(int)a];
	}
	int Get(Skill s) const
	{
		return skill[(int)s];
	}
	void Set(Attribute a, int value)
	{
		assert(unique);
		attrib[(int)a] = value;
	}
	void Set(Skill s, int value)
	{
		assert(unique);
		skill[(int)s] = value;
	}

	static StatsX* GetRandom(StatProfile* profile, int level);
	static StatsX* Get(Entry& e);
	static StatsX* Load(FileReader& f);
	static void Cleanup();
};
