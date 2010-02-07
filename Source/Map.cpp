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

uint32 MapTile::GetTerrain() const
{
	Map *pMap = Game::GetCurrent()->GetMap();
	return pMap->GetTerrain(terrain);
}

void MapTile::AddGroup(Group *_pGroup)
{
	_pGroup->pTile = this;
	_pGroup->pNext = pGroup;
	pGroup = _pGroup;
}

void MapTile::AddGroupToBack(Group *_pGroup)
{
	_pGroup->pTile = this;
	_pGroup->pNext = NULL;

	if(!pGroup)
	{
		pGroup = _pGroup;
	}
	else
	{
		Group *pT = pGroup;
		while(pT->pNext)
			pT = pT->pNext;
		pT->pNext = _pGroup;
	}
}

void MapTile::RemoveGroup(Group *_pGroup)
{
	if(!pGroup || !_pGroup)
		return;

	if(pGroup == _pGroup)
	{
		pGroup = pGroup->pNext;
	}
	else
	{
		Group *pG = pGroup;
		while(pG->pNext != _pGroup)
			pG = pG->pNext;
		if(pG->pNext)
			pG->pNext = pG->pNext->pNext;
		_pGroup->pNext = NULL;
	}

	_pGroup->pTile = NULL;
}

int MapTile::GetNumGroups()
{
	int groups = 0;
	for(Group *pG = pGroup; pG; pG = pG->pNext)
		++groups;
	return groups;
}

Group *MapTile::GetGroup(int group)
{
	for(Group *pG = pGroup; pG; pG = pG->pNext)
	{
		if(!group--)
			return pG;
	}
	return NULL;
}

Group *MapTile::FindUnitGroup(Unit *pUnit)
{
	for(Group *pG = pGroup; pG; pG = pG->pNext)
	{
		if(pG->IsInGroup(pUnit))
			return pG;
	}
	return NULL;
}

Unit *MapTile::FindVehicle()
{
	for(Group *pG = pGroup; pG; pG = pG->pNext)
	{
		Unit *pVehicle = pG->GetVehicle();
		if(pVehicle)
			return pVehicle;
	}
	return NULL;
}

void MapTile::BringGroupToFront(Group *_pGroup)
{
	RemoveGroup(_pGroup);
	_pGroup->pTile = this;
	_pGroup->pNext = pGroup;
	pGroup = _pGroup;
}

int MapTile::GetNumUnits()
{
	int units = 0;
	for(Group *pG = pGroup; pG; pG = pG->pNext)
		units += pG->GetNumUnits();
	return units;
}

bool MapTile::IsFriendlyTile(int player)
{
	if(pGroup)
		return pGroup->player == player;
	Castle *pCastle = GetCastle();
	if(pCastle)
		return pCastle->player == player;
	return false;
}

bool MapTile::IsEnemyTile(int player)
{
	if(pGroup)
		return pGroup->player != player;
	Castle *pCastle = GetCastle();
	if(pCastle)
		return pCastle->player != player;
	return false;
}

bool MapTile::CanMove(Group *_pGroup)
{
	if(IsEnemyTile(_pGroup->player))
		return false;
	if(pGroup)
		return _pGroup->GetNumUnits() <= GetAvailableUnitSpace();
	return true;
}

int GetCastles(MFIni *pIni, Castle *pCastles)
{
	int numCastles = 0;

	MFIniLine *pLine = pIni->GetFirstLine();
	while(pLine)
	{
		if(pLine->IsSection("Map"))
		{
			MFIniLine *pMapLine = pLine->Sub();

			while(pMapLine)
			{
				if(pMapLine->IsSection("Castles"))
				{
					MFIniLine *pCastleSection = pMapLine->Sub();

					numCastles = 0;

					pCastleSection = pMapLine->Sub();

					while(pCastleSection)
					{
						if(pCastleSection->IsSection("Castle"))
						{
							MFIniLine *pCastle = pCastleSection->Sub();

							int player = -1;

							CastleDetails details;
							MFString_Copy(details.name, "Untitled");
							details.x = -1;
							details.y = -1;
							details.numBuildUnits = 0;
							details.income = 0;

							while(pCastle)
							{
								if(!MFString_CaseCmp(pCastle->GetString(0), "name"))
								{
									MFString_Copy(details.name, pCastle->GetString(1));
								}
								else if(!MFString_CaseCmp(pCastle->GetString(0), "position"))
								{
									details.x = pCastle->GetInt(1);
									details.y = pCastle->GetInt(2);
								}
								else if(!MFString_CaseCmp(pCastle->GetString(0), "race"))
								{
									player = pCastle->GetInt(1);
								}
								else if(!MFString_CaseCmp(pCastle->GetString(0), "income"))
								{
									details.income = pCastle->GetInt(1);
								}
								else if(!MFString_CaseCmp(pCastle->GetString(0), "unit"))
								{
									int unit = pCastle->GetInt(1);
									details.numBuildUnits = MFMax(details.numBuildUnits, unit + 1);
									details.buildUnits[unit].unit = pCastle->GetInt(3);
									details.buildUnits[unit].cost = pCastle->GetInt(4);
									details.buildUnits[unit].buildTime = pCastle->GetInt(5);
								}

								pCastle = pCastle->Next();
							}

							pCastles[numCastles++].Init(details, player);
						}

						pCastleSection = pCastleSection->Next();
					}
				}

				pMapLine = pMapLine->Next();
			}
		}

		pLine = pLine->Next();
	}

	return numCastles;
}

Map *Map::Create(Game *pGame, const char *pMapFilename, bool bEditable)
{
	MFIni *pIni = MFIni::Create(pMapFilename);
	if(!pIni)
		return NULL;

	bool bRegionsPresent[16];
	MFZeroMemory(bRegionsPresent, sizeof(bRegionsPresent));

	Map *pMap = NULL;

	MFIniLine *pLine = pIni->GetFirstLine();
	while(pLine)
	{
		if(pLine->IsSection("Map"))
		{
			pMap = (Map*)MFHeap_AllocAndZero(sizeof(Map));
			pMap = new(pMap) Map;
			pMap->pGame = pGame;
			pMap->pCastles = (Castle*)MFHeap_AllocAndZero(sizeof(Castle) * 128);
			pMap->bEditable = bEditable;

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
					MFString_Copy(pMap->unitset, pMapLine->GetString(1));
					pMap->pUnits = UnitDefinitions::Load(pGame, pMapLine->GetString(1), pMap->pTiles->GetNumTerrainTypes());
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
					for(int a=0; a<pMap->mapWidth * pMap->mapHeight; ++a)
					{
						pMap->pMap[a].x = a % pMap->mapWidth;
						pMap->pMap[a].y = a / pMap->mapWidth;
					}

					int i = 0;

					MFIniLine *pTiles = pMapLine->Sub();
					while(pTiles)
					{
						MFDebug_Assert(pTiles->GetStringCount() == pMap->mapWidth, "Not enough tiles in row.");

						for(int a=0; a<pMap->mapWidth; ++a)
						{
							uint32 t = MFString_AsciiToInteger(pTiles->GetString(a), false, 16);
							pMap->pMap[i].terrain = (uint8)t;
							pMap->pMap[i].region = (uint8)(t >> 8) & 0xF;
							bRegionsPresent[pMap->pMap[i].region] = true;
							++i;
						}

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

						MapTile *pTile = pMap->pMap + y*pMap->mapWidth + x;

						if(bEditable || pTile->region == 0xF)
						{
							MFDebug_Assert(!MFString_Compare(pDetails->GetString(2), "="), "Expected: '='");

							for(int a=0; a<OT_Max; ++a)
							{
								if(!MFString_CaseCmp(pDetails->GetString(3), pObjectTypes[a]))
								{
									pTile->type = a;
									break;
								}
							}

							pTile->index = pDetails->GetInt(4);
						}

						pDetails = pDetails->Next();
					}
				}

				pMapLine = pMapLine->Next();
			}

			// get the castles and parse them
			Castle castles[128];
			MFZeroMemory(castles, sizeof(castles));

			int numCastles = GetCastles(pIni, castles);

			for(int c=0; c<numCastles; ++c)
			{
				Castle &castle = castles[c];

				MapTile *pTile = pMap->pMap + castle.details.y*pMap->mapWidth + castle.details.x;

				if(bEditable || pTile->region == 0xF)
				{
					castle.pTile = pTile;
					castle.pUnitDefs = pMap->pUnits;
					if(castle.player != -1)
						castle.player = pTile->region;

					for(int a=0; a<castle.details.numBuildUnits; ++a)
					{
						UnitDetails *pDetails = pMap->pUnits->GetUnitDetails(castle.details.buildUnits[a].unit);
						if(pDetails)
						{
							castle.details.buildUnits[a].cost += pDetails->cost;
							castle.details.buildUnits[a].buildTime += pDetails->buildTime;
						}
					}

					for(int a=0; a<4; ++a)
					{
						MapTile &tile = pMap->pMap[(castle.details.y + (a >> 1))*pMap->mapWidth + castle.details.x + (a & 1)];
						tile.pObject = &pMap->pCastles[pMap->numCastles];
						tile.type = OT_Castle;
						tile.index = pMap->numCastles;
						tile.castleTile = a;
					}

					pMap->pCastles[pMap->numCastles++] = castle;
				}
			}

			if(!bEditable)
			{
				for(int a=0; a<15; ++a)
				{
					if(!bRegionsPresent[a])
						continue;

					// load slices from other maps
					const char *pRace = pMap->pUnits->GetRaceName(pGame->GetPlayerRace(a));
					const char *pFilename = MFStr("%s-%s", pMapFilename, pRace);
					MFIni *pSlice = MFIni::Create(pFilename);;
					if(!pSlice)
					{
						MFDebug_Assert(false, MFStr("Couldn't load '%s.ini'!", pFilename));
						continue;
					}

					MFIniLine *pLine = pSlice->GetFirstLine();
					while(pLine)
					{
						if(pLine->IsSection("Map"))
						{
							MFIniLine *pMapLine = pLine->Sub();

							while(pMapLine)
							{
								if(pMapLine->IsSection("Tiles"))
								{
									MFIniLine *pTiles = pMapLine->Sub();

									int i = 0;
									while(pTiles)
									{
										MFDebug_Assert(pTiles->GetStringCount() == pMap->mapWidth, "Not enough tiles in row.");

										for(int t=0; t<pMap->mapWidth; ++t, ++i)
										{
											if(pMap->pMap[i].region == a)
											{
												uint32 terrain = MFString_AsciiToInteger(pTiles->GetString(t), false, 16);
												pMap->pMap[i].terrain = (uint8)terrain;
											}
										}

										pTiles = pTiles->Next();
									}
								}
								else if(pMapLine->IsSection("Details"))
								{
									MFIniLine *pDetails = pMapLine->Sub();
									while(pDetails)
									{
										int x = pDetails->GetInt(0);
										int y = pDetails->GetInt(1);

										MapTile *pTile = pMap->pMap + y*pMap->mapWidth + x;

										if(pTile->region == a)
										{
											MFDebug_Assert(!MFString_Compare(pDetails->GetString(2), "="), "Expected: '='");

											for(int a=0; a<OT_Max; ++a)
											{
												if(!MFString_CaseCmp(pDetails->GetString(3), pObjectTypes[a]))
												{
													pTile->type = a;
													break;
												}
											}

											pTile->index = pDetails->GetInt(4);
										}

										pDetails = pDetails->Next();
									}
								}

								pMapLine = pMapLine->Next();
							}
						}

						pLine = pLine->Next();
					}

					MFZeroMemory(castles, sizeof(castles));
					numCastles = GetCastles(pSlice, castles);

					for(int c=0; c<numCastles; ++c)
					{
						Castle &castle = castles[c];

						MapTile *pTile = pMap->pMap + castle.details.y*pMap->mapWidth + castle.details.x;

						if(pTile->region == a)
						{
							castle.pTile = pTile;
							castle.pUnitDefs = pMap->pUnits;
							if(castle.player != -1)
								castle.player = a;

							for(int u=0; u<castle.details.numBuildUnits; ++u)
							{
								UnitDetails *pDetails = pMap->pUnits->GetUnitDetails(castle.details.buildUnits[u].unit);
								if(pDetails)
								{
									castle.details.buildUnits[u].cost += pDetails->cost;
									castle.details.buildUnits[u].buildTime += pDetails->buildTime;
								}
							}

							for(int t=0; t<4; ++t)
							{
								MapTile &tile = pMap->pMap[(castle.details.y + (t >> 1))*pMap->mapWidth + castle.details.x + (t & 1)];
								tile.pObject = &pMap->pCastles[pMap->numCastles];
								tile.type = OT_Castle;
								tile.index = pMap->numCastles;
								tile.castleTile = t;
							}

							pMap->pCastles[pMap->numCastles++] = castle;
						}
					}

					MFIni::Destroy(pSlice);
				}
			}
		}

		pLine = pLine->Next();
	}

	if(!pMap)
		return NULL;

	pMap->path.Init(pMap);

	pMap->zoom = 1.0f;

	// if we want to edit the map, we need some memory for map traversal
	if(bEditable)
	{
		pMap->pTouched = (uint8*)MFHeap_AllocAndZero(pMap->mapWidth * pMap->mapHeight * sizeof(*pMap->pTouched));
		pMap->pChangeList = (MapCoord*)MFHeap_Alloc(sizeof(MapCoord)*1024);
		pMap->pRevertList = (RevertTile*)MFHeap_Alloc(sizeof(RevertTile)*1024);
		pMap->numChanges = 0;
	}

	int tileWidth, tileHeight;
	MFRect screen;
	pMap->pTiles->GetTileSize(&tileWidth, &tileHeight);
	GetDisplayRect(&screen);

	const int maxZoom = 2;
	int rtWidth = (int)screen.width*maxZoom+tileWidth*2-1;
	int rtHeight = (int)screen.height*maxZoom+tileHeight*2-1;
	rtWidth -= rtWidth%tileWidth;
	rtHeight -= rtHeight%tileHeight;
	rtWidth = MFUtil_NextPowerOf2(rtWidth);
	rtHeight = MFUtil_NextPowerOf2(rtHeight);
	pMap->pRenderTarget = MFTexture_CreateRenderTarget("MapSurface", rtWidth, rtHeight, TexFmt_SelectFastest_NoAlpha);
	pMap->pRenderTargetMaterial = MFMaterial_Create("MapSurface");

//	pMap->pMinimap = MFTexture_CreateRenderTarget("MiniMap", MFUtil_NextPowerOf2(pMap->mapWidth), MFUtil_NextPowerOf2(pMap->mapHeight));
//	pMap->pMinimapMaterial = MFMaterial_Create("MiniMap");
	pMap->pMinimap = NULL;
	pMap->pMinimapMaterial = NULL;

	// add some clouds
	pMap->pCloud = MFMaterial_Create("Cloud");

	for(int a=0; a<numClouds; ++a)
	{
		pMap->clouds[a].x = MFRand_Range(-64.f, (float)(pMap->mapWidth * tileWidth));
		pMap->clouds[a].y = MFRand_Range(-64.f, (float)(pMap->mapHeight * tileHeight));
	}


	pMap->moveButton = 0;

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

Map *Map::CreateNew(Game *pGame, const char *pTileset, const char *pUnits)
{
	Map *pNew = (Map*)MFHeap_AllocAndZero(sizeof(Map));
	pNew = new(pNew) Map;

	pNew->pGame = pGame;
	pNew->pCastles = (Castle*)MFHeap_AllocAndZero(sizeof(Castle) * 256);

	MFString_Copy(pNew->name, "Untitled");
	MFString_Copy(pNew->tileset, pTileset);
	MFString_Copy(pNew->unitset, pUnits);

	pNew->mapWidth = 128;
	pNew->mapHeight = 128;
	pNew->pMap = (MapTile*)MFHeap_AllocAndZero(pNew->mapWidth * pNew->mapHeight * sizeof(MapTile));
	for(int a=0; a<pNew->mapWidth * pNew->mapHeight; ++a)
	{
		pNew->pMap[a].x = a % pNew->mapWidth;
		pNew->pMap[a].y = a / pNew->mapWidth;
	}

	pNew->pTiles = Tileset::Create(pTileset);
	pNew->pUnits = UnitDefinitions::Load(pGame, pUnits, pNew->pTiles->GetNumTerrainTypes());

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
	GetDisplayRect(&screen);

	const int maxZoom = 2;
	int rtWidth = (int)screen.width*maxZoom+tileWidth*2-1;
	int rtHeight = (int)screen.height*maxZoom+tileHeight*2-1;
	rtWidth -= rtWidth%tileWidth;
	rtHeight -= rtHeight%tileHeight;
	rtWidth = MFUtil_NextPowerOf2(rtWidth);
	rtHeight = MFUtil_NextPowerOf2(rtHeight);
	pNew->pRenderTarget = MFTexture_CreateRenderTarget("MapSurface", rtWidth, rtHeight);
	pNew->pRenderTargetMaterial = MFMaterial_Create("MapSurface");

//	pNew->pMinimap = MFTexture_CreateRenderTarget("MiniMap", MFUtil_NextPowerOf2(pMap->mapWidth), MFUtil_NextPowerOf2(pMap->mapHeight));
	pNew->pMinimap = NULL;

	// editor stuff
	pNew->pTouched = (uint8*)MFHeap_AllocAndZero(pNew->mapWidth * pNew->mapHeight * sizeof(*pNew->pTouched));
	pNew->pChangeList = (MapCoord*)MFHeap_Alloc(sizeof(MapCoord)*1024);
	pNew->pRevertList = (RevertTile*)MFHeap_Alloc(sizeof(RevertTile)*1024);
	pNew->numChanges = 0;

	pNew->moveButton = 0;

	return pNew;
}

void Map::Destroy()
{
	path.Deinit();

	pTiles->Destroy();
	pUnits->Free();

	MFMaterial_Destroy(pCloud);

	MFHeap_Free(pCastles);
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
		name, tileset, unitset, mapWidth, mapHeight);
	int len = MFString_Length(pMapData);
	MFFile_Write(pFile, pMapData, len);

	char buffer[2048];
	for(int a=0; a<mapHeight; ++a)
	{
		MFString_Copy(buffer, "\t\t");
		int offset = MFString_Length(buffer);

		for(int b=0; b<mapWidth; ++b)
		{
			offset += sprintf(&buffer[offset], "%03x", (uint32)pMap[a*mapWidth + b].terrain | ((uint32)pMap[a*mapWidth + b].region << 8));
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
				int len = sprintf(buffer, "\t\t%d, %d = %s %d\n", b, a, pObjectTypes[type], (int)(int8)pMap[a*mapWidth + b].index);
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
			MapTile &tile = pMap[a*mapWidth + b];
			if(tile.type == OT_Castle && tile.castleTile == 0)
			{
				Castle &castle = pCastles[tile.index];
				int len = sprintf(buffer, "\t\t[Castle]\n\t\t{\n\t\t\tname = \"%s\"\n\t\t\tposition = %d, %d\n\t\t\tincome = %d\n\t\t\trace = %d\n", castle.details.name, b, a, castle.details.income, castle.player);
				MFFile_Write(pFile, buffer, len);
				for(int c=0; c<castle.details.numBuildUnits; ++c)
				{
					BuildUnit &unit = castle.details.buildUnits[c];
					int cost = unit.cost;
					int buildTime = unit.buildTime;

					UnitDetails *pDetails = pUnits->GetUnitDetails(unit.unit);
					if(pDetails)
					{
						cost -= pDetails->cost;
						buildTime -= pDetails->buildTime;
					}

					int len = sprintf(buffer, "\t\t\tunit %d = %d, %d, %d\n", c, unit.unit, cost, buildTime);
					MFFile_Write(pFile, buffer, len);
				}
				MFFile_Write(pFile, "\t\t}\n", MFString_Length("\t\t}\n"));
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

void Map::GetCursor(float x, float y, int *pX, int *pY)
{
	float tileWidth, tileHeight;
	GetVisibleTileSize(&tileWidth, &tileHeight);

	if(pX)
		*pX = (int)(xOffset + x / tileWidth);
	if(pY)
		*pY = (int)(yOffset + y / tileHeight);
}

bool Map::HandleInputEvent(InputEvent ev, InputInfo &info)
{
	if(info.device == IDD_Mouse && info.deviceID != 0)
		return false;

	float tileWidth, tileHeight;

	switch(ev)
	{
		case IE_Down:
			if(info.buttonID == moveButton)
			{
				pInputManager->SetExclusiveContactReceiver(info.contact, this);
				return true;
			}
			break;
		case IE_Drag:
			if(info.buttonID == moveButton)
			{
				GetVisibleTileSize(&tileWidth, &tileHeight);
				SetOffset(xOffset + -info.drag.deltaX/tileWidth, yOffset + -info.drag.deltaY/tileHeight);
				return true;
			}
			break;
		case IE_Pinch:
		{
			float newZoom = (info.device == IDD_Mouse) ? zoom + (info.pinch.deltaScale < 1.f ? -.25f : .25f) : zoom * info.pinch.deltaScale;
			GetVisibleTileSize(&tileWidth, &tileHeight);
			SetZoom(newZoom, info.pinch.centerX/tileWidth, info.pinch.centerY/tileHeight);
			return true;
		}
	}

	return false;
}

void Map::Update()
{
	int tileWidth, tileHeight;
	pTiles->GetTileSize(&tileWidth, &tileHeight);

	for(int a=0; a<numClouds; ++a)
	{
		clouds[a].x -= MFSystem_TimeDelta() * 5.f;

		if(clouds[a].x < -64)
		{
			clouds[a].x = (float)(mapWidth * tileWidth);
			clouds[a].y = MFRand_Range(-64.f, (float)(mapHeight * tileHeight));
		}
	}
}

void Map::Draw()
{
	MFView_Push();

	MFRenderer_SetRenderTarget(pRenderTarget, NULL);

	float texelCenter = MFRenderer_GetTexelCenterOffset();
	int xStart = (int)xOffset;
	int yStart = (int)yOffset;

	int xTiles, yTiles;
	SetRTOrtho(&xTiles, &yTiles);

	// HACK: make sure it doesn't attempt to draw too many
	if(xStart + xTiles > mapWidth)
		xTiles = mapWidth - xStart;
	if(yStart + yTiles > mapHeight)
		yTiles = mapHeight - yStart;

	MapTile *pStart = pMap + yStart*mapWidth + xStart;

	// blit map portion to a render target
	pTiles->DrawMap(xTiles, yTiles, &pStart->terrain, sizeof(MapTile), mapWidth, texelCenter);

	// now we should, like, render all the extra stuff
	struct RenderTile
	{
		uint8 x, y, flags;
		int8 i;
	} renderTiles[1024];

	int numTiles = 0;
	int numRoads = 0;
	int numCastleTiles = 0;

	Group *pSelection = NULL;

	for(int y=0; y<yTiles; ++y)
	{
		for(int x=xTiles-1; x>=0; --x)
		{
			MapTile *pTile = pStart + x;

			if(pTile->GetNumGroups())
			{
				Unit *pVehicle = pTile->FindVehicle();
				if(pVehicle)
					pVehicle->Draw((float)x, (float)y);

				Group *pGroup = pTile->GetGroup(0);
				if(pGroup->IsSelected())
					pSelection = pGroup;

				Unit *pUnit = pGroup->GetFeatureUnit();
				if(pUnit && (!pVehicle || pGroup->GetVehicle() != pVehicle))
					pUnit->Draw((float)x, (float)y);
			}

			if(pTile->type == OT_None)
				continue;

			RenderTile &drawTile = renderTiles[numTiles++];
			drawTile.x = x;
			drawTile.y = y;

			switch(pTile->type)
			{
				case OT_Road:
					drawTile.i = pTiles->FindRoad(pTile->index, GetTerrainAt(xStart+x, yStart+y));
					MFDebug_Assert(drawTile.i >= 0, "Invalid road!");

					drawTile.flags = 8;
					++numRoads;
					break;

				case OT_Castle:
				{
					if(pTile->castleTile != 0)
					{
						--numTiles;
						continue;
					}

					Castle *pCastle = GetCastle(pTile->index);
					drawTile.i = (int8)pCastle->player;
					drawTile.flags = 0;
					++numCastleTiles;
					break;
				}

				case OT_Flag:
					drawTile.i = (int8)pTile->index;
					drawTile.flags = 1;
					++numCastleTiles;
					break;

				case OT_Special:
					drawTile.i = (int8)pTile->index;
					drawTile.flags = 2;
					++numCastleTiles;
					break;
			}
		}

		pStart += mapWidth;
	}

	// draw the roads
	if(numRoads)
	{
		MFMaterial_SetMaterial(pTiles->GetRoadMaterial());

		MFPrimitive(PT_QuadList);
		MFBegin(numRoads*2);
		MFRect uvs;

		for(int a=0; a<numTiles; ++a)
		{
			RenderTile &drawTile = renderTiles[a];

			if(drawTile.flags & 8)
			{
				pTiles->GetRoadUVs(drawTile.i, &uvs, texelCenter);
				MFSetTexCoord1(uvs.x, uvs.y);
				MFSetPosition((float)renderTiles[a].x, (float)renderTiles[a].y, 0);
				MFSetTexCoord1(uvs.x + uvs.width, uvs.y + uvs.height);
				MFSetPosition((float)renderTiles[a].x + 1.f, (float)renderTiles[a].y + 1.f, 0);
			}
		}

		MFEnd();
	}

	// draw the castle stuff
	if(numCastleTiles)
	{
		MFMaterial_SetMaterial(pUnits->GetCastleMaterial());

		MFPrimitive(PT_QuadList);
		MFBegin(numCastleTiles*2);
		MFRect uvs;

		for(int a=0; a<numTiles; ++a)
		{
			RenderTile &drawTile = renderTiles[a];

			if(drawTile.flags < 8)
			{
				MFVector colour = MFVector::one;
				float width = 1.f;

				switch(drawTile.flags)
				{
					case 0:
						pUnits->GetCastleUVs(pGame->GetPlayerRace(drawTile.i), &uvs, texelCenter);
						colour = pGame->GetPlayerColour(drawTile.i);
						width = 2.f;
						break;
					case 1:
						pUnits->GetFlagUVs(pGame->GetPlayerRace(drawTile.i), &uvs, texelCenter);
						colour = pGame->GetPlayerColour(drawTile.i);
						break;
					case 2:
						pUnits->GetSpecialUVs(drawTile.i, &uvs, texelCenter);
						break;
				}

				MFSetColour(colour);
				MFSetTexCoord1(uvs.x, uvs.y);
				MFSetPosition((float)renderTiles[a].x, (float)renderTiles[a].y, 0);
				MFSetTexCoord1(uvs.x + uvs.width, uvs.y + uvs.height);
				MFSetPosition((float)renderTiles[a].x + width, (float)renderTiles[a].y + width, 0);
			}
		}

		MFEnd();
	}

	// draw the selection
	if(pSelection)
	{
		int x = pSelection->GetTile()->GetX() - xStart;
		int y = pSelection->GetTile()->GetY() - yStart;
		MFPrimitive_DrawUntexturedQuad((float)x, (float)y, 1, 1, MakeVector(0.f, 0.f, 0.8f, 0.4f));
	}

	// draw the units
	pUnits->DrawUnits(1.f, texelCenter);

	int tileWidth, tileHeight;
	pTiles->GetTileSize(&tileWidth, &tileHeight);
	float tileXScale = 1.f/(float)tileWidth;
	float tileYScale = 1.f/(float)tileHeight;

	// draw clouds
	MFMaterial_SetMaterial(pCloud);

	MFPrimitive(PT_QuadList);
	MFBegin(numClouds*2);

	for(int a=0; a<numClouds; ++a)
	{
		float x = clouds[a].x * tileXScale - (float)xStart;
		float y = clouds[a].y * tileYScale - (float)yStart;

		MFSetTexCoord1(0.f, 0.f);
		MFSetPosition(x, y, 0);
		MFSetTexCoord1(1.f, 1.f);
		MFSetPosition(x + 1.f, y + 2.f, 0);
	}

	MFEnd();

	// reset the render target
	MFRenderer_SetDeviceRenderTarget();

	// render the map to the screen
	MFMatrix orthoMat;
	GetOrthoMatrix(&orthoMat, true);
	MFView_SetCustomProjection(orthoMat, false);

	MFRect uvs;
	int targetWidth, targetHeight;

	GetDisplayRect(&uvs);
	MFTexture_GetTextureDimensions(pRenderTarget, &targetWidth, &targetHeight);

	float texelOffset = zoom <= 0.5f ? 0.f : texelCenter;
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

	MFRect screenRect;
	GetDisplayRect(&screenRect);
	screenRect.width /= zoom;
	screenRect.height /= zoom;

	int tileW, tileH;
	pTiles->GetTileSize(&tileW, &tileH);

	float screenWidth = screenRect.width / tileW;
	float screenHeight = screenRect.height / tileH;

	xOffset = (int)(xOffset*tileW) / (float)tileW;
	yOffset = (int)(yOffset*tileH) / (float)tileH;

	MFRect rect;
	rect.x = xOffset - (float)(int)xOffset;
	rect.y = yOffset - (float)(int)yOffset;
	rect.width = screenWidth;
	rect.height = screenHeight;
	MFView_SetOrtho(&rect);

	int xTiles = (int)MFCeil(screenWidth + 1.f);
	int yTiles = (int)MFCeil(screenHeight + 1.f);

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

	for(int y=0; y<yTiles; ++y)
	{
		for(int x=0; x<xTiles; ++x)
		{
			MapTile *pTile = GetTile(xOffsetI + x, yOffsetI + y);
			char text[16] = "X";
			if(pTile->region != 15)
				sprintf(text, "%d", pTile->region + 1);
			float w = MFFont_GetStringWidth(MFFont_GetDebugFont(), text, 0.5f);
			MFFont_DrawText(MFFont_GetDebugFont(), MakeVector((float)x + 0.5f - w*0.5f + .02f, (float)y + 0.25f + .02f), 0.5f, MFVector::black, text);
			MFFont_DrawText(MFFont_GetDebugFont(), MakeVector((float)x + 0.5f - w*0.5f, (float)y + 0.25f), 0.5f, MFVector::white, text);
		}
	}

	MFView_Pop();

	MFFont_DrawTextf(MFFont_GetDebugFont(), MakeVector(8, 8), 24, MFVector::yellow, "%d, %d", cursorX, cursorY);
}

void Map::SetRTOrtho(int *pXTiles, int *pYTiles)
{
	int targetWidth, targetHeight;
	int tileWidth, tileHeight;
	MFTexture_GetTextureDimensions(pRenderTarget, &targetWidth, &targetHeight);
	pTiles->GetTileSize(&tileWidth, &tileHeight);

	float screenWidth = (float)targetWidth / (float)tileWidth;
	float screenHeight = (float)targetHeight / (float)tileHeight;

	xOffset = (int)(xOffset*tileWidth) / (float)tileWidth;
	yOffset = (int)(yOffset*tileHeight) / (float)tileHeight;

	MFRect rect;
	rect.x = xOffset - (float)(int)xOffset;
	rect.y = yOffset - (float)(int)yOffset;
	rect.width = screenWidth;
	rect.height = screenHeight;
	MFView_SetOrtho(&rect);

	MFRect screenRect;
	GetDisplayRect(&screenRect);

	if(pXTiles)
		*pXTiles = (int)MFCeil((screenRect.width / tileWidth) / zoom + 1.f);
	if(pYTiles)
		*pYTiles = (int)MFCeil((screenRect.height / tileHeight) / zoom + 1.f);
}

void Map::SetMapOrtho(int *pXTiles, int *pYTiles)
{
	float tileWidth, tileHeight;
	GetVisibleTileSize(&tileWidth, &tileHeight);

	MFRect screenRect;
	GetDisplayRect(&screenRect);
	float screenWidth = screenRect.width / tileWidth;
	float screenHeight = screenRect.height / tileHeight;
	float screenHScale = 1.f / screenWidth;
	float screenVScale = 1.f / screenHeight;

	MFMatrix proj, mapTrans;
	mapTrans.SetXAxis3(MFVector::right * screenHScale);
	mapTrans.SetYAxis3(MFVector::up * screenVScale);
	mapTrans.SetZAxis3(MFVector::forward);
	mapTrans.SetTrans3(MakeVector(-xOffset * screenHScale, -yOffset * screenVScale, 0));

	GetOrthoMatrix(&proj, true);
	mapTrans.Multiply4x4(proj);

	MFView_SetCustomProjection(mapTrans, false);

	if(pXTiles)
		*pXTiles = (int)MFCeil(screenWidth) + 1;
	if(pYTiles)
		*pYTiles = (int)MFCeil(screenHeight) + 1;
}

void Map::SetOffset(float x, float y)
{
	float tileWidth, tileHeight;
	GetVisibleTileSize(&tileWidth, &tileHeight);

	MFRect display;
	GetDisplayRect(&display);
	float maxX = mapWidth - display.width / tileWidth;
	float maxY = mapHeight - display.height / tileHeight;

	xOffset = MFClamp(0.f, x, maxX);
	yOffset = MFClamp(0.f, y, maxY);
}

void Map::GetOffset(float *pX, float *pY)
{
	if(pX)
		*pX = xOffset;
	if(pY)
		*pY = yOffset;
}

void Map::CenterView(int x, int y)
{
	float tileWidth, tileHeight;
	GetVisibleTileSize(&tileWidth, &tileHeight);

	MFRect display;
	GetDisplayRect(&display);
	float screenWidth = display.width / tileWidth;
	float screenHeight = display.height / tileHeight;

	xOffset = (float)x + 0.5f - screenWidth*0.5f;
	yOffset = (float)y + 0.5f - screenHeight*0.5f;

	xOffset = MFClamp(0.f, xOffset, mapWidth - screenWidth);
	yOffset = MFClamp(0.f, yOffset, mapHeight - screenHeight);
}

void Map::ClaimFlags(int x, int y, int player)
{
	int startX = MFMax(x - 2, 0);
	int startY = MFMax(y - 2, 0);
	int endX = MFMin(x + 3, mapWidth);
	int endY = MFMin(y + 3, mapHeight);

	for(int y = startY; y < endY; ++y)
	{
		for(int x = startX; x < endX; ++x)
		{
			MapTile *pTile = GetTile(x, y);
			if(pTile->type == OT_Flag)
				pTile->index = player;
		}
	}
}

void Map::SetZoom(float _zoom, float pointX, float pointY)
{
	float tileWidth, tileHeight;
	GetVisibleTileSize(&tileWidth, &tileHeight);

	MFRect display;
	GetDisplayRect(&display);

	if(pointX < 0.f)
		pointX = display.width / tileWidth * 0.5f;
	if(pointY < 0.f)
		pointY = display.height / tileHeight * 0.5f;

	float newZoom = MFClamp(0.5f, _zoom, 2.f);
	float zoomDiff = zoom / newZoom;
	zoom = newZoom;

	// calculate new position
	xOffset += pointX - pointX*zoomDiff;
	yOffset += pointY - pointY*zoomDiff;

	// calculate clamps
	GetVisibleTileSize(&tileWidth, &tileHeight);
	float maxX = mapWidth - display.width / tileWidth;
	float maxY = mapHeight - display.height / tileHeight;

	// clamp the new position
	xOffset = MFClamp(0.f, xOffset, maxX);
	yOffset = MFClamp(0.f, yOffset, maxY);
}

bool Map::SetTile(int x, int y, uint32 tile, uint32 mask)
{
	if(tile == GetTerrainAt(x, y))
 		return true;

	if(!bRevert && pMap[y*mapWidth + x].region != editRegion)
	{
		pRevertList[numReverts].x = x;
		pRevertList[numReverts].y = y;
		pRevertList[numReverts].t = pMap[y*mapWidth + x].GetTerrain();
		++numReverts;
	}

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
		if(y > 0 && pTouched[(y-1)*mapWidth + x])
		{
			int t = GetTerrainAt(x, y-1);
			tl = DecodeBL(t);
			tr = DecodeBR(t);
			tlm = 0xFF;
			trm = 0xFF;
		}
		if(y < mapHeight-1 && pTouched[(y+1)*mapWidth + x])
		{
			int b = GetTerrainAt(x, y+1);
			bl = DecodeTL(b);
			br = DecodeTR(b);
			blm = 0xFF;
			brm = 0xFF;
		}
		if(x > 0 && pTouched[y*mapWidth + x - 1])
		{
			int l = GetTerrainAt(x-1, y);
			tl = DecodeTR(l);
			bl = DecodeBR(l);
			tlm = 0xFF;
			blm = 0xFF;
		}
		if(x < mapWidth-1 && pTouched[y*mapWidth + x + 1])
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
		MFZeroMemory(pTouched, mapWidth * mapHeight);
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
			if(y > 0 && pTouched[(y-1)*mapWidth + x])
			{
				int t = GetTerrainAt(x, y-1);
				tl = DecodeBL(t);
				tr = DecodeBR(t);
				tlm = 0xFF;
				trm = 0xFF;
			}
			if(y < mapHeight-1 && pTouched[(y+1)*mapWidth + x])
			{
				int b = GetTerrainAt(x, y+1);
				bl = DecodeTL(b);
				br = DecodeTR(b);
				blm = 0xFF;
				brm = 0xFF;
			}
			if(x > 0 && pTouched[y*mapWidth + x - 1])
			{
				int l = GetTerrainAt(x-1, y);
				tl = DecodeTR(l);
				bl = DecodeBR(l);
				tlm = 0xFF;
				blm = 0xFF;
			}
			if(x < mapWidth-1 && pTouched[y*mapWidth + x + 1])
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

void Map::SetRegion(int x, int y, int region)
{
	MapTile *pTile = pMap + y*mapWidth + x;
	pTile->region = region;
}

int Map::UpdateChange(int a)
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
	if(y > 0 && pTouched[(y-1)*mapWidth + x])
	{
		int t = GetTerrainAt(x, y-1);
		tl = DecodeBL(t);
		tr = DecodeBR(t);
		tlm = 0xFF;
		trm = 0xFF;
	}
	if(y < mapHeight-1 && pTouched[(y+1)*mapWidth + x])
	{
		int b = GetTerrainAt(x, y+1);
		bl = DecodeTL(b);
		br = DecodeTR(b);
		blm = 0xFF;
		brm = 0xFF;
	}
	if(x > 0 && pTouched[y*mapWidth + x - 1])
	{
		int l = GetTerrainAt(x-1, y);
		tl = DecodeTR(l);
		bl = DecodeBR(l);
		tlm = 0xFF;
		blm = 0xFF;
	}
	if(x < mapWidth-1 && pTouched[y*mapWidth + x + 1])
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

bool Map::PlaceCastle(int x, int y, int player)
{
	if(pMap[y*mapWidth + x].region != editRegion)
		return false;

	// check we have room on the map
	if(x >= mapWidth - 1 || y >= mapHeight - 1)
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
	for(int a=0; a<4; ++a)
	{
		MapTile *pTile = pMap + (y + (a >>1))*mapWidth + x + (a & 1);
		pTile->type = OT_Castle;
		pTile->index = numCastles;
		pTile->castleTile = a;
		pTile->pObject = GetCastle(numCastles);
	}

	MFZeroMemory(&pCastles[numCastles], sizeof(Castle));
	pCastles[numCastles].pUnitDefs = pUnits;
	pCastles[numCastles].pTile = pMap + y*mapWidth + x;
	pCastles[numCastles].player = player;
	pCastles[numCastles].details.x = x;
	pCastles[numCastles].details.y = y;
	MFString_Copy(pCastles[numCastles].details.name, "Unnamed");

	++numCastles;
	return true;
}

bool Map::PlaceFlag(int x, int y, int race)
{
	if(pMap[y*mapWidth + x].region != editRegion)
		return false;

	// check for compatible terrain
	const Tile *pT = GetTerrainTileAt(x, y);

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
	if(pMap[y*mapWidth + x].region != editRegion)
		return false;

	// check for compatible terrain
	const Tile *pT = GetTerrainTileAt(x, y);

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
	if(pMap[y*mapWidth + x].region != editRegion)
		return false;

	if(pMap[y*mapWidth + x].type == OT_Road)
		return true;

	// check for compatible terrain
	MapTile *pHere  = pMap + y*mapWidth + x;
	MapTile *pUp    = y > 0           ? pMap + (y-1)*mapWidth + x : NULL;
	MapTile *pDown  = y < mapHeight-1 ? pMap + (y+1)*mapWidth + x : NULL;
	MapTile *pLeft  = x > 0           ? pMap + y*mapWidth + x-1 : NULL;
	MapTile *pRight = x < mapWidth-1  ? pMap + y*mapWidth + x+1 : NULL;

	const Tile *pTHere  = GetTerrainTileAt(x, y);
	const Tile *pTUp    = pUp    ? GetTerrainTileAt(x, y-1) : NULL;
	const Tile *pTDown  = pDown  ? GetTerrainTileAt(x, y+1) : NULL;
	const Tile *pTLeft  = pLeft  ? GetTerrainTileAt(x-1, y) : NULL;
	const Tile *pTRight = pRight ? GetTerrainTileAt(x+1, y) : NULL;

	// find connections
	uint32 connections = 0;
	if(pUp && pUp->type == OT_Road && pTiles->FindRoad(pUp->index | 4, pTUp->terrain) != -1)
		connections |= 8;
	if(pDown && pDown->type == OT_Road && pTiles->FindRoad(pDown->index | 8, pTDown->terrain) != -1)
		connections |= 4;
	if(pLeft && pLeft->type == OT_Road && pTiles->FindRoad(pLeft->index | 1, pTLeft->terrain) != -1)
		connections |= 2;
	if(pRight && pRight->type == OT_Road && pTiles->FindRoad(pRight->index | 2, pTRight->terrain) != -1)
		connections |= 1;

	// find suitable roads
	int roads[8];
	int numRoads = pTiles->FindBestRoads(roads, connections, pTHere->terrain);
	if(numRoads == 0)
		return false;

	// choose a suitable road
	int r = roads[MFRand() % numRoads];
	connections = pTiles->GetRoadConnections(r);

	// terrain is compatible, place road
	ClearDetail(x, y);
	pHere->type = OT_Road;
	pHere->index = connections;

	// connect surrounding roads
	if(connections & 8)
	{
		if(pUp && pUp->type == OT_Road)
			pUp->index |= 4;
		else if(pUp)
			PlaceRoad(x, y-1);
	}
	if(connections & 4)
	{
		if(pDown && pDown->type == OT_Road)
			pDown->index |= 8;
		else if(pDown)
			PlaceRoad(x, y+1);
	}
	if(connections & 2)
	{
		if(pLeft && pLeft->type == OT_Road)
			pLeft->index |= 1;
		else if(pLeft)
			PlaceRoad(x-1, y);
	}
	if(connections & 1)
	{
		if(pRight && pRight->type == OT_Road)
			pRight->index |= 2;
		else if(pRight)
			PlaceRoad(x+1, y);
	}

	return true;
}

ObjectType Map::GetDetailType(int x, int y) const
{
	return (ObjectType)pMap[y*mapWidth + x].type;
}

int Map::GetDetail(int x, int y) const
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
	MapTile *pTile = pMap + y*mapWidth + x;

	if(pTile->region != editRegion)
		return;

	// if we removed a road, we need to correct the roads around it
	if(pTile->type == OT_Road)
	{
		// remove the item from the map
		pTile->type = OT_None;
		pTile->index = 0;

		// update surrounding roads to remove connection
		if(x > 0 && pTile[-1].type == OT_Road)
		{
			pTile[-1].index &= 0xE;

			if(pTiles->FindRoad(pTile[-1].index, pTiles->GetTile(pTile[-1].terrain)->terrain) == -1)
				ClearDetail(x-1, y);
		}
		if(x < mapWidth-1 && pTile[1].type == OT_Road)
		{
			pTile[1].index &= 0xD;

			if(pTiles->FindRoad(pTile[1].index, pTiles->GetTile(pTile[1].terrain)->terrain) == -1)
				ClearDetail(x+1, y);
		}
		if(y > 0 && pTile[-mapWidth].type == OT_Road)
		{
			pTile[-mapWidth].index &= 0xB;

			if(pTiles->FindRoad(pTile[-mapWidth].index, pTiles->GetTile(pTile[-mapWidth].terrain)->terrain) == -1)
				ClearDetail(x, y-1);
		}
		if(y < mapHeight-1 && pTile[mapWidth].type == OT_Road)
		{
			pTile[mapWidth].index &= 0x7;

			if(pTiles->FindRoad(pTile[mapWidth].index, pTiles->GetTile(pTile[mapWidth].terrain)->terrain) == -1)
				ClearDetail(x, y+1);
		}
	}
	else if(pTile->type == OT_Castle)
	{
		int castle = pTile->index;

		// clear all 4 castle squares
		int cx = pCastles[castle].details.x;
		int cy = pCastles[castle].details.y;

		for(int a=0; a<4; ++a)
		{
			MapTile *pCastleTile = pMap + (cy + (a >> 1))*mapWidth + cx + (a & 1);
			pCastleTile->castleTile = 0;
			pCastleTile->index = 0;
			pCastleTile->type = OT_None;
		}

		// correct all following castle indices
		int numTiles = mapWidth*mapHeight;
		for(int a=0; a<numTiles; ++a)
		{
			if(pMap[a].type == OT_Castle && pMap[a].index > castle)
				--pMap[a].index;
		}

		// destroy the castle
		for(int a = castle+1; a < numCastles; ++a)
			pCastles[a-1] = pCastles[a];

		--numCastles;
	}
	else
	{
		// remove the item from the map
		pTile->type = OT_None;
		pTile->index = 0;
	}
}

Step *Map::FindPath(Group *pGroup, int destX, int destY)
{
	return path.FindPath(pGroup, destX, destY);
}

Step *Map::StripStep(Step *pPath)
{
	return path.StripStep(pPath);
}

void Map::DestroyPath(Step *pPath)
{
	path.Destroy(pPath);
}
