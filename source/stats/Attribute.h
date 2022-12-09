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
// Unit attribute
struct Attribute
{
	AttributeId attribId;
	cstring id;
	string name, desc;

	static const int MIN = 1;
	static const int MAX = 255;

	Attribute(AttributeId attribId, cstring id) : attribId(attribId), id(id)
	{
	}

	static Attribute attributes[(int)AttributeId::MAX];
	static Attribute* Find(Cstring id);
	static void Validate(uint& err);
	static float GetModifier(int base);
};
