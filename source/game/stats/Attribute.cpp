#include "Pch.h"
#include "Base.h"
#include "Attribute.h"

//-----------------------------------------------------------------------------
// attribute gain table
struct StatGain
{
	float value; // attribute gain per 5 levels
	int weight; // weight for level calculation
};

//-----------------------------------------------------------------------------
// Skill gain table - used for calculating player level and values for specific level based od skills at 0 level.
// For example warrior has 65 str at 0 level which is 15 base bonus. At level 10 he should have 77 str: 10
// Even idiot with 35 base int gains some int, otherwise weaklings will be always stuck with small str. But in other way barbarian
// don't need that int/wis/cha.
static StatGain gain[] = {
	1.25f,	0,	// -5
	1.5f,	0,	// -4
	1.75f,	0,	// -3
	2.f,	0,	// -2
	2.25f,	0,	// -1
	2.5f,	0,	// 0
	2.75f,	0,	// 1
	3.f,	0,	// 2
	3.25f,	0,	// 3
	3.5f,	0,	// 4
	3.75f,	1,	// 5
	4.f,	1,	// 6
	4.25f,	1,	// 7
	4.5f,	1,	// 8
	4.75f,	1,	// 9
	5.f,	2,	// 10
	5.25f,	2,	// 11
	5.5f,	2,	// 12
	5.75f,	2,	// 13
	6.f,	2,	// 14
	6.25f,	3,	// 15
	6.5f,	3,	// 16
	6.75f,	3,	// 17
	7.f,	3,	// 18
	7.25f,	3,	// 19
	7.5f,	4,	// 20
	7.75f,	4,	// 21
	8.f,	4,	// 22
	8.25f,	4,	// 23
	8.5f,	4,	// 24
	8.75f,	5,	// 25
};

//-----------------------------------------------------------------------------
// All attributes
AttributeInfo g_attributes[(int)Attribute::MAX] = {
	AttributeInfo(Attribute::STR, "str"),
	AttributeInfo(Attribute::END, "end"),
	AttributeInfo(Attribute::DEX, "dex"),
	AttributeInfo(Attribute::INT, "int"),
	AttributeInfo(Attribute::WIS, "wis"),
	AttributeInfo(Attribute::CHA, "cha")
};

//=================================================================================================
AttributeInfo* AttributeInfo::Find(const string& id)
{
	for(AttributeInfo& ai : g_attributes)
	{
		if(id == ai.id)
			return &ai;
	}

	return nullptr;
}

//=================================================================================================
void AttributeInfo::Validate(uint& err)
{
	for(int i = 0; i<(int)Attribute::MAX; ++i)
	{
		AttributeInfo& ai = g_attributes[i];
		if(ai.attrib_id != (Attribute)i)
		{
			WARN(Format("Test: Attribute %s: id mismatch.", ai.id));
			++err;
		}
		if(ai.name.empty())
		{
			WARN(Format("Test: Attribute %s: empty name.", ai.id));
			++err;
		}
		if(ai.desc.empty())
		{
			WARN(Format("Test: Attribute %s: empty desc.", ai.id));
			++err;
		}
	}
}

//=================================================================================================
float AttributeInfo::GetModifier(int base, int& weight)
{
	if(base <= -5)
	{
		weight = 0;
		return 1.25f;
	}
	else if(base >= 25)
	{
		weight = 5;
		return 8.75f;
	}
	else
	{
		StatGain& sg = gain[base+5];
		weight = sg.weight;
		return sg.value;
	}
}
