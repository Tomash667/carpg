#pragma once

//-----------------------------------------------------------------------------
struct BulletCallback;

//-----------------------------------------------------------------------------
struct Bullet : public EntityType<Bullet>
{
	Unit* owner;
	Ability* ability;
	Vec3 pos, rot, start_pos;
	Mesh* mesh;
	float speed, timer, attack, tex_size, yspeed, poison_attack, backstab;
	int level;
	TexturePtr tex;
	TrailParticleEmitter* trail;
	ParticleEmitter* pe;

	static const int MIN_SIZE = 41;

	bool Update(float dt, LevelArea& area);
	void Save(GameWriter& f) const;
	void Load(GameReader& f);
	void Write(BitStreamWriter& f) const;
	bool Read(BitStreamReader& f, TmpLevelArea& tmp_area);

private:
	void OnHit(LevelArea& area, Unit* hitted, const Vec3& hitpoint, BulletCallback& callback);
};
