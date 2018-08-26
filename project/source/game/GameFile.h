#pragma once

#include "Item.h"
#include "Unit.h"

//-----------------------------------------------------------------------------
class GameReader final : public FileReader
{
public:
	explicit GameReader(HANDLE file) : FileReader(file)
	{
	}

	explicit GameReader(cstring filename) : FileReader(filename)
	{
	}

	using FileReader::operator >> ;

	void LoadArtifact(const Item*& item);

	void operator >> (Unit*& unit)
	{
		int refid = Read<int>();
		unit = Unit::GetByRefid(refid);
	}

	void operator >> (HumanData& hd)
	{
		hd.Load((HWND)file);
	}

	void operator >> (const Item*& item)
	{
		const string& id = ReadString1();
		if(IsOk())
			item = Item::Get(id);
	}

	void operator >> (Usable*& usable)
	{
		int refid = Read<int>();
		usable = Usable::GetByRefid(refid);
	}

	const Item* ReadItemOptional()
	{
		const string& id = ReadString1();
		if(id.empty())
			return nullptr;
		else
			return Item::Get(id);
	}
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
		int refid = (u ? u->refid : -1);
		FileWriter::operator << (refid);
	}

	void operator << (const HumanData& hd)
	{
		hd.Save(file);
	}

	void operator << (const Item* item)
	{
		if(item != nullptr)
			WriteString1(item->id);
		else
			Write<byte>(0);
	}
};
