#include "Pch.h"
#include "DestroyedObject.h"

#include "BaseUsable.h"
#include "BitStreamFunc.h"
#include "GameFile.h"

//=================================================================================================
bool DestroyedObject::Update(float dt)
{
	timer -= dt * 4;
	if(timer > 0)
		return false;
	delete this;
	return true;
}

//=================================================================================================
void DestroyedObject::Save(GameWriter& f)
{
	f << base->hash;
	f << pos;
	f << rot;
	f << timer;
}

//=================================================================================================
void DestroyedObject::Load(GameReader& f)
{
	base = BaseUsable::Get(f.Read<int>());
	f >> pos;
	f >> rot;
	f >> timer;
}

//=================================================================================================
void DestroyedObject::Write(BitStreamWriter& f) const
{
	f << base->hash;
	f << pos;
	f << rot;
	f << timer;
}

//=================================================================================================
bool DestroyedObject::Read(BitStreamReader& f)
{
	int hash;
	f >> hash;
	f >> pos;
	f >> rot;
	f >> timer;
	if(!f)
		return false;
	base = BaseUsable::Get(hash);
	return true;
}
