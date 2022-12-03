#pragma once

//-----------------------------------------------------------------------------
#include "GamePanel.h"
#include "ItemSlot.h"
#include <Scrollbar.h>
#include <Button.h>
#include <TooltipController.h>

//-----------------------------------------------------------------------------
enum InventoryMode
{
	I_NONE,
	I_INVENTORY,
	I_LOOT_BODY,
	I_LOOT_CHEST,
	I_TRADE,
	I_SHARE,
	I_GIVE,
	I_LOOT_CONTAINER
};

//-----------------------------------------------------------------------------
// Container for multiple inventories (single, my trade, other trade)
class Inventory
{
public:
	// item lock - keeps track of item while inventory is changing
	struct ItemLock
	{
		ITEM_SLOT slot;
		int index;
		const Item* item;
		float timer;
		bool isTeam, isGive;

		ItemLock() : index(-1) {}

		operator bool() const
		{
			return index != -1;
		}

		void operator = (nullptr_t)
		{
			index = -1;
			slot = SLOT_INVALID;
		}

		void Lock(int index, ItemSlot& slot, bool isGive = false)
		{
			assert(index >= 0 && slot.item);
			this->index = index;
			this->isGive = isGive;
			this->slot = SLOT_INVALID;
			item = slot.item;
			isTeam = slot.team_count > 0;
			timer = 1.f;
		}

		void Lock(ITEM_SLOT slot, bool isGive = false)
		{
			index = 0;
			this->slot = slot;
			this->isGive = isGive;
			timer = 1.f;
		}
	};

	~Inventory();
	void InitOnce();
	void LoadLanguage();
	void LoadData();
	void Setup(PlayerController* pc);
	void StartTrade(InventoryMode mode, Unit& unit);
	void StartTrade(InventoryMode mode, vector<ItemSlot>& items, Unit* unit = nullptr);
	void StartTrade2(InventoryMode mode, void* ptr);
	void BuildTmpInventory(int index);
	void EndLock();

	InventoryMode mode;
	GamePanelContainer* gpTrade;
	InventoryPanel* invMine, *invTradeMine, *invTradeOther;
	ItemLock lock;
	DialogBox* lockDialog;
	TooltipController tooltip;
	TexturePtr tItemBar, tEquipped, tGold, tStarHq, tStarM, tStarU, tTeamItem;
	cstring txGoldAndCredit, txGoldDropInfo, txCarryShort, txCarry, txCarryInfo, txTeamItem, txCantWear, txCantDoNow, txBuyTeamDialog, txDropGoldCount,
		txDropNoGold, txDropNotNow, txDropItemCount, txWontBuy, txPrice, txNeedMoreGoldItem, txBuyItemCount, txSellItemCount, txLooting, txLootingChest,
		txTrading, txPutGoldCount, txLootItemCount, txPutItemCount, txTakeAll, txInventory, txShareItems, txGiveItems, txPutGold, txGiveGold, txGiveGoldCount,
		txShareGiveItemCount, txCanCarryTeamOnly, txWontGiveItem, txShareTakeItemCount, txWontTakeItem, txSellTeamItem, txSellItem, txSellFreeItem,
		txGivePotionCount, txNpcCantCarry, txShowStatsFor, txStatsFor;

private:
	// przedmioty w czasie grabienia itp s� tu przechowywane indeksy
	// ujemne warto�ci odnosz� si� do slot�w (SLOT_WEAPON = -SLOT_WEAPON-1), pozytywne do zwyk�ych przedmiot�w
	vector<int> tmpInventory[2];
	int tmpInventoryShift[2];
};

//-----------------------------------------------------------------------------
// Inventory panel, allow equip, use, drop & sell items
class InventoryPanel : public GamePanel
{
	friend class Inventory;
public:
	enum Mode
	{
		INVENTORY,
		TRADE_MY,
		TRADE_OTHER,
		LOOT_MY,
		LOOT_OTHER,
		GIVE_MY,
		GIVE_OTHER,
		SHARE_MY,
		SHARE_OTHER
	};

	InventoryPanel(Inventory& base);

	void Draw() override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;

	void FormatBox(bool refresh = false);
	void InitTooltip();
	void RemoveSlotItem(ITEM_SLOT slot);
	void DropSlotItem(ITEM_SLOT slot);
	void ConsumeItem(int index);
	void EquipSlotItem(int index);
	void UpdateScrollbar();
	void Show();
	void Hide();
	void OnDropGold(int id);
	void OnDropItem(int id);
	void OnTakeItem(int id);
	void OnBuyItem(int id);
	void OnSellItem(int id);
	void BuyItem(int index, uint count);
	void SellItem(int index, uint count);
	void SellSlotItem(ITEM_SLOT slot);
	void OnPutGold(int id);
	void OnLootItem(int id);
	void LootItem(int index, uint count);
	void OnPutItem(int id);
	void PutItem(int index, uint count);
	void PutSlotItem(ITEM_SLOT slot);
	void OnGiveGold(int id);
	void OnShareGiveItem(int id);
	void OnShareTakeItem(int id);
	void ShareGiveItem(int index, uint count);
	void ShareTakeItem(int index, uint count);
	void OnGiveItem(int id);
	void OnGivePotion(int id);
	void GivePotion(int index, uint count);
	void GiveSlotItem(ITEM_SLOT slot);
	void IsBetterItemResponse(bool isBetter);

	string title;
	vector<int>* indices;
	vector<ItemSlot>* items;
	const Item* lastItem;
	const array<const Item*, SLOT_MAX>* equipped;
	Unit* unit;
	Scrollbar scrollbar;
	Button bt;
	int counter, giveItemMod;
	Mode mode;

private:
	void GetTooltip(TooltipController* tooltip, int group, int id, bool refresh);
	void UpdateGrid(bool mine);
	void FormatBox(int group, string& text, string& smallText, Texture*& img, bool refresh);
	bool AllowForUnit() { return Any(mode, GIVE_MY, GIVE_OTHER, SHARE_MY, SHARE_OTHER); }
	int GetLockIndexAndRelease();
	int GetLockIndexOrSlotAndRelease();
	bool HandleLeftClick(const Item* item);

	Inventory& base;
	float rot;
	const Item* itemVisible;
	const Item* dragAndDropItem;
	Int2 dragAndDropPos;
	bool forUnit, texReplaced, dragAndDrop;
};
