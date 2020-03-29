#include "Pch.h"
#include "NetChange.h"

NetChangeWriter::NetChangeWriter(NetChange& c)
	: data(((byte*)&c) + sizeof(NetChange::TYPE) + sizeof(byte)), len(*(((byte*)&c) + sizeof(NetChange::TYPE)))
{
	len = 0;
}

void NetChangeWriter::Write(const void* ptr, uint size)
{
	assert(size + len <= sizeof(NetChange) - sizeof(NetChange::TYPE) - sizeof(byte));
	memcpy(data, ptr, size);
	len += size;
	data += size;
}
