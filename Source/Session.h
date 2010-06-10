#if !defined(_SESSION_H)
#define _SESSION_H

#include "ServerRequest.h"

class Session
{
public:
	Session();
	~Session();

	ServerError Login(const char *pUsername, const char *pPassword);
	void BeginOffline();

	void LoadSession(uint32 user);
	void SaveSession();

	bool IsActive();
	bool IsOffline();

	static Session *SetCurrent(Session *pNew) { Session *pOld = pCurrent; pCurrent = pNew; return pOld; }
	static Session *GetCurrent() { return pCurrent; }

protected:
	UserDetails user;

	uint32 currentGames[64];
	int numCurrentGames;
	uint32 pastGames[1024];
	int numPastGames;
	uint32 pendingGames[32];
	int numPendingGames;

	bool bLoggedIn;
	bool bOffline;

	static Session *pCurrent;
};

#endif
