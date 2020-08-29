#include <CarpgLibCore.h>
#include "Pak.h"
#include <zlib.h>

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

void Pak::Extract()
{
	string dir;
	Buffer* buf = Buffer::Get();
	Buffer* decompressed = Buffer::Get();
	for(uint i = 0; i < file_count; ++i)
	{
		// read
		Pak::File& f = file_table[i];
		buf->Resize(f.compressed_size);
		file.SetPos(f.offset);
		file.Read(buf->Data(), f.compressed_size);

		// decompress
		Buffer* result;
		if(f.compressed_size == f.size)
			result = buf;
		else
		{
			uint real_size = f.size;
			decompressed->Resize(real_size);
			uncompress((Bytef*)decompressed->Data(), (uLongf*)&real_size, (const Bytef*)buf->Data(), f.compressed_size);
			result = decompressed;
		}

		// save
		dir = io::PathToDirectory(f.filename);
		if(!dir.empty())
			io::CreateDirectories(dir);
		FileWriter::WriteAll(f.filename, result);
	}
}
