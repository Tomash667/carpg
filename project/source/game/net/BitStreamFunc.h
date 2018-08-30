#pragma once

//=================================================================================================
#include "File.h"

//=================================================================================================
class BitStreamWriter : public StreamWriter
{
public:
	BitStreamWriter(BitStream& bitstream);

	using StreamWriter::Write;
	void Write(const void* ptr, uint size) override;
	uint GetPos() const override;
	bool SetPos(uint pos) override;

private:
	BitStream& bitstream;
};

//=================================================================================================
class BitStreamReader : public StreamReader
{
public:
	BitStreamReader(BitStream& bitstream);

	using StreamReader::Read;
	void Read(void* ptr, uint size) override;
	using StreamReader::Skip;
	void Skip(uint size) override;
	uint GetPos() const override;
	uint GetSize() const override;
	bool SetPos(uint pos) override;

private:
	BitStream& bitstream;
};
