#include "Pch.h"
#include "ItemLoader.h"

#include "Item.h"
#include "ScriptManager.h"
#include "Stock.h"

#include <Mesh.h>
#include <ResourceManager.h>

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
	G_BOOK_TYPE,
	G_STOCK_KEYWORD,
	G_BOOK_SCHEME_PROPERTY,
	G_SKILL,
	G_EFFECT,
	G_TAG,
	G_RECIPE,
	G_LIST_TYPE
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
	P_RECIPES,
	P_BLOCK,
	P_EFFECTS,
	P_TAG,
	P_ATTACK_MOD
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

enum RecipeProperty
{
	RP_RESULT,
	RP_INGREDIENTS,
	RP_SKILL,
	RP_AUTOLEARN,
	RP_ORDER
};

enum ListType
{
	LT_LEVELED,
	LT_PRIORITY
};

//=================================================================================================
void ItemLoader::DoLoading()
{
	bool requireId[IT_MAX];
	std::fill_n(requireId, IT_MAX, true);
	requireId[IT_START_ITEMS] = false;
	requireId[IT_BETTER_ITEMS] = false;

	Load("items.txt", G_ITEM_TYPE, requireId);
}

//=================================================================================================
void ItemLoader::Cleanup()
{
	DeleteElements(BookScheme::bookSchemes);
	DeleteElements(ItemList::lists);
	DeleteElements(Stock::stocks);
	DeleteElements(Recipe::items);

	for(auto it : Item::items)
		delete it.second;
	Item::items.clear();
}

//=================================================================================================
void ItemLoader::InitTokenizer()
{
	t.SetFlags(Tokenizer::F_MULTI_KEYWORDS);

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
		{ "stock", IT_STOCK },
		{ "bookScheme", IT_BOOK_SCHEME },
		{ "startItems", IT_START_ITEMS },
		{ "betterItems", IT_BETTER_ITEMS },
		{ "alias", IT_ALIAS },
		{ "recipe", IT_RECIPE }
		});

	t.AddKeywords(G_PROPERTY, {
		{ "weight", P_WEIGHT },
		{ "value", P_VALUE },
		{ "aiValue", P_AI_VALUE },
		{ "mesh", P_MESH },
		{ "tex", P_TEX },
		{ "attack", P_ATTACK },
		{ "reqStr", P_REQ_STR },
		{ "type", P_TYPE },
		{ "material", P_MATERIAL },
		{ "dmgType", P_DMG_TYPE },
		{ "flags", P_FLAGS },
		{ "defense", P_DEFENSE },
		{ "mobility", P_MOBILITY },
		{ "unitType", P_UNIT_TYPE },
		{ "texOverride", P_TEX_OVERRIDE },
		{ "time", P_TIME },
		{ "speed", P_SPEED },
		{ "scheme", P_SCHEME },
		{ "runic", P_RUNIC },
		{ "recipes", P_RECIPES },
		{ "block", P_BLOCK },
		{ "effects", P_EFFECTS },
		{ "tag", P_TAG },
		{ "attackMod", P_ATTACK_MOD }
		});

	t.AddKeywords(G_WEAPON_TYPE, {
		{ "shortBlade", WT_SHORT_BLADE },
		{ "longBlade", WT_LONG_BLADE },
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
		{ "bone", MAT_BONE },
		{ "slime", MAT_SLIME }
		});

	t.AddKeywords(G_DMG_TYPE, {
		{ "slash", DMG_SLASH },
		{ "pierce", DMG_PIERCE },
		{ "blunt", DMG_BLUNT }
		});

	// register item flags (ITEM_TEX_ONLY is not flag in item but property)
	t.AddKeywords(G_FLAGS, {
		{ "notShop", ITEM_NOT_SHOP },
		{ "quest", ITEM_QUEST },
		{ "notBlacksmith", ITEM_NOT_BLACKSMITH },
		{ "mage", ITEM_MAGE },
		{ "dontDrop", ITEM_DONT_DROP },
		{ "groundMesh", ITEM_GROUND_MESH },
		{ "crystalSound", ITEM_CRYSTAL_SOUND },
		{ "important", ITEM_IMPORTANT },
		{ "notMerchant", ITEM_NOT_MERCHANT },
		{ "notRandom", ITEM_NOT_RANDOM },
		{ "hq", ITEM_HQ },
		{ "magical", ITEM_MAGICAL },
		{ "unique", ITEM_UNIQUE },
		{ "magicScroll", ITEM_MAGIC_SCROLL },
		{ "wand", ITEM_WAND },
		{ "ingredient", ITEM_INGREDIENT },
		{ "singleUse", ITEM_SINGLE_USE },
		{ "notTeam", ITEM_NOT_TEAM },
		{ "meatSound", ITEM_MEAT_SOUND }
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

	t.AddKeywords<Consumable::Subtype>(G_CONSUMABLE_TYPE, {
		{ "potion", Consumable::Subtype::Potion },
		{ "drink", Consumable::Subtype::Drink },
		{ "food", Consumable::Subtype::Food },
		{ "herb", Consumable::Subtype::Herb }
		});

	t.AddKeywords<OtherItem::Subtype>(G_OTHER_TYPE, {
		{ "miscItem", OtherItem::Subtype::MiscItem },
		{ "tool", OtherItem::Subtype::Tool },
		{ "valuable", OtherItem::Subtype::Valuable },
		{ "ingredient", OtherItem::Subtype::Ingredient }
		});

	t.AddKeywords<Book::Subtype>(G_BOOK_TYPE, {
		{ "normalBook", Book::Subtype::NormalBook },
		{ "recipe", Book::Subtype::Recipe }
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
		t.AddKeyword(si.id, (int)si.skillId, G_SKILL);

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

	t.AddKeywords(G_RECIPE, {
		{ "result", RP_RESULT },
		{ "ingredients", RP_INGREDIENTS },
		{ "skill", RP_SKILL },
		{ "autolearn", RP_AUTOLEARN },
		{ "order", RP_ORDER }
		});

	t.AddKeywords(G_LIST_TYPE, {
		{ "leveled", LT_LEVELED },
		{ "priority", LT_PRIORITY }
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
	case IT_RECIPE:
		ParseRecipe(id);
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

	// check if all recipes are defined
	for(Recipe* recipe : Recipe::items)
	{
		if(!recipe->defined)
			LoadError("Missing declared recipe '%s'.", recipe->id.c_str());
	}

	CalculateCrc();

	Info("Loaded items (%u), lists (%u) - crc %p.", Item::items.size(), ItemList::lists.size(), content.crc[(int)Content::Id::Items]);
}

//=================================================================================================
void ItemLoader::ParseItem(ITEM_TYPE type, const string& id)
{
	if(Item::TryGet(id))
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
		req |= Bit(P_BLOCK) | Bit(P_REQ_STR) | Bit(P_MATERIAL) | Bit(P_EFFECTS) | Bit(P_ATTACK_MOD);
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
		req |= Bit(P_SCHEME) | Bit(P_RUNIC) | Bit(P_RECIPES) | Bit(P_TYPE);
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
		const string& parentId = t.MustGetItem();
		Item* parent = Item::TryGet(parentId);
		if(!parent)
			t.Throw("Missing parent item '%s'.", parentId.c_str());
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
			item->aiValue = t.MustGetInt();
			if(item->value < 0)
				t.Throw("Can't have negative ai value %d.", item->aiValue);
			break;
		case P_MESH:
			{
				if(IsSet(item->flags, ITEM_TEX_ONLY))
					t.Throw("Can't have mesh, it is texture only item.");
				const string& meshId = t.MustGetString();
				item->mesh = resMgr->TryGet<Mesh>(meshId);
				if(!item->mesh)
					LoadError("Missing mesh '%s'.", meshId.c_str());
			}
			break;
		case P_TEX:
			{
				if(item->mesh)
					t.Throw("Can't be texture only item, it have mesh.");
				const string& texId = t.MustGetString();
				item->tex = resMgr->TryGet<Texture>(texId);
				if(!item->tex)
					LoadError("Missing texture '%s'.", texId.c_str());
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
				int reqStr = t.MustGetInt();
				if(reqStr < 0)
					t.Throw("Can't have negative required strength %d.", reqStr);
				switch(item->type)
				{
				case IT_WEAPON:
					item->ToWeapon().reqStr = reqStr;
					break;
				case IT_BOW:
					item->ToBow().reqStr = reqStr;
					break;
				case IT_SHIELD:
					item->ToShield().reqStr = reqStr;
					break;
				case IT_ARMOR:
					item->ToArmor().reqStr = reqStr;
					break;
				}
			}
			break;
		case P_TYPE:
			switch(item->type)
			{
			case IT_WEAPON:
				item->ToWeapon().weaponType = (WEAPON_TYPE)t.MustGetKeywordId(G_WEAPON_TYPE);
				break;
			case IT_ARMOR:
				item->ToArmor().armorType = (ARMOR_TYPE)t.MustGetKeywordId(G_ARMOR_TYPE);
				break;
			case IT_CONSUMABLE:
				item->ToConsumable().subtype = (Consumable::Subtype)t.MustGetKeywordId(G_CONSUMABLE_TYPE);
				break;
			case IT_OTHER:
				item->ToOther().subtype = (OtherItem::Subtype)t.MustGetKeywordId(G_OTHER_TYPE);
				break;
			case IT_BOOK:
				item->ToBook().subtype = (Book::Subtype)t.MustGetKeywordId(G_BOOK_TYPE);
				break;
			}
			break;
		case P_UNIT_TYPE:
			item->ToArmor().armorUnitType = (ArmorUnitType)t.MustGetKeywordId(G_ARMOR_UNIT_TYPE);
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
			t.ParseFlags(G_DMG_TYPE, item->ToWeapon().dmgType);
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
				vector<TexOverride>& texOverride = item->ToArmor().texOverride;
				if(t.IsSymbol('{'))
				{
					t.Next();
					do
					{
						const string& texId = t.MustGetString();
						Texture* tex = resMgr->TryGet<Texture>(texId);
						if(tex)
							texOverride.push_back(TexOverride(tex));
						else
							LoadError("Missing texture override '%s'.", texId.c_str());
						t.Next();
					}
					while(!t.IsSymbol('}'));
				}
				else
				{
					const string& texId = t.MustGetString();
					Texture* tex = resMgr->TryGet<Texture>(texId);
					if(tex)
						texOverride.push_back(TexOverride(tex));
					else
						LoadError("Missing texture override '%s'.", texId.c_str());
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
		case P_RECIPES:
			{
				Book& book = item->ToBook();
				if(t.IsSymbol('{'))
				{
					t.Next();
					while(!t.IsSymbol('}'))
					{
						book.recipes.push_back(Recipe::ForwardGet(t.MustGetItem()));
						t.Next();
					}
				}
				else
					book.recipes.push_back(Recipe::ForwardGet(t.MustGetItem()));
			}
			break;
		case P_EFFECTS:
			t.AssertSymbol('{');
			t.Next();
			while(!t.IsSymbol('}'))
			{
				bool onAttack = false;
				if(t.IsItem("on_attack"))
				{
					onAttack = true;
					t.Next();
				}
				EffectId effect = (EffectId)t.MustGetKeywordId(G_EFFECT);
				t.Next();
				int effectValue;
				EffectInfo& info = EffectInfo::effects[(int)effect];
				if(info.valueType != EffectInfo::None)
				{
					if(info.valueType == EffectInfo::Attribute)
					{
						const string& value = t.MustGetItemKeyword();
						Attribute* attrib = Attribute::Find(value);
						if(!attrib)
							t.Throw("Invalid attribute '%s' for effect '%s'.", value.c_str(), info.id);
						effectValue = (int)attrib->attribId;
					}
					else
						effectValue = t.MustGetKeywordId(G_SKILL);
					t.Next();
				}
				else
					effectValue = -1;
				float power = t.MustGetFloat();
				t.Next();
				item->effects.push_back({ effect, power, effectValue, onAttack });
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
		case P_ATTACK_MOD:
			item->ToShield().attackMod = t.MustGetFloat();
			break;
		default:
			assert(0);
			break;
		}

		t.Next();
	}

	if(!item->mesh && !item->tex)
		LoadError("No mesh/texture.");

	Item* itemPtr = item.Pin();
	Item::items.insert(ItemsMap::value_type(itemPtr->id.c_str(), itemPtr));

	switch(itemPtr->type)
	{
	case IT_WEAPON:
		Weapon::weapons.push_back(static_cast<Weapon*>(itemPtr));
		break;
	case IT_BOW:
		Bow::bows.push_back(static_cast<Bow*>(itemPtr));
		break;
	case IT_SHIELD:
		Shield::shields.push_back(static_cast<Shield*>(itemPtr));
		break;
	case IT_ARMOR:
		Armor::armors.push_back(static_cast<Armor*>(itemPtr));
		break;
	case IT_AMULET:
		Amulet::amulets.push_back(static_cast<Amulet*>(itemPtr));
		break;
	case IT_RING:
		Ring::rings.push_back(static_cast<Ring*>(itemPtr));
		break;
	case IT_CONSUMABLE:
		{
			Consumable* consumable = static_cast<Consumable*>(itemPtr);
			if(consumable->subtype == Consumable::Subtype::Potion)
			{
				for(ItemEffect& e : consumable->effects)
				{
					if(e.effect == EffectId::Heal && e.power > 0.f)
					{
						consumable->aiType = Consumable::AiType::Healing;
						break;
					}
					else if(e.effect == EffectId::RestoreMana && e.power > 0.f)
					{
						consumable->aiType = Consumable::AiType::Mana;
						break;
					}
				}
			}
			Consumable::consumables.push_back(consumable);
		}
		break;
	case IT_OTHER:
		OtherItem::others.push_back(static_cast<OtherItem*>(itemPtr));
		break;
	case IT_BOOK:
		{
			Book& b = itemPtr->ToBook();
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
	if(ItemList::TryGet(id))
		t.Throw("Id must be unique.");

	Ptr<ItemList> lis;

	// id
	lis->id = id;
	lis->total = 0;
	lis->isLeveled = false;
	lis->isPriority = false;
	t.Next();

	// [leveled|priority]
	if(t.IsKeywordGroup(G_LIST_TYPE))
	{
		if(t.IsKeyword(LT_LEVELED, G_LIST_TYPE))
			lis->isLeveled = true;
		else
			lis->isPriority = true;
		t.Next();
	}

	// {
	t.AssertSymbol('{');
	t.Next();

	while(!t.IsSymbol('}'))
	{
		ItemList::Entry entry;

		if(lis->isPriority && t.IsInt())
		{
			entry.chance = t.GetInt();
			if(entry.chance < 0)
				t.Throw("Invalid item priority %d.", entry.chance);
			t.Next();
		}
		else
			entry.chance = 1;
		lis->total += entry.chance;

		const string& itemId = t.MustGetItemKeyword();
		entry.item = Item::TryGet(itemId);
		if(!entry.item)
			t.Throw("Missing item %s.", itemId.c_str());
		t.Next();

		if(lis->isLeveled)
		{
			entry.level = t.MustGetInt();
			if(entry.level < 0)
				t.Throw("Invalid item level %d.", entry.level);
			t.Next();
		}
		else
			entry.level = 0;

		lis->items.push_back(entry);
	}

	if(lis->isPriority && lis->total == (int)lis->items.size())
		lis->isPriority = false;

	ItemList::lists.push_back(lis.Pin());
}

//=================================================================================================
void ItemLoader::ParseStock(const string& id)
{
	if(Stock::TryGet(id))
		t.Throw("Id must be unique.");

	Ptr<Stock> stock;
	bool inSet = false, inCity = false, inCityElse;

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
			if(inCity)
			{
				t.Next();
				if(!inCityElse && t.IsKeyword(SK_ELSE, G_STOCK_KEYWORD))
				{
					inCityElse = true;
					stock->code.push_back(SE_NOT_CITY);
					t.Next();
					t.AssertSymbol('{');
					t.Next();
				}
				else
				{
					inCity = false;
					stock->code.push_back(SE_ANY_CITY);
				}
			}
			else if(inSet)
			{
				inSet = false;
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
				if(inSet)
					t.Throw("Can't have nested sets.");
				if(inCity)
					t.Throw("Can't have set block inside city block.");
				inSet = true;
				stock->code.push_back(SE_START_SET);
				t.AssertSymbol('{');
				t.Next();
				break;
			case SK_CITY:
				if(inCity)
					t.Throw("Already in city block.");
				if(t.IsSymbol('{'))
				{
					t.Next();
					inCity = true;
					inCityElse = false;
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
					uint chancePos = stock->code.size();
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
					stock->code[chancePos] = count;
					stock->code[chancePos + 1] = chance;
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
					stock->script = scriptMgr->PrepareScript(Format("stock_%s", stock->id.c_str()), block.c_str());
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
	ItemList* lis;
	const string& itemId = t.MustGetItemKeyword();
	const Item* item = FindItemOrList(itemId, lis);
	if(lis)
	{
		stock->code.push_back(SE_LIST);
		stock->code.push_back((int)lis);
	}
	else if(item)
	{
		stock->code.push_back(SE_ITEM);
		stock->code.push_back((int)item);
	}
	else
		t.Throw("Missing item or list '%s'.", itemId.c_str());
	t.Next();
}

//=================================================================================================
void ItemLoader::ParseBookScheme(const string& id)
{
	if(BookScheme::TryGet(id))
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
				scheme->tex = resMgr->TryGet<Texture>(str);
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

	BookScheme::bookSchemes.push_back(scheme.Pin());
}

//=================================================================================================
void ItemLoader::ParseStartItems()
{
	if(!StartItem::startItems.empty())
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
			bool mage;
			if(t.IsKeyword(ITEM_MAGE, G_FLAGS))
			{
				mage = true;
				t.Next();
			}
			else
				mage = false;

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

			StartItem::startItems.push_back(StartItem(skill, item, num, mage));
		}

		t.Next();
	}

	std::sort(StartItem::startItems.begin(), StartItem::startItems.end(),
		[](const StartItem& si1, const StartItem& si2) { return si1.skill > si2.skill; });
}

//=================================================================================================
void ItemLoader::ParseBetterItems()
{
	if(!betterItems.empty())
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

		betterItems[item] = item2;
		t.Next();
	}
}

//=================================================================================================
void ItemLoader::ParseRecipe(const string& id)
{
	int hash = Hash(id);
	Recipe* existingRecipe = Recipe::TryGet(hash);
	if(existingRecipe)
	{
		if(existingRecipe->id != id)
			t.Throw("Id hash collision.");
		else if(existingRecipe->defined)
			t.Throw("Id must be unique.");
	}

	Ptr<Recipe> recipe(existingRecipe, false);
	recipe->id = id;
	recipe->hash = hash;
	recipe->defined = true;
	recipe->order = Recipe::items.size();

	t.Next();
	t.AssertSymbol('{');
	t.Next();

	while(!t.IsSymbol('}'))
	{
		RecipeProperty prop = (RecipeProperty)t.MustGetKeywordId(G_RECIPE);
		t.Next();
		switch(prop)
		{
		case RP_RESULT:
			{
				const string& itemId = t.MustGetItem();
				recipe->result = Item::TryGet(itemId);
				if(!recipe->result)
					LoadError("Missing result item '%s'.", itemId.c_str());
			}
			break;
		case RP_INGREDIENTS:
			t.AssertSymbol('{');
			t.Next();
			while(!t.IsSymbol('}'))
			{
				uint count = 1;
				if(t.IsInt())
				{
					count = t.GetUint();
					if(count == 0u)
						LoadError("Invalid ingredients count %u.", count);
					t.Next();
				}

				const string& itemId = t.MustGetItem();
				const Item* item = Item::TryGet(itemId);
				if(!recipe->result)
					LoadError("Missing item '%s'.", itemId.c_str());
				else
				{
					bool added = false;
					for(pair<const Item*, uint>& p : recipe->ingredients)
					{
						if(p.first == item)
						{
							p.second += count;
							added = true;
							break;
						}
					}
					if(!added)
						recipe->ingredients.push_back(std::make_pair(item, count));
				}

				t.Next();
			}
			break;
		case RP_SKILL:
			recipe->skill = t.MustGetUint();
			break;
		case RP_AUTOLEARN:
			recipe->autolearn = t.MustGetBool();
			break;
		case RP_ORDER:
			recipe->order = t.MustGetInt();
			break;
		}
		t.Next();
	}

	if(!recipe->result)
		LoadError("No result item.");
	else if(recipe->ingredients.empty())
		LoadError("No ingredients.");
	else if(!existingRecipe)
	{
		Recipe::hashes[hash] = recipe.Get();
		Recipe::items.push_back(recipe.Pin());
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

	itemAliases[alias] = item;
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
		crc.Update(item->aiValue);
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
			crc.Update(effect.onAttack);
		}

		switch(item->type)
		{
		case IT_WEAPON:
			{
				Weapon& w = item->ToWeapon();
				crc.Update(w.dmg);
				crc.Update(w.dmgType);
				crc.Update(w.reqStr);
				crc.Update(w.weaponType);
				crc.Update(w.material);
			}
			break;
		case IT_BOW:
			{
				Bow& b = item->ToBow();
				crc.Update(b.dmg);
				crc.Update(b.reqStr);
				crc.Update(b.speed);
			}
			break;
		case IT_SHIELD:
			{
				Shield& s = item->ToShield();
				crc.Update(s.attackMod);
				crc.Update(s.block);
				crc.Update(s.reqStr);
				crc.Update(s.material);
			}
			break;
		case IT_ARMOR:
			{
				Armor& a = item->ToArmor();
				crc.Update(a.def);
				crc.Update(a.reqStr);
				crc.Update(a.mobility);
				crc.Update(a.material);
				crc.Update(a.armorType);
				crc.Update(a.armorUnitType);
				crc.Update(a.texOverride.size());
				for(TexOverride& texOverride : a.texOverride)
					crc.Update(texOverride.diffuse ? texOverride.diffuse->filename : "");
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
				crc.Update(c.subtype);
			}
			break;
		case IT_OTHER:
			{
				OtherItem& o = item->ToOther();
				crc.Update(o.subtype);
			}
			break;
		case IT_BOOK:
			{
				Book& b = item->ToBook();
				crc.Update(b.scheme->id);
				crc.Update(b.subtype);
				for(Recipe* recipe : b.recipes)
					crc.Update(recipe->hash);
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
		for(const ItemList::Entry& e : lis->items)
		{
			crc.Update(e.item->id);
			crc.Update(e.chance);
			crc.Update(e.level);
		}
	}

	for(BookScheme* scheme : BookScheme::bookSchemes)
	{
		crc.Update(scheme->id);
		crc.Update(scheme->tex->filename);
		crc.Update(scheme->size);
		crc.Update(scheme->prev);
		crc.Update(scheme->next);
		crc.Update(scheme->regions);
	}

	for(Recipe* recipe : Recipe::items)
	{
		if(!recipe->defined)
			continue;
		crc.Update(recipe->id);
		crc.Update(recipe->result->id);
		for(pair<const Item*, uint>& p : recipe->ingredients)
		{
			crc.Update(p.first->id);
			crc.Update(p.second);
		}
		crc.Update(recipe->skill);
	}

	content.crc[(int)Content::Id::Items] = crc.Get();
}
