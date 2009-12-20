#if !defined(_EDITOR_H)
#define _EDITOR_H

#include "Screen.h"
#include "Game.h"
#include "Map.h"
#include "Button.h"

class Chooser : public InputReceiver
{
public:
	Chooser();
	~Chooser();

	Button *AddButton(int page, MFRect *pUVs, MFMaterial *pImage, int buttonID, Button::TriggerCallback *pCallback, void *pUserData, bool bTriggerOnDown = false);

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
	virtual int UpdateInput();

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
	Chooser unitSelect;

	ObjectType brushType[2];
	int brushIndex[2];
	int brush;

	bool bPaintMode;
	bool bRemoveRoad;

	int lastX, lastY;
};

#endif
