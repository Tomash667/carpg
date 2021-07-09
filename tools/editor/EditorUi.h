#pragma once

#include <Control.h>

class EditorUi : public Control
{
public:
	EditorUi(Editor* editor);
	void Draw(ControlDrawData*) override;

private:
	Editor* editor;
	Font* font;
};
