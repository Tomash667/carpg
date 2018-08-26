#pragma once

#include "Item.h"
#include "Unit.h"

//-----------------------------------------------------------------------------
class GameReader : public FileReader
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

	bool operator >> (Unit*& u)
	{
		int refid;
		FileReader::operator >> (refid);
		u = Unit::GetByRefid(refid);
		return IsOk();
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
};

//-----------------------------------------------------------------------------
class GameWriter : public FileWriter
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
