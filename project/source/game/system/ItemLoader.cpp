#include "Pch.h"
#include "Core.h"
#include "Item.h"
#include "ContentLoader.h"

class ItemLoader
{
	enum Group
	{
		G_ITEM_TYPE,
		G_PROPERTY,
		G_WEAPON_TYPE,
		G_MATERIAL,
		G_DMG_TYPE,
		G_FLAGS,
		G_ARMOR_SKILL,
		G_ARMOR_UNIT_TYPE,
		G_CONSUMABLE_TYPE,
		G_EFFECT,
		G_OTHER_TYPE,
		G_STOCK_KEYWORD,
		G_BOOK_SCHEME_PROPERTY,
		G_SKILL
	};

	enum Property
	{
		P_WEIGHT,
		P_VALUE,
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
		P_SKILL,
		P_TEX_OVERRIDE,
		P_EFFECT,
		P_POWER,
		P_TIME,
		P_SPEED,
		P_SCHEME,
		P_RUNIC
		// max 32 bits
	};

	enum StockKeyword
	{
		SK_SET,
		SK_CITY,
		SK_ELSE,
		SK_CHANCE,
		SK_RANDOM,
		SK_SAME
	};

	enum BookSchemeProperty
	{
		BSP_TEXTURE,
		BSP_SIZE,
		BSP_REGIONS,
		BSP_PREV,
		BSP_NEXT
	};

public:
	void Load()
	{
		InitTokenizer();

		bool require_id[IT_MAX];
		std::fill_n(require_id, IT_MAX, true);
		require_id[IT_START_ITEMS] = false;
		require_id[IT_BETTER_ITEMS] = false;

		ContentLoader loader;
		bool ok = loader.Load(t, "items.txt", G_ITEM_TYPE, [&, this](int top, const string& id)
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
			}
		}, require_id);
		if(!ok)
			return;

		CalculateCrc();

		//Info("Loaded objects (%u), usables (%u), - crc %p.",
		//	BaseObject::objs.size() - BaseUsable::usables.size(), BaseUsable::usables.size(), content::objects_crc);
	}

private:
	void InitTokenizer()
	{
		t.AddKeywords(G_ITEM_TYPE, {
			{ "weapon", IT_WEAPON },
			{ "bow", IT_BOW },
			{ "shield", IT_SHIELD },
			{ "armor", IT_ARMOR },
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
			{ "skill", P_SKILL },
			{ "tex_override", P_TEX_OVERRIDE },
			{ "effect", P_EFFECT },
			{ "power", P_POWER },
			{ "time", P_TIME },
			{ "speed", P_SPEED },
			{ "scheme", P_SCHEME },
			{ "runic", P_RUNIC }
		});

		t.AddKeywords(G_WEAPON_TYPE, {
			{ "short_blade", WT_SHORT },
			{ "long_blade", WT_LONG },
			{ "axe", WT_AXE },
			{ "blunt", WT_MACE }
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
			{ "secret", ITEM_SECRET },
			{ "backstab", ITEM_BACKSTAB },
			{ "power1", ITEM_POWER_1 },
			{ "power2", ITEM_POWER_2 },
			{ "power3", ITEM_POWER_3 },
			{ "power4", ITEM_POWER_4 },
			{ "magic_resistance10", ITEM_MAGIC_RESISTANCE_10 },
			{ "magic_resistance25", ITEM_MAGIC_RESISTANCE_25 },
			{ "ground_mesh", ITEM_GROUND_MESH },
			{ "crystal_sound", ITEM_CRYSTAL_SOUND },
			{ "important", ITEM_IMPORTANT },
			{ "not_merchant", ITEM_NOT_MERCHANT },
			{ "not_random", ITEM_NOT_RANDOM },
			{ "hq", ITEM_HQ },
			{ "magical", ITEM_MAGICAL },
			{ "unique", ITEM_UNIQUE },
			{ "alpha", ITEM_ALPHA }
		});

		t.AddKeywords(G_ARMOR_SKILL, {
			{ "light_armor", (int)Skill::LIGHT_ARMOR },
			{ "medium_armor", (int)Skill::MEDIUM_ARMOR },
			{ "heavy_armor", (int)Skill::HEAVY_ARMOR }
		});

		t.AddKeywords(G_ARMOR_UNIT_TYPE, {
			{ "human", (int)ArmorUnitType::HUMAN },
			{ "goblin", (int)ArmorUnitType::GOBLIN },
			{ "orc", (int)ArmorUnitType::ORC }
		});

		t.AddKeywords(G_CONSUMABLE_TYPE, {
			{ "potion", (int)ConsumableType::Potion },
			{ "drink", (int)ConsumableType::Drink },
			{ "food", (int)ConsumableType::Food }
		});

		t.AddKeywords(G_EFFECT, {
			{ "heal", E_HEAL },
			{ "regenerate", E_REGENERATE },
			{ "natural", E_NATURAL },
			{ "antidote", E_ANTIDOTE },
			{ "poison", E_POISON },
			{ "alcohol", E_ALCOHOL },
			{ "str", E_STR },
			{ "end", E_END },
			{ "dex", E_DEX },
			{ "antimagic", E_ANTIMAGIC },
			{ "food", E_FOOD },
			{ "green_hair", E_GREEN_HAIR },
			{ "stamina", E_STAMINA },
			{ "stun", E_STUN }
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
			{ "same", SK_SAME }
		});

		t.AddKeywords(G_BOOK_SCHEME_PROPERTY, {
			{ "texture", BSP_TEXTURE },
			{ "size", BSP_SIZE },
			{ "regions", BSP_REGIONS },
			{ "prev", BSP_PREV },
			{ "next", BSP_NEXT }
		});

		for(SkillInfo& si : g_skills)
			t.AddKeyword(si.id, (int)si.skill_id, G_SKILL);
	}

	void ParseItem(ITEM_TYPE type, const string& id)
	{
		int req = BIT(P_WEIGHT) | BIT(P_VALUE) | BIT(P_MESH) | BIT(P_TEX) | BIT(P_FLAGS);
		Ptr<Item> item(nullptr);

		switch(type)
		{
		case IT_WEAPON:
			item = new Weapon;
			req |= BIT(P_ATTACK) | BIT(P_REQ_STR) | BIT(P_TYPE) | BIT(P_MATERIAL) | BIT(P_DMG_TYPE);
			break;
		case IT_BOW:
			item = new Bow;
			req |= BIT(P_ATTACK) | BIT(P_REQ_STR) | BIT(P_SPEED);
			break;
		case IT_SHIELD:
			item = new Shield;
			req |= BIT(P_DEFENSE) | BIT(P_REQ_STR) | BIT(P_MATERIAL);
			break;
		case IT_ARMOR:
			item = new Armor;
			req |= BIT(P_DEFENSE) | BIT(P_MOBILITY) | BIT(P_REQ_STR) | BIT(P_MATERIAL) | BIT(P_SKILL) | BIT(P_TYPE) | BIT(P_TEX_OVERRIDE);
			break;
		case IT_CONSUMABLE:
			item = new Consumable;
			req |= BIT(P_EFFECT) | BIT(P_TIME) | BIT(P_POWER) | BIT(P_TYPE);
			break;
		case IT_BOOK:
			item = new Book;
			req |= BIT(P_SCHEME) | BIT(P_RUNIC);
			break;
		case IT_GOLD:
			item = new Item(IT_GOLD);
			break;
		case IT_OTHER:
		default:
			item = new OtherItem;
			req |= BIT(P_TYPE);
			break;
		}

		auto existing_item = Item::TryGet(id);
		if(existing_item)
			t.Throw("Id must be unique.");

		// id
		item->id = id;
		t.Next();

		// parent item
		if(t.IsSymbol(':'))
		{
			t.Next();
			auto& parent_id = t.MustGetItem();
			auto parent = Item::TryGet(parent_id);
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
			if(!IS_SET(req, BIT(prop)))
				t.Throw("Can't have property '%s'.", t.GetTokenString().c_str());

			t.Next();

			switch(prop)
			{
			case P_WEIGHT:
				item->weight = int(t.MustGetNumberFloat() * 10);
				if(item->weight < 0)
					t.Throw("Can't have negative weight %g.", item->GetWeight());
				break;
			case P_VALUE:
				item->value = t.MustGetInt();
				if(item->value < 0)
					t.Throw("Can't have negative value %d.", item->value);
				break;
			case P_MESH:
				if(IS_SET(item->flags, ITEM_TEX_ONLY))
					t.Throw("Can't have mesh, it is texture only item.");
				item->mesh_id = t.MustGetString();
				if(item->mesh_id.empty())
					t.Throw("Empty mesh.");
				break;
			case P_TEX:
				if(!item->mesh_id.empty() && !IS_SET(item->flags, ITEM_TEX_ONLY))
					t.Throw("Can't be texture only item, it have mesh.");
				item->mesh_id = t.MustGetString();
				if(item->mesh_id.empty())
					t.Throw("Empty texture.");
				item->flags |= ITEM_TEX_ONLY;
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
					item->ToArmor().armor_type = (ArmorUnitType)t.MustGetKeywordId(G_ARMOR_UNIT_TYPE);
					break;
				case IT_CONSUMABLE:
					item->ToConsumable().cons_type = (ConsumableType)t.MustGetKeywordId(G_CONSUMABLE_TYPE);
					break;
				case IT_OTHER:
					item->ToOther().other_type = (OtherType)t.MustGetKeywordId(G_OTHER_TYPE);
					break;
				}
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
				item->ToWeapon().dmg_type = ReadFlags(t, G_DMG_TYPE);
				break;
			case P_FLAGS:
				{
					int old_flags = (item->flags & ITEM_TEX_ONLY);
					item->flags |= ReadFlags(t, G_FLAGS) | old_flags;
				}
				break;
			case P_DEFENSE:
				{
					int def = t.MustGetInt();
					if(def < 0)
						t.Throw("Can't have negative defense %d.", def);
					if(item->type == IT_SHIELD)
						item->ToShield().def = def;
					else
						item->ToArmor().def = def;
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
			case P_SKILL:
				item->ToArmor().skill = (Skill)t.MustGetKeywordId(G_ARMOR_SKILL);
				break;
			case P_TEX_OVERRIDE:
				{
					vector<TexId>& tex_o = item->ToArmor().tex_override;
					if(t.IsSymbol('{'))
					{
						t.Next();
						do
						{
							tex_o.push_back(TexId(t.MustGetString().c_str()));
							t.Next();
						} while(!t.IsSymbol('}'));
					}
					else
						tex_o.push_back(TexId(t.MustGetString().c_str()));
				}
				break;
			case P_EFFECT:
				item->ToConsumable().effect = (ConsumeEffect)t.MustGetKeywordId(G_EFFECT);
				break;
			case P_POWER:
				{
					float power = t.MustGetNumberFloat();
					if(power < 0.f)
						t.Throw("Can't have negative power %g.", power);
					item->ToConsumable().power = power;
				}
				break;
			case P_TIME:
				{
					float time = t.MustGetNumberFloat();
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
					auto scheme = BookScheme::TryGet(str);
					if(!scheme)
						t.Throw("Book scheme '%s' not found.", str.c_str());
					item->ToBook().scheme = scheme;
				}
				break;
			case P_RUNIC:
				item->ToBook().runic = t.MustGetBool();
				break;
			default:
				assert(0);
				break;
			}

			t.Next();
		}

		if(item->mesh_id.empty())
			t.Throw("No mesh/texture.");

		auto it = item.Pin();
		g_items.insert(it);

		switch(it->type)
		{
		case IT_WEAPON:
			Weapon::weapons.push_back((Weapon*)it);
			break;
		case IT_BOW:
			Bow::bows.push_back((Bow*)it);
			break;
		case IT_SHIELD:
			Shield::shields.push_back((Shield*)it);
			break;
		case IT_ARMOR:
			Armor::armors.push_back((Armor*)it);
			break;
		case IT_CONSUMABLE:
			Consumable::consumables.push_back((Consumable*)it);
			break;
		case IT_OTHER:
			{
				OtherItem& o = it->ToOther();
				OtherItem::others.push_back(&o);
				if(o.other_type == Artifact)
					OtherItem::artifacts.push_back(&o);
			}
			break;
		case IT_BOOK:
			{
				Book& b = item->ToBook();
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

	void ParseItemList(const string& id)
	{
		/*
		ItemList* lis = new ItemList;
		ItemListResult used_list;
		const Item* item;

		try
		{
			// id
			t.Next();
			lis->id = t.MustGetItemKeyword();
			t.Next();

			// {
			t.AssertSymbol('{');
			t.Next();

			while(!t.IsSymbol('}'))
			{
				item = FindItem(t.MustGetItemKeyword().c_str(), false, &used_list);
				if(used_list.lis != nullptr)
					t.Throw("Item list can't have item list '%s' inside.", used_list.GetId());
				if(!item)
					t.Throw("Missing item %s.", t.GetTokenString().c_str());

				lis->items.push_back(item);
				t.Next();
			}

			for(ItemList* l : g_item_lists)
			{
				if(l->id == lis->id)
					t.Throw("Item list with that id already exists.");
			}

			g_item_lists.push_back(lis);

			crc.Update(lis->id);
			crc.Update(lis->items.size());
			for(const Item* i : lis->items)
				crc.Update(i->id);

			return true;
		}
		catch(const Tokenizer::Exception& e)
		{
			Error("Failed to parse item list '%s': %s", lis->id.c_str(), e.ToString());
			delete lis;
			return false;
		}*/
	}

	void ParseLeveledItemList(const string& id)
	{

	}

	void ParseStock(const string& id)
	{

	}

	void ParseBookScheme(const string& id)
	{

	}

	void ParseStartItems()
	{

	}

	void ParseBetterItems()
	{

	}

	void ParseAlias(const string& id)
	{

	}

	void CalculateCrc()
	{
		/*cstring key = item->id.c_str();

		ItemsMap::iterator it = g_items.lower_bound(key);
		if(it != g_items.end() && !(g_items.key_comp()(key, it->first)))
			t.Throw("Item already exists.");
		else
			g_items.insert(it, ItemsMap::value_type(key, item));

		crc.Update(item->id);
		crc.Update(item->mesh_id);
		crc.Update(item->weight);
		crc.Update(item->value);
		crc.Update(item->flags);
		crc.Update(item->type);

		switch(item->type)
		{
		case IT_WEAPON:
			{
				Weapon& w = item->ToWeapon();
				Weapon::weapons.push_back(&w);

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
				Bow::bows.push_back(&b);

				crc.Update(b.dmg);
				crc.Update(b.req_str);
				crc.Update(b.speed);
			}
			break;
		case IT_SHIELD:
			{
				Shield& s = item->ToShield();
				Shield::shields.push_back(&s);

				crc.Update(s.def);
				crc.Update(s.req_str);
				crc.Update(s.material);
			}
			break;
		case IT_ARMOR:
			{
				Armor& a = item->ToArmor();
				Armor::armors.push_back(&a);

				crc.Update(a.def);
				crc.Update(a.req_str);
				crc.Update(a.mobility);
				crc.Update(a.material);
				crc.Update(a.skill);
				crc.Update(a.armor_type);
				crc.Update(a.tex_override.size());
				for(TexId t : a.tex_override)
					crc.Update(t.id);
			}
			break;
		case IT_CONSUMABLE:
			{
				Consumable& c = item->ToConsumable();
				Consumable::consumables.push_back(&c);

				crc.Update(c.effect);
				crc.Update(c.power);
				crc.Update(c.time);
				crc.Update(c.cons_type);
			}
			break;
		case IT_OTHER:
			{
				OtherItem& o = item->ToOther();
				OtherItem::others.push_back(&o);
				if(o.other_type == Artifact)
					OtherItem::artifacts.push_back(&o);

				crc.Update(o.other_type);
			}
			break;
		case IT_BOOK:
			{
				Book& b = item->ToBook();
				if(!b.scheme)
					t.Throw("Missing book '%s' scheme.", b.id.c_str());
				g_books.push_back(&b);

				crc.Update(b.scheme->id);
			}
			break;
		case IT_GOLD:
			break;
		}*/
	}

	Tokenizer t;
};
