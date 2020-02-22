#include "Pch.h"
#include "Attribute.h"

//-----------------------------------------------------------------------------
// All attributes
Attribute Attribute::attributes[(int)AttributeId::MAX] = {
	Attribute(AttributeId::STR, "str"),
	Attribute(AttributeId::END, "end"),
	Attribute(AttributeId::DEX, "dex"),
	Attribute(AttributeId::INT, "int"),
	Attribute(AttributeId::WIS, "wis"),
	Attribute(AttributeId::CHA, "cha")
};

//=================================================================================================
Attribute* Attribute::Find(Cstring id)
{
	for(Attribute& ai : attributes)
	{
		if(id == ai.id)
			return &ai;
	}

	return nullptr;
}

//=================================================================================================
void Attribute::Validate(uint& err)
{
	for(int i = 0; i < (int)AttributeId::MAX; ++i)
	{
		Attribute& ai = attributes[i];
		if(ai.attrib_id != (AttributeId)i)
		{
			Warn("Test: AttributeId %s: id mismatch.", ai.id);
			++err;
		}
		if(ai.name.empty())
		{
			Warn("Test: AttributeId %s: empty name.", ai.id);
			++err;
		}
		if(ai.desc.empty())
		{
			Warn("Test: AttributeId %s: empty desc.", ai.id);
			++err;
		}
	}
}

//=================================================================================================
float Attribute::GetModifier(int base)
{
	assert(base % 5 == 0);
	return float(base - 10) / 10;
}
