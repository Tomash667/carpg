#include "Pch.h"
#include "GameCore.h"
#include "ItemLoader.h"
#include "Item.h"
#include "Stock.h"
#include "ScriptManager.h"
#include <ResourceManager.h>
#include <Mesh.h>

//-----------------------------------------------------------------------------
enum Group
{
	G_ITEM_TYPE,
	G_PROPERTY,
	G_WEAPON_TYPE,
	G_MATERIAL,
	G_DMG_TYPE,
	G_FLAGS,
	G_ARMOR_TYPE,
	G_ARMOR_UNIT_TYPE,
	G_CONSUMABLE_TYPE,
	G_OTHER_TYPE,
	G_STOCK_KEYWORD,
	G_BOOK_SCHEME_PROPERTY,
	G_SKILL,
	G_EFFECT,
	G_TAG
};

enum Property
{
	P_WEIGHT,
	P_VALUE,
	P_AI_VALUE,
	P_MESH,
	P_TEX,
	P_ATTACK,
	P_REQ_STR,
	P_TYPE,
	P_MATERIAL,
	P_DMG_TYPE,
	P_FLAGS,
	P_DEFENSE,
	P_MOBILITY,
	P_UNIT_TYPE,
	P_TEX_OVERRIDE,
	P_TIME,
	P_SPEED,
	P_SCHEME,
	P_RUNIC,
	P_BLOCK,
	P_EFFECTS,
	P_TAG
	// max 32 bits
};

enum StockKeyword
{
	SK_SET,
	SK_CITY,
	SK_ELSE,
	SK_CHANCE,
	SK_RANDOM,
	SK_SAME,
	SK_SCRIPT
};

enum BookSchemeProperty
{
	BSP_TEXTURE,
	BSP_SIZE,
	BSP_REGIONS,
	BSP_PREV,
	BSP_NEXT
};

//=================================================================================================
void ItemLoader::DoLoading()
{
	bool require_id[IT_MAX];
	std::fill_n(require_id, IT_MAX, true);
	require_id[IT_START_ITEMS] = false;
	require_id[IT_BETTER_ITEMS] = false;

	Load("items.txt", G_ITEM_TYPE, require_id);
}

//=================================================================================================
void ItemLoader::Cleanup()
{
	DeleteElements(BookScheme::book_schemes);
	DeleteElements(ItemList::lists);
	DeleteElements(LeveledItemList::lists);
	DeleteElements(Stock::stocks);

	for(auto it : Item::items)
		delete it.second;
	Item::items.clear();
}

//=================================================================================================
void ItemLoader::InitTokenizer()
{
	t.SetFlags(Tokenizer::F_UNESCAPE | Tokenizer::F_MULTI_KEYWORDS);

	t.AddKeywords(G_ITEM_TYPE, {
		{ "weapon", IT_WEAPON },
		{ "bow", IT_BOW },
		{ "shield", IT_SHIELD },
		{ "armor", IT_ARMOR },
		{ "amulet", IT_AMULET },
		{ "ring", IT_RING },
		{ "other", IT_OTHER },
		{ "consumable", IT_CONSUMABLE },
		{ "book", IT_BOOK },
		{ "gold", IT_GOLD },
		{ "list", IT_LIST },
		{ "leveled_list", IT_LEVELED_LIST },
		{ "stock", IT_STOCK },
		{ "book_scheme", IT_BOOK_SCHEME },
		{ "start_items", IT_START_ITEMS },
		{ "better_items", IT_BETTER_ITEMS },
		{ "alias", IT_ALIAS }
		});

	t.AddKeywords(G_PROPERTY, {
		{ "weight", P_WEIGHT },
		{ "value", P_VALUE },
		{ "ai_value", P_AI_VALUE },
		{ "mesh", P_MESH },
		{ "tex", P_TEX },
		{ "attack", P_ATTACK },
		{ "req_str", P_REQ_STR },
		{ "type", P_TYPE },
		{ "material", P_MATERIAL },
		{ "dmg_type", P_DMG_TYPE },
		{ "flags", P_FLAGS },
		{ "defense", P_DEFENSE },
		{ "mobility", P_MOBILITY },
		{ "unit_type", P_UNIT_TYPE },
		{ "tex_override", P_TEX_OVERRIDE },
		{ "time", P_TIME },
		{ "speed", P_SPEED },
		{ "scheme", P_SCHEME },
		{ "runic", P_RUNIC },
		{ "block", P_BLOCK },
		{ "effects", P_EFFECTS },
		{ "tag", P_TAG }
		});

	t.AddKeywords(G_WEAPON_TYPE, {
		{ "short_blade", WT_SHORT_BLADE },
		{ "long_blade", WT_LONG_BLADE },
		{ "axe", WT_AXE },
		{ "blunt", WT_BLUNT }
		});

	t.AddKeywords(G_MATERIAL, {
		{ "wood", MAT_WOOD },
		{ "skin", MAT_SKIN },
		{ "metal", MAT_IRON },
		{ "crystal", MAT_CRYSTAL },
		{ "cloth", MAT_CLOTH },
		{ "rock", MAT_ROCK },
		{ "body", MAT_BODY },
		{ "bone", MAT_BONE }
		});

	t.AddKeywords(G_DMG_TYPE, {
		{ "slash", DMG_SLASH },
		{ "pierce", DMG_PIERCE },
		{ "blunt", DMG_BLUNT }
		});

	// register item flags (ITEM_TEX_ONLY is not flag in item but property)
	t.AddKeywords(G_FLAGS, {
		{ "not_chest", ITEM_NOT_CHEST },
		{ "not_shop", ITEM_NOT_SHOP },
		{ "not_alchemist", ITEM_NOT_ALCHEMIST },
		{ "quest", ITEM_QUEST },
		{ "not_blacksmith", ITEM_NOT_BLACKSMITH },
		{ "mage", ITEM_MAGE },
		{ "dont_drop", ITEM_DONT_DROP },
		{ "ground_mesh", ITEM_GROUND_MESH },
		{ "crystal_sound", ITEM_CRYSTAL_SOUND },
		{ "important", ITEM_IMPORTANT },
		{ "not_merchant", ITEM_NOT_MERCHANT },
		{ "not_random", ITEM_NOT_RANDOM },
		{ "hq", ITEM_HQ },
		{ "magical", ITEM_MAGICAL },
		{ "unique", ITEM_UNIQUE },
		{ "alpha", ITEM_ALPHA },
		{ "magic_scroll", ITEM_MAGIC_SCROLL }
		});

	t.AddKeywords(G_ARMOR_TYPE, {
		{ "light", AT_LIGHT },
		{ "medium", AT_MEDIUM },
		{ "heavy", AT_HEAVY }
		});

	t.AddKeywords(G_ARMOR_UNIT_TYPE, {
		{ "human", (int)ArmorUnitType::HUMAN },
		{ "goblin", (int)ArmorUnitType::GOBLIN },
		{ "orc", (int)ArmorUnitType::ORC }
		});

	t.AddKeywords(G_CONSUMABLE_TYPE, {
		{ "potion", (int)ConsumableType::Potion },
		{ "drink", (int)ConsumableType::Drink },
		{ "food", (int)ConsumableType::Food },
		{ "herb", (int)ConsumableType::Herb }
		});

	t.AddKeywords(G_OTHER_TYPE, {
		{ "tool", Tool },
		{ "valuable", Valuable },
		{ "other", OtherItems },
		{ "artifact", Artifact }
		});

	t.AddKeywords(G_STOCK_KEYWORD, {
		{ "set", SK_SET },
		{ "city", SK_CITY },
		{ "else", SK_ELSE },
		{ "chance", SK_CHANCE },
		{ "random", SK_RANDOM },
		{ "same", SK_SAME },
		{ "script", SK_SCRIPT }
		});

	t.AddKeywords(G_BOOK_SCHEME_PROPERTY, {
		{ "texture", BSP_TEXTURE },
		{ "size", BSP_SIZE },
		{ "regions", BSP_REGIONS },
		{ "prev", BSP_PREV },
		{ "next", BSP_NEXT }
		});

	for(Skill& si : Skill::skills)
		t.AddKeyword(si.id, (int)si.skill_id, G_SKILL);

	for(int i = 0; i < (int)EffectId::Max; ++i)
		t.AddKeyword(EffectInfo::effects[i].id, i, G_EFFECT);

	t.AddKeywords(G_TAG, {
		{ "str", TAG_STR },
		{ "dex", TAG_DEX },
		{ "melee", TAG_MELEE },
		{ "ranged", TAG_RANGED },
		{ "def", TAG_DEF },
		{ "stamina", TAG_STAMINA },
		{ "mage", TAG_MAGE },
		{ "mana", TAG_MANA },
		{ "cleric", TAG_CLERIC }
		});
}

//=================================================================================================
void ItemLoader::LoadEntity(int top, const string& id)
{
	ITEM_TYPE type = (ITEM_TYPE)top;
	switch(type)
	{
	default:
		ParseItem(type, id);
		break;
	case IT_LIST:
		ParseItemList(id);
		break;
	case IT_LEVELED_LIST:
		ParseLeveledItemList(id);
		break;
	case IT_STOCK:
		ParseStock(id);
		break;
	case IT_BOOK_SCHEME:
		ParseBookScheme(id);
		break;
	case IT_START_ITEMS:
		ParseStartItems();
		break;
	case IT_BETTER_ITEMS:
		ParseBetterItems();
		break;
	case IT_ALIAS:
		ParseAlias(id);
		break;
	}
}

//=================================================================================================
void ItemLoader::Finalize()
{
	Item::gold = Item::Get("gold");

	CalculateCrc();

	Info("Loaded items (%u), lists (%u) - crc %p.", Item::items.size(), ItemList::lists.size() + LeveledItemList::lists.size(),
		content.crc[(int)Content::Id::Items]);
}

//=================================================================================================
void ItemLoader::ParseItem(ITEM_TYPE type, const string& id)
{
	ItemsMap::iterator it = Item::items.lower_bound(id.c_str());
	if(it != Item::items.end() && id == it->first)
		t.Throw("Id must be unique.");

	// create
	int req = Bit(P_WEIGHT) | Bit(P_VALUE) | Bit(P_AI_VALUE) | Bit(P_MESH) | Bit(P_TEX) | Bit(P_FLAGS);
	Ptr<Item> item(nullptr);
	switch(type)
	{
	case IT_WEAPON:
		item = new Weapon;
		req |= Bit(P_ATTACK) | Bit(P_REQ_STR) | Bit(P_TYPE) | Bit(P_MATERIAL) | Bit(P_DMG_TYPE) | Bit(P_EFFECTS);
		break;
	case IT_BOW:
		item = new Bow;
		req |= Bit(P_ATTACK) | Bit(P_REQ_STR) | Bit(P_SPEED) | Bit(P_EFFECTS);
		break;
	case IT_SHIELD:
		item = new Shield;
		req |= Bit(P_BLOCK) | Bit(P_REQ_STR) | Bit(P_MATERIAL) | Bit(P_EFFECTS);
		break;
	case IT_ARMOR:
		item = new Armor;
		req |= Bit(P_DEFENSE) | Bit(P_MOBILITY) | Bit(P_REQ_STR) | Bit(P_MATERIAL) | Bit(P_UNIT_TYPE) | Bit(P_TYPE) | Bit(P_TEX_OVERRIDE) | Bit(P_EFFECTS);
		break;
	case IT_AMULET:
		item = new Amulet;
		req |= Bit(P_EFFECTS) | Bit(P_TAG);
		break;
	case IT_RING:
		item = new Ring;
		req |= Bit(P_EFFECTS) | Bit(P_TAG);
		break;
	case IT_CONSUMABLE:
		item = new Consumable;
		req |= Bit(P_TIME) | Bit(P_TYPE) | Bit(P_EFFECTS);
		break;
	case IT_BOOK:
		item = new Book;
		req |= Bit(P_SCHEME) | Bit(P_RUNIC);
		break;
	case IT_GOLD:
		item = new Item(IT_GOLD);
		break;
	case IT_OTHER:
	default:
		item = new OtherItem;
		req |= Bit(P_TYPE);
		break;
	}

	// id
	item->id = id;
	t.Next();

	// parent item
	if(t.IsSymbol(':'))
	{
		t.Next();
		const string& parent_id = t.MustGetItem();
		Item* parent = Item::TryGet(parent_id);
		if(!parent)
			t.Throw("Missing parent item '%s'.", parent_id.c_str());
		if(parent->type != type)
			t.Throw("Item can't inherit from other item type.");
		*item = *parent;
		t.Next();
	}

	// {
	t.AssertSymbol('{');
	t.Next();

	// properties
	while(!t.IsSymbol('}'))
	{
		Property prop = (Property)t.MustGetKeywordId(G_PROPERTY);
		if(!IsSet(req, Bit(prop)))
			t.Throw("Can't have property '%s'.", t.GetTokenString().c_str());

		t.Next();

		switch(prop)
		{
		case P_WEIGHT:
			item->weight = int(t.MustGetFloat() * 10);
			if(item->weight < 0)
				t.Throw("Can't have negative weight %g.", item->GetWeight());
			break;
		case P_VALUE:
			item->value = t.MustGetInt();
			if(item->value < 0)
				t.Throw("Can't have negative value %d.", item->value);
			break;
		case P_AI_VALUE:
			item->ai_value = t.MustGetInt();
			if(item->value < 0)
				t.Throw("Can't have negative ai value %d.", item->ai_value);
			break;
		case P_MESH:
			{
				if(IsSet(item->flags, ITEM_TEX_ONLY))
					t.Throw("Can't have mesh, it is texture only item.");
				const string& mesh_id = t.MustGetString();
				item->mesh = res_mgr->TryGet<Mesh>(mesh_id);
				if(!item->mesh)
					LoadError("Missing mesh '%s'.", mesh_id.c_str());
			}
			break;
		case P_TEX:
			{
				if(item->mesh)
					t.Throw("Can't be texture only item, it have mesh.");
				const string& tex_id = t.MustGetString();
				item->tex = res_mgr->TryGet<Texture>(tex_id);
				if(!item->tex)
					LoadError("Missing texture '%s'.", tex_id.c_str());
				else
					item->flags |= ITEM_TEX_ONLY;
			}
			break;
		case P_ATTACK:
			{
				int attack = t.MustGetInt();
				if(attack < 0)
					t.Throw("Can't have negative attack %d.", attack);
				if(item->type == IT_WEAPON)
					item->ToWeapon().dmg = attack;
				else
					item->ToBow().dmg = attack;
			}
			break;
		case P_REQ_STR:
			{
				int req_str = t.MustGetInt();
				if(req_str < 0)
					t.Throw("Can't have negative required strength %d.", req_str);
				switch(item->type)
				{
				case IT_WEAPON:
					item->ToWeapon().req_str = req_str;
					break;
				case IT_BOW:
					item->ToBow().req_str = req_str;
					break;
				case IT_SHIELD:
					item->ToShield().req_str = req_str;
					break;
				case IT_ARMOR:
					item->ToArmor().req_str = req_str;
					break;
				}
			}
			break;
		case P_TYPE:
			switch(item->type)
			{
			case IT_WEAPON:
				item->ToWeapon().weapon_type = (WEAPON_TYPE)t.MustGetKeywordId(G_WEAPON_TYPE);
				break;
			case IT_ARMOR:
				item->ToArmor().armor_type = (ARMOR_TYPE)t.MustGetKeywordId(G_ARMOR_TYPE);
				break;
			case IT_CONSUMABLE:
				item->ToConsumable().cons_type = (ConsumableType)t.MustGetKeywordId(G_CONSUMABLE_TYPE);
				break;
			case IT_OTHER:
				item->ToOther().other_type = (OtherType)t.MustGetKeywordId(G_OTHER_TYPE);
				break;
			}
			break;
		case P_UNIT_TYPE:
			item->ToArmor().armor_unit_type = (ArmorUnitType)t.MustGetKeywordId(G_ARMOR_UNIT_TYPE);
			break;
		case P_MATERIAL:
			{
				MATERIAL_TYPE mat = (MATERIAL_TYPE)t.MustGetKeywordId(G_MATERIAL);
				switch(item->type)
				{
				case IT_WEAPON:
					item->ToWeapon().material = mat;
					break;
				case IT_SHIELD:
					item->ToShield().material = mat;
					break;
				case IT_ARMOR:
					item->ToArmor().material = mat;
					break;
				}
			}
			break;
		case P_DMG_TYPE:
			t.ParseFlags(G_DMG_TYPE, item->ToWeapon().dmg_type);
			break;
		case P_FLAGS:
			{
				int prev = item->flags & ITEM_TEX_ONLY;
				t.ParseFlags(G_FLAGS, item->flags);
				item->flags |= prev;
			}
			break;
		case P_DEFENSE:
			{
				int def = t.MustGetInt();
				if(def < 0)
					t.Throw("Can't have negative defense %d.", def);
				item->ToArmor().def = def;
			}
			break;
		case P_BLOCK:
			{
				int block = t.MustGetInt();
				if(block < 0)
					t.Throw("Can't have negative block %d.", block);
				item->ToShield().block = block;
			}
			break;
		case P_MOBILITY:
			{
				int mob = t.MustGetInt();
				if(mob < 0)
					t.Throw("Can't have negative mobility %d.", mob);
				item->ToArmor().mobility = mob;
			}
			break;
		case P_TEX_OVERRIDE:
			{
				vector<TexOverride>& tex_o = item->ToArmor().tex_override;
				if(t.IsSymbol('{'))
				{
					t.Next();
					do
					{
						const string& tex_id = t.MustGetString();
						Texture* tex = res_mgr->TryGet<Texture>(tex_id);
						if(tex)
							tex_o.push_back(TexOverride(tex));
						else
							LoadError("Missing texture override '%s'.", tex_id.c_str());
						t.Next();
					}
					while(!t.IsSymbol('}'));
				}
				else
				{
					const string& tex_id = t.MustGetString();
					Texture* tex = res_mgr->TryGet<Texture>(tex_id);
					if(tex)
						tex_o.push_back(TexOverride(tex));
					else
						LoadError("Missing texture override '%s'.", tex_id.c_str());
				}
			}
			break;
		case P_TIME:
			{
				float time = t.MustGetFloat();
				if(time < 0.f)
					t.Throw("Can't have negative time %g.", time);
				item->ToConsumable().time = time;
			}
			break;
		case P_SPEED:
			{
				int speed = t.MustGetInt();
				if(speed < 1)
					t.Throw("Can't have less than 1 speed %d.", speed);
				item->ToBow().speed = speed;
			}
			break;
		case P_SCHEME:
			{
				const string& str = t.MustGetText();
				BookScheme* scheme = BookScheme::TryGet(str);
				if(!scheme)
					t.Throw("Book scheme '%s' not found.", str.c_str());
				item->ToBook().scheme = scheme;
			}
			break;
		case P_RUNIC:
			item->ToBook().runic = t.MustGetBool();
			break;
		case P_EFFECTS:
			t.AssertSymbol('{');
			t.Next();
			while(!t.IsSymbol('}'))
			{
				bool on_attack = false;
				if(t.IsItem("on_attack"))
				{
					on_attack = true;
					t.Next();
				}
				EffectId effect = (EffectId)t.MustGetKeywordId(G_EFFECT);
				t.Next();
				int effect_value;
				EffectInfo& info = EffectInfo::effects[(int)effect];
				if(info.value_type != EffectInfo::None)
				{
					if(info.value_type == EffectInfo::Attribute)
					{
						const string& value = t.MustGetItemKeyword();
						Attribute* attrib = Attribute::Find(value);
						if(!attrib)
							t.Throw("Invalid attribute '%s' for effect '%s'.", value.c_str(), info.id);
						effect_value = (int)attrib->attrib_id;
					}
					else
						effect_value = t.MustGetKeywordId(G_SKILL);
					t.Next();
				}
				else
					effect_value = -1;
				float power = t.MustGetFloat();
				t.Next();
				item->effects.push_back({ effect, power, effect_value, on_attack });
			}
			break;
		case P_TAG:
			{
				ItemTag* tags;
				if(item->type == IT_AMULET)
					tags = item->ToAmulet().tag;
				else
					tags = item->ToRing().tag;
				for(int i = 0; i < MAX_ITEM_TAGS; ++i)
					tags[i] = TAG_NONE;
				if(t.IsSymbol('{'))
				{
					t.Next();
					int index = 0;
					while(!t.IsSymbol('}'))
					{
						if(index >= MAX_ITEM_TAGS)
							t.Throw("Too many item tags.");
						tags[index] = (ItemTag)t.MustGetKeywordId(G_TAG);
						++index;
						t.Next();
					}
				}
				else
					tags[0] = (ItemTag)t.MustGetKeywordId(G_TAG);
			}
			break;
		default:
			assert(0);
			break;
		}

		t.Next();
	}

	if(!item->mesh && !item->tex)
		LoadError("No mesh/texture.");

	Item* item_ptr = item.Pin();
	Item::items.insert(it, ItemsMap::value_type(item_ptr->id.c_str(), item_ptr));

	switch(item_ptr->type)
	{
	case IT_WEAPON:
		Weapon::weapons.push_back(static_cast<Weapon*>(item_ptr));
		break;
	case IT_BOW:
		Bow::bows.push_back(static_cast<Bow*>(item_ptr));
		break;
	case IT_SHIELD:
		Shield::shields.push_back(static_cast<Shield*>(item_ptr));
		break;
	case IT_ARMOR:
		Armor::armors.push_back(static_cast<Armor*>(item_ptr));
		break;
	case IT_AMULET:
		Amulet::amulets.push_back(static_cast<Amulet*>(item_ptr));
		break;
	case IT_RING:
		Ring::rings.push_back(static_cast<Ring*>(item_ptr));
		break;
	case IT_CONSUMABLE:
		{
			Consumable* consumable = static_cast<Consumable*>(item_ptr);
			if(consumable->cons_type == ConsumableType::Potion)
			{
				for(ItemEffect& e : consumable->effects)
				{
					if(e.effect == EffectId::Heal && e.power > 0.f)
					{
						consumable->ai_type = ConsumableAiType::Healing;
						break;
					}
					else if(e.effect == EffectId::RestoreMana && e.power > 0.f)
					{
						consumable->ai_type = ConsumableAiType::Mana;
						break;
					}
				}
			}
			Consumable::consumables.push_back(consumable);
		}
		break;
	case IT_OTHER:
		{
			OtherItem& o = item_ptr->ToOther();
			OtherItem::others.push_back(&o);
			if(o.other_type == Artifact)
				OtherItem::artifacts.push_back(&o);
		}
		break;
	case IT_BOOK:
		{
			Book& b = item_ptr->ToBook();
			if(!b.scheme)
				t.Throw("Missing book '%s' scheme.", b.id.c_str());
			Book::books.push_back(&b);
		}
		break;
	case IT_GOLD:
		break;
	default:
		assert(0);
		break;
	}
}

//=================================================================================================
void ItemLoader::ParseItemList(const string& id)
{
	ItemListResult existing_list = ItemList::TryGet(id);
	if(existing_list)
		t.Throw("Id must be unique.");

	Ptr<ItemList> lis;

	// id
	lis->id = id;
	t.Next();

	// {
	t.AssertSymbol('{');
	t.Next();

	while(!t.IsSymbol('}'))
	{
		const string& item_id = t.MustGetItemKeyword();
		Item* item = Item::TryGet(item_id);
		if(!item)
			t.Throw("Missing item %s.", item_id.c_str());
		lis->items.push_back(item);
		t.Next();
	}

	ItemList::lists.push_back(lis.Pin());
}

//=================================================================================================
void ItemLoader::ParseLeveledItemList(const string& id)
{
	ItemListResult existing_list = ItemList::TryGet(id);
	if(existing_list)
		t.Throw("Id must be unique.");

	Ptr<LeveledItemList> lis;

	// id
	lis->id = t.MustGetItemKeyword();
	t.Next();

	// {
	t.AssertSymbol('{');
	t.Next();

	while(!t.IsSymbol('}'))
	{
		Item* item = Item::TryGet(t.MustGetItemKeyword());
		if(!item)
			t.Throw("Missing item '%s'.", t.GetTokenString().c_str());

		t.Next();
		int level = t.MustGetInt();
		if(level < 0)
			t.Throw("Can't have negative required level %d for item '%s'.", level, item->id.c_str());

		lis->items.push_back({ item, level });
		t.Next();
	}

	LeveledItemList::lists.push_back(lis.Pin());
}

//=================================================================================================
void ItemLoader::ParseStock(const string& id)
{
	Stock* existing_stock = Stock::TryGet(id);
	if(existing_stock)
		t.Throw("Id must be unique.");

	Ptr<Stock> stock;
	bool in_set = false, in_city = false, in_city_else;

	// id
	stock->id = t.MustGetItemKeyword();
	t.Next();

	// {
	t.AssertSymbol('{');
	t.Next();

	while(true)
	{
		if(t.IsSymbol('}'))
		{
			if(in_city)
			{
				t.Next();
				if(!in_city_else && t.IsKeyword(SK_ELSE, G_STOCK_KEYWORD))
				{
					in_city_else = true;
					stock->code.push_back(SE_NOT_CITY);
					t.Next();
					t.AssertSymbol('{');
					t.Next();
				}
				else
				{
					in_city = false;
					stock->code.push_back(SE_ANY_CITY);
				}
			}
			else if(in_set)
			{
				in_set = false;
				stock->code.push_back(SE_END_SET);
				t.Next();
			}
			else
				break;
		}
		else if(t.IsKeywordGroup(G_STOCK_KEYWORD))
		{
			StockKeyword k = (StockKeyword)t.GetKeywordId(G_STOCK_KEYWORD);
			t.Next();

			switch(k)
			{
			case SK_SET:
				if(in_set)
					t.Throw("Can't have nested sets.");
				if(in_city)
					t.Throw("Can't have set block inside city block.");
				in_set = true;
				stock->code.push_back(SE_START_SET);
				t.AssertSymbol('{');
				t.Next();
				break;
			case SK_CITY:
				if(in_city)
					t.Throw("Already in city block.");
				if(t.IsSymbol('{'))
				{
					t.Next();
					in_city = true;
					in_city_else = false;
					stock->code.push_back(SE_CITY);
				}
				else if(t.IsItem())
				{
					stock->code.push_back(SE_CITY);
					stock->code.push_back(SE_ADD);
					ParseItemOrList(stock);
					stock->code.push_back(SE_ANY_CITY);
				}
				else
				{
					char c = '{';
					t.StartUnexpected().Add(tokenizer::T_SYMBOL, (int*)&c).Add(tokenizer::T_ITEM).Throw();
				}
				break;
			case SK_CHANCE:
				if(t.IsSymbol('{'))
				{
					// chance { item X   item2 Y   ... }
					t.Next();
					stock->code.push_back(SE_CHANCE);
					uint chance_pos = stock->code.size();
					stock->code.push_back(0);
					stock->code.push_back(0);
					int count = 0, chance = 0;
					while(!t.IsSymbol('}'))
					{
						ParseItemOrList(stock);
						int ch = t.MustGetInt();
						if(ch <= 0)
							t.Throw("Negative chance %d.", ch);
						++count;
						chance += ch;
						stock->code.push_back(ch);
						t.Next();
					}
					if(count <= 1)
						t.Throw("Chance with 1 or less items.");
					stock->code[chance_pos] = count;
					stock->code[chance_pos + 1] = chance;
					t.Next();
				}
				else
				{
					// chance item1 item2
					stock->code.push_back(SE_CHANCE);
					stock->code.push_back(2);
					stock->code.push_back(2);
					for(int i = 0; i < 2; ++i)
					{
						ParseItemOrList(stock);
						stock->code.push_back(1);
					}
				}
				break;
			case SK_RANDOM:
				{
					// Random X Y [same] item
					int a = t.MustGetInt();
					t.Next();
					int b = t.MustGetInt();
					if(a >= b || a < 0 || b < 1)
						t.Throw("Invalid random values (%d, %d).", a, b);
					t.Next();

					StockEntry type = SE_RANDOM;
					if(t.IsKeyword(SK_SAME, G_STOCK_KEYWORD))
					{
						type = SE_SAME_RANDOM;
						t.Next();
					}

					stock->code.push_back(type);
					stock->code.push_back(a);
					stock->code.push_back(b);
					ParseItemOrList(stock);
				}
				break;
			case SK_SCRIPT:
				{
					if(!stock->code.empty())
						t.Throw("Stock script must be first command.");
					if(stock->script)
						t.Throw("Stock script already used.");
					const string& block = t.GetBlock('{', '}', false);
					stock->script = script_mgr->PrepareScript(Format("stock_%s", stock->id.c_str()), block.c_str());
					if(!stock->script)
						t.Throw("Failed to parse script.");
					t.Next();
				}
				break;
			default:
				t.Unexpected();
				break;
			}
		}
		else if(t.IsInt())
		{
			// X [same] item
			int count = t.GetInt();
			if(count < 1)
				t.Throw("Invalid count %d.", count);
			t.Next();

			StockEntry type = SE_MULTIPLE;
			if(t.IsKeyword(SK_SAME, G_STOCK_KEYWORD))
			{
				type = SE_SAME_MULTIPLE;
				t.Next();
			}

			stock->code.push_back(type);
			stock->code.push_back(count);
			ParseItemOrList(stock);
		}
		else if(t.IsItem())
		{
			// item
			stock->code.push_back(SE_ADD);
			ParseItemOrList(stock);
		}
		else
			t.Unexpected();
	}

	if(stock->code.empty() && !stock->script)
		t.Throw("No code.");

	Stock::stocks.push_back(stock.Pin());
}

//=================================================================================================
void ItemLoader::ParseItemOrList(Stock* stock)
{
	ItemListResult result;
	const string& item_id = t.MustGetItemKeyword();
	const Item* item = FindItemOrList(item_id, result);
	if(result.lis != nullptr)
	{
		if(result.is_leveled)
		{
			stock->code.push_back(SE_LEVELED_LIST);
			stock->code.push_back((int)result.llis);
		}
		else
		{
			stock->code.push_back(SE_LIST);
			stock->code.push_back((int)result.lis);
		}
	}
	else if(item)
	{
		stock->code.push_back(SE_ITEM);
		stock->code.push_back((int)item);
	}
	else
		t.Throw("Missing item or list '%s'.", item_id.c_str());
	t.Next();
}

//=================================================================================================
void ItemLoader::ParseBookScheme(const string& id)
{
	BookScheme* existing_scheme = BookScheme::TryGet(id);
	if(existing_scheme)
		t.Throw("Id must be unique.");

	Ptr<BookScheme> scheme;

	// id
	scheme->id = id;
	t.Next();

	// {
	t.AssertSymbol('{');
	t.Next();

	while(!t.IsSymbol('}'))
	{
		BookSchemeProperty key = (BookSchemeProperty)t.MustGetKeywordId(G_BOOK_SCHEME_PROPERTY);
		t.Next();

		switch(key)
		{
		case BSP_TEXTURE:
			{
				const string& str = t.MustGetString();
				scheme->tex = res_mgr->TryGet<Texture>(str);
				if(!scheme->tex)
					t.Throw("Missing texture '%s'.", str.c_str());
				t.Next();
			}
			break;
		case BSP_SIZE:
			t.Parse(scheme->size);
			break;
		case BSP_REGIONS:
			t.AssertSymbol('{');
			t.Next();
			while(!t.IsSymbol('}'))
			{
				Rect b;
				t.Parse(b);
				scheme->regions.push_back(b);
			}
			t.Next();
			break;
		case BSP_PREV:
			t.Parse(scheme->prev);
			break;
		case BSP_NEXT:
			t.Parse(scheme->next);
			break;
		}
	}

	if(scheme->regions.empty())
		t.Throw("No regions.");
	for(uint i = 1; i < scheme->regions.size(); ++i)
	{
		if(scheme->regions[0].Size() != scheme->regions[i].Size())
			t.Throw("Scheme region sizes don't match (TODO).");
	}
	if(!scheme->tex)
		t.Throw("No texture.");

	BookScheme::book_schemes.push_back(scheme.Pin());
}

//=================================================================================================
void ItemLoader::ParseStartItems()
{
	if(!StartItem::start_items.empty())
		t.Throw("Start items already declared.");

	// {
	t.AssertSymbol('{');
	t.Next();

	while(!t.IsSymbol('}'))
	{
		SkillId skill = (SkillId)t.MustGetKeywordId(G_SKILL);
		t.Next();
		t.AssertSymbol('{');
		t.Next();

		while(!t.IsSymbol('}'))
		{
			int num;
			if(t.IsSymbol('*'))
				num = HEIRLOOM;
			else
			{
				num = t.MustGetInt();
				if(num < 0 || num > 100)
					t.Throw("Invalid skill value %d.", num);
			}
			t.Next();

			const string& str = t.MustGetItemKeyword();
			const Item* item = Item::TryGet(str);
			if(!item)
				t.Throw("Missing item '%s'.", str.c_str());
			t.Next();

			StartItem::start_items.push_back(StartItem(skill, item, num));
		}

		t.Next();
	}

	std::sort(StartItem::start_items.begin(), StartItem::start_items.end(),
		[](const StartItem& si1, const StartItem& si2) { return si1.skill > si2.skill; });
}

//=================================================================================================
void ItemLoader::ParseBetterItems()
{
	if(!better_items.empty())
		t.Throw("Better items already declared.");

	// {
	t.AssertSymbol('{');
	t.Next();

	while(!t.IsSymbol('}'))
	{
		const string& str = t.MustGetItemKeyword();
		Item* item = Item::TryGet(str);
		if(!item)
			t.Throw("Missing item '%s'.", str.c_str());
		t.Next();

		const string& str2 = t.MustGetItemKeyword();
		Item* item2 = Item::TryGet(str2);
		if(!item2)
			t.Throw("Missing item '%s'.", str2.c_str());

		better_items[item] = item2;
		t.Next();
	}
}

//=================================================================================================
void ItemLoader::ParseAlias(const string& id)
{
	Item* item = Item::TryGet(id);
	if(!item)
		t.Throw("Missing item '%s'.", id.c_str());
	t.Next();

	const string& alias = t.MustGetItemKeyword();
	Item* item2 = Item::TryGet(alias);
	if(item2)
		t.Throw("Can't create alias '%s', already exists.", alias.c_str());

	item_aliases[alias] = item;
}

//=================================================================================================
void ItemLoader::CalculateCrc()
{
	Crc crc;

	for(auto& it : Item::items)
	{
		Item* item = it.second;

		crc.Update(item->id);
		crc.Update(item->value);
		crc.Update(item->ai_value);
		if(item->mesh)
			crc.Update(item->mesh->filename);
		else if(item->tex)
			crc.Update(item->tex->filename);
		crc.Update(item->weight);
		crc.Update(item->value);
		crc.Update(item->flags);
		crc.Update(item->type);
		for(ItemEffect& effect : item->effects)
		{
			crc.Update(effect.effect);
			crc.Update(effect.power);
			crc.Update(effect.value);
			crc.Update(effect.on_attack);
		}

		switch(item->type)
		{
		case IT_WEAPON:
			{
				Weapon& w = item->ToWeapon();
				crc.Update(w.dmg);
				crc.Update(w.dmg_type);
				crc.Update(w.req_str);
				crc.Update(w.weapon_type);
				crc.Update(w.material);
			}
			break;
		case IT_BOW:
			{
				Bow& b = item->ToBow();
				crc.Update(b.dmg);
				crc.Update(b.req_str);
				crc.Update(b.speed);
			}
			break;
		case IT_SHIELD:
			{
				Shield& s = item->ToShield();
				crc.Update(s.block);
				crc.Update(s.req_str);
				crc.Update(s.material);
			}
			break;
		case IT_ARMOR:
			{
				Armor& a = item->ToArmor();
				crc.Update(a.def);
				crc.Update(a.req_str);
				crc.Update(a.mobility);
				crc.Update(a.material);
				crc.Update(a.armor_type);
				crc.Update(a.armor_unit_type);
				crc.Update(a.tex_override.size());
				for(TexOverride& tex_o : a.tex_override)
					crc.Update(tex_o.diffuse ? tex_o.diffuse->filename : "");
			}
			break;
		case IT_AMULET:
			{
				Amulet& a = item->ToAmulet();
				crc.Update(a.tag);
			}
			break;
		case IT_RING:
			{
				Ring& r = item->ToRing();
				crc.Update(r.tag);
			}
			break;
		case IT_CONSUMABLE:
			{
				Consumable& c = item->ToConsumable();
				crc.Update(c.time);
				crc.Update(c.cons_type);
			}
			break;
		case IT_OTHER:
			{
				OtherItem& o = item->ToOther();
				crc.Update(o.other_type);
			}
			break;
		case IT_BOOK:
			{
				Book& b = item->ToBook();
				crc.Update(b.scheme->id);
			}
			break;
		case IT_GOLD:
			break;
		default:
			assert(0);
			break;
		}
	}

	for(ItemList* lis : ItemList::lists)
	{
		crc.Update(lis->id);
		crc.Update(lis->items.size());
		for(const Item* i : lis->items)
			crc.Update(i->id);
	}

	for(LeveledItemList* lis : LeveledItemList::lists)
	{
		crc.Update(lis->id);
		crc.Update(lis->items.size());
		for(LeveledItemList::Entry& e : lis->items)
		{
			crc.Update(e.item->id);
			crc.Update(e.level);
		}
	}

	for(auto scheme : BookScheme::book_schemes)
	{
		crc.Update(scheme->id);
		crc.Update(scheme->tex->filename);
		crc.Update(scheme->size);
		crc.Update(scheme->prev);
		crc.Update(scheme->next);
		crc.Update(scheme->regions);
	}

	content.crc[(int)Content::Id::Items] = crc.Get();
}
