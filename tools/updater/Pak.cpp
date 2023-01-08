#include <CarpgLibCore.h>
#include "Pak.h"

bool Pak::Open(Cstring filename)
{
	// open file
	if(!file.Open(filename))
		return false;

	// read header
	Pak::Header header;
	file >> header;
	if(!file || !header.HaveValidSign() || header.version != Pak::CURRENT_VERSION)
		return false;

	// read file list
	table = new byte[header.file_entry_table_size];
	file.Read(table, header.file_entry_table_size);
	if(!file)
		return false;

	// verify file list, set name
	file_count = header.file_count;
	uint total_size = file.GetSize();
	for(uint i = 0; i < file_count; ++i)
	{
		File& f = file_table[i];
		f.filename = (cstring)table + f.filename_offset;
		if(f.offset + f.compressed_size > total_size)
			return false;
	}

	return true;
}

bool Pak::HaveFile(Cstring filename)
{
	for(uint i = 0; i < file_count; ++i)
	{
		if(strcmp(file_table[i].filename, filename) == 0)
			return true;
	}
	return false;
}

bool Pak::Extract()
{
	bool ok = true;
	string dir;
	for(uint i = 0; i < file_count; ++i)
	{
		// read
		Pak::File& f = file_table[i];
		file.SetPos(f.offset);
		Buffer* buf = file.ReadToBuffer(f.compressed_size);

		// decompress
		if(f.compressed_size != f.size)
			buf = buf->Decompress(f.size);

		// save
		dir = io::PathToDirectory(f.filename);
		if(!dir.empty())
			io::CreateDirectories(dir);
		if(!FileWriter::WriteAll(f.filename, buf))
		{
			printf("ERROR: Failed to replace file '%s'.\n", f.filename);
			ok = false;
		}
		buf->Free();
	}
	return ok;
}
