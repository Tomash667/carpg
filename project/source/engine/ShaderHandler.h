#pragma once

class ShaderHandler
{
public:
	virtual void OnInit() = 0;
	virtual void OnReset() = 0;
	virtual void OnReload() = 0;
	virtual void OnRelease() = 0;
};
