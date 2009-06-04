#include "Warlords.h"
#include "Map.h"

#include "MFIni.h"
#include "MFView.h"
#include "MFMaterial.h"
#include "MFPrimitive.h"
#include "MFInput.h"
#include "MFSystem.h"
#include "MFFont.h"
#include "MFTexture.h"

Map *Map::Create(const char *pMapFilename)
{
	MFIni *pIni = MFIni::Create(pMapFilename);
	if(!pIni)
		return NULL;

	Map *pMap = NULL;

	MFIniLine *pLine = pIni->GetFirstLine();
	while(pLine)
	{
		if(pLine->IsSection("Map"))
		{
			pMap = (Map*)MFHeap_AllocAndZero(sizeof(Map));

			MFIniLine *pMapLine = pLine->Sub();

			while(pMapLine)
			{
				if(pMapLine->IsString(0, "name"))
				{
					MFString_Copy(pMap->name, pMapLine->GetString(1));
				}
				else if(pMapLine->IsString(0, "tileset"))
				{
					pMap->pTiles = Tileset::Create(pMapLine->GetString(1));
				}
				else if(pMapLine->IsString(0, "map_width"))
				{
					pMap->mapWidth = pMapLine->GetInt(1);
				}
				else if(pMapLine->IsString(0, "map_height"))
				{
					pMap->mapHeight = pMapLine->GetInt(1);
				}
				else if(pMapLine->IsSection("Tiles"))
				{
					MFDebug_Assert(pMap->mapWidth && pMap->mapHeight, "Invalid map dimensions");

					pMap->pMap = (MapTile*)MFHeap_AllocAndZero(pMap->mapWidth * pMap->mapHeight * sizeof(MapTile));

					int i = 0;

					MFIniLine *pTiles = pMapLine->Sub();
					while(pTiles)
					{
						MFDebug_Assert(pTiles->GetStringCount() == pMap->mapWidth, "Not enough tiles in row.");

						for(int a=0; a<pMap->mapWidth; ++a)
							pMap->pMap[i++].terrain = pTiles->GetInt(a);

						pTiles = pTiles->Next();
					}

					MFDebug_Assert(i == pMap->mapWidth * pMap->mapHeight, "Not enough rows.");
				}
				else if(pMapLine->IsSection("Objects"))
				{
				}

				pMapLine = pMapLine->Next();
			}
		}

		pLine = pLine->Next();
	}

	pMap->zoom = 1.0f;

	// if we want to edit the map, we need some memory for map traversal
	if(1)
	{
		pMap->pTouched = (uint8*)MFHeap_AllocAndZero(pMap->mapWidth * pMap->mapHeight * sizeof(*pMap->pTouched));
		pMap->pChangeList = (MapCoord*)MFHeap_Alloc(sizeof(MapCoord)*1024);
		pMap->numChanges = 0;
	}

	pMap->pRenderTarget = NULL;
	pMap->pMinimap = NULL;

	return pMap;
}

Map *Map::CreateNew(const char *pTileset, const char *pCastles)
{
	Map *pNew = (Map*)MFHeap_AllocAndZero(sizeof(Map));
	pNew = new(pNew) Map;

	MFString_Copy(pNew->name, "Untitled");

	pNew->mapWidth = 128;
	pNew->mapHeight = 128;
	pNew->pMap = (MapTile*)MFHeap_AllocAndZero(pNew->mapWidth * pNew->mapHeight * sizeof(MapTile));

	pNew->pTiles = Tileset::Create(pTileset);
	pNew->pCastles = CastleSet::Create(pCastles);

	// get default tiles
	int tiles[8];
	int numVariants = pNew->pTiles->FindBestTiles(tiles, 0);

	for(int y=0; y<pNew->mapHeight; ++y)
	{
		MapTile *pRow = pNew->pMap + y*pNew->mapWidth;
		for(int x=0; x<pNew->mapWidth; ++x)
			pRow[x].terrain = tiles[MFRand()%numVariants];
	}

	pNew->zoom = 1.f;
/*
	int width, height;
	pNew->pTiles->GetTileSize(&width, &height);

	int screenWidth = gDefaults.display.displayWidth + width + (width-1);
	int screenHeight = gDefaults.display.displayHeight + height + (height-1);
	screenWidth -= screenWidth % width;
	screenHeight -= screenHeight % height;

	pNew->pRenderTarget = MFTexture_CreateRenderTarget("Map", screenWidth, screenHeight);
	pNew->pMinimap = MFTexture_CreateDynamic("Minimap", pNew->mapWidth * 2, pNew->mapHeight * 2, TexFmt_A8R8G8B8);
*/
	// editor stuff
	pNew->pTouched = (uint8*)MFHeap_AllocAndZero(pNew->mapWidth * pNew->mapHeight * sizeof(*pNew->pTouched));
	pNew->pChangeList = (MapCoord*)MFHeap_Alloc(sizeof(MapCoord)*1024);
	pNew->numChanges = 0;

	return pNew;
}

void Map::Destroy()
{
	pTiles->Destroy();
	pCastles->Destroy();
	MFHeap_Free(pMap);
	MFHeap_Free(this);
}

void Map::GetVisibleTileSize(float *pWidth, float *pHeight)
{
	int tileWidth, tileHeight;
	pTiles->GetTileSize(&tileWidth, &tileHeight);

	if(pWidth)
		*pWidth = tileWidth*zoom;
	if(pHeight)
		*pHeight = tileHeight*zoom;
}

void Map::GetCursor(int *pX, int *pY)
{
	float tileWidth, tileHeight;
	GetVisibleTileSize(&tileWidth, &tileHeight);

	*pX = (int)(xOffset + MFInput_Read(Mouse_XPos, IDD_Mouse) / tileWidth);
	*pY = (int)(yOffset + MFInput_Read(Mouse_YPos, IDD_Mouse) / tileHeight);
}

int Map::UpdateInput()
{
	float tileWidth, tileHeight;
	GetVisibleTileSize(&tileWidth, &tileHeight);

	float mouseZoom = MFInput_Read(Mouse_Wheel, IDD_Mouse);
	if(mouseZoom)
	{
		float mouseX = MFInput_Read(Mouse_XPos, IDD_Mouse);
		float mouseY = MFInput_Read(Mouse_YPos, IDD_Mouse);
		SetZoom(zoom + 0.25f*mouseZoom, mouseX/tileWidth, mouseY/tileHeight);
	}

	if(MFInput_WasReleased(Mouse_RightButton, IDD_Mouse))
	{
		bIsDragging = false;
		ReleaseExclusive();
	}

	if(bIsDragging)
	{
		float xDelta = -MFInput_Read(Mouse_XDelta, IDD_Mouse);
		float yDelta = -MFInput_Read(Mouse_YDelta, IDD_Mouse);

		SetOffset(xOffset + xDelta/tileWidth, yOffset + yDelta/tileHeight);
	}

	if(MFInput_WasPressed(Mouse_RightButton, IDD_Mouse))
	{
		bIsDragging = true;
		SetExclusive();
	}

	return 0;
}

void Map::Update()
{
}

void Map::Draw()
{
	MFView_Push();

	int xStart = (int)xOffset;
	int yStart = (int)yOffset;

	int xTiles, yTiles;
	SetMapOrtho(&xTiles, &yTiles);

	MapTile *pStart = &pMap[yStart*mapWidth + xStart];

	// blit map portion to a render target
	pTiles->DrawMap(xTiles, yTiles, &pStart->terrain, sizeof(MapTile), mapWidth);

	// now we should, like, render all the extra stuff, except the roads
	for(int y=0; y<yTiles; ++y)
	{
		for(int x=0; x<xTiles; ++x)
		{
			MapTile *pTile = pStart + x;

			if(pTile->type == OT_None)
				continue;

			float tileWidth = 1.f;
			MFMaterial *pMat;
			MFRect uvs;

			if(pTile->type == OT_Road)
			{
				pMat = pCastles->GetRoadMaterial();

				int r = pCastles->FindRoad(pTile->index, GetTerrain(xStart+x, yStart+y));
				MFDebug_Assert(r >= 0, "Invalid road!");

				pCastles->GetRoadUVs(r, &uvs);
			}
			else
			{
				pMat = pCastles->GetCastleMaterial();

				switch(pTile->type)
				{
					case OT_Castle:
						pCastles->GetCastleUVs(pTile->index, &uvs);
						tileWidth = 2.f;
						break;
					case OT_Flag:
						pCastles->GetFlagUVs(pTile->index, &uvs);
						break;
					case OT_Special:
						pCastles->GetSpecialUVs(pTile->index, &uvs);
						break;
				}
			}

			MFMaterial_SetMaterial(pMat);
			MFPrimitive_DrawQuad((float)x, (float)y, tileWidth, tileWidth, MFVector::one, uvs.x, uvs.y, uvs.x + uvs.width, uvs.y + uvs.height);
		}

		pStart += mapWidth;
	}

	// and now the units

	// the tilemap should render to an image and we render it to the screen here...
	//...

	MFView_Pop();
}

void Map::DrawDebug()
{
	MFView_Push();

	int xTiles, yTiles;
	SetMapOrtho(&xTiles, &yTiles);

	float tileWidth, tileHeight;
	GetVisibleTileSize(&tileWidth, &tileHeight);

	int cursorX = (int)(xOffset + MFInput_Read(Mouse_XPos, IDD_Mouse) / tileWidth);
	int cursorY = (int)(yOffset + MFInput_Read(Mouse_YPos, IDD_Mouse) / tileHeight);

	int xOffsetI = (int)xOffset;
	int yOffsetI = (int)yOffset;

	MFPrimitive(PT_TriStrip|PT_Untextured);
	MFBegin(4);
	MFSetColour(0,0,0,.2f);
	MFSetPosition((float)(cursorX - xOffsetI), (float)(cursorY - yOffsetI), 0);
	MFSetPosition((float)(cursorX - xOffsetI + 1), (float)(cursorY - yOffsetI), 0);
	MFSetPosition((float)(cursorX - xOffsetI), (float)(cursorY - yOffsetI + 1), 0);
	MFSetPosition((float)(cursorX - xOffsetI + 1), (float)(cursorY - yOffsetI + 1), 0);
	MFEnd();

	MFPrimitive(PT_LineList|PT_Untextured);
	MFBegin(xTiles*2 + yTiles*2 + 4);
	MFSetColour(0.f, 0.4f, 1.f, 1.f);

	for(int x=0; x<=xTiles; ++x)
	{
		MFSetPosition((float)x, 0, 0);
		MFSetPosition((float)x, (float)yTiles, 0);
	}
	for(int y=0; y<=yTiles; ++y)
	{
		MFSetPosition(0, (float)y, 0);
		MFSetPosition((float)xTiles, (float)y, 0);
	}

	MFEnd();

	MFView_Pop();

	MFFont_DrawTextf(MFFont_GetDebugFont(), MakeVector(8, 8), 24, MFVector::yellow, "%d, %d", cursorX, cursorY);
}

void Map::SetMapOrtho(int *pXTiles, int *pYTiles)
{
	MFRect screenRect;
	MFView_GetOrthoRect(&screenRect);

	float tileWidth, tileHeight;
	GetVisibleTileSize(&tileWidth, &tileHeight);

	float screenWidth = screenRect.width / tileWidth;
	float screenHeight = screenRect.height / tileHeight;

	MFRect rect;
	rect.x = xOffset - (float)(int)xOffset;
	rect.y = yOffset - (float)(int)yOffset;
	rect.width = screenWidth;
	rect.height = screenHeight;
	MFView_SetOrtho(&rect);

	if(pXTiles)
		*pXTiles = (int)MFCeil(screenWidth + 1.f);
	if(pYTiles)
		*pYTiles = (int)MFCeil(screenHeight + 1.f);
}

void Map::SetOffset(float x, float y)
{
	float tileWidth, tileHeight;
	GetVisibleTileSize(&tileWidth, &tileHeight);

	float maxX = mapWidth - gDefaults.display.displayWidth / tileWidth;
	float maxY = mapHeight - gDefaults.display.displayHeight / tileHeight;

	xOffset = MFClamp(0.f, x, maxX);
	yOffset = MFClamp(0.f, y, maxY);
}

void Map::CenterView(int x, int y)
{
	float tileWidth, tileHeight;
	GetVisibleTileSize(&tileWidth, &tileHeight);

	float screenWidth = gDefaults.display.displayWidth / tileWidth;
	float screenHeight = gDefaults.display.displayHeight / tileHeight;

	xOffset = (float)x + 0.5f - screenWidth*0.5f;
	yOffset = (float)y + 0.5f - screenWidth*0.5f;

	xOffset = MFClamp(0.f, xOffset, mapWidth - screenWidth);
	yOffset = MFClamp(0.f, yOffset, mapHeight - screenHeight);
}

void Map::SetZoom(float _zoom, float pointX, float pointY)
{
	float tileWidth, tileHeight;
	GetVisibleTileSize(&tileWidth, &tileHeight);

	if(pointX < 0.f)
		pointX = gDefaults.display.displayWidth / tileWidth * 0.5f;
	if(pointY < 0.f)
		pointY = gDefaults.display.displayHeight / tileHeight * 0.5f;

	float newZoom = MFClamp(0.25f, _zoom, 2.f);
	float zoomDiff = zoom / newZoom;
	zoom = newZoom;

	// calculate new position
	xOffset += pointX - pointX*zoomDiff;
	yOffset += pointY - pointY*zoomDiff;

	// calculate clamps
	GetVisibleTileSize(&tileWidth, &tileHeight);
	float maxX = mapWidth - gDefaults.display.displayWidth / tileWidth;
	float maxY = mapHeight - gDefaults.display.displayHeight / tileHeight;

	// clamp the new position
	xOffset = MFClamp(0.f, xOffset, maxX);
	yOffset = MFClamp(0.f, yOffset, maxY);
}

bool Map::SetTile(int x, int y, uint32 tile, uint32 mask)
{
	if(tile == GetTerrain(x, y))
 		return true;

	int tiles[8];
	int matches = pTiles->FindBestTiles(tiles, tile, mask);
	MFDebug_Assert(matches, MFStr("Couldn't find matching tile at (%d, %d): %d, %d, %d, %d", x, y, tile & 0xFF, (tile >> 8) & 0xFF, (tile >> 16) & 0xFF, (tile >> 24) & 0xFF));
	if(!matches)
		return false;

	// clear any existing stuff on the tile
	ClearDetail(x, y);

	// TODO: use some logic to refine the selection based on quickest/valid route to target
	int t = tiles[MFRand()%matches];
	pMap[y*mapWidth + x].terrain = t;

	// mark the tile touched
	pTouched[y*mapWidth + x] = 1;

	// update adjacent tiles
	if(y > 0 && !pTouched[(y-1)*mapWidth + x])
	{
		pChangeList[numChanges].x = x;
		pChangeList[numChanges].y = y-1;
		++numChanges;
	}
	if(y < mapHeight-1 && !pTouched[(y+1)*mapWidth + x])
	{
		pChangeList[numChanges].x = x;
		pChangeList[numChanges].y = y+1;
		++numChanges;
	}
	if(x > 0 && !pTouched[y*mapWidth + x - 1])
	{
		pChangeList[numChanges].x = x-1;
		pChangeList[numChanges].y = y;
		++numChanges;
	}
	if(x < mapWidth-1 && !pTouched[y*mapWidth + x + 1])
	{
		pChangeList[numChanges].x = x+1;
		pChangeList[numChanges].y = y;
		++numChanges;
	}

	return true;
}

bool Map::SetTerrain(int x, int y, int tl, int tr, int bl, int br, uint32 mask)
{
	MFZeroMemory(pTouched, mapWidth * mapHeight);
	numChanges = 0;

	if(!SetTile(x, y, EncodeTile(tl, tr, bl, br), mask))
		return false;

//	MFDebug_Assert(numChanges, "!");

	for(int a=0; a<numChanges; ++a)
	{
		x = pChangeList[a].x;
		y = pChangeList[a].y;
		DecodeTile(GetTerrain(x, y), &tl, &tr, &bl, &br);

		// perform a bunch of logic to find a tile type suggestion...
		int tlm = 0, trm = 0, blm = 0, brm = 0;

		// update adjacent tiles
		if(y > 0 && pTouched[(y-1)*mapWidth + x])
		{
			int t = GetTerrain(x, y-1);
			tl = DecodeBL(t);
			tr = DecodeBR(t);
			tlm = 0xFF;
			trm = 0xFF;
		}
		if(y < mapHeight-1 && pTouched[(y+1)*mapWidth + x])
		{
			int b = GetTerrain(x, y+1);
			bl = DecodeTL(b);
			br = DecodeTR(b);
			blm = 0xFF;
			brm = 0xFF;
		}
		if(x > 0 && pTouched[y*mapWidth + x - 1])
		{
			int l = GetTerrain(x-1, y);
			tl = DecodeTR(l);
			bl = DecodeBR(l);
			tlm = 0xFF;
			blm = 0xFF;
		}
		if(x < mapWidth-1 && pTouched[y*mapWidth + x + 1])
		{
			int r = GetTerrain(x+1, y);
			tr = DecodeTL(r);
			br = DecodeBL(r);
			trm = 0xFF;
			brm = 0xFF;
		}

		SetTile(x, y, EncodeTile(tl, tr, bl, br), EncodeTile(tlm, trm, blm, brm));
	}

	return true;
}

int Map::UpdateChange(int a)
{
	if(a>=numChanges)
		return numChanges;

	int x, y, tl, tr, bl, br;

	x = pChangeList[a].x;
	y = pChangeList[a].y;
	DecodeTile(GetTerrain(x, y), &tl, &tr, &bl, &br);

	// perform a bunch of logic to find a tile type suggestion...
	int tlm = 0, trm = 0, blm = 0, brm = 0;

	// update adjacent tiles
	if(y > 0 && pTouched[(y-1)*mapWidth + x])
	{
		int t = GetTerrain(x, y-1);
		tl = DecodeBL(t);
		tr = DecodeBR(t);
		tlm = 0xFF;
		trm = 0xFF;
	}
	if(y < mapHeight-1 && pTouched[(y+1)*mapWidth + x])
	{
		int b = GetTerrain(x, y+1);
		bl = DecodeTL(b);
		br = DecodeTR(b);
		blm = 0xFF;
		brm = 0xFF;
	}
	if(x > 0 && pTouched[y*mapWidth + x - 1])
	{
		int l = GetTerrain(x-1, y);
		tl = DecodeTR(l);
		bl = DecodeBR(l);
		tlm = 0xFF;
		blm = 0xFF;
	}
	if(x < mapWidth-1 && pTouched[y*mapWidth + x + 1])
	{
		int r = GetTerrain(x+1, y);
		tr = DecodeTL(r);
		br = DecodeBL(r);
		trm = 0xFF;
		brm = 0xFF;
	}

	SetTile(x, y, EncodeTile(tl, tr, bl, br), EncodeTile(tlm, trm, blm, brm));

	return ++a;
}

bool Map::PlaceCastle(int x, int y, int race)
{
	// check we have room on the map
	if(x >= mapWidth - 1 || y >= mapHeight - 1)
		return false;

	// get the terrain tiles
	Tile *pT0 = GetTile(x, y);
	Tile *pT1 = GetTile(x+1, y);
	Tile *pT2 = GetTile(x, y+1);
	Tile *pT3 = GetTile(x+1, y+1);

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
	pMap[y*mapWidth + x].type = OT_Castle;
	pMap[y*mapWidth + x].index = race;

	return true;
}

bool Map::PlaceFlag(int x, int y, int race)
{
	// check for compatible terrain
	Tile *pT = GetTile(x, y);

	// check if we can build on this terrain type
	if(!pT->canBuild)
		return false;

	// clear the tile
	ClearDetail(x, y);

	// terrain is compatible, place flag
	pMap[y*mapWidth + x].type = OT_Flag;
	pMap[y*mapWidth + x].index = race;

	return true;
}

bool Map::PlaceSpecial(int x, int y, int index)
{
	// check for compatible terrain
	Tile *pT = GetTile(x, y);

	// check if the terrain type matches the special target.
	// TODO: for now, place on flat land...
	if(!pT->canBuild)
		return false;

	ClearDetail(x, y);

	// terrain is compatible, place special
	pMap[y*mapWidth + x].type = OT_Special;
	pMap[y*mapWidth + x].index = index;

	return true;
}

bool Map::PlaceRoad(int x, int y)
{
	if(pMap[y*mapWidth + x].type == OT_Road)
		return true;

	// check for compatible terrain
	Tile *pT = GetTile(x, y);

	// HACK: just assume grass for now
	if(pT->terrain != 0)
		return false;

	// terrain is compatible, place road
	ClearDetail(x, y);
	pMap[y*mapWidth + x].type = OT_Road;
	pMap[y*mapWidth + x].index = 0;

	// connect surrounding roads
	if(x > 0 && pMap[y*mapWidth + x-1].type == OT_Road)
	{
		// TODO: we need to find if the required intersections exists, and if so, connect them
		pMap[y*mapWidth + x].index |= 2;
		pMap[y*mapWidth + x-1].index |= 1;
	}
	if(x < mapWidth-1 && pMap[y*mapWidth + x+1].type == OT_Road)
	{
		// TODO: we need to find if the required intersections exists, and if so, connect them
		pMap[y*mapWidth + x].index |= 1;
		pMap[y*mapWidth + x+1].index |= 2;
	}
	if(y > 0 && pMap[(y-1)*mapWidth + x].type == OT_Road)
	{
		// TODO: we need to find if the required intersections exists, and if so, connect them
		pMap[y*mapWidth + x].index |= 8;
		pMap[(y-1)*mapWidth + x].index |= 4;
	}
	if(y < mapWidth-1 && pMap[(y+1)*mapWidth + x].type == OT_Road)
	{
		// TODO: we need to find if the required intersections exists, and if so, connect them
		pMap[y*mapWidth + x].index |= 4;
		pMap[(y+1)*mapWidth + x].index |= 8;
	}

	return true;
}

void Map::ClearDetail(int x, int y)
{
	uint8 oldType = pMap[y*mapWidth + x].type;

	// remove the item from the map
	pMap[y*mapWidth + x].type = OT_None;
	pMap[y*mapWidth + x].index = 0;

	// remove the item from this tile
	if(oldType == OT_Road)
	{
		// update surrounding roads to remove connection
		if(x > 0 && pMap[y*mapWidth + x-1].type == OT_Road)
		{
			// TODO: we need to find if the new piece exists, and if not, remove the road altogether
			pMap[y*mapWidth + x-1].index &= 0xE;
		}
		if(x < mapWidth-1 && pMap[y*mapWidth + x+1].type == OT_Road)
		{
			// TODO: we need to find if the new piece exists, and if not, remove the road altogether
			pMap[y*mapWidth + x+1].index &= 0xD;
		}
		if(y > 0 && pMap[(y-1)*mapWidth + x].type == OT_Road)
		{
			// TODO: we need to find if the new piece exists, and if not, remove the road altogether
			pMap[(y-1)*mapWidth + x].index &= 0xB;
		}
		if(y < mapWidth-1 && pMap[(y+1)*mapWidth + x].type == OT_Road)
		{
			// TODO: we need to find if the new piece exists, and if not, remove the road altogether
			pMap[(y+1)*mapWidth + x].index &= 0x7;
		}
	}

	// check if we are in the space of an oversize castle
	if(x > 0 && pMap[y*mapWidth + x-1].type == OT_Castle)
	{
		pMap[y*mapWidth + x-1].type = OT_None;
		pMap[y*mapWidth + x-1].index = 0;
	}
	else if(y > 0 && pMap[(y-1)*mapWidth + x].type == OT_Castle)
	{
		pMap[(y-1)*mapWidth + x].type = OT_None;
		pMap[(y-1)*mapWidth + x].index = 0;
	}
	else if(x > 0 && y > 0 && pMap[(y-1)*mapWidth + x-1].type == OT_Castle)
	{
		pMap[(y-1)*mapWidth + x-1].type = OT_None;
		pMap[(y-1)*mapWidth + x-1].index = 0;
	}
}
