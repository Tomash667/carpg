#include "Pch.h"
#include "Base.h"
#include "Control2.h"

using namespace gui;

/*void Control2::BeginUpdate()
{
	assert(!IS_SET(update_flags, UF_IN | UF_PARENT_IN));
	update_flags |= UF_IN;
	OnBeginUpdate();
}

void Control2::EndUpdate()
{
	assert(IS_SET(update_flags, UF_IN) && !IS_SET(update_flags, UF_PARENT_IN));
}*/

void Control2::Event2(GuiEvent2 e, void* data)
{
	switch(e)
	{
	case GE_GainFocus:
		// control gained focus
		focus = true;
		break;
	case GE_LostFocus:
		focus = false;
		break;
	case GE_Moved:
		UpdatePosition();
		break;
	case GE_Resized:
		UpdateSize();
		break;
	case GE_Shown:
	case GE_WindowResized:
		// applciation window resized, ignore, this is only handled by top controls - like overlay
		break;
	case GE_Closed:
	case GE_Created:
		UpdateSize();
		UpdatePosition();
		break;
	}
}

void Control2::UpdateSize()
{

}

void Control2::UpdatePosition()
{

}
