#include "Base.h"
#include "Tokenizer.h"
#include <Shlwapi.h>
#include "Crc.h"
#include <fstream>
#include <Shellapi.h>

EXTERN_C DECLSPEC_IMPORT int STDAPICALLTYPE SHCreateDirectoryExA(HWND hwnd, LPCSTR pszPath, const SECURITY_ATTRIBUTES *psa);

bool nozip, check_entry, copy_pdb;

enum EntryType
{
	ET_File,
	ET_Dir,
	ET_Pdb,
	ET_CDir
};

struct Entry
{
	EntryType type;
	string input, output;
};
vector<Entry> entries;

bool FillEntry()
{
	Tokenizer t;
	string s, s2, s3;
	if(!LoadFileToString("paklist.txt", s))
	{
		printf("Missing paklist.txt!");
		return false;
	}
	t.FromString(s);
	t.AddKeyword("file", 0);
	t.AddKeyword("dir", 1);
	t.AddKeyword("pdb", 2);
	t.AddKeyword("cdir", 3);

	try
	{
		while(true)
		{
			// koniec lub typ
			t.Next();
			if(t.IsEof())
				break;
			Entry& e = Add1(entries);
			e.type = (EntryType)t.MustGetKeywordId();

			// string
			t.Next();
			e.input = t.MustGetString();

			// string2
			if(e.type != ET_Pdb && e.type != ET_CDir)
			{
				t.Next();
				e.output = t.MustGetString();
			}
		}
	}
	catch(cstring err)
	{
		printf("Failed to parse 'paklist.txt': %s", err);
		return false;
	}

	return true;
}

struct PakEntry
{
	string path;
	uint crc;
	DWORD size;
	bool found;
};
struct str_cmp
{
	inline bool operator() (cstring a, cstring b) const
	{
		return strcmp(a, b) > 0;
	}
};
std::map<cstring, PakEntry*, str_cmp> pak_entries;

string pak_dir; // "out/0.4"
byte* buf;
char buf2[256];

uint CalculateCrc(HANDLE file)
{
	const DWORD chunk = 64 * 1024;
	if(!buf)
		buf = new byte[chunk];

	DWORD size_left = GetFileSize(file, NULL);
	CRC32 crc;

	while(size_left > 0)
	{
		uint count = min(chunk, size_left);
		ReadFile(file, buf, count, &tmp, NULL);
		crc.Update(buf, count);
		size_left -= count;
	}

	return crc.Get();
}

// input: bin/dlls/dll.dll
// output: pak/0.2.11/./dll.dll
bool PakFile(cstring input, cstring output, cstring path)
{
	HANDLE file = CreateFile(input, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(file == INVALID_HANDLE_VALUE)
	{
		printf("File '%s' can't be opened.\n", input);
		return false;
	}
	DWORD size = GetFileSize(file, NULL);

	cstring name = output + pak_dir.length();

	if(!check_entry)
	{
		if(path)
		{
			GetFullPathName(path, 256, buf2, NULL);
			SHCreateDirectoryExA(NULL, buf2, NULL);
		}
		CopyFile(input, output, FALSE);

		PakEntry* pe = new PakEntry;
		pe->path = name;
		pe->size = size;
		pe->crc = CalculateCrc(file);
		CloseHandle(file);

		pak_entries[pe->path.c_str()] = pe;
	}
	else
	{
		std::map<cstring, PakEntry*, str_cmp>::iterator it = pak_entries.find(name);
		bool ok = true;
		if(it == pak_entries.end())
		{
			CloseHandle(file);
			printf("Added '%s'.\n", input);
		}
		else
		{
			PakEntry& e = *it->second;
			e.found = true;
			if(e.size == size)
			{
				uint crc = CalculateCrc(file);
				CloseHandle(file);
				if(crc == e.crc)
					ok = false;
				else
					printf("Modified '%s'.\n", input);
			}
			else
			{
				CloseHandle(file);
				printf("Modified '%s'.\n", input);
			}
		}
		if(ok)
		{
			if(path)
			{
				GetFullPathName(path, 256, buf2, NULL);
				SHCreateDirectoryExA(NULL, buf2, NULL);
			}
			CopyFile(input, output, FALSE);
		}
	}

	return true;
}

// input: bin/dlls
// output: pak/0.2.11/./
bool PakDir(cstring input, cstring output)
{
	printf("Dir %s.\n", input);

	WIN32_FIND_DATA data;
	HANDLE find = FindFirstFile(Format("%s/*", input), &data);
	if(find == INVALID_HANDLE_VALUE)
	{
		printf("Failed to search directory '%s'.\n", input);
		return false;
	}

	do
	{
		if(strcmp(data.cFileName, ".") == 0 || strcmp(data.cFileName, "..") == 0)
			continue;

		if(IS_SET(data.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY))
		{
			string input2 = Format("%s/%s", input, data.cFileName),
				output2 = Format("%s%s/", output, data.cFileName);
			if(!PakDir(input2.c_str(), output2.c_str()))
			{
				FindClose(find);
				return false;
			}
		}
		else
		{
			if(!PakFile(Format("%s/%s", input, data.cFileName), Format("%s%s", output, data.cFileName), output))
			{
				FindClose(find);
				return false;
			}
		}
	} while(FindNextFile(find, &data));

	FindClose(find);
	return true;
}

void SaveEntries(cstring out)
{
	std::ofstream o(out);

	for(std::map<cstring, PakEntry*, str_cmp>::iterator it = pak_entries.begin(), end = pak_entries.end(); it != end; ++it)
	{
		PakEntry& e = *it->second;
		o << Format("\"%s\" %u %u\n", e.path.c_str(), e.crc, e.size);
	}
}

void DeleteEntries()
{
	for(std::map<cstring, PakEntry*, str_cmp>::iterator it = pak_entries.begin(), end = pak_entries.end(); it != end; ++it)
		delete it->second;
	pak_entries.clear();
}

bool CreatePak(char* pakname)
{
	check_entry = false;
	pak_dir = Format("out/%s", pakname);
	printf("Creating pak %s.\n", pak_dir.c_str());

	if(DirectoryExists(pak_dir.c_str()))
		DeleteDirectory(pak_dir.c_str());
	DeleteEntries();
	CreateDirectory(pak_dir.c_str(), NULL);

	for(vector<Entry>::iterator it = entries.begin(), end = entries.end(); it != end; ++it)
	{
		Entry& e = *it;
		if(e.type == ET_File)
		{
			cstring output = Format("%s/%s", pak_dir.c_str(), e.output.c_str());
			strcpy(BUF, output);
			cstring path = BUF;
			if(!PathRemoveFileSpec(BUF) || !BUF[0])
				path = NULL;
			if(!PakFile(e.input.c_str(), output, path))
				return false;
		}
		else if(e.type == ET_Dir)
		{
			string output = Format("%s/%s/", pak_dir.c_str(), e.output.c_str());
			if(!PakDir(e.input.c_str(), output.c_str()))
				return false;
		}
		else if(e.type == ET_Pdb)
		{
			if(!copy_pdb)
			{
				if(CopyFile(e.input.c_str(), Format("pdb/%s.pdb", pakname), FALSE) == FALSE)
				{
					printf("Failed to copy file '%s'.", e.input.c_str());
					return false;
				}
			}
		}
		else
			CreateDirectory(Format("%s/%s", pak_dir.c_str(), e.input.c_str()), NULL);
	}

	copy_pdb = true;
	printf("Saving database.\n");
	SaveEntries(Format("db/%s.txt", pakname));

	if(!nozip)
	{
		printf("Compressing pak.\n");
		ShellExecute(NULL, NULL, "7z", Format("a -tzip -r ../CaRpg_%s.zip *", pakname), pak_dir.c_str(), SW_SHOWNORMAL);
	}
	return true;
}

bool CreatePatch(char* pakname)
{
	check_entry = true;
	pak_dir = Format("out/patch_%s", pakname);
	printf("Creating patch %s.\n", pak_dir.c_str());

	if(DirectoryExists(pak_dir.c_str()))
		DeleteDirectory(pak_dir.c_str());
	CreateDirectory(pak_dir.c_str(), NULL);

	for(vector<Entry>::iterator it = entries.begin(), end = entries.end(); it != end; ++it)
	{
		Entry& e = *it;
		if(e.type == ET_File)
		{
			cstring output = Format("%s/%s", pak_dir.c_str(), e.output.c_str());
			strcpy(BUF, output);
			cstring path = BUF;
			if(!PathRemoveFileSpec(BUF) || !BUF[0])
				path = NULL;
			if(!PakFile(e.input.c_str(), output, path))
				return false;
		}
		else if(e.type == ET_Dir)
		{
			string output = Format("%s/%s/", pak_dir.c_str(), e.output.c_str());
			CreateDirectory(output.c_str(), NULL);
			if(!PakDir(e.input.c_str(), output.c_str()))
				return false;
		}
		else if(e.type == ET_Pdb)
		{
			if(!copy_pdb)
			{
				if(CopyFile(e.input.c_str(), Format("pdb/%s.pdb", pakname), FALSE) == FALSE)
				{
					printf("Failed to copy file '%s'.", e.input.c_str());
					return false;
				}
			}
		}
		else
			CreateDirectory(Format("%s/%s", pak_dir.c_str(), e.input.c_str()), NULL);
	}

	// create list of missing files
	copy_pdb = true;
	printf("Checking deleted files.\n");
	vector<PakEntry*> missing;
	for(std::map<cstring, PakEntry*, str_cmp>::iterator it = pak_entries.begin(), end = pak_entries.end(); it != end; ++it)
	{
		PakEntry& e = *it->second;
		if(!e.found)
		{
			printf("Deleted file '%s'.\n", e.path.c_str());
			missing.push_back(&e);
		}
	}
	if(!missing.empty())
	{
		CreateDirectory(Format("%s/system", pak_dir.c_str()), NULL);
		CreateDirectory(Format("%s/system/install", pak_dir.c_str()), NULL);
		std::ofstream o(Format("%s/system/install/%s.txt", pak_dir.c_str(), pakname));
		for(vector<PakEntry*>::iterator it = missing.begin(), end = missing.end(); it != end; ++it)
		{
			cstring entry = (*it)->path.c_str();
			if(entry[0] == '/')
				++entry;
			o << Format("remove \"%s\"\n", entry);
		}
	}

	// remove empty dirs
	for(vector<Entry>::iterator it = entries.begin(), end = entries.end(); it != end; ++it)
	{
		Entry& e = *it;
		if(e.type == ET_Dir)
			RemoveDirectory(Format("%s/%s", pak_dir.c_str(), e.output.c_str()));
		else
			RemoveDirectory(Format("%s/%s", pak_dir.c_str(), e.input.c_str()));
	}

	// compress
	if(!nozip)
	{
		printf("Compressing patch.\n");
		ShellExecute(NULL, NULL, "7z", Format("a -tzip -r ../CaRpg_patch_%s.zip *", pakname), pak_dir.c_str(), SW_SHOWNORMAL);
	}

	return true;
}

bool LoadEntries(char* pakname)
{
	printf("Searching for last version.\n");

	WIN32_FIND_DATA data;
	HANDLE find = FindFirstFile("db/*.txt", &data);
	if(find == INVALID_HANDLE_VALUE)
	{
		printf("Failed to find last version.\n");
		return false;
	}

	string cur_pak = Format("%s.txt", pakname);
	DWORD t1 = 0, t2 = 0;
	string best;

	do
	{
		if(cur_pak != data.cFileName && data.ftLastWriteTime.dwHighDateTime > t1 || (data.ftLastWriteTime.dwHighDateTime == t1 && data.ftLastWriteTime.dwLowDateTime > t2))
		{
			t1 = data.ftLastWriteTime.dwHighDateTime;
			t2 = data.ftLastWriteTime.dwLowDateTime;
			best = data.cFileName;
		}
	} while(FindNextFile(find, &data));

	FindClose(find);

	printf("Using version %s. Loading...\n", best.c_str());

	string s;
	if(!LoadFileToString(Format("db/%s", best.c_str()), s))
	{
		printf("Failed to load file '%s'!", best.c_str());
		return false;
	}

	Tokenizer t;
	t.FromString(s);
	DeleteEntries();

	try
	{
		while(true)
		{
			t.Next();
			if(t.IsEof())
				break;
			PakEntry* e = new PakEntry;
			e->path = t.MustGetString();
			t.Next();
			e->crc = t.MustGetUint();
			t.Next();
			e->size = t.MustGetInt();
			e->found = false;
			pak_entries[e->path.c_str()] = e;
		}
	}
	catch(cstring err)
	{
		printf("Error while reading file '%s': %s\n", best.c_str(), err);
		return false;
	}

	printf("Loaded database.\n");
	return true;
}

int main(int argc, char** argv)
{
	if(argc == 1)
	{
		printf("No arguments. Get some -help.");
		return 0;
	}

	CreateDirectory("db", NULL);
	CreateDirectory("pdb", NULL);
	CreateDirectory("out", NULL);

	if(!FillEntry())
		return 1;

	int mode = 0;

	for(int i = 1; i < argc; ++i)
	{
		if(argv[i][0] == '-')
		{
			char* arg = argv[i] + 1;
			if(strcmp(arg, "help") == 0 || strcmp(arg, "?") == 0)
			{
				printf("Paker tool. Usage: paker [switches] version.\n"
					"List of switches:\n"
					"-help - display help\n"
					"-nozip - don't zip pak\n"
					"-patch - create only patch pak\n"
					"-normal - create normal pak (default)\n"
					"-both - create normal & patch pak\n");
			}
			else if(strcmp(arg, "nozip") == 0)
				nozip = true;
			else if(strcmp(arg, "patch") == 0)
				mode = 1;
			else if(strcmp(arg, "normal") == 0)
				mode = 0;
			else if(strcmp(arg, "both") == 0)
				mode = 2;
			else
				printf("Unknown switch '%s'.\n", arg);
		}
		else
		{
			if(mode == 0)
			{
				if(!CreatePak(argv[i]))
					return 2;
			}
			else if(mode == 1)
			{
				if(!LoadEntries(argv[i]) || !CreatePatch(argv[i]))
					return 2;
			}
			else
			{
				if(!LoadEntries(argv[i]) || !CreatePatch(argv[i]) || !CreatePak(argv[i]))
					return 2;
			}
		}
	}

	return 0;
}
