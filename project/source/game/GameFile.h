#pragma once

#include "Item.h"
#include "Unit.h"

//-----------------------------------------------------------------------------
class GameReader : public FileReaderBase
{
public:
	explicit GameReader(HANDLE file) : FileReaderBase(file)
	{
	}

	explicit GameReader(cstring filename) : FileReaderBase(filename)
	{
	}

	using FileReaderBase::operator >> ;

	void LoadArtifact(const Item*& item);

	bool operator >> (Unit*& u)
	{
		int refid;
		FileReaderBase::operator >> (refid);
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
class GameWriter : public FileWriterBase
{
public:
	explicit GameWriter(HANDLE file) : FileWriterBase(file)
	{
	}

	explicit GameWriter(cstring filename) : FileWriterBase(filename)
	{
	}

	using FileWriterBase::operator <<;

	void operator << (Unit* u)
	{
		int refid = (u ? u->refid : -1);
		FileWriterBase::operator << (refid);
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
