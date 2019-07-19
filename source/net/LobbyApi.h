#pragma once

//-----------------------------------------------------------------------------
#include "Timer.h"

//-----------------------------------------------------------------------------
#undef IGNORE

//-----------------------------------------------------------------------------
namespace SLNet
{
	class HTTPConnection2;
	class NatPunchthroughClient;
	class TCPInterface;
}

//-----------------------------------------------------------------------------
class LobbyApi
{
	enum Operation
	{
		NONE,
		GET_SERVERS,
		GET_CHANGES,
		GET_VERSION,
		IGNORE,
		REPORT,
		GET_CHANGELOG
	};

	struct Op
	{
		Operation op;
		int value;
		string* str;
	};

public:
	LobbyApi();
	~LobbyApi();
	void Update();
	void Reset();
	void GetServers() { AddOperation({ GET_SERVERS, 0, nullptr }); }
	void GetChanges() { AddOperation({ GET_CHANGES, 0, nullptr }); }
	bool IsBusy() const { return current_op != NONE; }
	int GetVersion(delegate<bool()> cancel_clbk);
	bool GetChangelog(string& changelog, delegate<bool()> cancel_clbk);
	void StartPunchthrough(RakNetGUID* target);
	void EndPunchthrough();
	void Report(int id, cstring text);

	static cstring API_URL;
	static const int API_PORT;
	static const int PROXY_PORT;

private:
	void UpdateInternal();
	void AddOperation(Op op);
	void DoOperation(Op op);
	void ParseResponse(const char* response);

	TCPInterface* tcp;
	HTTPConnection2* http;
	NatPunchthroughClient* np_client;
	std::queue<Op> requests;
	Operation current_op;
	string changelog;
	int timestamp, version;
	bool np_attached, last_request_failed;
};
