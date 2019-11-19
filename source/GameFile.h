#pragma once

//-----------------------------------------------------------------------------
#include "Item.h"
#include "Unit.h"
#include "UnitGroup.h"

//-----------------------------------------------------------------------------
class GameReader final : public FileReader
{
public:
	explicit GameReader(HANDLE file) : FileReader(file) {}
	explicit GameReader(cstring filename) : FileReader(filename) {}
	explicit GameReader(const FileReader& f) : FileReader(f.GetHandle()) {}

	using FileReader::operator >>;

	void operator >> (Unit*& unit)
	{
		int id = Read<int>();
		unit = Unit::GetById(id);
	}

	void operator >> (SmartPtr<Unit>& unit)
	{
		int id = Read<int>();
		unit = Unit::GetById(id);
	}

	void operator >> (HumanData& hd)
	{
		hd.Load(*this);
	}

	void operator >> (const Item*& item)
	{
		const string& id = ReadString1();
		if(IsOk())
			item = Item::Get(id);
	}

	void operator >> (Usable*& usable)
	{
		int id = Read<int>();
		usable = Usable::GetById(id);
	}

	void ReadOptional(const Item*& item)
	{
		const string& id = ReadString1();
		if(id.empty())
			item = nullptr;
		else
			item = Item::Get(id);
	}

	void operator >> (UnitGroup*& group);

	bool is_local;
};

//-----------------------------------------------------------------------------
class GameWriter final : public FileWriter
{
public:
	explicit GameWriter(HANDLE file) : FileWriter(file)
	{
	}

	explicit GameWriter(cstring filename) : FileWriter(filename)
	{
	}

	using FileWriter::operator <<;

	void operator << (Unit* u)
	{
		int id = (u ? u->id : -1);
		FileWriter::operator << (id);
	}

	void operator << (const SmartPtr<Unit>& u)
	{
		int id = (u ? u->id : -1);
		FileWriter::operator << (id);
	}

	void operator << (const HumanData& hd)
	{
		hd.Save(*this);
	}

	void operator << (const Item* item)
	{
		if(item != nullptr)
			WriteString1(item->id);
		else
			Write<byte>(0);
	}

	void WriteOptional(const Item* item)
	{
		if(item)
			WriteString1(item->id);
		else
			Write0();
	}

	void operator << (UnitGroup* group)
	{
		WriteString1(group->id);
	}

	bool is_local;
};
