#pragma once

#include "Timer.h"

class LobbyApi
{
	enum Method
	{
		NONE,
		CREATE_SERVER,
		UPDATE_SERVER,
		PING_SERVER,
		REMOVE_SERVER
	};
public:
	LobbyApi();
	~LobbyApi();
	void Update(float dt);
	void RegisterServer(const string& name, int max_players, int flags, int port);
	void UpdateServer(int players);
	void UnregisterServer();

private:
	void ParseResponse();

	TCPInterface* tcp;
	HTTPConnection* http;
	std::queue<Method> requests;
	int server_id;
	string key;
	float timer;
};
