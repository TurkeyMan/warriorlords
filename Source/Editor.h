#if !defined(_EDITOR_H)
#define _EDITOR_H

#include "Screen.h"
#include "Map.h"
#include "Button.h"

class Editor : public Screen
{
public:
	Editor(const char *pMap = NULL);
	virtual ~Editor();

	virtual void Select();
	virtual int Update();
	virtual void Draw();
	virtual void Deselect();

	virtual int UpdateInput();

	static void BrushSelect(int button, void *pUserData, int buttonID);
	static void ChooseBrush(int button, void *pUserData, int buttonID);

protected:
	Map *pMap;

	Button *pBrushButton[2];
	Button *pTerrainButtons[15];
	float terrainSelectWindowWidth;
	float terrainSelectWindowHeight;

	int brushType[2];
	int brush;

	bool bIsPainting;
	bool bShowTerrainChooser;
};

#endif
