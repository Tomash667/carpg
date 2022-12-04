#include "Pch.h"
#include "LobbyApi.h"

#include "GameGui.h"
#include "Language.h"
#include "Net.h"
#include "PickServerPanel.h"

#include <Config.h>
#include <curl\curl.h>
#include <json.hpp>
#include <slikenet\NatPunchthroughClient.h>

LobbyApi* api;

cstring op_names[] = {
	"NONE",
	"GET_SERVERS",
	"GET_CHANGES",
	"GET_VERSION",
	"IGNORE",
	"REPORT"
};

LobbyApi::LobbyApi() : np_client(nullptr), np_attached(false), cm(nullptr)
{
}

LobbyApi::~LobbyApi()
{
	if(cm)
	{
		curl_multi_cleanup(cm);
		curl_global_cleanup();
	}

	if(np_client)
		NatPunchthroughClient::DestroyInstance(np_client);
}

void LobbyApi::Init(Config& cfg)
{
	lobby_url = cfg.GetString("lobbyUrl", "carpglobby.westeurope.cloudapp.azure.com");
	lobby_port = cfg.GetInt("lobbyPort", 8080);
	proxy_port = cfg.GetInt("proxyPort", 60481);

	curl_global_init(CURL_GLOBAL_ALL);
	cm = curl_multi_init();
	curl_multi_setopt(cm, CURLMOPT_MAXCONNECTS, MAX_REQUESTS);
}

void LobbyApi::Update()
{
	if(active_requests.empty())
		return;
	UpdateInternal();
}

void LobbyApi::UpdateInternal()
{
	int still_alive, msgs_left;
	curl_multi_perform(cm, &still_alive);

	CURLMsg* msg;
	while((msg = curl_multi_info_read(cm, &msgs_left)) != nullptr)
	{
		CURL* eh = msg->easy_handle;
		Op* op;
		int code;
		curl_easy_getinfo(eh, CURLINFO_RESPONSE_CODE, &code);
		curl_easy_getinfo(eh, CURLINFO_PRIVATE, &op);

		if(op->status == Status::DISCARD)
		{
			// ignore
		}
		else if(msg->data.result != CURLE_OK)
		{
			Error("LobbyApi: Request failed %d for %s.", msg->data.result, op_names[op->o]);
			SetStatus(op, false);
		}
		else if(code >= 400 || code == 0)
		{
			Error("LobbyApi: Bad server response %d for %s.", code, op_names[op->o]);
			SetStatus(op, false);
		}
		else if(op->o == REPORT)
		{
			// response not expected
		}
		else
		{
			try
			{
				ParseResponse(op);
			}
			catch(const nlohmann::json::exception& ex)
			{
				Error("LobbyApi: Failed to parse response for %s: %s\n%s", op_names[op->o], ex.what(), op->buf->AsString());
				SetStatus(op, false);
			}
		}

		for(auto it = active_requests.begin(), end = active_requests.end(); it != end; ++it)
		{
			if(*it == op)
			{
				active_requests.erase(it);
				if(op->status < Status::KEEP_ALIVE)
					op->Free();
				break;
			}
		}

		if(!requests.empty())
		{
			DoOperation(requests.front());
			requests.pop();
		}

		curl_multi_remove_handle(cm, eh);
		curl_easy_cleanup(eh);
	}
}

void LobbyApi::SetStatus(Op* op, bool ok)
{
	if(op->status == Status::KEEP_ALIVE)
		op->status = (ok ? Status::DONE : Status::FAILED);
	if(!ok && (op->o == GET_SERVERS || op->o == GET_CHANGES))
		gameGui->pickServer->HandleBadRequest();
}

void LobbyApi::ParseResponse(Op* op)
{
	cstring content = op->buf->AsString();

	auto j = nlohmann::json::parse(content);
	if(j["ok"] == false)
	{
		string& error = j["error"].get_ref<string&>();
		Error("LobbyApi: Server returned error for method %s: %s", op_names[op->o], error.c_str());
		SetStatus(op, false);
		return;
	}

	switch(op->o)
	{
	case GET_SERVERS:
		if(gameGui->pickServer->HandleGetServers(j))
			timestamp = j["timestamp"].get<int>();
		break;
	case GET_CHANGES:
		if(gameGui->pickServer->HandleGetChanges(j))
			timestamp = j["timestamp"].get<int>();
		break;
	case GET_VERSION:
		op->value = j["version"].get<int>();
		if(!j["changelog"].is_null())
			changelog = j["changelog"].get<string>();
		update = j["update"].get<bool>();
		break;
	}

	SetStatus(op, true);
}

void LobbyApi::Reset()
{
	if(!requests.empty())
	{
		std::queue<Op*> copy;
		while(!requests.empty())
		{
			Op* op = requests.front();
			if(op->status >= Status::DONT_DISCARD)
				copy.push(op);
			else
				op->Free();
			requests.pop();
		}
		requests = copy;
	}

	for(Op* op : active_requests)
	{
		if(op->status == Status::NONE)
			op->status = Status::DISCARD;
	}
}

LobbyApi::Op* LobbyApi::AddOperation(Operation o)
{
	Op* op = Op::Get();
	op->o = o;
	op->value = 0;
	op->str.clear();
	AddOperation(op);
	return op;
}

void LobbyApi::AddOperation(Op* op)
{
	if(op->o == GET_CHANGES && timestamp == 0)
		op->o = GET_SERVERS;
	if(active_requests.size() < MAX_REQUESTS)
		DoOperation(op);
	else
		requests.push(op);
}

static size_t WriteMemoryCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
	LobbyApi::Op* op = reinterpret_cast<LobbyApi::Op*>(userp);

	if(!op->buf)
	{
		op->buf = Buffer::Get();
		op->buf->Clear();
	}

	size_t realsize = size * nmemb;
	op->buf->Append(contents, realsize);
	return realsize;
}

void LobbyApi::DoOperation(Op* op)
{
	CURL* eh = curl_easy_init();
	cstring url;

	if(op->o == REPORT)
	{
		string text = UrlEncode(op->str);
		url = Format("carpg.pl/reports.php?action=add&id=%d&text=%s&ver=%s", op->value, text.c_str(), VERSION_STR);
	}
	else
	{
		cstring path;
		switch(op->o)
		{
		default:
			assert(0);
		case GET_SERVERS:
			path = "/api/servers";
			break;
		case GET_CHANGES:
			path = Format("/api/servers/%d", timestamp);
			break;
		case GET_VERSION:
			path = Format("/api/version?ver=%d&lang=%s", VERSION, Language::prefix.c_str());
			break;
		}
		if(lobby_port != 80)
			url = Format("%s:%d%s", lobby_url.c_str(), lobby_port, path);
		else
			url = Format("%s%s", lobby_url.c_str(), path);
	}

	curl_easy_setopt(eh, CURLOPT_URL, url);
	curl_easy_setopt(eh, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
	curl_easy_setopt(eh, CURLOPT_WRITEDATA, op);
	curl_easy_setopt(eh, CURLOPT_PRIVATE, op);
	curl_easy_setopt(eh, CURLOPT_TIMEOUT, 4);
	curl_easy_setopt(eh, CURLOPT_CONNECTTIMEOUT, 4);
	curl_multi_add_handle(cm, eh);
	active_requests.push_back(op);
}

class NatPunchthroughDebugInterface_InfoLogger : public NatPunchthroughDebugInterface
{
public:
	void OnClientMessage(const char *msg) override
	{
		Info("NAT: %s", msg);
	}
};

void LobbyApi::StartPunchthrough(RakNetGUID* target)
{
	assert(!np_attached);

	if(!np_client)
	{
		static NatPunchthroughDebugInterface_InfoLogger logger;
		np_client = NatPunchthroughClient::GetInstance();
		np_client->SetDebugInterface(&logger);
	}

	net->peer->AttachPlugin(np_client);
	np_attached = true;

	if(target)
		np_client->OpenNAT(*target, net->master_server_adr);
	else
		np_client->FindRouterPortStride(net->master_server_adr);
}

void LobbyApi::EndPunchthrough()
{
	if(np_attached)
	{
		net->peer->DetachPlugin(np_client);
		np_attached = false;
	}
}

int LobbyApi::GetVersion(delegate<bool()> cancel_clbk, string& changelog, bool& update)
{
	Op* op = AddOperation(GET_VERSION);
	op->status = Status::KEEP_ALIVE;

	while(true)
	{
		if(cancel_clbk())
			return -2;

		UpdateInternal();
		if(op->status != Status::KEEP_ALIVE)
			break;

		Sleep(30);
	}

	int version;
	if(op->status == Status::DONE)
	{
		version = op->value;
		changelog = this->changelog;
		update = this->update;
	}
	else
	{
		Error("LobbyApi: Failed to get version.");
		version = -1;
	}

	op->Free();
	return version;
}

void LobbyApi::Report(int id, cstring text)
{
	Op* op = Op::Get();
	op->o = REPORT;
	op->status = Status::DONT_DISCARD;
	op->value = id;
	op->str = text;
	AddOperation(op);
}
