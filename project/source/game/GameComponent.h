#pragma once

class GameComponent
{
public:
	virtual ~GameComponent() {}
	virtual void Prepare() {}
	virtual void InitOnce() {}
	virtual void LoadLanguage() {}
	virtual void PostInit() {}
	virtual void Cleanup() {}
};
