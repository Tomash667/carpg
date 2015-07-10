#include <cstdio>
#include <fstream>
#include <string>

using namespace std;

bool get_next(ifstream& i, string& s)
{
	s.clear();

	char c;
	bool started = false;

	while(i >> c)
	{
		if(c == '$')
		{
			if(!started)
				started = true;
			else
				return true;
		}
		else if(c == '"' && started)
		{
			printf("Warning, '%s' this may be error.\n", s.c_str());
			return true;
		}
		else if(started)
			s += c;
	}

	return false;
}

int main(int argc, char** argv)
{
	if(argc != 3)
	{
		printf("Usage: %s file1 file2", argv[0]);
		return 0;
	}

	ifstream i1(argv[1]), i2(argv[2]);
	if(!i1)
	{
		printf("Failed to open '%s'.", argv[1]);
		return 1;
	}
	if(!i2)
	{
		printf("Failed to open '%s'.", argv[2]);
		return 1;
	}

	string s1, s2;

	while(true)
	{
		bool r1 = get_next(i1, s1),
			 r2 = get_next(i2, s2);

		if(r1 == r2)
		{
			if(r1)
			{
				if(s1 != s2)
					printf("Missmatch [%s] <> [%s]!\n", s1.c_str(), s2.c_str());
			}
			else
				return 0;
		}
		else
		{
			printf("Count missmatch [%s] <> [%s]!\n", s1.c_str(), s2.c_str());
			return 0;
		}
	}
}
