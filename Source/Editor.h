#if !defined(_EDITOR_H)
#define _EDITOR_H

#include "Screen.h"
#include "Game.h"
#include "Map.h"
#include "Button.h"

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
	Button *pChooserButtons[3][11];
	Button *pFlipButton;

	int pageButtonCount[3];
	int numPages;

	float terrainSelectWindowWidth;
	float terrainSelectWindowHeight;

	ObjectType brushType[2];
	int brushIndex[2];
	int brush;

	bool bPaintMode;
	bool bIsPainting;
	bool bRemoveRoad;

	int tileChooser;
};

#endif
