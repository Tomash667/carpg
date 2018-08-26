#include "Pch.h"
#include "GameCore.h"
#include "BitStreamFunc.h"

//-----------------------------------------------------------------------------
BitStreamWriter::BitStreamWriter(BitStream& bitstream) : bitstream(bitstream)
{
}

void BitStreamWriter::Write(const void* ptr, uint size)
{
	bitstream.Write((const char*)ptr, size);
}

uint BitStreamWriter::GetPos() const
{
	return BITS_TO_BYTES(bitstream.GetWriteOffset());
}

bool BitStreamWriter::SetPos(uint pos)
{
	uint size = bitstream.GetNumberOfBytesUsed();
	if(pos > size)
		return false;
	bitstream.SetWriteOffset(BYTES_TO_BITS(pos));
	return true;
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
