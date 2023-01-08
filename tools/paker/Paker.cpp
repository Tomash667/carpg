#include <CarpgLibCore.h>
#include <Tokenizer.h>
#include <Crc.h>
#include <File.h>

#include <Shlwapi.h>
#include <fstream>
#include <Shellapi.h>

#include "BlobProxy.h"

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

struct PakEntry
{
	string path, move;
	uint crc, size;
	bool found;
};

vector<Entry> entries;
std::map<cstring, PakEntry*, CstringComparer> pakEntries;
string prevVer;
string pakDir; // "out/0.4"
bool nozip, checkEntry, copyPdb, recreate;

bool FillEntry()
{
	Tokenizer t;
	if(!t.FromFile("paklist.txt"))
	{
		printf("Missing paklist.txt!\n");
		return false;
	}
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
		printf("Failed to parse 'paklist.txt': %s\n", err);
		return false;
	}

	return true;
}

// input: bin/dlls/dll.dll
// output: pak/0.2.11/./dll.dll
bool PakFile(cstring input, cstring output, cstring path)
{
	FileReader file(input);
	if(!file)
	{
		printf("File '%s' can't be opened.\n", input);
		return false;
	}
	uint size = file.GetSize();

	cstring name = output + pakDir.length();

	if(!checkEntry)
	{
		if(path)
			io::CreateDirectories(path);
		CopyFile(input, output, FALSE);

		PakEntry* pe = new PakEntry;
		pe->path = name;
		pe->size = size;
		pe->crc = Crc::Calculate(file);

		pakEntries[pe->path.c_str()] = pe;
	}
	else
	{
		auto it = pakEntries.find(name);
		bool ok = true;
		if(it == pakEntries.end())
		{
			uint crc = Crc::Calculate(file);
			it = std::find_if(pakEntries.begin(), pakEntries.end(), [=](const pair<cstring, PakEntry*>& e)
			{
				return e.second->size == size && e.second->crc == crc;
			});

			if(it == pakEntries.end())
				printf("Added '%s'.\n", input);
			else
			{
				printf("Moved '%s' -> '%s'.\n", it->second->path.c_str(), input);
				ok = false;
				it->second->move = input;
			}
		}
		else
		{
			PakEntry& e = *it->second;
			e.found = true;
			if(e.size == size)
			{
				uint crc = Crc::Calculate(file);
				if(crc == e.crc)
					ok = false;
				else
					printf("Modified '%s'.\n", input);
			}
			else
				printf("Modified '%s'.\n", input);
		}

		if(ok)
		{
			if(path)
				io::CreateDirectories(path);
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
		// ignore special directories (., .., .git, etc)
		if(data.cFileName[0] == '.')
			continue;

		if(IsSet(data.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY))
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
	}
	while(FindNextFile(find, &data));

	FindClose(find);
	return true;
}

void SaveEntries(cstring ver)
{
	std::ofstream o("db.txt");
	o << Format("version \"%s\"\n", ver);
	for(auto it = pakEntries.begin(), end = pakEntries.end(); it != end; ++it)
	{
		PakEntry& e = *it->second;
		o << Format("\"%s\" %u %u\n", e.path.c_str(), e.crc, e.size);
	}
}

void DeleteEntries()
{
	for(auto it = pakEntries.begin(), end = pakEntries.end(); it != end; ++it)
		delete it->second;
	pakEntries.clear();
}

void RunCommand(cstring file, cstring parameters, cstring directory)
{
	SHELLEXECUTEINFO info = {};
	info.cbSize = sizeof(info);
	info.fMask = SEE_MASK_NOCLOSEPROCESS;
	info.lpFile = file;
	info.lpParameters = parameters;
	info.lpDirectory = directory;
	info.nShow = SW_SHOWNORMAL;
	ShellExecuteEx(&info);
	WaitForSingleObject(info.hProcess, INFINITE);
	CloseHandle(info.hProcess);
}

bool CreatePak(char* pakname)
{
	if(!recreate && !nozip && io::FileExists(Format("out/CaRpg_%s.zip", pakname)))
	{
		printf("Full zip already exists, skipping...\n");
		return true;
	}

	checkEntry = false;
	pakDir = Format("out/%s", pakname);
	printf("Creating pak %s.\n", pakDir.c_str());

	if(io::DirectoryExists(pakDir.c_str()))
		io::DeleteDirectory(pakDir.c_str());
	DeleteEntries();
	CreateDirectory(pakDir.c_str(), NULL);

	for(vector<Entry>::iterator it = entries.begin(), end = entries.end(); it != end; ++it)
	{
		Entry& e = *it;
		if(e.type == ET_File)
		{
			cstring output = Format("%s/%s", pakDir.c_str(), e.output.c_str());
			strcpy_s(BUF, output);
			cstring path = BUF;
			if(!PathRemoveFileSpec(BUF) || !BUF[0])
				path = NULL;
			if(!PakFile(e.input.c_str(), output, path))
				return false;
		}
		else if(e.type == ET_Dir)
		{
			string output = Format("%s/%s/", pakDir.c_str(), e.output.c_str());
			if(!PakDir(e.input.c_str(), output.c_str()))
				return false;
		}
		else if(e.type == ET_Pdb)
		{
			if(!copyPdb)
			{
				if(CopyFile(e.input.c_str(), Format("pdb/%s.pdb", pakname), FALSE) == FALSE)
				{
					printf("Failed to copy file '%s'.\n", e.input.c_str());
					return false;
				}
			}
		}
		else
			CreateDirectory(Format("%s/%s", pakDir.c_str(), e.input.c_str()), NULL);
	}

	copyPdb = true;
	printf("Saving database.\n");
	SaveEntries(pakname);

	if(!nozip)
	{
		printf("Compressing pak.\n");
		RunCommand("7z", Format("a -tzip -r ../CaRpg_%s.zip *", pakname), pakDir.c_str());
	}
	return true;
}

bool NeedCreatePatch(char* pakname, bool blob)
{
	if(recreate || nozip)
		return true;

	if(blob)
	{
		cstring path = Format("out/CaRpg_patch_%s.pak", pakname);
		if(io::FileExists(path))
		{
			printf("Patch pak already exists, skipping...\n");
			return false;
		}
	}
	else
	{
		cstring path = Format("out/CaRpg_patch_%s.zip", pakname);
		if(io::FileExists(path))
		{
			printf("Patch zip already exists, skipping...\n");
			return false;
		}
	}

	return true;
}

bool CreatePatch(char* pakname, bool blob)
{
	if(!NeedCreatePatch(pakname, blob))
	{
		if(blob)
		{
			cstring path = Format("out/CaRpg_patch_%s.pak", pakname);
			uint crc = Crc::Calculate(path);

			printf("Uploading to blob & api.\n");
			cstring result = AddChange(pakname, prevVer.c_str(), path, crc, false);
			if(result)
				printf(result);
		}
		return true;
	}

	checkEntry = true;
	pakDir = Format("out/patch_%s", pakname);
	printf("Creating patch %s.\n", pakDir.c_str());

	if(io::DirectoryExists(pakDir.c_str()))
		io::DeleteDirectory(pakDir.c_str());
	CreateDirectory(pakDir.c_str(), NULL);

	for(vector<Entry>::iterator it = entries.begin(), end = entries.end(); it != end; ++it)
	{
		Entry& e = *it;
		if(e.type == ET_File)
		{
			cstring output = Format("%s/%s", pakDir.c_str(), e.output.c_str());
			strcpy_s(BUF, output);
			cstring path = BUF;
			if(!PathRemoveFileSpec(BUF) || !BUF[0])
				path = NULL;
			if(!PakFile(e.input.c_str(), output, path))
				return false;
		}
		else if(e.type == ET_Dir)
		{
			string output = Format("%s/%s/", pakDir.c_str(), e.output.c_str());
			CreateDirectory(output.c_str(), NULL);
			if(!PakDir(e.input.c_str(), output.c_str()))
				return false;
		}
		else if(e.type == ET_Pdb)
		{
			if(!copyPdb)
			{
				if(CopyFile(e.input.c_str(), Format("pdb/%s.pdb", pakname), FALSE) == FALSE)
				{
					printf("Failed to copy file '%s'.\n", e.input.c_str());
					return false;
				}
			}
		}
		else
			CreateDirectory(Format("%s/%s", pakDir.c_str(), e.input.c_str()), NULL);
	}

	// check if install script is required
	copyPdb = true;
	bool needInstall = false, errors = false;
	for(auto it = pakEntries.begin(), end = pakEntries.end(); it != end; ++it)
	{
		PakEntry& e = *it->second;
		if(e.found && !e.move.empty())
		{
			// TODO
			errors = true;
			printf("ERROR: File '%s' duplicate found.\n", e.path.c_str());
		}
		else if(!e.move.empty())
			needInstall = true;
		else if(!e.found)
		{
			printf("Deleted file '%s'.\n", e.path.c_str());
			needInstall = true;
		}
	}
	if(errors)
		exit(1);

	// create install script
	if(needInstall)
	{
		CreateDirectory(Format("%s/install", pakDir.c_str()), NULL);
		std::ofstream o(Format("%s/install/%s.txt", pakDir.c_str(), pakname));
		for(auto it = pakEntries.begin(), end = pakEntries.end(); it != end; ++it)
		{
			PakEntry& entry = *it->second;
			if(entry.found || entry.move.empty())
				continue;

			cstring path = entry.path.c_str();
			if(path[0] == '/')
				++path;

			if(entry.move.empty())
				o << Format("remove \"%s\"\n", path);
			else
			{
				cstring path2 = entry.move.c_str();
				if(path2[0] == '/')
					++path2;
				o << Format("move \"%s\" \"%s\"\n", path, path2);
			}
		}
	}

	// remove empty dirs
	for(vector<Entry>::iterator it = entries.begin(), end = entries.end(); it != end; ++it)
	{
		Entry& e = *it;
		if(e.type == ET_Dir)
			RemoveDirectory(Format("%s/%s", pakDir.c_str(), e.output.c_str()));
		else
			RemoveDirectory(Format("%s/%s", pakDir.c_str(), e.input.c_str()));
	}

	// compress
	if(!nozip)
	{
		if(blob)
		{
			printf("Compressing patch (blob).\n");
			RunCommand("pak.exe", Format("-path -o out/CaRpg_patch_%s.pak %s", pakname, pakDir.c_str()), nullptr);

			cstring path = Format("out/CaRpg_patch_%s.pak", pakname);
			uint crc = Crc::Calculate(path);

			printf("Uploading to blob & api.\n");
			cstring result = AddChange(pakname, prevVer.c_str(), path, crc, true);
			if(result)
				printf(result);
		}
		else
		{
			printf("Compressing patch (zip).\n");
			RunCommand("7z", Format("a -tzip -r ../CaRpg_patch_%s.zip *", pakname), pakDir.c_str());
		}
	}

	return true;
}

bool LoadEntries()
{
	printf("Reading previous version entries.\n");

	Tokenizer t;
	if(!t.FromFile("db.txt"))
	{
		printf("Failed to load db!\n");
		return false;
	}
	DeleteEntries();

	try
	{
		t.Next();
		t.AssertItem("version");
		t.Next();
		prevVer = t.MustGetString();
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
			pakEntries[e->path.c_str()] = e;
		}
	}
	catch(Tokenizer::Exception& ex)
	{
		printf("Error while reading db: %s\n", ex.ToString());
		return false;
	}

	printf("Loaded database.\n");
	return true;
}

int main(int argc, char** argv)
{
	if(argc == 1)
	{
		printf("No arguments. Get some -help.\n");
		return 0;
	}

	CreateDirectory("pdb", NULL);
	CreateDirectory("out", NULL);

	if(!FillEntry())
		return 1;

	int mode = 0;
	bool blob = false;

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
					"-nozip - don't create zip or pak\n"
					"-patch - create patch pak\n"
					"-normal - create full pak (default)\n"
					"-both - create full & patch pak\n"
					"-blob - for patch don't create zip but pak file that is uploaded to azure blob\n"
				);
			}
			else if(strcmp(arg, "nozip") == 0)
				nozip = true;
			else if(strcmp(arg, "patch") == 0)
				mode = 1;
			else if(strcmp(arg, "normal") == 0)
				mode = 0;
			else if(strcmp(arg, "both") == 0)
				mode = 2;
			else if(strcmp(arg, "blob") == 0)
				blob = true;
			else if(strcmp(arg, "recreate") == 0)
				recreate = true;
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
				if(!LoadEntries() || !CreatePatch(argv[i], blob))
					return 2;
			}
			else
			{
				if(!LoadEntries() || !CreatePatch(argv[i], blob) || !CreatePak(argv[i]))
					return 2;
			}
		}
	}

	return 0;
}
