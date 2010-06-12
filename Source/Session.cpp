#include "Warlords.h"
#include "Session.h"

Session *Session::pCurrent = NULL;

Session::Session()
{
	bLoggedIn = false;
	bOffline = false;
}

Session::~Session()
{
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

	numCurrentGames = sizeof(currentGames)/sizeof(currentGames[0]);
	err = WLServ_GetActiveGames(id, currentGames, &numCurrentGames);
	if(err != SE_NO_ERROR)
		return err;

	numPastGames = sizeof(pastGames)/sizeof(pastGames[0]);
	err = WLServ_GetPastGames(id, pastGames, &numPastGames);
	if(err != SE_NO_ERROR)
		return err;

	numPendingGames = sizeof(pendingGames)/sizeof(pendingGames[0]);
	err = WLServ_GetPendingGames(id, pendingGames, &numPendingGames);
	if(err != SE_NO_ERROR)
		return err;

	// we're logged in!
	bLoggedIn = true;
	bOffline = false;

	return SE_NO_ERROR;
}

void Session::BeginOffline()
{
	bOffline = true;
	bLoggedIn = false;
}
