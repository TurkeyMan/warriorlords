#include "Warlords.h"
#include "Session.h"

Session *Session::pCurrent = NULL;

Session::Session()
{
	bLoggedIn = false;
	bOffline = false;

	pCurrentGames = NULL;
	pPendingGames = NULL;
	pPastGames = NULL;
}

Session::~Session()
{
}

ServerError Session::UpdateState()
{
	// clear the old cache...
	numCurrentGames = numPendingGames = numPastGames = 0;

	if(pCurrentGames)
	{
		MFHeap_Free(pCurrentGames);
		pCurrentGames = NULL;
	}
	if(pPendingGames)
	{
		MFHeap_Free(pPendingGames);
		pPendingGames = NULL;
	}
	if(pPastGames)
	{
		MFHeap_Free(pPastGames);
		pPastGames = NULL;
	}

	uint32 games[1024];

	// get current games
	numCurrentGames = sizeof(games)/sizeof(games[0]);
	ServerError err = WLServ_GetActiveGames(user.id, games, &numCurrentGames);
	if(err != SE_NO_ERROR)
		return err;

	if(numCurrentGames)
	{
		pCurrentGames = (GameState*)MFHeap_AllocAndZero(sizeof(GameState) * numCurrentGames);
		for(int a=0; a<numCurrentGames; ++a)
			WLServ_GameState(games[a], &pCurrentGames[a]);
	}

	// get pending games
	numPendingGames = sizeof(games)/sizeof(games[0]);
	err = WLServ_GetPendingGames(user.id, games, &numPendingGames);
	if(err != SE_NO_ERROR)
		return err;

	if(numPendingGames)
	{
		pPendingGames = (GameDetails*)MFHeap_AllocAndZero(sizeof(GameDetails) * numPendingGames);
		for(int a=0; a<numPendingGames; ++a)
			WLServ_GetGameByID(games[a], &pPendingGames[a]);
	}

	// get past games
	numPastGames = sizeof(pastGames)/sizeof(pastGames[0]);
	err = WLServ_GetPastGames(user.id, pastGames, &numPastGames);
	if(err != SE_NO_ERROR)
		return err;

	// don't fetch the past games until they are requested (user views history)
	pPastGames = NULL;

	return SE_NO_ERROR;
}

ServerError Session::Login(const char *pUsername, const char *pPassword)
{
	// try and login...
	uint32 id;
	ServerError err = WLServ_Login(pUsername, pPassword, &id);
	if(err != SE_NO_ERROR)
		return err;

	err = WLServ_GetUserByID(id, &user);
	if(err != SE_NO_ERROR)
		return err;

	// we're logged in!
	bLoggedIn = true;
	bOffline = false;

	// reset the game cache
	numCurrentGames = numPendingGames = numPastGames = 0;

	return SE_NO_ERROR;
}

void Session::BeginOffline()
{
	bOffline = true;
	bLoggedIn = false;
}
