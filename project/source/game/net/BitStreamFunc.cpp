#include "Pch.h"
#include "GameCore.h"
#include "BitStreamFunc.h"
#include "Item.h"

//-----------------------------------------------------------------------------
static ObjectPool<BitStream> bitstream_pool;

//-----------------------------------------------------------------------------
BitStreamWriter::BitStreamWriter() : bitstream(*bitstream_pool.Get()), total_size(bitstream.GetNumberOfBytesUsed()), owned(true)
{
}

BitStreamWriter::BitStreamWriter(BitStream& bitstream) : bitstream(bitstream), total_size(bitstream.GetNumberOfBytesUsed()), owned(false)
{
}

BitStreamWriter::~BitStreamWriter()
{
	if(owned)
	{
		bitstream.Reset();
		bitstream_pool.Free(&bitstream);
	}
}

void BitStreamWriter::Write(const void* ptr, uint size)
{
	bitstream.Write((const char*)ptr, size);
	uint new_size = GetPos() + size;
	if(new_size > total_size)
		total_size = new_size;
}

uint BitStreamWriter::GetPos() const
{
	return BITS_TO_BYTES(bitstream.GetWriteOffset());
}

bool BitStreamWriter::SetPos(uint pos)
{
	if(pos > total_size)
		return false;
	bitstream.SetWriteOffset(BYTES_TO_BITS(pos));
	return true;
}

cstring BitStreamWriter::GetData() const
{
	return (cstring)bitstream.GetData();
}

uint BitStreamWriter::GetSize() const
{
	return bitstream.GetNumberOfBytesUsed();
}

void BitStreamWriter::operator << (const Item& item)
{
	operator << (item.id);
	if(item.id[0] == '$')
		operator << (item.refid);
}

//-----------------------------------------------------------------------------
BitStreamReader::BitStreamReader(BitStream& bitstream) : bitstream(bitstream)
{
	ok = true;
}

void BitStreamReader::Read(void* ptr, uint size)
{
	if(!ok || GetPos() + size > GetSize())
		ok = false;
	else
		bitstream.Read((char*)ptr, size);
}

void BitStreamReader::Skip(uint size)
{
	if(!ok || GetPos() + size > GetSize())
		ok = false;
	else
		bitstream.SetReadOffset(BYTES_TO_BITS(GetPos() + size));
}

uint BitStreamReader::GetPos() const
{
	return BITS_TO_BYTES(bitstream.GetReadOffset());
}

uint BitStreamReader::GetSize() const
{
	return bitstream.GetNumberOfBytesUsed();
}

bool BitStreamReader::SetPos(uint pos)
{
	if(!ok || pos > GetSize())
		ok = false;
	else
		bitstream.SetReadOffset(BYTES_TO_BITS(pos));
	return ok;
}
