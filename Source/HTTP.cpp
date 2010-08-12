#include "Warlords.h"
#include "HTTP.h"

#include "MFSockets.h"
#include "stdio.h"

static char gResponseBuffer[256 * 1024];

const char *HTTP_Get(const char *pServer, int port, const char *pResourcePath)
{
	MFAddressInfo *pAddrInfo;
	if(MFSockets_GetAddressInfo(pServer, MFStr("%d", port), NULL, &pAddrInfo) == 0)
	{
		char buffer[1024];
		int len = sprintf(buffer, "GET %s HTTP/1.1\nHost: %s:%d\nUser-Agent: WarriorLords Client/1.0\nContent-Type: application/x-www-form-urlencoded\n\n\n", pResourcePath, pServer, port);

		MFSocket s = MFSockets_CreateSocket(pAddrInfo->pAddress->family, MFSockType_Stream, MFProtocol_TCP);

		int r = MFSockets_Connect(s, *pAddrInfo->pAddress);
		if(r != 0)
		{
			MFSockets_CloseSocket(s);
			return NULL;
		}

		MFSockets_Send(s, buffer, len, 0);
		MFSockets_Recv(s, gResponseBuffer, sizeof(gResponseBuffer)-1, 0);
		MFSockets_CloseSocket(s);

		gResponseBuffer[sizeof(gResponseBuffer)-1] = 0;
		return gResponseBuffer;
	}

	return NULL;
}

const char *HTTP_Post(const char *pServer, int port, const char *pResourcePath, MFFileHTTPRequestArg *pArgs, int numArgs)
{
	MFAddressInfo *pAddrInfo;
	if(MFSockets_GetAddressInfo(pServer, MFStr("%d", port), NULL, &pAddrInfo) == 0)
	{
		char *pArgString = gResponseBuffer;
		pArgString[0] = 0;

		if(pArgs)
		{
			int argLen = 0;

			// build the string from the supplied args
			for(int a=0; a<numArgs; ++a)
			{
				switch(pArgs[a].type)
				{
					case MFFileHTTPRequestArg::AT_Int:
						argLen += sprintf(pArgString + argLen, "%s%s=%d", argLen ? "&" : "", MFStr_URLEncodeString(pArgs[a].pArg), pArgs[a].iValue);
						break;
					case MFFileHTTPRequestArg::AT_Float:
						argLen += sprintf(pArgString + argLen, "%s%s=%g", argLen ? "&" : "", MFStr_URLEncodeString(pArgs[a].pArg), pArgs[a].fValue);
						break;
					case MFFileHTTPRequestArg::AT_String:
						argLen += sprintf(pArgString + argLen, "%s%s=%s", argLen ? "&" : "", MFStr_URLEncodeString(pArgs[a].pArg), MFStr_URLEncodeString(pArgs[a].pValue));
						break;
				}
			}
			pArgString[argLen] = 0;
		}

		char buffer[2048];
		int len = sprintf_s(buffer, sizeof(buffer), "POST %s HTTP/1.1\nHost: %s:%d\nUser-Agent: WarriorLords Client/1.0\nContent-Type: application/x-www-form-urlencoded\nContent-Length: %d\n\n%s", pResourcePath, pServer, port, MFString_Length(pArgString), pArgString);

		MFSocket s = MFSockets_CreateSocket(pAddrInfo->pAddress->family, MFSockType_Stream, MFProtocol_TCP);

		int r = MFSockets_Connect(s, *pAddrInfo->pAddress);
		if(r != 0)
		{
			MFSockets_CloseSocket(s);
			return NULL;
		}

		MFSockets_Send(s, buffer, len, 0);
		int bytes = MFSockets_Recv(s, gResponseBuffer, sizeof(gResponseBuffer)-1, 0);
		MFSockets_CloseSocket(s);

		gResponseBuffer[bytes] = 0;
		gResponseBuffer[sizeof(gResponseBuffer)-1] = 0;
		return gResponseBuffer;
	}

	return NULL;
}
