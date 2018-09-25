#include "Pch.h"
#include "GameCore.h"
#include "Inventory.h"
#include "Item.h"
#include "Unit.h"
#include "Game.h"
#include "Language.h"
#include "GetNumberDialog.h"
#include "GlobalGui.h"
#include "GameGui.h"
#include "AIController.h"
#include "Chest.h"
#include "Team.h"
#include "BookPanel.h"
#include "SoundManager.h"
#include "ResourceManager.h"
#include "ItemHelper.h"
#include "DirectX.h"

/* UWAGI CO DO ZMIENNYCH
index - indeks do items [0, 1, 2, 3...]
t_index - indeks to tmp_inv [0, 1, 2, 3...]
i_index - warto�� dla t_index (dla ujemnych slot) [-1, -2, -3, 0, 1, 2, 3...]
i_index == index dla nie za�o�onych przedmiot�w

last_index to tutaj t_index
*/

//-----------------------------------------------------------------------------
const int INDEX_GOLD = -2;
const int INDEX_CARRY = -3;


//=================================================================================================
Inventory::~Inventory()
{
	delete inv_mine;
	delete inv_trade_mine;
	delete inv_trade_other;
	delete gp_trade;
}

//=================================================================================================
void Inventory::InitOnce()
{
	inv_mine = new InventoryPanel(*this);
	inv_mine->InitTooltip();
	inv_mine->mode = InventoryPanel::INVENTORY;
	inv_mine->visible = false;

	gp_trade = new GamePanelContainer;

	inv_trade_mine = new InventoryPanel(*this);
	inv_trade_mine->focus = true;
	gp_trade->Add(inv_trade_mine);

	inv_trade_other = new InventoryPanel(*this);
	gp_trade->Add(inv_trade_other);

	gp_trade->Hide();
}

//=================================================================================================
void Inventory::LoadLanguage()
{
	auto section = Language::GetSection("Inventory");
	txGoldAndCredit = section.Get("goldAndCredit");
	txGoldDropInfo = section.Get("goldDropInfo");
	txCarryShort = section.Get("carryShort");
	txCarry = section.Get("carry");
	txCarryInfo = section.Get("carryInfo");
	txTeamItem = section.Get("teamItem");
	txCantWear = section.Get("cantWear");
	txCantDoNow = section.Get("cantDoNow");
	txBuyTeamDialog = section.Get("buyTeamDialog");
	txDropGoldCount = section.Get("dropGoldCount");
	txDropNoGold = section.Get("dropNoGold");
	txDropNotNow = section.Get("dropNotNow");
	txDropItemCount = section.Get("dropItemCount");
	txWontBuy = section.Get("wontBuy");
	txPrice = section.Get("price");
	txNeedMoreGoldItem = section.Get("needMoreGoldItem");
	txBuyItemCount = section.Get("buyItemCount");
	txSellItemCount = section.Get("sellItemCount");
	txLooting = section.Get("looting");
	txLootingChest = section.Get("lootingChest");
	txTrading = section.Get("trading");
	txPutGoldCount = section.Get("putGoldCount");
	txLootItemCount = section.Get("lootItemCount");
	txPutItemCount = section.Get("putItemCount");
	txTakeAll = section.Get("takeAll");
	txInventory = section.Get("inventory");
	txShareItems = section.Get("shareItems");
	txGiveItems = section.Get("giveItems");
	txPutGold = section.Get("putGold");
	txGiveGold = section.Get("giveGold");
	txGiveGoldCount = section.Get("giveGoldCount");
	txShareGiveItemCount = section.Get("shareGiveItemCount");
	txCanCarryTeamOnly = section.Get("canCarryTeamOnly");
	txWontGiveItem = section.Get("wontGiveItem");
	txShareTakeItemCount = section.Get("shareTakeItemCount");
	txWontTakeItem = section.Get("wontTakeItem");
	txSellTeamItem = section.Get("sellTeamItem");
	txSellItem = section.Get("sellItem");
	txSellFreeItem = section.Get("sellFreeItem");
	txGivePotionCount = section.Get("givePotionCount");
	txNpcCantCarry = section.Get("npcCantCarry");
	txStatsFor = section.Get("statsFor");
	txShowStatsFor = section.Get("showStatsFor");

	inv_mine->bt.text = txTakeAll;
	inv_mine->title = txInventory;
	inv_trade_mine->bt.text = txTakeAll;
	inv_trade_mine->title = txInventory;
	inv_trade_other->bt.text = txTakeAll;
	inv_trade_other->title = txInventory;
}

//=================================================================================================
void Inventory::LoadData()
{
	auto& tex_mgr = ResourceManager::Get<Texture>();
	tex_mgr.AddLoadTask("item_bar.png", tItemBar);
	tex_mgr.AddLoadTask("equipped.png", tEquipped);
	tex_mgr.AddLoadTask("coins.png", tGold);
	tex_mgr.AddLoadTask("star_hq.png", tStarHq);
	tex_mgr.AddLoadTask("star_m.png", tStarM);
	tex_mgr.AddLoadTask("star_u.png", tStarU);
	tex_mgr.AddLoadTask("team_item.png", tTeamItem);
}

//=================================================================================================
void Inventory::OnReset()
{
	if(tooltip.img == Game::Get().tItemRegionRot)
	{
		tooltip.img = nullptr;
		tex_replaced = true;
	}
}

//=================================================================================================
void Inventory::OnReload()
{
	if(tex_replaced)
	{
		if(!tooltip.img)
			tooltip.img = Game::Get().tItemRegionRot;
		tex_replaced = false;
	}
}

//=================================================================================================
void Inventory::Setup(PlayerController* pc)
{
	inv_trade_mine->i_items = inv_mine->i_items = &tmp_inventory[0];
	inv_trade_mine->items = inv_mine->items = &pc->unit->items;
	inv_trade_mine->slots = inv_mine->slots = pc->unit->slots;
	inv_trade_mine->unit = inv_mine->unit = pc->unit;
	inv_trade_other->i_items = &tmp_inventory[1];
}

//=================================================================================================
void Inventory::StartTrade(InventoryMode mode, Unit& unit)
{
	Game& game = Game::Get();
	game.gui->game_gui->ClosePanels();
	this->mode = mode;
	PlayerController* pc = game.pc;

	inv_trade_other->unit = &unit;
	inv_trade_other->items = &unit.items;
	inv_trade_other->slots = unit.slots;

	switch(mode)
	{
	case I_LOOT_BODY:
		inv_trade_mine->mode = InventoryPanel::LOOT_MY;
		inv_trade_other->mode = InventoryPanel::LOOT_OTHER;
		inv_trade_other->title = Format("%s - %s", txLooting, unit.GetName());
		break;
	case I_SHARE:
		inv_trade_mine->mode = InventoryPanel::SHARE_MY;
		inv_trade_other->mode = InventoryPanel::SHARE_OTHER;
		inv_trade_other->title = Format("%s - %s", txShareItems, unit.GetName());
		pc->action = PlayerController::Action_ShareItems;
		pc->action_unit = &unit;
		pc->chest_trade = &unit.items;
		break;
	case I_GIVE:
		inv_trade_mine->mode = InventoryPanel::GIVE_MY;
		inv_trade_other->mode = InventoryPanel::GIVE_OTHER;
		inv_trade_other->title = Format("%s - %s", txGiveItems, unit.GetName());
		pc->action = PlayerController::Action_GiveItems;
		pc->action_unit = &unit;
		pc->chest_trade = &unit.items;
		break;
	default:
		assert(0);
		break;
	}

	BuildTmpInventory(0);
	BuildTmpInventory(1);
	gp_trade->Show();
}

//=================================================================================================
void Inventory::StartTrade(InventoryMode mode, vector<ItemSlot>& items, Unit* unit)
{
	Game& game = Game::Get();
	game.gui->game_gui->ClosePanels();
	this->mode = mode;
	PlayerController* pc = game.pc;

	inv_trade_other->items = &items;
	inv_trade_other->slots = nullptr;

	switch(mode)
	{
	case I_LOOT_CHEST:
		inv_trade_mine->mode = InventoryPanel::LOOT_MY;
		inv_trade_other->mode = InventoryPanel::LOOT_OTHER;
		inv_trade_other->unit = nullptr;
		inv_trade_other->title = txLootingChest;
		break;
	case I_TRADE:
		assert(unit);
		inv_trade_mine->mode = InventoryPanel::TRADE_MY;
		inv_trade_other->mode = InventoryPanel::TRADE_OTHER;
		inv_trade_other->unit = unit;
		inv_trade_other->title = Format("%s - %s", txTrading, unit->GetName());
		pc->action = PlayerController::Action_Trade;
		pc->action_unit = unit;
		pc->chest_trade = &items;
		break;
	case I_LOOT_CONTAINER:
		inv_trade_mine->mode = InventoryPanel::LOOT_MY;
		inv_trade_other->mode = InventoryPanel::LOOT_OTHER;
		inv_trade_other->unit = nullptr;
		inv_trade_other->title = Format("%s - %s", txLooting, pc->action_container->base->name.c_str());
		break;
	default:
		assert(0);
		break;
	}

	BuildTmpInventory(0);
	BuildTmpInventory(1);
	gp_trade->Show();
}

//=================================================================================================
void Inventory::StartTrade2(InventoryMode mode, void* ptr)
{
	Game& game = Game::Get();
	PlayerController* pc = game.pc;
	game.gui->game_gui->ClosePanels();
	mode = I_LOOT_CHEST;

	switch(mode)
	{
	case I_LOOT_CHEST:
		{
			Chest* chest = (Chest*)ptr;
			pc->action = PlayerController::Action_LootChest;
			pc->action_chest = chest;
			pc->action_chest->user = pc->unit;
			pc->chest_trade = &pc->action_chest->items;
			inv_trade_mine->mode = InventoryPanel::LOOT_MY;
			inv_trade_other->unit = nullptr;
			inv_trade_other->items = &chest->items;
			inv_trade_other->slots = nullptr;
			inv_trade_other->title = txLootingChest;
			inv_trade_other->mode = InventoryPanel::LOOT_OTHER;
		}
		break;
	case I_LOOT_CONTAINER:
		{
			Usable* usable = (Usable*)ptr;
			pc->action = PlayerController::Action_LootContainer;
			pc->action_container = usable;
			pc->chest_trade = &pc->action_container->container->items;
			inv_trade_mine->mode = InventoryPanel::LOOT_MY;
			inv_trade_other->unit = nullptr;
			inv_trade_other->items = &pc->action_container->container->items;
			inv_trade_other->slots = nullptr;
			inv_trade_other->title = Format("%s - %s", txLooting, usable->base->name.c_str());
			inv_trade_other->mode = InventoryPanel::LOOT_OTHER;
		}
		break;
	}

	BuildTmpInventory(0);
	BuildTmpInventory(1);
	gp_trade->Show();
}

//=================================================================================================
// zbuduj tymczasowy ekwipunek kt�ry ��czy slots i items postaci
void Inventory::BuildTmpInventory(int index)
{
	assert(index == 0 || index == 1);

	PlayerController* pc = Game::Get().pc;
	vector<int>& ids = tmp_inventory[index];
	const Item** slots;
	vector<ItemSlot>* items;
	int& shift = tmp_inventory_shift[index];
	shift = 0;

	if(index == 0)
	{
		// przedmioty gracza
		slots = pc->unit->slots;
		items = &pc->unit->items;
	}
	else
	{
		// przedmioty innej postaci, w skrzyni
		if(pc->action == PlayerController::Action_LootChest
			|| pc->action == PlayerController::Action_Trade
			|| pc->action == PlayerController::Action_LootContainer)
			slots = nullptr;
		else
			slots = pc->action_unit->slots;
		items = pc->chest_trade;
	}

	ids.clear();

	// je�li to posta� to dodaj za�o�one przedmioty
	if(slots)
	{
		for(int i = 0; i < SLOT_MAX; ++i)
		{
			if(slots[i])
			{
				ids.push_back(-i - 1);
				++shift;
			}
		}
	}

	// nie za�o�one przedmioty
	for(int i = 0, ile = (int)items->size(); i < ile; ++i)
		ids.push_back(i);
}


//=================================================================================================
InventoryPanel::InventoryPanel(Inventory& base) : base(base), last_item(nullptr), i_items(nullptr), game(Game::Get()), for_unit(false)
{
	scrollbar.total = 100;
	scrollbar.offset = 0;
	scrollbar.part = 100;
	bt.id = GuiEvent_Custom;
	bt.parent = this;
}

//=================================================================================================
void InventoryPanel::InitTooltip()
{
	base.tooltip.Init(TooltipGetText(this, &InventoryPanel::GetTooltip));
}

//=================================================================================================
void InventoryPanel::Draw(ControlDrawData*)
{
	GamePanel::Draw();

	int ile_w = (size.x - 48) / 63;
	int ile_h = (size.y - 64 - 34) / 63;
	int shift_x = pos.x + 12 + (size.x - 48) % 63 / 2;
	int shift_y = pos.y + 48 + (size.y - 64 - 34) % 63 / 2;

	// scrollbar
	scrollbar.Draw();

	// miejsce na z�oto i ud�wig
	int bar_size = (ile_w * 63 - 8) / 2;
	int bar_y = shift_y + ile_h * 63 + 8;
	float load = 0.f;
	if(mode != TRADE_OTHER && mode != LOOT_OTHER)
	{
		load = unit->GetLoad();
		GUI.DrawItem(base.tItemBar, Int2(shift_x, bar_y), Int2(bar_size, 32), Color::White, 4);
		GUI.DrawItem(base.tEquipped, Int2(shift_x + bar_size + 10, bar_y), Int2(int(min(1.f, load)*bar_size), 32), Color::White, 4);
		GUI.DrawItem(base.tItemBar, Int2(shift_x + bar_size + 10, bar_y), Int2(bar_size, 32), Color::White, 4);
	}
	else if(mode == LOOT_OTHER)
		bt.Draw();

	// napis u g�ry
	Rect rect = {
		pos.x,
		pos.y + 10,
		pos.x + size.x,
		pos.y + size.y
	};
	GUI.DrawText(GUI.fBig, title, DTF_CENTER | DTF_SINGLELINE, Color::Black, rect, &rect);

	if(mode != TRADE_OTHER && mode != LOOT_OTHER)
	{
		// ikona z�ota
		GUI.DrawSprite(base.tGold, Int2(shift_x, bar_y));

		// z�oto
		rect = Rect::Create(Int2(shift_x, bar_y), Int2(bar_size, 32));
		GUI.DrawText(GUI.default_font, Format("%d", unit->gold), DTF_CENTER | DTF_VCENTER, Color::Black, rect);

		// ud�wig
		rect.Left() = shift_x + bar_size + 10;
		rect.Right() = rect.Left() + bar_size;
		cstring weight_str = Format(base.txCarryShort, float(unit->weight) / 10, float(unit->weight_max) / 10);
		int w = GUI.default_font->LineWidth(weight_str);
		GUI.DrawText(GUI.default_font, (w > bar_size ? Format("%g/%g", float(unit->weight) / 10, float(unit->weight_max) / 10) : weight_str),
			DTF_CENTER | DTF_VCENTER, (load > 1.f ? Color::Red : Color::Black), rect);
	}

	// rysuj kratki
	for(int y = 0; y < ile_h; ++y)
	{
		for(int x = 0; x < ile_w; ++x)
			GUI.DrawSprite(base.tItemBar, Int2(shift_x + x * 63, shift_y + y * 63));
	}

	// rysuj przedmioty
	bool have_team = Team.GetActiveTeamSize() > 1 && mode != TRADE_OTHER;
	int shift = int(scrollbar.offset / 63)*ile_w;
	for(int i = 0, ile = min(ile_w*ile_h, (int)i_items->size() - shift); i < ile; ++i)
	{
		int i_item = i_items->at(i + shift);
		const Item* item;
		int count, team;
		if(i_item < 0)
		{
			item = slots[-i_item - 1];
			count = 1;
			team = (mode == LOOT_OTHER ? 1 : 0);
		}
		else
		{
			ItemSlot& slot = items->at(i_item);
			item = slot.item;
			count = slot.count;
			if(slot.count == slot.team_count)
				team = 2;
			else if(slot.team_count == 0)
				team = 0;
			else
				team = 1;
		}

		int x = i%ile_w,
			y = i / ile_w;

		// obrazek za�o�onego przedmiotu
		if(i_item < 0)
			GUI.DrawSprite(base.tEquipped, Int2(shift_x + x * 63, shift_y + y * 63));

		// item icon
		GUI.DrawSprite(item->icon, Int2(shift_x + x * 63, shift_y + y * 63));

		// item quality icon
		TEX icon;
		if(IS_SET(item->flags, ITEM_HQ))
			icon = base.tStarHq;
		else if(IS_SET(item->flags, ITEM_MAGICAL))
			icon = base.tStarM;
		else if(IS_SET(item->flags, ITEM_UNIQUE))
			icon = base.tStarU;
		else
			icon = nullptr;
		if(icon)
			GUI.DrawSprite(icon, Int2(shift_x + x * 63, shift_y + (y + 1) * 63 - 24));

		// team item icon
		if(have_team && team != 0)
			GUI.DrawSprite(base.tTeamItem, Int2(shift_x + x * 63, shift_y + y * 63), team == 2 ? Color::Black : Color(0, 0, 0, 128));

		// count
		if(count > 1)
		{
			Rect rect3 = Rect::Create(Int2(shift_x + x * 63 + 2, shift_y + y * 63), Int2(64, 63));
			GUI.DrawText(GUI.default_font, Format("%d", count), DTF_BOTTOM, Color::Black, rect3);
		}
	}

	if(mode == INVENTORY)
		base.tooltip.Draw();
	else
		DrawBox();
}

//=================================================================================================
void InventoryPanel::Update(float dt)
{
	GamePanel::Update(dt);

	if(game.gui->book->visible)
		return;

	if(base.lock && base.lock.is_give)
	{
		base.lock.timer -= dt;
		if(base.lock.timer <= 0.f)
		{
			Error("IS_BETTER_ITEM timed out.");
			base.lock = nullptr;
		}
	}

	int ile_w = (size.x - 48) / 63;
	int ile_h = (size.y - 64 - 34) / 63;
	int shift_x = pos.x + 12 + (size.x - 48) % 63 / 2;
	int shift_y = pos.y + 48 + (size.y - 64 - 34) % 63 / 2;

	int new_index = INDEX_INVALID;

	Int2 cursor_pos = GUI.cursor_pos;

	bool have_focus = (mode == INVENTORY ? focus : mouse_focus);

	if(have_focus && Key.Focus() && IsInside(GUI.cursor_pos))
		scrollbar.ApplyMouseWheel();
	if(focus)
	{
		scrollbar.mouse_focus = have_focus;
		scrollbar.Update(dt);
	}

	int shift = int(scrollbar.offset / 63)*ile_w;

	if(last_index >= 0)
	{
		if(last_index >= (int)i_items->size())
		{
			last_index = INDEX_INVALID;
			last_item = nullptr;
		}
		else
		{
			int i_index = i_items->at(last_index);
			const Item* item;
			if(i_index < 0)
				item = slots[-i_index - 1];
			else
				item = items->at(i_index).item;
			if(item != last_item)
			{
				last_index = INDEX_INVALID;
				last_item = nullptr;
			}
		}
	}

	if(have_focus)
	{
		// przedmiot
		if(cursor_pos.x >= shift_x && cursor_pos.y >= shift_y)
		{
			int x = (cursor_pos.x - shift_x) / 63,
				y = (cursor_pos.y - shift_y) / 63;
			if(x >= 0 && x < ile_w && y >= 0 && y < ile_h)
			{
				int i = x + y*ile_w;
				if(i < (int)i_items->size() - shift)
					new_index = i + shift;
			}
		}

		int bar_size = (ile_w * 63 - 8) / 2;
		int bar_y = shift_y + ile_h * 63 + 8;

		// pasek ze z�otem lub ud�wigiem
		if(mode != TRADE_OTHER && mode != LOOT_OTHER)
		{
			if(cursor_pos.y >= bar_y && cursor_pos.y < bar_y + 32)
			{
				if(cursor_pos.x >= shift_x && cursor_pos.x < shift_x + bar_size)
					new_index = INDEX_GOLD;
				else if(cursor_pos.x >= shift_x + bar_size + 10 && cursor_pos.x < shift_x + bar_size + 10 + bar_size)
					new_index = INDEX_CARRY;
			}
		}

		// przycisk
		if(mode == LOOT_OTHER)
		{
			bt.mouse_focus = focus;
			bt.Update(dt);
		}
	}

	// klawisz to podnoszenia wszystkich przedmiot�w
	if(mode == LOOT_OTHER && (focus || base.inv_trade_mine->focus) && Key.Focus() && GKey.PressedRelease(GK_TAKE_ALL))
		Event(GuiEvent_Custom);

	// aktualizuj box
	if(mode == INVENTORY)
		base.tooltip.UpdateTooltip(dt, new_index, -1);
	else
	{
		bool old_for_unit = for_unit;
		if(AllowForUnit())
			for_unit = Key.Down(VK_SHIFT);
		else
			for_unit = false;
		UpdateBoxIndex(dt, new_index, -1, old_for_unit != for_unit);
		if(box_state == BOX_NOT_VISIBLE)
			item_visible = nullptr;
	}
	if(item_visible)
	{
		SURFACE surface = game.DrawItemImage(*item_visible, game.tItemRegionRot, game.sItemRegionRot, rot);
		surface->Release();
		rot += PI * dt;
	}

	if(new_index >= 0)
	{
		// pobierz przedmiot
		const Item* item;
		ItemSlot* slot;
		ITEM_SLOT slot_type;
		int i_index = i_items->at(new_index);
		if(i_index < 0)
		{
			item = slots[-i_index - 1];
			slot = nullptr;
			slot_type = (ITEM_SLOT)(-i_index - 1);
		}
		else
		{
			slot = &items->at(i_index);
			item = slot->item;
			slot_type = SLOT_INVALID;
		}

		last_item = item;

		if(!focus || !(game.pc->unit->action == A_NONE || game.pc->unit->CanDoWhileUsing()) || base.lock)
			return;

		// obs�uga kilkania w ekwipunku
		if(mode == INVENTORY && Key.Focus() && Key.PressedRelease(VK_RBUTTON) && game.pc->unit->action == A_NONE)
		{
			// wyrzu� przedmiot
			if(IS_SET(item->flags, ITEM_DONT_DROP) && game.IsAnyoneTalking())
				GUI.SimpleDialog(base.txCantDoNow, this);
			else
			{
				if(!slot)
				{
					if(SlotRequireHideWeapon(slot_type))
					{
						// drop item after hiding it
						unit->HideWeapon();
						unit->player->next_action = NA_DROP;
						unit->player->next_action_data.slot = slot_type;
						if(Net::IsClient())
							Net::PushChange(NetChange::SET_NEXT_ACTION);
					}
					else
						DropSlotItem(slot_type);
				}
				else
				{
					if(Key.Down(VK_SHIFT))
					{
						// wyrzu� wszystkie
						unit->DropItems(i_index, 0);
						last_index = INDEX_INVALID;
						if(mode == INVENTORY)
							base.tooltip.Clear();
						base.BuildTmpInventory(0);
						UpdateScrollbar();
					}
					else if(Key.Down(VK_CONTROL) || slot->count == 1)
					{
						// wyrzu� jeden
						if(unit->DropItem(i_index))
						{
							base.BuildTmpInventory(0);
							UpdateScrollbar();
							last_index = INDEX_INVALID;
							if(mode == INVENTORY)
								base.tooltip.Clear();
						}
						else
							FormatBox();
					}
					else
					{
						// wyrzu� okre�lon� liczb�
						counter = 1;
						base.lock.Lock(i_index, *slot);
						GetNumberDialog::Show(this, delegate<void(int)>(this, &InventoryPanel::OnDropItem), base.txDropItemCount, 0, slot->count, &counter);
						last_index = INDEX_INVALID;
						if(mode == INVENTORY)
							base.tooltip.Clear();
					}
				}
			}
		}
		else if(Key.Focus() && Key.PressedRelease(VK_LBUTTON))
		{
			switch(mode)
			{
			case INVENTORY:
				// use/equip/unequip item
				if(!slot)
				{
					// zdejmij przedmiot
					if(SlotRequireHideWeapon(slot_type))
					{
						// hide weapon & add next action to unequip
						unit->HideWeapon();
						unit->player->next_action = NA_REMOVE;
						unit->player->next_action_data.slot = slot_type;
						if(Net::IsClient())
							Net::PushChange(NetChange::SET_NEXT_ACTION);
					}
					else
						RemoveSlotItem(slot_type);
				}
				else
				{
					if(item->IsWearableByHuman() && slot->team_count > 0 && Team.GetActiveTeamSize() > 1)
					{
						DialogInfo di;
						di.event = delegate<void(int)>(this, &InventoryPanel::OnTakeItem);
						di.name = "takeover_item";
						di.parent = this;
						di.pause = false;
						di.text = base.txBuyTeamDialog;
						di.order = ORDER_NORMAL;
						di.type = DIALOG_YESNO;
						GUI.ShowDialog(di);
						base.lock.Lock(i_index, *slot);
						last_index = INDEX_INVALID;
						if(mode == INVENTORY)
							base.tooltip.Clear();
					}
					else if(item->type == IT_CONSUMABLE)
						ConsumeItem(i_index);
					else if(item->type == IT_BOOK)
						ReadBook(item);
					else if(item->IsWearable())
					{
						ITEM_SLOT type = ItemTypeToSlot(item->type);
						if(SlotRequireHideWeapon(type))
						{
							// hide equipped item & equip new one
							unit->HideWeapon();
							unit->player->next_action = NA_EQUIP;
							unit->player->next_action_data.item = item;
							unit->player->next_action_data.index = i_index;
							if(Net::IsClient())
								Net::PushChange(NetChange::SET_NEXT_ACTION);
						}
						else
						{
							// sprawd� czy mo�na u�ywa�
							bool ok = true;
							if(type == SLOT_ARMOR)
							{
								if(item->ToArmor().armor_type != ArmorUnitType::HUMAN)
								{
									ok = false;
									GUI.SimpleDialog(base.txCantWear, this);
								}
							}

							if(ok)
								EquipSlotItem(type, i_index);
						}
					}
				}
				break;
			case TRADE_MY:
				// selling items
				if(item->value <= 1 || !game.CanBuySell(item))
					GUI.SimpleDialog(base.txWontBuy, this);
				else if(!slot)
				{
					// za�o�ony przedmiot
					if(SlotRequireHideWeapon(slot_type))
					{
						// hide equipped item and sell it
						unit->HideWeapon();
						unit->player->next_action = NA_SELL;
						unit->player->next_action_data.slot = slot_type;
					}
					else
						SellSlotItem(slot_type);
				}
				else
				{
					// nie za�o�ony przedmiot
					// ustal ile gracz chce sprzeda� przedmiot�w
					uint ile;
					if(item->IsStackable() && slot->count != 1)
					{
						if(Key.Down(VK_SHIFT))
							ile = slot->count;
						else if(Key.Down(VK_CONTROL))
							ile = 1;
						else
						{
							counter = 1;
							base.lock.Lock(i_index, *slot);
							GetNumberDialog::Show(this, delegate<void(int)>(this, &InventoryPanel::OnSellItem), base.txSellItemCount, 0, slot->count, &counter);
							last_index = INDEX_INVALID;
							if(mode == INVENTORY)
								base.tooltip.Clear();
							break;
						}
					}
					else
						ile = 1;

					SellItem(i_index, ile);
				}
				break;
			case TRADE_OTHER:
				// buying items
				{
					// ustal ile gracz chce kupi� przedmiot�w
					uint ile;
					if(item->IsStackable() && slot->count != 1)
					{
						if(Key.Down(VK_SHIFT))
							ile = slot->count;
						else if(Key.Down(VK_CONTROL))
							ile = 1;
						else
						{
							counter = 1;
							base.lock.Lock(i_index, *slot);
							GetNumberDialog::Show(this, delegate<void(int)>(this, &InventoryPanel::OnBuyItem), base.txBuyItemCount, 0, slot->count, &counter);
							last_index = INDEX_INVALID;
							if(mode == INVENTORY)
								base.tooltip.Clear();
							break;
						}
					}
					else
						ile = 1;

					BuyItem(i_index, ile);
				}
				break;
			case LOOT_MY:
				// put item into corpse/chest/container
				if(slot)
				{
					// nie za�o�ony przedmiot
					uint ile;
					if(item->IsStackable() && slot->count != 1)
					{
						if(Key.Down(VK_SHIFT))
							ile = slot->count;
						else if(Key.Down(VK_CONTROL))
							ile = 1;
						else
						{
							counter = 1;
							base.lock.Lock(i_index, *slot);
							GetNumberDialog::Show(this, delegate<void(int)>(this, &InventoryPanel::OnPutItem), base.txPutItemCount, 0, slot->count, &counter);
							last_index = INDEX_INVALID;
							if(mode == INVENTORY)
								base.tooltip.Clear();
							break;
						}
					}
					else
						ile = 1;

					PutItem(i_index, ile);
				}
				else
				{
					// za�o�ony przedmiot
					if(SlotRequireHideWeapon(slot_type))
					{
						// hide equipped item and put it in container
						unit->HideWeapon();
						unit->player->next_action = NA_PUT;
						unit->player->next_action_data.slot = slot_type;
					}
					else
						PutSlotItem(slot_type);
				}
				break;
			case LOOT_OTHER:
				// take item from corpse/chest/container
				if(slot)
				{
					// nie za�o�ony przedmiot
					uint ile;
					if(item->IsStackable() && slot->count != 1)
					{
						int option;
						if(item->type == IT_GOLD)
						{
							if(Key.Down(VK_CONTROL))
								option = 1;
							else if(Key.Down(VK_MENU))
								option = 0;
							else
								option = 2;
						}
						else
						{
							if(Key.Down(VK_SHIFT))
								option = 2;
							else if(Key.Down(VK_CONTROL))
								option = 1;
							else
								option = 0;
						}
						if(option == 2)
							ile = slot->count;
						else if(option == 1)
							ile = 1;
						else
						{
							counter = 1;
							base.lock.Lock(i_index, *slot);
							GetNumberDialog::Show(this, delegate<void(int)>(this, &InventoryPanel::OnLootItem), base.txLootItemCount, 0, slot->count, &counter);
							last_index = INDEX_INVALID;
							if(mode == INVENTORY)
								base.tooltip.Clear();
							break;
						}
					}
					else
						ile = 1;

					LootItem(i_index, ile);
				}
				else
				{
					// za�o�ony przedmiot
					last_index = INDEX_INVALID;
					if(mode == INVENTORY)
						base.tooltip.Clear();
					// dodaj
					game.pc->unit->AddItem(item);
					base.BuildTmpInventory(0);
					// usu�
					if(slot_type == SLOT_WEAPON && slots[SLOT_WEAPON] == unit->used_item)
					{
						unit->used_item = nullptr;
						if(Net::IsServer())
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::REMOVE_USED_ITEM;
							c.unit = unit;
						}
					}
					slots[slot_type] = nullptr;
					unit->weight -= item->weight;
					base.BuildTmpInventory(1);
					// d�wi�k
					game.sound_mgr->PlaySound2d(game.GetItemSound(item));
					// komunikat
					if(Net::IsOnline())
					{
						NetChange& c = Add1(Net::changes);
						if(Net::IsServer())
						{
							c.type = NetChange::CHANGE_EQUIPMENT;
							c.unit = unit;
							c.id = slot_type;
						}
						else
						{
							c.type = NetChange::GET_ITEM;
							c.id = i_index;
							c.ile = 1;
						}
					}
				}
				break;
			case SHARE_MY:
				// give item to companion to store
				if(slot && slot->team_count > 0)
				{
					// nie za�o�ony przedmiot
					uint ile;
					if(item->IsStackable() && slot->team_count != 1)
					{
						if(Key.Down(VK_CONTROL))
							ile = 1;
						else if(Key.Down(VK_SHIFT))
							ile = slot->team_count;
						else
						{
							counter = 1;
							base.lock.Lock(i_index, *slot);
							GetNumberDialog::Show(this, delegate<void(int)>(this, &InventoryPanel::OnShareGiveItem),
								base.txShareGiveItemCount, 0, slot->team_count, &counter);
							last_index = INDEX_INVALID;
							if(mode == INVENTORY)
								base.tooltip.Clear();
							break;
						}
					}
					else
						ile = 1;

					Unit* t = unit->player->action_unit;
					if(t->CanTake(item))
						ShareGiveItem(i_index, ile);
					else
						GUI.SimpleDialog(Format(base.txNpcCantCarry, t->GetName()), this);
				}
				else
				{
					last_index = INDEX_INVALID;
					if(mode == INVENTORY)
						base.tooltip.Clear();
					GUI.SimpleDialog(base.txCanCarryTeamOnly, this);
				}
				break;
			case SHARE_OTHER:
			case GIVE_OTHER:
				// take item from companion
				if(slot && slot->team_count > 0)
				{
					// nie za�o�ony przedmiot
					uint ile;
					if(item->IsStackable() && slot->team_count != 1)
					{
						if(Key.Down(VK_SHIFT))
							ile = 1;
						else if(Key.Down(VK_CONTROL))
							ile = slot->team_count;
						else
						{
							counter = 1;
							base.lock.Lock(i_index, *slot);
							GetNumberDialog::Show(this, delegate<void(int)>(this, &InventoryPanel::OnShareTakeItem), base.txShareTakeItemCount, 0, slot->team_count,
								&counter);
							last_index = INDEX_INVALID;
							if(mode == INVENTORY)
								base.tooltip.Clear();
							break;
						}
					}
					else
						ile = 1;

					ShareTakeItem(i_index, ile);
				}
				else
				{
					last_index = INDEX_INVALID;
					if(mode == INVENTORY)
						base.tooltip.Clear();
					GUI.SimpleDialog(base.txWontGiveItem, this);
				}
				break;
			case GIVE_MY:
				// give item to companion
				Unit* t = unit->player->action_unit;
				if(slot)
				{
					// nie za�o�ony przedmiot
					if(item->IsWearableByHuman())
					{
						last_index = INDEX_INVALID;
						if(mode == INVENTORY)
							base.tooltip.Clear();

						if(Net::IsLocal())
						{
							if(!t->IsBetterItem(item))
								GUI.SimpleDialog(base.txWontTakeItem, this);
							else if(t->CanTake(item))
							{
								DialogInfo info;
								if(slot->team_count > 0)
								{
									// daj dru�ynowy przedmiot
									info.text = Format(base.txSellTeamItem, item->value / 2);
									give_item_mode = 0;
								}
								else if(t->gold >= item->value / 2)
								{
									// odkup przedmiot
									info.text = Format(base.txSellItem, item->value / 2);
									give_item_mode = 1;
								}
								else
								{
									// zaproponuj inn� cen�
									info.text = Format(base.txSellFreeItem, item->value / 2);
									give_item_mode = 2;
								}
								base.lock.Lock(i_index, *slot);
								info.event = delegate<void(int)>(this, &InventoryPanel::OnGiveItem);
								info.order = ORDER_NORMAL;
								info.parent = this;
								info.type = DIALOG_YESNO;
								info.pause = false;
								GUI.ShowDialog(info);
							}
							else
								GUI.SimpleDialog(Format(base.txNpcCantCarry, t->GetName()), this);
						}
						else
						{
							base.lock.Lock(i_index, *slot, true);
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::IS_BETTER_ITEM;
							c.id = i_index;
						}
					}
					else if(item->type == IT_CONSUMABLE && item->ToConsumable().IsHealingPotion())
					{
						uint ile;
						if(slot->count != 1)
						{
							if(Key.Down(VK_CONTROL))
								ile = 1;
							else if(Key.Down(VK_SHIFT))
								ile = slot->count;
							else
							{
								counter = 1;
								base.lock.Lock(i_index, *slot);
								GetNumberDialog::Show(this, delegate<void(int)>(this, &InventoryPanel::OnGivePotion), base.txGivePotionCount, 0, slot->count, &counter);
								last_index = INDEX_INVALID;
								if(mode == INVENTORY)
									base.tooltip.Clear();
								break;
							}
						}
						else
							ile = 1;

						if(t->CanTake(item, ile))
							GivePotion(i_index, ile);
						else
							GUI.SimpleDialog(Format(base.txNpcCantCarry, t->GetName()), this);
					}
					else
					{
						last_index = INDEX_INVALID;
						if(mode == INVENTORY)
							base.tooltip.Clear();
						GUI.SimpleDialog(base.txWontTakeItem, this);
					}
				}
				else
				{
					if(Net::IsLocal())
					{
						if(t->IsBetterItem(item))
						{
							// za�o�ony przedmiot
							if(t->CanTake(item))
							{
								if(SlotRequireHideWeapon(slot_type))
								{
									// hide equipped item and give it
									unit->HideWeapon();
									unit->player->next_action = NA_GIVE;
									unit->player->next_action_data.slot = slot_type;
								}
								else
									GiveSlotItem(slot_type);
							}
							else
								GUI.SimpleDialog(Format(base.txNpcCantCarry, t->GetName()), this);
						}
						else
						{
							last_index = INDEX_INVALID;
							if(mode == INVENTORY)
								base.tooltip.Clear();
							GUI.SimpleDialog(base.txWontTakeItem, this);
						}
					}
					else
					{
						base.lock.Lock(slot_type, true);
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::IS_BETTER_ITEM;
						c.id = i_index;
					}
				}
				break;
			}
		}
	}
	else if(new_index == INDEX_GOLD && mode != GIVE_OTHER && mode != SHARE_OTHER && !base.lock)
	{
		// wyrzucanie/chowanie/dawanie z�ota
		if(Key.PressedRelease(VK_LBUTTON))
		{
			last_index = INDEX_INVALID;
			if(mode == INVENTORY)
				base.tooltip.Clear();
			counter = 0;
			if(mode == INVENTORY)
				GetNumberDialog::Show(this, delegate<void(int)>(this, &InventoryPanel::OnDropGold), base.txDropGoldCount, 0, unit->gold, &counter);
			else if(mode == LOOT_MY)
				GetNumberDialog::Show(this, delegate<void(int)>(this, &InventoryPanel::OnPutGold), base.txPutGoldCount, 0, unit->gold, &counter);
			else if(mode == GIVE_MY || mode == SHARE_MY)
				GetNumberDialog::Show(this, delegate<void(int)>(this, &InventoryPanel::OnGiveGold), base.txGiveGoldCount, 0, unit->gold, &counter);
		}
	}

	if(mode == INVENTORY && Key.Focus() && Key.PressedRelease(VK_ESCAPE))
		Hide();
}

//=================================================================================================
void InventoryPanel::Event(GuiEvent e)
{
	GamePanel::Event(e);

	if(e == GuiEvent_Moved)
	{
		scrollbar.global_pos = global_pos + scrollbar.pos;
		bt.global_pos = global_pos + bt.pos;
	}
	else if(e == GuiEvent_Resize || e == GuiEvent_GainFocus || e == GuiEvent_Show)
	{
		UpdateScrollbar();
		int ile_w = (size.x - 48) / 63;
		int ile_h = (size.y - 64 - 34) / 63;
		int shift_x = 12 + (size.x - 48) % 63 / 2;
		int shift_y = 48 + (size.y - 64 - 34) % 63 / 2;
		int bar_size = (ile_w * 63 - 8) / 2;
		int bar_y = shift_y + ile_h * 63 + 8;
		bt.pos = Int2(shift_x + bar_size + 10, bar_y);
		bt.size = Int2(bar_size, 36);
		bt.global_pos = global_pos + bt.pos;
		if(e == GuiEvent_Show)
		{
			base.tooltip.Clear();
			bt.text = Game::Get().GetShortcutText(GK_TAKE_ALL);
		}
	}
	else if(e == GuiEvent_LostFocus)
		scrollbar.LostFocus();
	else if(e == GuiEvent_Custom)
	{
		if(game.pc->unit->action != A_NONE)
			return;

		// przycisk - zabierz wszystko
		bool zloto = false;
		SOUND snd[3] = { 0 };
		vector<ItemSlot>& itms = game.pc->unit->items;
		bool changes = false;

		// sloty
		if(game.pc->action != PlayerController::Action_LootChest && game.pc->action != PlayerController::Action_LootContainer)
		{
			const Item** unit_slots = game.pc->action_unit->slots;
			for(int i = 0; i < SLOT_MAX; ++i)
			{
				if(unit_slots[i])
				{
					SOUND s = game.GetItemSound(unit_slots[i]);
					if(s == game.sCoins)
						zloto = true;
					else
					{
						for(int i = 0; i < 3; ++i)
						{
							if(snd[i] == s)
								break;
							else if(!snd[i])
							{
								snd[i] = s;
								break;
							}
						}
					}

					InsertItemBare(itms, unit_slots[i]);
					game.pc->unit->weight += unit_slots[i]->weight;
					unit_slots[i] = nullptr;

					if(Net::IsServer())
					{
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::CHANGE_EQUIPMENT;
						c.unit = game.pc->action_unit;
						c.id = i;
					}

					changes = true;
				}
			}

			// wyzeruj wag� ekwipunku
			game.pc->action_unit->weight = 0;
		}

		// zwyk�e przedmioty
		for(vector<ItemSlot>::iterator it = game.pc->chest_trade->begin(), end = game.pc->chest_trade->end(); it != end; ++it)
		{
			if(!it->item)
				continue;

			if(it->item->type == IT_GOLD)
			{
				zloto = true;
				game.pc->unit->AddItem(Item::gold, it->count, it->team_count);
			}
			else
			{
				InsertItemBare(itms, it->item, it->count, it->team_count);
				game.pc->unit->weight += it->item->weight * it->count;
				SOUND s = game.GetItemSound(it->item);
				for(int i = 0; i < 3; ++i)
				{
					if(snd[i] == s)
						break;
					else if(!snd[i])
					{
						snd[i] = s;
						break;
					}
				}
				changes = true;
			}
		}
		game.pc->chest_trade->clear();

		if(!Net::IsLocal())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::GET_ALL_ITEMS;
		}

		// d�wi�k podnoszenia przedmiot�w
		for(int i = 0; i < 3; ++i)
		{
			if(snd[i])
				game.sound_mgr->PlaySound2d(snd[i]);
		}
		if(zloto)
			game.sound_mgr->PlaySound2d(game.sCoins);

		// zamknij ekwipunek
		if(changes)
			SortItems(itms);
		base.gp_trade->Hide();
		game.OnCloseInventory();
	}
}

//=================================================================================================
bool InventoryPanel::SlotRequireHideWeapon(ITEM_SLOT slot)
{
	switch(slot)
	{
	case SLOT_WEAPON:
	case SLOT_SHIELD:
		return (unit->weapon_state == WS_TAKEN && unit->weapon_taken == W_ONE_HANDED);
	case SLOT_BOW:
		return (unit->weapon_state == WS_TAKEN && unit->weapon_taken == W_BOW);
	default:
		return false;
	}
}

//=================================================================================================
// przek�ada przedmiot ze slotu do ekwipunku
void InventoryPanel::RemoveSlotItem(ITEM_SLOT slot)
{
	unit->AddItem(slots[slot], 1, false);
	unit->weight -= slots[slot]->weight;
	slots[slot] = nullptr;
	base.BuildTmpInventory(0);

	if(Net::IsOnline())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::CHANGE_EQUIPMENT;
		if(Net::IsServer())
		{
			c.unit = unit;
			c.id = slot;
		}
		else
			c.id = SlotToIIndex(slot);
	}
}

//=================================================================================================
void InventoryPanel::DropSlotItem(ITEM_SLOT slot)
{
	unit->DropItem(slot);
	base.BuildTmpInventory(0);
	UpdateScrollbar();
}

//=================================================================================================
void InventoryPanel::ConsumeItem(int index)
{
	int w = unit->ConsumeItem(index);
	if(w == 0)
	{
		last_index = INDEX_INVALID;
		if(mode == INVENTORY)
			base.tooltip.Clear();
		base.BuildTmpInventory(0);
		UpdateScrollbar();
	}
	else if(w == 1 && last_index - base.tmp_inventory_shift[0] == index)
		FormatBox();
}

//=================================================================================================
void InventoryPanel::EquipSlotItem(ITEM_SLOT slot, int i_index)
{
	if(slots[slot])
	{
		const Item* prev_item = slots[slot];
		// ustaw slot
		slots[slot] = items->at(i_index).item;
		items->erase(items->begin() + i_index);
		// dodaj stary przedmiot
		unit->AddItem(prev_item, 1, false);
		unit->weight -= prev_item->weight;
	}
	else
	{
		// ustaw slot
		slots[slot] = items->at(i_index).item;
		// usu� przedmiot
		items->erase(items->begin() + i_index);
	}

	base.BuildTmpInventory(0);
	last_index = INDEX_INVALID;
	if(mode == INVENTORY)
		base.tooltip.Clear();

	if(Net::IsOnline())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::CHANGE_EQUIPMENT;
		if(Net::IsServer())
		{
			c.unit = unit;
			c.id = slot;
		}
		else
			c.id = i_index;
	}
}

//=================================================================================================
void InventoryPanel::EquipSlotItem(int index)
{
	EquipSlotItem(ItemTypeToSlot(items->at(index).item->type), index);
}

//=================================================================================================
void InventoryPanel::FormatBox()
{
	FormatBox(last_index, box_text, box_text_small, box_img);
}

//=================================================================================================
void InventoryPanel::FormatBox(int group, string& text, string& small_text, TEX& img)
{
	if(group == INDEX_GOLD)
	{
		text = Format(base.txGoldAndCredit, unit->gold, unit->IsPlayer() ? unit->player->credit : unit->hero->credit);
		switch(mode)
		{
		case INVENTORY:
			small_text = base.txGoldDropInfo;
			break;
		case TRADE_MY:
		case TRADE_OTHER:
		case LOOT_OTHER:
		case GIVE_OTHER:
		case SHARE_OTHER:
			small_text.clear();
			break;
		case LOOT_MY:
			small_text = base.txPutGold;
			break;
		case SHARE_MY:
		case GIVE_MY:
			small_text = base.txGiveGold;
			break;
		}
		img = base.tGold;
		item_visible = nullptr;
	}
	else if(group == INDEX_CARRY)
	{
		text = Format(base.txCarry, float(unit->weight) / 10, int(unit->GetLoad() * 100), float(unit->weight_max) / 10);
		small_text = base.txCarryInfo;
		img = nullptr;
		item_visible = nullptr;
	}
	else if(group != INDEX_INVALID)
	{
		const Item* item;
		int count, team_count;
		int i_index = i_items->at(group);
		if(i_index < 0)
		{
			item = slots[-i_index - 1];
			count = 1;
			team_count = (mode == LOOT_OTHER ? 1 : 0);
		}
		else
		{
			ItemSlot& slot = items->at(i_index);
			item = slot.item;
			count = slot.count;
			team_count = slot.team_count;
		}

		Unit* target;
		if(for_unit)
			target = game.pc->action_unit;
		else
			target = game.pc->unit;

		GetItemString(text, item, target, (uint)count);
		if(mode != TRADE_OTHER && team_count && Team.GetActiveTeamSize() > 1)
		{
			text += '\n';
			text += base.txTeamItem;
			if(count != 1)
				text += Format(" (%d)", team_count);
		}
		if(mode == TRADE_MY)
		{
			text += '\n';
			int price = ItemHelper::GetItemPrice(item, *game.pc->unit, false);
			if(price == 0 || !game.CanBuySell(item))
				text += base.txWontBuy;
			else
				text += Format(base.txPrice, price);
		}
		else if(mode == TRADE_OTHER)
		{
			int price = ItemHelper::GetItemPrice(item, *game.pc->unit, true);
			text += '\n';
			text += Format(base.txPrice, price);
		}
		small_text = item->desc;
		if(AllowForUnit())
		{
			if(!small_text.empty())
				small_text += '\n';
			if(for_unit)
				small_text += Format(base.txStatsFor, game.pc->action_unit->GetName());
			else
				small_text += Format(base.txShowStatsFor, game.pc->action_unit->GetName());
		}

		if(item->mesh)
		{
			img = game.tItemRegionRot;
			rot = 0.f;
			item_visible = item;
		}
		else
		{
			img = item->icon;
			item_visible = nullptr;
		}
	}
}

//=================================================================================================
void InventoryPanel::GetTooltip(TooltipController*, int group, int)
{
	if(group == INDEX_INVALID)
	{
		item_visible = nullptr;
		base.tooltip.anything = false;
		return;
	}

	base.tooltip.anything = true;
	base.tooltip.big_text.clear();

	FormatBox(group, base.tooltip.text, base.tooltip.small_text, base.tooltip.img);
}

//=================================================================================================
void InventoryPanel::OnDropGold(int id)
{
	if(id == BUTTON_CANCEL || counter == 0)
		return;

	if(counter > unit->gold)
		GUI.SimpleDialog(base.txDropNoGold, this);
	else if(!unit->CanAct())
		GUI.SimpleDialog(base.txDropNotNow, this);
	else
		game.pc->unit->DropGold(counter);
}

//=================================================================================================
void InventoryPanel::OnDropItem(int id)
{
	int index = GetLockIndexAndRelease();
	if(id == BUTTON_CANCEL || counter == 0 || index == -1)
		return;

	if(!unit->CanAct())
		GUI.SimpleDialog(base.txDropNotNow, this);
	else
	{
		if(unit->DropItems(index, counter))
		{
			base.BuildTmpInventory(0);
			UpdateScrollbar();
		}
	}
}

//=================================================================================================
void InventoryPanel::OnTakeItem(int id)
{
	int index = GetLockIndexAndRelease();
	if(id == BUTTON_NO || index == -1)
		return;

	ItemSlot& slot = items->at(index);
	unit->player->credit += slot.item->value / 2;
	slot.team_count = 0;

	if(Net::IsLocal())
		Team.CheckCredit(true);
	else
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::TAKE_ITEM_CREDIT;
		c.id = index;
	}
}

//=================================================================================================
void InventoryPanel::UpdateScrollbar()
{
	if(!i_items)
		return;

	int ile_w = (size.x - 48) / 63;
	int ile_h = (size.y - 64 - 34) / 63;
	int shift_x = pos.x + 12 + (size.x - 48) % 63 / 2;
	int shift_y = pos.y + 48 + (size.y - 64 - 34) % 63 / 2;
	int ile = i_items->size();
	int s = ((ile + ile_w - 1) / ile_w) * 63;
	scrollbar.size = Int2(16, ile_h * 63);
	scrollbar.total = s;
	scrollbar.part = min(s, scrollbar.size.y);
	scrollbar.pos = Int2(shift_x + ile_w * 63 + 8 - pos.x, shift_y - pos.y);
	scrollbar.global_pos = global_pos + scrollbar.pos;
	if(scrollbar.offset + scrollbar.part > scrollbar.total)
		scrollbar.offset = float(scrollbar.total - scrollbar.part);
	if(scrollbar.offset < 0)
		scrollbar.offset = 0;
}

//=================================================================================================
void InventoryPanel::OnBuyItem(int id)
{
	int index = GetLockIndexAndRelease();
	if(id == BUTTON_OK && counter > 0 && index != -1)
		BuyItem(index, counter);
}

//=================================================================================================
void InventoryPanel::OnSellItem(int id)
{
	int index = GetLockIndexAndRelease();
	if(id == BUTTON_OK && counter > 0 && index != -1)
		SellItem(index, counter);
}

//=================================================================================================
void InventoryPanel::BuyItem(int index, uint count)
{
	ItemSlot& slot = items->at(index);
	int price = ItemHelper::GetItemPrice(slot.item, *game.pc->unit, true) * count;

	if(price > game.pc->unit->gold)
	{
		// gracz ma za ma�o z�ota
		GUI.SimpleDialog(Format(base.txNeedMoreGoldItem, price - game.pc->unit->gold, slot.item->name.c_str()), this);
	}
	else
	{
		// d�wi�k
		game.sound_mgr->PlaySound2d(game.GetItemSound(slot.item));
		game.sound_mgr->PlaySound2d(game.sCoins);
		// usu� z�oto
		game.pc->unit->gold -= price;
		// dodaj przedmiot graczowi
		if(!game.pc->unit->AddItem(slot.item, count, 0u))
			UpdateGrid(true);
		// usu� przedmiot sprzedawcy
		slot.count -= count;
		if(slot.count == 0)
		{
			last_index = INDEX_INVALID;
			if(mode == INVENTORY)
				base.tooltip.Clear();
			items->erase(items->begin() + index);
			UpdateGrid(false);
		}
		else
			FormatBox();
		// komunikat
		if(!Net::IsLocal())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::GET_ITEM;
			c.id = index;
			c.ile = count;
		}
	}
}

//=================================================================================================
void InventoryPanel::SellItem(int index, uint count)
{
	ItemSlot& slot = items->at(index);
	uint team_count = min(count, slot.team_count);
	uint normal_count = count - team_count;

	// d�wi�k
	game.sound_mgr->PlaySound2d(game.GetItemSound(slot.item));
	game.sound_mgr->PlaySound2d(game.sCoins);
	// dodaj z�oto
	if(Net::IsLocal())
	{
		int price = ItemHelper::GetItemPrice(slot.item, *game.pc->unit, false);
		if(team_count)
			game.AddGold(price * team_count);
		if(normal_count)
			unit->gold += price * normal_count;
	}
	// dodaj przedmiot kupcowi
	if(!InsertItem(*unit->player->chest_trade, slot.item, count, team_count))
		UpdateGrid(false);
	// usu� przedmiot graczowi
	unit->weight -= slot.item->weight*count;
	slot.count -= count;
	if(slot.count == 0)
	{
		last_index = INDEX_INVALID;
		if(mode == INVENTORY)
			base.tooltip.Clear();
		items->erase(items->begin() + index);
		UpdateGrid(true);
	}
	else
	{
		FormatBox();
		slot.team_count -= team_count;
	}
	// komunikat
	if(!Net::IsLocal())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::PUT_ITEM;
		c.id = index;
		c.ile = count;
	}
}

//=================================================================================================
void InventoryPanel::SellSlotItem(ITEM_SLOT slot)
{
	const Item* item = slots[slot];

	// d�wi�k
	game.sound_mgr->PlaySound2d(game.GetItemSound(item));
	game.sound_mgr->PlaySound2d(game.sCoins);
	// dodaj z�oto
	unit->gold += ItemHelper::GetItemPrice(item, *game.pc->unit, false);
	// dodaj przedmiot kupcowi
	InsertItem(*unit->player->chest_trade, item, 1, 0);
	UpdateGrid(false);
	// usu� przedmiot graczowi
	slots[slot] = nullptr;
	unit->weight -= item->weight;
	UpdateGrid(true);
	// komunikat
	if(Net::IsOnline())
	{
		NetChange& c = Add1(Net::changes);
		if(Net::IsServer())
		{
			c.type = NetChange::CHANGE_EQUIPMENT;
			c.unit = unit;
			c.id = slot;
		}
		else
		{
			c.type = NetChange::PUT_ITEM;
			c.id = SlotToIIndex(slot);
			c.ile = 1;
		}
	}
}

//=================================================================================================
void InventoryPanel::OnPutGold(int id)
{
	if(id == BUTTON_OK && counter > 0 && unit->gold >= counter)
	{
		if(counter > game.pc->unit->gold)
		{
			GUI.SimpleDialog(base.txDropNoGold, this);
			return;
		}
		// dodaj z�oto
		if(!InsertItem(*unit->player->chest_trade, Item::gold, counter, 0))
			UpdateGrid(false);
		// usu�
		unit->gold -= counter;
		// d�wi�k
		game.sound_mgr->PlaySound2d(game.sCoins);
		if(!Net::IsLocal())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::PUT_GOLD;
			c.ile = counter;
		}
	}
}

//=================================================================================================
void InventoryPanel::OnLootItem(int id)
{
	int index = GetLockIndexAndRelease();
	if(id == BUTTON_OK && counter > 0 && index != -1)
		LootItem(index, counter);
}

//=================================================================================================
void InventoryPanel::LootItem(int index, uint count)
{
	ItemSlot& slot = items->at(index);
	uint team_count = min(count, slot.team_count);
	// d�wi�k
	game.sound_mgr->PlaySound2d(game.GetItemSound(slot.item));
	// dodaj
	if(!game.pc->unit->AddItem(slot.item, count, team_count))
		UpdateGrid(true);
	// usu�
	if(base.mode == I_LOOT_BODY)
	{
		unit->weight -= slot.item->weight*count;
		if(slot.item == unit->used_item)
		{
			unit->used_item = nullptr;
			if(Net::IsServer())
			{
				NetChange& c = Add1(Net::changes);
				c.type = NetChange::REMOVE_USED_ITEM;
				c.unit = unit;
			}
		}
	}
	slot.count -= count;
	if(slot.count == 0)
	{
		last_index = INDEX_INVALID;
		if(mode == INVENTORY)
			base.tooltip.Clear();
		items->erase(items->begin() + index);
		UpdateGrid(false);
	}
	else
	{
		FormatBox();
		slot.team_count -= team_count;
	}
	// komunikat
	if(!Net::IsLocal())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::GET_ITEM;
		c.id = index;
		c.ile = count;
	}
}

//=================================================================================================
void InventoryPanel::OnPutItem(int id)
{
	int index = GetLockIndexAndRelease();
	if(id == BUTTON_OK && counter > 0 && index != -1)
		PutItem(index, counter);
}

//=================================================================================================
void InventoryPanel::PutItem(int index, uint count)
{
	ItemSlot& slot = items->at(index);
	uint team_count = min(count, slot.team_count);

	// play sound
	game.sound_mgr->PlaySound2d(game.GetItemSound(slot.item));

	// add to container
	if(base.mode == I_LOOT_BODY)
	{
		if(!unit->player->action_unit->AddItem(slot.item, count, team_count))
			UpdateGrid(false);
	}
	else if(base.mode == I_LOOT_CONTAINER)
	{
		if(!unit->player->action_container->container->AddItem(slot.item, count, team_count))
			UpdateGrid(false);
	}
	else
	{
		if(!unit->player->action_chest->AddItem(slot.item, count, team_count))
			UpdateGrid(false);
	}

	// remove from player
	unit->weight -= slot.item->weight*count;
	slot.count -= count;
	if(slot.count == 0)
	{
		last_index = INDEX_INVALID;
		if(mode == INVENTORY)
			base.tooltip.Clear();
		items->erase(items->begin() + index);
		UpdateGrid(true);
	}
	else
	{
		FormatBox();
		slot.team_count -= team_count;
	}

	// send change
	if(!Net::IsLocal())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::PUT_ITEM;
		c.id = index;
		c.ile = count;
	}
}

//=================================================================================================
void InventoryPanel::PutSlotItem(ITEM_SLOT slot)
{
	const Item* item = slots[slot];
	last_index = INDEX_INVALID;
	if(mode == INVENTORY)
		base.tooltip.Clear();

	// play sound
	game.sound_mgr->PlaySound2d(game.GetItemSound(item));

	// add to container
	if(base.mode == I_LOOT_BODY)
		unit->player->action_unit->AddItem(item, 1u, 0u);
	else if(base.mode == I_LOOT_CONTAINER)
		unit->player->action_container->container->AddItem(item, 1u, 0u);
	else
		unit->player->action_chest->AddItem(item, 1u, 0u);
	UpdateGrid(false);

	// remove from player
	slots[slot] = nullptr;
	UpdateGrid(true);
	unit->weight -= item->weight;

	// send change
	if(Net::IsOnline())
	{
		NetChange& c = Add1(Net::changes);
		if(Net::IsServer())
		{
			c.type = NetChange::CHANGE_EQUIPMENT;
			c.unit = unit;
			c.id = slot;
		}
		else
		{
			c.type = NetChange::PUT_ITEM;
			c.id = SlotToIIndex(slot);
			c.ile = 1;
		}
	}
}

//=================================================================================================
void InventoryPanel::OnGiveGold(int id)
{
	if(id == BUTTON_OK && counter > 0)
	{
		if(counter > game.pc->unit->gold)
		{
			GUI.SimpleDialog(base.txDropNoGold, this);
			return;
		}

		game.pc->unit->gold -= counter;
		game.sound_mgr->PlaySound2d(game.sCoins);
		Unit* u = game.pc->action_unit;
		if(Net::IsLocal())
		{
			u->gold += counter;
			if(u->IsPlayer() && u->player != game.pc)
			{
				NetChangePlayer& c = Add1(u->player->player_info->changes);
				c.type = NetChangePlayer::GOLD_RECEIVED;
				c.id = game.pc->id;
				c.ile = counter;
				u->player->player_info->UpdateGold();
			}
		}
		else
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::GIVE_GOLD;
			c.id = u->netid;
			c.ile = counter;
		}
	}
}

//=================================================================================================
void InventoryPanel::OnShareGiveItem(int id)
{
	int index = GetLockIndexAndRelease();
	if(id == BUTTON_OK && counter > 0 && index != -1)
	{
		Unit* t = unit->player->action_unit;
		if(t->CanTake(items->at(index).item, counter))
			ShareGiveItem(index, counter);
		else
			GUI.SimpleDialog(Format(base.txNpcCantCarry, t->GetName()), this);
	}
}

//=================================================================================================
void InventoryPanel::OnShareTakeItem(int id)
{
	int index = GetLockIndexAndRelease();
	if(id == BUTTON_OK && counter > 0 && index != -1)
		ShareTakeItem(index, counter);
}

//=================================================================================================
void InventoryPanel::ShareGiveItem(int index, uint count)
{
	ItemSlot& slot = items->at(index);
	const Item* item = slot.item;
	uint team_count = min(count, slot.team_count);
	// d�wi�k
	game.sound_mgr->PlaySound2d(game.GetItemSound(slot.item));
	// dodaj
	if(!unit->player->action_unit->AddItem(slot.item, count, team_count))
		UpdateGrid(false);
	// usu�
	unit->weight -= slot.item->weight*count;
	slot.count -= count;
	if(slot.count == 0)
	{
		last_index = INDEX_INVALID;
		if(mode == INVENTORY)
			base.tooltip.Clear();
		items->erase(items->begin() + index);
		UpdateGrid(true);
	}
	else
	{
		FormatBox();
		slot.team_count -= team_count;
	}
	// komunikat
	if(!Net::IsLocal())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::PUT_ITEM;
		c.id = index;
		c.ile = count;
	}
	else if(item->type == IT_CONSUMABLE && item->ToConsumable().effect == E_HEAL)
		unit->player->action_unit->ai->have_potion = 2;
}

//=================================================================================================
void InventoryPanel::ShareTakeItem(int index, uint count)
{
	ItemSlot& slot = items->at(index);
	const Item* item = slot.item;
	uint team_count = min(count, slot.team_count);
	// d�wi�k
	game.sound_mgr->PlaySound2d(game.GetItemSound(slot.item));
	// dodaj
	if(!game.pc->unit->AddItem(slot.item, count, team_count))
		UpdateGrid(true);
	// usu�
	unit->weight -= slot.item->weight*count;
	slot.count -= count;
	if(slot.count == 0)
	{
		last_index = INDEX_INVALID;
		if(mode == INVENTORY)
			base.tooltip.Clear();
		items->erase(items->begin() + index);
		UpdateGrid(false);
	}
	else
	{
		FormatBox();
		slot.team_count -= team_count;
	}
	// komunikat
	if(!Net::IsLocal())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::GET_ITEM;
		c.id = index;
		c.ile = count;
	}
	else if(item->type == IT_CONSUMABLE && item->ToConsumable().effect == E_HEAL)
		unit->ai->have_potion = 1;
}

//=================================================================================================
void InventoryPanel::OnGiveItem(int id)
{
	int iindex = GetLockIndexOrSlotAndRelease();
	if(id != BUTTON_YES || iindex == Unit::INVALID_IINDEX)
		return;

	last_index = INDEX_INVALID;
	if(mode == INVENTORY)
		base.tooltip.Clear();
	const Item* item;
	ItemSlot* slot;
	ITEM_SLOT slot_type;

	if(iindex >= 0)
	{
		slot = &items->at(iindex);
		slot_type = SLOT_INVALID;
		item = slot->item;
	}
	else
	{
		slot = nullptr;
		slot_type = IIndexToSlot(iindex);
		item = slots[slot_type];
	}

	// dodaj
	Unit* t = unit->player->action_unit;
	int price = ItemHelper::GetItemPrice(item, *game.pc->unit, false);
	switch(give_item_mode)
	{
	case 0: // kredyt
		t->hero->credit += price;
		if(Net::IsLocal())
			Team.CheckCredit(true);
		break;
	case 1: // z�oto
		if(t->gold < price)
			return;
		t->gold -= price;
		unit->gold += price;
		game.sound_mgr->PlaySound2d(game.sCoins);
		break;
	case 2: // darmo
		break;
	}
	t->AddItem(item, 1u, 0u);
	UpdateGrid(false);
	// d�wi�k
	game.sound_mgr->PlaySound2d(game.GetItemSound(item));
	// usu�
	unit->weight -= item->weight;
	if(slot)
		items->erase(items->begin() + iindex);
	else
	{
		slots[slot_type] = nullptr;
		if(Net::IsServer())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::CHANGE_EQUIPMENT;
			c.unit = unit;
			c.id = slot_type;
		}
	}
	UpdateGrid(true);
	// ustaw przedmioty
	if(Net::IsLocal())
	{
		t->UpdateInventory();
		base.BuildTmpInventory(1);
	}
	// komunikat
	if(!Net::IsLocal())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::PUT_ITEM;
		c.id = iindex;
		c.ile = 1;
	}
}

//=================================================================================================
void InventoryPanel::OnGivePotion(int id)
{
	int index = GetLockIndexAndRelease();
	if(id == BUTTON_OK && counter > 0 && index != -1)
	{
		Unit* t = unit->player->action_unit;
		if(t->CanTake(items->at(index).item, counter))
			GivePotion(index, counter);
		else
			GUI.SimpleDialog(Format(base.txNpcCantCarry, t->GetName()), this);
	}
}

//=================================================================================================
void InventoryPanel::GivePotion(int index, uint count)
{
	ItemSlot& slot = items->at(index);
	uint team_count = min(count, slot.team_count);
	// d�wi�k
	game.sound_mgr->PlaySound2d(game.GetItemSound(slot.item));
	// dodaj
	if(!unit->player->action_unit->AddItem(slot.item, count, 0u))
		UpdateGrid(false);
	// usu�
	unit->weight -= slot.item->weight*count;
	slot.count -= count;
	if(slot.count == 0)
	{
		last_index = INDEX_INVALID;
		if(mode == INVENTORY)
			base.tooltip.Clear();
		items->erase(items->begin() + index);
		UpdateGrid(true);
	}
	else
	{
		FormatBox();
		slot.team_count -= team_count;
	}
	// komunikat
	if(!Net::IsLocal())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::PUT_ITEM;
		c.id = index;
		c.ile = count;
	}
	else
		unit->player->action_unit->ai->have_potion = 2;
}

//=================================================================================================
void InventoryPanel::GiveSlotItem(ITEM_SLOT slot)
{
	last_index = INDEX_INVALID;
	if(mode == INVENTORY)
		base.tooltip.Clear();
	DialogInfo info;
	const Item* item = slots[slot];
	if(unit->player->action_unit->gold >= item->value / 2)
	{
		// odkup przedmiot
		info.text = Format(base.txSellItem, item->value / 2);
		give_item_mode = 1;
	}
	else
	{
		// zaproponuj inn� cen�
		info.text = Format(base.txSellFreeItem, item->value / 2);
		give_item_mode = 2;
	}

	base.lock.Lock(slot);
	info.event = delegate<void(int)>(this, &InventoryPanel::OnGiveItem);
	info.order = ORDER_NORMAL;
	info.parent = this;
	info.type = DIALOG_YESNO;
	info.pause = false;
	GUI.ShowDialog(info);
}

//=================================================================================================
void InventoryPanel::IsBetterItemResponse(bool is_better)
{
	if(!base.lock || !base.lock.is_give)
	{
		Error("InventoryPanel::IsBetterItemResponse, inventory not is_give locked.");
		return;
	}

	int iindex = GetLockIndexOrSlotAndRelease();
	if(iindex == Unit::INVALID_IINDEX)
		Warn("InventoryPanel::IsBetterItem, item removed.");
	else if(game.pc->action != PlayerController::Action_GiveItems)
		Warn("InventoryPanel::IsBetterItem, no longer giving items.");
	else if(!is_better)
		GUI.SimpleDialog(base.txWontTakeItem, this);
	else
	{
		Unit* t = game.pc->action_unit;
		if(iindex >= 0)
		{
			// not equipped item
			ItemSlot& slot = unit->items[iindex];
			if(!t->CanTake(slot.item))
				GUI.SimpleDialog(Format(base.txNpcCantCarry, t->GetName()), this);
			else
			{
				DialogInfo info;
				if(slot.team_count > 0)
				{
					// give team item
					info.text = Format(base.txSellTeamItem, slot.item->value / 2);
					give_item_mode = 0;
				}
				else if(t->gold >= slot.item->value / 2)
				{
					// sell item
					info.text = Format(base.txSellItem, slot.item->value / 2);
					give_item_mode = 1;
				}
				else
				{
					// give item for free
					info.text = Format(base.txSellFreeItem, slot.item->value / 2);
					give_item_mode = 2;
				}

				base.lock.Lock(iindex, slot);
				info.event = delegate<void(int)>(this, &InventoryPanel::OnGiveItem);
				info.order = ORDER_NORMAL;
				info.parent = this;
				info.type = DIALOG_YESNO;
				info.pause = false;
				GUI.ShowDialog(info);
			}
		}
		else
		{
			// equipped item
			ITEM_SLOT slot_type = IIndexToSlot(iindex);
			const Item*& item = unit->slots[slot_type];
			if(!t->CanTake(item))
				GUI.SimpleDialog(Format(base.txNpcCantCarry, t->GetName()), this);
			else if(SlotRequireHideWeapon(slot_type))
			{
				// hide equipped item and give it
				unit->HideWeapon();
				unit->player->next_action = NA_GIVE;
				unit->player->next_action_data.slot = slot_type;
			}
			else
				GiveSlotItem(slot_type);
		}
	}
}

//=================================================================================================
void InventoryPanel::Show()
{
	base.BuildTmpInventory(0);
	visible = true;
	item_visible = nullptr;
	Event(GuiEvent_Show);
	GainFocus();
}

//=================================================================================================
void InventoryPanel::Hide()
{
	if(game.gui->book->visible)
		game.gui->book->Hide();
	LostFocus();
	visible = false;
	item_visible = nullptr;
}

//=================================================================================================
void InventoryPanel::UpdateGrid(bool mine)
{
	if(mine)
	{
		base.BuildTmpInventory(0);
		base.inv_trade_mine->UpdateScrollbar();
	}
	else
	{
		base.BuildTmpInventory(1);
		base.inv_trade_other->UpdateScrollbar();
	}
}

//=================================================================================================
void InventoryPanel::ReadBook(const Item* item)
{
	assert(item && item->type == IT_BOOK);
	game.gui->book->Show((const Book*)item);
	base.tooltip.Clear();
}

//=================================================================================================
int InventoryPanel::GetLockIndexAndRelease()
{
	assert(base.lock);
	int index = FindItemIndex(*items, base.lock.index, base.lock.item, base.lock.is_team);
	base.lock = nullptr;
	return index;
}

//=================================================================================================
int InventoryPanel::GetLockIndexOrSlotAndRelease()
{
	assert(base.lock);
	int iindex;
	if(base.lock.slot == SLOT_INVALID)
	{
		iindex = FindItemIndex(*items, base.lock.index, base.lock.item, base.lock.is_team);
		if(iindex == -1)
			iindex = Unit::INVALID_IINDEX;
	}
	else if(slots && slots[(int)base.lock.slot])
		iindex = SlotToIIndex(base.lock.slot);
	else
		iindex = Unit::INVALID_IINDEX;
	base.lock = nullptr;
	return iindex;
}
