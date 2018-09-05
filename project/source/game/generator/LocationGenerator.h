#pragma once

class LocationGenerator
{
public:
	virtual ~LocationGenerator() {}
	virtual void Init() {}
	virtual int GetNumberOfSteps();
	virtual void Generate() = 0;

//protected:
	Location* loc;
	bool first, reenter;
};
