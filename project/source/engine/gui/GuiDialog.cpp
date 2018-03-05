#include "Pch.h"
#include "EngineCore.h"
#include "GuiDialog.h"
#include "Overlay.h"

using namespace gui;

void GuiDialog::Close()
{
	GUI.GetOverlay()->CloseDialog(this);
}
