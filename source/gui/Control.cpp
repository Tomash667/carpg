#include "Pch.h"
#include "Base.h"
#include "Control.h"
#include "Overlay.h"

void Control::TakeFocus()
{
	GUI.GetOverlay()->SetFocus(this);
}
