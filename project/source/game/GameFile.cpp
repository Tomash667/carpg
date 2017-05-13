#include "Pch.h"
#include "Base.h"
#include "GameFile.h"
#include "SaveState.h"

void GameReader::LoadArtifact(const Item*& item)
{
	if(LOAD_VERSION >= V_0_4)
		operator >> (item);
	else
	{
		int what;
		cstring id;
		operator >> (what);

		switch(what)
		{
		default:
		case 0: id = "a_amulet"; break;
		case 1: id = "a_amulet2"; break;
		case 2: id = "a_amulet3"; break;
		case 3: id = "a_brosza"; break;
		case 4: id = "a_butelka"; break;
		case 5: id = "a_cos"; break;
		case 6: id = "a_czaszka"; break;
		case 7: id = "a_figurka"; break;
		case 8: id = "a_figurka2"; break;
		case 9: id = "a_figurka3"; break;
		case 10: id = "a_ksiega"; break;
		case 11: id = "a_maska"; break;
		case 12: id = "a_maska2"; break;
		case 13: id = "a_misa"; break;
		case 14: id = "a_moneta"; break;
		case 15: id = "a_pierscien"; break;
		case 16: id = "a_pierscien2"; break;
		case 17: id = "a_pierscien3"; break;
		case 18: id = "a_talizman"; break;
		case 19: id = "a_talizman2"; break;
		}

		item = FindItem(id);
	}
}
