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
	T_GROUP,
	T_ALIAS
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
	UP_LIMIT_ROT,
	UP_SOUND
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
	DeleteElements(BaseUsable::usables);
	DeleteElements(variantObjects);
}

//=================================================================================================
void ObjectLoader::InitTokenizer()
{
	t.AddKeywords(G_TOP, {
		{ "object", T_OBJECT },
		{ "usable", T_USABLE },
		{ "group", T_GROUP },
		{ "alias", T_ALIAS }
		});

	t.AddKeywords(G_OBJECT_PROPERTY, {
		{ "mesh", OP_MESH },
		{ "cylinder", OP_CYLINDER },
		{ "centerY", OP_CENTER_Y },
		{ "flags", OP_FLAGS },
		{ "alpha", OP_ALPHA },
		{ "variants", OP_VARIANTS },
		{ "extraDist", OP_EXTRA_DIST }
		});

	t.AddKeywords(G_OBJECT_FLAGS, {
		{ "nearWall", OBJ_NEAR_WALL },
		{ "noPhysics", OBJ_NO_PHYSICS },
		{ "high", OBJ_HIGH },
		{ "isChest", OBJ_IS_CHEST },
		{ "onWall", OBJ_ON_WALL },
		{ "preload", OBJ_PRELOAD },
		{ "light", OBJ_LIGHT },
		{ "tableSpawner", OBJ_TABLE_SPAWNER },
		{ "campfireEffect", OBJ_CAMPFIRE_EFFECT },
		{ "important", OBJ_IMPORTANT },
		{ "tmpPhysics", OBJ_TMP_PHYSICS },
		{ "scaleable", OBJ_SCALEABLE },
		{ "physicsPtr", OBJ_PHYSICS_PTR },
		{ "building", OBJ_BUILDING },
		{ "doublePhysics", OBJ_DOUBLE_PHYSICS },
		{ "bloodEffect", OBJ_BLOOD_EFFECT },
		{ "blocksCamera", OBJ_PHY_BLOCKS_CAM },
		{ "rotatePhysics", OBJ_PHY_ROT },
		{ "waterEffect", OBJ_WATER_EFFECT },
		{ "multiplePhysics", OBJ_MULTI_PHYSICS },
		{ "cameraColliders", OBJ_CAM_COLLIDERS },
		{ "noCulling", OBJ_NO_CULLING }
		});

	t.AddKeywords(G_USABLE_PROPERTY, {
		{ "requiredItem", UP_REQUIRED_ITEM },
		{ "animation", UP_ANIMATION },
		{ "animationSound", UP_ANIMATION_SOUND },
		{ "limitRot", UP_LIMIT_ROT },
		{ "sound", UP_SOUND }
		});

	t.AddKeywords(G_USABLE_FLAGS, {
		{ "allowUseItem", BaseUsable::ALLOW_USE_ITEM },
		{ "slowStaminaRestore", BaseUsable::SLOW_STAMINA_RESTORE },
		{ "container", BaseUsable::CONTAINER },
		{ "isBench", BaseUsable::IS_BENCH },
		{ "alchemy", BaseUsable::ALCHEMY },
		{ "destroyable", BaseUsable::DESTROYABLE }
		});
}

//=================================================================================================
void ObjectLoader::LoadEntity(int top, const string& id)
{
	if(top != T_ALIAS)
	{
		int hash = Hash(id);
		ObjectGroup* group = nullptr;
		BaseObject* existingObj = BaseObject::TryGet(hash, &group);
		if(existingObj)
		{
			if((group && group->id != id) || existingObj->id != id)
				t.Throw("Id hash collision.");
			else
				t.Throw("Id must be unique.");
		}
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
	case T_ALIAS:
		ParseAlias(id);
		break;
	}
}

//=================================================================================================
void ObjectLoader::Finalize()
{
	CalculateCrc();

	Info("Loaded objects (%u), usables (%u) - crc %p.",
		BaseObject::items.size(), BaseUsable::usables.size(), content.crc[(int)Content::Id::Objects]);
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
	BaseObject::hashes[o->hash] = o;
	BaseObject::items.push_back(o);
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
				}
				break;
			case UP_ANIMATION:
				use->anim = t.MustGetString();
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
				}
				break;
			case UP_LIMIT_ROT:
				use->limitRot = t.MustGetInt();
				if(use->limitRot < 0)
					t.Throw("Invalid limit rot.");
				break;
			case UP_SOUND:
				{
					const string& soundId = t.MustGetString();
					use->sound = resMgr->TryGet<Sound>(soundId);
					if(!use->sound)
						LoadError("Missing sound '%s'.", soundId.c_str());
				}
				break;
			}

			t.Next();
		}
		else
			t.ThrowExpecting("usable property");
	}

	use->flags |= OBJ_USABLE;

	BaseUsable* u = use.Pin();
	BaseObject::hashes[u->hash] = u;
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
	ObjectGroup::hashes[g->hash] = g;
	ObjectGroup::items.push_back(g);
}

//=================================================================================================
void ObjectLoader::ParseAlias(const string& id)
{
	ObjectGroup* group = nullptr;
	BaseObject* obj = BaseObject::TryGet(id, &group);
	if(!obj)
		t.Throw("Missing object.");
	if(group)
		t.Throw("Can't create alias for group."); // YAGNI
	t.Next();

	const string& alias = t.MustGetItemKeyword();
	int hash = Hash(alias);
	if(BaseObject::TryGet(hash))
		t.Throw("Alias or object '%s' already exists.", alias.c_str());

	BaseObject::hashes[hash] = obj;
}

//=================================================================================================
void ObjectLoader::CalculateCrc()
{
	Crc crc;

	for(BaseObject* pObj : BaseObject::items)
	{
		BaseObject& obj = *pObj;
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

	for(ObjectGroup* group : ObjectGroup::items)
	{
		crc.Update(group->id);
		UpdateObjectGroupCrc(crc, group->list);
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
