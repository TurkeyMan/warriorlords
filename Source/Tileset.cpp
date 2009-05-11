#include "Warlords.h"
#include "Tileset.h"

#include "MFIni.h"
#include "MFMaterial.h"
#include "MFPrimitive.h"

Tileset *Tileset::Create(const char *pFilename)
{
	MFIni *pIni = MFIni::Create(pFilename);
	if(!pIni)
		return NULL;

	Tileset *pNew = NULL;

	MFIniLine *pLine = pIni->GetFirstLine();

	while(pLine)
	{
		if(pLine->IsSection("Tilemap"))
		{
			pNew = (Tileset*)MFHeap_AllocAndZero(sizeof(Tileset));
			MFMemSet(pNew->tiles, 0xFF, sizeof(pNew->tiles));

			MFIniLine *pTilemap = pLine->Sub();

			while(pTilemap)
			{
				if(pTilemap->IsString(0, "name"))
				{
					MFString_Copy(pNew->name, pTilemap->GetString(1));
				}
				else if(pTilemap->IsString(0, "tilemap"))
				{
					pNew->pTileMap = MFMaterial_Create(MFStr_TruncateExtension(pTilemap->GetString(1)));
//					pNew->imageWidth = ;
//					pNew->imageHeight = ;
				}
				else if(pTilemap->IsString(0, "tile_width"))
				{
					pNew->tileWidth = pTilemap->GetInt(1);
				}
				else if(pTilemap->IsString(0, "tile_height"))
				{
					pNew->tileHeight = pTilemap->GetInt(1);
				}
				else if(pTilemap->IsSection("Terrain"))
				{
					pNew->terrainCount = 0;

					MFIniLine *pTerrain = pTilemap->Sub();
					while(pTerrain)
					{
						if(!pTerrain->IsString(0, "section"))
							pNew->terrainCount = MFMax(pNew->terrainCount, pTerrain->GetInt(0) + 1);

						pTerrain = pTerrain->Next();
					}

					pNew->pTerrainTypes = (TerrainType*)MFHeap_AllocAndZero(sizeof(TerrainType) * pNew->terrainCount);

					pTerrain = pTilemap->Sub();
					while(pTerrain)
					{
						if(!pTerrain->IsString(0, "section"))
							MFString_Copy(pNew->pTerrainTypes[pTerrain->GetInt(0)].name, pTerrain->GetString(1));
						else if(pTerrain->IsSection("Transitions"))
						{
							MFIniLine *pTransitions = pTerrain->Sub();

							while(pTransitions)
							{
								MFDebug_Assert(pTransitions->GetStringCount()-1 == pNew->terrainCount, "Invalid transition table dimensions!");
								for(int a=0; a<pNew->terrainCount; ++a)
									pNew->terrainTransitions[pTransitions->GetInt(0)][a] = pTransitions->GetInt(a + 1);

								pTransitions = pTransitions->Next();
							}
						}

						pTerrain = pTerrain->Next();
					}
				}
				else if(pTilemap->IsSection("Special"))
				{
					pNew->specialCount = 0;

					MFIniLine *pSpecial = pTilemap->Sub();
					while(pSpecial)
					{
						pNew->specialCount = MFMax(pNew->specialCount, pSpecial->GetInt(0) + 1);
						pSpecial = pSpecial->Next();
					}

					pNew->pSpecialTiles = (SpecialType*)MFHeap_AllocAndZero(sizeof(SpecialType) * pNew->specialCount);

					pSpecial = pTilemap->Sub();
					while(pSpecial)
					{
						int s = pSpecial->GetInt(0);
						MFString_Copy(pNew->pSpecialTiles[s].name, pSpecial->GetString(1));

						for(int a=2; a<pSpecial->GetStringCount(); ++a)
						{
							if(pSpecial->IsString(a, "searchable"))
								pNew->pSpecialTiles[s].canSearch = 1;
						}

						pSpecial = pSpecial->Next();
					}
				}
				else if(pTilemap->IsSection("Tiles"))
				{
					MFIniLine *pTile = pTilemap->Sub();
					while(pTile)
					{
						int x = pTile->GetInt(0);
						int y = pTile->GetInt(1);
						MFDebug_Assert(x < 16 && y < 16, "Tile sheets may have a maximum of 16x16 tiles.");
						MFDebug_Assert(pTile->IsString(2, "="), "Expected '='.");

						uint8 i = (x & 0xF) | ((y << 4) & 0xF0);
						Tile &t = pNew->tiles[i];

						t.x = x;
						t.y = y;
						t.terrain = EncodeTile(pTile->GetInt(3), pTile->GetInt(4), pTile->GetInt(5), pTile->GetInt(6));

						if(pTile->IsString(7, "tile"))
						{
							t.type = Tile::Regular;
						}
						else if(pTile->IsString(7, "road"))
						{
							t.type = Tile::Road;
						}
						else if(pTile->IsString(7, "special"))
						{
							t.type = Tile::Special;
						}

						pTile = pTile->Next();
					}
				}

				pTilemap = pTilemap->Next();
			}
		}

		pLine = pLine->Next();
	}

	MFIni::Destroy(pIni);

	return pNew;
}

void Tileset::Destroy()
{
	MFMaterial_Destroy(pTileMap);
	MFHeap_Free(pTerrainTypes);
	MFHeap_Free(pSpecialTiles);
	MFHeap_Free(this);
}

void Tileset::DrawMap(int xTiles, int yTiles, uint8 *pTileData, int stride)
{
	// we should set the ortho rect to map tiles to the render target texture...
//	MFView_Push();
//	MFRect rect = { 0, 0, textureWidth / tileWidth, textureHeight / tileHeight };
//	MFView_SetOrtho(&rect);

	MFMaterial_SetMaterial(pTileMap);

	float xScale = (1.f / 1024.f) * tileWidth;
	float yScale = (1.f / 1024.f) * tileHeight;
	float halfX = 0.5f / 1024.f;
	float halfY = 0.5f / 1024.f;

	MFPrimitive(PT_TriList);
	MFBegin(6*xTiles*yTiles);
	MFSetColour(MFVector::white);

	for(int y=0; y<yTiles; ++y)
	{
		for(int x=0; x<xTiles; ++x)
		{
			Tile &t = tiles[pTileData[x]];

			MFSetTexCoord1(t.x*xScale + halfX, t.y*yScale + halfY);
			MFSetPosition((float)x, (float)y, 0);
			MFSetTexCoord1((t.x+1)*xScale + halfX, t.y*yScale + halfY);
			MFSetPosition((float)(x+1), (float)y, 0);
			MFSetTexCoord1(t.x*xScale + halfX, (t.y+1)*yScale + halfY);
			MFSetPosition((float)x, (float)(y+1), 0);

			MFSetTexCoord1((t.x+1)*xScale + halfX, t.y*yScale + halfY);
			MFSetPosition((float)(x+1), (float)y, 0);
			MFSetTexCoord1((t.x+1)*xScale + halfX, (t.y+1)*yScale + halfY);
			MFSetPosition((float)(x+1), (float)(y+1), 0);
			MFSetTexCoord1(t.x*xScale + halfX, (t.y+1)*yScale + halfY);
			MFSetPosition((float)x, (float)(y+1), 0);
		}

		pTileData += stride;
	}

	MFEnd();

//	MFView_Pop();
}

int Tileset::FindBestTiles(int *pTiles, uint32 tile, uint32 mask, int maxMatches)
{
	uint32 maskedTile = tile & mask;

	int found = 0;
	int error = 4;
	for(int a=0; a<256; ++a)
	{
		if((tiles[a].terrain & mask) == maskedTile)
		{
			int e = GetTileError(tiles[a].terrain, tile);
			if(e < error)
			{
				error = e;
				pTiles[0] = a;
				found = 1;
			}
			else if(e == error && found < maxMatches)
				pTiles[found++] = a;
		}
	}

	return found;
}

void Tileset::GetTileUVs(int tile, MFRect *pUVs)
{
	float xScale = (1.f / 1024.f) * tileWidth;
	float yScale = (1.f / 1024.f) * tileHeight;
	float halfX = 0.5f / 1024.f;
	float halfY = 0.5f / 1024.f;

	Tile &t = tiles[tile];
	pUVs->x = t.x*xScale + halfX;
	pUVs->y = t.y*yScale + halfY;
	pUVs->width = xScale;
	pUVs->height = yScale;
}


/*
#include<iostream.h>
#include<stdlib.h>

#define MAX 20
#define INFINITY 9999

class dijkstra
{
private:
 int n;
 int graph[MAX][MAX];
 int colour[MAX];
 int start;
 int distance[MAX];
 int predecessor[MAX];
 enum {green,yellow,red};
public:
 void read_graph();
 void initialize();
 int select_min_distance_lable();
 void update(int);
 void output();
 void function();
};

void dijkstra::read_graph()
{
 cout<<”Enter the no. of nodes in the graph ::”;
 cin>>n;
 cout<<”Enter the adjacency matrix for the graph ::\n”;
 int i,j;
 for(i=1;i<=n;i++)
  for(j=1;j<=n;j++)
   cin>>graph[i][j];
 for(i=1;i<=n;i++)
  colour[i]=green;

 cout<<”Enter the start vertex ::”;
 cin>>start;
}

void dijkstra::initialize()
{
 for(int i=1;i<=n;i++)
 {
  if(i==start)
   distance[i]=0;
  else
   distance[i]=INFINITY;
 }

 for(int j=1;j<=n;j++)
 {
  if(graph[start][j]!=0)
   predecessor[j]=start;
  else
   predecessor[j]=0;
 }
}

int dijkstra::select_min_distance_lable()
{
 int min=INFINITY;
 int p=0;
 for(int i=1;i<=n;i++)
 {
  if(colour[i]==green)
  {
   if(min>=distance[i])
   {
    min=distance[i];
    p=i;
   }
  }
 }
 return  p;
}

void dijkstra::update(int p)       // p is a yellow colour node
{
 cout<<”\nupdated distances are ::\n”;
 for(int i=1;i<=n;i++)
 {
  if(colour[i]==green)
  {
   if(graph[p][i]!=0)
   {
    if(distance[i]>graph[p][i]+distance[p])
    {
     distance[i]=graph[p][i]+distance[p];
     predecessor[i]=p;
    }
   }
  }
  cout<<distance[i]<<’\t’;
 }
}

void dijkstra::output()
{
 cout<<”****** The final paths and the distacnes are ******\n\n”;

 for(int i=1;i<=n;i++)
 {
  if(predecessor[i]==0 && i!=start)
  {
   cout<<”path does not exists between “<<i<<” and the start vertex “
    <<start<<endl;
   exit(1);
  }
  cout<<”path for node “<<i<<” is ::\n”;
  int j=i;
  int array[MAX];
  int l=0;
  while(predecessor[j]!=0)
  {
   array[++l]=predecessor[j];
   j=predecessor[j];
  }
  for(int k=l;k>=1;k–)
   cout<<array[k]<<”->”;

  cout<<i<<endl;
  cout<<”distance is “<<distance[i]<<endl<<endl<<endl;
 }
}

void dijkstra::function()
{
 cout<<”\n**********************************************************************\n”;
 cout<<”This program is to implement dijkstra’s algorithm using colour codes \n”;
 cout<<”**********************************************************************\n\n”;
 read_graph();
 initialize();
 //repeate until all nodes become red
 int flag=0;
 int i;

 cout<<”\n\n******** The working of the algorithm is **********\n\n”;

 for(i=1;i<=n;i++)
  if(colour[i]!=red)
   flag=1;

 cout<<”The initial distances are ::\n”;
 for(i=1;i<=n;i++)
  cout<<distance[i]<<’\t’;
 cout<<endl;

 while(flag)
 {
  int p=select_min_distance_lable();
  cout<<”\nThe min distance lable that is coloured yellow is “<<p;
  colour[p]=yellow;

  update(p);
  cout<<”\nnode “<<p<<” is coloured red “<<endl;
  colour[p]=red;

  flag=0;
  for(i=1;i<=n;i++)
   if(colour[i]!=red)
    flag=1;

  cout<<endl<<endl<<endl;
 }
 output();
}
*/