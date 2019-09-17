#pragma once

//-----------------------------------------------------------------------------
#undef IGNORE

//-----------------------------------------------------------------------------
namespace SLNet
{
	class NatPunchthroughClient;
}

//-----------------------------------------------------------------------------
typedef void CURLM;

//-----------------------------------------------------------------------------
class LobbyApi
{
public:
	enum Operation
	{
		NONE,
		GET_SERVERS,
		GET_CHANGES,
		GET_VERSION,
		GET_VERSION2,
		IGNORE,
		REPORT,
		GET_CHANGELOG
	};

	enum class Status
	{
		NONE,
		DISCARD,
		DONT_DISCARD,
		KEEP_ALIVE,
		DONE,
		FAILED
	};

	struct Op : public ObjectPoolProxy<Op>
	{
		Operation o;
		int value;
		string str;
		Buffer* buf;
		Status status;

		void OnGet()
		{
			buf = nullptr;
			status = Status::NONE;
		}
		void OnFree() { if(buf) buf->Free(); }
	};

	LobbyApi();
	~LobbyApi();
	void Init(Config& cfg);
	void Update();
	void Reset();
	void GetServers() { AddOperation(GET_SERVERS); }
	void GetChanges() { AddOperation(GET_CHANGES); }
	int GetVersion(delegate<bool()> cancel_clbk);
	bool GetChangelog(string& changelog, delegate<bool()> cancel_clbk);
	void StartPunchthrough(RakNetGUID* target);
	void EndPunchthrough();
	void Report(int id, cstring text);
	cstring GetApiUrl() const { return lobby_url.c_str(); }
	int GetProxyPort() const { return proxy_port; }

private:
	void UpdateInternal();
	void SetStatus(Op* op, bool ok);
	Op* AddOperation(Operation o);
	void AddOperation(Op* op);
	void DoOperation(Op* op);
	void ParseResponse(Op* op);

	static const int MAX_REQUESTS = 4;
	NatPunchthroughClient* np_client;
	std::queue<Op*> requests;
	vector<Op*> active_requests;
	string changelog, lobby_url;
	int timestamp, lobby_port, proxy_port;
	bool np_attached;
	CURLM* cm;
};
