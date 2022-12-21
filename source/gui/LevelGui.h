#pragma once

//-----------------------------------------------------------------------------
#include <Container.h>
#include <Scrollbar.h>
#include <TooltipController.h>

//-----------------------------------------------------------------------------
enum class OpenPanel
{
	None,
	Stats,
	Inventory,
	Team,
	Journal,
	Minimap,
	Ability,
	Trade,
	Unknown,
	Book,
	Craft
};

//-----------------------------------------------------------------------------
enum class SideButtonId
{
	Menu,
	Team,
	Minimap,
	Journal,
	Inventory,
	Ability,
	Stats,
	Talk,
	Max
};

//-----------------------------------------------------------------------------
struct SpeechBubble
{
	string text;
	Unit* unit;
	Int2 size;
	float time, length;
	int skipId;
	Vec3 lastPos;
	bool visible;
};

//-----------------------------------------------------------------------------
struct BuffImage
{
	Vec2 pos;
	Texture* tex;
	int id;

	BuffImage(const Vec2& pos, Texture* tex, int id) : pos(pos), tex(tex), id(id)
	{
	}
};

//-----------------------------------------------------------------------------
// Ingame gui - contains player hp/stamina/mana bars, shortcuts, buffs, side menu & dialog menu
class LevelGui : public Container
{
public:
	LevelGui();
	~LevelGui();
	void LoadLanguage();
	void LoadData();
	void Draw() override;
	void Update(float dt) override;
	bool NeedCursor() const override;
	void Event(GuiEvent e) override;
	void AddSpeechBubble(Unit* unit, cstring text);
	void AddSpeechBubble(const Vec3& pos, cstring text);
	void Reset();
	bool UpdateChoice();
	void UpdateScrollbar(int choices);
	bool HavePanelOpen() const;
	bool CanFocusMpBox() const { return !HavePanelOpen(); }
	void ClosePanels(bool closeMpBox = false);
	void GetGamePanels(vector<GamePanel*>& panels);
	OpenPanel GetOpenPanel();
	void ShowPanel(OpenPanel p, OpenPanel open = OpenPanel::Unknown);
	void PositionPanels();
	void Save(GameWriter& f) const;
	void Load(GameReader& f);
	bool IsMouseInsideDialog() const { return Rect::IsInside(gui->cursorPos, dialogPos, dialogSize); }
	void RemoveUnitView(Unit* unit);
	void DrawEndOfGameScreen();
	void StartDragAndDrop(int type, int value, Texture* tex);
	bool IsDragAndDrop() const { return dragAndDrop == 2; }
	void ResetCutscene();
	void SetCutsceneImage(Texture* tex, float time);
	void SetCutsceneText(const string& text, float time);
	void SetBoss(Unit* boss, bool instant);

	Int2 dialogCursorPos;
	bool useCursor;

private:
	enum CutsceneState
	{
		CS_NONE,
		CS_FADE_IN,
		CS_WAIT,
		CS_FADE_OUT
	};

	struct UnitView
	{
		Unit* unit;
		Vec3 lastPos;
		float time;
		bool valid;
	};

	struct SortedUnitView
	{
		Unit* unit;
		float dist;
		int alpha;
		Vec3* lastPos;
	};

	struct SortedSpeechBubble
	{
		SpeechBubble* bubble;
		float dist;
		Int2 pt;
	};

	int DrawFallback();
	void DrawFront();
	void DrawBack();
	void DrawDeathScreen();
	void DrawSpeechBubbles();
	void DrawObjectInfo(cstring text, const Vec3& pos);
	void DrawUnitInfo(cstring text, Unit& unit, const Vec3& pos, int alpha = 255);
	int GetShortcutIndex();
	void UpdateSpeechBubbles(float dt);
	void GetTooltip(TooltipController*, int group, int id, bool refresh);
	void SortUnits();
	void UpdatePlayerView(float dt);
	void AddUnitView(Unit* unit);
	void UpdateCutscene(float dt);
	void DrawCutscene(int fallbackAlpha);
	int GetAlpha(CutsceneState cs, float timer, int fallbackAlpha);

	TooltipController tooltip;
	vector<BuffImage> buffImages;
	vector<SortedUnitView> sortedUnits;
	float buffScale, sidebar, cutsceneImageTimer, cutsceneTextTimer, bossAlpha;
	int sidebarState[(int)SideButtonId::Max], dragAndDrop, dragAndDropType, dragAndDropIndex;
	CutsceneState cutsceneImageState, cutsceneTextState;
	Int2 dragAndDropPos;
	TexturePtr tBar, tHpBar, tPoisonedHpBar, tStaminaBar, tManaBar, tShortcut, tShortcutHover, tShortcutDown, tSideButton[(int)SideButtonId::Max], tMinihp,
		tMinistamina, tMinimp, tCrosshair, tCrosshair2, tBubble, tDamageLayer, tActionCooldown, tMelee, tRanged, tHealthPotion, tManaPotion, tEmerytura,
		tEquipped, tDialog, tShortcutAction, tRip, tBlack;
	TexturePtr dragAndDropIcon, cutsceneImage;
	vector<pair<Texture*, float>> cutsceneNextImages;
	Scrollbar scrollbar;
	vector<SpeechBubble*> speechBubbles;
	vector<SortedSpeechBubble> sortedSpeechBubbles;
	string cutsceneText;
	vector<pair<string, float>> cutsceneNextTexts;
	cstring txMenu, txDeath, txDeathAlone, txGameTimeout, txChest, txDoor, txDoorLocked, txHp, txMana, txStamina, txMeleeWeapon, txRangedWeapon,
		txHealthPotion, txManaPotion, txMeleeWeaponDesc, txRangedWeaponDesc, txHealthPotionDesc, txManaPotionDesc, txSkipCutscene;
	Int2 debugInfoSize, dialogPos, dialogSize;
	vector<UnitView> unitViews;
	bool debugInfo, debugInfo2, bossState;
	Unit* boss;
};
