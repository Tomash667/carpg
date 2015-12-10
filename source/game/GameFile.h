#pragma once

//-----------------------------------------------------------------------------
#include "SaveState.h"

//-----------------------------------------------------------------------------
class GameReader : public FileReader
{
public:
	explicit inline GameReader(HANDLE file) : FileReader(file)
	{
	}

	explicit inline GameReader(cstring filename) : FileReader(filename)
	{
	}

	using FileReader::operator >>;

	inline void LoadArtifact(const Item*& item)
	{
		if(LOAD_VERSION >= V_0_4)
			operator >> (item);
		else
		{
			int what;
			cstring id;
			operator >> (what);

			switch(what)
			{
			default:
			case 0: id = "a_amulet"; break;
			case 1: id = "a_amulet2"; break;
			case 2: id = "a_amulet3"; break;
			case 3: id = "a_brosza"; break;
			case 4: id = "a_butelka"; break;
			case 5: id = "a_cos"; break;
			case 6: id = "a_czaszka"; break;
			case 7: id = "a_figurka"; break;
			case 8: id = "a_figurka2"; break;
			case 9: id = "a_figurka3"; break;
			case 10: id = "a_ksiega"; break;
			case 11: id = "a_maska"; break;
			case 12: id = "a_maska2"; break;
			case 13: id = "a_misa"; break;
			case 14: id = "a_moneta"; break;
			case 15: id = "a_pierscien"; break;
			case 16: id = "a_pierscien2"; break;
			case 17: id = "a_pierscien3"; break;
			case 18: id = "a_talizman"; break;
			case 19: id = "a_talizman2"; break;
			}

			item = FindItem(id);
		}
	}

	inline bool operator >> (Unit*& u)
	{
		int refid;
		bool result = (FileReader::operator >> (refid));
		u = Unit::GetByRefid(refid);
		return result;
	}

	inline void operator >> (HumanData& hd)
	{
		hd.Load(file);
	}

	inline void operator >> (const Item*& item)
	{
		if(ReadStringBUF())
			item = FindItem(BUF);
	}
};

//-----------------------------------------------------------------------------
class GameWriter : public FileWriter
{
public:
	explicit inline GameWriter(HANDLE file) : FileWriter(file)
	{
	}

	explicit inline GameWriter(cstring filename) : FileWriter(filename)
	{
	}

	using FileWriter::operator <<;

	inline void operator << (Unit* u)
	{
		int refid = (u ? u->refid : -1);
		FileWriter::operator << (refid);
	}

	inline void operator << (const HumanData& hd)
	{
		hd.Save(file);
	}

	inline void operator << (const Item* item)
	{
		if(item != nullptr)
			WriteString1(item->id);
		else
			Write<byte>(0);
	}
};
