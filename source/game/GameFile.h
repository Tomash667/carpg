#pragma once

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
};
