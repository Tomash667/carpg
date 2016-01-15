#include <windows.h>
#include <cstdio>
#include <conio.h>
#include <vector>
#include <string>
#include <zlib.h>

using namespace std;

typedef unsigned char byte;
typedef unsigned int uint;

struct File
{
	string path, name;
	uint size, compressed_size, offset, name_offset;
};

void Crypt(char *inp, DWORD inplen, const char* key, DWORD keylen)
{
	//we will consider size of sbox 256 bytes
	//(extra byte are only to prevent any mishep just in case)
	char Sbox[257], Sbox2[257];
	unsigned long i, j, t, x;

	//this unsecured key is to be used only when there is no input key from user
	char temp , k;
	i = j = t = x = 0;
	temp = k = 0;

	//always initialize the arrays with zero
	ZeroMemory(Sbox, sizeof(Sbox));
	ZeroMemory(Sbox2, sizeof(Sbox2));

	//initialize sbox i
	for(i = 0; i < 256U; i++)
	{
		Sbox[i] = (char)i;
	}

	j = 0;
	//initialize the sbox2 with user key
	for(i = 0; i < 256U ; i++)
	{
		if(j == keylen)
		{
			j = 0;
		}
		Sbox2[i] = key[j++];
	}    

	j = 0 ; //Initialize j
	//scramble sbox1 with sbox2
	for(i = 0; i < 256; i++)
	{
		j = (j + (unsigned long) Sbox[i] + (unsigned long) Sbox2[i]) % 256U ;
		temp =  Sbox[i];                    
		Sbox[i] = Sbox[j];
		Sbox[j] =  temp;
	}

	i = j = 0;
	for(x = 0; x < inplen; x++)
	{
		//increment i
		i = (i + 1U) % 256U;
		//increment j
		j = (j + (unsigned long) Sbox[i]) % 256U;

		//Scramble SBox #1 further so encryption routine will
		//will repeat itself at great interval
		temp = Sbox[i];
		Sbox[i] = Sbox[j] ;
		Sbox[j] = temp;

		//Get ready to create pseudo random  byte for encryption key
		t = ((unsigned long) Sbox[i] + (unsigned long) Sbox[j]) %  256U ;

		//get the random byte
		k = Sbox[t];

		//xor with the data and done
		inp[x] = (inp[x] ^ k);
	}    
}

int FindCharInString(const char* str, const char* chars)
{
	int last = -1, index = 0;
	char c;
	while((c = *str++) != 0)
	{
		const char* cs = chars;
		char c2;
		while((c2 = *cs++) != 0)
		{
			if(c == c2)
			{
				last = index;
				break;
			}
		}
		++index;
	}
	
	return last;
}

string CombinePath(const char* path, const char* filename)
{
	string s;
	int pos = FindCharInString(path, "/\\");
	if(pos != -1)
	{
		s.assign(path, pos);
		s += '/';
		s += filename;
	}
	else
		s = filename;
	return s;
}

void Add(const char* path, vector<File>& files, bool subdir)
{
	printf("Scanning: %s ...\n", path);
	// start find
	WIN32_FIND_DATA find_data;
	HANDLE find = FindFirstFile(path, &find_data);
	if(find == INVALID_HANDLE_VALUE)
	{
		DWORD result = GetLastError();
		printf("ERROR: Can't search for '%s', result %u.\n", path, result);
		return;
	}
	
	do
	{
		if(strcmp(find_data.cFileName, ".") != 0 && strcmp(find_data.cFileName, "..") != 0)
		{
			string new_path = CombinePath(path, find_data.cFileName);
			if((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
			{
				// directory
				if(subdir)
				{
					new_path += "/*";
					Add(new_path.c_str(), files, true);
				}
			}
			else
			{
				File f;
				f.path = new_path;
				f.name = find_data.cFileName;
				f.size = find_data.nFileSizeLow;
				files.push_back(f);
			}
		}
	} while(FindNextFile(find, &find_data));

	DWORD result = GetLastError();
	if(result != ERROR_NO_MORE_FILES)
		printf("ERROR: Can't search for more files '%s', result %u.\n", path, result);
	
	FindClose(find);
}

int main(int argc, char** argv)
{
	bool compr = true, encrypt = false, full_encrypt = false, subdir = true, help = false;
	string crypt_key;
	vector<File> files;

	// process parameters
	for(int i = 1; i < argc; ++i)
	{
		if(argv[i][0] == '-')
		{
			const char* cmd = argv[i] + 1;
			if(strcmp(cmd, "?") == 0 || strcmp(cmd, "h") == 0)
			{
				printf("CaRpg paker v1. Switches:\n"
					"-?/h - help\n"
					"-e pswd - encrypt file entries with password\n"
					"- fe pswd - full encrypt with password\n"
					"-nc - don't compress\n"
					"-ns - don't process subdirectories\n"
					"Parameters without '-' are treated as files/directories.\n");
				help = true;
			}
			else if(strcmp(cmd, "e") == 0)
			{
				if(i < argc)
				{
					printf("Using encryption.\n");
					crypt_key = argv[i];
					encrypt = true;
					full_encrypt = false;
				}
				else
					printf("ERROR: Missing encryption password.\n");
			}
			else if(strcmp(cmd, "fe") == 0)
			{
				if(i < argc)
				{
					printf("Using full encryption.\n");
					crypt_key = argv[i];
					encrypt = true;
					full_encrypt = true;
				}
				else
					printf("ERROR: Missing encryption password.\n");
			}
			else if(strcmp(cmd, "nc") == 0)
			{
				printf("Not using compression.\n");
				compr = false;
			}
			else if(strcmp(cmd, "ns") == 0)
			{
				printf("Disabled processing subdirectories.\n");
				subdir = false;
			}
			else
				printf("ERROR: Invalid switch '%s'.\n", argv[i]);
		}
		else
			Add(argv[i], files, subdir);
	}

	// anything to do?
	if(files.size() == 0)
	{
		if(!help)
			printf("No input files. Use '%s -?' to get help.\n", argv[0]);
		return 0;
	}

	// open pak
	printf("Creating pak file...\n");
	HANDLE pak = CreateFile("data.pak", GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if(pak == INVALID_HANDLE_VALUE)
	{
		DWORD result = GetLastError();
		printf("ERROR: Failed to open 'data.pak' (%u).\n", result);
		return 1;
	}

	// calculate data size & offset
	const uint header_size = 16;
	const uint entries_offset = header_size;
	const uint entries_size = files.size() * 16;
	const uint names_offset = entries_offset + entries_size;
	uint names_size = 0;
	uint offset = names_offset - header_size;
	for(File& f : files)
	{
		f.name_offset = offset;
		uint len = 1 + f.name.length();
		offset += len;
		names_size += len;
	}
	const uint table_size = entries_size + names_size;
	const uint data_offset = entries_offset + table_size;
		
	// write header
	printf("Writing header...\n");
	DWORD tmp;
	char sign[4] = { 'P', 'A', 'K', 1 };
	WriteFile(pak, sign, sizeof(sign), &tmp, NULL);
	int flags = 0;
	if(encrypt)
		flags |= 0x01;
	if(full_encrypt)
		flags |= 0x02;
	WriteFile(pak, &flags, sizeof(flags), &tmp, NULL);
	uint count = files.size();
	WriteFile(pak, &count, sizeof(count), &tmp, NULL);
	WriteFile(pak, &table_size, sizeof(table_size), &tmp, NULL);

	// write data
	printf("Writing files data...\n");
	vector<byte> buf;
	vector<byte> cbuf;
	offset = data_offset;
	SetFilePointer(pak, offset, NULL, FILE_BEGIN);
	for(File& f : files)
	{
		f.offset = offset;

		// read file to buf
		HANDLE file = CreateFile(f.path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if(file == INVALID_HANDLE_VALUE)
		{
			DWORD result = GetLastError();
			printf("ERROR: Failed to open file '%s' (%u).", f.path.c_str(), result);
			f.size = 0;
			f.compressed_size = 0;
			continue;
		}
		buf.resize(f.size);
		ReadFile(file, &buf[0], f.size, &tmp, NULL);
		CloseHandle(file);

		// compress if required
		byte* b = NULL;
		uint size;
		if(compr)
		{
			uLong cbuf_size = compressBound(f.size);
			cbuf.resize(cbuf_size);
			compress(&cbuf[0], &cbuf_size, &buf[0], f.size);
			if(cbuf_size < f.size)
			{
				b = &cbuf[0];
				f.compressed_size = cbuf_size;
				size = cbuf_size;
			}
		}
		if(!b)
		{
			b = &buf[0];
			size = f.size;
			f.compressed_size = f.size;
		}
		if(full_encrypt)
			Crypt((char*)b, f.compressed_size, crypt_key.c_str(), crypt_key.length());

		// write
		WriteFile(pak, b, size, &tmp, NULL);
		offset += size;
	}

	if(!encrypt)
	{
		printf("Writing file entries...\n");

		// file entries
		SetFilePointer(pak, entries_offset, NULL, FILE_BEGIN);
		for(File& f : files)
		{
			WriteFile(pak, &f.name_offset, sizeof(f.name_offset), &tmp, NULL);
			WriteFile(pak, &f.size, sizeof(f.size), &tmp, NULL);
			WriteFile(pak, &f.compressed_size, sizeof(f.compressed_size), &tmp, NULL);
			WriteFile(pak, &f.offset, sizeof(f.offset), &tmp, NULL);
		}

		// file names
		byte zero = 0;
		for(File& f : files)
		{
			WriteFile(pak, f.name.c_str(), f.name.length(), &tmp, NULL);
			WriteFile(pak, &zero, 1, &tmp, NULL);
		}
	}
	else
	{
		printf("Writing encrypted file entries...\n");

		buf.resize(table_size);
		byte* b = &cbuf[0];

		// file entries
		for(File& f : files)
		{
			memcpy(b, &f.name_offset, sizeof(f.name_offset));
			b += 4;
			memcpy(b, &f.size, sizeof(f.size));
			b += 4;
			memcpy(b, &f.compressed_size, sizeof(f.compressed_size));
			b += 4;
			memcpy(b, &f.offset, sizeof(f.offset));
			b += 4;
		}

		// file names
		byte zero = 0;
		for(File& f : files)
		{
			memcpy(b, f.name.c_str(), f.name.length());
			b += f.name.length();
			*b = 0;
			++b;
		}

		Crypt((char*)&buf[0], table_size, crypt_key.c_str(), crypt_key.length());

		SetFilePointer(pak, entries_offset, NULL, FILE_BEGIN);
		WriteFile(pak, &buf[0], buf.size(), &tmp, NULL);
	}

	CloseHandle(pak);
	printf("Done.");
	return 0;
}
