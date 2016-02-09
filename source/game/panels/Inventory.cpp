#include "Pch.h"
#include "Base.h"
#include "Inventory.h"
#include "Item.h"
#include "Unit.h"
#include "Game.h"
#include "Language.h"
#include "GetNumberDialog.h"

/* UWAGI CO DO ZMIENNYCH
index - indeks do items [0, 1, 2, 3...]
t_index - indeks to tmp_inv [0, 1, 2, 3...]
i_index - wartoœæ dla t_index (dla ujemnych slot) [-1, -2, -3, 0, 1, 2, 3...]
i_index == index dla nie za³o¿onych przedmiotów

last_index to tutaj t_index
*/

//-----------------------------------------------------------------------------
TEX Inventory::tItemBar, Inventory::tEquipped, Inventory::tGold, Inventory::tStarHq, Inventory::tStarM, Inventory::tStarU;
cstring Inventory::txGoldAndCredit, Inventory::txGoldDropInfo, Inventory::txCarryShort, Inventory::txCarry, Inventory::txCarryInfo, Inventory::txTeamItem, Inventory::txCantWear,
	Inventory::txCantDoNow, Inventory::txBuyTeamDialog, Inventory::txDropGoldCount, Inventory::txDropNoGold, Inventory::txDropNotNow, Inventory::txDropItemCount, Inventory::txWontBuy,
	Inventory::txPrice, Inventory::txNeedMoreGoldItem, Inventory::txBuyItemCount, Inventory::txSellItemCount, Inventory::txLooting, Inventory::txTrading, Inventory::txPutGoldCount,
	Inventory::txLootItemCount, Inventory::txPutItemCount, Inventory::txTakeAll, Inventory::txInventory, Inventory::txLootingChest, Inventory::txShareItems, Inventory::txGiveItems,
	Inventory::txPutGold, Inventory::txGiveGold, Inventory::txGiveGoldCount, Inventory::txShareGiveItemCount, Inventory::txCanCarryTeamOnly, Inventory::txWontGiveItem,
	Inventory::txShareTakeItemCount, Inventory::txWontTakeItem, Inventory::txSellTeamItem, Inventory::txSellItem, Inventory::txSellFreeItem, Inventory::txGivePotionCount,
	Inventory::txNpcCantCarry;
LOCK_MODE Inventory::lock_id;
int Inventory::lock_index;
bool Inventory::lock_give;
float Inventory::lock_timer;
TooltipController Inventory::tooltip;

//-----------------------------------------------------------------------------
#define INDEX_GOLD -2
#define INDEX_CARRY -3

//=================================================================================================
Inventory::Inventory() : last_item(nullptr), i_items(nullptr), game(Game::Get())
{
	scrollbar.total = 100;
	scrollbar.offset = 0;
	scrollbar.part = 100;
	bt.text = txTakeAll;
	bt.id = GuiEvent_Custom;
	bt.parent = this;
}

//=================================================================================================
void Inventory::LoadText()
{
	txGoldAndCredit = Str("goldAndCredit");
	txGoldDropInfo = Str("goldDropInfo");
	txCarryShort = Str("carryShort");
	txCarry = Str("carry");
	txCarryInfo = Str("carryInfo");
	txTeamItem = Str("teamItem");
	txCantWear = Str("cantWear");
	txCantDoNow = Str("cantDoNow");
	txBuyTeamDialog = Str("buyTeamDialog");
	txDropGoldCount = Str("dropGoldCount");
	txDropNoGold = Str("dropNoGold");
	txDropNotNow = Str("dropNotNow");
	txDropItemCount = Str("dropItemCount");
	txWontBuy = Str("wontBuy");
	txPrice = Str("price");
	txNeedMoreGoldItem = Str("needMoreGoldItem");
	txBuyItemCount = Str("buyItemCount");
	txSellItemCount = Str("sellItemCount");
	txLooting = Str("looting");
	txTrading = Str("trading");
	txPutGoldCount = Str("putGoldCount");
	txLootItemCount = Str("lootItemCount");
	txPutItemCount = Str("putItemCount");
	txTakeAll = Str("takeAll");
	txInventory = Str("inventory");
	txLootingChest = Str("lootingChest");
	txShareItems = Str("shareItems");
	txGiveItems = Str("giveItems");
	txPutGold = Str("putGold");
	txGiveGold = Str("giveGold2");
	txGiveGoldCount = Str("giveGoldCount");
	txShareGiveItemCount = Str("shareGiveItemCount");
	txCanCarryTeamOnly = Str("canCarryTeamOnly");
	txWontGiveItem = Str("wontGiveItem");
	txShareTakeItemCount = Str("shareTakeItemCount");
	txWontTakeItem = Str("wontTakeItem");
	txSellTeamItem = Str("sellTeamItem");
	txSellItem = Str("sellItem");
	txSellFreeItem = Str("sellFreeItem");
	txGivePotionCount = Str("givePotionCount");
	txNpcCantCarry = Str("npcCantCarry");
}

//=================================================================================================
void Inventory::LoadData()
{
	ResourceManager& resMgr = ResourceManager::Get();
	resMgr.GetLoadedTexture("item_bar.png", tItemBar);
	resMgr.GetLoadedTexture("equipped.png", tEquipped);
	resMgr.GetLoadedTexture("coins.png", tGold);
	resMgr.GetLoadedTexture("star_hq.png", tStarHq);
	resMgr.GetLoadedTexture("star_m.png", tStarM);
	resMgr.GetLoadedTexture("star_u.png", tStarU);
}

//=================================================================================================
void Inventory::InitTooltip()
{
	tooltip.Init(TooltipGetText(this, &Inventory::GetTooltip));
}

//=================================================================================================
void Inventory::Draw(ControlDrawData*)
{
	GamePanel::Draw();

	int ile_w = (size.x-48)/63;
	int ile_h = (size.y-64-34)/63;
	int shift_x = pos.x+12+(size.x-48)%63/2;
	int shift_y = pos.y+48+(size.y-64-34)%63/2;

	// scrollbar
	scrollbar.Draw();

	// miejsce na z³oto i udŸwig
	int bar_size = (ile_w*63-8)/2;
	int bar_y = shift_y+ile_h*63+8;
	float load = 0.f;
	if(mode != TRADE_OTHER && mode != LOOT_OTHER)
	{
		load = unit->GetLoad();
		GUI.DrawItem(tItemBar, INT2(shift_x, bar_y), INT2(bar_size,32), WHITE, 4);
		GUI.DrawItem(tEquipped, INT2(shift_x+bar_size+10, bar_y), INT2(int(min(1.f, load)*bar_size),32), WHITE, 4);
		GUI.DrawItem(tItemBar, INT2(shift_x+bar_size+10, bar_y), INT2(bar_size,32), WHITE, 4);
	}
	else if(mode == LOOT_OTHER)
		bt.Draw();

	// napis u góry
	RECT rect = {
		pos.x,
		pos.y+8,
		pos.x+size.x,
		pos.y+size.y
	};
	GUI.DrawText(GUI.fBig, title, DT_CENTER|DT_SINGLELINE, BLACK, rect, &rect);

	if(mode != TRADE_OTHER && mode != LOOT_OTHER)
	{
		// ikona z³ota
		GUI.DrawSprite(tGold, INT2(shift_x, bar_y));

		// z³oto
		rect.left = shift_x;
		rect.right = shift_x+bar_size;
		rect.top = bar_y;
		rect.bottom = rect.top+32;
		GUI.DrawText(GUI.default_font, Format("%d", unit->gold), DT_CENTER|DT_VCENTER, BLACK, rect);

		// udŸwig
		rect.left = shift_x+bar_size+10;
		rect.right = rect.left+bar_size;
		cstring weight_str = Format(txCarryShort, float(unit->weight)/10, float(unit->weight_max)/10);
		int w = GUI.default_font->LineWidth(weight_str);
		GUI.DrawText(GUI.default_font, (w > bar_size ? Format("%g/%g", float(unit->weight)/10, float(unit->weight_max)/10) : weight_str), DT_CENTER|DT_VCENTER, (load > 1.f ? RED : BLACK), rect);
	}

	// rysuj kratki
	for(int y=0; y<ile_h; ++y)
	{
		for(int x=0; x<ile_w; ++x)
			GUI.DrawSprite(tItemBar, INT2(shift_x+x*63, shift_y+y*63));
	}

	// rysuj przedmioty
	int shift = int(scrollbar.offset/63)*ile_w;
	for(int i=0, ile = min(ile_w*ile_h, (int)i_items->size()-shift); i<ile; ++i)
	{
		int i_item = i_items->at(i+shift);
		const Item* item;
		int count;
		if(i_item < 0)
		{
			item = slots[-i_item-1];
			count = 1;
		}
		else
		{
			ItemSlot& slot = items->at(i_item);
			item = slot.item;
			count = slot.count;
		}

		int x = i%ile_w,
			y = i/ile_w;

		// obrazek za³o¿onego przedmiotu
		if(i_item < 0)
			GUI.DrawSprite(tEquipped, INT2(shift_x+x*63, shift_y+y*63));

		// item quality icon
		TEX icon;
		if(IS_SET(item->flags, ITEM_HQ))
			icon = tStarHq;
		else if(IS_SET(item->flags, ITEM_MAGICAL))
			icon = tStarM;
		else if(IS_SET(item->flags, ITEM_UNIQUE))
			icon = tStarU;
		else
			icon = nullptr;
		if(icon)
			GUI.DrawSprite(icon, INT2(shift_x+x*63, shift_y+(y+1)*63-32));

		// obrazek przedmiotu
		GUI.DrawSprite(item->tex, INT2(shift_x+x*63, shift_y+y*63));

		// iloœæ
		if(count > 1)
		{
			RECT rect3 = {shift_x+x*63+2, shift_y+y*63};
			rect3.right = rect3.left + 64;
			rect3.bottom = rect3.top + 63;
			GUI.DrawText(GUI.default_font, Format("%d", count), DT_BOTTOM, BLACK, rect3);
		}
	}

	if(mode == INVENTORY)
		tooltip.Draw();
	else
		DrawBox();
}

//=================================================================================================
void Inventory::Update(float dt)
{
	GamePanel::Update(dt);

	if(lock_id == LOCK_TRADE_MY && lock_give)
	{
		lock_timer -= dt;
		if(lock_timer <= 0.f)
		{
			ERROR("IS_BETTER_ITEM timed out.");
			lock_id = LOCK_NO;
			lock_give = false;
		}
	}

	int ile_w = (size.x-48)/63;
	int ile_h = (size.y-64-34)/63;
	int shift_x = pos.x+12+(size.x-48)%63/2;
	int shift_y = pos.y+48+(size.y-64-34)%63/2;

	int new_index = INDEX_INVALID;

	INT2 cursor_pos = GUI.cursor_pos;

	bool have_focus = (mode == INVENTORY ? focus : mouse_focus);

	if(have_focus && Key.Focus() && IsInside(GUI.cursor_pos))
		scrollbar.ApplyMouseWheel();
	if(focus)
		scrollbar.Update(dt);

	int shift = int(scrollbar.offset/63)*ile_w;

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
				item = slots[-i_index-1];
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
			int x = (cursor_pos.x-shift_x)/63,
				y = (cursor_pos.y-shift_y)/63;
			if(x >= 0 && x < ile_w && y >= 0 && y < ile_h)
			{
				int i = x+y*ile_w;
				if(i < (int)i_items->size()-shift)
					new_index = i+shift;
			}
		}

		int bar_size = (ile_w*63-8)/2;
		int bar_y = shift_y+ile_h*63+8;

		// pasek ze z³otem lub udŸwigiem
		if(mode != TRADE_OTHER && mode != LOOT_OTHER)
		{
			if(cursor_pos.y >= bar_y && cursor_pos.y < bar_y+32)
			{
				if(cursor_pos.x >= shift_x && cursor_pos.x < shift_x+bar_size)
					new_index = INDEX_GOLD;
				else if(cursor_pos.x >= shift_x+bar_size+10 && cursor_pos.x < shift_x+bar_size+10+bar_size)
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

	// klawisz to podnoszenia wszystkich przedmiotów
	if(mode == LOOT_OTHER && (focus || game.game_gui->inv_trade_mine->focus) && Key.Focus() && GKey.PressedRelease(GK_TAKE_ALL))
		Event(GuiEvent_Custom);
	
	// aktualizuj box
	if(mode == INVENTORY)
		tooltip.Update(dt, new_index, -1);
	else
		UpdateBoxIndex(dt, new_index);

	if(new_index >= 0)
	{
		//int t_index = new_index;

		// pobierz przedmiot
		const Item* item;
		ItemSlot* slot;
		ITEM_SLOT slot_type;
		int i_index = i_items->at(new_index);
		if(i_index < 0)
		{
			item = slots[-i_index-1];
			slot = nullptr;
			slot_type = (ITEM_SLOT)(-i_index-1);
		}
		else
		{
			slot = &items->at(i_index);
			item = slot->item;
			slot_type = SLOT_INVALID;
		}

		last_item = item;

		if(!focus || !(game.pc->unit->action == A_NONE || game.pc->unit->CanDoWhileUsing()) || lock_give || lock_id != LOCK_NO)
			return;

		// obs³uga kilkania w ekwipunku
		if(mode == INVENTORY && Key.Focus() && Key.PressedRelease(VK_RBUTTON) && game.pc->unit->action == A_NONE)
		{
			// wyrzucanie przedmiotu
			// wyrzuæ przedmiot
			if(IS_SET(item->flags, ITEM_DONT_DROP) && game.IsAnyoneTalking())
				GUI.SimpleDialog(txCantDoNow, this);
			else
			{
				if(!slot)
				{
					if(SlotRequireHideWeapon(slot_type))
					{
						// ma wyjêty ten przedmiot, schowaj go
						unit->HideWeapon();
						// potem wyrzuæ
						assert(lock_id == LOCK_NO);
						unit->player->next_action = NA_DROP;
						lock_id = LOCK_MY;
						lock_index = SlotToIIndex(slot_type);
					}
					else
						DropSlotItem(slot_type);
				}
				else
				{
					if(Key.Down(VK_SHIFT))
					{
						// wyrzuæ wszystkie
						unit->DropItems(i_index, 0);
						last_index = INDEX_INVALID;
						if(mode == INVENTORY)
							tooltip.Clear();
						game.BuildTmpInventory(0);
						UpdateScrollbar();
					}
					else if(Key.Down(VK_CONTROL) || slot->count == 1)
					{
						// wyrzuæ jeden
						if(unit->DropItem(i_index))
						{
							game.BuildTmpInventory(0);
							UpdateScrollbar();
							last_index = INDEX_INVALID;
							if(mode == INVENTORY)
								tooltip.Clear();
						}
						else
							FormatBox();
					}
					else
					{
						// wyrzuæ okreœlon¹ liczbê
						assert(lock_id == LOCK_NO);
						counter = 1;
						lock_id = LOCK_MY;
						lock_index = i_index;
						GetNumberDialog::Show(this, fastdelegate::FastDelegate1<int>(this, &Inventory::OnDropItem), txDropItemCount, 0, slot->count, &counter);
						last_index = INDEX_INVALID;
						if(mode == INVENTORY)
							tooltip.Clear();
					}
				}
			}
		}
		else if(Key.Focus() && Key.PressedRelease(VK_LBUTTON))
		{
			switch(mode)
			{
			case INVENTORY:
				// u¿yj/za³ó¿/zdejmij przedmiot
				if(!slot)
				{
					// zdejmij przedmiot
					if(SlotRequireHideWeapon(slot_type))
					{
						// schowaj broñ
						unit->HideWeapon();
						// ustaw ¿eby j¹ zdj¹æ
						assert(lock_id == LOCK_NO);
						lock_id = LOCK_MY;
						lock_index = SlotToIIndex(slot_type);
						unit->player->next_action = NA_REMOVE;
					}
					else
						RemoveSlotItem(slot_type);
				}
				else
				{
					if(item->IsWearableByHuman() && slot->team_count > 0 && game.GetActiveTeamSize() > 1)
					{
						assert(lock_id == LOCK_NO);
						DialogInfo di;
						di.event = fastdelegate::FastDelegate1<int>(this, &Inventory::OnTakeItem);
						di.name = "takeover_item";
						di.parent = this;
						di.pause = false;
						di.text = txBuyTeamDialog;
						di.order = ORDER_NORMAL;
						di.type = DIALOG_YESNO;
						GUI.ShowDialog(di);
						lock_id = LOCK_MY;
						lock_index = i_index;
						last_index = INDEX_INVALID;
						if(mode == INVENTORY)
							tooltip.Clear();
					}
					else if(item->type == IT_CONSUMEABLE)
						ConsumeItem(i_index);
					else if(item->IsWearable())
					{
						ITEM_SLOT type = ItemTypeToSlot(item->type);
						if(SlotRequireHideWeapon(type))
						{
							// ma wyjêty przedmiot, schowaj go a potem za³ó¿ nowy
							assert(lock_id == LOCK_NO);
							unit->HideWeapon();
							unit->player->next_action = NA_EQUIP;
							lock_id = LOCK_MY;
							lock_index = i_index;
						}
						else
						{
							// sprawdŸ czy mo¿na u¿ywaæ
							bool ok = true;
							if(type == SLOT_ARMOR)
							{
								if(item->ToArmor().armor_type != ArmorUnitType::HUMAN)
								{
									ok = false;
									GUI.SimpleDialog(txCantWear, this);
								}
							}

							if(ok)
								EquipSlotItem(type, i_index);
						}
					}
				}
				break;
			case TRADE_MY:
				// sprzedawanie przedmiotów
				if(item->value <= 1 || !game.CanBuySell(item))
					GUI.SimpleDialog(txWontBuy, this);
				else if(!slot)
				{
					// za³o¿ony przedmiot
					if(SlotRequireHideWeapon(slot_type))
					{
						// ma wyjêty ten przedmiot, schowaj go i sprzedaj
						assert(Inventory::lock_id == LOCK_NO);
						lock_id = LOCK_TRADE_MY;
						lock_index = SlotToIIndex(slot_type);
						unit->HideWeapon();
						unit->player->next_action = NA_SELL;
					}
					else
						SellSlotItem(slot_type);
				}
				else
				{
					// nie za³o¿ony przedmiot
					// ustal ile gracz chce sprzedaæ przedmiotów
					uint ile;
					if(item->IsStackable() && slot->count != 1)
					{
						if(Key.Down(VK_SHIFT))
							ile = slot->count;
						else if(Key.Down(VK_CONTROL))
							ile = 1;
						else
						{
							assert(lock_id == LOCK_NO);
							counter = 1;
							lock_id = LOCK_TRADE_MY;
							lock_index = i_index;
							GetNumberDialog::Show(this, fastdelegate::FastDelegate1<int>(this, &Inventory::OnSellItem), txSellItemCount, 0, slot->count, &counter);
							last_index = INDEX_INVALID;
							if(mode == INVENTORY)
								tooltip.Clear();
							break;
						}
					}
					else
						ile = 1;

					SellItem(i_index, ile);
				}
				break;
			case TRADE_OTHER:
				// kupowanie przedmiotów
				{
					// ustal ile gracz chce kupiæ przedmiotów
					uint ile;
					if(item->IsStackable() && slot->count != 1)
					{
						if(Key.Down(VK_SHIFT))
							ile = slot->count;
						else if(Key.Down(VK_CONTROL))
							ile = 1;
						else
						{
							assert(lock_id == LOCK_NO);
							counter = 1;
							lock_id = LOCK_TRADE_OTHER;
							lock_index = i_index;
							GetNumberDialog::Show(this, fastdelegate::FastDelegate1<int>(this, &Inventory::OnBuyItem), txBuyItemCount, 0, slot->count, &counter);
							last_index = INDEX_INVALID;
							if(mode == INVENTORY)
								tooltip.Clear();
							break;
						}
					}
					else
						ile = 1;

					BuyItem(i_index, ile);
				}
				break;
			case LOOT_MY:
				// chowanie przedmiotów do zw³ok/skrzyni
				if(slot)
				{
					// nie za³o¿ony przedmiot
					uint ile;
					if(item->IsStackable() && slot->count != 1)
					{
						if(Key.Down(VK_SHIFT))
							ile = slot->count;
						else if(Key.Down(VK_CONTROL))
							ile = 1;
						else
						{
							assert(lock_id == LOCK_NO);
							counter = 1;
							lock_id = LOCK_TRADE_MY;
							lock_index = i_index;
							GetNumberDialog::Show(this, fastdelegate::FastDelegate1<int>(this, &Inventory::OnPutItem), txPutItemCount, 0, slot->count, &counter);
							last_index = INDEX_INVALID;
							if(mode == INVENTORY)
								tooltip.Clear();
							break;
						}
					}
					else
						ile = 1;

					PutItem(i_index, ile);
				}
				else
				{
					// za³o¿ony przedmiot
					if(SlotRequireHideWeapon(slot_type))
					{
						// ma wyjêty ten przedmiot, schowaj go a potem wyrzuæ
						assert(lock_id == LOCK_NO);
						lock_id = LOCK_TRADE_MY;
						lock_index = SlotToIIndex(slot_type);
						unit->HideWeapon();
						unit->player->next_action = NA_PUT;
					}
					else
						PutSlotItem(slot_type);
				}
				break;
			case LOOT_OTHER:
				// zabieranie przedmiotów ze zw³ok/skrzyni
				if(slot)
				{
					// nie za³o¿ony przedmiot
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
							assert(lock_id == LOCK_NO);
							counter = 1;
							lock_id = LOCK_TRADE_OTHER;
							lock_index = i_index;
							GetNumberDialog::Show(this, fastdelegate::FastDelegate1<int>(this, &Inventory::OnLootItem), txLootItemCount, 0, slot->count, &counter);
							last_index = INDEX_INVALID;
							if(mode == INVENTORY)
								tooltip.Clear();
							break;
						}
					}
					else
						ile = 1;

					LootItem(i_index, ile);
				}
				else
				{
					// za³o¿ony przedmiot
					last_index = INDEX_INVALID;
					if(mode == INVENTORY)
						tooltip.Clear();
					// dodaj
					game.pc->unit->AddItem(item);
					game.BuildTmpInventory(0);
					// usuñ
					if(slot_type == SLOT_WEAPON && slots[SLOT_WEAPON] == unit->used_item)
					{
						unit->used_item = nullptr;
						if(game.IsServer())
						{
							NetChange& c = Add1(game.net_changes);
							c.type = NetChange::REMOVE_USED_ITEM;
							c.unit = unit;
						}
					}
					slots[slot_type] = nullptr;
					unit->weight -= item->weight;
					game.BuildTmpInventory(1);
					// dŸwiêk
					if(game.sound_volume)
						game.PlaySound2d(game.GetItemSound(item));
					// komunikat
					if(game.IsOnline())
					{
						NetChange& c = Add1(game.net_changes);
						if(game.IsServer())
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
				// dawanie przedmiotów sojusznikowi na przechowanie
				if(slot && slot->team_count > 0)
				{
					// nie za³o¿ony przedmiot
					uint ile;
					if(item->IsStackable() && slot->team_count != 1)
					{
						if(Key.Down(VK_CONTROL))
							ile = 1;
						else if(Key.Down(VK_SHIFT))
							ile = slot->team_count;
						else
						{
							assert(lock_id == LOCK_NO);
							counter = 1;
							lock_id = LOCK_TRADE_MY;
							lock_index = i_index;
							GetNumberDialog::Show(this, fastdelegate::FastDelegate1<int>(this, &Inventory::OnShareGiveItem), txShareGiveItemCount, 0, slot->team_count, &counter);
							last_index = INDEX_INVALID;
							if(mode == INVENTORY)
								tooltip.Clear();
							break;
						}
					}
					else
						ile = 1;

					Unit* t = unit->player->action_unit;
					if(t->CanTake(item))
						ShareGiveItem(i_index, ile);
					else
						GUI.SimpleDialog(Format(txNpcCantCarry, t->GetName()), this);
				}
				else
				{
					last_index = INDEX_INVALID;
					if(mode == INVENTORY)
						tooltip.Clear();
					GUI.SimpleDialog(txCanCarryTeamOnly, this);
				}
				break;
			case SHARE_OTHER:
			case GIVE_OTHER:
				// zabieranie przedmiotów od sojusznika
				if(slot && slot->team_count > 0)
				{
					// nie za³o¿ony przedmiot
					uint ile;
					if(item->IsStackable() && slot->team_count != 1)
					{
						if(Key.Down(VK_SHIFT))
							ile = 1;
						else if(Key.Down(VK_CONTROL))
							ile = slot->team_count;
						else
						{
							assert(lock_id == LOCK_NO);
							counter = 1;
							lock_id = LOCK_TRADE_OTHER;
							lock_index = i_index;
							GetNumberDialog::Show(this, fastdelegate::FastDelegate1<int>(this, &Inventory::OnShareTakeItem), txShareTakeItemCount, 0, slot->team_count, &counter);
							last_index = INDEX_INVALID;
							if(mode == INVENTORY)
								tooltip.Clear();
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
						tooltip.Clear();
					GUI.SimpleDialog(txWontGiveItem, this);
				}
				break;
			case GIVE_MY:
				// dawanie przedmiotów sojusznikowi
				Unit* t = unit->player->action_unit;
				if(slot)
				{
					// nie za³o¿ony przedmiot
					if(item->IsWearableByHuman())
					{
						last_index = INDEX_INVALID;
						if(mode == INVENTORY)
							tooltip.Clear();

						if(game.IsLocal())
						{
							if(!t->IsBetterItem(item))
								GUI.SimpleDialog(txWontTakeItem, this);
							else if(t->CanTake(item))
							{
								DialogInfo info;
								if(slot->team_count > 0)
								{
									// daj dru¿ynowy przedmiot
									info.text = Format(txSellTeamItem, item->value/2);
									give_item_mode = 0;
								}
								else if(t->gold >= item->value/2)
								{
									// odkup przedmiot
									info.text = Format(txSellItem, item->value/2);
									give_item_mode = 1;
								}
								else
								{
									// zaproponuj inn¹ cenê
									info.text = Format(txSellFreeItem, item->value/2);
									give_item_mode = 2;
								}
								assert(lock_id == LOCK_NO);
								lock_id = LOCK_TRADE_MY;
								lock_index = i_index;
								info.event = fastdelegate::FastDelegate1<int>(this, &Inventory::OnGiveItem);
								info.order = ORDER_NORMAL;
								info.parent = this;
								info.type = DIALOG_YESNO;
								info.pause = false;
								GUI.ShowDialog(info);
							}
							else
								GUI.SimpleDialog(Format(txNpcCantCarry, t->GetName()), this);
						}
						else
						{
							assert(lock_id == LOCK_NO);
							lock_id = LOCK_TRADE_MY;
							lock_index = i_index;
							lock_give = true;
							lock_timer = 1.f;
							NetChange& c = Add1(game.net_changes);
							c.type = NetChange::IS_BETTER_ITEM;
							c.id = i_index;
						}
					}
					else if(item->type == IT_CONSUMEABLE && item->ToConsumeable().IsHealingPotion())
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
								assert(lock_id == LOCK_NO);
								counter = 1;
								lock_id = LOCK_TRADE_MY;
								lock_index = i_index;
								GetNumberDialog::Show(this, fastdelegate::FastDelegate1<int>(this, &Inventory::OnGivePotion), txGivePotionCount, 0, slot->count, &counter);
								last_index = INDEX_INVALID;
								if(mode == INVENTORY)
									tooltip.Clear();
								break;
							}
						}
						else
							ile = 1;

						if(t->CanTake(item, ile))
							GivePotion(i_index, ile);
						else
							GUI.SimpleDialog(Format(txNpcCantCarry, t->GetName()), this);
					}
					else
					{
						last_index = INDEX_INVALID;
						if(mode == INVENTORY)
							tooltip.Clear();
						GUI.SimpleDialog(txWontTakeItem, this);
					}
				}
				else
				{
					if(game.IsLocal())
					{
						if(t->IsBetterItem(item))
						{
							// za³o¿ony przedmiot
							if(t->CanTake(item))
							{
								if(SlotRequireHideWeapon(slot_type))
								{
									// ma wyjêty ten przedmiot, schowaj go a potem daj
									assert(lock_id == LOCK_NO);
									lock_id = LOCK_TRADE_MY;
									lock_index = SlotToIIndex(slot_type);
									unit->HideWeapon();
									unit->player->next_action = NA_GIVE;
								}
								else
									GiveSlotItem(slot_type);
							}
							else
								GUI.SimpleDialog(Format(txNpcCantCarry, t->GetName()), this);
						}
						else
						{
							last_index = INDEX_INVALID;
							if(mode == INVENTORY)
								tooltip.Clear();
							GUI.SimpleDialog(txWontTakeItem, this);
						}
					}
					else
					{
						assert(lock_id == LOCK_NO);
						lock_id = LOCK_TRADE_MY;
						lock_index = i_index;
						lock_give = true;
						lock_timer = 1.f;
						NetChange& c = Add1(game.net_changes);
						c.type = NetChange::IS_BETTER_ITEM;
						c.id = i_index;
					}
				}
				break;
			}
		}
	}
	else if(new_index == INDEX_GOLD && mode != GIVE_OTHER && mode != SHARE_OTHER && lock_id == LOCK_NO)
	{
		// wyrzucanie/chowanie/dawanie z³ota
		if(Key.PressedRelease(VK_LBUTTON))
		{
			last_index = INDEX_INVALID;
			if(mode == INVENTORY)
				tooltip.Clear();
			counter = 0;
			if(mode == INVENTORY)
				GetNumberDialog::Show(this, fastdelegate::FastDelegate1<int>(this, &Inventory::OnDropGold), txDropGoldCount, 0, unit->gold, &counter);
			else if(mode == LOOT_MY)
				GetNumberDialog::Show(this, fastdelegate::FastDelegate1<int>(this, &Inventory::OnPutGold), txPutGoldCount, 0, unit->gold, &counter);
			else if(mode == GIVE_MY || mode == SHARE_MY)
				GetNumberDialog::Show(this, fastdelegate::FastDelegate1<int>(this, &Inventory::OnGiveGold), txGiveGoldCount, 0, unit->gold, &counter);
		}
	}

	if(mode == INVENTORY && Key.Focus() && Key.PressedRelease(VK_ESCAPE))
		Hide();
}

//=================================================================================================
void Inventory::Event(GuiEvent e)
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
		int ile_w = (size.x-48)/63;
		int ile_h = (size.y-64-34)/63;
		int shift_x = 12+(size.x-48)%63/2;
		int shift_y = 48+(size.y-64-34)%63/2;
		int bar_size = (ile_w*63-8)/2;
		int bar_y = shift_y+ile_h*63+8;
		bt.pos = INT2(shift_x+bar_size+10, bar_y);
		bt.size = INT2(bar_size,36);
		bt.global_pos = global_pos + bt.pos;
		if(e == GuiEvent_Show)
			tooltip.Clear();
	}
	else if(e == GuiEvent_LostFocus)
		scrollbar.LostFocus();
	else if(e == GuiEvent_Custom)
	{
		if(game.pc->unit->action != A_NONE)
			return;

		// przycisk - zabierz wszystko
		bool zloto = false;
		SOUND snd[3] = {0};
		vector<ItemSlot>& itms = game.pc->unit->items;
		bool changes = false;

		// sloty
		if(game.pc->action != PlayerController::Action_LootChest)
		{
			const Item** unit_slots = game.pc->action_unit->slots;
			for(int i=0; i<SLOT_MAX; ++i)
			{
				if(unit_slots[i])
				{
					SOUND s = game.GetItemSound(unit_slots[i]);
					if(s == game.sCoins)
						zloto = true;
					else
					{
						for(int i=0; i<3; ++i)
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

					if(game.IsServer())
					{
						NetChange& c = Add1(game.net_changes);
						c.type = NetChange::CHANGE_EQUIPMENT;
						c.unit = game.pc->action_unit;
						c.id = i;
					}

					changes = true;
				}
			}

			// wyzeruj wagê ekwipunku
			game.pc->action_unit->weight = 0;
		}

		// zwyk³e przedmioty
		for(vector<ItemSlot>::iterator it = game.pc->chest_trade->begin(), end = game.pc->chest_trade->end(); it != end; ++it)
		{
			if(!it->item)
				continue;

			if(it->item->type == IT_GOLD)
			{
				zloto = true;
				game.pc->unit->AddItem(game.gold_item_ptr, it->count, it->team_count);
			}
			else
			{
				InsertItemBare(itms, it->item, it->count, it->team_count);
				game.pc->unit->weight += it->item->weight * it->count;
				SOUND s = game.GetItemSound(it->item);
				for(int i=0; i<3; ++i)
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

		if(!game.IsLocal())
		{
			NetChange& c = Add1(game.net_changes);
			c.type = NetChange::GET_ALL_ITEMS;
		}

		// dŸwiêk podnoszenia przedmiotów
		if(game.sound_volume)
		{
			for(int i=0; i<3; ++i)
			{
				if(snd[i])
					game.PlaySound2d(snd[i]);
			}
			if(zloto)
				game.PlaySound2d(game.sCoins);
		}

		// zamknij ekwipunek
		if(changes)
			SortItems(itms);
		game.game_gui->gp_trade->Hide();
		game.OnCloseInventory();
	}
}

//=================================================================================================
bool Inventory::SlotRequireHideWeapon(ITEM_SLOT slot)
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
// przek³ada przedmiot ze slotu do ekwipunku
void Inventory::RemoveSlotItem(ITEM_SLOT slot)
{
	unit->AddItem(slots[slot], 1, false);
	unit->weight -= slots[slot]->weight;
	slots[slot] = nullptr;
	game.BuildTmpInventory(0);

	if(game.IsOnline())
	{
		NetChange& c = Add1(game.net_changes);
		c.type = NetChange::CHANGE_EQUIPMENT;
		if(game.IsServer())
		{
			c.unit = unit;
			c.id = slot;
		}
		else
			c.id = SlotToIIndex(slot);
	}
}

//=================================================================================================
void Inventory::DropSlotItem(ITEM_SLOT slot)
{
	unit->DropItem(slot);
	game.BuildTmpInventory(0);
	UpdateScrollbar();
}

//=================================================================================================
void Inventory::ConsumeItem(int index)
{
	int w = unit->ConsumeItem(index);
	if(w == 0)
	{
		last_index = INDEX_INVALID;
		if(mode == INVENTORY)
			tooltip.Clear();
		game.BuildTmpInventory(0);
		UpdateScrollbar();
	}
	else if(w == 1 && last_index-game.tmp_inventory_shift[0] == index)
		FormatBox();
}

//=================================================================================================
void Inventory::EquipSlotItem(ITEM_SLOT slot, int i_index)
{
	if(slots[slot])
	{
		const Item* prev_item = slots[slot];
		// ustaw slot
		slots[slot] = items->at(i_index).item;
		items->erase(items->begin()+i_index);
		// dodaj stary przedmiot
		unit->AddItem(prev_item, 1, false);
		unit->weight -= prev_item->weight;
	}
	else
	{
		// ustaw slot
		slots[slot] = items->at(i_index).item;
		// usuñ przedmiot
		items->erase(items->begin()+i_index);
	}

	game.BuildTmpInventory(0);

	if(game.IsOnline())
	{
		NetChange& c = Add1(game.net_changes);
		c.type = NetChange::CHANGE_EQUIPMENT;
		if(game.IsServer())
		{
			c.unit = unit;
			c.id = slot;
		}
		else
			c.id = i_index;
	}
}

//=================================================================================================
void Inventory::EquipSlotItem(int index)
{
	EquipSlotItem(ItemTypeToSlot(items->at(index).item->type), index);
}

//=================================================================================================
void Inventory::FormatBox()
{
	if(last_index == INDEX_GOLD)
	{
		box_text = Format(txGoldAndCredit, unit->gold, unit->IsPlayer() ? unit->player->credit : unit->hero->credit);
		switch(mode)
		{
		case INVENTORY:
			box_text_small = txGoldDropInfo;
			break;
		case TRADE_MY:
		case TRADE_OTHER:
		case LOOT_OTHER:
		case GIVE_OTHER:
		case SHARE_OTHER:
			box_text_small.clear();
			break;
		case LOOT_MY:
			box_text_small = txPutGold;
			break;
		case SHARE_MY:
		case GIVE_MY:
			box_text_small = txGiveGold;
			break;
		}
		box_img = tGold;
	}
	else if(last_index == INDEX_CARRY)
	{
		box_text = Format(txCarry, float(unit->weight)/10, int(unit->GetLoad()*100), float(unit->weight_max)/10);
		box_text_small = txCarryInfo;
		box_img = nullptr;
	}
	else if(last_index != INDEX_INVALID)
	{
		const Item* item;
		int count, team_count;
		int i_index = i_items->at(last_index);
		if(i_index < 0)
		{
			item = slots[-i_index-1];
			count = 1;
			team_count = 0;
		}
		else
		{
			ItemSlot& slot = items->at(i_index);
			item = slot.item;
			count = slot.count;
			team_count = slot.team_count;
		}

		Unit* for_unit;
		if(mode == LOOT_OTHER || mode == TRADE_OTHER)
			for_unit = game.pc->unit;
		else
			for_unit = unit;

		GetItemString(box_text, item, for_unit, (uint)count);
		if(mode != TRADE_OTHER && team_count && game.active_team.size() > 1)
		{
			box_text += '\n';
			box_text += txTeamItem;
			if(count != 1)
				box_text += Format(" (%d)", team_count);
		}
		if(mode == TRADE_MY)
		{
			box_text += '\n';
			int price = game.GetItemPrice(item, *game.pc->unit, false);
			if(price == 0 || !game.CanBuySell(item))
				box_text += txWontBuy;
			else
				box_text += Format(txPrice, price);
		}
		else if(mode == TRADE_OTHER)
		{
			int price = game.GetItemPrice(item, *game.pc->unit, true);
			box_text += '\n';
			box_text += Format(txPrice, price);
		}
		box_text_small = item->desc;
		box_img = item->tex;
	}
}

//=================================================================================================
void Inventory::GetTooltip(TooltipController*, int group, int)
{
	if(group == INDEX_INVALID)
	{
		tooltip.anything = false;
		return;
	}
	
	tooltip.anything = true;
	
	if(group == INDEX_GOLD)
	{
		tooltip.img = tGold;
		tooltip.big_text.clear();
		tooltip.text = Format(txGoldAndCredit, unit->gold, unit->IsPlayer() ? unit->player->credit : unit->hero->credit);
		switch(mode)
		{
		case INVENTORY:
			tooltip.small_text = txGoldDropInfo;
			break;
		case TRADE_MY:
		case TRADE_OTHER:
		case LOOT_OTHER:
		case GIVE_OTHER:
		case SHARE_OTHER:
			tooltip.small_text.clear();
			break;
		case LOOT_MY:
			tooltip.small_text = txPutGold;
			break;
		case SHARE_MY:
		case GIVE_MY:
			tooltip.small_text = txGiveGold;
			break;
		}
	}
	else if(group == INDEX_CARRY)
	{
		tooltip.img = nullptr;
		tooltip.text = Format(txCarry, float(unit->weight) / 10, int(unit->GetLoad() * 100), float(unit->weight_max) / 10);
		tooltip.small_text = txCarryInfo;
		tooltip.big_text.clear();
	}
	else
	{
		const Item* item;
		int count, team_count;
		int i_index = i_items->at(group);
		if(i_index < 0)
		{
			item = slots[-i_index - 1];
			count = 1;
			team_count = 0;
		}
		else
		{
			ItemSlot& slot = items->at(i_index);
			item = slot.item;
			count = slot.count;
			team_count = slot.team_count;
		}

		Unit* for_unit;
		if(mode == LOOT_OTHER || mode == TRADE_OTHER)
			for_unit = game.pc->unit;
		else
			for_unit = unit;

		GetItemString(tooltip.text, item, for_unit, (uint)count);
		if(mode != TRADE_OTHER && team_count && game.active_team.size() > 1)
		{
			tooltip.text += '\n';
			tooltip.text += txTeamItem;
			if(count != 1)
				tooltip.text += Format(" (%d)", team_count);
		}
		if(mode == TRADE_MY)
		{
			tooltip.text += '\n';
			int price = game.GetItemPrice(item, *game.pc->unit, false);
			if(price == 0 || !game.CanBuySell(item))
				tooltip.text += txWontBuy;
			else
				tooltip.text += Format(txPrice, price);
		}
		else if(mode == TRADE_OTHER)
		{
			int price = game.GetItemPrice(item, *game.pc->unit, true);
			tooltip.text += '\n';
			tooltip.text += Format(txPrice, price);
		}
		tooltip.small_text = item->desc;
		tooltip.big_text.clear();
		tooltip.img = item->tex;
	}
}

//=================================================================================================
void Inventory::OnDropGold(int id)
{
	if(id == BUTTON_CANCEL || counter == 0)
		return;

	if(counter > unit->gold)
		GUI.SimpleDialog(txDropNoGold, this);
	else if(unit->action != A_NONE || unit->live_state != Unit::ALIVE)
		GUI.SimpleDialog(txDropNotNow, this);
	else
		game.DropGold(counter);
}

//=================================================================================================
void Inventory::OnDropItem(int id)
{
	assert(lock_id == LOCK_MY);
	lock_id = LOCK_NO;

	if(id == BUTTON_CANCEL || counter == 0 || lock_index == LOCK_REMOVED)
		return;

	if(unit->action != A_NONE || unit->live_state != Unit::ALIVE)
		GUI.SimpleDialog(txDropNotNow, this);
	else
	{
		if(unit->DropItems(lock_index, counter))
		{
			game.BuildTmpInventory(0);
			UpdateScrollbar();
		}
	}
}

//=================================================================================================
void Inventory::OnTakeItem(int id)
{
	assert(lock_id == LOCK_MY);
	lock_id = LOCK_NO;
	if(id == BUTTON_NO || lock_index == LOCK_REMOVED)
		return;

	ItemSlot& slot = items->at(lock_index);
	unit->player->credit += slot.item->value/2;
	slot.team_count = 0;

	if(game.IsLocal())
		game.CheckCredit(true);
	else
	{
		NetChange& c = Add1(game.net_changes);
		c.type = NetChange::TAKE_ITEM_CREDIT;
		c.id = lock_index;
	}
}

//=================================================================================================
void Inventory::UpdateScrollbar()
{
	if(!i_items)
		return;

	int ile_w = (size.x-48)/63;
	int ile_h = (size.y-64-34)/63;
	int shift_x = pos.x+12+(size.x-48)%63/2;
	int shift_y = pos.y+48+(size.y-64-34)%63/2;
	int ile = i_items->size();
	int s = ((ile+ile_w-1)/ile_w)*63;
	scrollbar.size = INT2(16, ile_h*63);
	scrollbar.total = s;
	scrollbar.part = min(s, scrollbar.size.y);
	scrollbar.pos = INT2(shift_x+ile_w*63+8-pos.x, shift_y-pos.y);
	scrollbar.global_pos = global_pos + scrollbar.pos;
	if(scrollbar.offset+scrollbar.part > scrollbar.total)
		scrollbar.offset = float(scrollbar.total-scrollbar.part);
	if(scrollbar.offset < 0)
		scrollbar.offset = 0;
}

//=================================================================================================
void Inventory::OnBuyItem(int id)
{
	assert(lock_id == LOCK_TRADE_OTHER);
	lock_id = LOCK_NO;
	if(id == BUTTON_OK && counter > 0 && lock_index != LOCK_REMOVED)
		BuyItem(lock_index, counter);
}

//=================================================================================================
void Inventory::OnSellItem(int id)
{
	assert(lock_id == LOCK_TRADE_MY);
	lock_id = LOCK_NO;
	if(id == BUTTON_OK && counter > 0 && lock_index != LOCK_REMOVED)
		SellItem(lock_index, counter);
}

//=================================================================================================
void Inventory::BuyItem(int index, uint count)
{
	ItemSlot& slot = items->at(index);
	int price = game.GetItemPrice(slot.item, *game.pc->unit, true) * count;

	if(price > game.pc->unit->gold)
	{
		// gracz ma za ma³o z³ota
		GUI.SimpleDialog(Format(txNeedMoreGoldItem, price - game.pc->unit->gold, slot.item->name.c_str()), this);
	}
	else
	{
		// dŸwiêk
		if(game.sound_volume)
		{
			game.PlaySound2d(game.GetItemSound(slot.item));
			game.PlaySound2d(game.sCoins);
		}
		// usuñ z³oto
		game.pc->unit->gold -= price;
		// dodaj przedmiot graczowi
		if(!game.pc->unit->AddItem(slot.item, count, 0u))
			UpdateGrid(true);
		// usuñ przedmiot sprzedawcy
		slot.count -= count;
		if(slot.count == 0)
		{
			last_index = INDEX_INVALID;
			if(mode == INVENTORY)
				tooltip.Clear();
			items->erase(items->begin()+index);
			UpdateGrid(false);
		}
		else
			FormatBox();
		// komunikat
		if(!game.IsLocal())
		{
			NetChange& c = Add1(game.net_changes);
			c.type = NetChange::GET_ITEM;
			c.id = index;
			c.ile = count;
		}
	}
}

//=================================================================================================
void Inventory::SellItem(int index, uint count)
{
	ItemSlot& slot = items->at(index);
	uint team_count = min(count, slot.team_count);
	uint normal_count = count - team_count;

	// dŸwiêk
	if(game.sound_volume)
	{
		game.PlaySound2d(game.GetItemSound(slot.item));
		game.PlaySound2d(game.sCoins);
	}
	// dodaj z³oto
	if(game.IsLocal())
	{
		int price = game.GetItemPrice(slot.item, *game.pc->unit, false);
		if(team_count)
			game.AddGold(price * team_count);
		if(normal_count)
			unit->gold += price * normal_count;
	}
	// dodaj przedmiot kupcowi
	if(!InsertItem(*unit->player->chest_trade, slot.item, count, team_count))
		UpdateGrid(false);
	// usuñ przedmiot graczowi
	unit->weight -= slot.item->weight*count;
	slot.count -= count;
	if(slot.count == 0)
	{
		last_index = INDEX_INVALID;
		if(mode == INVENTORY)
			tooltip.Clear();
		items->erase(items->begin()+index);
		UpdateGrid(true);
	}
	else
	{
		FormatBox();
		slot.team_count -= team_count;
	}
	// komunikat
	if(!game.IsLocal())
	{
		NetChange& c = Add1(game.net_changes);
		c.type = NetChange::PUT_ITEM;
		c.id = index;
		c.ile = count;
	}
}

//=================================================================================================
void Inventory::SellSlotItem(ITEM_SLOT slot)
{
	const Item* item = slots[slot];

	// dŸwiêk
	if(game.sound_volume)
	{
		game.PlaySound2d(game.GetItemSound(item));
		game.PlaySound2d(game.sCoins);
	}
	// dodaj z³oto
	unit->gold += game.GetItemPrice(item, *game.pc->unit, false);
	// dodaj przedmiot kupcowi
	InsertItem(*unit->player->chest_trade, item, 1, 0);
	UpdateGrid(false);
	// usuñ przedmiot graczowi
	slots[slot] = nullptr;
	unit->weight -= item->weight;
	UpdateGrid(true);
	// komunikat
	if(game.IsOnline())
	{
		NetChange& c = Add1(game.net_changes);
		if(game.IsServer())
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
void Inventory::OnPutGold(int id)
{
	if(id == BUTTON_OK && counter > 0 && unit->gold >= counter)
	{
		if(counter > game.pc->unit->gold)
		{
			GUI.SimpleDialog(txDropNoGold, this);
			return;
		}
		// dodaj z³oto
		if(!InsertItem(*unit->player->chest_trade, game.gold_item_ptr, counter, 0))
			UpdateGrid(false);
		// usuñ
		unit->gold -= counter;
		// dŸwiêk
		if(game.sound_volume)
			game.PlaySound2d(game.sCoins);
		if(!game.IsLocal())
		{
			NetChange& c = Add1(game.net_changes);
			c.type = NetChange::PUT_GOLD;
			c.ile = counter;
		}
	}
}

//=================================================================================================
void Inventory::OnLootItem(int id)
{
	assert(lock_id == LOCK_TRADE_OTHER);
	lock_id = LOCK_NO;
	if(id == BUTTON_OK && counter > 0 && lock_index != LOCK_REMOVED)
		LootItem(lock_index, counter);
}

//=================================================================================================
void Inventory::LootItem(int index, uint count)
{
	ItemSlot& slot = items->at(index);
	uint team_count = min(count, slot.team_count);
	// dŸwiêk
	if(game.sound_volume)
		game.PlaySound2d(game.GetItemSound(slot.item));
	// dodaj
	if(!game.pc->unit->AddItem(slot.item, count, team_count))
		UpdateGrid(true);
	// usuñ
	if(game.inventory_mode == I_LOOT_BODY)
	{
		unit->weight -= slot.item->weight*count;
		if(slot.item == unit->used_item)
		{
			unit->used_item = nullptr;
			if(game.IsServer())
			{
				NetChange& c = Add1(game.net_changes);
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
			tooltip.Clear();
		items->erase(items->begin()+index);
		UpdateGrid(false);
	}
	else
	{
		FormatBox();
		slot.team_count -= team_count;
	}
	// komunikat
	if(!game.IsLocal())
	{
		NetChange& c = Add1(game.net_changes);
		c.type = NetChange::GET_ITEM;
		c.id = index;
		c.ile = count;
	}
}

//=================================================================================================
void Inventory::OnPutItem(int id)
{
	assert(lock_id == LOCK_TRADE_MY);
	lock_id = LOCK_NO;
	if(id == BUTTON_OK && counter > 0 && lock_index != LOCK_REMOVED)
		PutItem(lock_index, counter);
}

//=================================================================================================
void Inventory::PutItem(int index, uint count)
{
	ItemSlot& slot = items->at(index);
	uint team_count = min(count, slot.team_count);
	// dŸwiêk
	if(game.sound_volume)
		game.PlaySound2d(game.GetItemSound(slot.item));
	// dodaj
	if(game.inventory_mode == I_LOOT_BODY)
	{
		if(!unit->player->action_unit->AddItem(slot.item, count, team_count))
			UpdateGrid(false);
	}
	else
	{
		if(!unit->player->action_chest->AddItem(slot.item, count, team_count))
			UpdateGrid(false);
	}
	// usuñ
	unit->weight -= slot.item->weight*count;
	slot.count -= count;
	if(slot.count == 0)
	{
		last_index = INDEX_INVALID;
		if(mode == INVENTORY)
			tooltip.Clear();
		items->erase(items->begin()+index);
		UpdateGrid(true);
	}
	else
	{
		FormatBox();
		slot.team_count -= team_count;
	}
	// komunikat
	if(!game.IsLocal())
	{
		NetChange& c = Add1(game.net_changes);
		c.type = NetChange::PUT_ITEM;
		c.id = index;
		c.ile = count;
	}
}

//=================================================================================================
void Inventory::PutSlotItem(ITEM_SLOT slot)
{
	const Item* item = slots[slot];
	last_index = INDEX_INVALID;
	if(mode == INVENTORY)
		tooltip.Clear();
	// dŸwiêk
	if(game.sound_volume)
		game.PlaySound2d(game.GetItemSound(item));
	// dodaj
	if(game.inventory_mode == I_LOOT_BODY)
		unit->player->action_unit->AddItem(item, 1u, 0u);
	else
		unit->player->action_chest->AddItem(item, 1u, 0u);
	UpdateGrid(false);
	// usuñ
	slots[slot] = nullptr;
	UpdateGrid(true);
	unit->weight -= item->weight;
	// komunikat
	if(game.IsOnline())
	{
		NetChange& c = Add1(game.net_changes);
		if(game.IsServer())
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
void Inventory::OnGiveGold(int id)
{
	if(id == BUTTON_OK && counter > 0)
	{
		if(counter > game.pc->unit->gold)
		{
			GUI.SimpleDialog(txDropNoGold, this);
			return;
		}

		game.pc->unit->gold -= counter;
		if(game.sound_volume)
			game.PlaySound2d(game.sCoins);
		Unit* u = game.pc->action_unit;
		if(game.IsLocal())
		{
			u->gold += counter;
			if(u->IsPlayer() && u->player != game.pc)
			{
				NetChangePlayer& c = Add1(game.net_changes_player);
				c.type = NetChangePlayer::GOLD_RECEIVED;
				c.pc = u->player;
				c.id = game.pc->id;
				c.ile = counter;
				game.GetPlayerInfo(u->player).NeedUpdateAndGold();
			}
		}
		else
		{
			NetChange& c = Add1(game.net_changes);
			c.type = NetChange::GIVE_GOLD;
			c.id = u->netid;
			c.ile = counter;
		}
	}
}

//=================================================================================================
void Inventory::OnShareGiveItem(int id)
{
	assert(lock_id == LOCK_TRADE_MY);
	lock_id = LOCK_NO;
	if(id == BUTTON_OK && counter > 0 && lock_index != LOCK_REMOVED)
	{
		Unit* t = unit->player->action_unit;
		if(t->CanTake(items->at(lock_index).item, counter))
			ShareGiveItem(lock_index, counter);
		else
			GUI.SimpleDialog(Format(txNpcCantCarry, t->GetName()), this);
	}
}

//=================================================================================================
void Inventory::OnShareTakeItem(int id)
{
	assert(lock_id == LOCK_TRADE_OTHER);
	lock_id = LOCK_NO;
	if(id == BUTTON_OK && counter > 0 && lock_index != LOCK_REMOVED)
		ShareTakeItem(lock_index, counter);
}

//=================================================================================================
void Inventory::ShareGiveItem(int index, uint count)
{
	ItemSlot& slot = items->at(index);
	const Item* item = slot.item;
	uint team_count = min(count, slot.team_count);
	// dŸwiêk
	if(game.sound_volume)
		game.PlaySound2d(game.GetItemSound(slot.item));
	// dodaj
	if(!unit->player->action_unit->AddItem(slot.item, count, team_count))
		UpdateGrid(false);
	// usuñ
	unit->weight -= slot.item->weight*count;
	slot.count -= count;
	if(slot.count == 0)
	{
		last_index = INDEX_INVALID;
		if(mode == INVENTORY)
			tooltip.Clear();
		items->erase(items->begin()+index);
		UpdateGrid(true);
	}
	else
	{
		FormatBox();
		slot.team_count -= team_count;
	}
	// komunikat
	if(!game.IsLocal())
	{
		NetChange& c = Add1(game.net_changes);
		c.type = NetChange::PUT_ITEM;
		c.id = index;
		c.ile = count;
	}
	else if(item->type == IT_CONSUMEABLE && item->ToConsumeable().effect == E_HEAL)
		unit->player->action_unit->ai->have_potion = 2;
}

//=================================================================================================
void Inventory::ShareTakeItem(int index, uint count)
{
	ItemSlot& slot = items->at(index);
	const Item* item = slot.item;
	uint team_count = min(count, slot.team_count);
	// dŸwiêk
	if(game.sound_volume)
		game.PlaySound2d(game.GetItemSound(slot.item));
	// dodaj
	if(!game.pc->unit->AddItem(slot.item, count, team_count))
		UpdateGrid(true);
	// usuñ
	unit->weight -= slot.item->weight*count;
	slot.count -= count;
	if(slot.count == 0)
	{
		last_index = INDEX_INVALID;
		if(mode == INVENTORY)
			tooltip.Clear();
		items->erase(items->begin()+index);
		UpdateGrid(false);
	}
	else
	{
		FormatBox();
		slot.team_count -= team_count;
	}
	// komunikat
	if(!game.IsLocal())
	{
		NetChange& c = Add1(game.net_changes);
		c.type = NetChange::GET_ITEM;
		c.id = index;
		c.ile = count;
	}
	else if(item->type == IT_CONSUMEABLE && item->ToConsumeable().effect == E_HEAL)
		unit->ai->have_potion = 1;
}

//=================================================================================================
void Inventory::OnGiveItem(int id)
{
	assert(lock_id == LOCK_TRADE_MY);
	lock_id = LOCK_NO;
	if(id != BUTTON_YES || lock_index == LOCK_REMOVED)
		return;

	last_index = INDEX_INVALID;
	if(mode == INVENTORY)
		tooltip.Clear();
	const Item* item;
	ItemSlot* slot;
	ITEM_SLOT slot_type;

	if(lock_index >= 0)
	{
		slot = &items->at(lock_index);
		slot_type = SLOT_INVALID;
		item = slot->item;
	}
	else
	{
		slot = nullptr;
		slot_type = IIndexToSlot(lock_index);
		item = slots[slot_type];
	}

	// dodaj
	Unit* t = unit->player->action_unit;
	int price = game.GetItemPrice(item, *game.pc->unit, false);
	switch(give_item_mode)
	{
	case 0: // kredyt
		t->hero->credit += price;
		if(game.IsLocal())
			game.CheckCredit(true);
		break;
	case 1: // z³oto
		if(t->gold < price)
			return;
		t->gold -= price;
		unit->gold += price;
		if(game.sound_volume)
			game.PlaySound2d(game.sCoins);
		break;
	case 2: // darmo
		break;
	}
	t->AddItem(item, 1u, 0u);
	UpdateGrid(false);
	// dŸwiêk
	if(game.sound_volume)
		game.PlaySound2d(game.GetItemSound(item));
	// usuñ
	unit->weight -=item->weight;
	if(slot)
		items->erase(items->begin()+lock_index);
	else
	{
		slots[slot_type] = nullptr;
		if(game.IsServer())
		{
			NetChange& c = Add1(game.net_changes);
			c.type = NetChange::CHANGE_EQUIPMENT;
			c.unit = unit;
			c.id = slot_type;
		}
	}
	UpdateGrid(true);
	// ustaw przedmioty
	if(game.IsLocal())
	{
		game.UpdateUnitInventory(*t);
		game.BuildTmpInventory(1);
	}
	// komunikat
	if(!game.IsLocal())
	{
		NetChange& c = Add1(game.net_changes);
		c.type = NetChange::PUT_ITEM;
		c.id = lock_index;
		c.ile = 1;
	}
}

//=================================================================================================
void Inventory::OnGivePotion(int id)
{
	assert(lock_id == LOCK_TRADE_MY);
	lock_id = LOCK_NO;
	if(id == BUTTON_OK && counter > 0 && lock_index != LOCK_REMOVED)
	{
		Unit* t = unit->player->action_unit;
		if(t->CanTake(items->at(lock_index).item, counter))
			GivePotion(lock_index, counter);
		else
			GUI.SimpleDialog(Format(txNpcCantCarry, t->GetName()), this);
	}
}

//=================================================================================================
void Inventory::GivePotion(int index, uint count)
{
	ItemSlot& slot = items->at(index);
	uint team_count = min(count, slot.team_count);
	// dŸwiêk
	if(game.sound_volume)
		game.PlaySound2d(game.GetItemSound(slot.item));
	// dodaj
	if(!unit->player->action_unit->AddItem(slot.item, count, 0u))
		UpdateGrid(false);
	// usuñ
	unit->weight -= slot.item->weight*count;
	slot.count -= count;
	if(slot.count == 0)
	{
		last_index = INDEX_INVALID;
		if(mode == INVENTORY)
			tooltip.Clear();
		items->erase(items->begin()+index);
		UpdateGrid(true);
	}
	else
	{
		FormatBox();
		slot.team_count -= team_count;
	}
	// komunikat
	if(!game.IsLocal())
	{
		NetChange& c = Add1(game.net_changes);
		c.type = NetChange::PUT_ITEM;
		c.id = index;
		c.ile = count;
	}
	else
		unit->player->action_unit->ai->have_potion = 2;
}

//=================================================================================================
void Inventory::GiveSlotItem(ITEM_SLOT slot)
{
	last_index = INDEX_INVALID;
	if(mode == INVENTORY)
		tooltip.Clear();
	DialogInfo info;
	const Item* item = slots[slot];
	if(unit->player->action_unit->gold >= item->value/2)
	{
		// odkup przedmiot
		info.text = Format(txSellItem, item->value/2);
		give_item_mode = 1;
	}
	else
	{
		// zaproponuj inn¹ cenê
		info.text = Format(txSellFreeItem, item->value/2);
		give_item_mode = 2;
	}

	assert(lock_id == LOCK_NO);
	lock_id = LOCK_TRADE_MY;
	lock_index = SlotToIIndex(slot);
	info.event = fastdelegate::FastDelegate1<int>(this, &Inventory::OnGiveItem);
	info.order = ORDER_NORMAL;
	info.parent = this;
	info.type = DIALOG_YESNO;
	info.pause = false;
	GUI.ShowDialog(info);
}

//=================================================================================================
void Inventory::IsBetterItemResponse(bool is_better)
{
	if(lock_id == LOCK_TRADE_MY && lock_give)
	{
		lock_id = LOCK_NO;
		lock_give = false;

		if(lock_index == LOCK_REMOVED)
			WARN(Format("Inventory::IsBetterItem, item removed (%d).", lock_index));
		else if(game.pc->action != PlayerController::Action_GiveItems)
			WARN("Inventory::IsBetterItem, no longer giving items.");
		else if(!is_better)
			GUI.SimpleDialog(txWontTakeItem, this);
		else
		{
			Unit* t = game.pc->action_unit;
			if(lock_index >= 0)
			{
				// przedmiot w ekwipunku
				if(lock_index >= (int)unit->items.size())
					ERROR(Format("Inventory::IsBetterItem, invalid item index %d.", lock_index));
				else
				{
					ItemSlot& slot = unit->items[lock_index];
					if(!t->CanTake(slot.item))
						GUI.SimpleDialog(Format(txNpcCantCarry, t->GetName()), this);
					else
					{
						DialogInfo info;
						if(slot.team_count > 0)
						{
							// daj dru¿ynowy przedmiot
							info.text = Format(txSellTeamItem, slot.item->value/2);
							give_item_mode = 0;
						}
						else if(t->gold >= slot.item->value/2)
						{
							// odkup przedmiot
							info.text = Format(txSellItem, slot.item->value/2);
							give_item_mode = 1;
						}
						else
						{
							// zaproponuj inn¹ cenê
							info.text = Format(txSellFreeItem, slot.item->value/2);
							give_item_mode = 2;
						}
						assert(lock_id == LOCK_NO);
						lock_id = LOCK_TRADE_MY;
						info.event = fastdelegate::FastDelegate1<int>(this, &Inventory::OnGiveItem);
						info.order = ORDER_NORMAL;
						info.parent = this;
						info.type = DIALOG_YESNO;
						info.pause = false;
						GUI.ShowDialog(info);
					}
				}
			}
			else
			{
				// za³o¿ony przedmiot
				ITEM_SLOT slot_type = IIndexToSlot(lock_index);
				if(slot_type == SLOT_INVALID)
					ERROR(Format("Inventory::IsBetterItem, invalid slot index %d.", lock_index));
				else
				{
					const Item*& item = unit->slots[slot_type];
					if(!item)
						ERROR(Format("Inventory::IsBetterItem, missing slot item %d.", slot_type));
					else if(!t->CanTake(item))
						GUI.SimpleDialog(Format(txNpcCantCarry, t->GetName()), this);
					else if(SlotRequireHideWeapon(slot_type))
					{
						// ma wyjêty ten przedmiot, schowaj go a potem daj
						assert(lock_id == LOCK_NO);
						lock_id = LOCK_TRADE_MY;
						unit->HideWeapon();
						unit->player->next_action = NA_GIVE;
					}
					else
						GiveSlotItem(slot_type);
				}
			}
		}
	}
	else
		ERROR(Format("Inventory::IsBetterItemResponse, inventory not locked (%d) or no give lock (%d).", lock_id, lock_give ? 1 : 0));
}

//=================================================================================================
void Inventory::Show()
{
	game.BuildTmpInventory(0);
	visible = true;
	Event(GuiEvent_Show);
	GainFocus();
}

//=================================================================================================
void Inventory::Hide()
{
	LostFocus();
	visible = false;
}

//=================================================================================================
void Inventory::UpdateGrid(bool mine)
{
	if(mine)
	{
		game.BuildTmpInventory(0);
		game.game_gui->inv_trade_mine->UpdateScrollbar();
	}
	else
	{
		game.BuildTmpInventory(1);
		game.game_gui->inv_trade_other->UpdateScrollbar();
	}
}
