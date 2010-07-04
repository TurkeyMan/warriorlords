#if !defined(_SESSION_H)
#define _SESSION_H

#include "ServerRequest.h"

class Session
{
public:
	Session();
	~Session();

	ServerError UpdateState();

	ServerError Login(const char *pUsername, const char *pPassword);
	void BeginOffline();

	void LoadSession(uint32 user);
	void SaveSession();

	bool IsActive() { return bLoggedIn || bOffline; }
	bool IsOffline() { return bOffline; }

	uint32 GetUserID() { return user.id; }
	const char *GetUsername() { return user.userName; }

	static Session *SetCurrent(Session *pNew) { Session *pOld = pCurrent; pCurrent = pNew; return pOld; }
	static Session *GetCurrent() { return pCurrent; }

	int GetNumCurrentGames() { return numCurrentGames; }
	int GetNumPendingGames() { return numPendingGames; }
	int GetNumPastGames() { return numPastGames; }

	GameDetails *GetCurrentGame(int game) { return &pCurrentGames[game]; }
	GameDetails *GetPendingGame(int game) { return &pPendingGames[game]; }
	GameDetails *GetPastGame(int game) { return &pPastGames[game]; }

protected:
	UserDetails user;

	bool bLoggedIn;
	bool bOffline;

	GameDetails *pCurrentGames;
	int numCurrentGames;
	GameDetails *pPendingGames;
	int numPendingGames;

	uint32 pastGames[1024];
	GameDetails *pPastGames;
	int numPastGames;

	// manage these locally
	uint32 localGames[64];
	int numLocalGames;

	static Session *pCurrent;
};

#endif
