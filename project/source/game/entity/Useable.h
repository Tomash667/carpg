#pragma once

//-----------------------------------------------------------------------------
#include "BaseUseable.h"

//-----------------------------------------------------------------------------
struct Unit;
struct Useable;

//-----------------------------------------------------------------------------
struct UseableRequest
{
	Useable** useable;
	int refid;
	Unit* user;

	UseableRequest(Useable** useable, int refid, Unit* user) : useable(useable), refid(refid), user(user)
	{

	}
};

//-----------------------------------------------------------------------------
struct Useable
{
	VEC3 pos;
	float rot;
	Unit* user;
	int type, refid, netid, variant;

	static const int MIN_SIZE = 22;

	Useable() : variant(-1) {}

	void Save(HANDLE file, bool local);
	void Load(HANDLE file, bool local);
	void Write(BitStream& stream) const;
	bool Read(BitStream& stream);

	static Useable* GetByRefid(int _refid)
	{
		if(_refid == -1 || _refid >= (int)refid_table.size())
			return nullptr;
		else
			return refid_table[_refid];
	}

	static void AddRequest(Useable** useable, int refid, Unit* user)
	{
		assert(useable && refid != -1 && user);
		refid_request.push_back(UseableRequest(useable, refid, user));
	}

	static void AddRefid(Useable* useable)
	{
		assert(useable);
		useable->refid = (int)refid_table.size();
		refid_table.push_back(useable);
	}

	BaseUsable* GetBase() const
	{
		return &g_base_usables[type];
	}
	Obj* GetBaseObj() const
	{
		return GetBase()->obj;
	}

	Animesh* GetMesh() const;

	static vector<Useable*> refid_table;
	static vector<UseableRequest> refid_request;
};
