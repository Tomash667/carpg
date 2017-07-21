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
	Vec3 from, to, center, real_from, real_to;
	Vec2 rot, real_rot;
	Matrix matViewProj, matViewInv;
	float dist, tmp_dist, draw_range, springiness, d;
	FrustumPlanes frustum, frustum2;
	byte free_rot_key;
	bool free_rot;

private:
	int reset;

public:
	Camera(float springiness = 40.f);

	void Reset();
	void UpdateRot(float dt, const Vec2& new_rot);
	void Update(float dt, const Vec3& new_from, const Vec3& new_to);
};
