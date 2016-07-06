#include "Pch.h"
#include "Base.h"
#include "Container2.h"

using namespace gui;

void Container2::Event2(GuiEvent2 e, void* data)
{
	switch(e)
	{
	case GE_GainFocus:
		focus = false;
		break;
	case GE_LostFocus:
		focus = false;
		break;
	default:
		Control2::Event2(e, data);
		break;
	}
}
