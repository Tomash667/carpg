#pragma once

#include "SaveState.h"

class GameFile : public File
{
public:
	GameFile()
	{
	}

	GameFile(HANDLE file) : File(file)
	{
	}

	GameFile(cstring filename, Mode mode = READ) : File(filename, mode)
	{
	}

	using File::operator <<;
	using File::operator >>;

	inline void LoadArtifact(const Item*& item)
	{
		if(LOAD_VERSION >= V_DEVEL)
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

	// unit
	inline void operator << (Unit* u)
	{
		int refid = (u ? u->refid : -1);
		File::operator << (refid);
	}

	inline bool operator >> (Unit*& u)
	{
		int refid;
		bool result = (File::operator >> (refid));
		u = Unit::GetByRefid(refid);
		return result;
	}

	// human data
	inline void operator << (const HumanData& hd)
	{
		hd.Save(file);
	}

	inline void operator >> (HumanData& hd)
	{
		hd.Load(file);
	}

	// item
	inline void operator << (const Item* item)
	{
		if(item != NULL)
			WriteString1(item->id);
		else
			Write<byte>(0);
	}

	inline void operator >> (const Item*& item)
	{
		if(ReadStringBUF())
			item = FindItem(BUF);
	}
};
