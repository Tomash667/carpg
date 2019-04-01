#pragma once

//-----------------------------------------------------------------------------
struct GroundItem
{
	int netid, refid;
	const Item* item;
	uint count, team_count;
	Vec3 pos;
	float rot;

	static const int MIN_SIZE = 23;
	static int netid_counter;

	void Save(FileWriter& f);
	void Load(FileReader& f);
	void Write(BitStreamWriter& f);
	bool Read(BitStreamReader& f);

	static vector<GroundItem*> refid_table;
	static GroundItem* GetByRefid(int refid)
	{
		if(refid == -1 || refid >= (int)refid_table.size())
			return nullptr;
		else
			return refid_table[refid];
	}
	static void AddRefid(GroundItem* item)
	{
		assert(item);
		item->refid = (int)refid_table.size();
		refid_table.push_back(item);
	}
};
