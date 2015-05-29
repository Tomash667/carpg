#include "Pch.h"
#include "Base.h"
#include "Attribute.h"

//-----------------------------------------------------------------------------
// attribute gain table
struct StatGain
{
	int base;
	float value;
	int weight;
};

//-----------------------------------------------------------------------------
static StatGain gain[] = {
	25, 8.f, 5, // 75
	20, 7.5f, 4, // 70
	15, 7.f, 3, // 65
	10, 6.5f, 2, // 60
	5, 6.0f, 1, // 55
	0, 5.f, 0, // 50
	-5, 4.5f, 0, // 45
	-10, 3.5f, 0, // 40
	-15, 2.5f, 0, // 35
	-20, 1.5f, 0, // 30
	-25, 0.5f, 0, // 25
	-30, 0, 0, // 20
};

//-----------------------------------------------------------------------------
AttributeInfo g_attributes[(int)Attribute::MAX] = {
	AttributeInfo(Attribute::STR, "str"),
	AttributeInfo(Attribute::CON, "con"),
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

	return NULL;
}

//=================================================================================================
void AttributeInfo::Validate(int& err)
{
	for(int i = 0; i<(int)Attribute::MAX; ++i)
	{
		AttributeInfo& ai = g_attributes[i];
		if(ai.attrib_id != (Attribute)i)
		{
			WARN(Format("Attribute %s: id mismatch.", ai.id));
			++err;
		}
		if(ai.name.empty())
		{
			WARN(Format("Attribute %s: empty name.", ai.id));
			++err;
		}
		if(ai.desc.empty())
		{
			WARN(Format("Attribute %s: empty desc.", ai.id));
			++err;
		}
	}
}

//=================================================================================================
float AttributeInfo::GetModifier(int base, int& weight)
{
	assert(base % 5 == 0);
	if(base <= -30)
	{
		weight = 0;
		return 0.f;
	}
	if(base > 25)
	{
		StatGain& sg = gain[0];
		weight = sg.weight;
		return sg.value;
	}
	for(StatGain& sg : gain)
	{
		if(sg.base == base)
		{
			weight = sg.weight;
			return sg.value;
		}
	}
	weight = 0;
	return 0.f;
}
