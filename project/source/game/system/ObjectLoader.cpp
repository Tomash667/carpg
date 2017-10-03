#include "Pch.h"
#include "Core.h"
#include "ContentLoader.h"
#include "BaseObject.h"
#include "BaseUsable.h"
#include "Crc.h"

uint content::objects_crc;
static vector<VariantObject*> variant_objects;

//=================================================================================================
class ObjectLoader
{
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
		T_USABLE
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

public:
	//=================================================================================================
	void Load()
	{
		InitTokenizer();

		ContentLoader loader;
		bool ok = loader.Load(t, "objects.txt", G_TOP, [&, this](int top, const string& id)
		{
			if(BaseObject::TryGet(id.c_str()))
				t.Throw("Id must be unique.");

			switch(top)
			{
			case T_OBJECT:
				ParseObject(id);
				break;
			case T_USABLE:
				ParseUsable(id);
				break;
			}
		});
		if(!ok)
			return;

		CalculateCrc();

		Info("Loaded objects (%u), usables (%u), - crc %p.",
			BaseObject::objs.size() - BaseUsable::usables.size(), BaseUsable::usables.size(), content::objects_crc);
	}

private:
	//=================================================================================================
	void InitTokenizer()
	{
		t.AddKeywords(G_TOP, {
			{ "object", T_OBJECT },
			{ "usable", T_USABLE }
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
			{ "billboard", OBJ_BILLBOARD },
			{ "scaleable", OBJ_SCALEABLE },
			{ "physics_ptr", OBJ_PHYSICS_PTR },
			{ "building", OBJ_BUILDING },
			{ "double_physics", OBJ_DOUBLE_PHYSICS },
			{ "blood_effect", OBJ_BLOOD_EFFECT },
			{ "required", OBJ_REQUIRED },
			{ "in_middle", OBJ_IN_MIDDLE },
			{ "blocks_camera", OBJ_PHY_BLOCKS_CAM },
			{ "rotate_physics", OBJ_PHY_ROT },
			{ "water_effect", OBJ_WATER_EFFECT },
			{ "multiple_physics", OBJ_MULTI_PHYSICS },
			{ "camera_colliders", OBJ_CAM_COLLIDERS }
		});

		t.AddKeywords(G_USABLE_PROPERTY, {
			{ "required_item", UP_REQUIRED_ITEM },
			{ "animation", UP_ANIMATION },
			{ "animation_sound", UP_ANIMATION_SOUND },
			{ "limit_rot", UP_LIMIT_ROT }
		});

		t.AddKeywords(G_USABLE_FLAGS, {
			{ "allow_use", BaseUsable::ALLOW_USE },
			{ "slow_stamina_restore", BaseUsable::SLOW_STAMINA_RESTORE },
			{ "container", BaseUsable::CONTAINER },
			{ "is_bench", BaseUsable::IS_BENCH }
		});
	}

	//=================================================================================================
	void ParseObject(const string& id)
	{
		Ptr<BaseObject> obj;
		obj->id = id;
		t.Next();

		if(t.IsSymbol(':'))
		{
			t.Next();
			const string& parent_id = t.MustGetItem();
			auto parent = BaseObject::TryGet(parent_id.c_str());
			if(!parent)
				t.Throw("Missing parent object '%s'.", parent_id.c_str());
			t.Next();
			*obj = *parent;
		}

		t.AssertSymbol('{');
		t.Next();

		while(!t.IsSymbol('}'))
		{
			auto prop = (ObjectProperty)t.MustGetKeywordId(G_OBJECT_PROPERTY);
			t.Next();

			ParseObjectProperty(prop, obj);
		}

		BaseObject::objs.push_back(obj.Pin());
	}

	//=================================================================================================
	void ParseObjectProperty(ObjectProperty prop, BaseObject* obj)
	{
		switch(prop)
		{
		case OP_MESH:
			obj->mesh_id = t.MustGetString();
			t.Next();
			break;
		case OP_CYLINDER:
			t.AssertSymbol('{');
			t.Next();
			obj->r = t.MustGetNumberFloat();
			t.Next();
			obj->h = t.MustGetNumberFloat();
			if(obj->r <= 0 || obj->h <= 0)
				t.Throw("Invalid cylinder size.");
			obj->type = OBJ_CYLINDER;
			t.Next();
			t.AssertSymbol('}');
			t.Next();
			break;
		case OP_CENTER_Y:
			obj->centery = t.MustGetNumberFloat();
			t.Next();
			break;
		case OP_FLAGS:
			ReadFlags2(t, G_OBJECT_FLAGS, obj->flags);
			t.Next();
			break;
		case OP_ALPHA:
			obj->alpha = t.MustGetInt();
			if(obj->alpha < -1)
				t.Throw("Invalid alpha value.");
			t.Next();
			break;
		case OP_VARIANTS:
			{
				t.AssertSymbol('{');
				t.Next();
				obj->variants = new VariantObject;
				obj->variants->loaded = false;
				variant_objects.push_back(obj->variants);
				while(!t.IsSymbol('}'))
				{
					obj->variants->entries.push_back({ t.MustGetString(), nullptr });
					t.Next();
				}
				if(obj->variants->entries.size() < 2u)
					t.Throw("Variant with not enought meshes.");
				t.Next();
			}
			break;
		case OP_EXTRA_DIST:
			obj->extra_dist = t.MustGetNumberFloat();
			if(obj->extra_dist < 0.f)
				t.Throw("Invalid extra distance.");
			t.Next();
			break;
		}
	}

	//=================================================================================================
	void ParseUsable(const string& id)
	{
		Ptr<BaseUsable> use;
		use->id = id;
		t.Next();

		if(t.IsSymbol(':'))
		{
			t.Next();
			const string& parent_id = t.MustGetItem();
			auto parent_usable = BaseUsable::TryGet(parent_id.c_str());
			if(parent_usable)
				*use = *parent_usable;
			else
			{
				auto parent_obj = BaseObject::TryGet(parent_id.c_str());
				if(parent_obj)
					*use = *parent_obj;
				else
					t.Throw("Missing parent usable or object '%s'.", parent_id.c_str());
			}
			t.Next();
		}

		t.AssertSymbol('{');
		t.Next();

		while(!t.IsSymbol('}'))
		{
			if(t.IsKeywordGroup(G_OBJECT_PROPERTY))
			{
				auto prop = (ObjectProperty)t.MustGetKeywordId(G_OBJECT_PROPERTY);
				t.Next();

				if(prop == OP_FLAGS)
				{
					ReadFlags2(t, {
						{ &use->flags, G_OBJECT_FLAGS },
						{ &use->use_flags, G_USABLE_FLAGS }
					});
					t.Next();
				}
				else
					ParseObjectProperty(prop, use);
			}
			else if(t.IsKeywordGroup(G_USABLE_PROPERTY))
			{
				auto prop = (UsableProperty)t.MustGetKeywordId(G_USABLE_PROPERTY);
				t.Next();

				switch(prop)
				{
				case UP_REQUIRED_ITEM:
					use->item_id = t.MustGetItem();
					t.Next();
					break;
				case UP_ANIMATION:
					use->anim = t.MustGetString();
					t.Next();
					break;
				case UP_ANIMATION_SOUND:
					t.AssertSymbol('{');
					t.Next();
					use->sound_id = t.MustGetString();
					t.Next();
					use->sound_timer = t.MustGetNumberFloat();
					if(!InRange(use->sound_timer, 0.f, 1.f))
						t.Throw("Invalid animation sound timer.");
					t.Next();
					t.AssertSymbol('}');
					t.Next();
					break;
				case UP_LIMIT_ROT:
					use->limit_rot = t.MustGetInt();
					if(use->limit_rot < 0)
						t.Throw("Invalid limit rot.");
					t.Next();
					break;
				}
			}
			else
				t.ThrowExpecting("usable property");
		}

		use->flags |= OBJ_USABLE;

		auto u = use.Pin();
		BaseObject::objs.push_back(u);
		BaseUsable::usables.push_back(u);
	}

	//=================================================================================================
	void CalculateCrc()
	{
		Crc crc;

		for(BaseObject* obj : BaseObject::objs)
		{
			crc.Update(obj->id);
			crc.Update(obj->mesh_id);
			crc.Update(obj->type);
			if(obj->type == OBJ_CYLINDER)
			{
				crc.Update(obj->r);
				crc.Update(obj->h);
			}
			crc.Update(obj->centery);
			crc.Update(obj->flags);
			crc.Update(obj->alpha);
			if(obj->variants)
			{
				for(auto& e : obj->variants->entries)
					crc.Update(e.mesh_id);
			}
			crc.Update(obj->extra_dist);

			if(obj->IsUsable())
			{
				BaseUsable* use = (BaseUsable*)obj;
				crc.Update(use->anim);
				crc.Update(use->item_id);
				crc.Update(use->sound_id);
				crc.Update(use->sound_timer);
				crc.Update(use->limit_rot);
				crc.Update(use->use_flags);
			}
		}

		content::objects_crc = crc.Get();
	}

	Tokenizer t;
};

//=================================================================================================
void content::LoadObjects()
{
	ObjectLoader loader;
	loader.Load();
}

//=================================================================================================
void content::CleanupObjects()
{
	DeleteElements(BaseObject::objs);
	DeleteElements(variant_objects);
}
