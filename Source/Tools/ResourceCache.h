#if !defined(_RESOURCE_CACHE)
#define _RESOURCE_CACHE

#include "HashTable.h"

struct MFMaterial;
struct MFSound;
struct MFModel;
struct MFFont;

class MFIniLine;

class ResourceCache
{
public:
	void Init();
	void Deinit();

	void LoadResources(MFIniLine *pList);
	void UnloadResources(MFIniLine *pList);

	void LoadMaterial(const char *pName, const char *pFilename);
	void LoadSound(const char *pName, const char *pFilename);
	void LoadModel(const char *pName, const char *pFilename);
	void LoadFont(const char *pName, const char *pFilename);

	MFMaterial *FindMaterial(const char *pName);
	MFSound *FindSound(const char *pName);
	MFModel *FindModel(const char *pName);
	MFFont *FindFont(const char *pName);

protected:
	HashList<MFMaterial> materialList;
	HashList<MFSound> soundList;
	HashList<MFModel> modelList;
	HashList<MFFont> fontList;
};

#endif
