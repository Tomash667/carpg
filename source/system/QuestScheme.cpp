#include "Pch.h"
#include "QuestScheme.h"

#include "GameDialog.h"

#include <angelscript.h>

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
GameDialog* QuestScheme::GetDialog(const string& id)
{
	for(GameDialog* dialog : dialogs)
	{
		if(dialog->id == id)
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

//=================================================================================================
int QuestScheme::GetPropertyId(uint nameHash)
{
	if(!varAlias.empty())
	{
		for(pair<uint, uint>& alias : varAlias)
		{
			if(alias.second == nameHash)
			{
				nameHash = alias.first;
				break;
			}
		}
	}

	const uint props = scriptType->GetPropertyCount();
	for(uint i = 0; i < props; ++i)
	{
		cstring name;
		scriptType->GetProperty(i, &name);
		if(nameHash == Hash(name))
			return i;
	}

	return -1;
}
