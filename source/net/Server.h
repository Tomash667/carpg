#pragma once

class Server
{
public:
	enum Mode
	{
		SP,
		LAN,
		Internet
	};

	enum State
	{
		Lobby
	};

	Server();
	void CreateServerSp();
	void CreateServer(Mode mode, cstring name, int maxPlayers);

private:
	void Loop();

	thread serverThread;
	std::condition_variable cv;
	std::mutex m;
	Mode mode;
	State state;
	string name;
	int maxPlayers;
	bool initialized, paused;
};
