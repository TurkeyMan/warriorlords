#include "Warlords.h"
#include "EditableMap.h"
#include "GameState.h"

EditableMap::EditableMap(::GameState &gameState, MFString mapFilename)
: edit(mapFilename)
, Map(edit, gameState)
{
	// if we want to edit the map, we need some memory for map traversal
	pTouched = (uint8*)MFHeap_AllocAndZero(width * height * sizeof(*pTouched));
	pChangeList = (MapCoord*)MFHeap_Alloc(sizeof(MapCoord)*1024);
	pRevertList = (RevertTile*)MFHeap_Alloc(sizeof(RevertTile)*1024);
	numChanges = 0;
}

EditableMap::EditableMap(::GameState &gameState, MFString tileset, MFString units, int width, int height)
: edit(tileset, units, width, height)
, Map(edit, gameState)
{
	edit.name = "Untitled";

	pMap = (MapTile*)MFHeap_AllocAndZero(width * height * sizeof(MapTile));
	for(int a=0; a<width * height; ++a)
	{
		pMap[a].x = a % width;
		pMap[a].y = a / width;
	}

	path.Init(this);

	// get default tiles
	int tiles[8];
	int numVariants = Tileset().FindBestTiles(tiles, 0);

	for(int y=0; y<height; ++y)
	{
		MapTile *pRow = pMap + y*width;
		for(int x=0; x<width; ++x)
			pRow[x].terrain = ChooseTile(tiles, numVariants);
	}

	// editor stuff
	pTouched = (uint8*)MFHeap_AllocAndZero(width * height * sizeof(*pTouched));
	pChangeList = (MapCoord*)MFHeap_Alloc(sizeof(MapCoord)*1024);
	pRevertList = (RevertTile*)MFHeap_Alloc(sizeof(RevertTile)*1024);
	numChanges = 0;
}

EditableMap::~EditableMap()
{
	MFHeap_Free(pTouched);
	MFHeap_Free(pChangeList);
	MFHeap_Free(pRevertList);

	// delete the mapTemplate
	delete &edit;
}

CastleDetails *EditableMap::GetCastleTemplate(int x, int y)
{
	for(size_t a=0; a<edit.templates[editRace].castles.size(); ++a)
	{
		if(edit.templates[editRace].castles[a].x == x && edit.templates[editRace].castles[a].y == y)
			return &edit.templates[editRace].castles[a];
	}

	return NULL;
}

int EditableMap::ChooseTile(int *pSelectedTiles, int numVariants)
{
	if(numVariants == 1)
		return pSelectedTiles[0];

	int t, total = 0;
	for(t=0; t<numVariants; ++t)
		total += Tileset().GetTile(pSelectedTiles[t])->bias;

	int selection = (int)(MFRand() % total);
	for(t=0; t<numVariants; ++t)
	{
		int tile = pSelectedTiles[t];
		selection -= Tileset().GetTile(tile)->bias;

		if(selection < 0)
			return tile;
	}

	return 0;
}

bool EditableMap::SetTile(int x, int y, uint32 tile, uint32 mask)
{
	if(tile == GetTerrainAt(x, y))
 		return true;

	if(!bRevert && pMap[y*width + x].region != editRegion)
	{
		pRevertList[numReverts].x = x;
		pRevertList[numReverts].y = y;
		pRevertList[numReverts].t = pMap[y*width + x].GetTerrain();
		++numReverts;
	}

	int tiles[8];
	int matches = Tileset().FindBestTiles(tiles, tile, mask);
	MFDebug_Assert(matches, MFStr("Couldn't find matching tile at (%d, %d): %d, %d, %d, %d", x, y, tile & 0xFF, (tile >> 8) & 0xFF, (tile >> 16) & 0xFF, (tile >> 24) & 0xFF));
	if(!matches)
		return false;

	// clear any existing stuff on the tile
	ClearDetail(x, y);

	// TODO: use some logic to refine the selection based on quickest/valid route to target
	int t = ChooseTile(tiles, matches);
	pMap[y*width + x].terrain = t;
	edit.templates[editRace].pMap[y*width + x].terrain = t;

	// mark the tile touched
	pTouched[y*width + x] = 1;

	// update adjacent tiles
	if(y > 0 && !pTouched[(y-1)*width + x])
	{
		pChangeList[numChanges].x = x;
		pChangeList[numChanges].y = y-1;
		++numChanges;
	}
	if(y < height-1 && !pTouched[(y+1)*width + x])
	{
		pChangeList[numChanges].x = x;
		pChangeList[numChanges].y = y+1;
		++numChanges;
	}
	if(x > 0 && !pTouched[y*width + x - 1])
	{
		pChangeList[numChanges].x = x-1;
		pChangeList[numChanges].y = y;
		++numChanges;
	}
	if(x < width-1 && !pTouched[y*width + x + 1])
	{
		pChangeList[numChanges].x = x+1;
		pChangeList[numChanges].y = y;
		++numChanges;
	}

	return true;
}

bool EditableMap::SetTerrain(int x, int y, int tl, int tr, int bl, int br, uint32 mask)
{
	MFZeroMemory(pTouched, width * height);
	numChanges = 0;
	numReverts = 0;
	bRevert = false;

	if(!SetTile(x, y, EncodeTile(tl, tr, bl, br), mask))
		return false;

	for(int a=0; a<numChanges; ++a)
	{
		x = pChangeList[a].x;
		y = pChangeList[a].y;
		DecodeTile(GetTerrainAt(x, y), &tl, &tr, &bl, &br);

		// perform a bunch of logic to find a tile type suggestion...
		int tlm = 0, trm = 0, blm = 0, brm = 0;

		// update adjacent tiles
		if(y > 0 && pTouched[(y-1)*width + x])
		{
			int t = GetTerrainAt(x, y-1);
			tl = DecodeBL(t);
			tr = DecodeBR(t);
			tlm = 0xFF;
			trm = 0xFF;
		}
		if(y < height-1 && pTouched[(y+1)*width + x])
		{
			int b = GetTerrainAt(x, y+1);
			bl = DecodeTL(b);
			br = DecodeTR(b);
			blm = 0xFF;
			brm = 0xFF;
		}
		if(x > 0 && pTouched[y*width + x - 1])
		{
			int l = GetTerrainAt(x-1, y);
			tl = DecodeTR(l);
			bl = DecodeBR(l);
			tlm = 0xFF;
			blm = 0xFF;
		}
		if(x < width-1 && pTouched[y*width + x + 1])
		{
			int r = GetTerrainAt(x+1, y);
			tr = DecodeTL(r);
			br = DecodeBL(r);
			trm = 0xFF;
			brm = 0xFF;
		}

		SetTile(x, y, EncodeTile(tl, tr, bl, br), EncodeTile(tlm, trm, blm, brm));
	}

	bRevert = true;

	for(int a=0; a<numReverts; ++a)
	{
		MFZeroMemory(pTouched, width * height);
		numChanges = 0;

		SetTile(pRevertList[a].x, pRevertList[a].y, pRevertList[a].t, 0xFFFFFFFF);

		for(int a=0; a<numChanges; ++a)
		{
			x = pChangeList[a].x;
			y = pChangeList[a].y;
			DecodeTile(GetTerrainAt(x, y), &tl, &tr, &bl, &br);

			// perform a bunch of logic to find a tile type suggestion...
			int tlm = 0, trm = 0, blm = 0, brm = 0;

			// update adjacent tiles
			if(y > 0 && pTouched[(y-1)*width + x])
			{
				int t = GetTerrainAt(x, y-1);
				tl = DecodeBL(t);
				tr = DecodeBR(t);
				tlm = 0xFF;
				trm = 0xFF;
			}
			if(y < height-1 && pTouched[(y+1)*width + x])
			{
				int b = GetTerrainAt(x, y+1);
				bl = DecodeTL(b);
				br = DecodeTR(b);
				blm = 0xFF;
				brm = 0xFF;
			}
			if(x > 0 && pTouched[y*width + x - 1])
			{
				int l = GetTerrainAt(x-1, y);
				tl = DecodeTR(l);
				bl = DecodeBR(l);
				tlm = 0xFF;
				blm = 0xFF;
			}
			if(x < width-1 && pTouched[y*width + x + 1])
			{
				int r = GetTerrainAt(x+1, y);
				tr = DecodeTL(r);
				br = DecodeBL(r);
				trm = 0xFF;
				brm = 0xFF;
			}

			SetTile(x, y, EncodeTile(tl, tr, bl, br), EncodeTile(tlm, trm, blm, brm));
		}
	}

	return true;
}

void EditableMap::SetRegion(int x, int y, int region)
{
	MapTile *pTile = pMap + y*width + x;
	pTile->region = region;
}

int EditableMap::UpdateChange(int a)
{
	if(a>=numChanges)
		return numChanges;

	int x, y, tl, tr, bl, br;

	x = pChangeList[a].x;
	y = pChangeList[a].y;
	DecodeTile(GetTerrainAt(x, y), &tl, &tr, &bl, &br);

	// perform a bunch of logic to find a tile type suggestion...
	int tlm = 0, trm = 0, blm = 0, brm = 0;

	// update adjacent tiles
	if(y > 0 && pTouched[(y-1)*width + x])
	{
		int t = GetTerrainAt(x, y-1);
		tl = DecodeBL(t);
		tr = DecodeBR(t);
		tlm = 0xFF;
		trm = 0xFF;
	}
	if(y < height-1 && pTouched[(y+1)*width + x])
	{
		int b = GetTerrainAt(x, y+1);
		bl = DecodeTL(b);
		br = DecodeTR(b);
		blm = 0xFF;
		brm = 0xFF;
	}
	if(x > 0 && pTouched[y*width + x - 1])
	{
		int l = GetTerrainAt(x-1, y);
		tl = DecodeTR(l);
		bl = DecodeBR(l);
		tlm = 0xFF;
		blm = 0xFF;
	}
	if(x < width-1 && pTouched[y*width + x + 1])
	{
		int r = GetTerrainAt(x+1, y);
		tr = DecodeTL(r);
		br = DecodeBL(r);
		trm = 0xFF;
		brm = 0xFF;
	}

	SetTile(x, y, EncodeTile(tl, tr, bl, br), EncodeTile(tlm, trm, blm, brm));

	return ++a;
}

bool EditableMap::PlaceCastle(int x, int y, int player)
{
	if(pMap[y*width + x].region != editRegion)
		return false;

	// check we have room on the map
	if(x >= width - 1 || y >= height - 1)
		return false;

	// get the terrain tiles
	const Tile *pT0 = GetTerrainTileAt(x, y);
	const Tile *pT1 = GetTerrainTileAt(x+1, y);
	const Tile *pT2 = GetTerrainTileAt(x, y+1);
	const Tile *pT3 = GetTerrainTileAt(x+1, y+1);

	// test we can build here...
	if(!pT0->canBuild)
		return false;

	// test we have 4 clear tiles
	if(pT0->terrain != pT1->terrain || pT2->terrain != pT3->terrain || pT0->terrain != pT2->terrain)
		return false;

	// cool, we can place a castle here!
	// clear the space for the castle...
	ClearDetail(x, y);
	ClearDetail(x+1, y);
	ClearDetail(x, y+1);
	ClearDetail(x+1, y+1);

	// place castle
	int castleId = castles.size();
	for(int a=0; a<4; ++a)
	{
		MapTile *pTile = pMap + (y + (a >>1))*width + x + (a & 1);
		pTile->type = OT_Castle;
		pTile->index = castleId;
		pTile->objectX = a & 1;
		pTile->objectY = a >> 1;
	}

	Castle &castle = castles.push();
	MFZeroMemory(&castle, sizeof(Castle));
	castle.pTile = pMap + y*width + x;
	castle.player = player;
	castle.details.x = x;
	castle.details.y = y;
	castle.details.name = "Unnamed";

	CastleDetails &details = edit.templates[editRace].castles.push();
	MFZeroMemory(&details, sizeof(details));
	details.x = x;
	details.y = y;
	details.bCapital = player != -1;
	details.name = "Unnamed";
	return true;
}

bool EditableMap::PlaceFlag(int x, int y, int race)
{
	if(pMap[y*width + x].region != editRegion)
		return false;

	// check for compatible terrain
	const Tile *pT = GetTerrainTileAt(x, y);

	// check if we can build on this terrain type
	if(!pT->canBuild)
		return false;

	// clear the tile
	ClearDetail(x, y);

	// terrain is compatible, place flag
	pMap[y*width + x].type = OT_Flag;
	pMap[y*width + x].index = race;

	edit.templates[editRace].pMap[y*width + x].type = OT_Flag;
	edit.templates[editRace].pMap[y*width + x].index = race;

	return true;
}

bool EditableMap::PlaceSpecial(int x, int y, int index)
{
	if(pMap[y*width + x].region != editRegion)
		return false;

	// check for compatible terrain
	const Tile *pT = GetTerrainTileAt(x, y);

	// check if the terrain type matches the special target.
	// TODO: for now, place on flat land...
	if(!pT->canBuild)
		return false;

	ClearDetail(x, y);

	// terrain is compatible, place special
	pMap[y*width + x].type = OT_Place;
	pMap[y*width + x].index = index;

	edit.templates[editRace].pMap[y*width + x].type = OT_Place;
	edit.templates[editRace].pMap[y*width + x].index = index;

	return true;
}

bool EditableMap::PlaceRoad(int x, int y)
{
	if(pMap[y*width + x].region != editRegion)
		return false;

	if(pMap[y*width + x].type == OT_Road)
		return true;

	// check for compatible terrain
	MapTile *pHere  = pMap + y*width + x;
	MapTile *pUp    = y > 0           ? pMap + (y-1)*width + x : NULL;
	MapTile *pDown  = y < height-1 ? pMap + (y+1)*width + x : NULL;
	MapTile *pLeft  = x > 0           ? pMap + y*width + x-1 : NULL;
	MapTile *pRight = x < width-1  ? pMap + y*width + x+1 : NULL;

	const Tile *pTHere  = GetTerrainTileAt(x, y);
	const Tile *pTUp    = pUp    ? GetTerrainTileAt(x, y-1) : NULL;
	const Tile *pTDown  = pDown  ? GetTerrainTileAt(x, y+1) : NULL;
	const Tile *pTLeft  = pLeft  ? GetTerrainTileAt(x-1, y) : NULL;
	const Tile *pTRight = pRight ? GetTerrainTileAt(x+1, y) : NULL;

	// find connections
	uint32 connections = 0;
	if(pUp && pUp->type == OT_Road && Tileset().FindRoad(pUp->index | 4, pTUp->terrain) != -1)
		connections |= 8;
	if(pDown && pDown->type == OT_Road && Tileset().FindRoad(pDown->index | 8, pTDown->terrain) != -1)
		connections |= 4;
	if(pLeft && pLeft->type == OT_Road && Tileset().FindRoad(pLeft->index | 1, pTLeft->terrain) != -1)
		connections |= 2;
	if(pRight && pRight->type == OT_Road && Tileset().FindRoad(pRight->index | 2, pTRight->terrain) != -1)
		connections |= 1;

	// find suitable roads
	int roads[8];
	int numRoads = Tileset().FindBestRoads(roads, connections, pTHere->terrain);
	if(numRoads == 0)
		return false;

	// choose a suitable road
	int r = roads[MFRand() % numRoads];
	connections = Tileset().GetRoadConnections(r);

	// terrain is compatible, place road
	ClearDetail(x, y);
	pHere->type = OT_Road;
	pHere->index = connections;

	MapTemplate::MapRegionTemplate::TileDetails *pTemplate = edit.templates[editRace].pMap + y*width + x;
	pTemplate->type = OT_Road;
	pTemplate->index = connections;

	// connect surrounding roads
	if(connections & 8)
	{
		if(pUp && pUp->type == OT_Road)
		{
			pUp->index |= 4;
			pTemplate[-width].index |= 4;
		}
		else if(pUp)
			PlaceRoad(x, y-1);
	}
	if(connections & 4)
	{
		if(pDown && pDown->type == OT_Road)
		{
			pDown->index |= 8;
			pTemplate[width].index |= 8;
		}
		else if(pDown)
			PlaceRoad(x, y+1);
	}
	if(connections & 2)
	{
		if(pLeft && pLeft->type == OT_Road)
		{
			pLeft->index |= 1;
			pTemplate[-1].index |= 1;
		}
		else if(pLeft)
			PlaceRoad(x-1, y);
	}
	if(connections & 1)
	{
		if(pRight && pRight->type == OT_Road)
		{
			pRight->index |= 2;
			pTemplate[1].index |= 2;
		}
		else if(pRight)
			PlaceRoad(x+1, y);
	}

	return true;
}

void EditableMap::ClearDetail(int x, int y)
{
	const ::Tileset &tileset = Tileset();

	MapTile *pTile = pMap + y*width + x;

	if(pTile->region != editRegion)
		return;

	// get the template tile
	MapTemplate::MapRegionTemplate::TileDetails *pTemplate = edit.templates[editRace].pMap + y*width + x;

	// if we removed a road, we need to correct the roads around it
	if(pTile->type == OT_Road)
	{
		// remove the road from the map
		pTile->type = OT_None;
		pTile->index = 0;

		// remove the road from the map template
		pTemplate->type = OT_None;
		pTemplate->index = 0;

		// update surrounding roads to remove connection
		if(x > 0 && pTile[-1].type == OT_Road)
		{
			pTile[-1].index &= 0xE;
			pTemplate[-1].index &= 0xE;

			if(tileset.FindRoad(pTile[-1].index, tileset.GetTile(pTile[-1].terrain)->terrain) == -1)
				ClearDetail(x-1, y);
		}
		if(x < width-1 && pTile[1].type == OT_Road)
		{
			pTile[1].index &= 0xD;
			pTemplate[1].index &= 0xD;

			if(tileset.FindRoad(pTile[1].index, tileset.GetTile(pTile[1].terrain)->terrain) == -1)
				ClearDetail(x+1, y);
		}
		if(y > 0 && pTile[-width].type == OT_Road)
		{
			pTile[-width].index &= 0xB;
			pTemplate[-width].index &= 0xB;

			if(tileset.FindRoad(pTile[-width].index, tileset.GetTile(pTile[-width].terrain)->terrain) == -1)
				ClearDetail(x, y-1);
		}
		if(y < height-1 && pTile[width].type == OT_Road)
		{
			pTile[width].index &= 0x7;
			pTemplate[width].index &= 0x7;

			if(tileset.FindRoad(pTile[width].index, tileset.GetTile(pTile[width].terrain)->terrain) == -1)
				ClearDetail(x, y+1);
		}
	}
	else if(pTile->type == OT_Castle)
	{
		int castle = pTile->index;

		// clear all 4 castle squares
		int cx = castles[castle].details.x;
		int cy = castles[castle].details.y;

		for(int a=0; a<4; ++a)
		{
			MapTile *pCastleTile = pMap + (cy + (a >> 1))*width + cx + (a & 1);
			pCastleTile->objectX = 0;
			pCastleTile->objectY = 0;
			pCastleTile->index = 0;
			pCastleTile->type = OT_None;
		}

		// correct all following castle indices
		int numTiles = width*height;
		for(int a=0; a<numTiles; ++a)
		{
			if(pMap[a].type == OT_Castle && pMap[a].index > castle)
				--pMap[a].index;
		}

		// destroy the castle
		castles.remove(castle);

		// destroy the castle template
		size_t a = 0;
		for(; a < edit.templates[editRace].castles.size(); ++a)
		{
			if(edit.templates[editRace].castles[a].x == cx && edit.templates[editRace].castles[a].y == cy)
				break;
		}
		edit.templates[editRace].castles.remove(a);
	}
	else
	{
		// remove the item from the map
		pTile->type = OT_None;
		pTile->index = 0;

		// remove it from the map template
		pTemplate->type = OT_None;
		pTemplate->index = 0;
	}
}
