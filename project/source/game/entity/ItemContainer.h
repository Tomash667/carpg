#pragma once

//-----------------------------------------------------------------------------
#include "ItemSlot.h"

//-----------------------------------------------------------------------------
struct ItemContainer
{
	vector<ItemSlot> items;

	void Save(HANDLE file);
	void Load(HANDLE file);
};
