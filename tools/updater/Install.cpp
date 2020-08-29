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
	vector<InstallScript> scripts;

	Tokenizer t;
	t.AddKeyword("install", 0);
	t.AddKeyword("version", 1);
	t.AddKeyword("remove", 2);

	io::FindFiles("install/*.txt", [&](const io::FileInfo& info)
	{
		int major, minor, patch;

		// read file to find version info
		try
		{
			if(t.FromFile(Format("install/%s", info.filename)))
			{
				t.Next();
				if(t.MustGetKeywordId() == 2)
				{
					// old install script
					if(sscanf_s(info.filename, "%d.%d.%d.txt", &major, &minor, &patch) != 3)
					{
						if(sscanf_s(info.filename, "%d.%d.txt", &major, &minor) == 2)
							patch = 0;
						else
						{
							// unknown version
							major = 0;
							minor = 0;
							patch = 0;
						}
					}
				}
				else
				{
					t.AssertKeyword(0);
					t.Next();
					if(t.MustGetInt() != 1)
						t.Throw("Unknown install script version '%d'.", t.MustGetInt());
					t.Next();
					t.AssertKeyword(1);
					t.Next();
					major = t.MustGetInt();
					t.Next();
					minor = t.MustGetInt();
					t.Next();
					patch = t.MustGetInt();
				}

				InstallScript& s = Add1(scripts);
				s.filename = info.filename;
				s.version = (((major & 0xFF) << 16) | ((minor & 0xFF) << 8) | (patch & 0xFF));
			}
		}
		catch(const Tokenizer::Exception& e)
		{
			Warn("Unknown install script '%s': %s", info.filename, e.ToString());
		}

		return true;
	});

	if(scripts.empty())
		return true;

	std::sort(scripts.begin(), scripts.end());

	GetModuleFileName(nullptr, BUF, 256);
	char buf[512], buf2[512];
	char* filename;
	GetFullPathName(BUF, 512, buf, &filename);
	*filename = 0;
	uint len = strlen(buf);

	LocalString s, s2;

	for(vector<InstallScript>::iterator it = scripts.begin(), end = scripts.end(); it != end; ++it)
	{
		cstring path = Format("install/%s", it->filename.c_str());

		try
		{
			if(!t.FromFile(path))
			{
				printf("ERROR: Failed to load install script '%s'.\n", it->filename.c_str());
				continue;
			}
			printf("Running install script %s.\n", it->filename.c_str());

			t.Next();
			t.AssertKeyword();
			if(t.MustGetKeywordId() == 0)
			{
				// skip install 1, version X Y Z W
				t.Next();
				t.AssertInt();
				t.Next();
				t.AssertKeyword(1);
				t.Next();
				t.AssertInt();
				t.Next();
				t.AssertInt();
				t.Next();
				t.AssertInt();
				t.Next();
				t.AssertInt();
				t.Next();
			}

			while(true)
			{
				if(t.IsEof())
					break;
				t.AssertKeyword(2);

				t.Next();
				s2 = t.MustGetString();

				if(GetFullPathName(s2->c_str(), 512, buf2, nullptr) == 0 || strncmp(buf, buf2, len) != 0)
				{
					printf("ERROR: Invalid file path '%s'.\n", s2->c_str());
					return false;
				}

				DeleteFile(buf2);
				t.Next();
			}

			DeleteFile(path);
		}
		catch(cstring err)
		{
			printf("ERROR: Failed to parse install script '%s': %s\n", path, err);
		}
	}

	return true;
}
