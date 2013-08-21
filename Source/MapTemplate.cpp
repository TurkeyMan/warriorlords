#include "Warlords.h"
#include "MapTemplate.h"

#include "Fuji/MFIni.h"

#include "stdio.h"

static int gMapTemplateResource = -1;

const char *gpMapObjectTypes[] =
{
	"none",
	"terrain",
	"castle",
	"flag",
	"special",
	"road"
};

static int CastleSort(const CastleDetails *pC1, const CastleDetails *pC2)
{
	if(pC1->y == pC2->y && pC1->x == pC2->x)
		return 0;

	if(pC1->y != pC2->y)
		return pC1->y < pC2->y ? -1 : 1;
	return pC1->x < pC2->x ? -1 : 1;
}

void MapTemplate::Init()
{
	gMapTemplateResource = MFResource_Register("MapTemplate", &MapTemplate::Destroy);
}

MapTemplate *MapTemplate::Create(MFString mapFilename)
{
	MapTemplate *pTemplate = (MapTemplate*)MFResource_Find(mapFilename.GetHash());
	if(!pTemplate)
	{
		pTemplate = new MapTemplate(mapFilename);

		MFResource_AddResource(pTemplate, gMapTemplateResource, mapFilename.GetHash(), pTemplate->FileName().CStr());
	}
	return pTemplate;
}

int MapTemplate::AddRef()
{
	return MFResource_AddRef(this);
}

int MapTemplate::Release()
{
	return MFResource_Release(this);
}

MapTemplate::MapTemplate(MFString tileset, MFString unitset, int width, int height)
{
	MFZeroMemory(bRegionsPresent, sizeof(bRegionsPresent));
	for(int a=0; a<16; ++a)
		templates[a].pMap = NULL;
	numPlayers = 0;

	pTiles = Tileset::Create(tileset);
	pUnits = new UnitDefinitions(unitset, pTiles->NumTerrainTypes());

	this->tileset = tileset;
	this->unitset = unitset;

	mapWidth = width;
	mapHeight = height;
}

MapTemplate::MapTemplate(MFString mapFilename)
{
	MFZeroMemory(bRegionsPresent, sizeof(bRegionsPresent));
	for(int a=0; a<16; ++a)
		templates[a].pMap = NULL;
	numPlayers = 0;

	MFIni *pIni = MFIni::Create(mapFilename.CStr());
	MFDebug_Assert(pIni, "Couldn't load map!");

	filename = mapFilename;

	MFIniLine *pLine = pIni->GetFirstLine();
	while(pLine)
	{
		if(pLine->IsSection("Map"))
		{
			MFIniLine *pMapLine = pLine->Sub();

			while(pMapLine)
			{
				if(pMapLine->IsString(0, "name"))
				{
					name = pMapLine->GetString(1);
				}
				else if(pMapLine->IsString(0, "tileset"))
				{
					tileset = pMapLine->GetString(1);
					pTiles = Tileset::Create(pMapLine->GetString(1));
				}
				else if(pMapLine->IsString(0, "units"))
				{
					unitset = pMapLine->GetString(1);
					pUnits = new UnitDefinitions(pMapLine->GetString(1), pTiles->NumTerrainTypes());
				}
				else if(pMapLine->IsString(0, "map_width"))
				{
					mapWidth = pMapLine->GetInt(1);
				}
				else if(pMapLine->IsString(0, "map_height"))
				{
					mapHeight = pMapLine->GetInt(1);
				}
				else if(pMapLine->IsSection("Regions"))
				{
					MFDebug_Assert(mapWidth && mapHeight, "Invalid map dimensions");

					pRegions = (uint8*)MFHeap_AllocAndZero(mapWidth * mapHeight * sizeof(uint8));

					int i = 0;
					MFIniLine *pRegions = pMapLine->Sub();
					while(pRegions)
					{
						MFDebug_Assert(pRegions->GetStringCount() == mapWidth, "Not enough tiles in row.");

						for(int a=0; a<mapWidth; ++a)
						{
							uint32 t = MFString_AsciiToInteger(pRegions->GetString(a), false, 16);
							this->pRegions[i] = t;
							if(t < 15 && !bRegionsPresent[t])
								++numPlayers;
							bRegionsPresent[t] = true;
							++i;
						}

						pRegions = pRegions->Next();
					}

					MFDebug_Assert(i == mapWidth * mapHeight, "Not enough rows.");
				}
				else if(!MFString_CaseCmp(pMapLine->GetString(0), "section"))
				{
					int numRaces = pUnits->GetNumRaces();
					int slice = 0;

					if(MFString_CaseCmp(pMapLine->GetString(1), "Template"))
					{
						slice = 1;
						for(; slice<numRaces; ++slice)
						{
							if(pUnits->GetRaceName(slice) == pMapLine->GetString(1))
								break;
						}
					}

					if(slice < numRaces)
					{
						MFIniLine *pSlice = pMapLine->Sub();

						while(pSlice)
						{
							if(pSlice->IsSection("Tiles"))
							{
								MFDebug_Assert(mapWidth && mapHeight, "Invalid map dimensions");

								templates[slice].pMap = (MapRegionTemplate::TileDetails*)MFHeap_AllocAndZero(mapWidth * mapHeight * sizeof(MapRegionTemplate::TileDetails));

								int i = 0;
								MFIniLine *pTiles = pSlice->Sub();
								while(pTiles)
								{
									MFDebug_Assert(pTiles->GetStringCount() == mapWidth, "Not enough tiles in row.");

									for(int a=0; a<mapWidth; ++a)
									{
										uint32 t = MFString_AsciiToInteger(pTiles->GetString(a), false, 16);
										templates[slice].pMap[i].terrain = (uint8)t;
										++i;
									}

									pTiles = pTiles->Next();
								}

								MFDebug_Assert(i == mapWidth * mapHeight, "Not enough rows.");
							}
							else if(pSlice->IsSection("Details"))
							{
								MFIniLine *pDetails = pSlice->Sub();
								while(pDetails)
								{
									MFDebug_Assert(!MFString_Compare(pDetails->GetString(2), "="), "Expected: '='");

									int x = pDetails->GetInt(0);
									int y = pDetails->GetInt(1);

									MapRegionTemplate::TileDetails *pTile = templates[slice].pMap + y*mapWidth + x;
									for(int a=0; a<OT_Max; ++a)
									{
										if(!MFString_CaseCmp(pDetails->GetString(3), gpMapObjectTypes[a]))
										{
											pTile->type = a;
											break;
										}
									}
									pTile->index = pDetails->GetInt(4);

									pDetails = pDetails->Next();
								}
							}
							else if(pSlice->IsSection("Castles"))
							{
								MFIniLine *pCastleSection = pSlice->Sub();

								pCastleSection = pSlice->Sub();
								while(pCastleSection)
								{
									if(pCastleSection->IsSection("Castle"))
									{
										MFIniLine *pCastle = pCastleSection->Sub();

										CastleDetails &details = templates[slice].castles.push();
										details.name = "Untitled";
										details.x = -1;
										details.y = -1;
										details.numBuildUnits = 0;
										details.bCapital = false;

										while(pCastle)
										{
											if(!MFString_CaseCmp(pCastle->GetString(0), "name"))
											{
												details.name = pCastle->GetString(1);
											}
											else if(!MFString_CaseCmp(pCastle->GetString(0), "position"))
											{
												details.x = pCastle->GetInt(1);
												details.y = pCastle->GetInt(2);
											}
											else if(!MFString_CaseCmp(pCastle->GetString(0), "capital"))
											{
												details.bCapital = pCastle->GetBool(1);
											}
											else if(!MFString_CaseCmp(pCastle->GetString(0), "unit"))
											{
												int unit = pCastle->GetInt(1);
												details.numBuildUnits = MFMax(details.numBuildUnits, unit + 1);
												details.buildUnits[unit].unit = pUnits->FindUnit(pCastle->GetString(3));
												MFDebug_Assert(details.buildUnits[unit].unit != -1, MFStr("Unknown unit: %s", pCastle->GetString(3)));
												details.buildUnits[unit].buildTime = pCastle->GetInt(4);
											}

											pCastle = pCastle->Next();
										}
									}

									pCastleSection = pCastleSection->Next();
								}
							}

							pSlice = pSlice->Next();
						}
					}
				}

				pMapLine = pMapLine->Next();
			}

			// don't look for any more maps
			break;
		}

		pLine = pLine->Next();
	}
}

MapTemplate::~MapTemplate()
{
	for(int a=0; a<16; ++a)
	{
		if(templates[a].pMap)
			MFHeap_Free(templates[a].pMap);
	}

	pTiles->Destroy();
	delete pUnits;
}

void MapTemplate::Destroy(MFResource *pRes)
{
	MapTemplate *pMap = (MapTemplate*)pRes;
	delete pMap;
}

void MapTemplate::Save()
{
	MFFile *pFile = MFFileSystem_Open(MFStr("game:%s.ini", filename.CStr()), MFOF_Write|MFOF_Binary);
	if(!pFile)
		return;

	// write the map to disk...
	const char *pMapData = MFStr(
		"[Map]\n"
		"{\n"
		"\tname = \"%s\"\n\n"
		"\ttileset = %s\n"
		"\tunits = %s\n\n"
		"\tmap_width = %d\n"
		"\tmap_Height = %d\n"
		"\n"
		"\t[Regions]\n"
		"\t{\n",
		name.CStr(), tileset.CStr(), unitset.CStr(), mapWidth, mapHeight);
	int len = MFString_Length(pMapData);
	MFFile_Write(pFile, pMapData, len);

	char buffer[2048];
	for(int a=0; a<mapHeight; ++a)
	{
		MFString_Copy(buffer, "\t\t");
		int offset = MFString_Length(buffer);

		for(int b=0; b<mapWidth; ++b)
		{
			offset += sprintf(&buffer[offset], "%x", (int)pRegions[a*mapWidth + b]);
			buffer[offset++] = b < mapWidth-1 ? ',' : '\n';
		}

		MFFile_Write(pFile, buffer, offset);
	}

	const char *pMapData2 =
		"\t}\n"
		"\n";
	MFFile_Write(pFile, pMapData2, MFString_Length(pMapData2));

	for(int r=0; r<16; ++r)
	{
		if(templates[r].pMap == NULL)
			continue;

		const char *pMapData3 = MFStr(
			"%s"
			"\t[%s]\n"
			"\t{\n"
			"\t\t[Tiles]\n"
			"\t\t{\n",
			r != 0 ? "\n" : "", r == 0 ? "Template" : UnitDefs()->GetRaceName(r).CStr());
		MFFile_Write(pFile, pMapData3, MFString_Length(pMapData3));

		for(int a=0; a<mapHeight; ++a)
		{
			MFString_Copy(buffer, "\t\t\t");
			int offset = MFString_Length(buffer);

			for(int b=0; b<mapWidth; ++b)
			{
				offset += sprintf(&buffer[offset], "%03x", templates[r].pMap[a*mapWidth + b].terrain);
				buffer[offset++] = b < mapWidth-1 ? ',' : '\n';
			}

			MFFile_Write(pFile, buffer, offset);
		}

		const char *pMapData4 =
			"\t\t}\n"
			"\n"
			"\t\t[Details]\n"
			"\t\t{\n";
		MFFile_Write(pFile, pMapData4, MFString_Length(pMapData4));

		for(int a=0; a<mapHeight; ++a)
		{
			for(int b=0; b<mapWidth; ++b)
			{
				int type = templates[r].pMap[a*mapWidth + b].type;
				if(type && type != OT_Castle)
				{
					int len = sprintf(buffer, "\t\t\t%d, %d = %s %d\n", b, a, gpMapObjectTypes[type], (int)templates[r].pMap[a*mapWidth + b].index);
					MFFile_Write(pFile, buffer, len);
				}
			}
		}

		const char *pMapData5 =
			"\t\t}\n"
			"\n"
			"\t\t[Castles]\n"
			"\t\t{\n";
		MFFile_Write(pFile, pMapData5, MFString_Length(pMapData5));

		// sort the castles in order of appearance on the map
		templates[r].castles.qsort(CastleSort);

		for(size_t a=0; a<templates[r].castles.size(); ++a)
		{
			CastleDetails &castle = templates[r].castles[a];
			int len = sprintf(buffer, "\t\t\t[Castle]\n\t\t\t{\n\t\t\t\tname = \"%s\"\n\t\t\t\tposition = %d, %d\n%s", castle.name.CStr(), castle.x, castle.y, castle.bCapital ? "\t\t\t\tcapital = true\n" : "");
			MFFile_Write(pFile, buffer, len);
			for(int c=0; c<castle.numBuildUnits; ++c)
			{
				BuildUnit &unit = castle.buildUnits[c];
				int len = sprintf(buffer, "\t\t\t\tunit %d = \"%s\", %d\n", c, UnitDefs()->GetUnitDetails(unit.unit).name.CStr(), unit.buildTime);
				MFFile_Write(pFile, buffer, len);
			}
			MFFile_Write(pFile, "\t\t\t}\n", MFString_Length("\t\t\t}\n"));
		}

		const char *pMapData6 =
			"\t\t}\n"
			"\t}\n";
		MFFile_Write(pFile, pMapData6, MFString_Length(pMapData6));
	}

	const char *pMapData7 =
		"}\n";
	MFFile_Write(pFile, pMapData7, MFString_Length(pMapData7));

	MFFile_Close(pFile);
}
