#pragma once

//-----------------------------------------------------------------------------
struct Portal
{
	SceneNode* node;
	Vec3 pos;
	float rot;
	int at_level;
	int index;
	int target_loc;
	Portal* next_portal;

	static const int MIN_SIZE = 17;

	Portal() : node(nullptr) {}
	void Cleanup() { node = nullptr; }
	void Save(GameWriter& f);
	void Load(GameReader& f);
	Vec3 GetSpawnPos() const
	{
		return pos + Vec3(sin(rot) * 2, 0, cos(rot) * 2);
	}
};
