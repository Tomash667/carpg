#pragma once

#include "OutsideLocationGenerator.h"

class ForestGenerator : public OutsideLocationGenerator
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
