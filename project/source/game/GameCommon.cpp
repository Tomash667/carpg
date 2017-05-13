#include "Pch.h"
#include "Base.h"
#include "GameCommon.h"
#include "Version.h"
#include "Language.h"

//-----------------------------------------------------------------------------
vector<uint> _to_remove;

//-----------------------------------------------------------------------------
cstring kierunek_nazwa[8];
cstring kierunek_nazwa_s[] = {
	"N",
	"S",
	"E",
	"W",
	"NE",
	"NW",
	"SE",
	"SW"
};

//=================================================================================================
void SetGameCommonText()
{
	kierunek_nazwa[DIR_N] = Str("dirN");
	kierunek_nazwa[DIR_S] = Str("dirS");
	kierunek_nazwa[DIR_E] = Str("dirE");
	kierunek_nazwa[DIR_W] = Str("dirW");
	kierunek_nazwa[DIR_NE] = Str("dirNE");
	kierunek_nazwa[DIR_NW] = Str("dirNW");
	kierunek_nazwa[DIR_SE] = Str("dirSE");
	kierunek_nazwa[DIR_SW] = Str("dirSW");
}

//=================================================================================================
//         PI
//         /|\
//          |
//          |
// PI/2 <---+---> 3/2 PI
//          |
//          |
//         \|/
//          0
KIERUNEK AngleToDir(float angle)
{
	assert(in_range(angle, 0.f, 2*PI));
	if(angle < 1.f/8*PI)
		return DIR_S;
	else if(angle < 3.f/8*PI)
		return DIR_SW;
	else if(angle < 5.f/8*PI)
		return DIR_W;
	else if(angle < 7.f/8*PI)
		return DIR_NW;
	else if(angle < 9.f/8*PI)
		return DIR_N;
	else if(angle < 11.f/8*PI)
		return DIR_NE;
	else if(angle < 13.f/8*PI)
		return DIR_E;
	else if(angle < 15.f/8*PI)
		return DIR_SE;
	else
		return DIR_S;
}

//=================================================================================================
KIERUNEK GetLocationDir(const VEC2& from, const VEC2& to)
{
	return AngleToDir(lookat_angle(from, to));
}

//=================================================================================================
cstring VersionToString(uint wersja)
{
	uint major = GET_MAJOR(wersja),
		 minor = GET_MINOR(wersja),
		 patch = GET_PATCH(wersja);

	return Format("%u.%u.%u", major, minor, patch);
}

//=================================================================================================
uint StringToVersion(cstring wersja)
{
	string s(wersja);
	uint major, minor, patch;
	int wynik = sscanf_s(wersja, "%u.%u.%u", &major, &minor, &patch);
	if(wynik != 3 || !in_range(major, 0u, 255u) || !in_range(minor, 0u, 255u) || !in_range(patch, 0u, 255u))
		return -1;
	else
		return ((major<<16)|(minor<<8)|patch);
}
