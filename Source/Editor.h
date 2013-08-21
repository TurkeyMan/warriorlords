#if !defined(_EDITOR_H)
#define _EDITOR_H

#include "Screen.h"
#include "Game.h"
#include "EditableMap.h"
#include "Button.h"
#include "StringBox.h"

struct MFFont;

class Chooser : public InputReceiver
{
public:
	Chooser();
	~Chooser();

	Button *AddButton(int page, MFMaterial *pImage, MFRect *pUVs, const MFVector &colour, int buttonID, Button::ClickCallback callback);

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

	void FlipPage(int button, int buttonID);
};

class CastleEdit : public InputReceiver
{
public:
	CastleEdit(Editor &editor);
	~CastleEdit();

	void Update();
	void Draw();

	virtual bool HandleInputEvent(InputEvent ev, InputInfo &info);

	void Show(Castle *pCastle);
	void Hide();

protected:
	Editor &editor;

	Castle *pCastle;
	CastleDetails *pTemplate;

	MFFont *pFont;
	MFMaterial *pIcons;

	MFRect window, title, units, lower, right;

	Button *pBuildUnits[4];
	StringBox *pName;

	Chooser unitSelect;

	bool bVisible;
	bool bHide;

	void SelectUnit(int button, int buttonID);
	void SetUnit(int button, int buttonID);
	void NameChangeCallback(MFString string);
};

class Editor : public Game, public Screen
{
public:
	Editor(MFString mapFilename);
	virtual ~Editor();

	virtual void Select();
	virtual int Update();
	virtual void Draw();
	virtual void Deselect();

	virtual bool HandleInputEvent(InputEvent ev, InputInfo &info);

	void BrushSelect(int button,int buttonID);
	void ChooseBrush(int button, int buttonID);
	void FlipPage(int button, int buttonID);
	void ShowMiniMap(int button, int buttonID);
	void ChangeMode(int button, int buttonID);

	EditableMap &Map()	{ return map; }

protected:
	EditableMap map;
	GameState gameState;

	MFMaterial *pIcons;

	Button *pMiniMap;
	Button *pModeButton;
	Button *pBrushButton[2];

	Chooser brushSelect;
	CastleEdit castleEdit;

	ObjectType brushType[2];
	int brushIndex[2];
	int brush;
	int brushSize;

	bool bPaintMode;
	bool bRemoveRoad;

	int lastX, lastY;

	int editRace;
	int editRegion;
};

#endif
