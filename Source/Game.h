#if !defined(_GAME_H)
#define _GAME_H

#include "Map.h"
#include "Unit.h"
#include "Screens/RequestBox.h"

struct MFFont;

class MapScreen;
class Battle;

struct Player
{
	MFVector colour;
	int race;
	int gold;

	int cursorX, cursorY;

	Unit *pHero;
};

class Game
{
public:
	Game(const char *pMap, bool bEditable = false);
	~Game();

	MapScreen *GetMapScreen() { return pMapScreen; }
	Battle *GetBattleScreen() { return pBattle; }

	void BeginGame();
	void BeginTurn(int player);
	void EndTurn();
	void BeginBattle(Group *pGroup, MapTile *pTarget);
	void EndBattle(Group *pGroup, MapTile *pTarget);

	Map *GetMap() { return pMap; }
	UnitDefinitions *GetUnitDefs() { return pUnitDefs; }

	int GetCurrentTurn() { return currentTurn; }
	int CurrentPlayer() { return currentPlayer; }
	int GetPlayerRace(int player) { return player == -1 ? 0 : players[player].race; }
	int GetPlayerGold(int player) { return player == -1 ? 0 : players[player].gold; }
	MFVector GetPlayerColour(int player) { return player == -1 ? pUnitDefs->GetRaceColour(0) : players[player].colour; }
	Unit *GetPlayerHero(int player) { return players[player].pHero; }

	static void SetCurrent(Game *pGame) { pCurrent = pGame; }
	static Game *GetCurrent() { return pCurrent; }

	Group *CreateUnit(int unit, Castle *pCastle);

	void DrawWindow(const MFRect &rect);
	void DrawLine(float sx, float sy, float dx, float dy);

	MFFont *GetTextFont() { return pText; }
	MFFont *GetBattleNumbersFont() { return pBattleNumbersFont; }
	MFFont *GetSmallNumbersFont() { return pSmallNumbersFont; }

	void ShowRequest(const char *pMessage, RequestBox::SelectCallback *pCallback, bool bNotification, void *pUserData = NULL);
	bool DrawRequest();

protected:
	// game resources
	MFFont *pText;
	MFFont *pBattleNumbersFont;
	MFFont *pSmallNumbersFont;

	MFMaterial *pWindow;
	MFMaterial *pHorizLine;

	// game data
	Map *pMap;
	UnitDefinitions *pUnitDefs;

	Player players[8];
	int numPlayers;

	// screens
	MapScreen *pMapScreen;
	Battle *pBattle;

	RequestBox *pRequestBox;

	// game state data
	int currentPlayer;
	int currentTurn;

	static Game *pCurrent;
};

#endif
