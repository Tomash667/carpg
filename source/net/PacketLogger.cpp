#include "Pch.h"
#include "GameCore.h"
#include "PacketLogger.h"

//=================================================================================================
PacketLogger::PacketLogger()
{
	io::CreateDirectory("packets");

	time_t t = time(0);
	tm tm;
	localtime_s(&tm, &t);
	cstring path = Format("packets/log_%02d-%02d-%02d_%02d-%02d-%02d.bin",
		tm.tm_year - 100, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	file.Open(path);
	Info("Using packet logger '%s'.", path);
}

//=================================================================================================
PluginReceiveResult PacketLogger::OnReceive(Packet *packet)
{
	file << 0x4C455744;
	file << packet->data[0];
	file << time(0);
	file << SystemAddress::ToInteger(packet->systemAddress);
	file << packet->length;
	if(packet->length >= 1000)
	{
		Buffer* buf = io::Compress(packet->data, packet->length);
		file << buf->Size();
		file.Write(buf->Data(), buf->Size());
		buf->Free();
	}
	else
	{
		file << 0;
		file.Write(packet->data, packet->length);
	}
	return RR_CONTINUE_PROCESSING;
}
