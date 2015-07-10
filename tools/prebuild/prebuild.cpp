#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fstream>

const char* HEADER_FILE = "..\\source\\CompileTime.h";

/* OPTIONS:
default: read build version and save it with new date
-inc: read build version, incrase by 1 and save
-create: create header if it not exists
*/

int main(int argc, char** argv)
{
	// sprawdz czy istnieje
	if(argc == 2 && strcmp(argv[1], "-create") == 0)
	{
		std::ifstream f(HEADER_FILE);
		if(f.good())
			return 0;
		f.close();
	}

	// zapisz czas
	time_t t = time(0);
	tm time;
	localtime_s(&time, &t);

	//2013-07-13 16:49:41
	std::ofstream f(HEADER_FILE);
	char buf[128];
	sprintf_s(buf, 128, "const char* g_ctime = \"%04d-%02d-%02d %02d:%02d:%02d\";\n;", time.tm_year+1900, time.tm_mon+1, time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec);
	f << buf;

	return 0;
}
