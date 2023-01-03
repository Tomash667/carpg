#include "Pch.h"
#include "LocationLoader.h"

#include "BaseObject.h"
#include "RoomType.h"

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
		{ "inMiddle", RoomType::Obj::F_IN_MIDDLE }
		});
}

//=================================================================================================
void LocationLoader::LoadEntity(int, const string& id)
{
	RoomType* existingType = RoomType::Get(id);
	if(existingType)
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
				const string& objId = t.MustGetItem();
				ObjectGroup* group = nullptr;
				BaseObject* obj = BaseObject::TryGet(objId, &group);
				if(!obj)
					t.Throw("Missing object '%s'.", objId.c_str());
				t.Next();
				RoomType::Obj roomObj;
				roomObj.forcePos = false;
				roomObj.forceRot = false;
				roomObj.flags = 0;
				if(group)
				{
					roomObj.group = group;
					roomObj.isGroup = true;
				}
				else
				{
					roomObj.obj = obj;
					roomObj.isGroup = false;
				}
				if(t.IsInt())
				{
					roomObj.count.x = t.GetUint();
					t.Next();
					if(t.IsInt())
					{
						roomObj.count.y = t.GetUint();
						t.Next();
					}
					else
						roomObj.count.y = roomObj.count.x;
				}
				else
					roomObj.count = Int2(1);
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
							t.Parse(roomObj.pos);
							roomObj.forcePos = true;
							break;
						case OK_ROT:
							roomObj.rot = t.MustGetFloat();
							roomObj.forceRot = true;
							t.Next();
							break;
						case OK_FLAGS:
							t.ParseFlags(G_OBJECT_FLAG, roomObj.flags);
							t.Next();
							break;
						}
					}
					t.Next();
				}
				type->objs.push_back(roomObj);
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
