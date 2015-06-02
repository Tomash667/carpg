// ró¿ne statystyki postaci
#include "Pch.h"
#include "Base.h"
#include "UnitStats.h"

DWORD stat_state_colors[(int)StatState::MAX] = {
	BLACK, // NORMAL
	GREEN, // POSITIVE
	COLOR_RGB(128,255,0), // POSITIVE_MIXED
	COLOR_RGB(255,255,0), // MIXED
	COLOR_RGB(255,128,0), // NEGATIVE_MIXED
	RED, // NEGATIVE
};
