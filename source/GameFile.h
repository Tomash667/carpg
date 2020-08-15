#pragma once

//-----------------------------------------------------------------------------
#include "Item.h"
#include "Location.h"
#include "Unit.h"
#include "UnitGroup.h"

//-----------------------------------------------------------------------------
class GameReader final : public FileReader
{
public:
	explicit GameReader(HANDLE file) : FileReader(file), isLocal(false) {}
	explicit GameReader(cstring filename) : FileReader(filename), isLocal(false) {}
	explicit GameReader(const FileReader& f) : FileReader(f.GetHandle()), isLocal(false) {}

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
	void operator >> (Location*& loc);

	bool isLocal;
};

//-----------------------------------------------------------------------------
class GameWriter final : public FileWriter
{
public:
	explicit GameWriter(HANDLE file) : FileWriter(file), isLocal(false)
	{
	}

	explicit GameWriter(cstring filename) : FileWriter(filename), isLocal(false)
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

	void operator << (Location* loc)
	{
		int index = loc ? loc->index : -1;
		Write(index);
	}

	bool isLocal;
};
