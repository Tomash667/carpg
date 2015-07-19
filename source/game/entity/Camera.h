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
	float dist, draw_range, springiness, d;
	FrustumPlanes frustum, frustum2;
	bool reset;

	inline Camera() : springiness(100), reset(true)
	{

	}

	inline void Reset()
	{
		reset = true;
	}

	inline void UpdateRot(float dt, const VEC2& new_rot)
	{
		if(!reset)
			d = 1.0f - exp(log(0.5f) * springiness * dt);
		else
		{
			d = 1.0f;
			reset = false;
		}

		real_rot = new_rot;
		rot = slerp(rot, real_rot, d);
	}

	inline void Update(float dt, const VEC3& new_from, const VEC3& new_to)
	{
		real_from = new_from;
		real_to = new_to;

		from += (real_from - from) * d;
		to += (real_to - to) * d;
	}
};
