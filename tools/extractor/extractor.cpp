#include <cstdio>
#include <Windows.h>
#include <vector>

typedef unsigned int uint;

using std::vector;

struct Node
{
	bool ok;
	int type;
	uint length, offset;
};

vector<Node> nodes;
vector<byte> data;

struct Reader
{
	byte* b, *bs, *be;

	Reader()
	{
		b = data.size();
		bs = b;
		be = b + data.size();
	}

	template<typename T>
	bool Read(T& v)
	{
		if(be - b >= sizeof(v))
		{
			v = *(T*)b;
			b += sizeof(T);
			return true;
		}
		else
			return false;
	}

	bool Skip(uint size)
	{
		if(be - b >= size)
		{
			b += size;
			return true;
		}
		else
			return false;
	}
};

void ProcessFile()
{
	Reader r;

	while(true)
	{
		byte sign;
		byte ok;
		byte type;
		sockaddr_in adr;
		// http://www.retran.com/beej/inet_ntoaman.html
		uint length;
		byte[] data;
	}
}

bool MainLoop()
{
	return true;
}

int main(int argc, char** argv)
{
	if(argc != 2)
	{
		printf("Usage: %s file", argv[0]);
		return 0;
	}

	HANDLE file = CreateFile(argv[1], GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(file == INVALID_HANDLE_VALUE)
	{
		printf("Failed to open file. Error %u.", GetLastError());
		return 1;
	}

	DWORD size = GetFileSize(file, NULL);
	data.resize(size);
	DWORD tmp;
	ReadFile(file, data.data(), size, &tmp, NULL);
	if(tmp != size)
	{
		printf("Failed to read file. Error &u.", GetLastError());
		return 2;
	}

	CloseHandle(file);

	ProcessFile();
	while(MainLoop());

	return 0;
}
