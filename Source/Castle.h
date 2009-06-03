#if !defined(_CASTLE_H)
#define _CASTLE_H

struct MFMaterial;

class CastleSet
{
public:
	static CastleSet *Create(const char *pFilename);
	void Destroy();

	int DrawCastle(int race);
	int DrawFlag(int race);
	int DrawSpecial(int index);

	inline int GetNumRaces() { return raceCount; }
	inline const char *GetRaceName(int type) { return pRaces[type].name; }

	inline int GetNumSpecials() { return specialCount; }
	inline const char *GetSpecialName(int type) { return pSpecials[type].name; }

	inline MFMaterial *GetCastleMaterial() { return pImage; }
	inline MFMaterial *GetRoadMaterial() { return pRoadMap; }

	void GetCastleUVs(int race, MFRect *pUVs);
	void GetFlagUVs(int race, MFRect *pUVs);
	void GetSpecialUVs(int index, MFRect *pUVs);
	void GetRoadUVs(int index, MFRect *pUVs);

	MFVector GetRaceColour(int race);

protected:
	struct Race
	{
		char name[32];
		uint8 castlex, castley;
		uint8 flagx, flagy;
		uint32 colour;
	};

	struct Special
	{
		char name[28];
		uint8 x, y;
		uint16 canSearch : 1;
		uint16 flags : 15;
	};

	struct Road
	{
		uint8 x, y;
		uint8 directions;
		uint8 reserved;
		uint32 terrain;
	};

	char name[64];

	int tileWidth, tileHeight;
	int imageWidth, imageHeight;

	MFMaterial *pImage;
	MFMaterial *pRoadMap;

	Race *pRaces;
	int raceCount;

	Special *pSpecials;
	int specialCount;

	Road *pRoads;
	int roadCount;
};

#endif
