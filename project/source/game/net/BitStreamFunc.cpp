#include "Pch.h"
#include "GameCore.h"
#include "BitStreamFunc.h"

//=================================================================================================
BitStreamSource::BitStreamSource(BitStream& bitstream, bool write) : bitstream(bitstream), write(write)
{
	size = bitstream.GetNumberOfBytesUsed();
	offset = (write ? bitstream.GetWriteOffset() : bitstream.GetReadOffset()) / 8;
	valid = true;
}

//=================================================================================================
bool BitStreamSource::Read(void* ptr, uint data_size)
{
	assert(ptr && !write);
	if(!Ensure(data_size))
		return false;
	bitstream.Read((char*)ptr, data_size);
	offset += data_size;
	return true;
}

//=================================================================================================
bool BitStreamSource::Skip(uint data_size)
{
	assert(!write);
	if(!Ensure(data_size))
		return false;
	offset += data_size;
	uint pos = offset * 8;
	if(write)
		bitstream.SetWriteOffset(pos);
	else
		bitstream.SetReadOffset(pos);
	return true;
}

//=================================================================================================
void BitStreamSource::Write(const void* ptr, uint data_size)
{
	assert(ptr && write);
	bitstream.Write((const char*)ptr, data_size);
	size += data_size;
	offset += data_size;
}

//=================================================================================================
void BitStreamSource::SetOffset(uint offset)
{
	assert(valid && offset <= size);
	this->offset = offset;
	uint pos = this->offset * 8;
	if(write)
		bitstream.SetWriteOffset(pos);
	else
		bitstream.SetReadOffset(pos);
}

//=================================================================================================
StreamWriter&& CreateBitStreamWriter(BitStream& bitstream)
{
	BitStreamSource* source = StreamSourcePool::Get<BitStreamSource>(bitstream, true);
	return std::move(StreamWriter(source));
}

//=================================================================================================
StreamReader&& CreateBitStreamReader(BitStream& bitstream)
{
	BitStreamSource* source = StreamSourcePool::Get<BitStreamSource>(bitstream, false);
	return std::move(StreamReader(source));
}
