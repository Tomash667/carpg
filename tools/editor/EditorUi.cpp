#include "Pch.h"
#include "EditorUi.h"

#include "Editor.h"
#include "Level.h"
#include "Room.h"

#include <ResourceManager.h>

EditorUi::EditorUi(Editor* editor) : editor(editor)
{
	font = gui->GetFont("Arial", 20);
	tCrosshair = res_mgr->Load<Texture>("editor_crosshair.png");
}

void EditorUi::Draw(ControlDrawData*)
{
	// crosshair
	gui->DrawSprite(tCrosshair, Int2((gui->wnd_size.x - 16) / 2, (gui->wnd_size.y - 16) / 2));

	// text
	LocalString text = Format("Level: %s", editor->level->filename.c_str());
	if(editor->action != A_NONE)
	{
		Vec3 size;
		cstring name;
		switch(editor->action)
		{
		default:
		case A_ADD_ROOM:
			name = "add room";
			size = editor->roomBox.Size();
			break;
		case A_MOVE:
			name = "move";
			size = editor->roomSelect->box.Size();
			break;
		case A_RESIZE:
			name = "resize";
			size = editor->roomSelect->box.Size();
			break;
		}
		text += Format("\nAction: %s (%gx%gx%g)", name, FLT10(size.x), FLT10(size.y), FLT10(size.z));
	}
	else if(editor->roomSelect)
	{
		const Vec3 size = editor->roomSelect->box.Size();
		text += Format("\nSelected room: %gx%gx%g", FLT10(size.x), FLT10(size.y), FLT10(size.z));
	}
	if(editor->markerValid)
		text += Format("\nPos: %g; %g; %g", FLT10(editor->marker.x), FLT10(editor->marker.y), FLT10(editor->marker.z));
	gui->DrawText(font, text.c_str(), 0, Color::Black, Rect(0, 0, 500, 100));
}
