#if !defined(_CASTLE_H)
#define _CASTLE_H

struct MFMaterial;

class CastleSet
{
public:
	static CastleSet *Create(const char *pFilename);
	void Destroy();

protected:

	char name[64];

	int tileWidth, tileHeight;
	int imageWidth, imageHeight;
};

#endif
