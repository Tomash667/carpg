#include "Pch.h"
#include "GameCore.h"
#include "EntityInterpolator.h"
#include "Net.h"

ObjectPool<EntityInterpolator> EntityInterpolator::Pool;

//=================================================================================================
void EntityInterpolator::Reset(const Vec3& pos, float rot)
{
	valid_entries = 1;
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

	valid_entries = min(valid_entries + 1, MAX_ENTRIES);
}

//=================================================================================================
void EntityInterpolator::Update(float dt, Vec3& pos, float& rot)
{
	for(int i = 0; i < MAX_ENTRIES; ++i)
		entries[i].timer += dt;

	if(net->mp_use_interp)
	{
		if(entries[0].timer > net->mp_interp)
		{
			// nie ma nowszej klatki
			// extrapolation ? nie dziú...
			pos = entries[0].pos;
			rot = entries[0].rot;
		}
		else
		{
			// znajdü odpowiednie klatki
			for(int i = 0; i < valid_entries; ++i)
			{
				if(Equal(entries[i].timer, net->mp_interp))
				{
					// rÛwne trafienie w klatke
					pos = entries[i].pos;
					rot = entries[i].rot;
					return;
				}
				else if(entries[i].timer > net->mp_interp)
				{
					// interpolacja pomiÍdzy dwoma klatkami ([i-1],[i])
					Entry& e1 = entries[i - 1];
					Entry& e2 = entries[i];
					float t = (net->mp_interp - e1.timer) / (e2.timer - e1.timer);
					pos = Vec3::Lerp(e1.pos, e2.pos, t);
					rot = Clip(Slerp(e1.rot, e2.rot, t));
					return;
				}
			}

			// brak ruchu do tej pory
		}
	}
	else
	{
		// nie uøywa interpolacji
		pos = entries[0].pos;
		rot = entries[0].rot;
	}

	assert(rot >= 0.f && rot < PI * 2);
}
