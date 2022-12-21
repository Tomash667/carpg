#include "Pch.h"
#include "Explo.h"

#include "Ability.h"
#include "BitStreamFunc.h"
#include "LocationPart.h"
#include "Net.h"
#include "Unit.h"

//=================================================================================================
bool Explo::Update(float dt, LocationPart& locPart)
{
	// increase size
	bool deleteMe = false;
	size += sizemax * dt;
	if(size >= sizemax)
	{
		deleteMe = true;
		size = sizemax;
	}

	if(Net::IsLocal())
	{
		// deal damage
		Unit* owner = this->owner;
		const float dmg = this->dmg * Lerp(1.f, 0.1f, size / sizemax);
		for(Unit* unit : locPart.units)
		{
			if(!unit->IsAlive() || (owner && owner->IsFriend(*unit, true)))
				continue;

			if(!IsInside(hitted, unit))
			{
				Box box;
				unit->GetBox(box);

				if(SphereToBox(pos, size, box))
				{
					unit->GiveDmg(dmg, owner, nullptr, Unit::DMG_NO_BLOOD | Unit::DMG_MAGICAL);
					hitted.push_back(unit);
				}
			}
		}
	}

	if(deleteMe)
		delete this;
	return deleteMe;
}

//=================================================================================================
void Explo::Save(GameWriter& f)
{
	f << pos;
	f << size;
	f << sizemax;
	f << dmg;
	f << hitted;
	f << owner;
	f << ability->hash;
}

//=================================================================================================
void Explo::Load(GameReader& f)
{
	f >> pos;
	f >> size;
	f >> sizemax;
	f >> dmg;
	f >> hitted;
	f >> owner;
	if(LOAD_VERSION >= V_0_13)
		ability = Ability::Get(f.Read<int>());
	else
	{
		const string& tex = f.ReadString1();
		for(Ability* ability : Ability::abilities)
		{
			if(ability->texExplode.diffuse && ability->texExplode.diffuse->filename == tex)
			{
				this->ability = ability;
				break;
			}
		}
	}
}

//=================================================================================================
void Explo::Write(BitStreamWriter& f)
{
	f << ability->hash;
	f << pos;
	f << size;
	f << sizemax;
}

//=================================================================================================
bool Explo::Read(BitStreamReader& f)
{
	ability = Ability::Get(f.Read<int>());
	f >> pos;
	f >> size;
	f >> sizemax;
	return f.IsOk();
}
