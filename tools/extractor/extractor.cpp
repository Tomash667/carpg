#include <Pch.h>
#include <Base.h>
#include <Tokenizer.h>
#define FAR
#include <WinSock2.h>

typedef unsigned int uint;

using std::vector;

struct Node
{
	bool ok;
	int index, type;
	sockaddr_in adr;
	uint length;
	byte* data;
	// http://www.retran.com/beej/inet_ntoaman.html
};

vector<Node> nodes;
vector<byte> data;

struct Reader
{
	byte* b, *bs, *be;

	Reader()
	{
		b = (byte*)data.size();
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

	inline bool IsEof() const
	{
		return b == be;
	}
};

void ProcessFile()
{
	Reader r;

	int index = 0;
	while(!r.IsEof())
	{
		byte sign;
		byte ok;
		byte type;
		sockaddr_in adr;
		uint length;

		if(!r.Read(sign)
			|| !r.Read(ok)
			|| !r.Read(type)
			|| !r.Read(adr)
			|| !r.Read(length))
		{
			printf("Read failed at index %d.\n", index);
			return;
		}

		if(sign != 0xFF)
		{
			printf("Read sign failed at index %d.\n", index);
			return;
		}

		byte* ptr = r.b;
		if(!r.Skip(length))
		{
			printf("Read data failed at index %d.\n", index);
			return;
		}

		Node node;
		node.ok = (ok != 0);
		node.index = index;
		node.type = type;
		node.length = length;
		node.data = ptr;
		node.adr = adr;
		nodes.push_back(node);
	}
}

bool MainLoop()
{
	/*
	cmds:
	help
	list
	list group
	list type
	list adr
	exit
	filter (none, ok=0/1, index=> >= < <= !=0/1/n, type, adr
	adr
	*/

	Tokenizer t;

	/*
	exit
	list
	select
	save
	*/
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
	if(nodes.empty())
	{
		printf("Failed to read file, closing...");
		return 3;
	}

	while(MainLoop());

	return 0;
}
