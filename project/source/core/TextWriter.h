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

	io2::FileWriter file;
};
