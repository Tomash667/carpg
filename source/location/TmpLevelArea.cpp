#include "Pch.h"
#include "TmpLevelArea.h"

#include "BitStreamFunc.h"
#include "Bullet.h"
#include "Electro.h"
#include "Explo.h"

#include <BasicScene.h>
#include <ParticleSystem.h>

//=================================================================================================
TmpLevelArea::TmpLevelArea()
{
	scene = new BasicScene;
}

//=================================================================================================
TmpLevelArea::~TmpLevelArea()
{
	delete scene;
}

//=================================================================================================
void TmpLevelArea::Clear()
{
	DeleteElements(explos);
	DeleteElements(electros);
	DeleteElements(bullets);
	drains.clear();
	colliders.clear();
	DeleteElements(pes);
	DeleteElements(tpes);
	scene->Clear();
}

//=================================================================================================
void TmpLevelArea::Save(GameWriter& f)
{
	f << pes.size();
	for(ParticleEmitter* pe : pes)
		pe->Save(f);

	f << tpes.size();
	for(TrailParticleEmitter* tpe : tpes)
		tpe->Save(f);

	f << explos.size();
	for(Explo* explo : explos)
		explo->Save(f);

	f << electros.size();
	for(Electro* electro : electros)
		electro->Save(f);

	f << drains.size();
	for(Drain& drain : drains)
		drain.Save(f);

	f << bullets.size();
	for(Bullet* bullet : bullets)
		bullet->Save(f);
}

//=================================================================================================
void TmpLevelArea::Load(GameReader& f)
{
	const int particle_version = (LOAD_VERSION >= V_0_13 ? 2 : (LOAD_VERSION >= V_0_12 ? 1 : 0));

	pes.resize(f.Read<uint>());
	for(ParticleEmitter*& pe : pes)
	{
		pe = new ParticleEmitter;
		pe->Load(f, particle_version);
	}

	tpes.resize(f.Read<uint>());
	for(TrailParticleEmitter*& tpe : tpes)
	{
		tpe = new TrailParticleEmitter;
		tpe->Load(f, particle_version);
	}

	explos.resize(f.Read<uint>());
	for(Explo*& explo : explos)
	{
		explo = new Explo;
		explo->Load(f);
	}

	electros.resize(f.Read<uint>());
	for(Electro*& electro : electros)
	{
		electro = new Electro;
		electro->area = area;
		electro->Load(f);
	}

	drains.resize(f.Read<uint>());
	for(Drain& drain : drains)
		drain.Load(f);

	bullets.resize(f.Read<uint>());
	for(Bullet*& bullet : bullets)
	{
		bullet = new Bullet;
		bullet->Load(f);
	}
}

//=================================================================================================
void TmpLevelArea::Write(BitStreamWriter& f)
{
	// bullets
	f.Write(bullets.size());
	for(Bullet* bullet : bullets)
		bullet->Write(f);

	// explosions
	f.Write(explos.size());
	for(Explo* explo : explos)
		explo->Write(f);

	// electros
	f.Write(electros.size());
	for(Electro* electro : electros)
		electro->Write(f);
}

//=================================================================================================
bool TmpLevelArea::Read(BitStreamReader& f)
{
	// bullets
	uint count;
	f >> count;
	if(!f.Ensure(count * Bullet::MIN_SIZE))
	{
		Error("Read tmp area: Broken bullet count.");
		return false;
	}
	bullets.resize(count);
	for(Bullet*& bullet : bullets)
	{
		bullet = new Bullet;
		if(!bullet->Read(f, *this))
		{
			Error("Read tmp area: Broken bullet.");
			return false;
		}
	}

	// explosions
	f >> count;
	if(!f.Ensure(count * Explo::MIN_SIZE))
	{
		Error("Read tmp area: Broken explosion count.");
		return false;
	}
	explos.resize(count);
	for(Explo*& explo : explos)
	{
		explo = new Explo;
		if(!explo->Read(f))
		{
			Error("Read tmp area: Broken explosion.");
			return false;
		}
	}

	// electro effects
	f >> count;
	if(!f.Ensure(count * Electro::MIN_SIZE))
	{
		Error("Read tmp area: Broken electro count.");
		return false;
	}
	electros.resize(count);
	for(Electro*& electro : electros)
	{
		electro = new Electro;
		electro->area = area;
		if(!electro->Read(f))
		{
			Error("Read tmp area: Broken electro.");
			return false;
		}
	}

	return true;
}
