#include "Pch.h"
#include <json.hpp>
#include "GameCore.h"
#include "LobbyApi.h"
#include <slikenet\TCPInterface.h>
#include <slikenet\HTTPConnection.h>
#include "Game.h"
#include "GlobalGui.h"
#include "PickServerPanel.h"

cstring LobbyApi::API_URL = "localhost"; // "http://carpglobby.westeurope.cloudapp.azure.com:8080/";
const int LobbyApi::API_PORT = 8080;

cstring op_names[] = {
	"NONE",
	"GET_SERVERS",
	"GET_CHANGES",
	"GET_VERSION",
	"IGNORE"
};

LobbyApi::LobbyApi()
{
	tcp = TCPInterface::GetInstance();
	tcp->Start(0, 1);

	http = HTTPConnection::GetInstance();
	http->Init(tcp, API_URL, API_PORT);
}

LobbyApi::~LobbyApi()
{
	HTTPConnection::DestroyInstance(http);
	TCPInterface::DestroyInstance(tcp);
}

void LobbyApi::Update(float dt)
{
	if(current_op == NONE)
		return;

	Packet* packet = tcp->Receive();
	if(packet)
	{
		http->ProcessTCPPacket(packet);
		tcp->DeallocatePacket(packet);
		http->Update();

		if(current_op != IGNORE)
		{
			int code;
			if(http->HasBadResponse(&code, nullptr))
			{
				Error("LobbyApi: Bad server response %d for %s.", code, op_names[current_op]);
				if(current_op == GET_SERVERS || current_op == GET_CHANGES)
					Game::Get().gui->pick_server->HandleBadRequest();
			}
			else
				ParseResponse();
		}

		if(!requests.empty())
		{
			DoOperation(requests.back());
			requests.pop();
		}
		else
			current_op = NONE;
	}
	
	http->Update();
}

void LobbyApi::ParseResponse()
{
	RakString str = http->Read();
	auto j = nlohmann::json::parse(str.C_String());
	if(j["ok"] == false)
	{
		string& error = j["error"].get_ref<string&>();
		Error("LobbyApi: Server returned error for method %s: %s", op_names[current_op], error.c_str());
		requests.pop();
		return;
	}

	switch(current_op)
	{
	case GET_SERVERS:
		{
			timestamp = j["timestamp"].get<int>();
			Game::Get().gui->pick_server->HandleGetServers(j);
			
		}
		break;
	case GET_CHANGES:
		{
			timestamp = j["timestamp"].get<int>();
			Game::Get().gui->pick_server->HandleGetChanges(j);
		}
		break;
	}
}

void LobbyApi::Reset()
{
	while(!requests.empty())
		requests.pop();
	if(current_op != NONE)
		current_op = IGNORE;
}

void LobbyApi::AddOperation(Operation op)
{
	if(current_op == NONE)
		DoOperation(op);
	else
		requests.push(op);
}

void LobbyApi::DoOperation(Operation op)
{
	current_op = op;
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
	}
	http->Get(path);
}
