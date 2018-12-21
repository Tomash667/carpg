#pragma once

#include "Timer.h"

#undef IGNORE

namespace SLNet
{
	class HTTPConnection2;
	class NatPunchthroughClient;
	class TCPInterface;
}

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
	void Update();
	void Reset();
	void GetServers() { AddOperation(GET_SERVERS); }
	void GetChanges() { AddOperation(GET_CHANGES); }
	bool IsBusy() const { return current_op != NONE; }
	int GetVersion(delegate<bool()> cancel_clbk);
	void StartPunchthrough(RakNetGUID* target);
	void EndPunchthrough();

	static cstring API_URL;
	static const int API_PORT;
	static const int PROXY_PORT;

private:
	void UpdateInternal();
	void AddOperation(Operation op);
	void DoOperation(Operation op);
	void ParseResponse(const char* response);

	TCPInterface* tcp;
	HTTPConnection2* http;
	NatPunchthroughClient* np_client;
	std::queue<Operation> requests;
	Operation current_op;
	int timestamp, version, version2;
	bool np_attached;
};
