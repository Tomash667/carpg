#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#undef min
#undef max
#undef STRICT
#include <Pch.h>
#include <Core.h>
#include <Tokenizer.h>
#include <iostream>

Logger* logger;

cstring types[] = {
	"None",
	"PickServer",
	"PingIp",
	"Connect",
	"Quitting",
	"QuittingServer",
	"Transfer",
	"TransferServer",
	"ServerSend",
	"UpdateLobbyServer",
	"UpdateLobbyClient",
	"UpdateGameServer",
	"UpdateGameClient"
};

enum STATUS
{
	STATUS_OK,
	STATUS_ERROR,
	STATUS_WRITE
};

cstring status_str[] = {
	"READ OK",
	"READ ERROR",
	"WRITE"
};

struct Node
{
	STATUS status;
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
		b = (byte*)data.data();
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
		byte status;
		byte type;
		sockaddr_in adr;
		uint length;

		if(!r.Read(sign)
			|| !r.Read(status)
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
		node.status = (STATUS)status;
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

enum GamePacket : byte
{
	ID_HELLO = 135,
	ID_LOBBY_UPDATE,
	ID_CANT_JOIN,
	ID_JOIN,
	ID_CHANGE_READY,
	ID_SAY,
	ID_WHISPER,
	ID_SERVER_SAY,
	ID_LEAVE,
	ID_SERVER_CLOSE,
	ID_PICK_CHARACTER,
	ID_TIMER,
	ID_END_TIMER,
	ID_STARTUP,
	ID_STATE,
	ID_WORLD_DATA,
	ID_PLAYER_START_DATA,
	ID_READY,
	ID_CHANGE_LEVEL,
	ID_LEVEL_DATA,
	ID_PLAYER_DATA,
	ID_START,
	ID_CONTROL,
	ID_CHANGES,
	ID_PLAYER_UPDATE
};

cstring GetMsg(byte type)
{
	switch (type)
	{
	case ID_HELLO:
		return "ID_HELLO";
	case ID_LOBBY_UPDATE:
		return "ID_LOBBY_UPDATE";
	case ID_CANT_JOIN:
		return "ID_CANT_JOIN";
	case ID_JOIN:
		return "ID_JOIN";
	case ID_CHANGE_READY:
		return "ID_CHANGE_READY";
	case ID_SAY:
		return "ID_SAY";
	case ID_WHISPER:
		return "ID_WHISPER";
	case ID_SERVER_SAY:
		return "ID_SERVER_SAY";
	case ID_LEAVE:
		return "ID_LEAVE";
	case ID_SERVER_CLOSE:
		return "ID_SERVER_CLOSE";
	case ID_PICK_CHARACTER:
		return "ID_PICK_CHARACTER";
	case ID_TIMER:
		return "ID_TIMER";
	case ID_END_TIMER:
		return "ID_END_TIMER";
	case ID_STARTUP:
		return "ID_STARTUP";
	case ID_STATE:
		return "ID_STATE";
	case ID_WORLD_DATA:
		return "ID_WORLD_DATA";
	case ID_PLAYER_START_DATA:
		return "ID_PLAYER_START_DATA";
	case ID_READY:
		return "ID_READY";
	case ID_CHANGE_LEVEL:
		return "ID_CHANGE_LEVEL";
	case ID_LEVEL_DATA:
		return "ID_LEVEL_DATA";
	case ID_PLAYER_DATA:
		return "ID_PLAYER_DATA";
	case ID_START:
		return "ID_START";
	case ID_CONTROL:
		return "ID_CONTROL";
	case ID_CHANGES:
		return "ID_CHANGES";
	case ID_PLAYER_UPDATE:
		return "ID_PLAYER_UPDATE";
	default:
		if (type < 135)
			return Format("SYSTEM(%u)", type);
		else
			return Format("INVALID(%u)", type);
	}
}

void LogStream(Node& node, int index)
{
	string m = "(";
	for (int i = 0; i < min(4, (int)node.length) - 1; ++i)
	{
		if (i != 0)
			m += ";";
		m += Format("%u", node.data[i + 1]);
	}
	if (m.length() == 1)
		m.clear();
	else
		m += ")";
	printf("%s: Index: %d, type: %s(%d), size: %u, from: %s:%u, cmd: %s%s\n", status_str[node.status], index, types[node.type], node.type, node.length, inet_ntoa(node.adr.sin_addr),
		node.adr.sin_port, GetMsg(node.data[0]), m.c_str());
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
				"select index/all - select single stream\n");
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
								printf("%d - %d: Type %s(%d)\n", start_index, index - 1, types[prev_type], prev_type);
							else
								printf("%d: Type %s(%d)\n", start_index, types[prev_type], prev_type);
						}
						prev_type = node.type;
						start_index = index;
					}
					++index;
				}
				if(prev_type != -1)
				{
					if(start_index != index - 1)
						printf("%d - %d: Type %s(%d)\n", start_index, index - 1, types[prev_type], prev_type);
					else
						printf("%d: Type %s(%d)\n", start_index, types[prev_type], prev_type);
				}
			}
			break;
		case K_SELECT:
			{
				t.Next();
				if (t.IsItem("all"))
				{
					int index = 0;
					for (Node& node : nodes)
					{
						LogStream(node, index);
						++index;
					}
				}
				else
				{
					int index = t.MustGetInt();
					if (index >= (int)nodes.size())
						t.Throw("Invalid index %d.\n", index);
					LogStream(nodes[index], index);
				}
			}
			break;
		case K_SAVE:
			{
				t.Next();
				int index = t.MustGetInt();
				if(index >= (int)nodes.size())
					t.Throw("Invalid index %d.\n", index);
				t.Next();
				string out;
				if(t.IsEof())
					out = Format("packet%d.bin", index);
				else
					out = t.MustGetString();
				HANDLE file = CreateFile(out.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
				if(file == INVALID_HANDLE_VALUE)
					t.Throw("Failed to open file '%s' (%d).\n", out.c_str(), GetLastError());
				Node& node = nodes[index];
				WriteFile(file, node.data, node.length, &tmp, nullptr);
				CloseHandle(file);
			}
			break;
		}

		t.Next();
		t.AssertEof();
	}
	catch(Tokenizer::Exception& ex)
	{
		printf("Failed to parse command: %s\n", ex.ToString());
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

	HANDLE file = CreateFile(argv[1], GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if(file == INVALID_HANDLE_VALUE)
	{
		printf("Failed to open file. Error %u.", GetLastError());
		return 1;
	}

	DWORD size = GetFileSize(file, nullptr);
	data.resize(size);
	DWORD tmp;
	ReadFile(file, data.data(), size, &tmp, nullptr);
	if(tmp != size)
	{
		printf("Failed to read file. Error %u.", GetLastError());
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
