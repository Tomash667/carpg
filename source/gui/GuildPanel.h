#pragma once

#include <Container.h>

class GuildPanel : public Container
{
public:
	GuildPanel();
	void Draw(ControlDrawData*) override;
};
