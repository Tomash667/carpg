#pragma once

//-----------------------------------------------------------------------------
#include "GamePanel.h"
#include "ItemSlot.h"
#include "Scrollbar.h"
#include "Button.h"
#include "TooltipController.h"

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
		bool is_team, is_give;

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

		void Lock(int index, ItemSlot& slot, bool is_give = false)
		{
			assert(index >= 0 && slot.item);
			this->index = index;
			this->is_give = is_give;
			this->slot = SLOT_INVALID;
			item = slot.item;
			is_team = slot.team_count > 0;
			timer = 1.f;
		}

		void Lock(ITEM_SLOT slot, bool is_give = false)
		{
			index = 0;
			this->slot = slot;
			this->is_give = is_give;
			timer = 1.f;
		}
	};

	~Inventory();
	void InitOnce();
	void LoadLanguage();
	void LoadData();
	void OnReset();
	void OnReload();
	void Setup(PlayerController* pc);
	void StartTrade(InventoryMode mode, Unit& unit);
	void StartTrade(InventoryMode mode, vector<ItemSlot>& items, Unit* unit = nullptr);
	void StartTrade2(InventoryMode mode, void* ptr);
	void BuildTmpInventory(int index);

	InventoryMode mode;
	GamePanelContainer* gp_trade;
	InventoryPanel* inv_mine, *inv_trade_mine, *inv_trade_other;
	// przedmioty w czasie grabienia itp s¹ tu przechowywane indeksy
	// ujemne wartoœci odnosz¹ siê do slotów (SLOT_WEAPON = -SLOT_WEAPON-1), pozytywne do zwyk³ych przedmiotów
	vector<int> tmp_inventory[2];
	int tmp_inventory_shift[2];
	ItemLock lock;
	TooltipController tooltip;
	TEX tItemBar, tEquipped, tGold, tStarHq, tStarM, tStarU, tTeamItem;
	cstring txGoldAndCredit, txGoldDropInfo, txCarryShort, txCarry, txCarryInfo, txTeamItem, txCantWear, txCantDoNow, txBuyTeamDialog, txDropGoldCount,
		txDropNoGold, txDropNotNow, txDropItemCount, txWontBuy, txPrice, txNeedMoreGoldItem, txBuyItemCount, txSellItemCount, txLooting, txLootingChest,
		txTrading, txPutGoldCount, txLootItemCount, txPutItemCount, txTakeAll, txInventory, txShareItems, txGiveItems, txPutGold, txGiveGold, txGiveGoldCount,
		txShareGiveItemCount, txCanCarryTeamOnly, txWontGiveItem, txShareTakeItemCount, txWontTakeItem, txSellTeamItem, txSellItem, txSellFreeItem,
		txGivePotionCount, txNpcCantCarry, txShowStatsFor, txStatsFor;
};

//-----------------------------------------------------------------------------
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

	void Draw(ControlDrawData* cdd = nullptr) override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;

	void FormatBox(bool refresh = false);
	void InitTooltip();
	bool SlotRequireHideWeapon(ITEM_SLOT slot);
	void RemoveSlotItem(ITEM_SLOT slot);
	void DropSlotItem(ITEM_SLOT slot);
	void ConsumeItem(int index);
	void EquipSlotItem(ITEM_SLOT slot, int i_index);
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
	void IsBetterItemResponse(bool is_better);
	string title;
	vector<int>* i_items;
	vector<ItemSlot>* items;
	const Item* last_item;
	const Item** slots;
	Unit* unit;
	Scrollbar scrollbar;
	Button bt;
	int counter, give_item_mode;
	Mode mode;

private:
	void GetTooltip(TooltipController* tooltip, int group, int id);
	void UpdateGrid(bool mine);
	void ReadBook(const Item* item, int index);
	void FormatBox(int group, string& text, string& small_text, TEX& img, bool refresh);
	bool AllowForUnit() { return Any(mode, GIVE_MY, GIVE_OTHER, SHARE_MY, SHARE_OTHER); }
	int GetLockIndexAndRelease();
	int GetLockIndexOrSlotAndRelease();

	Inventory& base;
	Game& game;
	float rot;
	const Item* item_visible;
	bool for_unit, tex_replaced;
};
