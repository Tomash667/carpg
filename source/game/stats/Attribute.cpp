#include "Pch.h"
#include "Base.h"
#include "Attribute.h"

// attribute gain table
struct StatGain
{
	int base;
	float value;
};

StatGain gain[] = {
	25, 8.f, // 75
	20, 7.5f, // 70
	15, 7.f, // 65
	10, 6.5f, // 60
	5, 6.0f, // 55
	0, 5.f, // 50
	-5, 4.5f, // 45
	-10, 3.5f, // 40
	-15, 2.5f, // 35
	-20, 1.5f, // 30
	-25, 0.5f, // 25
	-30, 0, // 20
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
float AttributeInfo::GetModifier(int base)
{
	assert(base % 5 == 0);
	if(base <= -30)
		return 0.f;
	if(base > 25)
		return 8.f;
	for(StatGain& sg : gain)
	{
		if(sg.base == base)
			return sg.value;
	}
	return 0.f;
}
