#include "Pch.h"
#include "Base.h"
#include "Attribute.h"

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
