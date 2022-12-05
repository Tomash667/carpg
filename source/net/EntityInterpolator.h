#pragma once

//-----------------------------------------------------------------------------
// used in MP to smooth units movement
struct EntityInterpolator
{
	static ObjectPool<EntityInterpolator> Pool;

	static const int MAX_ENTRIES = 4;
	struct Entry
	{
		Vec3 pos;
		float rot;
		float timer;

		void operator = (const Entry& e)
		{
			pos = e.pos;
			rot = e.rot;
			timer = e.timer;
		}
	} entries[MAX_ENTRIES];
	int validEntries;

	void Reset(const Vec3& pos, float rot);
	void Add(const Vec3& pos, float rot);
	void Update(float dt, Vec3& pos, float& rot);
};
