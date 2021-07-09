#include "Pch.h"
#include "EditorUi.h"

#include "Editor.h"
#include "Level.h"

EditorUi::EditorUi(Editor* editor) : editor(editor)
{
	font = gui->GetFont("Arial", 20);
}

void EditorUi::Draw(ControlDrawData*)
{
	gui->DrawText(font, Format("Level: %s", editor->level->filename.c_str()), 0, Color::Black, Rect(0, 0, 500, 100));
}
