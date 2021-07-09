#include "Pch.h"
#include <AppEntry.h>
#include "Editor.h"

int AppEntry(char*)
{
	Editor editor;
	return editor.Start() ? 0 : 1;
}
