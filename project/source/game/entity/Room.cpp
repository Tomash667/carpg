#include "Pch.h"
#include "GameCore.h"
#include "Room.h"
#include "SaveState.h"
#include "BitStreamFunc.h"

//=================================================================================================
void Room::Save(FileWriter& f)
{
	f << pos;
	f << size;
	f << connected;
	f << target;
}

//=================================================================================================
void Room::Load(FileReader& f)
{
	f >> pos;
	f >> size;
	f >> connected;
	if(LOAD_VERSION >= V_0_5)
		f >> target;
	else
	{
		int old_target;
		bool corridor;
		f >> old_target;
		f >> corridor;
		if(old_target == 0)
			target = (corridor ? RoomTarget::Corridor : RoomTarget::None);
		else
			target = (RoomTarget)(old_target + 1);
	}
}

//=================================================================================================
void Room::Write(BitStreamWriter& f) const
{
	f << pos;
	f << size;
	f.WriteVectorCasted<byte, byte>(connected);
	f.WriteCasted<byte>(target);
}

//=================================================================================================
void Room::Read(BitStreamReader& f)
{
	f >> pos;
	f >> size;
	f.ReadVectorCasted<byte, byte>(connected);
	f.ReadCasted<byte>(target);
}
