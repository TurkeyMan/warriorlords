#if !defined(_SERVER_H)
#define _SERVER_H

enum ServerError
{
	SE_CONNECTION_REFUSED = -3,
	SE_CANT_FIND_HOST = -2,
	SE_CONNECTION_FAILED = -1,

	SE_NO_ERROR = 0,

	SE_INVALID_REQUEST,
	SE_INVALID_ARGUMENTS,
	SE_INVALID_USER,
	SE_INVALID_GAME,
	SE_INVALID_LOGIN,
	SE_GAME_FULL,
	SE_ALREADY_PRESENT,
	SE_NOT_IN_GAME,
	SE_UNABLE_TO_START_GAME
};

ServerError WLServ_CreateAccount(const char *pUsername, const char *pPassword, const char *pEmail, uint32 *pUserID);
ServerError WLServ_Login(const char *pUsername, const char *pPassword, uint32 *pUserID);

#endif
