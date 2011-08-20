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

HTTPRequest *HTTPRequest::pUpdateList[MAX_ACTIVE_REQUESTS];
int HTTPRequest::numLiveRequests = 0;

void HTTPRequest::UpdateHTTPEvents()
{
	for(int a=0; a<numLiveRequests; ++a)
		pUpdateList[a]->UpdateEvents();
}

HTTPRequest::HTTPRequest()
{
	bFinished = bOldFinished = false;
	status = oldStatus = CS_NotStarted;

	transferThread = NULL;
	mutex = MFThread_CreateMutex("HTTP Mutex");
	MFDebug_Assert(mutex != NULL, "Too many mutexes!");

	reqAlloc = 512;
	pRequestBuffer = (char*)MFHeap_Alloc(reqAlloc);

	pResponse = NULL;

	eventDelegate.clear();
	completeDelegate.clear();

	pUpdateList[numLiveRequests++] = this;
}

HTTPRequest::~HTTPRequest()
{
	MFHeap_Free(pRequestBuffer);

	if(pResponse)
	{
		pResponse->Destroy();
		pResponse = NULL;
	}

	for(int a=0; a<numLiveRequests; ++a)
	{
		if(pUpdateList[a] == this)
		{
			--numLiveRequests;
			for(; a<numLiveRequests; ++a)
				pUpdateList[a] = pUpdateList[a+1];
		}
	}

	MFThread_DestroyMutex(mutex);
	mutex = NULL;
}

void HTTPRequest::UpdateEvents()
{
	Lock();

	if(status != oldStatus && status != CS_NotStarted)
	{
		if(eventDelegate)
			eventDelegate(status);
	}

	if(bFinished && !bOldFinished)
	{
		if(completeDelegate)
			completeDelegate(status);
	}

	oldStatus = status;
	bOldFinished = bFinished;

	Unlock();
}

void HTTPRequest::Get(const char *_pServer, int _port, const char *_pResourcePath)
{
	if(pResponse)
	{
		pResponse->Destroy();
		pResponse = NULL;
	}

	pServer = _pServer;
	pResourcePath = _pResourcePath;
	port = _port;

	// *** TODO *** 
	// test to see if there is enough buffer allocated...

	// generate request
	reqLen = sprintf(pRequestBuffer, "GET %s HTTP/1.1\nHost: %s:%d\nUser-Agent: WarriorLords Client/1.0\nContent-Type: application/x-www-form-urlencoded\n\n\n", pResourcePath, pServer, port);

	// begin async request
	CreateConnection();
}

void HTTPRequest::Post(const char *_pServer, int _port, const char *_pResourcePath, MFFileHTTPRequestArg *pArgs, int numArgs)
{
	Reset();

	if(pResponse)
	{
		pResponse->Destroy();
		pResponse = NULL;
	}

	pServer = _pServer;
	pResourcePath = _pResourcePath;
	port = _port;

	// generate the request template
	int sizeOffset = sprintf(pRequestBuffer, "POST %s HTTP/1.1\nHost: %s:%d\nUser-Agent: WarriorLords Client/1.0\nContent-Type: application/x-www-form-urlencoded\nContent-Length: ", pResourcePath, pServer, port);
	int dataOffset = sizeOffset + sprintf(pRequestBuffer + sizeOffset, "        \n\n");
	reqLen = dataOffset;

	// append args
	if(pArgs)
	{
		// build the string from the supplied args
		for(int a=0; a<numArgs; ++a)
		{
			// count the num chars to write
			int numChars = MFString_URLEncode(NULL, pArgs[a].pArg);
			if(pArgs[a].type == MFFileHTTPRequestArg::AT_String)
				numChars += MFString_URLEncode(NULL, pArgs[a].pValue);

			// make sure we have enough space in the dest buffer
			while(reqLen > reqAlloc - numChars - 32)
			{
				reqAlloc *= 2;
				pRequestBuffer = (char*)MFHeap_Realloc(pRequestBuffer, reqAlloc);
			}

			// separate args with an '&'
			if(a > 0)
				pRequestBuffer[reqLen++] = '&';

			// append the arg
			reqLen += MFString_URLEncode(pRequestBuffer + reqLen, pArgs[a].pArg);
			pRequestBuffer[reqLen++] = '=';

			switch(pArgs[a].type)
			{
				case MFFileHTTPRequestArg::AT_Int:
					reqLen += sprintf(pRequestBuffer + reqLen, "%d", pArgs[a].iValue);
					break;
				case MFFileHTTPRequestArg::AT_Float:
					reqLen += sprintf(pRequestBuffer + reqLen, "%g", pArgs[a].fValue);
					break;
				case MFFileHTTPRequestArg::AT_String:
					reqLen += MFString_URLEncode(pRequestBuffer + reqLen, pArgs[a].pValue);
					break;
			}
		}
	}

	// write size to the buffer
	sizeOffset += sprintf(pRequestBuffer + sizeOffset, "%d", reqLen - dataOffset);
	pRequestBuffer[sizeOffset] = ' ';

	// begin async request
	CreateConnection();
}

void HTTPRequest::CreateConnection()
{
	SetStatus(CS_ResolvingHost, false);

	MFThread_LockMutex(mutex);
	transferThread = MFThread_CreateThread("HTTP Thread", RequestThread, this);
	MFDebug_Assert(transferThread != NULL, "Too many threads!");
	MFThread_ReleaseMutex(mutex);
}

int HTTPRequest::RequestThread(void *_pRequest)
{
	HTTPRequest *pThis = (HTTPRequest*)_pRequest;

	pThis->HandleRequest();

	// clean up the thread
	MFDebug_Assert(pThis->transferThread != NULL, "Errr...");

	MFThread thread = pThis->transferThread;
	MFThread_LockMutex(pThis->mutex);
	pThis->transferThread = NULL;
	MFThread_ReleaseMutex(pThis->mutex);
	MFThread_DestroyThread(thread);
	return 0;
}

void HTTPRequest::HandleRequest()
{
	MFAddressInfo *pAddrInfo;
	if(MFSockets_GetAddressInfo(pServer, MFStr("%d", port), NULL, &pAddrInfo) != 0)
	{
		SetStatus(CS_CouldntResolveHost, false);
		return;
	}

	MFSocket s = MFSockets_CreateSocket(pAddrInfo->pAddress->family, MFSockType_Stream, MFProtocol_TCP);

	SetStatus(CS_WaitingForHost, false);

	int r = MFSockets_Connect(s, *pAddrInfo->pAddress);
	if(r != 0)
	{
		MFSockets_CloseSocket(s);

		SetStatus(CS_CouldntConnect, false);
		return;
	}

	SetStatus(CS_Pending, false);

	MFSockets_Send(s, pRequestBuffer, reqLen, 0);
	pResponse = HTTPResponse::Create(s);
	MFSockets_CloseSocket(s);

	SetStatus(CS_Succeeded, true);
}

void HTTPRequest::SetStatus(Status _status, bool _bFinished)
{
	Lock();
	status = _status;
	bFinished = _bFinished;
	Unlock();
}

bool HTTPRequest::IsFinished()
{
	Lock();
	bool bF = bFinished;
	Unlock();
	return bF;
}

bool HTTPRequest::RequestPending()
{
	bool bPending = false;
	Lock();
	bPending = status != CS_NotStarted && !bFinished;
	Unlock();
	return bPending;
}

HTTPRequest::Status HTTPRequest::GetStatus()
{
	Lock();
	Status s = status;
	Unlock();
	return s;
}

void HTTPRequest::Reset()
{
	Lock();
	status = CS_NotStarted;
	bFinished = false;
	Unlock();
}

HTTPResponse *HTTPRequest::GetResponse()
{
	Lock();
	HTTPResponse *pR = (status == CS_Succeeded ? pResponse : NULL);
	Unlock();
	return pR;
}

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
		int len = sprintf(buffer, "POST %s HTTP/1.1\nHost: %s:%d\nUser-Agent: WarriorLords Client/1.0\nContent-Type: application/x-www-form-urlencoded\nContent-Length: %d\n\n%s", pResourcePath, pServer, port, MFString_Length(pArgString), pArgString);

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
//	MFDebug_Log(0, "---------------------- Begin Transfer ----------------------");

	char revcBuffer[2048];
	int bytes = MFSockets_Recv(s, revcBuffer, sizeof(revcBuffer)-1, 0);
	revcBuffer[bytes] = 0;

//	MFDebug_Log(0, MFStr("Recv: %d", bytes));

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
				bytes = MFSockets_Recv(s, revcBuffer, MFMin(pHeader->dataSize - receivedData, (uint32)sizeof(revcBuffer)), 0);
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
				MFDebug_Assert(MFString_Length(pNumBytes) > 0, "Expected: Chunk size!");

				uint32 numBytes = MFString_AsciiToInteger(pNumBytes, false, 16);

//				MFDebug_Log(0, MFStr("ChunkBytes: %d - '%s'", numBytes, pNumBytes));

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

//					MFDebug_Log(0, MFStr("CopyBytes: %d", bytesToCopy));

					MFCopyMemory(pHeader->pData + pHeader->dataSize, pPacketData, bytesToCopy);

					// adjust pointers
					pHeader->dataSize += bytesToCopy;
					pPacketData += bytesToCopy;
					numBytes -= bytesToCopy;

					if(numBytes)
					{
						bytes = MFSockets_Recv(s, revcBuffer, sizeof(revcBuffer)-1, 0);
						revcBuffer[bytes] = 0;

//						MFDebug_Log(0, MFStr("Recv: %d", bytes));

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

//				MFDebug_Log(0, MFStr("RemainingBuffer: %d\n%s", MFString_Length(pPacketData), pPacketData));

				// check that the server has actually sent the next chunk
				if(*pPacketData == 0)
				{
					bytes = MFSockets_Recv(s, revcBuffer, sizeof(revcBuffer)-1, 0);
					revcBuffer[bytes] = 0;

//					MFDebug_Log(0, MFStr("Recv: %d", bytes));

					pPacketData = revcBuffer;
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

//	MFDebug_Log(0, MFStr("Done: %d", pHeader->dataSize));

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
