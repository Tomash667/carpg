#pragma once

/*template<typename T, int N=5>
struct LerpBuffer
{
	T items[N];
	int index, last;

	inline void Reset()
	{
		index = 0;
		last = 0;
	}

	inline T Update(const T& item)
	{
		assert(index <= N);

		items[last] = item;
		last = (last + 1)%N;
		if(index < N)
			++index;

		T result = items[0];
		for(int i = 1; i<index; ++i)
			result += items[i];
		result /= (float)index;
		return result;
	}
};*/

class Camera
{
public:
	VEC3 from, to, center, real_from, real_to;
	VEC2 rot, real_rot;
	MATRIX matViewProj, matViewInv;
	float dist, tmp_dist, draw_range, springiness, d;
	FrustumPlanes frustum, frustum2;
	byte free_rot_key;
	bool free_rot;

private:
	int reset;

public:
	inline Camera(float springiness = 40.f) : springiness(springiness), reset(2), free_rot(false)
	{

	}

	inline void Reset()
	{
		reset = 2;
		free_rot = false;
		from = real_from;
		to = real_to;
		tmp_dist = dist;
	}

	inline void UpdateRot(float dt, const VEC2& new_rot)
	{
		if(reset == 0)
			d = 1.0f - exp(log(0.5f) * springiness * dt);
		else
		{
			d = 1.0f;
			reset = 1;
		}

		real_rot = new_rot;
		rot = clip(slerp(rot, real_rot, d));
		tmp_dist += (dist - tmp_dist) * d;
	}

	inline void Update(float dt, const VEC3& new_from, const VEC3& new_to)
	{
		real_from = new_from;
		real_to = new_to;

		if(reset == 0)
		{
			from += (real_from - from) * d;
			to += (real_to - to) * d;
		}
		else
		{
			from = real_from;
			to = real_to;
			reset = 0;
		}
	}
};
