#include "Warlords.h"
#include "Castle.h"
#include "Tileset.h"

#include "MFIni.h"
#include "MFMaterial.h"
#include "MFPrimitive.h"

CastleSet *CastleSet::Create(const char *pFilename)
{
	MFIni *pIni = MFIni::Create(pFilename);
	if(!pIni)
		return NULL;

	CastleSet *pNew = NULL;

	MFIniLine *pLine = pIni->GetFirstLine();

	while(pLine)
	{
		if(pLine->IsSection("CastleSet"))
		{
			pNew = (CastleSet*)MFHeap_AllocAndZero(sizeof(CastleSet));

			MFIniLine *pCastleSet = pLine->Sub();

			while(pCastleSet)
			{
				if(pCastleSet->IsString(0, "name"))
				{
					MFString_Copy(pNew->name, pCastleSet->GetString(1));
				}
				else if(pCastleSet->IsString(0, "roads"))
				{
					pNew->pRoadMap = MFMaterial_Create(MFStr_TruncateExtension(pCastleSet->GetString(1)));
				}
				else if(pCastleSet->IsString(0, "tile_width"))
				{
					pNew->tileWidth = pCastleSet->GetInt(1);
				}
				else if(pCastleSet->IsString(0, "tile_height"))
				{
					pNew->tileHeight = pCastleSet->GetInt(1);
				}

				pCastleSet = pCastleSet->Next();
			}
		}

		pLine = pLine->Next();
	}

	MFIni::Destroy(pIni);

	return pNew;
}

void CastleSet::Destroy()
{
	MFMaterial_Destroy(pRoadMap);
	MFHeap_Free(this);
}
