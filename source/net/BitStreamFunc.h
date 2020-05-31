#pragma once

//-----------------------------------------------------------------------------
#include "File.h"

//-----------------------------------------------------------------------------
class BitStreamWriter : public StreamWriter
{
public:
	BitStreamWriter();
	BitStreamWriter(BitStream& bitstream);
	~BitStreamWriter();

	using StreamWriter::Write;
	using StreamWriter::operator <<;
	void Write(const void* ptr, uint size) override;
	uint GetPos() const override;
	bool SetPos(uint pos) override;
	cstring GetData() const;
	uint GetSize() const;
	BitStream& GetBitStream() const { return bitstream; }
	void Reset();
	void WriteItemList(const vector<ItemSlot>& items);
	void WriteItemListTeam(const vector<ItemSlot>& items);

	void operator << (const Item& item);
	void operator << (const Item* item)
	{
		if(item)
			operator << (*item);
		else
			Write0();
	}

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
	BitStreamReader(Packet* packet);
	~BitStreamReader();

	using StreamReader::Read;
	void Read(void* ptr, uint size) override;
	using StreamReader::Skip;
	void Skip(uint size) override;
	uint GetPos() const override;
	uint GetSize() const override;
	bool SetPos(uint pos) override;
	BitStream& GetBitStream() const { return bitstream; }
	int ReadItemAndFind(const Item*& item);
	bool ReadItemList(vector<ItemSlot>& items);
	bool ReadItemListTeam(vector<ItemSlot>& items, bool skip = false);

private:
	BitStream& CreateBitStream(Packet* packet);

	BitStream& bitstream;
	bool pooled;
};
