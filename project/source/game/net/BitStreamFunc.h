#pragma once

//=================================================================================================
#include "File.h"

//=================================================================================================
class BitStreamWriter : public StreamWriter
{
public:
	BitStreamWriter();
	BitStreamWriter(BitStream& bitstream);
	~BitStreamWriter();

	using StreamWriter::Write;
	void Write(const void* ptr, uint size) override;
	uint GetPos() const override;
	bool SetPos(uint pos) override;
	cstring GetData() const;
	uint GetSize() const;
	BitStream& GetBitStream() const { return bitstream; }

private:
	BitStream& bitstream;
	uint total_size;
	bool owned;
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
	BitStream& GetBitStream() const { return bitstream; }

private:
	BitStream& bitstream;
};
