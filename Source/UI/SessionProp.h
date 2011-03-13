#if !defined(_SESSIONPROP_H)
#define _SESSIONPROP_H

#include "Entity.h"
#include "Session.h"

class uiSessionProp : public uiEntity
{
public:
	static void RegisterEntity();
	static void *Create() { return new uiSessionProp; }

	uiSessionProp();
	virtual ~uiSessionProp();

	virtual void Update();
	virtual void Draw(const uiDrawState &state);

protected:
	void RunScript(const char *pScript, const char *pParams = NULL);

	static MFString GetUserID(uiEntity *pEntity);
	static MFString GetUsername(uiEntity *pEntity);
	static MFString GetOnline(uiEntity *pEntity);

	static MFString GetCurrentGame(uiEntity *pEntity);
	static MFString GetCurrentGameOnline(uiEntity *pEntity);

	static void LoginAction(uiEntity *pEntity, uiRuntimeArgs *pArguments);
	static void LogoutAction(uiEntity *pEntity, uiRuntimeArgs *pArguments);
	static void FindGames(uiEntity *pEntity, uiRuntimeArgs *pArguments);

	static void CreateOnline(uiEntity *pEntity, uiRuntimeArgs *pArguments);
	static void CreateOffline(uiEntity *pEntity, uiRuntimeArgs *pArguments);
	static void JoinGame(uiEntity *pEntity, uiRuntimeArgs *pArguments);
	static void ResumeGame(uiEntity *pEntity, uiRuntimeArgs *pArguments);

	static void EnterGame(uiEntity *pEntity, uiRuntimeArgs *pArguments);

	Session *pSession;

	MFString callback;

	MFString gameName;
	uint32 gameID;
	bool bOffline;
};

#endif
