#include "Pch.h"
#include "GameCore.h"
#include "GroundItem.h"
#include "Item.h"
#include "QuestConsts.h"
#include "QuestManager.h"
#include "BitStreamFunc.h"
#include "Game.h"
#include "GameFile.h"
#include "SaveState.h"
#include <SceneNode.h>
#include <Scene.h>
#include "GameResources.h"
#include "LevelArea.h"

EntityType<GroundItem>::Impl EntityType<GroundItem>::impl;

//=================================================================================================
void GroundItem::CreateNode(Scene* scene)
{
	node = SceneNode::Get();
	node->pos = pos;
	if(IsSet(item->flags, ITEM_GROUND_MESH))
	{
		node->SetMesh(item->mesh);
		node->pos.y -= item->mesh->head.bbox.v1.y;
	}
	else
		node->SetMesh(game_res->aBag);
	if(IsSet(item->flags, ITEM_ALPHA))
		node->flags |= SceneNode::F_ALPHA_TEST;
	node->mat = Matrix::RotationY(rot) * Matrix::Translation(node->pos);
	node->type = SceneNode::NORMAL;
	scene->Add(node);
}

//=================================================================================================
void GroundItem::Save(GameWriter& f)
{
	f << id;
	f << pos;
	f << rot;
	f << count;
	f << team_count;
	f << item->id;
	if(item->id[0] == '$')
		f << item->quest_id;
}

//=================================================================================================
void GroundItem::Load(GameReader& f)
{
	if(LOAD_VERSION >= V_0_12)
		f >> id;
	Register();
	f >> pos;
	f >> rot;
	f >> count;
	f >> team_count;
	const string& item_id = f.ReadString1();
	if(item_id[0] != '$')
		item = Item::Get(item_id);
	else
	{
		int quest_id = f.Read<int>();
		quest_mgr->AddQuestItemRequest(&item, item_id.c_str(), quest_id, nullptr);
		item = QUEST_ITEM_PLACEHOLDER;
	}
	if(LOAD_VERSION < V_0_12)
		f.Skip<int>(); // old netid
	if(f.is_local)
		CreateNode(f.area->scene);
}

//=================================================================================================
void GroundItem::Write(BitStreamWriter& f)
{
	f << id;
	f << pos;
	f << rot;
	f << count;
	f << team_count;
	f << item->id;
	if(item->IsQuest())
		f << item->quest_id;
}

//=================================================================================================
bool GroundItem::Read(BitStreamReader& f)
{
	f >> id;
	f >> pos;
	f >> rot;
	f >> count;
	f >> team_count;
	Register();
	if(f.ReadItemAndFind(item) > 0)
	{
		CreateNode(f.area->scene);
		return true;
	}
	return false;
}
