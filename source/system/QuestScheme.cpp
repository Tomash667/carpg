#include "Pch.h"
#include "QuestScheme.h"

#include "GameDialog.h"

#include <angelscript.h>

vector<QuestScheme*> QuestScheme::schemes;
std::map<string, QuestScheme*> QuestScheme::ids;

//=================================================================================================
QuestScheme::~QuestScheme()
{
	DeleteElements(dialogs);
}

//=================================================================================================
QuestScheme* QuestScheme::TryGet(const string& id)
{
	auto it = ids.find(id);
	if(it != ids.end())
		return it->second;
	return nullptr;
}

//=================================================================================================
GameDialog* QuestScheme::GetDialog(const string& id)
{
	auto it = dialogMap.find(id);
	if(it != dialogMap.end())
		return it->second;
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
