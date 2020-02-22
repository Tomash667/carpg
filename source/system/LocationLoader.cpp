#include "Pch.h"
#include "LocationLoader.h"
#include "RoomType.h"
#include "BaseObject.h"

enum Group
{
	G_TOP,
	G_ROOM_KEYWORD,
	G_ROOM_FLAG
};

enum TopKeyword
{
	T_ROOM
};

enum RoomKeyword
{
	RK_FLAGS,
	RK_OBJECTS
};

//=================================================================================================
void LocationLoader::DoLoading()
{
	Load("locations.txt", G_TOP);
}

//=================================================================================================
void LocationLoader::Cleanup()
{
	DeleteElements(RoomType::types);
}

//=================================================================================================
void LocationLoader::InitTokenizer()
{
	t.AddKeyword("room", T_ROOM, G_TOP);

	t.AddKeywords(G_ROOM_KEYWORD, {
		{ "flags", RK_FLAGS },
		{ "objects", RK_OBJECTS }
		});

	t.AddKeywords(G_ROOM_FLAG, {
		{ "repeat", RoomType::REPEAT },
		{ "treasure", RoomType::TREASURE }
		});
	enum Group
	{
		G_TOP,
		G_ROOM_KEYWORD,
		G_ROOM_FLAG
	};

	enum TopKeyword
	{
		T_ROOM
	};

	enum RoomKeyword
	{
		RK_FLAGS,
		RK_OBJECTS
	};
}

//=================================================================================================
void LocationLoader::LoadEntity(int, const string& id)
{
	RoomType* existing_type = RoomType::Get(id);
	if(existing_type)
		t.Throw("Id must be unique.");

	Ptr<RoomType> type;
	type->id = id;
	t.Next();

	t.AssertSymbol('{');
	t.Next();

	while(!t.IsSymbol('}'))
	{
		const RoomKeyword k = (RoomKeyword)t.MustGetKeywordId(G_ROOM_KEYWORD);
		t.Next();

		switch(k)
		{
		case RK_FLAGS:
			t.ParseFlags(G_ROOM_FLAG, type->flags);
			t.Next();
			break;
		case RK_OBJECTS:
			t.AssertSymbol('{');
			t.Next();
			while(!t.IsSymbol('}'))
			{
				const string& obj_id = t.MustGetItem();
				ObjectGroup* group = nullptr;
				BaseObject* obj = BaseObject::TryGet(obj_id, &group);
				if(!obj)
					t.Throw("Missing object '%s'.", obj_id.c_str());
				t.Next();
				RoomType::Obj room_obj;
				if(group)
				{
					room_obj.group = group;
					room_obj.is_group = true;
				}
				else
				{
					room_obj.obj2 = obj;
					room_obj.is_group = false;
				}
				if(t.IsInt())
				{
					room_obj.count.x = t.GetUint();
					t.Next();
					if(t.IsInt())
					{
						room_obj.count.y = t.GetUint();
						t.Next();
					}
					else
						room_obj.count.y = room_obj.count.x;
				}
				else
					room_obj.count = Int2(1);
				type->objs.push_back(room_obj);
			}
			t.Next();
			break;
		}
	}

	if(type->objs.empty())
		LoadWarning("Room without objects.");

	RoomType::types.push_back(type.Pin());
}

//=================================================================================================
void LocationLoader::Finalize()
{
	content.crc[(int)Content::Id::Locations] = 0;
}
