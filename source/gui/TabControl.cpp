#include "Pch.h"
#include "Base.h"
/*#include "TabControl.h"

void TabElement::BeginLayoutUpdate()
{
	assert(!layoutUpdate);
	layoutUpdate = true;
	if(panel)
		panel->BeginLayoutUpdate();
}

void TabElement::EndLayoutUpdate()
{
	assert(layoutUpdate);
	layoutUpdate = false;
	if(panel)
		panel->EndLayoutUpdate();
	if(changes)
		UpdateLayout();
}

void TabElement::UpdateLayout()
{

}

TabControl::~TabControl()
{
	DeleteElements(tabs);
}

void TabControl::Add(TabElement* e)
{
	assert(e);
	e->parent = this;
	if(layoutUpdate)
		e->layoutUpdate = true;
	if(e->panel)
	{
		e->panel->parent = this;
		if(layoutUpdate)
			e->panel->layoutUpdate = true;
	}
	tabs.push_back(e);
}

void TabControl::BeginLayoutUpdate()
{
	assert(!layoutUpdate);
	layoutUpdate = true;
	for(vector<TabElement*>::iterator it = tabs.begin(), end = tabs.end(); it != end; ++it)
		(*it)->BeginLayoutUpdate();
}

void TabControl::EndLayoutUpdate()
{
	assert(layoutUpdate);
	layoutUpdate = false;

}

void TabControl::UpdateLayout()
{
	// ustal rozmiar pola klienta
	INT2 newClientSize = size - clientOffset*2;
	newClientSize.y -= tabHeight;
	newClientSize = Max(clientSize, INT2(0,0));

	// ustaw taby
	switch(tabWidthMode)
	{
	case TabWidthMode_Normal:
	case TabWidthMode_Fixed:
	case TabWidthMode_Max:
	case TabWidthMode_Split:
	}
}*/
