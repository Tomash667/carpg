#include "PCH.hpp"
#include "MeshTask.hpp"

string group_file, output_file;
GROUP_OPTION gopt;
bool export_phy, force_output;

//=================================================================================================
// Parsuj plik konfiguracyjny aby wydoby� nazw� pliku
//=================================================================================================
void ParseConfig(ConversionData& cs, std::string& filename)
{
	FileStream file(filename, FM_READ);
	Tokenizer t(&file, 0);
	t.RegisterKeyword(0, "file");
	t.RegisterKeyword(1, "output");

	t.Next();
	t.AssertKeyword(0);
	t.Next();
	t.AssertSymbol(':');
	t.Next();
	t.AssertToken(Tokenizer::TOKEN_STRING);

	cs.input = t.GetString();
	cs.gopt = GO_FILE;
	cs.group_file = filename;

	t.Next();
	if(t.QueryKeyword(1) && !force_output)
	{
		t.Next();
		t.AssertSymbol(':');
		t.Next();
		t.AssertToken(Tokenizer::TOKEN_STRING);
		cs.output = t.GetString();
	}

	printf("U�ywanie pliku konfiguracyjnego '%s'.\n", filename.c_str());
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
			catch(Error& er)
			{
				string msg;
				er.GetMessage_(&msg);
				printf("Plik '%s' nie jest plikiem konfiguracyjnym!\n%s\n", filename.c_str(), msg.c_str());
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
		if( pos == string::npos )
			cs.output = cs.input + ".qmsh";
		else
			cs.output = cs.input.substr(0,pos);
	}
	else
		cs.output = output_file;

	if(cs.gopt == GO_CREATE && group_file.empty())
	{
		string::size_type pos = cs.input.find_first_of('.');
		if( pos == string::npos )
			cs.group_file = cs.input + ".cfg";
		else
			cs.group_file = cs.input.substr(0,pos) + ".cfg";
	}

	cs.force_output = force_output;

	try
	{
		Convert(cs);

		printf("Ok.\n");
		return true;
	}
	catch(const Error &e)
	{
		string Msg;
		e.GetMessage_(&Msg, "  ");
		printf("B��d: %s\n", Msg.c_str());
		return false;
	}
}

//=================================================================================================
// Pocz�tek programu
//=================================================================================================
int main(int argc, char **argv)
{
	setlocale(LC_ALL, "");
	setlocale(LC_NUMERIC, "C");

	if(argc == 1)
	{
		printf("Konwerter plik�w QMSH, wersja %d.\nU�ycie: \"converter PLIK.qmsh.tmp\".\nWpisz \"converter -h\" aby uzyska� list� komend.\n", QMSH_VERSION);
		return 0;
	}
	export_phy = false;

	int result = 0;

	for(int i=1; i<argc; ++i)
	{
		char* cstr = argv[i];

		if(cstr[0] == '-')
		{
			string str(cstr);

			if(str == "-h" || str == "-help" || str == "-?")
			{
				printf("Prze��czniki konwertera:\n"
					"-h/help/? - lista komend\n"
					"-v - wy�wietla wersj� konwertera i plik�w wej�ciowych\n"
					"-o PLIK - nazwa pliku wyj�ciowego\n"
					"-g1 - u�ywanie jednej grupy animacji (domy�lnie)\n"
					"-gf PLIK - u�ywanie grupy animacji z pliku\n"
					"-gcreate - stworzenie pliku konfiguracyjnego dla modelu\n"
					"-gcreaten PLIK - stworzenie nazwanego pliku konfiguracyjnego dla modelu\n"
					"-phy - eksportuje sam� siatk�\n"
					"-normal - normalny eksport\n"
					"Parametry bez znaku '-' s� traktowane jako pliki wej�ciowe.\n");
			}
			else if(str == "-v")
				printf("Wersja plik�w wej�ciowych: %d..%d\nWersja konwertera: %d\n", QMSH_TMP_VERSION_MIN, QMSH_TMP_VERSION_MAX, QMSH_VERSION);
			else if(str == "-g1")
				gopt = GO_ONE;
			else if(str == "-gf")
			{
				if(i+1 < argc)
				{
					++i;
					group_file = argv[i];
					gopt = GO_FILE;
				}
				else
					printf("Brak nazwy pliku dla prze��cznika '-gf'!\n");
			}
			else if(str == "-gcreate")
			{
				gopt = GO_CREATE;
				group_file.clear();
			}
			else if(str == "-gcreaten")
			{
				if(i+1 < argc)
				{
					++i;
					group_file = argv[i];
					gopt = GO_CREATE;
				}
				else
					printf("Brak nazwy pliku dla prze��cznika '-gcreaten'!\n");
			}			
			else if(str == "-o")
			{
				if(i+1 < argc)
				{
					++i;
					output_file = argv[i];
					force_output = true;
				}
				else
					printf("Brak nazwy pliku dla prze��cznika '-o'!\n");
			}
			else if(str == "-phy")
				export_phy = true;
			else if(str == "-normal")
				export_phy = false;
			else
				printf("Nieznany prze��cznik \"%s\"!\n", cstr);
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

	return result;
}
