#pragma once

//-----------------------------------------------------------------------------
struct BulletCallback;

//-----------------------------------------------------------------------------
struct Bullet : public EntityType<Bullet>
{
	Unit* owner;
	Ability* ability;
	Vec3 pos, rot, startPos;
	Mesh* mesh;
	float speed, timer, attack, texSize, yspeed, poisonAttack, backstab;
	int level;
	TexturePtr tex;
	TrailParticleEmitter* trail;
	ParticleEmitter* pe;
	bool isArrow;

	static const int MIN_SIZE = 41;

	bool Update(float dt, LocationPart& locPart);
	void Save(GameWriter& f) const;
	void Load(GameReader& f);
	void Write(BitStreamWriter& f) const;
	bool Read(BitStreamReader& f, LevelPart& lvl);

private:
	void OnHit(LocationPart& locPart, Unit* hitted, const Vec3& hitpoint, BulletCallback& callback);
};
