#include "Pch.h"
#include "EntityInterpolator.h"

#include "Net.h"

ObjectPool<EntityInterpolator> EntityInterpolator::Pool;

//=================================================================================================
void EntityInterpolator::Reset(const Vec3& pos, float rot)
{
	validEntries = 1;
	entries[0].pos = pos;
	entries[0].rot = rot;
	entries[0].timer = 0.f;
}

//=================================================================================================
void EntityInterpolator::Add(const Vec3& pos, float rot)
{
	for(int i = MAX_ENTRIES - 1; i > 0; --i)
		entries[i] = entries[i - 1];

	entries[0].pos = pos;
	entries[0].rot = rot;
	entries[0].timer = 0.f;

	validEntries = min(validEntries + 1, MAX_ENTRIES);
}

//=================================================================================================
void EntityInterpolator::Update(float dt, Vec3& pos, float& rot)
{
	for(int i = 0; i < MAX_ENTRIES; ++i)
		entries[i].timer += dt;

	if(net->mpUseInterp)
	{
		if(entries[0].timer > net->mpInterp)
		{
			// no new frame
			// extrapolation? not today...
			pos = entries[0].pos;
			rot = entries[0].rot;
		}
		else
		{
			// find correct frame
			for(int i = 0; i < validEntries; ++i)
			{
				if(Equal(entries[i].timer, net->mpInterp))
				{
					// exact frame found
					pos = entries[i].pos;
					rot = entries[i].rot;
					return;
				}
				else if(entries[i].timer > net->mpInterp)
				{
					// interpolate between two frames
					Entry& e1 = entries[i - 1];
					Entry& e2 = entries[i];
					float t = (net->mpInterp - e1.timer) / (e2.timer - e1.timer);
					pos = Vec3::Lerp(e1.pos, e2.pos, t);
					rot = Clip(Slerp(e1.rot, e2.rot, t));
					return;
				}
			}
		}
	}
	else
	{
		// don't use interpolation
		pos = entries[0].pos;
		rot = entries[0].rot;
	}

	assert(rot >= 0.f && rot < PI * 2);
}
