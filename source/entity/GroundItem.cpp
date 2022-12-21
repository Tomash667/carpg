#include "Pch.h"
#include "GroundItem.h"

#include "BitStreamFunc.h"
#include "GameResources.h"
#include "Item.h"
#include "QuestConsts.h"
#include "QuestManager.h"

#include <SceneNode.h>

EntityType<GroundItem>::Impl EntityType<GroundItem>::impl;

//=================================================================================================
void GroundItem::CreateSceneNode()
{
	Mesh* mesh;
	Vec3 nodePos = pos;
	if(IsSet(item->flags, ITEM_GROUND_MESH))
	{
		mesh = item->mesh;
		mesh->EnsureIsLoaded();
		nodePos.y -= mesh->head.bbox.v1.y;
	}
	else
		mesh = gameRes->aBag;
	node = SceneNode::Get();
	node->SetMesh(mesh);
	node->center = pos;
	node->mat = Matrix::Rotation(rot) * Matrix::Translation(nodePos);
	node->persistent = true;
}

//=================================================================================================
void GroundItem::Save(GameWriter& f)
{
	f << id;
	f << pos;
	f << rot;
	f << count;
	f << teamCount;
	f << item->id;
	if(item->id[0] == '$')
		f << item->questId;
}

//=================================================================================================
void GroundItem::Load(GameReader& f)
{
	if(LOAD_VERSION >= V_0_12)
		f >> id;
	Register();
	f >> pos;
	if(LOAD_VERSION >= V_0_14)
		f >> rot;
	else
	{
		float rotY;
		f >> rotY;
		rot = Quat::RotY(rotY);
	}
	f >> count;
	f >> teamCount;
	const string& itemId = f.ReadString1();
	if(itemId[0] != '$')
		item = Item::Get(itemId);
	else
	{
		int questId = f.Read<int>();
		questMgr->AddQuestItemRequest(&item, itemId.c_str(), questId, nullptr);
		item = QUEST_ITEM_PLACEHOLDER;
	}
	if(LOAD_VERSION < V_0_12)
		f.Skip<int>(); // old netid
}

//=================================================================================================
void GroundItem::Write(BitStreamWriter& f)
{
	f << id;
	f << pos;
	f << rot;
	f << count;
	f << teamCount;
	f << item->id;
	if(item->IsQuest())
		f << item->questId;
}

//=================================================================================================
bool GroundItem::Read(BitStreamReader& f)
{
	f >> id;
	f >> pos;
	f >> rot;
	f >> count;
	f >> teamCount;
	Register();
	return f.IsOk() && f.ReadItemAndFind(item) > 0;
}
