#pragma once

#include "LocationGenerator.h"

class ForestGenerator : public LocationGenerator
{
	enum Type
	{
		FOREST,
		CAMP,
		MOONWELL,
		SECRET
	};

public:
	void Init() override;
	int GetNumberOfSteps() override;
	void Generate() override;
	void OnEnter() override;

private:
	Type type;
};
