#include "Pch.h"
#include "NativeDialogs.h"

#include <Engine.h>
#define INCLUDE_COMMON_DIALOGS
#include <WindowsIncludes.h>

static char buf[MAX_PATH] = {};

// filer example: "Maps\0*.map\0Text files\0*.txt\0\0"
// ext example: "map"
cstring PickFile(cstring filter, bool save, cstring ext)
{
	OPENFILENAME open = {};
	open.lStructSize = sizeof(OPENFILENAME);
	open.hwndOwner = app::engine->GetWindowHandle();
	open.lpstrFilter = filter;
	open.lpstrFile = buf;
	open.nMaxFile = MAX_PATH;
	open.lpstrDefExt = ext;
	if(save)
		open.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
	else
		open.Flags = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

	app::engine->UnlockCursor();

	BOOL result;
	if(save)
		result = GetSaveFileName(&open);
	else
		result = GetOpenFileName(&open);

	app::engine->LockCursor();
	app::engine->RestoreFocus();

	return result ? buf : nullptr;
}
