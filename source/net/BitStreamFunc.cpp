#include "Pch.h"
#include "BitStreamFunc.h"

#include "GameResources.h"
#include "Item.h"
#include "ItemSlot.h"
#include "Location.h"
#include "QuestManager.h"
#include "Unit.h"
#include "World.h"

//-----------------------------------------------------------------------------
static ObjectPool<BitStream> bitstreamWritePool, bitstreamReadPool;

//-----------------------------------------------------------------------------
BitStreamWriter::BitStreamWriter() : bitstream(*bitstreamWritePool.Get()), totalSize(bitstream.GetNumberOfBytesUsed()), owned(true)
{
}

BitStreamWriter::BitStreamWriter(BitStream& bitstream) : bitstream(bitstream), totalSize(bitstream.GetNumberOfBytesUsed()), owned(false)
{
}

BitStreamWriter::~BitStreamWriter()
{
	if(owned)
	{
		bitstream.Reset();
		bitstreamWritePool.Free(&bitstream);
	}
}

void BitStreamWriter::Write(const void* ptr, uint size)
{
	bitstream.Write((const char*)ptr, size);
	uint newSize = GetPos() + size;
	if(newSize > totalSize)
		totalSize = newSize;
}

uint BitStreamWriter::GetPos() const
{
	return BITS_TO_BYTES(bitstream.GetWriteOffset());
}

bool BitStreamWriter::SetPos(uint pos)
{
	if(pos > totalSize)
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
		operator << (item.questId);
}

void BitStreamWriter::Reset()
{
	bitstream.Reset();
}

void BitStreamWriter::WriteItemList(const vector<ItemSlot>& items)
{
	operator << (items.size());
	for(const ItemSlot& slot : items)
	{
		operator << (*slot.item);
		operator << (slot.count);
	}
}

void BitStreamWriter::WriteItemListTeam(const vector<ItemSlot>& items)
{
	operator << (items.size());
	for(const ItemSlot& slot : items)
	{
		operator << (*slot.item);
		operator << (slot.count);
		operator << (slot.teamCount);
	}
}

void BitStreamWriter::operator << (Unit* unit)
{
	int id = (unit ? unit->id : -1);
	Write(id);
}

void BitStreamWriter::operator << (Location* loc)
{
	int index = (loc ? loc->index : -1);
	Write(index);
}

//-----------------------------------------------------------------------------
BitStreamReader::BitStreamReader(BitStream& bitstream) : bitstream(bitstream), pooled(false)
{
	ok = true;
}

BitStreamReader::BitStreamReader(Packet* packet) : bitstream(CreateBitStream(packet)), pooled(true)
{
	ok = true;
}

BitStreamReader::~BitStreamReader()
{
	if(pooled)
		bitstreamReadPool.Free(&bitstream);
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

BitStream& BitStreamReader::CreateBitStream(Packet* packet)
{
	BitStream* bitstream = bitstreamReadPool.Get();
	new(bitstream)BitStream(packet->data, packet->length, false);
	return *bitstream;
}

// -2 read error, -1 not found, 0 empty, 1 found
int BitStreamReader::ReadItemAndFind(const Item*& item)
{
	item = nullptr;

	const string& itemId = ReadString1();
	if(!IsOk())
		return -2;

	if(itemId.empty())
		return 0;

	if(itemId[0] == '$')
	{
		int questId = Read<int>();
		if(!IsOk())
			return -2;

		item = questMgr->FindQuestItemClient(itemId.c_str(), questId);
		if(!item)
		{
			Warn("Missing quest item '%s' (%d).", itemId.c_str(), questId);
			return -1;
		}
		else
			return 1;
	}
	else
	{
		item = Item::TryGet(itemId);
		if(!item)
		{
			Warn("Missing item '%s'.", itemId.c_str());
			return -1;
		}
		else
			return 1;
	}
}

bool BitStreamReader::ReadItemList(vector<ItemSlot>& items)
{
	const int MIN_SIZE = 5;

	uint count = Read<uint>();
	if(!Ensure(count * MIN_SIZE))
		return false;

	items.resize(count);
	for(ItemSlot& slot : items)
	{
		if(ReadItemAndFind(slot.item) < 1)
			return false;
		operator >> (slot.count);
		if(!IsOk())
			return false;
		gameRes->PreloadItem(slot.item);
		slot.teamCount = 0;
	}

	return true;
}

bool BitStreamReader::ReadItemListTeam(vector<ItemSlot>& items, bool skip)
{
	const int MIN_SIZE = 9;

	uint count;
	operator >> (count);
	if(!Ensure(count * MIN_SIZE))
		return false;

	items.resize(count);
	for(ItemSlot& slot : items)
	{
		if(ReadItemAndFind(slot.item) < 1)
			return false;
		operator >> (slot.count);
		operator >> (slot.teamCount);
		if(!IsOk())
			return false;
		if(!skip)
			gameRes->PreloadItem(slot.item);
	}

	return true;
}

void BitStreamReader::operator >> (Unit*& unit)
{
	int id = Read<int>();
	unit = Unit::GetById(id);
}

void BitStreamReader::operator >> (Location*& loc)
{
	int index = Read<int>();
	if(index == -1)
		loc = nullptr;
	else
		loc = world->GetLocation(index);
}
