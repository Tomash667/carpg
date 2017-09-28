#include "Pch.h"
#include "Core.h"
#include "Content.h"
#include "ContentLoader.h"
#include "BaseObject.h"
#include "BaseUsable.h"

vector<VariantObject*> variant_objects;

class ObjectLoader
{
	enum Group
	{
		G_TOP,
		G_OBJECT_PROPERTY
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
		OP_VARIANT,
		OP_EXTRA_DIST
	};

public:
	void Load()
	{
		InitTokenizer();

		ContentLoader loader;
		bool ok = loader.Load(t, "objects.txt", G_TOP, [this](int top, const string& id)
		{
			VerifyNameIsUnique(id);
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

		// TODO
		//Info("Loaded objects (%u), usables (%u), - crc %p.",
		//	content::buildings.size(), content::building_groups.size(), content::building_scripts.size(), content::buildings_crc);
	}

private:
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
			{ "variant", OP_VARIANT },
			{ "extra_dist", OP_EXTRA_DIST }
		});
	}

	void ParseObject(const string& id)
	{
		Ptr<BaseObject> obj;
		obj->id = id;
		t.Next();

		if(t.IsSymbol(':'))
		{
			t.Next();
			const string& parent_id = t.MustGetItem();
			auto parent = BaseObject::TryGet(parent_id);
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
				t.Next();
				t.AssertSymbol('}');
				t.Next();
				if(obj->r <= 0 || obj->h <= 0)
					t.Throw("Invalid cylinder size.");
				break;
			case OP_CENTER_Y:
				obj->centery = t.MustGetNumberFloat();
				t.Next();
				break;
			case OP_FLAGS:
			case OP_ALPHA:
				obj->alpha = t.MustGetInt();
				t.Next();
				if(obj->alpha < -1)
					t.Throw("Invalid alpha value.");
				break;
			case OP_VARIANT:
				{
					t.AssertSymbol('{');
					t.Next();
					obj->variant = new VariantObject;
					obj->variant->loaded = false;
					variant_objects.push_back(obj->variant);
					while(!t.IsSymbol('}'))
					{
						obj->variant->entries.push_back({ t.MustGetString(), nullptr });
						t.Next();
					}
					if(obj->variant->entries.size() < 2u)
						t.Throw("Variant with not enought meshes.");
					t.Next();
				}
				break;
			case OP_EXTRA_DIST:
				obj->extra_dist = t.MustGetNumberFloat();
				t.Next();
				if(obj->extra_dist < 0.f)
					t.Throw("Invalid extra distance.");
				break;
			}
		}

		BaseObject::objs.push_back(obj.Pin()));
	}

	void ParseUsable(const string& id)
	{

	}

	void VerifyNameIsUnique(const string& id)
	{
		if(BaseObject::TryGet(id) || BaseUsable::TryGet(id))
			t.Throw("Id must be unique.");
	}

	void CalculateCrc()
	{

	}

	Tokenizer t;
};

void content::LoadObjects()
{
	ObjectLoader loader;
	loader.Load();
}

void content::CleanupObjects()
{

}
