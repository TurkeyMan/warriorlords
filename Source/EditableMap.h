#pragma once
#if !defined(_EDITABLE_MAP_H)
#define _EDITABLE_MAP_H

#include "Map.h"

class EditableMap : public Map
{
	friend class Editor;
public:
	EditableMap(::GameState &gameState, MFString mapFilename);
	EditableMap(::GameState &gameState, MFString tileset, MFString units, int width, int height);
	virtual ~EditableMap();

	bool SetTerrain(int x, int y, int tl, int tr, int bl, int br, uint32 mask = 0xFFFFFFFF);
	void SetRegion(int x, int y, int region);

	bool PlaceCastle(int x, int y, int player);
	bool PlaceFlag(int x, int y, int race = 0);
	bool PlaceSpecial(int x, int y, int index);
	bool PlaceRoad(int x, int y);

	void ClearDetail(int x, int y);

	void SetEditRegion(int region)	{ editRegion = region; }
	void SetEditRace(int race)		{ editRace = race; }

	CastleDetails *GetCastleTemplate(int x, int y);

	int UpdateChange(int a);

	void Save()						{ edit.Save(); }

private:
	struct MapCoord
	{
		uint16 x, y;
	};

	struct RevertTile
	{
		uint16 x, y;
		uint32 t;
	};

	MapTemplate edit;

	uint8 *pTouched;
	MapCoord *pChangeList;
	int numChanges;
	RevertTile *pRevertList;
	int numReverts;

	bool bRevert;

	int editRegion;
	int editRace;

	bool SetTile(int x, int y, uint32 tile, uint32 mask);
	int ChooseTile(int *pSelectedTiles, int numVariants);
};

#endif
