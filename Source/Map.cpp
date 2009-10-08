#include "Warlords.h"
#include "Map.h"
#include "Castle.h"

#include "Display.h"
#include "MFRenderer.h"
#include "MFIni.h"
#include "MFView.h"
#include "MFMaterial.h"
#include "MFPrimitive.h"
#include "MFInput.h"
#include "MFSystem.h"
#include "MFFont.h"
#include "MFTexture.h"
#include "MFFileSystem.h"

#include "stdio.h"

static const char * const pObjectTypes[] =
{
	"none",
	"terrain",
	"castle",
	"flag",
	"special",
	"road"
};

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
			pMap = new(pMap) Map;

			MFIniLine *pMapLine = pLine->Sub();

			while(pMapLine)
			{
				if(pMapLine->IsString(0, "name"))
				{
					MFString_Copy(pMap->name, pMapLine->GetString(1));
				}
				else if(pMapLine->IsString(0, "tileset"))
				{
					MFString_Copy(pMap->tileset, pMapLine->GetString(1));
					pMap->pTiles = Tileset::Create(pMapLine->GetString(1));
				}
				else if(pMapLine->IsString(0, "units"))
				{
					MFString_Copy(pMap->units, pMapLine->GetString(1));
          pMap->pUnits = UnitDefinitions::Load(pMapLine->GetString(1));
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
							pMap->pMap[i++].terrain = MFString_AsciiToInteger(pTiles->GetString(a), false, 16);

						pTiles = pTiles->Next();
					}

					MFDebug_Assert(i == pMap->mapWidth * pMap->mapHeight, "Not enough rows.");
				}
				else if(pMapLine->IsSection("Details"))
				{
					MFIniLine *pDetails = pMapLine->Sub();
					while(pDetails)
					{
						int x = pDetails->GetInt(0);
						int y = pDetails->GetInt(1);
						MFDebug_Assert(!MFString_Compare(pDetails->GetString(2), "="), "Expected: '='");

						MapTile &tile = pMap->pMap[y*pMap->mapWidth + x];

						for(int a=0; a<OT_Max; ++a)
						{
							if(!MFString_CaseCmp(pDetails->GetString(3), pObjectTypes[a]))
							{
								tile.type = a;
								break;
							}
						}
						tile.index = pDetails->GetInt(4);

						pDetails = pDetails->Next();
					}
				}
				else if(pMapLine->IsSection("Castles"))
				{
					MFIniLine *pCastles = pMapLine->Sub();

					while(pCastles)
					{
						if(pCastles->IsSection("Castle"))
						{
							MFIniLine *pCastle = pCastles->Sub();

							char name[32];
							int x = 0;
							int y = 0;
							int race;

							while(pCastle)
							{
								if(!MFString_CaseCmp(pCastle->GetString(0), "name"))
								{
									MFString_Copy(name, pCastle->GetString(1));
								}
								else if(!MFString_CaseCmp(pCastle->GetString(0), "position"))
								{
									x = pCastle->GetInt(1);
									y = pCastle->GetInt(2);
								}
								else if(!MFString_CaseCmp(pCastle->GetString(0), "race"))
								{
									race = pCastle->GetInt(1);
								}

								pCastle = pCastle->Next();
							}

							MapTile &tile = pMap->pMap[y*pMap->mapWidth + x];
							tile.type = OT_Castle;
							tile.index = race;

							// insert additional map data...
						}

						pCastles = pCastles->Next();
					}
				}

				pMapLine = pMapLine->Next();
			}
		}

		pLine = pLine->Next();
	}

	if(!pMap)
		return NULL;

	pMap->path.Init(pMap);

	pMap->zoom = 1.0f;

	// if we want to edit the map, we need some memory for map traversal
	if(1)
	{
		pMap->pTouched = (uint8*)MFHeap_AllocAndZero(pMap->mapWidth * pMap->mapHeight * sizeof(*pMap->pTouched));
		pMap->pChangeList = (MapCoord*)MFHeap_Alloc(sizeof(MapCoord)*1024);
		pMap->numChanges = 0;
	}

	int tileWidth, tileHeight;
	MFRect screen;
	pMap->pTiles->GetTileSize(&tileWidth, &tileHeight);
	MFDisplay_GetDisplayRect(&screen);

	const int maxZoom = 2;
	int rtWidth = (int)screen.width*maxZoom+tileWidth*2-1;
	int rtHeight = (int)screen.height*maxZoom+tileHeight*2-1;
	rtWidth -= rtWidth%tileWidth;
	rtHeight -= rtHeight%tileHeight;
	rtWidth = MFUtil_NextPowerOf2(rtWidth);
	rtHeight = MFUtil_NextPowerOf2(rtHeight);
	pMap->pRenderTarget = MFTexture_CreateRenderTarget("MapSurface", rtWidth, rtHeight);
	pMap->pRenderTargetMaterial = MFMaterial_Create("MapSurface");

//	pMap->pMinimap = MFTexture_CreateRenderTarget("MiniMap", MFUtil_NextPowerOf2(pMap->mapWidth), MFUtil_NextPowerOf2(pMap->mapHeight));
//	pMap->pMinimapMaterial = MFMaterial_Create("MiniMap");
	pMap->pMinimap = NULL;
	pMap->pMinimapMaterial = NULL;

	pMap->bRightMove = false;

	return pMap;
}

int Map::ChooseTile(int *pSelectedTiles, int numVariants)
{
	if(numVariants == 1)
		return pSelectedTiles[0];

	int t, total = 0;
	for(t=0; t<numVariants; ++t)
		total += pTiles->GetTile(pSelectedTiles[t])->bias;

	int selection = (int)(MFRand() % total);
	for(t=0; t<numVariants; ++t)
	{
		int tile = pSelectedTiles[t];
		selection -= pTiles->GetTile(tile)->bias;

		if(selection < 0)
			return tile;
	}

	return 0;
}

Map *Map::CreateNew(const char *pTileset, const char *pUnits)
{
	Map *pNew = (Map*)MFHeap_AllocAndZero(sizeof(Map));
	pNew = new(pNew) Map;

	MFString_Copy(pNew->name, "Untitled");
	MFString_Copy(pNew->tileset, pTileset);
	MFString_Copy(pNew->units, pUnits);

	pNew->mapWidth = 128;
	pNew->mapHeight = 128;
	pNew->pMap = (MapTile*)MFHeap_AllocAndZero(pNew->mapWidth * pNew->mapHeight * sizeof(MapTile));

	pNew->pTiles = Tileset::Create(pTileset);
	pNew->pUnits = UnitDefinitions::Load(pUnits);

	pNew->path.Init(pNew);

	// get default tiles
	int tiles[8];
	int numVariants = pNew->pTiles->FindBestTiles(tiles, 0);

	for(int y=0; y<pNew->mapHeight; ++y)
	{
		MapTile *pRow = pNew->pMap + y*pNew->mapWidth;
		for(int x=0; x<pNew->mapWidth; ++x)
			pRow[x].terrain = pNew->ChooseTile(tiles, numVariants);
	}

	pNew->zoom = 1.f;

	int tileWidth, tileHeight;
	MFRect screen;
	pNew->pTiles->GetTileSize(&tileWidth, &tileHeight);
	MFDisplay_GetDisplayRect(&screen);
	int rtWidth = (int)screen.width*4+tileWidth*2-1;
	int rtHeight = (int)screen.height*4+tileHeight*2-1;
	rtWidth -= rtWidth%tileWidth;
	rtHeight -= rtHeight%tileHeight;
	rtWidth = MFUtil_NextPowerOf2(rtWidth);
	rtHeight = MFUtil_NextPowerOf2(rtHeight);
	pNew->pRenderTarget = MFTexture_CreateRenderTarget("MapSurface", rtWidth, rtHeight);

//	pNew->pMinimap = MFTexture_CreateRenderTarget("MiniMap", MFUtil_NextPowerOf2(pMap->mapWidth), MFUtil_NextPowerOf2(pMap->mapHeight));
	pNew->pMinimap = NULL;

	// editor stuff
	pNew->pTouched = (uint8*)MFHeap_AllocAndZero(pNew->mapWidth * pNew->mapHeight * sizeof(*pNew->pTouched));
	pNew->pChangeList = (MapCoord*)MFHeap_Alloc(sizeof(MapCoord)*1024);
	pNew->numChanges = 0;

	pNew->bRightMove = false;

	return pNew;
}

void Map::Destroy()
{
	path.Deinit();

	pTiles->Destroy();
	pUnits->Free();
	MFHeap_Free(pMap);
	MFHeap_Free(this);
}

void Map::Save(const char *pFilename)
{
	MFFile *pFile = MFFileSystem_Open(MFStr("game:%s", pFilename), MFOF_Write|MFOF_Binary);
	if(!pFile)
		return;

	// write the map to disk...
	const char *pMapData = MFStr(
		"[Map]\n"
		"{\n"
		"\tname = %s\n\n"
		"\ttileset = %s\n"
		"\tunits = %s\n\n"
		"\tmap_width = %d\n"
		"\tmap_Height = %d\n"
		"\n"
		"\t[Tiles]\n"
		"\t{\n",
		name, tileset, units, mapWidth, mapHeight);
	int len = MFString_Length(pMapData);
	MFFile_Write(pFile, pMapData, len);

	char buffer[2048];
	for(int a=0; a<mapHeight; ++a)
	{
		MFString_Copy(buffer, "\t\t");
		int offset = MFString_Length(buffer);

		for(int b=0; b<mapWidth; ++b)
		{
			offset += sprintf(&buffer[offset], "%02x", pMap[a*mapWidth + b].terrain);
			buffer[offset++] = b < mapWidth-1 ? ',' : '\n';
		}

		MFFile_Write(pFile, buffer, offset);
	}

	const char *pMapData2 =
		"\t}\n"
		"\n"
		"\t[Details]\n"
		"\t{\n";
	MFFile_Write(pFile, pMapData2, MFString_Length(pMapData2));

	for(int a=0; a<mapHeight; ++a)
	{
		for(int b=0; b<mapWidth; ++b)
		{
			int type = pMap[a*mapWidth + b].type;
			if(type && type != OT_Castle)
			{
				int len = sprintf(buffer, "\t\t%d, %d = %s %d\n", b, a, pObjectTypes[type], pMap[a*mapWidth + b].index);
				MFFile_Write(pFile, buffer, len);
			}
		}
	}

	const char *pMapData3 =
		"\t}\n"
		"\n"
		"\t[Castles]\n"
		"\t{\n";
	MFFile_Write(pFile, pMapData3, MFString_Length(pMapData3));

	for(int a=0; a<mapHeight; ++a)
	{
		for(int b=0; b<mapWidth; ++b)
		{
			if(pMap[a*mapWidth + b].type == OT_Castle)
			{
				int race = pMap[a*mapWidth + b].index;
				int len = sprintf(buffer, "\t\t[Castle]\n\t\t{\n\t\t\tname = \"%s\"\n\t\t\tposition = %d, %d\n\t\t\trace = %d\n\t\t}\n", "unnamed", b, a, race);
				MFFile_Write(pFile, buffer, len);
			}
		}
	}

	const char *pMapData4 =
		"\t}\n"
		"}\n";
	MFFile_Write(pFile, pMapData4, MFString_Length(pMapData4));

	MFFile_Close(pFile);
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

	if(MFInput_WasReleased(bRightMove ? Mouse_RightButton : Mouse_LeftButton, IDD_Mouse))
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

	if(MFInput_WasPressed(bRightMove ? Mouse_RightButton : Mouse_LeftButton, IDD_Mouse))
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

	MFRenderer_SetRenderTarget(pRenderTarget, NULL);

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
				pMat = pTiles->GetRoadMaterial();

				int r = pTiles->FindRoad(pTile->index, GetTerrain(xStart+x, yStart+y));
				MFDebug_Assert(r >= 0, "Invalid road!");

				pTiles->GetRoadUVs(r, &uvs);
			}
			else
			{
				pMat = pUnits->GetCastleMaterial();

				switch(pTile->type)
				{
					case OT_Castle:
						pUnits->GetCastleUVs(pTile->index, &uvs);
						tileWidth = 2.f;
						break;
					case OT_Flag:
						pUnits->GetFlagUVs(pTile->index, &uvs);
						break;
					case OT_Special:
						pUnits->GetSpecialUVs(pTile->index, &uvs);
						break;
				}
			}

			MFMaterial_SetMaterial(pMat);
			MFPrimitive_DrawQuad((float)x, (float)y, tileWidth, tileWidth, MFVector::one, uvs.x, uvs.y, uvs.x + uvs.width, uvs.y + uvs.height);
		}

		pStart += mapWidth;
	}

	// and now the units

	MFRenderer_SetDeviceRenderTarget();

	MFRect orthoRect;
	orthoRect.x = orthoRect.y = 0;
	orthoRect.width = orthoRect.height = 1;
	MFView_SetOrtho(&orthoRect);

	MFRect uvs;
	int targetWidth, targetHeight;
	int tileWidth, tileHeight;

	MFDisplay_GetDisplayRect(&uvs);
	MFTexture_GetTextureDimensions(pRenderTarget, &targetWidth, &targetHeight);
	pTiles->GetTileSize(&tileWidth, &tileHeight);

	float texelOffset = zoom <= 0.5f ? 1.f : 0.5f;
	uvs.x = (xOffset - (float)(int)xOffset) * (tileWidth / targetWidth) + (texelOffset/targetWidth);
	uvs.y = (yOffset - (float)(int)yOffset) * (tileHeight / targetHeight) + (texelOffset/targetHeight);
	uvs.width = uvs.width / targetWidth / zoom;
	uvs.height = uvs.height / targetHeight / zoom;

	MFMaterial_SetMaterial(pRenderTargetMaterial);
	MFPrimitive_DrawQuad(0, 0, 1, 1, MFVector::one, uvs.x, uvs.y, uvs.x + uvs.width, uvs.y + uvs.height);

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
	int targetWidth, targetHeight;
	int tileWidth, tileHeight;
	MFTexture_GetTextureDimensions(pRenderTarget, &targetWidth, &targetHeight);
	pTiles->GetTileSize(&tileWidth, &tileHeight);

	float screenWidth = (float)targetWidth / (float)tileWidth;
	float screenHeight = (float)targetHeight / (float)tileHeight;

	xOffset = (int)(xOffset*tileWidth) / (float)tileWidth;
	yOffset = (int)(yOffset*tileWidth) / (float)tileWidth;

	MFRect rect;
	rect.x = xOffset - (float)(int)xOffset;
	rect.y = yOffset - (float)(int)yOffset;
	rect.width = screenWidth;
	rect.height = screenHeight;
	MFView_SetOrtho(&rect);

	MFRect screenRect;
	MFDisplay_GetDisplayRect(&screenRect);

	if(pXTiles)
		*pXTiles = (int)MFCeil((screenRect.width / tileWidth) / zoom + 1.f);
	if(pYTiles)
		*pYTiles = (int)MFCeil((screenRect.height / tileHeight) / zoom + 1.f);
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

	float newZoom = MFClamp(0.5f, _zoom, 2.f);
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
	int t = ChooseTile(tiles, matches);
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

ObjectType Map::GetDetailType(int x, int y)
{
	if(pMap[y*mapWidth + x].type)
		return (ObjectType)pMap[y*mapWidth + x].type;

	// check if we are in the space of an oversize castle
	if((x > 0 && pMap[y*mapWidth + x-1].type == OT_Castle) ||
		(y > 0 && pMap[(y-1)*mapWidth + x].type == OT_Castle) ||
		(x > 0 && y > 0 && pMap[(y-1)*mapWidth + x-1].type == OT_Castle))
		return OT_Castle;

	// nothing there
	return OT_None;
}

int Map::GetDetail(int x, int y)
{
	if(pMap[y*mapWidth + x].type)
		return pMap[y*mapWidth + x].index;

	// check if we are in the space of an oversize castle
	if(x > 0 && pMap[y*mapWidth + x-1].type == OT_Castle)
		return pMap[y*mapWidth + x-1].index;
	if(y > 0 && pMap[(y-1)*mapWidth + x].type == OT_Castle)
		return pMap[(y-1)*mapWidth + x].index;
	if(x > 0 && y > 0 && pMap[(y-1)*mapWidth + x-1].type == OT_Castle)
		return pMap[(y-1)*mapWidth + x-1].index;

	// nothing there
	return 0;
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
