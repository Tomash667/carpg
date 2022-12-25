#include "Pch.h"
#include "ObjectLoader.h"

#include "BaseUsable.h"
#include "GameDialog.h"
#include "Item.h"

#include <Mesh.h>
#include <ResourceManager.h>

static vector<VariantObject*> variantObjects;

enum Group
{
	G_TOP,
	G_OBJECT_PROPERTY,
	G_OBJECT_FLAGS,
	G_USABLE_PROPERTY,
	G_USABLE_FLAGS
};

enum Top
{
	T_OBJECT,
	T_USABLE,
	T_GROUP
};

enum ObjectProperty
{
	OP_MESH,
	OP_CYLINDER,
	OP_CENTER_Y,
	OP_FLAGS,
	OP_ALPHA,
	OP_VARIANTS,
	OP_EXTRA_DIST
};

enum UsableProperty
{
	UP_REQUIRED_ITEM,
	UP_ANIMATION,
	UP_ANIMATION_SOUND,
	UP_LIMIT_ROT
};

//=================================================================================================
void ObjectLoader::DoLoading()
{
	Load("objects.txt", G_TOP);
}

//=================================================================================================
void ObjectLoader::Cleanup()
{
	DeleteElements(BaseObject::items);
	DeleteElements(ObjectGroup::items);
	DeleteElements(variantObjects);
}

//=================================================================================================
void ObjectLoader::InitTokenizer()
{
	t.AddKeywords(G_TOP, {
		{ "object", T_OBJECT },
		{ "usable", T_USABLE },
		{ "group", T_GROUP }
		});

	t.AddKeywords(G_OBJECT_PROPERTY, {
		{ "mesh", OP_MESH },
		{ "cylinder", OP_CYLINDER },
		{ "center_y", OP_CENTER_Y },
		{ "flags", OP_FLAGS },
		{ "alpha", OP_ALPHA },
		{ "variants", OP_VARIANTS },
		{ "extra_dist", OP_EXTRA_DIST }
		});

	t.AddKeywords(G_OBJECT_FLAGS, {
		{ "near_wall", OBJ_NEAR_WALL },
		{ "no_physics", OBJ_NO_PHYSICS },
		{ "high", OBJ_HIGH },
		{ "is_chest", OBJ_IS_CHEST },
		{ "on_wall", OBJ_ON_WALL },
		{ "preload", OBJ_PRELOAD },
		{ "light", OBJ_LIGHT },
		{ "table_spawner", OBJ_TABLE_SPAWNER },
		{ "campfire_effect", OBJ_CAMPFIRE_EFFECT },
		{ "important", OBJ_IMPORTANT },
		{ "tmp_physics", OBJ_TMP_PHYSICS },
		{ "scaleable", OBJ_SCALEABLE },
		{ "physics_ptr", OBJ_PHYSICS_PTR },
		{ "building", OBJ_BUILDING },
		{ "double_physics", OBJ_DOUBLE_PHYSICS },
		{ "blood_effect", OBJ_BLOOD_EFFECT },
		{ "blocks_camera", OBJ_PHY_BLOCKS_CAM },
		{ "rotate_physics", OBJ_PHY_ROT },
		{ "water_effect", OBJ_WATER_EFFECT },
		{ "multiple_physics", OBJ_MULTI_PHYSICS },
		{ "camera_colliders", OBJ_CAM_COLLIDERS },
		{ "no_culling", OBJ_NO_CULLING }
		});

	t.AddKeywords(G_USABLE_PROPERTY, {
		{ "required_item", UP_REQUIRED_ITEM },
		{ "animation", UP_ANIMATION },
		{ "animation_sound", UP_ANIMATION_SOUND },
		{ "limit_rot", UP_LIMIT_ROT }
		});

	t.AddKeywords(G_USABLE_FLAGS, {
		{ "allow_use_item", BaseUsable::ALLOW_USE_ITEM },
		{ "slow_stamina_restore", BaseUsable::SLOW_STAMINA_RESTORE },
		{ "container", BaseUsable::CONTAINER },
		{ "is_bench", BaseUsable::IS_BENCH },
		{ "alchemy", BaseUsable::ALCHEMY },
		{ "destroyable", BaseUsable::DESTROYABLE }
		});
}

//=================================================================================================
void ObjectLoader::LoadEntity(int top, const string& id)
{
	int hash = Hash(id);
	ObjectGroup* group;
	BaseObject* existingObj = BaseObject::TryGet(hash, &group);
	if(existingObj)
	{
		if((group && group->id != id) || existingObj->id != id)
			t.Throw("Id hash collision.");
		else
			t.Throw("Id must be unique.");
	}

	switch(top)
	{
	case T_OBJECT:
		ParseObject(id);
		break;
	case T_USABLE:
		ParseUsable(id);
		break;
	case T_GROUP:
		ParseGroup(id);
		break;
	}
}

//=================================================================================================
void ObjectLoader::Finalize()
{
	CalculateCrc();

	Info("Loaded objects (%u), usables (%u) - crc %p.",
		BaseObject::items.size() - BaseUsable::usables.size(), BaseUsable::usables.size(), content.crc[(int)Content::Id::Objects]);
}

//=================================================================================================
void ObjectLoader::ParseObject(const string& id)
{
	Ptr<BaseObject> obj;
	obj->id = id;
	obj->hash = Hash(id);
	t.Next();

	if(t.IsSymbol(':'))
	{
		t.Next();
		const string& parentId = t.MustGetItem();
		BaseObject* parent = BaseObject::TryGet(parentId);
		if(!parent)
			t.Throw("Missing parent object '%s'.", parentId.c_str());
		t.Next();
		*obj = *parent;
	}

	t.AssertSymbol('{');
	t.Next();

	while(!t.IsSymbol('}'))
	{
		ObjectProperty prop = t.MustGetKeywordId<ObjectProperty>(G_OBJECT_PROPERTY);
		t.Next();

		ParseObjectProperty(prop, obj);
	}

	BaseObject* o = obj.Pin();
	BaseObject::items[o->hash] = o;
}

//=================================================================================================
void ObjectLoader::ParseObjectProperty(ObjectProperty prop, BaseObject* obj)
{
	switch(prop)
	{
	case OP_MESH:
		{
			const string& meshId = t.MustGetString();
			obj->mesh = resMgr->TryGet<Mesh>(meshId);
			if(!obj->mesh)
				LoadError("Missing mesh '%s'.", meshId.c_str());
			t.Next();
		}
		break;
	case OP_CYLINDER:
		t.AssertSymbol('{');
		t.Next();
		obj->r = t.MustGetFloat();
		t.Next();
		obj->h = t.MustGetFloat();
		if(obj->r <= 0 || obj->h <= 0)
			t.Throw("Invalid cylinder size.");
		obj->type = OBJ_CYLINDER;
		t.Next();
		t.AssertSymbol('}');
		t.Next();
		break;
	case OP_CENTER_Y:
		obj->centery = t.MustGetFloat();
		t.Next();
		break;
	case OP_FLAGS:
		t.ParseFlags(G_OBJECT_FLAGS, obj->flags);
		t.Next();
		break;
	case OP_VARIANTS:
		{
			t.AssertSymbol('{');
			t.Next();
			obj->variants = new VariantObject;
			variantObjects.push_back(obj->variants);
			while(!t.IsSymbol('}'))
			{
				const string& meshId = t.MustGetString();
				Mesh* mesh = resMgr->TryGet<Mesh>(meshId);
				if(mesh)
					obj->variants->meshes.push_back(mesh);
				else
					LoadError("Missing variant mesh '%s'.", meshId.c_str());
				t.Next();
			}
			if(obj->variants->meshes.size() < 2u)
				t.Throw("Variant with not enough meshes.");
			t.Next();
		}
		break;
	case OP_EXTRA_DIST:
		obj->extraDist = t.MustGetFloat();
		if(obj->extraDist < 0.f)
			t.Throw("Invalid extra distance.");
		t.Next();
		break;
	}
}

//=================================================================================================
void ObjectLoader::ParseUsable(const string& id)
{
	Ptr<BaseUsable> use;
	use->id = id;
	use->hash = Hash(id);
	t.Next();

	if(t.IsSymbol(':'))
	{
		t.Next();
		const string& parentId = t.MustGetItem();
		BaseUsable* parentUsable = BaseUsable::TryGet(parentId.c_str());
		if(parentUsable)
			*use = *parentUsable;
		else
		{
			BaseObject* parentObj = BaseObject::TryGet(parentId);
			if(parentObj)
				*use = *parentObj;
			else
				t.Throw("Missing parent usable or object '%s'.", parentId.c_str());
		}
		t.Next();
	}

	t.AssertSymbol('{');
	t.Next();

	while(!t.IsSymbol('}'))
	{
		if(t.IsKeywordGroup(G_OBJECT_PROPERTY))
		{
			ObjectProperty prop = t.MustGetKeywordId<ObjectProperty>(G_OBJECT_PROPERTY);
			t.Next();

			if(prop == OP_FLAGS)
			{
				t.ParseFlags({
					{ &use->flags, G_OBJECT_FLAGS },
					{ &use->useFlags, G_USABLE_FLAGS }
					});
				t.Next();
			}
			else
				ParseObjectProperty(prop, use);
		}
		else if(t.IsKeywordGroup(G_USABLE_PROPERTY))
		{
			UsableProperty prop = t.MustGetKeywordId<UsableProperty>(G_USABLE_PROPERTY);
			t.Next();

			switch(prop)
			{
			case UP_REQUIRED_ITEM:
				{
					const string& itemId = t.MustGetItem();
					use->item = Item::TryGet(itemId);
					if(!use->item)
						LoadError("Missing item '%s'.", itemId.c_str());
					t.Next();
				}
				break;
			case UP_ANIMATION:
				use->anim = t.MustGetString();
				t.Next();
				break;
			case UP_ANIMATION_SOUND:
				{
					t.AssertSymbol('{');
					t.Next();
					const string& soundId = t.MustGetString();
					use->sound = resMgr->TryGet<Sound>(soundId);
					if(!use->sound)
						LoadError("Missing sound '%s'.", soundId.c_str());
					t.Next();
					use->soundTimer = t.MustGetFloat();
					if(!InRange(use->soundTimer, 0.f, 1.f))
						LoadError("Invalid animation sound timer.");
					t.Next();
					t.AssertSymbol('}');
					t.Next();
				}
				break;
			case UP_LIMIT_ROT:
				use->limitRot = t.MustGetInt();
				if(use->limitRot < 0)
					t.Throw("Invalid limit rot.");
				t.Next();
				break;
			}
		}
		else
			t.ThrowExpecting("usable property");
	}

	use->flags |= OBJ_USABLE;

	BaseUsable* u = use.Pin();
	BaseObject::items[u->hash] = u;
	BaseUsable::usables.push_back(u);
}

//=================================================================================================
void ObjectLoader::ParseGroup(const string& id)
{
	Ptr<ObjectGroup> group;
	ObjectGroup::EntryList* list = &group->list;
	list->totalChance = 0;
	list->parent = nullptr;
	group->id = id;
	group->hash = Hash(id);
	t.Next();

	t.AssertSymbol('{');
	t.Next();

	while(true)
	{
		if(t.IsSymbol('}'))
		{
			if(!list->parent)
				break;
			t.Next();
			list = list->parent;
			continue;
		}

		int chance = 1;
		if(t.IsInt())
		{
			chance = t.GetInt();
			if(chance < 1)
				t.Throw("Invalid chance.");
			t.Next();
		}

		if(t.IsSymbol('{'))
		{
			auto newList = new ObjectGroup::EntryList;
			newList->totalChance = 0;
			newList->parent = list;
			ObjectGroup::EntryList::Entry e;
			e.list = newList;
			e.isList = true;
			e.chance = chance;
			list->entries.push_back(std::move(e));
			list->totalChance += chance;
			list = newList;
			t.Next();
			e.isList = false;
		}
		else if(t.IsText())
		{
			const string& objId = t.GetText();
			ObjectGroup* group = nullptr;
			BaseObject* obj = BaseObject::TryGet(objId, &group);
			if(group)
				t.Throw("Can't use group inside group."); // YAGNI
			if(!obj)
				t.Throw("Missing object '%s'.", objId.c_str());
			ObjectGroup::EntryList::Entry e;
			e.obj = obj;
			e.isList = false;
			e.chance = chance;
			list->entries.push_back(std::move(e));
			list->totalChance += chance;
			t.Next();
		}
		else
			t.Unexpected("object or group");
	}

	if(group->list.entries.empty())
		t.Throw("Empty group.");

	ObjectGroup* g = group.Pin();
	ObjectGroup::items[g->hash] = g;
}

//=================================================================================================
void ObjectLoader::CalculateCrc()
{
	Crc crc;

	for(pair<const int, BaseObject*>& p : BaseObject::items)
	{
		BaseObject& obj = *p.second;
		crc.Update(obj.id);
		if(obj.mesh)
			crc.Update(obj.mesh->filename);
		crc.Update(obj.type);
		if(obj.type == OBJ_CYLINDER)
		{
			crc.Update(obj.r);
			crc.Update(obj.h);
		}
		crc.Update(obj.centery);
		crc.Update(obj.flags);
		if(obj.variants)
		{
			for(Mesh* mesh : obj.variants->meshes)
				crc.Update(mesh->filename);
		}
		crc.Update(obj.extraDist);

		if(obj.IsUsable())
		{
			BaseUsable& use = static_cast<BaseUsable&>(obj);
			crc.Update(use.anim);
			if(use.item)
				crc.Update(use.item->id);
			if(use.sound)
				crc.Update(use.sound->filename);
			crc.Update(use.soundTimer);
			crc.Update(use.limitRot);
			crc.Update(use.useFlags);
		}
	}

	for(pair<const int, ObjectGroup*>& p : ObjectGroup::items)
	{
		crc.Update(p.second->id);
		UpdateObjectGroupCrc(crc, p.second->list);
	}

	content.crc[(int)Content::Id::Objects] = crc.Get();
}

//=================================================================================================
void ObjectLoader::UpdateObjectGroupCrc(Crc& crc, ObjectGroup::EntryList& list)
{
	crc.Update(list.totalChance);
	for(auto& e : list.entries)
	{
		crc.Update(e.chance);
		if(e.isList)
			UpdateObjectGroupCrc(crc, *e.list);
		else
			crc.Update(e.obj->id);
	}
}
