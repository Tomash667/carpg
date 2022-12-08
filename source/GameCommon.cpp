#include "Pch.h"
#include "GameCommon.h"

#include "Language.h"
#include "Version.h"

//-----------------------------------------------------------------------------
vector<uint> _toRemove;

//-----------------------------------------------------------------------------
cstring dirName[8];
cstring dirNameShort[] = {
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
	Language::Section s = Language::GetSection("Directions");
	dirName[DIR_N] = s.Get("dirN");
	dirName[DIR_S] = s.Get("dirS");
	dirName[DIR_E] = s.Get("dirE");
	dirName[DIR_W] = s.Get("dirW");
	dirName[DIR_NE] = s.Get("dirNE");
	dirName[DIR_NW] = s.Get("dirNW");
	dirName[DIR_SE] = s.Get("dirSE");
	dirName[DIR_SW] = s.Get("dirSW");
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
Direction AngleToDir(float angle)
{
	assert(InRange(angle, 0.f, 2 * PI));
	if(angle < 1.f / 8 * PI)
		return DIR_S;
	else if(angle < 3.f / 8 * PI)
		return DIR_SW;
	else if(angle < 5.f / 8 * PI)
		return DIR_W;
	else if(angle < 7.f / 8 * PI)
		return DIR_NW;
	else if(angle < 9.f / 8 * PI)
		return DIR_N;
	else if(angle < 11.f / 8 * PI)
		return DIR_NE;
	else if(angle < 13.f / 8 * PI)
		return DIR_E;
	else if(angle < 15.f / 8 * PI)
		return DIR_SE;
	else
		return DIR_S;
}

//=================================================================================================
Direction GetLocationDir(const Vec2& from, const Vec2& to)
{
	return AngleToDir(Vec2::LookAtAngle(from, to));
}

//=================================================================================================
cstring VersionToString(uint version)
{
	uint major = GET_MAJOR(version),
		minor = GET_MINOR(version),
		patch = GET_PATCH(version);

	if(patch == 0)
		return Format("%u.%u", major, minor);
	else
		return Format("%u.%u.%u", major, minor, patch);
}

//=================================================================================================
uint StringToVersion(cstring version)
{
	string s(version);
	uint major, minor, patch;
	int result = sscanf_s(version, "%u.%u.%u", &major, &minor, &patch);
	if(result != 3 || !InRange(major, 0u, 255u) || !InRange(minor, 0u, 255u) || !InRange(patch, 0u, 255u))
	{
		result = sscanf_s(version, "%u.%u", &major, &minor);
		if(result != 2 || !InRange(major, 0u, 255u) || !InRange(minor, 0u, 255u))
			return -1;
		else
			return (major << 16) | (minor << 8);
	}
	else
		return ((major << 16) | (minor << 8) | patch);
}
