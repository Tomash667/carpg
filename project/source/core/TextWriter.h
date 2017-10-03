#pragma once

class TextWriter
{
public:
	TextWriter(cstring filename) : file(filename)
	{
	}

	operator bool() const
	{
		return file.IsOpen();
	}

	void operator << (const string& str)
	{
		file.Write(str.c_str(), str.length());
	}

	void operator << (cstring str)
	{
		file.Write(str, strlen(str));
	}

	void operator << (char c)
	{
		file << c;
	}

	struct Flag
	{
		int value;
		cstring name;
	};

	struct FlagsGroup
	{
		int flags;
		std::initializer_list<Flag> const& flags_def;
	};

	void WriteFlags(std::initializer_list<FlagsGroup> const& flag_groups);

	io2::FileWriter file;
};
