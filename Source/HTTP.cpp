#include "Warlords.h"
#include "HTTP.h"

#include "MFSockets.h"
#include "stdio.h"
#include "string.h"

static const char *gTransferEncodings[HTTP_TE_Max] =
{
	"identity",
	"chunked",
	"compress",
	"deflate",
	"gzip"
};

HTTPResponse *HTTP_Get(const char *pServer, int port, const char *pResourcePath)
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
		HTTPResponse *pResponse = HTTPResponse::Create(s);
		MFSockets_CloseSocket(s);

		return pResponse;
	}

	return NULL;
}

HTTPResponse *HTTP_Post(const char *pServer, int port, const char *pResourcePath, MFFileHTTPRequestArg *pArgs, int numArgs)
{
	MFAddressInfo *pAddrInfo;
	if(MFSockets_GetAddressInfo(pServer, MFStr("%d", port), NULL, &pAddrInfo) == 0)
	{
		char pArgString[2048];
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
		HTTPResponse *pResponse = HTTPResponse::Create(s);
		MFSockets_CloseSocket(s);

		return pResponse;
	}

	return NULL;
}

HTTPResponse *HTTPResponse::Create(MFSocket s)
{
	char revcBuffer[2048];
	int bytes = MFSockets_Recv(s, revcBuffer, sizeof(revcBuffer)-1, 0);
	revcBuffer[bytes] = 0;

	// parse HTTP header
	HTTPResponse *pHeader = NULL;
	uint32 headerBytes = 0;

	char *pNextLine = NULL;
	char *pFormatTag = MFTokeniseLine(revcBuffer, &pNextLine);

	if(MFString_CaseCmpN(pFormatTag, "HTTP", 4))
	{
		// this is not a HTTP response O_o
		return NULL;
	}

	// get the version, response code and details.
	char *pBig = pFormatTag + 5;
	char *pSmall = MFString_Chr(pBig, '.');
	*pSmall++ = 0;

	int httpVersion = MFString_AsciiToInteger(pBig, false, 10)*100 + MFString_AsciiToInteger(pSmall, false, 10);

	char *pResponseCode = MFSeekNextWord(pSmall);
	int responseCode = MFString_AsciiToInteger(pResponseCode, false, 10);

	char *pResponseMessage = MFSeekNextWord(pResponseCode);
	headerBytes += MFString_Length(pResponseMessage) + 1;

	// read header
	char *pKeys[256];
	char *pValues[256];
	int numFields = 0;

	pKeys[numFields] = MFTokeniseLine(pNextLine, &pNextLine);
	while(pKeys[numFields])
	{
		if(!MFString_Length(pKeys[numFields]))
		{
			// end of header
			pHeader = (HTTPResponse*)MFHeap_AllocAndZero(sizeof(HTTPResponse) + sizeof(ResponseField)*numFields + headerBytes);
			pHeader->pValues = (ResponseField*)&pHeader[1];
			char *pStrings = (char*)&pHeader->pValues[numFields];

			pHeader->error = HTTP_UnknownError;

			pHeader->httpVersion = httpVersion;
			pHeader->responseCode = responseCode;

			pHeader->pResponseMessage = MFString_Copy(pStrings, pResponseMessage);
			pStrings += MFString_Length(pHeader->pResponseMessage) + 1;

			for(int a=0; a<numFields; ++a)
			{
				// populate string pair
				ResponseField &field = pHeader->pValues[pHeader->valueCount++];

				field.pField = MFString_Copy(pStrings, pKeys[a]);
				pStrings += MFString_Length(field.pField) + 1;

				field.pValue = MFString_Copy(pStrings, pValues[a]);
				pStrings += MFString_Length(field.pValue) + 1;

				// parse common fields
				if(!MFString_CaseCmp(field.pField, "Content-Length"))
				{
					pHeader->dataSize = MFString_AsciiToInteger(field.pValue, false, 10);

					// allocate data buffer
					pHeader->pData = (char*)MFHeap_Alloc(pHeader->dataSize + 1);
					pHeader->pData[pHeader->dataSize] = 0;
				}
				else if(!MFString_CaseCmp(field.pField, "Host"))
				{
					pHeader->pHost = field.pValue;
				}
				else if(!MFString_CaseCmp(field.pField, "Server"))
				{
					pHeader->pServer = field.pValue;
				}
				else if(!MFString_CaseCmp(field.pField, "Content-Type"))
				{
					pHeader->pContentType = field.pValue;
				}
				else if(!MFString_CaseCmp(field.pField, "Content-Encoding"))
				{
					pHeader->pContentEncoding = field.pValue;
				}
				else if(!MFString_CaseCmp(field.pField, "Transfer-Encoding"))
				{
					pHeader->transferEncoding = HTTP_TE_Unknown;

					for(int b=0; b<HTTP_TE_Max; ++b)
					{
						if(!MFString_CaseCmp(field.pValue, gTransferEncodings[b]))
						{
							pHeader->transferEncoding = (HTTPTransferEncoding)b;
							break;
						}
					}
				}
			}

			break;
		}
		else
		{
			pValues[numFields] = MFString_Chr(pKeys[numFields], ':');
			*pValues[numFields]++ = 0;
			pValues[numFields] = MFSkipWhite(pValues[numFields]);

			int fieldLen = MFString_Length(pKeys[numFields]);
			int valLen = MFString_Length(pValues[numFields]);

			headerBytes += fieldLen + valLen + 2;
			++numFields;
		}

		pKeys[numFields] = MFTokeniseLine(pNextLine, &pNextLine);
	}

	if(!pHeader)
	{
		// there was some sort of error parsing the header...
		return NULL;
	}

	// read packet data
	char *pPacketData = pNextLine;

	switch(pHeader->transferEncoding)
	{
		case HTTP_TE_Identity:
		{
			// allocate and copy packet data into data buffer
			uint32 packetBytes = bytes - (uint32)(pPacketData - revcBuffer);
			uint32 receivedData = MFMin(pHeader->dataSize, packetBytes);
			MFCopyMemory(pHeader->pData, pPacketData, receivedData);

			// while there is still data pending
			while(receivedData < pHeader->dataSize)
			{
				bytes = MFSockets_Recv(s, revcBuffer, MFMin(pHeader->dataSize - receivedData, sizeof(revcBuffer)), 0);
				MFCopyMemory(pHeader->pData + receivedData, revcBuffer, bytes);
				receivedData += bytes;
			}
			break;
		}
		case HTTP_TE_Chunked:
		{
			// receive chunked data
			while(1)
			{
				// read chunk size
				char *pNumBytes = MFTokeniseLine(pPacketData, &pPacketData);
				uint32 numBytes = MFString_AsciiToInteger(pNumBytes, false, 16);

				// check if we've reached the end of the stream
				if(numBytes == 0)
					break;

				// allocate space in the recv buffer
				pHeader->pData = (char*)MFHeap_Realloc(pHeader->pData, pHeader->dataSize + numBytes + 1);
				pHeader->pData[pHeader->dataSize + numBytes] = 0;

				while(numBytes)
				{
					// read numBytes from the stream
					uint32 bytesRemaining = bytes - (uint32)(pPacketData - revcBuffer);
					uint32 bytesToCopy = MFMin(bytesRemaining, numBytes);

					MFCopyMemory(pHeader->pData + pHeader->dataSize, pPacketData, bytesToCopy);

					// adjust pointers
					pHeader->dataSize += bytesToCopy;
					pPacketData += bytesToCopy;
					numBytes -= bytesToCopy;

					if(numBytes)
					{
						bytes = MFSockets_Recv(s, revcBuffer, sizeof(revcBuffer)-1, 0);
						revcBuffer[bytes] = 0;

						pPacketData = revcBuffer;
					}
				}

				// a blank line should follow
				char *pBlank = MFTokeniseLine(pPacketData, &pPacketData);
				if(MFString_Length(pBlank))
				{
					MFDebug_Assert(false, "Should a blank line follow a data packet?!");
					pHeader->error = HTTP_UnknownError;
					break;
				}
			}
			break;
		}
		case HTTP_TE_Compress:
		case HTTP_TE_Deflate:
		case HTTP_TE_GZip:
			pHeader->error = HTTP_UnsupportedTransferEncodingType;
			break;
	}

	pHeader->error = HTTP_OK;
	return pHeader;
}

void HTTPResponse::Destroy(bool bDestroyData)
{
	MFHeap_Free(pData);
	MFHeap_Free(this);
}

const char *HTTPResponse::GetResponseParameter(const char *pKey)
{
	return NULL;
}
