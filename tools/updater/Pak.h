#pragma once

#include <File.h>

struct Pak
{
	static const byte CURRENT_VERSION = 1;

	struct Header
	{
		char sign[3];
		byte version;
		int flags;
		uint file_count, file_entry_table_size;

		bool HaveValidSign()
		{
			return sign[0] == 'P' && sign[1] == 'A' && sign[2] == 'K';
		}
	};

	struct File
	{
		union
		{
			cstring filename;
			uint filename_offset;
		};
		uint size;
		uint compressed_size;
		uint offset;
	};

	enum Flags
	{
		F_ENCRYPTION = 1 << 0,
		F_FULL_ENCRYPTION = 1 << 1
	};

	FileReader file;
	union
	{
		byte* table;
		File* file_table;
	};
	uint file_count;
	bool encrypted;

	Pak() : table(nullptr) {}
	~Pak() { delete[] table; }
	bool Open(Cstring filename);
	void Extract();
};
