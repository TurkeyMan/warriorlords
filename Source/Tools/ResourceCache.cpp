#include "Fuji.h"
#include "MFIni.h"
#include "MFMaterial.h"
#include "MFSound.h"
#include "MFModel.h"
#include "MFFont.h"
#include "HashTable.h"
#include "ResourceCache.h"

void ResourceCache::Init()
{
	materialList.Create(256, 256, 64);
	soundList.Create(256, 256, 64);
	modelList.Create(256, 256, 64);
	fontList.Create(256, 256, 64);
}

void ResourceCache::Deinit()
{
	// free the resources

	materialList.Destroy();
	soundList.Destroy();
	modelList.Destroy();
	fontList.Destroy();
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
			MFFont *pFont = fontList.Find(pResource);
			MFFont_Destroy(pFont);
			fontList.Remove(pResource);
		}
		else if(pLine->IsString(1, "material") || pLine->IsString(1, "image"))
		{
			MFMaterial *pMaterial = materialList.Find(pResource);
			MFMaterial_Destroy(pMaterial);
			materialList.Remove(pResource);
		}
		else if(pLine->IsString(1, "model"))
		{
			MFModel *pModel = modelList.Find(pResource);
			MFModel_Destroy(pModel);
			modelList.Remove(pResource);
		}
		else if(pLine->IsString(1, "sound"))
		{
			MFSound *pSound = soundList.Find(pResource);
			MFSound_Destroy(pSound);
			soundList.Remove(pResource);
		}
		pLine = pLine->Next();
	}
}

void ResourceCache::LoadMaterial(const char *pName, const char *pFilename)
{
	MFMaterial *pMaterial = MFMaterial_Create(pFilename);
	if(pMaterial)
		materialList.Add(pMaterial, pName);
}

void ResourceCache::LoadSound(const char *pName, const char *pFilename)
{
	MFSound *pSound = MFSound_Create(pFilename);
	if(pSound)
		soundList.Add(pSound, pName);
}

void ResourceCache::LoadModel(const char *pName, const char *pFilename)
{
	MFModel *pModel = MFModel_Create(pFilename);
	if(pModel)
		modelList.Add(pModel, pName);
}

void ResourceCache::LoadFont(const char *pName, const char *pFilename)
{
	MFFont *pFont = MFFont_Create(pFilename);
	if(pFont)
		fontList.Add(pFont, pName);
}

MFMaterial *ResourceCache::FindMaterial(const char *pName)
{
	return materialList.Find(pName);
}

MFSound *ResourceCache::FindSound(const char *pName)
{
	return soundList.Find(pName);
}

MFModel *ResourceCache::FindModel(const char *pName)
{
	return modelList.Find(pName);
}

MFFont *ResourceCache::FindFont(const char *pName)
{
	return fontList.Find(pName);
}
