#include "PCH.hpp"
#include "MeshTask.hpp"
#include "QmshTmpLoader.h"
#include "Qmsh.h"
#include <conio.h>
#include <Windows.h>

const char* CONVERTER_VERSION = "21.3";

string group_file, output_file;
GROUP_OPTION gopt;
bool export_phy, force_output;

//=================================================================================================
// Parsuj plik konfiguracyjny aby wydobyæ nazwê pliku
//=================================================================================================
void ParseConfig(ConversionData& cs, std::string& filename)
{
	Tokenizer t(Tokenizer::F_JOIN_MINUS | Tokenizer::F_UNESCAPE);

	try
	{
		t.FromFile(filename);
		t.AddKeyword("file", 0);
		t.AddKeyword("output", 1);

		t.Next();
		t.AssertKeyword(0);
		t.Next();
		t.AssertSymbol(':');
		t.Next();

		cs.input = t.MustGetString();
		cs.gopt = GO_FILE;
		cs.group_file = filename;

		t.Next();
		if(t.IsKeyword(1) && !force_output)
		{
			t.Next();
			t.AssertSymbol(':');
			t.Next();
			cs.output = t.MustGetString();
		}

		Info("Using configuration file '%s'.", filename.c_str());
	}
	catch(const Tokenizer::Exception& ex)
	{
		Error("Failed to parse configuration file '%s': %s", filename.c_str(), ex.ToString());
	}
}

//=================================================================================================
// Przygotuj parametry do konwersji
//=================================================================================================
bool ConvertToQmsh(std::string& filename)
{
	ConversionData cs;
	cs.gopt = GO_ONE;
	cs.export_phy = export_phy;

	uint xpos = filename.find_last_of('.');
	if(xpos != string::npos)
	{
		if(filename.substr(xpos) == ".cfg")
		{
			try
			{
				ParseConfig(cs, filename);
			}
			catch(const Tokenizer::Exception& ex)
			{
				Error("File '%s' is not configuration file!\n%s", filename.c_str(), ex.ToString());
			}
		}
	}

	if(cs.gopt == GO_ONE)
	{
		cs.group_file = group_file;
		cs.gopt = gopt;
		cs.input = filename;
	}

	if(output_file.empty())
	{
		string::size_type pos = cs.input.find_last_of('.');
		if(pos == string::npos)
		{
			if(cs.export_phy)
				cs.output = cs.input + ".phy";
			else
				cs.output = cs.input + ".qmsh";
		}
		else
			cs.output = cs.input.substr(0, pos);
	}
	else
		cs.output = output_file;

	if(cs.gopt == GO_CREATE && group_file.empty())
	{
		string::size_type pos = cs.input.find_first_of('.');
		if(pos == string::npos)
			cs.group_file = cs.input + ".cfg";
		else
			cs.group_file = cs.input.substr(0, pos) + ".cfg";
	}

	cs.force_output = force_output;

	try
	{
		Convert(cs);

		Info("Ok.");
		return true;
	}
	catch(cstring err)
	{
		Error("B³¹d: %s", err);
		return false;
	}
}

//=================================================================================================
// Pocz¹tek programu
//=================================================================================================
int main(int argc, char **argv)
{
	Logger::SetInstance(new ConsoleLogger);

	setlocale(LC_ALL, "");
	setlocale(LC_NUMERIC, "C");

	if(argc == 1)
	{
		printf("QMSH converter, version %s (output %d).\nUsage: \"converter FILE.qmsh.tmp\". Use \"converter -h\" for list of commands.\n",
			CONVERTER_VERSION, QMSH::VERSION);
		return 0;
	}
	export_phy = false;

	int result = 0;
	bool check_subdir = true, force_update = false;

	for(int i = 1; i < argc; ++i)
	{
		char* cstr = argv[i];

		if(cstr[0] == '-')
		{
			string str(cstr);

			if(str == "-h" || str == "-help" || str == "-?")
			{
				printf("Converter switches:\n"
					"-h/help/? - list of commands\n"
					"-v - show converter version and input file versions\n"
					"-o FILE - output file name\n"
					"-g1 - use single group (default)\n"
					"-gf FILE - use animation groups from file\n"
					"-gcreate - create animation groups and save as file\n"
					"-gcreaten FILE - create animation groups and save as named file\n"
					"-phy - export only physic mesh (default extension .phy)\n"
					"-normal - export normal mesh\n"
					"-info FILE - show information about mesh (version etc)\n"
					"-details OPTIONS FILE - like info but more details\n"
					"-compare FILE FILE2 - compare two meshes and show differences\n"
					"-upgrade FILE - upgrade mesh to newest version\n"
					"-upgradedir DIR - upgrade all meshes in directory and subdirectories\n"
					"-subdir - check subdirectories in upgradedir (default)\n"
					"-nosubdir - don't check subdirectories in upgradedir\n"
					"-force - force upgrade operation\n"
					"-noforce - don't force upgrade operation (default)\n"
					"Parameters without '-' are treated as input file.\n");
			}
			else if(str == "-v")
			{
				printf("Converter version %s\nHandled input file version: %d..%d\nOutput file version: %d\n",
					CONVERTER_VERSION, QmshTmpLoader::QMSH_TMP_HANDLED_VERSION.x, QmshTmpLoader::QMSH_TMP_HANDLED_VERSION.y, QMSH::VERSION);
			}
			else if(str == "-g1")
				gopt = GO_ONE;
			else if(str == "-gf")
			{
				if(i + 1 < argc)
				{
					++i;
					group_file = argv[i];
					gopt = GO_FILE;
				}
				else
					printf("Missing FILE name for '-gf'!\n");
			}
			else if(str == "-gcreate")
			{
				gopt = GO_CREATE;
				group_file.clear();
			}
			else if(str == "-gcreaten")
			{
				if(i + 1 < argc)
				{
					++i;
					group_file = argv[i];
					gopt = GO_CREATE;
				}
				else
					printf("Missing FILE name for '-gcreaten'!\n");
			}
			else if(str == "-o")
			{
				if(i + 1 < argc)
				{
					++i;
					output_file = argv[i];
					force_output = true;
				}
				else
					printf("Missing OUTPUT PATH for '-o'!\n");
			}
			else if(str == "-phy")
				export_phy = true;
			else if(str == "-normal")
				export_phy = false;
			else if(str == "-info")
			{
				if(i + 1 < argc)
				{
					++i;
					MeshInfo(argv[i]);
				}
				else
					printf("Missing FILE for '-info'!\n");
			}
			else if(str == "-details")
			{
				if(i + 2 < argc)
				{
					MeshInfo(argv[i + 2], argv[i + 1]);
					i += 2;
				}
				else
				{
					printf("Missing OPTIONS or FILE for '-details'!\n");
					++i;
				}
			}
			else if(str == "-compare")
			{
				if(i + 2 < argc)
				{
					Compare(argv[i + 1], argv[i + 2]);
					i += 2;
				}
				else
				{
					printf("Missing FILEs for '-compare'!\n");
					++i;
				}
			}
			else if(str == "-upgrade")
			{
				if(i + 1 < argc)
				{
					++i;
					Upgrade(argv[i], force_update);
				}
				else
					printf("Missing FILE for '-upgrade'!\n");
			}
			else if(str == "-upgradedir")
			{
				if(i + 1 < argc)
				{
					++i;
					UpgradeDir(argv[i], force_update, check_subdir);
				}
				else
					printf("Missing FILE for '-upgradedir'!\n");
			}
			else if(str == "-subdir")
				check_subdir = true;
			else if(str == "-nosubdir")
				check_subdir = false;
			else if(str == "-force")
				force_update = true;
			else if(str == "-noforce")
				force_update = false;
			else
				printf("Unknown switch \"%s\"!\n", cstr);
		}
		else
		{
			string tstr(cstr);
			if(!ConvertToQmsh(tstr))
				result = 1;
			group_file.clear();
			output_file.clear();
			gopt = GO_ONE;
		}
	}

	if(IsDebuggerPresent())
	{
		printf("Press any key to exit...");
		_getch();
	}

	return result;
}
