#include "Fuji/Fuji.h"
#include "Fuji/MFIni.h"
#include "Fuji/MFMaterial.h"
#include "Fuji/MFSound.h"
#include "Fuji/MFModel.h"
#include "Fuji/MFFont.h"
#include "ResourceCache.h"

void ResourceCache::Init()
{
	materialList.Init(256, 256, 64);
	soundList.Init(256, 256, 64);
	modelList.Init(256, 256, 64);
	fontList.Init(256, 256, 64);
}

void ResourceCache::Deinit()
{
	// free the resources

	materialList.Deinit();
	soundList.Deinit();
	modelList.Deinit();
	fontList.Deinit();
}

void ResourceCache::LoadResources(MFIniLine *pLine)
{
	while(pLine)
	{
		const char *pResource = pLine->GetString(0);

		if(pLine->IsString(1, "font"))
		{
			LoadFont(pResource, MFStr_TruncateExtension(pLine->GetString(2)));
		}
		else if(pLine->IsString(1, "material") || pLine->IsString(1, "image"))
		{
			LoadMaterial(pResource, MFStr_TruncateExtension(pLine->GetString(2)));
		}
		else if(pLine->IsString(1, "model"))
		{
			LoadModel(pResource, MFStr_TruncateExtension(pLine->GetString(2)));
		}
		else if(pLine->IsString(1, "sound"))
		{
			LoadSound(pResource, MFStr_TruncateExtension(pLine->GetString(2)));
		}
		pLine = pLine->Next();
	}
}

void ResourceCache::UnloadResources(MFIniLine *pLine)
{
	while(pLine)
	{
		const char *pResource = pLine->GetString(0);

		if(pLine->IsString(1, "font"))
		{
			MFFont *pFont = fontList[pResource];
			MFFont_Destroy(pFont);
			fontList.Destroy(pResource);
		}
		else if(pLine->IsString(1, "material") || pLine->IsString(1, "image"))
		{
			MFMaterial *pMaterial = materialList[pResource];
			MFMaterial_Destroy(pMaterial);
			materialList.Destroy(pResource);
		}
		else if(pLine->IsString(1, "model"))
		{
			MFModel *pModel = modelList[pResource];
			MFModel_Destroy(pModel);
			modelList.Destroy(pResource);
		}
		else if(pLine->IsString(1, "sound"))
		{
			MFSound *pSound = soundList[pResource];
			MFSound_Destroy(pSound);
			soundList.Destroy(pResource);
		}
		pLine = pLine->Next();
	}
}

void ResourceCache::LoadMaterial(const char *pName, const char *pFilename)
{
	MFMaterial *pMaterial = MFMaterial_Create(pFilename);
	if(pMaterial)
		materialList.Add(pName, pMaterial);
}

void ResourceCache::LoadSound(const char *pName, const char *pFilename)
{
	MFSound *pSound = MFSound_Create(pFilename);
	if(pSound)
		soundList.Add(pName, pSound);
}

void ResourceCache::LoadModel(const char *pName, const char *pFilename)
{
	MFModel *pModel = MFModel_Create(pFilename);
	if(pModel)
		modelList.Add(pName, pModel);
}

void ResourceCache::LoadFont(const char *pName, const char *pFilename)
{
	MFFont *pFont = MFFont_Create(pFilename);
	if(pFont)
		fontList.Add(pName, pFont);
}

MFMaterial *ResourceCache::FindMaterial(const char *pName)
{
	MFMaterial **ppT = materialList.Get(pName);
	return ppT ? *ppT : NULL;
}

MFSound *ResourceCache::FindSound(const char *pName)
{
	MFSound **ppT = soundList.Get(pName);
	return ppT ? *ppT : NULL;
}

MFModel *ResourceCache::FindModel(const char *pName)
{
	MFModel **ppT = modelList.Get(pName);
	return ppT ? *ppT : NULL;
}

MFFont *ResourceCache::FindFont(const char *pName)
{
	MFFont **ppT = fontList.Get(pName);
	return ppT ? *ppT : NULL;
}
