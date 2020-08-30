#include "Pch.h"
#include "Explo.h"

#include "Ability.h"
#include "BitStreamFunc.h"
#include "LevelArea.h"
#include "Net.h"
#include "Unit.h"

#include <Scene.h>
#include <SceneNode.h>

//=================================================================================================
bool Explo::Update(float dt, LevelArea& area)
{
	// increase size
	bool delete_me = false;
	size += sizemax * dt;
	if(size >= sizemax)
	{
		delete_me = true;
		size = sizemax;
	}

	if(Net::IsLocal())
	{
		// deal damage
		Unit* owner = this->owner;
		const float dmg = this->dmg * Lerp(1.f, 0.1f, size / sizemax);
		for(Unit* unit : area.units)
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

	if(delete_me)
	{
		area.tmp->scene->Remove(node);
		node->Free();
		delete this;
		return true;
	}
	else
	{
		node->radius = node->mesh->head.radius * size;
		node->mat = Matrix::Scale(size) * Matrix::Translation(pos);
		node->tint.w = 1.f - size / sizemax;
		return false;
	}
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
			if(ability->tex_explode.diffuse && ability->tex_explode.diffuse->filename == tex)
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
