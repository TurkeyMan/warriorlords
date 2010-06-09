#include "Warlords.h"
#include "HTTP.h"
#include "ServerRequest.h"

#include "string.h"

ServerError WLServ_CreateAccount(const char *pUsername, const char *pPassword, const char *pEmail, uint32 *pUserID)
{
	ServerError err = SE_NO_ERROR;

	MFFileHTTPRequestArg args[4];
	args[0].SetString("request", "CREATEACCOUNT");
	args[1].SetString("username", pUsername);
	args[2].SetString("password", pPassword);
	args[3].SetString("email", pEmail);

	const char *pResponse = HTTP_Post("localhost", 8888, "/warriorlordsserv", args, sizeof(args)/sizeof(args[0]));

	if(!pResponse)
		return SE_CONNECTION_FAILED;

	char *pLine = strtok((char*)pResponse, "\n");
	while(pLine)
	{
		if(pUserID && !MFString_CaseCmpN(pLine, "USERID", 6) && pLine[6] == '=')
		{
			*pUserID = MFString_AsciiToInteger(pLine + 7);
		}
		else if(!MFString_CaseCmpN(pLine, "ERROR", 5) && pLine[5] == '=')
		{
			err = (ServerError)MFString_AsciiToInteger(pLine + 6);
		}

		pLine = strtok(NULL, "\n");
	}

	return err;
}

ServerError WLServ_Login(const char *pUsername, const char *pPassword, uint32 *pUserID)
{
	ServerError err = SE_NO_ERROR;

	MFFileHTTPRequestArg args[3];
	args[0].SetString("request", "LOGIN");
	args[1].SetString("username", pUsername);
	args[2].SetString("password", pPassword);

	const char *pResponse = HTTP_Post("localhost", 8888, "/warriorlordsserv", args, sizeof(args)/sizeof(args[0]));

	if(!pResponse)
		return SE_CONNECTION_FAILED;

	char *pLine = strtok((char*)pResponse, "\n");
	while(pLine)
	{
		if(pUserID && !MFString_CaseCmpN(pLine, "USERID", 6) && pLine[6] == '=')
		{
			*pUserID = MFString_AsciiToInteger(pLine + 7);
		}
		else if(!MFString_CaseCmpN(pLine, "ERROR", 5) && pLine[5] == '=')
		{
			err = (ServerError)MFString_AsciiToInteger(pLine + 6);
		}

		pLine = strtok(NULL, "\n");
	}

	return err;
}
