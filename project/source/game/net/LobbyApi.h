#pragma once

#include "Timer.h"

#undef IGNORE

class LobbyApi
{
	enum Operation
	{
		NONE,
		GET_SERVERS,
		GET_CHANGES,
		GET_VERSION,
		IGNORE
	};
public:
	LobbyApi();
	~LobbyApi();
	void Update(float dt);
	void Reset();
	void GetServers() { AddOperation(GET_SERVERS); }
	void GetChanges() { AddOperation(GET_CHANGES); }
	bool IsBusy() const { return current_op != NONE; }
	int GetVersion();

	static cstring API_URL;
	static const int API_PORT;

private:
	void AddOperation(Operation op);
	void DoOperation(Operation op);
	void ParseResponse();

	TCPInterface* tcp;
	HTTPConnection* http;
	std::queue<Operation> requests;
	Operation current_op;
	int timestamp;
};
