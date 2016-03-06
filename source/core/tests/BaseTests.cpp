#include "Pch.h"
#include "Base.h"

int test_errors;

void RunAngleTests()
{
	struct Case
	{
		VEC2 p, p2;
		float result;
	};

	static const Case cases[] = {
		{ VEC2(0, 0), VEC2(1,0), 0.f },
		{ VEC2(0, 0), VEC2(0,1), PI*3/2 },
		{ VEC2(0, 0), VEC2(-1,0), PI },
		{ VEC2(0, 0), VEC2(0,-1), PI/2 },
		{ VEC2(1, 1), VEC2(3, 1), 0.f },
		{ VEC2(2, 2 ), VEC2(2, 5), PI*3/2 },
		{ VEC2(3, 4), VEC2(-2, 4), PI },
		{ VEC2(5, 6), VEC2(5, 0),  PI/2},
		{ VEC2(1, 2), VEC2(2,3), PI*1.75f },
		{ VEC2(-1, -3), VEC2(-3, -5), PI*0.75f }
	};

	int index = 0;
	for(const Case& c : cases)
	{
		float result = new_angle(c.p.x, c.p.y, c.p2.x, c.p2.y);
		if(!equal(result, c.result, 0.001f))
		{
			++test_errors;
			ERROR(Format("Test case angle %d failed, for (%g,%g) (%g,%g) expected result %g, returned %g.", index, c.p.x, c.p.y, c.p2.x, c.p2.y, c.result,
				result));
		}
		++index;
	}
}

void RunBaseTests()
{
	RunAngleTests();

	if(test_errors)
		throw Format("Unit tests failed with %d errors!", test_errors);
}
