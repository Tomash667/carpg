#include "Pch.h"
#include "Base.h"
#include "GuiDialog.h"
#include "Overlay.h"

using namespace gui;

void GuiDialog::Close()
{
	GUI.GetOverlay()->CloseDialog(this);
}
