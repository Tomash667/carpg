#pragma once

struct TypeItem
{
	string toolset_path;

	template<typename T>
	T& To()
	{
		return *(T*)this;
	}
};
