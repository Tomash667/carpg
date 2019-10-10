#pragma once

//-----------------------------------------------------------------------------
#include <slikenet\PluginInterface2.h>

//-----------------------------------------------------------------------------
class PacketLogger : public SLNet::PluginInterface2
{
public:
	PacketLogger();
	PluginReceiveResult OnReceive(Packet* packet) override;

private:
	FileWriter file;
};
