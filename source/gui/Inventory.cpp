#include "Pch.h"
#include "Inventory.h"

#include "AIController.h"
#include "BookPanel.h"
#include "Chest.h"
#include "Game.h"
#include "GameGui.h"
#include "GameResources.h"
#include "Item.h"
#include "ItemHelper.h"
#include "Language.h"
#include "LevelGui.h"
#include "PlayerInfo.h"
#include "Quest2.h"
#include "Team.h"
#include "Unit.h"

#include <GetNumberDialog.h>
#include <RenderTarget.h>
#include <ResourceManager.h>
#include <SoundManager.h>

/* REMARKS ON SOME VARIABLES
iIndex for positive values is index to items [0, 1, 2, 3...]
negative values are for equipped items [-1, -2, -3...]
*/

//-----------------------------------------------------------------------------
const int INDEX_GOLD = -2;
const int INDEX_CARRY = -3;

//=================================================================================================
Inventory::~Inventory()
{
	delete invMine;
	delete invTradeMine;
	delete invTradeOther;
	delete gpTrade;
}

//=================================================================================================
void Inventory::InitOnce()
{
	invMine = new InventoryPanel(*this);
	invMine->InitTooltip();
	invMine->mode = InventoryPanel::INVENTORY;
	invMine->visible = false;

	gpTrade = new GamePanelContainer;

	invTradeMine = new InventoryPanel(*this);
	invTradeMine->focus = true;
	gpTrade->Add(invTradeMine);

	invTradeOther = new InventoryPanel(*this);
	gpTrade->Add(invTradeOther);

	gpTrade->Hide();
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

	invMine->bt.text = txTakeAll;
	invMine->title = txInventory;
	invTradeMine->bt.text = txTakeAll;
	invTradeMine->title = txInventory;
	invTradeOther->bt.text = txTakeAll;
	invTradeOther->title = txInventory;
}

//=================================================================================================
void Inventory::LoadData()
{
	tItemBar = resMgr->Load<Texture>("item_bar.png");
	tEquipped = resMgr->Load<Texture>("equipped.png");
	tGold = resMgr->Load<Texture>("coins.png");
	tStarHq = resMgr->Load<Texture>("star_hq.png");
	tStarM = resMgr->Load<Texture>("star_m.png");
	tStarU = resMgr->Load<Texture>("star_u.png");
	tTeamItem = resMgr->Load<Texture>("team_item.png");
}

//=================================================================================================
void Inventory::Setup(PlayerController* pc)
{
	invTradeMine->indices = invMine->indices = &tmpInventory[0];
	invTradeMine->items = invMine->items = &pc->unit->items;
	invTradeMine->equipped = invMine->equipped = &pc->unit->GetEquippedItems();
	invTradeMine->unit = invMine->unit = pc->unit;
	invTradeOther->indices = &tmpInventory[1];
}

//=================================================================================================
void Inventory::StartTrade(InventoryMode mode, Unit& unit)
{
	gameGui->levelGui->ClosePanels();
	this->mode = mode;
	PlayerController* pc = game->pc;

	invTradeOther->unit = &unit;
	invTradeOther->items = &unit.items;
	invTradeOther->equipped = &unit.GetEquippedItems();

	switch(mode)
	{
	case I_LOOT_BODY:
		invTradeMine->mode = InventoryPanel::LOOT_MY;
		invTradeOther->mode = InventoryPanel::LOOT_OTHER;
		invTradeOther->title = Format("%s - %s", txLooting, unit.GetName());
		break;
	case I_SHARE:
		invTradeMine->mode = InventoryPanel::SHARE_MY;
		invTradeOther->mode = InventoryPanel::SHARE_OTHER;
		invTradeOther->title = Format("%s - %s", txShareItems, unit.GetName());
		pc->action = PlayerAction::ShareItems;
		pc->actionUnit = &unit;
		pc->chestTrade = &unit.items;
		break;
	case I_GIVE:
		invTradeMine->mode = InventoryPanel::GIVE_MY;
		invTradeOther->mode = InventoryPanel::GIVE_OTHER;
		invTradeOther->title = Format("%s - %s", txGiveItems, unit.GetName());
		pc->action = PlayerAction::GiveItems;
		pc->actionUnit = &unit;
		pc->chestTrade = &unit.items;
		break;
	default:
		assert(0);
		break;
	}

	BuildTmpInventory(0);
	BuildTmpInventory(1);
	gpTrade->Show();
}

//=================================================================================================
void Inventory::StartTrade(InventoryMode mode, vector<ItemSlot>& items, Unit* unit)
{
	PlayerController* pc = game->pc;
	gameGui->levelGui->ClosePanels();
	this->mode = mode;

	invTradeOther->items = &items;
	invTradeOther->equipped = nullptr;

	switch(mode)
	{
	case I_LOOT_CHEST:
		invTradeMine->mode = InventoryPanel::LOOT_MY;
		invTradeOther->mode = InventoryPanel::LOOT_OTHER;
		invTradeOther->unit = nullptr;
		invTradeOther->title = txLootingChest;
		break;
	case I_TRADE:
		assert(unit);
		invTradeMine->mode = InventoryPanel::TRADE_MY;
		invTradeOther->mode = InventoryPanel::TRADE_OTHER;
		invTradeOther->unit = unit;
		invTradeOther->title = Format("%s - %s", txTrading, unit->GetName());
		pc->action = PlayerAction::Trade;
		pc->actionUnit = unit;
		pc->chestTrade = &items;
		break;
	case I_LOOT_CONTAINER:
		invTradeMine->mode = InventoryPanel::LOOT_MY;
		invTradeOther->mode = InventoryPanel::LOOT_OTHER;
		invTradeOther->unit = nullptr;
		invTradeOther->title = Format("%s - %s", txLooting, pc->actionUsable->base->name.c_str());
		break;
	default:
		assert(0);
		break;
	}

	BuildTmpInventory(0);
	BuildTmpInventory(1);
	gpTrade->Show();
}

//=================================================================================================
void Inventory::StartTrade2(InventoryMode mode, void* ptr)
{
	PlayerController* pc = game->pc;
	gameGui->levelGui->ClosePanels();
	this->mode = mode;

	switch(mode)
	{
	case I_LOOT_CHEST:
		{
			Chest* chest = (Chest*)ptr;
			pc->action = PlayerAction::LootChest;
			pc->actionChest = chest;
			pc->chestTrade = &pc->actionChest->items;
			invTradeMine->mode = InventoryPanel::LOOT_MY;
			invTradeOther->unit = nullptr;
			invTradeOther->items = &chest->items;
			invTradeOther->equipped = nullptr;
			invTradeOther->title = txLootingChest;
			invTradeOther->mode = InventoryPanel::LOOT_OTHER;
		}
		break;
	case I_LOOT_CONTAINER:
		{
			Usable* usable = (Usable*)ptr;
			pc->action = PlayerAction::LootContainer;
			pc->actionUsable = usable;
			pc->chestTrade = &pc->actionUsable->container->items;
			invTradeMine->mode = InventoryPanel::LOOT_MY;
			invTradeOther->unit = nullptr;
			invTradeOther->items = &pc->actionUsable->container->items;
			invTradeOther->equipped = nullptr;
			invTradeOther->title = Format("%s - %s", txLooting, usable->base->name.c_str());
			invTradeOther->mode = InventoryPanel::LOOT_OTHER;
		}
		break;
	}

	BuildTmpInventory(0);
	BuildTmpInventory(1);
	gpTrade->Show();
}

//=================================================================================================
// build temporary inventory that contain normal & equipped items
void Inventory::BuildTmpInventory(int index)
{
	assert(index == 0 || index == 1);

	PlayerController* pc = game->pc;
	vector<int>& ids = tmpInventory[index];
	const array<const Item*, SLOT_MAX>* equipped;
	vector<ItemSlot>* items;
	int& shift = tmpInventoryShift[index];
	shift = 0;

	if(index == 0)
	{
		// player items
		equipped = &pc->unit->GetEquippedItems();
		items = &pc->unit->items;
	}
	else
	{
		// other unit or chest items
		if(pc->action == PlayerAction::LootChest
			|| pc->action == PlayerAction::Trade
			|| pc->action == PlayerAction::LootContainer)
			equipped = nullptr;
		else
			equipped = &pc->actionUnit->GetEquippedItems();
		items = pc->chestTrade;
	}

	ids.clear();

	// add equipped items if unit
	if(equipped)
	{
		for(int i = 0; i < SLOT_MAX; ++i)
		{
			if(equipped->at(i))
			{
				ids.push_back(SlotToIIndex((ITEM_SLOT)i));
				++shift;
			}
		}
	}

	// add normal items
	for(int i = 0, count = (int)items->size(); i < count; ++i)
		ids.push_back(i);
}

//=================================================================================================
void Inventory::EndLock()
{
	if(!lock)
		return;
	lock = nullptr;
	if(lockDialog)
		lockDialog->CloseDialog();
}

//=================================================================================================
InventoryPanel::InventoryPanel(Inventory& base) : base(base), lastItem(nullptr), indices(nullptr), forUnit(false), texReplaced(false)
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
	base.tooltip.Init(TooltipController::Callback(this, &InventoryPanel::GetTooltip));
}

//=================================================================================================
void InventoryPanel::Draw()
{
	GamePanel::Draw();

	int cells_w = (size.x - 48) / 63;
	int cells_h = (size.y - 64 - 34) / 63;
	int shift_x = pos.x + 12 + (size.x - 48) % 63 / 2;
	int shift_y = pos.y + 48 + (size.y - 64 - 34) % 63 / 2;

	// scrollbar
	scrollbar.Draw();

	// place for gold & carry capacity
	int bar_size = (cells_w * 63 - 8) / 2;
	int bar_y = shift_y + cells_h * 63 + 8;
	float load = 0.f;
	if(mode != TRADE_OTHER && mode != LOOT_OTHER)
	{
		load = unit->GetLoad();
		gui->DrawItem(base.tItemBar, Int2(shift_x, bar_y), Int2(bar_size, 32), Color::White, 12);
		gui->DrawItem(base.tEquipped, Int2(shift_x + bar_size + 10, bar_y), Int2(int(min(1.f, load) * bar_size), 32), Color::White, 12);
		gui->DrawItem(base.tItemBar, Int2(shift_x + bar_size + 10, bar_y), Int2(bar_size, 32), Color::White, 12);
	}
	else if(mode == LOOT_OTHER)
		bt.Draw();

	// title
	Rect rect = {
		pos.x,
		pos.y + 10,
		pos.x + size.x,
		pos.y + size.y
	};
	gui->DrawText(GameGui::fontBig, title, DTF_CENTER | DTF_SINGLELINE, Color::Black, rect, &rect);

	if(mode != TRADE_OTHER && mode != LOOT_OTHER)
	{
		// gold icon
		gui->DrawSprite(base.tGold, Int2(shift_x, bar_y));

		// gold value
		rect = Rect::Create(Int2(shift_x, bar_y), Int2(bar_size, 32));
		gui->DrawText(GameGui::font, Format("%d", unit->gold), DTF_CENTER | DTF_VCENTER, Color::Black, rect);

		// carry capacity
		rect.Left() = shift_x + bar_size + 10;
		rect.Right() = rect.Left() + bar_size;
		cstring weight_str = Format(base.txCarryShort, float(unit->weight) / 10, float(unit->weightMax) / 10);
		int w = GameGui::font->LineWidth(weight_str);
		gui->DrawText(GameGui::font, (w > bar_size ? Format("%g/%g", float(unit->weight) / 10, float(unit->weightMax) / 10) : weight_str),
			DTF_CENTER | DTF_VCENTER, (load > 1.f ? Color::Red : Color::Black), rect);
	}

	// draw tiles
	for(int y = 0; y < cells_h; ++y)
	{
		for(int x = 0; x < cells_w; ++x)
			gui->DrawSprite(base.tItemBar, Int2(shift_x + x * 63, shift_y + y * 63));
	}

	// draw items
	bool have_team = team->GetActiveTeamSize() > 1 && mode != TRADE_OTHER;
	int shift = int(scrollbar.offset / 63) * cells_w;
	for(int i = 0, cells = min(cells_w * cells_h, (int)indices->size() - shift); i < cells; ++i)
	{
		int i_item = indices->at(i + shift);
		const Item* item;
		int count, isTeam;
		if(i_item < 0)
		{
			item = equipped->at(-i_item - 1);
			count = 1;
			isTeam = (mode == LOOT_OTHER ? 2 : 0);
		}
		else
		{
			ItemSlot& slot = items->at(i_item);
			item = slot.item;
			count = slot.count;
			if(slot.count == slot.teamCount)
				isTeam = 2;
			else if(slot.teamCount == 0)
				isTeam = 0;
			else
				isTeam = 1;
		}

		if(!item)
		{
			// temporary fix
			game->ReportError(11, Format("Null item in inventory (mode:%d, i_item:%d, unit:%s)", mode, i_item, unit->data->id.c_str()), true);
			continue;
		}

		int x = i % cells_w,
			y = i / cells_w;

		// equipped item effect
		if(i_item < 0)
			gui->DrawSprite(base.tEquipped, Int2(shift_x + x * 63, shift_y + y * 63));

		// item icon
		if(!item->icon)
		{
			// temporary fix
			game->ReportError(12, Format("Null item icon '%s'", item->id.c_str()));
			gameRes->GenerateItemIcon(const_cast<Item&>(*item));
		}
		gui->DrawSprite(item->icon, Int2(shift_x + x * 63, shift_y + y * 63));

		// item quality icon
		Texture* icon;
		if(IsSet(item->flags, ITEM_HQ))
			icon = base.tStarHq;
		else if(IsSet(item->flags, ITEM_MAGICAL))
			icon = base.tStarM;
		else if(IsSet(item->flags, ITEM_UNIQUE))
			icon = base.tStarU;
		else
			icon = nullptr;
		if(icon)
			gui->DrawSprite(icon, Int2(shift_x + (x + 1) * 63 - 25, shift_y + (y + 1) * 63 - 25));

		// team item icon
		if(have_team && isTeam != 0)
			gui->DrawSprite(base.tTeamItem, Int2(shift_x + x * 63 + 2, shift_y + y * 63 + 2), Color::Alpha(isTeam == 2 ? 255 : 128));

		// count
		if(count > 1)
		{
			Rect rect3 = Rect::Create(Int2(shift_x + x * 63 + 5, shift_y + y * 63 - 3), Int2(64, 63));
			gui->DrawText(GameGui::font, Format("%d", count), DTF_BOTTOM | DTF_OUTLINE, Color::White, rect3);
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

	if(gameGui->book->visible)
	{
		dragAndDrop = false;
		base.tooltip.Clear();
		return;
	}

	if(mode == INVENTORY && input->Focus() && input->PressedRelease(Key::Escape))
	{
		Hide();
		return;
	}

	if(base.lock && base.lock.isGive)
	{
		base.lock.timer -= dt;
		if(base.lock.timer <= 0.f)
		{
			Error("IS_BETTER_ITEM timed out.");
			base.lock = nullptr;
		}
	}

	int cells_w = (size.x - 48) / 63;
	int cells_h = (size.y - 64 - 34) / 63;
	int shift_x = pos.x + 12 + (size.x - 48) % 63 / 2;
	int shift_y = pos.y + 48 + (size.y - 64 - 34) % 63 / 2;

	int new_index = INDEX_INVALID;

	Int2 cursorPos = gui->cursorPos;

	bool have_focus = (mode == INVENTORY ? focus : mouseFocus);

	if(have_focus && input->Focus() && IsInside(gui->cursorPos))
		scrollbar.ApplyMouseWheel();
	if(focus)
	{
		scrollbar.mouseFocus = have_focus;
		scrollbar.Update(dt);
	}

	int shift = int(scrollbar.offset / 63) * cells_w;

	if(lastIndex >= 0)
	{
		if(lastIndex >= (int)indices->size())
		{
			lastIndex = INDEX_INVALID;
			lastItem = nullptr;
		}
		else
		{
			int iIndex = indices->at(lastIndex);
			const Item* item;
			if(iIndex < 0)
				item = equipped->at(IIndexToSlot(iIndex));
			else
				item = items->at(iIndex).item;
			if(item != lastItem)
			{
				lastIndex = INDEX_INVALID;
				lastItem = nullptr;
			}
		}
	}

	if(have_focus && !gameGui->levelGui->IsDragAndDrop())
	{
		// item
		if(cursorPos.x >= shift_x && cursorPos.y >= shift_y)
		{
			int x = (cursorPos.x - shift_x) / 63,
				y = (cursorPos.y - shift_y) / 63;
			if(x >= 0 && x < cells_w && y >= 0 && y < cells_h)
			{
				int i = x + y * cells_w;
				if(i < (int)indices->size() - shift)
					new_index = i + shift;
			}
		}

		int bar_size = (cells_w * 63 - 8) / 2;
		int bar_y = shift_y + cells_h * 63 + 8;

		// bar with gold or carry capacity
		if(mode != TRADE_OTHER && mode != LOOT_OTHER)
		{
			if(cursorPos.y >= bar_y && cursorPos.y < bar_y + 32)
			{
				if(cursorPos.x >= shift_x && cursorPos.x < shift_x + bar_size)
					new_index = INDEX_GOLD;
				else if(cursorPos.x >= shift_x + bar_size + 10 && cursorPos.x < shift_x + bar_size + 10 + bar_size)
					new_index = INDEX_CARRY;
			}
		}

		// take all button
		if(mode == LOOT_OTHER)
		{
			bt.mouseFocus = focus;
			bt.Update(dt);
		}
	}

	// handle button to take all
	if(mode == LOOT_OTHER && (focus || base.invTradeMine->focus) && input->Focus() && GKey.PressedRelease(GK_TAKE_ALL))
	{
		Event(GuiEvent_Custom);
		return;
	}

	// update box
	if(mode == INVENTORY)
		base.tooltip.UpdateTooltip(dt, new_index, -1);
	else
	{
		bool old_for_unit = forUnit;
		if(AllowForUnit())
			forUnit = input->Down(Key::Shift);
		else
			forUnit = false;
		UpdateBoxIndex(dt, new_index, -1, old_for_unit != forUnit);
		if(boxState == BOX_NOT_VISIBLE)
			itemVisible = nullptr;
	}
	if(itemVisible)
	{
		gameRes->DrawItemIcon(*itemVisible, game->rtItemRot, rot);
		rot += PI * dt / 2;
	}

	if(focus && input->Focus() && IsInside(gui->cursorPos))
	{
		for(int i = 0; i < Shortcut::MAX; ++i)
		{
			if(GKey.PressedRelease((GAME_KEYS)(GK_SHORTCUT1 + i)))
			{
				if(new_index >= 0)
				{
					const Item* item;
					int iIndex = indices->at(new_index);
					if(iIndex < 0)
						item = equipped->at(IIndexToSlot(iIndex));
					else
						item = items->at(iIndex).item;
					game->pc->SetShortcut(i, Shortcut::TYPE_ITEM, (int)item);
				}
			}
		}
	}

	if(dragAndDrop)
	{
		if(Int2::Distance(gui->cursorPos, dragAndDropPos) > 3)
		{
			gameGui->levelGui->StartDragAndDrop(Shortcut::TYPE_ITEM, (int)dragAndDropItem, dragAndDropItem->icon);
			dragAndDrop = false;
		}
	}

	if(new_index >= 0)
	{
		// get item
		const Item* item;
		ItemSlot* slot;
		ITEM_SLOT slot_type;
		int iIndex = indices->at(new_index);
		if(iIndex < 0)
		{
			slot = nullptr;
			slot_type = IIndexToSlot(iIndex);
			item = equipped->at(slot_type);
		}
		else
		{
			slot = &items->at(iIndex);
			item = slot->item;
			slot_type = SLOT_INVALID;
		}

		lastItem = item;

		if(!focus || !(game->pc->unit->action == A_NONE || game->pc->unit->CanDoWhileUsing()) || base.lock || !input->Focus() || !game->pc->unit->IsStanding())
			return;

		// handling on clicking in inventory
		if(mode == INVENTORY && input->PressedRelease(Key::RightButton) && game->pc->unit->action == A_NONE)
		{
			// drop item
			if(IsSet(item->flags, ITEM_DONT_DROP) && team->IsAnyoneTalking())
				gui->SimpleDialog(base.txCantDoNow, this);
			else
			{
				if(!slot)
				{
					if(unit->SlotRequireHideWeapon(slot_type))
					{
						// drop item after hiding it
						unit->HideWeapon();
						unit->player->nextAction = NA_DROP;
						unit->player->nextActionData.slot = slot_type;
						if(Net::IsClient())
							Net::PushChange(NetChange::SET_NEXT_ACTION);
					}
					else
						DropSlotItem(slot_type);
				}
				else
				{
					if(input->Down(Key::Shift))
					{
						// drop all
						unit->DropItem(iIndex, 0);
						lastIndex = INDEX_INVALID;
						if(mode == INVENTORY)
							base.tooltip.Clear();
						base.BuildTmpInventory(0);
						UpdateScrollbar();
					}
					else if(input->Down(Key::Control) || slot->count == 1)
					{
						// drop single
						if(unit->DropItem(iIndex))
						{
							base.BuildTmpInventory(0);
							UpdateScrollbar();
							lastIndex = INDEX_INVALID;
							if(mode == INVENTORY)
								base.tooltip.Clear();
						}
						else
							base.tooltip.Refresh();
					}
					else
					{
						// drop selected count
						counter = 1;
						base.lock.Lock(iIndex, *slot);
						base.lockDialog = GetNumberDialog::Show(this, delegate<void(int)>(this, &InventoryPanel::OnDropItem), base.txDropItemCount, 1, slot->count, &counter);
						lastIndex = INDEX_INVALID;
						if(mode == INVENTORY)
							base.tooltip.Clear();
					}
				}
			}
		}
		else if(HandleLeftClick(item))
		{
			switch(mode)
			{
			case INVENTORY:
				// use/equip/unequip item
				if(!slot)
				{
					// unequip item
					if(unit->SlotRequireHideWeapon(slot_type))
					{
						// hide weapon & add next action to unequip
						unit->HideWeapon();
						unit->player->nextAction = NA_REMOVE;
						unit->player->nextActionData.slot = slot_type;
						if(Net::IsClient())
							Net::PushChange(NetChange::SET_NEXT_ACTION);
					}
					else
						RemoveSlotItem(slot_type);
				}
				else
				{
					if(unit->CanWear(item) && slot->teamCount > 0 && team->GetActiveTeamSize() > 1)
					{
						DialogInfo di;
						di.event = delegate<void(int)>(this, &InventoryPanel::OnTakeItem);
						di.name = "takeover_item";
						di.parent = this;
						di.pause = false;
						di.text = base.txBuyTeamDialog;
						di.order = DialogOrder::Normal;
						di.type = DIALOG_YESNO;
						base.lockDialog = gui->ShowDialog(di);
						base.lock.Lock(iIndex, *slot);
						lastIndex = INDEX_INVALID;
						if(mode == INVENTORY)
							base.tooltip.Clear();
					}
					else if(item->type == IT_CONSUMABLE)
						ConsumeItem(iIndex);
					else if(item->type == IT_BOOK)
						game->pc->ReadBook(iIndex);
					else if(item->IsWearable())
					{
						ITEM_SLOT type = ItemTypeToSlot(item->type);
						if(unit->SlotRequireHideWeapon(type))
						{
							// hide equipped item & equip new one
							unit->HideWeapon();
							unit->player->nextAction = NA_EQUIP;
							unit->player->nextActionData.item = item;
							unit->player->nextActionData.index = iIndex;
							if(Net::IsClient())
								Net::PushChange(NetChange::SET_NEXT_ACTION);
						}
						else
						{
							// check if can be equipped
							bool ok = true;
							if(type == SLOT_ARMOR)
							{
								if(item->ToArmor().armorUnitType != ArmorUnitType::HUMAN)
								{
									ok = false;
									gui->SimpleDialog(base.txCantWear, this);
								}
							}

							if(ok)
								EquipSlotItem(iIndex);
						}
					}
				}
				break;
			case TRADE_MY:
				// selling items
				if(item->value <= 1 || !unit->player->actionUnit->data->trader->CanBuySell(item))
					gui->SimpleDialog(base.txWontBuy, this);
				else if(!slot)
				{
					if(unit->SlotRequireHideWeapon(slot_type))
					{
						// hide equipped item and sell it
						unit->HideWeapon();
						unit->player->nextAction = NA_SELL;
						unit->player->nextActionData.slot = slot_type;
					}
					else
						SellSlotItem(slot_type);
				}
				else
				{
					// decide how many to sell
					uint count;
					if(item->IsStackable() && slot->count != 1)
					{
						if(input->Down(Key::Shift))
							count = slot->count;
						else if(input->Down(Key::Control))
							count = 1;
						else
						{
							counter = 1;
							base.lock.Lock(iIndex, *slot);
							base.lockDialog = GetNumberDialog::Show(this, delegate<void(int)>(this, &InventoryPanel::OnSellItem), base.txSellItemCount, 1, slot->count, &counter);
							lastIndex = INDEX_INVALID;
							if(mode == INVENTORY)
								base.tooltip.Clear();
							break;
						}
					}
					else
						count = 1;

					SellItem(iIndex, count);
				}
				break;
			case TRADE_OTHER:
				// buying items
				{
					// how many player want to buy?
					uint count;
					if(item->IsStackable() && slot->count != 1)
					{
						if(input->Down(Key::Shift))
							count = slot->count;
						else if(input->Down(Key::Control))
							count = 1;
						else
						{
							counter = 1;
							base.lock.Lock(iIndex, *slot);
							base.lockDialog = GetNumberDialog::Show(this, delegate<void(int)>(this, &InventoryPanel::OnBuyItem), base.txBuyItemCount, 1, slot->count, &counter);
							lastIndex = INDEX_INVALID;
							if(mode == INVENTORY)
								base.tooltip.Clear();
							break;
						}
					}
					else
						count = 1;

					BuyItem(iIndex, count);
				}
				break;
			case LOOT_MY:
				// put item into corpse/chest/container
				if(slot)
				{
					uint count;
					if(item->IsStackable() && slot->count != 1)
					{
						if(input->Down(Key::Shift))
							count = slot->count;
						else if(input->Down(Key::Control))
							count = 1;
						else
						{
							counter = 1;
							base.lock.Lock(iIndex, *slot);
							base.lockDialog = GetNumberDialog::Show(this, delegate<void(int)>(this, &InventoryPanel::OnPutItem), base.txPutItemCount, 1, slot->count, &counter);
							lastIndex = INDEX_INVALID;
							if(mode == INVENTORY)
								base.tooltip.Clear();
							break;
						}
					}
					else
						count = 1;

					PutItem(iIndex, count);
				}
				else
				{
					if(unit->SlotRequireHideWeapon(slot_type))
					{
						// hide equipped item and put it in container
						unit->HideWeapon();
						unit->player->nextAction = NA_PUT;
						unit->player->nextActionData.slot = slot_type;
					}
					else
						PutSlotItem(slot_type);
				}
				break;
			case LOOT_OTHER:
				// take item from corpse/chest/container
				if(slot)
				{
					uint count;
					if(item->IsStackable() && slot->count != 1)
					{
						int option;
						if(item->type == IT_GOLD)
						{
							if(input->Down(Key::Control))
								option = 1;
							else if(input->Down(Key::Alt))
								option = 0;
							else
								option = 2;
						}
						else
						{
							if(input->Down(Key::Shift))
								option = 2;
							else if(input->Down(Key::Control))
								option = 1;
							else
								option = 0;
						}

						if(option == 2)
							count = slot->count;
						else if(option == 1)
							count = 1;
						else
						{
							counter = 1;
							base.lock.Lock(iIndex, *slot);
							base.lockDialog = GetNumberDialog::Show(this, delegate<void(int)>(this, &InventoryPanel::OnLootItem), base.txLootItemCount, 1, slot->count, &counter);
							lastIndex = INDEX_INVALID;
							if(mode == INVENTORY)
								base.tooltip.Clear();
							break;
						}
					}
					else
						count = 1;

					LootItem(iIndex, count);
				}
				else
				{
					lastIndex = INDEX_INVALID;
					// add to player
					if(!game->pc->unit->AddItem(item, 1u, 1u))
						UpdateGrid(true);
					// remove equipped item
					unit->RemoveEquippedItem(slot_type);
					UpdateGrid(false);
					// sound
					soundMgr->PlaySound2d(gameRes->GetItemSound(item));
					// event
					ScriptEvent e(EVENT_PICKUP);
					e.item = item;
					unit->FireEvent(e);
					// message
					if(Net::IsClient())
					{
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::GET_ITEM;
						c.id = iIndex;
						c.count = 1;
					}
				}
				break;
			case SHARE_MY:
				// give item to companion to store
				if(slot && slot->teamCount > 0)
				{
					uint count;
					if(item->IsStackable() && slot->teamCount != 1)
					{
						if(input->Down(Key::Shift))
							count = slot->teamCount;
						else if(input->Down(Key::Control))
							count = 1;
						else
						{
							counter = 1;
							base.lock.Lock(iIndex, *slot);
							base.lockDialog = GetNumberDialog::Show(this, delegate<void(int)>(this, &InventoryPanel::OnShareGiveItem),
								base.txShareGiveItemCount, 1, slot->teamCount, &counter);
							lastIndex = INDEX_INVALID;
							if(mode == INVENTORY)
								base.tooltip.Clear();
							break;
						}
					}
					else
						count = 1;

					Unit* t = unit->player->actionUnit;
					if(t->CanTake(item))
						ShareGiveItem(iIndex, count);
					else
						gui->SimpleDialog(Format(base.txNpcCantCarry, t->GetName()), this);
				}
				else
				{
					lastIndex = INDEX_INVALID;
					gui->SimpleDialog(base.txCanCarryTeamOnly, this);
				}
				break;
			case SHARE_OTHER:
			case GIVE_OTHER:
				// take item from companion
				if(slot && slot->teamCount > 0)
				{
					uint count;
					if(item->IsStackable() && slot->teamCount != 1)
					{
						if(input->Down(Key::Shift))
							count = slot->teamCount;
						else if(input->Down(Key::Control))
							count = 1;
						else
						{
							counter = 1;
							base.lock.Lock(iIndex, *slot);
							base.lockDialog = GetNumberDialog::Show(this, delegate<void(int)>(this, &InventoryPanel::OnShareTakeItem), base.txShareTakeItemCount,
								1, slot->teamCount, &counter);
							lastIndex = INDEX_INVALID;
							if(mode == INVENTORY)
								base.tooltip.Clear();
							break;
						}
					}
					else
						count = 1;

					ShareTakeItem(iIndex, count);
				}
				else
				{
					lastIndex = INDEX_INVALID;
					if(mode == INVENTORY)
						base.tooltip.Clear();
					gui->SimpleDialog(base.txWontGiveItem, this);
				}
				break;
			case GIVE_MY:
				// give item to companion
				Unit* t = unit->player->actionUnit;
				if(slot)
				{
					if(t->CanWear(item))
					{
						lastIndex = INDEX_INVALID;

						if(Net::IsLocal())
						{
							if(!t->IsBetterItem(item))
								gui->SimpleDialog(base.txWontTakeItem, this);
							else if(t->CanTake(item))
							{
								DialogInfo info;
								int price = item->value / 2;
								if(slot->teamCount > 0)
								{
									// give team item for credit
									info.text = Format(base.txSellTeamItem, price);
									giveItemMod = 0;
								}
								else if(t->gold >= price)
								{
									// sell item
									info.text = Format(base.txSellItem, price);
									giveItemMod = 1;
								}
								else
								{
									// give item for free
									info.text = Format(base.txSellFreeItem, price);
									giveItemMod = 2;
								}
								base.lock.Lock(iIndex, *slot);
								info.event = delegate<void(int)>(this, &InventoryPanel::OnGiveItem);
								info.order = DialogOrder::Normal;
								info.parent = this;
								info.type = DIALOG_YESNO;
								info.pause = false;
								base.lockDialog = gui->ShowDialog(info);
							}
							else
								gui->SimpleDialog(Format(base.txNpcCantCarry, t->GetName()), this);
						}
						else
						{
							base.lock.Lock(iIndex, *slot, true);
							base.lockDialog = nullptr;
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::IS_BETTER_ITEM;
							c.id = iIndex;
						}
					}
					else if(t->WantItem(item))
					{
						uint count;
						if(slot->count != 1)
						{
							if(input->Down(Key::Shift))
								count = slot->count;
							else if(input->Down(Key::Control))
								count = 1;
							else
							{
								counter = 1;
								base.lock.Lock(iIndex, *slot);
								base.lockDialog = GetNumberDialog::Show(this, delegate<void(int)>(this, &InventoryPanel::OnGivePotion), base.txGivePotionCount,
									1, slot->count, &counter);
								lastIndex = INDEX_INVALID;
								if(mode == INVENTORY)
									base.tooltip.Clear();
								break;
							}
						}
						else
							count = 1;

						if(t->CanTake(item, count))
							GivePotion(iIndex, count);
						else
							gui->SimpleDialog(Format(base.txNpcCantCarry, t->GetName()), this);
					}
					else
					{
						lastIndex = INDEX_INVALID;
						gui->SimpleDialog(base.txWontTakeItem, this);
					}
				}
				else
				{
					if(Net::IsLocal())
					{
						if(t->IsBetterItem(item))
						{
							if(t->CanTake(item))
							{
								if(unit->SlotRequireHideWeapon(slot_type))
								{
									// hide equipped item and give it
									unit->HideWeapon();
									unit->player->nextAction = NA_GIVE;
									unit->player->nextActionData.slot = slot_type;
								}
								else
									GiveSlotItem(slot_type);
							}
							else
								gui->SimpleDialog(Format(base.txNpcCantCarry, t->GetName()), this);
						}
						else
						{
							lastIndex = INDEX_INVALID;
							if(mode == INVENTORY)
								base.tooltip.Clear();
							gui->SimpleDialog(base.txWontTakeItem, this);
						}
					}
					else
					{
						base.lock.Lock(slot_type, true);
						base.lockDialog = nullptr;
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::IS_BETTER_ITEM;
						c.id = iIndex;
					}
				}
				break;
			}
		}
	}
	else if(new_index == INDEX_GOLD && mode != GIVE_OTHER && mode != SHARE_OTHER && !base.lock && game->pc->unit->IsStanding() && game->pc->unit->action == A_NONE)
	{
		// give/drop/put gold
		if(input->PressedRelease(Key::LeftButton))
		{
			lastIndex = INDEX_INVALID;
			if(mode == INVENTORY)
				base.tooltip.Clear();
			counter = 1;
			if(mode == INVENTORY)
				GetNumberDialog::Show(this, delegate<void(int)>(this, &InventoryPanel::OnDropGold), base.txDropGoldCount, 1, unit->gold, &counter);
			else if(mode == LOOT_MY)
				GetNumberDialog::Show(this, delegate<void(int)>(this, &InventoryPanel::OnPutGold), base.txPutGoldCount, 1, unit->gold, &counter);
			else if(mode == GIVE_MY || mode == SHARE_MY)
				GetNumberDialog::Show(this, delegate<void(int)>(this, &InventoryPanel::OnGiveGold), base.txGiveGoldCount, 1, unit->gold, &counter);
		}
	}
}

//=================================================================================================
void InventoryPanel::Event(GuiEvent e)
{
	GamePanel::Event(e);

	if(e == GuiEvent_Moved)
	{
		scrollbar.globalPos = globalPos + scrollbar.pos;
		bt.globalPos = globalPos + bt.pos;
	}
	else if(e == GuiEvent_Resize || e == GuiEvent_GainFocus || e == GuiEvent_Show)
	{
		UpdateScrollbar();
		int cells_w = (size.x - 48) / 63;
		int cells_h = (size.y - 64 - 34) / 63;
		int shift_x = 12 + (size.x - 48) % 63 / 2;
		int shift_y = 48 + (size.y - 64 - 34) % 63 / 2;
		int bar_size = (cells_w * 63 - 8) / 2;
		int bar_y = shift_y + cells_h * 63 + 8;
		bt.pos = Int2(shift_x + bar_size + 10, bar_y);
		bt.size = Int2(bar_size, 36);
		bt.globalPos = globalPos + bt.pos;
		if(e == GuiEvent_Show)
		{
			base.tooltip.Clear();
			bt.text = game->GetShortcutText(GK_TAKE_ALL);
			dragAndDrop = false;
		}
	}
	else if(e == GuiEvent_LostFocus)
		scrollbar.LostFocus();
	else if(e == GuiEvent_Custom)
	{
		if(game->pc->unit->action != A_NONE)
			return;

		// take all event
		Unit* lootedUnit = nullptr;
		SoundPtr sound[3]{};
		vector<ItemSlot>& itms = game->pc->unit->items;
		bool gold = false, changes = false, pickupEvent = false;

		// slots
		if(game->pc->action != PlayerAction::LootChest && game->pc->action != PlayerAction::LootContainer)
		{
			lootedUnit = game->pc->actionUnit;
			pickupEvent = lootedUnit->HaveEventHandler(EVENT_PICKUP);

			array<const Item*, SLOT_MAX>& equipped = lootedUnit->GetEquippedItems();
			for(int i = 0; i < SLOT_MAX; ++i)
			{
				if(!equipped[i])
					continue;

				Sound* s = gameRes->GetItemSound(equipped[i]);
				for(int i = 0; i < 3; ++i)
				{
					if(sound[i] == s)
						break;
					else if(!sound[i])
					{
						sound[i] = s;
						break;
					}
				}

				InsertItemBare(itms, equipped[i]);
				game->pc->unit->weight += equipped[i]->weight;
				changes = true;

				if(pickupEvent)
				{
					ScriptEvent e(EVENT_PICKUP);
					e.item = equipped[i];
					lootedUnit->FireEvent(e);
				}
			}

			lootedUnit->weight = 0;
			lootedUnit->RemoveAllEquippedItems();
		}

		// items
		for(vector<ItemSlot>::iterator it = game->pc->chestTrade->begin(), end = game->pc->chestTrade->end(); it != end; ++it)
		{
			if(!it->item)
				continue;

			if(it->item->type == IT_GOLD)
			{
				gold = true;
				game->pc->unit->AddItem(Item::gold, it->count, it->teamCount);
			}
			else
			{
				InsertItemBare(itms, it->item, it->count, it->teamCount);
				game->pc->unit->weight += it->item->weight * it->count;
				Sound* s = gameRes->GetItemSound(it->item);
				for(int i = 0; i < 3; ++i)
				{
					if(sound[i] == s)
						break;
					else if(!sound[i])
					{
						sound[i] = s;
						break;
					}
				}
				changes = true;

				if(pickupEvent)
				{
					ScriptEvent e(EVENT_PICKUP);
					e.item = it->item;
					lootedUnit->FireEvent(e);
				}
			}
		}
		game->pc->chestTrade->clear();

		if(Net::IsClient())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::GET_ALL_ITEMS;
		}

		// pick item sound
		for(int i = 0; i < 3; ++i)
		{
			if(sound[i])
				soundMgr->PlaySound2d(sound[i]);
		}
		if(gold)
			soundMgr->PlaySound2d(gameRes->sCoins);

		// close inventory
		if(changes)
			SortItems(itms);
		base.gpTrade->Hide();
		game->OnCloseInventory();
	}
}

//=================================================================================================
void InventoryPanel::RemoveSlotItem(ITEM_SLOT slot)
{
	const Item* item = equipped->at(slot);
	soundMgr->PlaySound2d(gameRes->GetItemSound(item));
	unit->RemoveEquippedItem(slot);
	unit->AddItem(item, 1, false);
	base.BuildTmpInventory(0);

	if(Net::IsClient())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::CHANGE_EQUIPMENT;
		c.id = SlotToIIndex(slot);
	}
}

//=================================================================================================
void InventoryPanel::DropSlotItem(ITEM_SLOT slot)
{
	soundMgr->PlaySound2d(gameRes->GetItemSound(unit->GetEquippedItem(slot)));
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
		lastIndex = INDEX_INVALID;
		if(mode == INVENTORY)
			base.tooltip.Clear();
		base.BuildTmpInventory(0);
		UpdateScrollbar();
	}
	else
		base.tooltip.Refresh();
}

//=================================================================================================
void InventoryPanel::EquipSlotItem(int index)
{
	// play sound
	const Item* item = items->at(index).item;
	soundMgr->PlaySound2d(gameRes->GetItemSound(item));

	// equip
	unit->EquipItem(index);
	base.BuildTmpInventory(0);
	lastIndex = INDEX_INVALID;
	if(mode == INVENTORY)
		base.tooltip.Clear();

	// send message
	if(Net::IsClient())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::CHANGE_EQUIPMENT;
		c.id = index;
	}
}

//=================================================================================================
void InventoryPanel::FormatBox(bool refresh)
{
	FormatBox(lastIndex, boxText, boxTextSmall, boxImg, refresh);
}

//=================================================================================================
void InventoryPanel::FormatBox(int group, string& text, string& smallText, Texture*& img, bool refresh)
{
	if(group == INDEX_GOLD)
	{
		text = Format(base.txGoldAndCredit, unit->gold, unit->IsPlayer() ? unit->player->credit : unit->hero->credit);
		switch(mode)
		{
		case INVENTORY:
			smallText = base.txGoldDropInfo;
			break;
		case TRADE_MY:
		case TRADE_OTHER:
		case LOOT_OTHER:
		case GIVE_OTHER:
		case SHARE_OTHER:
			smallText.clear();
			break;
		case LOOT_MY:
			smallText = base.txPutGold;
			break;
		case SHARE_MY:
		case GIVE_MY:
			smallText = base.txGiveGold;
			break;
		}
		img = base.tGold;
		itemVisible = nullptr;
	}
	else if(group == INDEX_CARRY)
	{
		text = Format(base.txCarry, float(unit->weight) / 10, int(unit->GetLoad() * 100), float(unit->weightMax) / 10);
		smallText = base.txCarryInfo;
		img = nullptr;
		itemVisible = nullptr;
	}
	else if(group != INDEX_INVALID)
	{
		const Item* item;
		int count, teamCount;
		int iIndex = indices->at(group);
		if(iIndex < 0)
		{
			item = equipped->at(IIndexToSlot(iIndex));
			count = 1;
			teamCount = (mode == LOOT_OTHER ? 1 : 0);
		}
		else
		{
			ItemSlot& slot = items->at(iIndex);
			item = slot.item;
			count = slot.count;
			teamCount = slot.teamCount;
		}

		Unit* target;
		if(forUnit)
			target = game->pc->actionUnit;
		else
			target = game->pc->unit;

		GetItemString(text, item, target, (uint)count);
		if(mode != TRADE_OTHER && teamCount && team->GetActiveTeamSize() > 1)
		{
			text += '\n';
			text += base.txTeamItem;
			if(count != 1)
				text += Format(" (%d)", teamCount);
		}
		if(mode == TRADE_MY)
		{
			text += '\n';
			int price = ItemHelper::GetItemPrice(item, *game->pc->unit, false);
			if(price == 0 || !unit->player->actionUnit->data->trader->CanBuySell(item))
				text += base.txWontBuy;
			else
				text += Format(base.txPrice, price);
		}
		else if(mode == TRADE_OTHER)
		{
			int price = ItemHelper::GetItemPrice(item, *game->pc->unit, true);
			text += '\n';
			text += Format(base.txPrice, price);
		}
		smallText = item->desc;
		if(AllowForUnit() && game->pc->actionUnit->CanWear(item) && !Any(item->type, IT_AMULET, IT_RING))
		{
			if(!smallText.empty())
				smallText += '\n';
			if(forUnit)
				smallText += Format(base.txStatsFor, game->pc->actionUnit->GetName());
			else
				smallText += Format(base.txShowStatsFor, game->pc->actionUnit->GetName());
		}

		if(item->mesh)
		{
			img = game->rtItemRot;
			if(!refresh)
				rot = 0.f;
			itemVisible = item;
		}
		else
		{
			img = item->icon;
			itemVisible = nullptr;
		}
	}
}

//=================================================================================================
void InventoryPanel::GetTooltip(TooltipController*, int group, int, bool refresh)
{
	if(group == INDEX_INVALID)
	{
		itemVisible = nullptr;
		base.tooltip.anything = false;
		return;
	}

	base.tooltip.anything = true;
	base.tooltip.bigText.clear();

	FormatBox(group, base.tooltip.text, base.tooltip.smallText, base.tooltip.img, refresh);
}

//=================================================================================================
void InventoryPanel::OnDropGold(int id)
{
	if(id == BUTTON_CANCEL || counter == 0)
		return;

	if(counter > unit->gold)
		gui->SimpleDialog(base.txDropNoGold, this);
	else if(!unit->CanAct())
		gui->SimpleDialog(base.txDropNotNow, this);
	else
		game->pc->unit->DropGold(counter);
}

//=================================================================================================
void InventoryPanel::OnDropItem(int id)
{
	int index = GetLockIndexAndRelease();
	if(id == BUTTON_CANCEL || counter == 0 || index == -1)
		return;

	if(!unit->CanAct())
		gui->SimpleDialog(base.txDropNotNow, this);
	else
	{
		if(unit->DropItem(index, counter))
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
	slot.teamCount = 0;

	if(Net::IsLocal())
		team->CheckCredit(true);
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
	if(!indices)
		return;

	int cells_w = (size.x - 48) / 63;
	int cells_h = (size.y - 64 - 34) / 63;
	int shift_x = pos.x + 12 + (size.x - 48) % 63 / 2;
	int shift_y = pos.y + 48 + (size.y - 64 - 34) % 63 / 2;
	int count = indices->size();
	int s = ((count + cells_w - 1) / cells_w) * 63;
	scrollbar.size = Int2(16, cells_h * 63);
	scrollbar.total = s;
	scrollbar.part = min(s, scrollbar.size.y);
	scrollbar.pos = Int2(shift_x + cells_w * 63 + 8 - pos.x, shift_y - pos.y);
	scrollbar.globalPos = globalPos + scrollbar.pos;
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
	int price = ItemHelper::GetItemPrice(slot.item, *game->pc->unit, true) * count;

	if(price > game->pc->unit->gold)
	{
		// not enough gold
		gui->SimpleDialog(Format(base.txNeedMoreGoldItem, price - game->pc->unit->gold, slot.item->name.c_str()), this);
	}
	else
	{
		// sound
		soundMgr->PlaySound2d(gameRes->GetItemSound(slot.item));
		soundMgr->PlaySound2d(gameRes->sCoins);
		// remove old
		game->pc->unit->gold -= price;
		if(Net::IsLocal())
			game->pc->Train(TrainWhat::Trade, (float)price, 0);
		// add player item
		if(!game->pc->unit->AddItem(slot.item, count, 0u))
			UpdateGrid(true);
		// remove item from seller
		slot.count -= count;
		if(slot.count == 0)
		{
			lastIndex = INDEX_INVALID;
			if(mode == INVENTORY)
				base.tooltip.Clear();
			items->erase(items->begin() + index);
			UpdateGrid(false);
		}
		else
			base.tooltip.Refresh();
		// message
		if(Net::IsClient())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::GET_ITEM;
			c.id = index;
			c.count = count;
		}
	}
}

//=================================================================================================
void InventoryPanel::SellItem(int index, uint count)
{
	ItemSlot& slot = items->at(index);
	uint teamCount = min(count, slot.teamCount);
	uint normal_count = count - teamCount;

	// sound
	soundMgr->PlaySound2d(gameRes->GetItemSound(slot.item));
	soundMgr->PlaySound2d(gameRes->sCoins);
	// add gold
	if(Net::IsLocal())
	{
		int price = ItemHelper::GetItemPrice(slot.item, *game->pc->unit, false);
		game->pc->Train(TrainWhat::Trade, (float)price, 0);
		if(teamCount)
			team->AddGold(price * teamCount);
		if(normal_count)
			unit->gold += price * normal_count;
	}
	// add item to trader
	if(!InsertItem(*unit->player->chestTrade, slot.item, count, teamCount))
		UpdateGrid(false);
	// remove item from player
	unit->weight -= slot.item->weight * count;
	slot.count -= count;
	if(slot.count == 0)
	{
		lastIndex = INDEX_INVALID;
		if(mode == INVENTORY)
			base.tooltip.Clear();
		items->erase(items->begin() + index);
		UpdateGrid(true);
	}
	else
	{
		base.tooltip.Refresh();
		slot.teamCount -= teamCount;
	}
	// message
	if(Net::IsClient())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::PUT_ITEM;
		c.id = index;
		c.count = count;
	}
}

//=================================================================================================
void InventoryPanel::SellSlotItem(ITEM_SLOT slot)
{
	const Item* item = equipped->at(slot);

	// sound
	soundMgr->PlaySound2d(gameRes->GetItemSound(item));
	soundMgr->PlaySound2d(gameRes->sCoins);
	// add gold
	int price = ItemHelper::GetItemPrice(item, *game->pc->unit, false);
	unit->gold += price;
	if(Net::IsLocal())
		game->pc->Train(TrainWhat::Trade, (float)price, 0);
	// add item to trader
	InsertItem(*unit->player->chestTrade, item, 1, 0);
	UpdateGrid(false);
	// remove player item
	unit->RemoveEquippedItem(slot);
	UpdateGrid(true);
	// message
	if(Net::IsClient())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::PUT_ITEM;
		c.id = SlotToIIndex(slot);
		c.count = 1;
	}
}

//=================================================================================================
void InventoryPanel::OnPutGold(int id)
{
	if(id == BUTTON_OK && counter > 0 && unit->gold >= counter)
	{
		if(counter > game->pc->unit->gold)
		{
			gui->SimpleDialog(base.txDropNoGold, this);
			return;
		}
		// add gold
		if(!InsertItem(*unit->player->chestTrade, Item::gold, counter, 0))
			UpdateGrid(false);
		// remove
		unit->gold -= counter;
		// sound
		soundMgr->PlaySound2d(gameRes->sCoins);
		if(Net::IsClient())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::PUT_GOLD;
			c.count = counter;
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
	uint teamCount = min(count, slot.teamCount);
	// sound
	soundMgr->PlaySound2d(gameRes->GetItemSound(slot.item));
	// add
	if(!game->pc->unit->AddItem(slot.item, count, teamCount))
		UpdateGrid(true);
	// remove
	if(base.mode == I_LOOT_BODY)
	{
		unit->weight -= slot.item->weight * count;

		if(slot.item == unit->usedItem)
		{
			unit->usedItem = nullptr;
			if(Net::IsServer())
			{
				NetChange& c = Add1(Net::changes);
				c.type = NetChange::REMOVE_USED_ITEM;
				c.unit = unit;
			}
		}

		if(IsSet(slot.item->flags, ITEM_IMPORTANT))
		{
			unit->mark = false;
			if(Net::IsServer())
			{
				NetChange& c = Add1(Net::changes);
				c.type = NetChange::MARK_UNIT;
				c.unit = unit;
				c.id = 0;
			}
		}

		ScriptEvent e(EVENT_PICKUP);
		e.item = slot.item;
		unit->FireEvent(e);
	}
	slot.count -= count;
	if(slot.count == 0)
	{
		lastIndex = INDEX_INVALID;
		if(mode == INVENTORY)
			base.tooltip.Clear();
		items->erase(items->begin() + index);
		UpdateGrid(false);
	}
	else
	{
		base.tooltip.Refresh();
		slot.teamCount -= teamCount;
	}

	// message
	if(Net::IsClient())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::GET_ITEM;
		c.id = index;
		c.count = count;
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
	uint teamCount = min(count, slot.teamCount);

	// play sound
	soundMgr->PlaySound2d(gameRes->GetItemSound(slot.item));

	// add to container
	if(base.mode == I_LOOT_BODY)
	{
		if(!unit->player->actionUnit->AddItem(slot.item, count, teamCount))
			UpdateGrid(false);
	}
	else if(base.mode == I_LOOT_CONTAINER)
	{
		if(!unit->player->actionUsable->container->AddItem(slot.item, count, teamCount))
			UpdateGrid(false);
	}
	else
	{
		if(!unit->player->actionChest->AddItem(slot.item, count, teamCount))
			UpdateGrid(false);
	}

	// remove from player
	unit->weight -= slot.item->weight * count;
	slot.count -= count;
	if(slot.count == 0)
	{
		lastIndex = INDEX_INVALID;
		if(mode == INVENTORY)
			base.tooltip.Clear();
		items->erase(items->begin() + index);
		UpdateGrid(true);
	}
	else
	{
		base.tooltip.Refresh();
		slot.teamCount -= teamCount;
	}

	// send change
	if(Net::IsClient())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::PUT_ITEM;
		c.id = index;
		c.count = count;
	}
}

//=================================================================================================
void InventoryPanel::PutSlotItem(ITEM_SLOT slot)
{
	const Item* item = equipped->at(slot);
	lastIndex = INDEX_INVALID;
	if(mode == INVENTORY)
		base.tooltip.Clear();

	// play sound
	soundMgr->PlaySound2d(gameRes->GetItemSound(item));

	// add to container
	if(base.mode == I_LOOT_BODY)
		unit->player->actionUnit->AddItem(item, 1u, 0u);
	else if(base.mode == I_LOOT_CONTAINER)
		unit->player->actionUsable->container->AddItem(item, 1u, 0u);
	else
		unit->player->actionChest->AddItem(item, 1u, 0u);
	UpdateGrid(false);

	// remove from player
	unit->RemoveEquippedItem(slot);
	UpdateGrid(true);

	// send change
	if(Net::IsClient())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::PUT_ITEM;
		c.id = SlotToIIndex(slot);
		c.count = 1;
	}
}

//=================================================================================================
void InventoryPanel::OnGiveGold(int id)
{
	if(id == BUTTON_OK && counter > 0)
	{
		if(counter > game->pc->unit->gold)
		{
			gui->SimpleDialog(base.txDropNoGold, this);
			return;
		}

		game->pc->unit->gold -= counter;
		soundMgr->PlaySound2d(gameRes->sCoins);
		Unit* u = game->pc->actionUnit;
		if(Net::IsLocal())
		{
			u->gold += counter;
			if(u->IsPlayer() && u->player != game->pc)
			{
				NetChangePlayer& c = Add1(u->player->playerInfo->changes);
				c.type = NetChangePlayer::GOLD_RECEIVED;
				c.id = game->pc->id;
				c.count = counter;
				u->player->playerInfo->UpdateGold();
			}
		}
		else
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::GIVE_GOLD;
			c.id = u->id;
			c.count = counter;
		}
	}
}

//=================================================================================================
void InventoryPanel::OnShareGiveItem(int id)
{
	int index = GetLockIndexAndRelease();
	if(id == BUTTON_OK && counter > 0 && index != -1)
	{
		Unit* t = unit->player->actionUnit;
		if(t->CanTake(items->at(index).item, counter))
			ShareGiveItem(index, counter);
		else
			gui->SimpleDialog(Format(base.txNpcCantCarry, t->GetName()), this);
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
	uint teamCount = min(count, slot.teamCount);
	// sound
	soundMgr->PlaySound2d(gameRes->GetItemSound(slot.item));
	// add
	if(!unit->player->actionUnit->AddItem(slot.item, count, teamCount))
		UpdateGrid(false);
	if(Net::IsLocal() && item->type == IT_CONSUMABLE)
	{
		const Consumable& pot = item->ToConsumable();
		if(pot.aiType == Consumable::AiType::Healing)
			unit->player->actionUnit->ai->havePotion = HavePotion::Yes;
		else if(pot.aiType == Consumable::AiType::Mana)
			unit->player->actionUnit->ai->haveMpPotion = HavePotion::Yes;
	}
	// remove
	unit->weight -= slot.item->weight * count;
	slot.count -= count;
	if(slot.count == 0)
	{
		lastIndex = INDEX_INVALID;
		if(mode == INVENTORY)
			base.tooltip.Clear();
		items->erase(items->begin() + index);
		UpdateGrid(true);
	}
	else
	{
		base.tooltip.Refresh();
		slot.teamCount -= teamCount;
	}
	// message
	if(Net::IsClient())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::PUT_ITEM;
		c.id = index;
		c.count = count;
	}
}

//=================================================================================================
void InventoryPanel::ShareTakeItem(int index, uint count)
{
	ItemSlot& slot = items->at(index);
	const Item* item = slot.item;
	uint teamCount = min(count, slot.teamCount);
	// sound
	soundMgr->PlaySound2d(gameRes->GetItemSound(slot.item));
	// add
	if(!game->pc->unit->AddItem(slot.item, count, teamCount))
		UpdateGrid(true);
	if(Net::IsLocal() && item->type == IT_CONSUMABLE)
	{
		const Consumable& pot = item->ToConsumable();
		if(pot.aiType == Consumable::AiType::Healing)
			unit->ai->havePotion = HavePotion::Check;
		else if(pot.aiType == Consumable::AiType::Mana)
			unit->ai->haveMpPotion = HavePotion::Check;
	}
	// remove
	unit->weight -= slot.item->weight * count;
	slot.count -= count;
	if(slot.count == 0)
	{
		lastIndex = INDEX_INVALID;
		if(mode == INVENTORY)
			base.tooltip.Clear();
		items->erase(items->begin() + index);
		UpdateGrid(false);
	}
	else
	{
		base.tooltip.Refresh();
		slot.teamCount -= teamCount;
	}
	// message
	if(Net::IsClient())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::GET_ITEM;
		c.id = index;
		c.count = count;
	}
}

//=================================================================================================
void InventoryPanel::OnGiveItem(int id)
{
	int iindex = GetLockIndexOrSlotAndRelease();
	if(id != BUTTON_YES || iindex == Unit::INVALID_IINDEX)
		return;

	lastIndex = INDEX_INVALID;
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
		item = equipped->at(slot_type);
	}

	// add
	Unit* t = unit->player->actionUnit;
	int price = item->value / 2;
	switch(giveItemMod)
	{
	case 0: // give team item for credit
		t->hero->credit += price;
		if(Net::IsLocal())
			team->CheckCredit(true);
		break;
	case 1: // sell item
		if(t->gold < price)
			return;
		t->gold -= price;
		unit->gold += price;
		soundMgr->PlaySound2d(gameRes->sCoins);
		break;
	case 2: // give item for free
		break;
	}
	t->AddItem(item, 1u, 0u);
	UpdateGrid(false);
	// sound
	soundMgr->PlaySound2d(gameRes->GetItemSound(item));
	// remove
	unit->weight -= item->weight;
	if(slot)
		items->erase(items->begin() + iindex);
	else
		unit->RemoveEquippedItem(slot_type);
	UpdateGrid(true);
	// setup inventory
	if(Net::IsLocal())
	{
		t->UpdateInventory();
		base.BuildTmpInventory(1);
	}
	// message
	if(Net::IsClient())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::PUT_ITEM;
		c.id = iindex;
		c.count = 1;
	}
}

//=================================================================================================
void InventoryPanel::OnGivePotion(int id)
{
	int index = GetLockIndexAndRelease();
	if(id == BUTTON_OK && counter > 0 && index != -1)
	{
		Unit* t = unit->player->actionUnit;
		if(t->CanTake(items->at(index).item, counter))
			GivePotion(index, counter);
		else
			gui->SimpleDialog(Format(base.txNpcCantCarry, t->GetName()), this);
	}
}

//=================================================================================================
void InventoryPanel::GivePotion(int index, uint count)
{
	ItemSlot& slot = items->at(index);
	uint teamCount = min(count, slot.teamCount);
	// sound
	soundMgr->PlaySound2d(gameRes->GetItemSound(slot.item));
	// add
	if(!unit->player->actionUnit->AddItem(slot.item, count, teamCount))
		UpdateGrid(false);
	if(Net::IsLocal() && slot.item->type == IT_CONSUMABLE)
	{
		const Consumable& pot = slot.item->ToConsumable();
		if(pot.aiType == Consumable::AiType::Healing)
			unit->player->actionUnit->ai->havePotion = HavePotion::Yes;
		else if(pot.aiType == Consumable::AiType::Mana)
			unit->player->actionUnit->ai->haveMpPotion = HavePotion::Yes;
	}
	// remove
	unit->weight -= slot.item->weight * count;
	slot.count -= count;
	if(slot.count == 0)
	{
		lastIndex = INDEX_INVALID;
		if(mode == INVENTORY)
			base.tooltip.Clear();
		items->erase(items->begin() + index);
		UpdateGrid(true);
	}
	else
	{
		base.tooltip.Refresh();
		slot.teamCount -= teamCount;
	}
	// message
	if(Net::IsClient())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::PUT_ITEM;
		c.id = index;
		c.count = count;
	}
}

//=================================================================================================
void InventoryPanel::GiveSlotItem(ITEM_SLOT slot)
{
	lastIndex = INDEX_INVALID;
	if(mode == INVENTORY)
		base.tooltip.Clear();
	DialogInfo info;
	const Item* item = equipped->at(slot);
	if(unit->player->actionUnit->gold >= item->value / 2)
	{
		// buy item
		info.text = Format(base.txSellItem, item->value / 2);
		giveItemMod = 1;
	}
	else
	{
		// offer lower price
		info.text = Format(base.txSellFreeItem, item->value / 2);
		giveItemMod = 2;
	}

	base.lock.Lock(slot);
	info.event = delegate<void(int)>(this, &InventoryPanel::OnGiveItem);
	info.order = DialogOrder::Normal;
	info.parent = this;
	info.type = DIALOG_YESNO;
	info.pause = false;
	base.lockDialog = gui->ShowDialog(info);
}

//=================================================================================================
void InventoryPanel::IsBetterItemResponse(bool isBetter)
{
	if(!base.lock || !base.lock.isGive)
	{
		Error("InventoryPanel::IsBetterItemResponse, inventory not isGive locked.");
		return;
	}

	int iindex = GetLockIndexOrSlotAndRelease();
	if(iindex == Unit::INVALID_IINDEX)
		Warn("InventoryPanel::IsBetterItem, item removed.");
	else if(game->pc->action != PlayerAction::GiveItems)
		Warn("InventoryPanel::IsBetterItem, no longer giving items.");
	else if(!isBetter)
		gui->SimpleDialog(base.txWontTakeItem, this);
	else
	{
		Unit* t = game->pc->actionUnit;
		if(iindex >= 0)
		{
			// not equipped item
			ItemSlot& slot = unit->items[iindex];
			if(!t->CanTake(slot.item))
				gui->SimpleDialog(Format(base.txNpcCantCarry, t->GetName()), this);
			else
			{
				DialogInfo info;
				if(slot.teamCount > 0)
				{
					// give team item
					info.text = Format(base.txSellTeamItem, slot.item->value / 2);
					giveItemMod = 0;
				}
				else if(t->gold >= slot.item->value / 2)
				{
					// sell item
					info.text = Format(base.txSellItem, slot.item->value / 2);
					giveItemMod = 1;
				}
				else
				{
					// give item for free
					info.text = Format(base.txSellFreeItem, slot.item->value / 2);
					giveItemMod = 2;
				}

				base.lock.Lock(iindex, slot);
				info.event = delegate<void(int)>(this, &InventoryPanel::OnGiveItem);
				info.order = DialogOrder::Normal;
				info.parent = this;
				info.type = DIALOG_YESNO;
				info.pause = false;
				base.lockDialog = gui->ShowDialog(info);
			}
		}
		else
		{
			// equipped item
			ITEM_SLOT slot_type = IIndexToSlot(iindex);
			const Item* item = unit->GetEquippedItem(slot_type);
			if(!t->CanTake(item))
				gui->SimpleDialog(Format(base.txNpcCantCarry, t->GetName()), this);
			else if(unit->SlotRequireHideWeapon(slot_type))
			{
				// hide equipped item and give it
				unit->HideWeapon();
				unit->player->nextAction = NA_GIVE;
				unit->player->nextActionData.slot = slot_type;
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
	itemVisible = nullptr;
	Event(GuiEvent_Show);
	GainFocus();
}

//=================================================================================================
void InventoryPanel::Hide()
{
	if(gameGui->book->visible)
		gameGui->book->Hide();
	LostFocus();
	visible = false;
	itemVisible = nullptr;
}

//=================================================================================================
void InventoryPanel::UpdateGrid(bool mine)
{
	if(mine)
	{
		base.BuildTmpInventory(0);
		base.invTradeMine->UpdateScrollbar();
	}
	else
	{
		base.BuildTmpInventory(1);
		base.invTradeOther->UpdateScrollbar();
	}
}

//=================================================================================================
int InventoryPanel::GetLockIndexAndRelease()
{
	assert(base.lock);
	int index = FindItemIndex(*items, base.lock.index, base.lock.item, base.lock.isTeam);
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
		iindex = FindItemIndex(*items, base.lock.index, base.lock.item, base.lock.isTeam);
		if(iindex == -1)
			iindex = Unit::INVALID_IINDEX;
	}
	else if(equipped && equipped->at(base.lock.slot))
		iindex = SlotToIIndex(base.lock.slot);
	else
		iindex = Unit::INVALID_IINDEX;
	base.lock = nullptr;
	return iindex;
}

//=================================================================================================
bool InventoryPanel::HandleLeftClick(const Item* item)
{
	if(mode == INVENTORY)
	{
		if(dragAndDrop)
		{
			if(input->Released(Key::LeftButton))
			{
				dragAndDrop = false;
				return true;
			}
		}
		else if(input->Pressed(Key::LeftButton))
		{
			dragAndDrop = true;
			dragAndDropPos = gui->cursorPos;
			dragAndDropItem = item;
		}
		return false;
	}
	else
		return input->PressedRelease(Key::LeftButton);
}
