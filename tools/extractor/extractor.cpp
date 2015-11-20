#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <Pch.h>
#include <Base.h>
#include <Tokenizer.h>
#undef FAR
#define FAR
#include <WinSock2.h>
#include <iostream>

Logger* logger;


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
		if(be - b >= (int)size)
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

Tokenizer t;

enum Keyword
{
	K_EXIT,
	K_HELP,
	K_LIST,
	K_SELECT,
	K_SAVE
};

void Init()
{
	t.AddKeywords(0, {
		{ "exit", K_EXIT },
		{ "help", K_HELP },
		{ "list", K_LIST },
		{ "select", K_SELECT },
		{ "save", K_SAVE }
	});	
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
	string str;
	printf(">");
	std::getline(std::cin, str);
	t.FromString(str);

	bool do_exit = false;

	try
	{
		t.Next();
		Keyword key = (Keyword)t.MustGetKeywordId(0);
		switch(key)
		{
		case K_EXIT:
			do_exit = true;
			break;
		case K_HELP:
			printf("HELP - List of commands:\n"
				"exit - close application\n"
				"help - display this message\n"
				"list - display list of messages grouped by type\n"
				"save index [filename] - save stream data to file\n"
				"select index - select single stream\n");
			break;
		case K_LIST:
			{
				printf("List by type:\n");
				int prev_type = -1;
				int start_index = -1, index = 0;
				for(Node& node : nodes)
				{
					if(prev_type != node.type)
					{
						if(prev_type != -1)
						{
							if(start_index != index - 1)
								printf("%d - %d: Type %d\n", start_index, index - 1, prev_type);
							else
								printf("%d: Type %d\n", start_index, prev_type);
						}
						prev_type = node.type;
					}
					++index;
				}
				if(prev_type != -1)
				{
					if(start_index != index - 1)
						printf("%d - %d: Type %d\n", start_index, index - 1, prev_type);
					else
						printf("%d: Type %d\n", start_index, prev_type);
				}
			}
			break;
		case K_SELECT:
			{
				t.Next();
				int index = t.MustGetInt();
				if(index >= (int)nodes.size())
					t.Throw("Invalid index %d.", index);
				Node& node = nodes[index];
				printf("Index: %d, type: %d, ok: %d, size: %u, from: %s:%u\n", index, node.type, node.ok ? 1 : 0, node.length, inet_ntoa(node.adr.sin_addr),
					node.adr.sin_port);
			}
			break;
		case K_SAVE:
			{
				t.Next();
				int index = t.MustGetInt();
				if(index >= (int)nodes.size())
					t.Throw("Invalid index %d.", index);
				t.Next();
				string out;
				if(t.IsEof())
					out = Format("packet%d.bin", index);
				else
					out = t.MustGetString();
				HANDLE file = CreateFile(out.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
				if(file == INVALID_HANDLE_VALUE)
					t.Throw("Failed to open file '%s' (%d).", out.c_str(), GetLastError());
				Node& node = nodes[index];
				WriteFile(file, node.data, node.length, &tmp, NULL);
				CloseHandle(file);
			}
			break;
		}

		t.Next();
		t.AssertEof();
	}
	catch(Tokenizer::Exception& ex)
	{
		printf("Failed to parse command: %s", ex.ToString());
		do_exit = false;
	}

	return !do_exit;
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

	Init();

	while(MainLoop());

	return 0;
}
