#include "Pch.h"
#include "LevelPart.h"

#include "BitStreamFunc.h"
#include "Bullet.h"
#include "DestroyedObject.h"
#include "Electro.h"
#include "Explo.h"
#include "GameResources.h"
#include "GroundItem.h"
#include "Item.h"
#include "LocationPart.h"

#include <ParticleSystem.h>
#include <Scene.h>

//=================================================================================================
LevelPart::LevelPart(LocationPart* locPart) : locPart(locPart), scene(new Scene), lightsDt(1.0f)
{
}

//=================================================================================================
LevelPart::~LevelPart()
{
	delete scene;

	DeleteElements(explos);
	DeleteElements(electros);
	DeleteElements(bullets);
	DeleteElements(pes);
	DeleteElements(tpes);
	DeleteElements(destroyedObjects);
}

//=================================================================================================
void LevelPart::Save(GameWriter& f)
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

	f << destroyedObjects.size();
	for(DestroyedObject* obj : destroyedObjects)
		obj->Save(f);
}

//=================================================================================================
void LevelPart::Load(GameReader& f)
{
	int particleVersion;
	if(LOAD_VERSION >= V_DEV)
		particleVersion = 3;
	else if(LOAD_VERSION >= V_0_13)
		particleVersion = 2;
	else if(LOAD_VERSION >= V_0_12)
		particleVersion = 1;
	else
		particleVersion = 0;

	pes.resize(f.Read<uint>());
	for(ParticleEmitter*& pe : pes)
	{
		pe = new ParticleEmitter;
		pe->Load(f, particleVersion);
	}

	tpes.resize(f.Read<uint>());
	for(TrailParticleEmitter*& tpe : tpes)
	{
		tpe = new TrailParticleEmitter;
		tpe->Load(f, particleVersion);
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
		electro->locPart = locPart;
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

	if(LOAD_VERSION >= V_DEV)
	{
		destroyedObjects.resize(f.Read<uint>());
		for(DestroyedObject* obj : destroyedObjects)
		{
			obj = new DestroyedObject;
			obj->Load(f);
		}
	}
	else
		destroyedObjects.clear();
}

//=================================================================================================
void LevelPart::Write(BitStreamWriter& f)
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

	// destroyed objects
	f.Write(destroyedObjects.size());
	for(DestroyedObject* obj : destroyedObjects)
		obj->Write(f);
}

//=================================================================================================
bool LevelPart::Read(BitStreamReader& f)
{
	// bullets
	uint count;
	f >> count;
	if(!f.Ensure(count * Bullet::MIN_SIZE))
	{
		Error("Read level part: Broken bullet count.");
		return false;
	}
	bullets.resize(count);
	for(Bullet*& bullet : bullets)
	{
		bullet = new Bullet;
		if(!bullet->Read(f, *this))
		{
			Error("Read level part: Broken bullet.");
			return false;
		}
	}

	// explosions
	f >> count;
	if(!f.Ensure(count * Explo::MIN_SIZE))
	{
		Error("Read level part: Broken explosion count.");
		return false;
	}
	explos.resize(count);
	for(Explo*& explo : explos)
	{
		explo = new Explo;
		if(!explo->Read(f))
		{
			Error("Read level part: Broken explosion.");
			return false;
		}
	}

	// electro effects
	f >> count;
	if(!f.Ensure(count * Electro::MIN_SIZE))
	{
		Error("Read level part: Broken electro count.");
		return false;
	}
	electros.resize(count);
	for(Electro*& electro : electros)
	{
		electro = new Electro;
		electro->locPart = locPart;
		if(!electro->Read(f))
		{
			Error("Read level part: Broken electro.");
			return false;
		}
	}

	// destroyed objects
	f >> count;
	if(!f.Ensure(count * DestroyedObject::MIN_SIZE))
	{
		Error("Read level part: Broken destroyed objects.");
		return false;
	}
	destroyedObjects.resize(count);
	for(DestroyedObject*& obj : destroyedObjects)
	{
		obj = new DestroyedObject;
		if(!obj->Read(f))
		{
			Error("Read level part: Broken destroyed object.");
			return false;
		}
	}

	return true;
}
