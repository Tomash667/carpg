#include "Pch.h"
#include "LocationLoader.h"
#include "RoomType.h"
#include "BaseObject.h"

enum Group
{
	G_TOP,
	G_ROOM_KEYWORD,
	G_ROOM_FLAG,
	G_OBJECT_KEYWORD,
	G_OBJECT_FLAG
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

enum ObjectKeyword
{
	OK_POS,
	OK_ROT,
	OK_FLAGS
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
	t.SetFlags(Tokenizer::F_MULTI_KEYWORDS);

	t.AddKeyword("room", T_ROOM, G_TOP);

	t.AddKeywords(G_ROOM_KEYWORD, {
		{ "flags", RK_FLAGS },
		{ "objects", RK_OBJECTS }
		});

	t.AddKeywords(G_ROOM_FLAG, {
		{ "repeat", RoomType::REPEAT },
		{ "treasure", RoomType::TREASURE }
		});

	t.AddKeywords(G_OBJECT_KEYWORD, {
		{ "pos", OK_POS },
		{ "rot", OK_ROT },
		{ "flags", OK_FLAGS }
		});

	t.AddKeywords(G_OBJECT_FLAG, {
		{ "required", RoomType::Obj::F_REQUIRED },
		{ "in_middle", RoomType::Obj::F_IN_MIDDLE }
		});
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
				room_obj.force_pos = false;
				room_obj.force_rot = false;
				room_obj.flags = 0;
				if(group)
				{
					room_obj.group = group;
					room_obj.is_group = true;
				}
				else
				{
					room_obj.obj = obj;
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
				if(t.IsSymbol('{'))
				{
					t.Next();
					while(!t.IsSymbol('}'))
					{
						const ObjectKeyword k = (ObjectKeyword)t.MustGetKeywordId(G_OBJECT_KEYWORD);
						t.Next();

						switch(k)
						{
						case OK_POS:
							t.Parse(room_obj.pos);
							room_obj.force_pos = true;
							break;
						case OK_ROT:
							room_obj.rot = t.MustGetFloat();
							room_obj.force_rot = true;
							t.Next();
							break;
						case OK_FLAGS:
							t.ParseFlags(G_OBJECT_FLAG, room_obj.flags);
							t.Next();
							break;
						}
					}
					t.Next();
				}
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
