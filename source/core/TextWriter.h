#pragma once

class TextWriter
{
public:
	TextWriter(cstring filename)
	{
		file = CreateFile(filename, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	}

	~TextWriter()
	{
		if(file != INVALID_HANDLE_VALUE)
			CloseHandle(file);
	}

	operator bool () const
	{
		return file != INVALID_HANDLE_VALUE;
	}

	void operator << (const string& str)
	{
		WriteFile(file, str.c_str(), str.length(), &tmp, nullptr);
	}

	void operator << (cstring str)
	{
		WriteFile(file, str, strlen(str), &tmp, nullptr);
	}

	HANDLE file;
	DWORD tmp;
};
