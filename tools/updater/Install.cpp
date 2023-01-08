#include <CarpgLibCore.h>
#include <Tokenizer.h>
#include <File.h>
#include <WindowsIncludes.h>

struct InstallScript
{
	string filename;
	int version;

	bool operator < (const InstallScript& s) const
	{
		return version < s.version;
	}
};

//=================================================================================================
bool RunInstallScripts()
{
	// find install scripts
	vector<InstallScript> scripts;
	io::FindFiles("install/*.txt", [&](const io::FileInfo& info)
	{
		int major, minor, patch;

		if(sscanf_s(info.filename, "%d.%d.%d.txt", &major, &minor, &patch) != 3)
		{
			if(sscanf_s(info.filename, "%d.%d.txt", &major, &minor) == 2)
				patch = 0;
			else
				return true;
		}

		InstallScript& s = Add1(scripts);
		s.filename = info.filename;
		s.version = (((major & 0xFF) << 16) | ((minor & 0xFF) << 8) | (patch & 0xFF));
		return true;
	});

	if(scripts.empty())
		return true;

	std::sort(scripts.begin(), scripts.end());

	// get full path to directory with exe, this is to prevent changing files from parent directory
	GetModuleFileName(nullptr, BUF, 256);
	char origin[512], filePath[512], filePath2[512];
	char* originFilename;
	GetFullPathName(BUF, 512, origin, &originFilename);
	*originFilename = 0;
	uint len = strlen(origin);

	Tokenizer t;
	t.AddKeyword("remove", 0);
	t.AddKeyword("move", 1);

	LocalString filename, filename2;

	for(vector<InstallScript>::iterator it = scripts.begin(), end = scripts.end(); it != end; ++it)
	{
		cstring path = Format("install/%s", it->filename.c_str());

		try
		{
			if(!t.FromFile(path))
			{
				Error("Failed to load install script '%s'.", it->filename.c_str());
				continue;
			}
			Info("Using install script %s.", it->filename.c_str());

			t.Next();
			while(true)
			{
				if(t.IsEof())
					break;

				int keyword = t.MustGetKeywordId();
				t.Next();
				if(keyword == 0)
				{
					// remove
					filename = t.MustGetString();
					t.Next();

					if(GetFullPathName(filename->c_str(), 512, filePath, nullptr) == 0 || strncmp(origin, filePath, len) != 0)
					{
						Error("Invalid file path '%s'.", filename->c_str());
						return false;
					}

					io::DeleteFile(filePath);
				}
				else
				{
					// move
					filename = t.MustGetString();
					t.Next();
					filename2 = t.MustGetString();
					t.Next();

					if(GetFullPathName(filename->c_str(), 512, filePath, nullptr) == 0 || strncmp(origin, filePath, len) != 0)
					{
						Error("Invalid file path '%s'.", filename->c_str());
						return false;
					}

					if(GetFullPathName(filename2->c_str(), 512, filePath2, nullptr) == 0 || strncmp(origin, filePath2, len) != 0)
					{
						Error("Invalid file path '%s'.", filename2->c_str());
						return false;
					}

					io::MoveFile(filePath, filePath2);
				}
			}

			io::DeleteFile(path);
		}
		catch(const Tokenizer::Exception& ex)
		{
			Error("Failed to parse install script '%s': %s", path, ex.ToString());
		}
	}

	io::DeleteDirectory("install");

	return true;
}
