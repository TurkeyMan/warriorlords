#if !defined(_EDITOR_H)
#define _EDITOR_H

#include "Screen.h"
#include "Game.h"
#include "Map.h"
#include "Button.h"
#include "StringBox.h"

struct MFFont;

class Chooser : public InputReceiver
{
public:
	Chooser();
	~Chooser();

	Button *AddButton(int page, MFRect *pUVs, MFMaterial *pImage, int buttonID, Button::TriggerCallback *pCallback, void *pUserData);

	void Show();
	void Hide();

	void Draw();

	virtual bool HandleInputEvent(InputEvent ev, InputInfo &info);

protected:
	void AssembleButtons();

	float windowWidth;
	float windowHeight;

	MFMaterial *pIcons;

	Button *pFlipButton;
	Button *pButtons[8][12];
	int numButtons[8];
	int numPages;

	int currentPage;
	bool bVisible;

	static void FlipPage(int button, void *pUserData, int buttonID);
};

class CastleEdit : public InputReceiver
{
public:
	CastleEdit();
	~CastleEdit();

	void Update();
	void Draw();

	virtual bool HandleInputEvent(InputEvent ev, InputInfo &info);

	void Show(Castle *pCastle);
	void Hide();

protected:
	Castle *pCastle;

	MFFont *pFont;
	MFMaterial *pIcons;

	MFRect window, title, units, lower, right;

	Button *pBuildUnits[4];
	StringBox *pName;
	StringBox *pIncome;

	Chooser unitSelect;

	bool bVisible;
	bool bHide;

	static void SelectUnit(int button, void *pUserData, int buttonID);
	static void SetUnit(int button, void *pUserData, int buttonID);
	static void ChangeCallback(const char *pString, void *pUserData);
};

class Editor : public Screen
{
public:
	Editor(Game *pGame);
	virtual ~Editor();

	virtual void Select();
	virtual int Update();
	virtual void Draw();
	virtual void Deselect();

	virtual bool HandleInputEvent(InputEvent ev, InputInfo &info);

	static void BrushSelect(int button, void *pUserData, int buttonID);
	static void ChooseBrush(int button, void *pUserData, int buttonID);
	static void FlipPage(int button, void *pUserData, int buttonID);
	static void ShowMiniMap(int button, void *pUserData, int buttonID);
	static void ChangeMode(int button, void *pUserData, int buttonID);

protected:
	Game *pGame;
	Map *pMap;

	MFMaterial *pIcons;

	Button *pMiniMap;
	Button *pModeButton;
	Button *pBrushButton[2];

	Chooser brushSelect;
	CastleEdit castleEdit;

	ObjectType brushType[2];
	int brushIndex[2];
	int brush;

	bool bPaintMode;
	bool bRemoveRoad;

	int lastX, lastY;
};

#endif
