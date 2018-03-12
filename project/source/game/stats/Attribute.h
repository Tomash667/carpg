#pragma once

//-----------------------------------------------------------------------------
enum class AttributeId
{
	STR,
	END,
	DEX,
	INT,
	WIS,
	CHA,
	MAX,
	NONE
};

//-----------------------------------------------------------------------------
struct Attribute
{
	AttributeId attrib_id;
	cstring id;
	string name, desc;

	static const int MIN = 1;
	static const int MAX = 255;

	Attribute(AttributeId attrib_id, cstring id) : attrib_id(attrib_id), id(id)
	{
	}

	static Attribute attributes[(int)AttributeId::MAX];
	static Attribute* Find(const AnyString& id);
	static void Validate(uint& err);
	static float GetModifier(int base, int& mod);
};
