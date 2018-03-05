#include "Pch.h"
#include "GameCore.h"
#include "FrameInfo.h"

//-----------------------------------------------------------------------------
vector<FrameInfo*> FrameInfo::frames;

//=================================================================================================
FrameInfo* FrameInfo::TryGet(const AnyString& id)
{
	for(auto frame : frames)
	{
		if(frame->id == id)
			return frame;
	}

	return nullptr;
}
