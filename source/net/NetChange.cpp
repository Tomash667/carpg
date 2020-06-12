#include "Pch.h"
#include "NetChange.h"

NetChangeWriter::NetChangeWriter(NetChange& c) : data(reinterpret_cast<byte*>(&c.data)), len(c.size)
{
	len = 0;
}

void NetChangeWriter::Write(const void* ptr, uint size)
{
	assert(size + len <= sizeof(NetChange) - offsetof(NetChange, data));
	memcpy(data, ptr, size);
	len += size;
	data += size;
}
