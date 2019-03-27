#include "Pch.h"
#include <json.hpp>
#include "GameCore.h"
#include "LobbyApi.h"
#include <slikenet\TCPInterface.h>
#include <slikenet\HTTPConnection2.h>
#include <slikenet\NatPunchthroughClient.h>
#include "Game.h"
#include "GlobalGui.h"
#include "PickServerPanel.h"
#include <sstream>

cstring LobbyApi::API_URL = "carpglobby.westeurope.cloudapp.azure.com";
const int LobbyApi::API_PORT = 8080;
const int LobbyApi::PROXY_PORT = 60481;

cstring op_names[] = {
	"NONE",
	"GET_SERVERS",
	"GET_CHANGES",
	"GET_VERSION",
	"IGNORE",
	"REPORT"
};

LobbyApi::LobbyApi() : np_client(nullptr), np_attached(false), current_op(NONE)
{
	tcp = TCPInterface::GetInstance();
	http = HTTPConnection2::GetInstance();
	tcp->AttachPlugin(http);
	tcp->Start(0, 2);
}

LobbyApi::~LobbyApi()
{
	if(np_client)
		NatPunchthroughClient::DestroyInstance(np_client);
	TCPInterface::DestroyInstance(tcp);
	HTTPConnection2::DestroyInstance(http);
}

void LobbyApi::Update()
{
	if(current_op == NONE)
		return;
	UpdateInternal();
}

void LobbyApi::UpdateInternal()
{
	Packet* packet;
	tcp->HasCompletedConnectionAttempt();
	for(packet = tcp->Receive(); packet; tcp->DeallocatePacket(packet), packet = tcp->Receive())
		;
	tcp->HasFailedConnectionAttempt();
	tcp->HasLostConnection();

	while(http->HasResponse())
	{
		HTTPConnection2::Request* request;
		http->GetRawResponse(request);
		Operation received_op = (Operation)(int)request->userData;
		if(received_op == current_op)
		{
			int code = request->GetStatusCode();
			if(code >= 400 || code == 0)
			{
				Error("LobbyApi: Bad server response %d for %s.", code, op_names[current_op]);
				if(current_op == GET_SERVERS || current_op == GET_CHANGES)
					Game::Get().gui->pick_server->HandleBadRequest();
			}
			else if(current_op == REPORT)
			{
				// do nothing
			}
			else if(request->binaryData)
			{
				if(current_op != GET_VERSION || request->contentLength != 8)
					Error("LobbyApi: Binary response for %s.", code, op_names[current_op]);
				else
				{
					int* data = (int*)request->binaryData;
					if(data[0] != 0x475052CA)
					{
						Error("LobbyApi: Invalid binary version sign 0x%p.", data[0]);
						Error(request->stringReceived.C_String());
						version2 = -1;
					}
					else
						version2 = (data[1] & 0xFFFFFF);
				}
			}
			else
			{
				try
				{
					ParseResponse(request->stringReceived.C_String() + request->contentOffset);
				}
				catch(const nlohmann::json::parse_error& ex)
				{
					Error("LobbyApi: Failed to parse response for %s: %s", op_names[current_op], ex.what());
				}
			}
		}
		OP_DELETE(request, _FILE_AND_LINE_);

		if(current_op != GET_VERSION)
		{
			if(!requests.empty())
			{
				DoOperation(requests.front());
				requests.pop();
			}
			else
				current_op = NONE;
		}
	}
}

void LobbyApi::ParseResponse(const char* response)
{
	auto j = nlohmann::json::parse(response);
	if(j["ok"] == false)
	{
		string& error = j["error"].get_ref<string&>();
		Error("LobbyApi: Server returned error for method %s: %s", op_names[current_op], error.c_str());
		return;
	}

	switch(current_op)
	{
	case GET_SERVERS:
		if(Game::Get().gui->pick_server->HandleGetServers(j))
			timestamp = j["timestamp"].get<int>();
		break;
	case GET_CHANGES:
		if(Game::Get().gui->pick_server->HandleGetChanges(j))
			timestamp = j["timestamp"].get<int>();
		break;
	case GET_VERSION:
		version = j["version"].get<int>();
		break;
	}
}

void LobbyApi::Reset()
{
	if(!requests.empty())
	{
		std::queue<Op> copy;
		while(!requests.empty())
		{
			if(requests.front().op == REPORT)
				copy.push(requests.front());
			requests.pop();
		}
		requests = copy;
	}
	if(current_op != NONE)
		current_op = IGNORE;
}

void LobbyApi::AddOperation(Op opp)
{
	if(opp.op == GET_CHANGES && timestamp == 0)
		opp.op = GET_SERVERS;
	if(current_op == NONE)
		DoOperation(opp);
	else
		requests.push(opp);
}

std::string urlencode(const std::string &s)
{
	static const char lookup[] = "0123456789abcdef";
	std::stringstream e;
	for(int i = 0, ix = s.length(); i < ix; i++)
	{
		const char& c = s[i];
		if((48 <= c && c <= 57) ||//0-9
			(65 <= c && c <= 90) ||//abc...xyz
			(97 <= c && c <= 122) || //ABC...XYZ
			(c == '-' || c == '_' || c == '.' || c == '~')
			)
		{
			e << c;
		}
		else
		{
			e << '%';
			e << lookup[(c & 0xF0) >> 4];
			e << lookup[(c & 0x0F)];
		}
	}
	return e.str();
}

void LobbyApi::DoOperation(Op opp)
{
	Operation op = opp.op;
	current_op = op;

	if(op == REPORT)
	{
		string text = urlencode(*opp.str);
		cstring url = Format("carpg.pl/reports.php?action=add&id=%d&text=%s", opp.value, text.c_str());
		http->TransmitRequest(RakString::FormatForGET(url), "carpg.pl", 80, false, 4, UNASSIGNED_SYSTEM_ADDRESS, (void*)op);
	}
	else
	{
		cstring path;
		switch(op)
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
			path = "/api/version";
			break;
		}
		http->TransmitRequest(RakString::FormatForGET(Format("%s%s", API_URL, path)), API_URL, API_PORT, false, 4, UNASSIGNED_SYSTEM_ADDRESS, (void*)op);
	}
	
	if(opp.str)
		StringPool.Free(opp.str);
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

	N.peer->AttachPlugin(np_client);
	np_attached = true;

	if(target)
		np_client->OpenNAT(*target, N.master_server_adr);
	else
		np_client->FindRouterPortStride(N.master_server_adr);
}

void LobbyApi::EndPunchthrough()
{
	if(np_attached)
	{
		N.peer->DetachPlugin(np_client);
		np_attached = false;
	}
}

int LobbyApi::GetVersion(delegate<bool()> cancel_clbk)
{
	assert(!IsBusy());
	Timer t;
	float time = 0;
	version = -1;
	version2 = -1;
	DoOperation({ GET_VERSION,0,nullptr });
	http->TransmitRequest(RakString::FormatForGET("carpg.pl/carpgdata/wersja"), "carpg.pl", 80, false, 4, UNASSIGNED_SYSTEM_ADDRESS, (void*)current_op);

	while(true)
	{
		if(cancel_clbk())
			return -2;

		UpdateInternal();
		if(version >= 0)
			break;

		Sleep(30);
		time += t.Tick();
		if(time >= 2.f)
			break;
	}

	current_op = NONE;
	Reset();
	if(version >= 0)
	{
		if(version2 != -1 && version != version2)
			Warn("LobbyApi: Get version mismatch %s and %s.", VersionToString(version), VersionToString(version2));
		return version;
	}
	else if(version2 >= 0)
	{
		Warn("LobbyApi: Failed to get version from primary server. Secondary returned %s.", VersionToString(version2));
		return version2;
	}
	else
	{
		Error("LobbyApi: Failed to get version.");
		return -1;
	}
}

void LobbyApi::Report(int id, cstring text)
{
	string* str = StringPool.Get();
	*str = text;
	AddOperation({ REPORT, id, str });
}
