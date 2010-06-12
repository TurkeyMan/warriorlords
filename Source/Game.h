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

struct GameParams
{
	const char *pMap;

	int numPlayers;
	int playerRaces[16];
	uint32 playerColours[16];

	bool bOnline;
	bool bEditMap;
};

class Game
{
public:
	Game(GameParams *pParams);
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

	void PushGroupPosition(Group *pGroup);
	Group *UndoLastMove();
	int GetUndoDepth() { return undoDepth; }
	void ClearUndoStack();

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

	struct UndoStack
	{
		Group *pGroup;
		int x, y;
		int vehicleMove;
		int unitMove[10];
	};

	static const int MaxUndo = 64;
	UndoStack undoStack[MaxUndo];
	int undoDepth;

	static Game *pCurrent;
};

#endif
