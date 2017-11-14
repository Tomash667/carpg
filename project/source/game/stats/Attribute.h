#pragma once

//-----------------------------------------------------------------------------
enum class Attribute
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
struct AttributeInfo
{
	Attribute attrib_id;
	cstring id;
	string name, desc;

	static const int MAX = 255;

	AttributeInfo(Attribute attrib_id, cstring id) : attrib_id(attrib_id), id(id)
	{
	}

	static AttributeInfo attributes[(int)Attribute::MAX];
	static AttributeInfo* Find(const AnyString& id);
	static void Validate(uint& err);
	static float GetModifier(int base, int& mod);
};
