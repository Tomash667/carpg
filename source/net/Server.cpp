#include "Pch.h"
#include "Server.h"

#include <thread>

Server::Server() : initialized(false), paused(true)
{
}

void Server::CreateServerSp()
{
	CreateServer(SP, "sp", 1);
}

void Server::CreateServer(Mode mode, cstring name, int maxPlayers)
{
	assert(paused);
	assert(name && maxPlayers > 0);

	this->mode = mode;
	this->name = name;
	this->maxPlayers = maxPlayers;
	state = Lobby;

	if(!initialized)
	{
		initialized = true;
		paused = false;
		serverThread = thread(&Server::Loop, this);
	}
	else
	{

	}
}

void Server::Loop()
{

}
