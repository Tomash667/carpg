#pragma once

#include "InsideLocationGenerator.h"

class TutorialLocationGenerator : public InsideLocationGenerator
{
public:
	void Generate() override {}
	void OnEnter() override;
};
