#include "Pch.h"
#include "QuestScheme.h"

#include "GameDialog.h"

vector<QuestScheme*> QuestScheme::schemes;

//=================================================================================================
QuestScheme::~QuestScheme()
{
	DeleteElements(dialogs);
}

//=================================================================================================
QuestScheme* QuestScheme::TryGet(const string& id)
{
	for(QuestScheme* scheme : schemes)
	{
		if(scheme->id == id)
			return scheme;
	}
	return nullptr;
}

//=================================================================================================
GameDialog* QuestScheme::GetDialog(const string& dialog_id)
{
	for(GameDialog* dialog : dialogs)
	{
		if(dialog->id == dialog_id)
			return dialog;
	}
	return nullptr;
}

//=================================================================================================
int QuestScheme::GetProgress(const string& progressId)
{
	for(int i = 0, size = (int)progress.size(); i < size; ++i)
	{
		if(progress[i] == progressId)
			return i;
	}
	return -1;
}
