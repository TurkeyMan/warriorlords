#if !defined(_SERVER_REQUEST_H)
#define _SERVER_REQUEST_H

#pragma warning(disable:4355)

#include "HTTP.h"
#include "Fuji/MFDocumentJSON.h"

class Session;

struct GameCreateDetails
{
	const char *pName;
	const char *pMap;
	int numPlayers;
	int turnTime;
};

class ServerRequest
{
public:
	enum ServerError
	{
		SE_CONNECTION_REFUSED = -5,
		SE_CANT_FIND_HOST = -4,
		SE_CONNECTION_FAILED = -3,
		SE_INVALID_RESPONSE = -2,
		SE_PENDING = -1,

		SE_NO_ERROR = 0,

		SE_SERVER_ERROR,
		SE_EXPECTED_ARGUMENTS,
		SE_INVALID_ARGUMENTS,
		SE_ALREADY_EXISTS,
		SE_INVALID_USER,
		SE_INVALID_GAME,
		SE_INVALID_LOGIN,
		SE_GAME_FULL,
		SE_ALREADY_PRESENT,
		SE_NOT_IN_GAME,
	};

	typedef FastDelegate1<ServerRequest*> EventDelegate;

	ServerRequest(EventDelegate completeDelegate = NULL);
	~ServerRequest();

	void CreateAccount(const char *pUsername, const char *pPassword, const char *pEmail);
	void Login(const char *pUsername, const char *pPassword);
	void GetUserByID(uint32 id);
	void GetUserByName(const char *pUsername);

	void CreateGame(Session *pSession, const GameCreateDetails *pDetails);
	void GetGame(Session *pSession, uint32 id, bool bActions = true, bool bPlayers = true, int firstAction = 0);

	void JoinGame(Session *pSession, uint32 game);
	void LeaveGame(Session *pSession, uint32 game);
	void ConfigureGame(Session *pSession, uint32 game, int race, int colour, int hero, bool bReady);

	void CommitActions(Session *pSession, uint32 game, Action *pActions, int numActions);

	HTTPRequest::Status HTTPStatus() { return pRequest->GetStatus(); }
	HTTPResponse *Response() { return pRequest->GetResponse(); }

	ServerError Status() { return error; }
	MFString Error() { return ""; }

	MFJSONValue *Json();

private:
	HTTPRequest *pRequest;
	EventDelegate completeDelegate;

	ServerError error;
	const char *pStatusString;
	const char *pErrorString;

	MFDocumentJSON *pJson;

	void OnHTTPEvent(HTTPRequest*);
	void OnHTTPCompleted(HTTPRequest*);
};

#endif
