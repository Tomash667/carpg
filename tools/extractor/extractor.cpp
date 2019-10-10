#include <CoreInclude.h>
#include <File.h>
#include <Tokenizer.h>
#include <iostream>

struct Node
{
	byte type;
	time_t time;
	uint length, hash, compressed;
	byte* data;
};

vector<Node> nodes;
vector<byte> data;

bool Read(cstring path)
{
	FileReader file(path);
	if(!file)
	{
		printf("ERROR: Failed to open file.");
		return false;
	}

	while(!file.IsEof())
	{
		int sign;
		Node& node = Add1(nodes);

		file >> sign;
		file >> node.type;
		file >> node.time;
		file >> node.hash;
		file >> node.length;
		file >> node.compressed;

		if(sign != 0x4C455744)
		{
			printf("Missing node signature.");
			return false;
		}

		node.data = new byte[node.length];
		if(node.compressed == 0)
			file.Read(node.data, node.length);
		else
		{
			Buffer* buf = Buffer::Get();
			buf->Resize(node.compressed);
			file.Read(buf->Data(), node.compressed);
			Buffer* buf2 = buf->Decompress(node.length);
			memcpy(node.data, buf2->Data(), node.length);
			buf2->Free();
		}
	}

	if(nodes.empty())
	{
		printf("Empty file.");
		return false;
	}

	return true;
}

Tokenizer t;

enum Keyword
{
	K_EXIT,
	K_HELP,
	K_LIST,
	K_SAVE
};

void Init()
{
	t.AddKeywords(0, {
		{ "exit", K_EXIT },
		{ "help", K_HELP },
		{ "list", K_LIST },
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
	switch(type)
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
		if(type < 135)
			return Format("SYSTEM(%u)", type);
		else
			return Format("INVALID(%u)", type);
	}
}

void WriteNodeInfo(int index, Node& node)
{
	tm tm;
	localtime_s(&tm, &node.time);
	printf("%d %02d:%02d:%02d %s [length:%u, from:%u]\n",
		index, tm.tm_hour, tm.tm_min, tm.tm_sec, GetMsg(node.type), node.length, node.hash);
}

bool MainLoop()
{
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
				"list [all] - display list of messages\n"
				"save index - save stream data to file\n");
			break;
		case K_LIST:
			{
				t.Next();
				if(t.IsItem("all"))
				{
					int index = 0;
					for(Node& node : nodes)
					{
						WriteNodeInfo(index, node);
						++index;
					}
				}
				else
				{
					byte prev = 0;
					int index = 0;
					bool more = false;
					for(Node& node : nodes)
					{
						if(node.type == prev)
						{
							++index;
							more = true;
							continue;
						}
						if(more)
						{
							more = false;
							printf("...\n");
							WriteNodeInfo(index - 1, nodes[index - 1]);
						}
						WriteNodeInfo(index, node);
						prev = node.type;
						++index;
					}
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
				cstring out = Format("packet%d.bin", index);
				FileWriter f(out);
				Node& node = nodes[index];
				f.Write(node.data, node.length);
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

	cstring path = argv[1];
	if(!Read(path))
	{
		return 1;
	}

	tm tm;
	localtime_s(&tm, &nodes[0].time);
	printf("%04d-%02d-%02d %02d:%02d:%02d\nLoaded nodes: %u\n\n",
		tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, nodes.size());
	Init();

	while(MainLoop());

	return 0;
}
