#pragma once

//-----------------------------------------------------------------------------
#include "GamePanel.h"
#include "ItemSlot.h"
#include "Scrollbar.h"
#include "Button.h"
#include "TooltipController.h"

//-----------------------------------------------------------------------------
struct Unit;
struct Game;

//-----------------------------------------------------------------------------
enum LOCK_MODE
{
	LOCK_NO,
	LOCK_MY,
	LOCK_TRADE_MY,
	LOCK_TRADE_OTHER
};

//-----------------------------------------------------------------------------
const int LOCK_REMOVED = -SLOT_INVALID-1;

//-----------------------------------------------------------------------------
class Inventory : public GamePanel
{
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

	Inventory();
	void Draw(ControlDrawData* cdd=nullptr);
	void Update(float dt);
	void Event(GuiEvent e);
	void FormatBox();

	static void LoadText();
	static void LoadData();

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
	static cstring txGoldAndCredit, txGoldDropInfo, txCarryShort, txCarry, txCarryInfo, txTeamItem, txCantWear, txCantDoNow, txBuyTeamDialog, txDropGoldCount, txDropNoGold, txDropNotNow,
		txDropItemCount, txWontBuy, txPrice, txNeedMoreGoldItem, txBuyItemCount, txSellItemCount, txLooting, txTrading, txPutGoldCount, txLootItemCount, txPutItemCount, txTakeAll, txInventory,
		txLootingChest, txShareItems, txGiveItems, txPutGold, txGiveGold, txGiveGoldCount, txShareGiveItemCount, txCanCarryTeamOnly, txWontGiveItem, txShareTakeItemCount, txWontTakeItem,
		txSellTeamItem, txSellItem, txSellFreeItem, txGivePotionCount, txNpcCantCarry;
	Scrollbar scrollbar;
	Button bt;
	int counter, give_item_mode;
	Mode mode;
	
	static TEX tItemBar, tEquipped, tGold, tStarHq, tStarM, tStarU;
	// sposób na aktualizacje ekwipunku gdy dziej¹ siê dwie rzeczy na raz
	// np. chcesz wypiæ miksturkê, zaczynasz chowaæ broñ a w tym czasie z nik¹d pojawi siê jakiœ przedmiot, indeks miksturki siê zmienia
	static LOCK_MODE lock_id;
	static int lock_index;
	static bool lock_give;
	static float lock_timer;

private:
	void GetTooltip(TooltipController* tooltip, int group, int id);
	void UpdateGrid(bool mine);

	static TooltipController tooltip;
	Game& game;
};
