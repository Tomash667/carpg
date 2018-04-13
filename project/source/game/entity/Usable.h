#pragma once

//-----------------------------------------------------------------------------
#include "BaseUsable.h"
#include "ItemContainer.h"

//-----------------------------------------------------------------------------
struct Unit;
struct Usable;

//-----------------------------------------------------------------------------
struct UsableRequest
{
	Usable** usable;
	int refid;
	Unit* user;

	UsableRequest(Usable** usable, int refid, Unit* user) : usable(usable), refid(refid), user(user)
	{
	}
};

//-----------------------------------------------------------------------------
struct Usable
{
	int netid;
	BaseUsable* base;
	Vec3 pos;
	float rot;
	Unit* user;
	ItemContainer* container;
	int refid, variant;

	static const int MIN_SIZE = 22;
	static int netid_counter;

	Usable() : variant(-1), container(nullptr) {}
	~Usable() { delete container; }

	void Save(HANDLE file, bool local);
	void Load(HANDLE file, bool local);
	void Write(BitStream& stream) const;
	bool Read(BitStream& stream);

	static Usable* GetByRefid(int _refid)
	{
		if(_refid == -1 || _refid >= (int)refid_table.size())
			return nullptr;
		else
			return refid_table[_refid];
	}

	static void AddRequest(Usable** usable, int refid, Unit* user)
	{
		assert(usable && refid != -1);
		refid_request.push_back(UsableRequest(usable, refid, user));
	}

	static void AddRefid(Usable* usable)
	{
		assert(usable);
		usable->refid = (int)refid_table.size();
		refid_table.push_back(usable);
	}

	Mesh* GetMesh() const;

	static vector<Usable*> refid_table;
	static vector<UsableRequest> refid_request;
};
