#include "Pch.h"
#include <json.hpp>
#include "GameCore.h"
#include "LobbyApi.h"
#include <slikenet\TCPInterface.h>
#include <slikenet\HTTPConnection.h>

using json = nlohmann::json;

cstring API_URL = "http://carpglobby.westeurope.cloudapp.azure.com:8080/";
const float PING_TIMER = 10.f;
const float PACKET_TIMER = 2.f;

LobbyApi::LobbyApi()
{
	server_id = -1;

	tcp = TCPInterface::GetInstance();
	tcp->Start(0, 1);

	http = HTTPConnection::GetInstance();
	http->Init(tcp, API_URL);
}

LobbyApi::~LobbyApi()
{
	HTTPConnection::DestroyInstance(http);
	TCPInterface::DestroyInstance(tcp);
}

void LobbyApi::Update(float dt)
{
	// ping server to keep it alive
	if(server_id != -1)
	{
		timer += dt;
		if(timer >= PING_TIMER && requests.empty())
		{
			http->Request(HTTPConnection::HTTP_PUT, Format("api/servers/ping/%d?key=%s", server_id, key.c_str()));
			requests.push(PING_SERVER);
		}
	}

	http->Update();

	if(!requests.empty())
	{
		Packet* packet = tcp->Receive();
		if(packet)
		{
			http->ProcessTCPPacket(packet);
			tcp->DeallocatePacket(packet);

			int code;
			if(http->HasBadResponse(&code, nullptr))
			{
				Error("LobbyApi: Bad server response %d for %d.", code, requests.front());
				requests.pop();
			}
			else
				ParseResponse();
		}
	}
}

void LobbyApi::ParseResponse()
{
	RakString str = http->Read();
	json j = json::parse(str.C_String());
	if(j["Ok"] == false)
	{
		string& error = j["Error"].get_ref<string&>();
		Error("LobbyApi: Server returned error for method %d: %s", requests.front(), error.c_str());
		requests.pop();
		return;
	}

	Method method = requests.front();
	requests.pop();
	switch(method)
	{
	case CREATE_SERVER:
		server_id = j["ServerID"].get<int>();
		key = j["Key"].get<string>();
		timer = 0;
		Info("LobbyApi: Server %d created.", server_id);
		break;
	case UPDATE_SERVER:
	case PING_SERVER:
		timer = 0;
		break;
	case REMOVE_SERVER:
		Info("LobbyApi: Server removed.");
		server_id = -1;
		break;
	}

	return;
}

void LobbyApi::RegisterServer(const string& name, int max_players, int flags, int port)
{
	json j = {
		{"Name", name},
		{"Players", max_players},
		{"Flags", flags},
		{"Port", port}
	};
	string str = j.dump();

	http->Post("api/servers", str.c_str(), "application/json");
	requests.push(CREATE_SERVER);
}

void LobbyApi::UpdateServer(int players)
{
	if(server_id != -1)
	{
		http->Request(HTTPConnection::HTTP_PUT, Format("api/servers/%d?key=%s&players=%d", server_id, key.c_str(), players));
		requests.push(UPDATE_SERVER);
	}
}

void LobbyApi::UnregisterServer()
{
	http->Request(HTTPConnection::HTTP_DELETE, Format("api/servers/%d&key=%s", server_id, key.c_str()));
	requests.push(REMOVE_SERVER);
}
