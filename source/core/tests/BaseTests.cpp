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
		{ VEC2(0, 0), VEC2(0,1), 0.f }
	};

	int index = 0;
	for(const Case& c : cases)
	{
		float result = angle(c.p, c.p2);
		if(!equal(result, c.result))
		{
			++test_errors;
			ERROR(Format("Test case angle %d failed, for (%g,%g) (%g,%g) expected result %g, returned %g.", index, c.p.x, c.p.y, c.p2.x, c.p2.y, c.result,
				result));
		}
	}
}

void RunBaseTests()
{
	RunAngleTests();

	if(test_errors)
		throw Format("Unit tests failed with %d errors!", test_errors);
}
