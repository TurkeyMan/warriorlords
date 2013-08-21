#include "Warlords.h"
#include "HTTP.h"

#include "Fuji/MFSockets.h"
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
	{
		if(pUpdateList[a]->bDestroy && !pUpdateList[a]->transferThread)
			delete pUpdateList[a--];
	}

	for(int a=0; a<numLiveRequests; ++a)
		pUpdateList[a]->UpdateEvents();
}

HTTPRequest::HTTPRequest(HTTPEventDelegate completeDelegate, HTTPEventDelegate eventDelegate)
: completeDelegate(completeDelegate)
, eventDelegate(eventDelegate)
{
	bFinished = bOldFinished = false;
	status = oldStatus = CS_NotStarted;

	transferThread = NULL;
	mutex = MFThread_CreateMutex("HTTP Mutex");
	MFDebug_Assert(mutex != NULL, "Too many mutexes!");

	pResponse = NULL;

	bDestroy = false;

//	eventDelegate.clear();
//	completeDelegate.clear();

	pUpdateList[numLiveRequests++] = this;
}

HTTPRequest::~HTTPRequest()
{
	if(pResponse)
	{
		delete pResponse;
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
			eventDelegate(this);
	}

	if(bFinished && !bOldFinished)
	{
		if(completeDelegate)
			completeDelegate(this);
	}

	oldStatus = status;
	bOldFinished = bFinished;

	Unlock();
}

void HTTPRequest::Get(const char *pServer, int port, const char *pResourcePath)
{
	if(pResponse)
	{
		delete pResponse;
		pResponse = NULL;
	}

	server = pServer;
	resourcePath = pResourcePath;
	this->port = port;

	// generate request
	request.Sprintf("GET %s HTTP/1.1\r\nHost: %s:%d\r\nUser-Agent: WarriorLords Client/1.0\r\n\r\n", pResourcePath, pServer, port);

	// begin async request
	CreateConnection();
}

void HTTPRequest::Post(const char *pServer, int port, const char *pResourcePath, MFFileHTTPRequestArg *pArgs, int numArgs)
{
	Reset();

	if(pResponse)
	{
		delete pResponse;
		pResponse = NULL;
	}

	server = pServer;
	resourcePath = pResourcePath;
	this->port = port;

	// append args
	MFString content;
	if(pArgs)
	{
		char arg[128];
		char val[1024];

		// build the string from the supplied args
		for(int a=0; a<numArgs; ++a)
		{
			MFString_URLEncode(arg, pArgs[a].pArg);

			switch(pArgs[a].type)
			{
				case MFFileHTTPRequestArg::AT_Int:
					content += MFString::Format(a > 0 ? "&%s=%d" : "%s=%d", arg, pArgs[a].iValue);
					break;
				case MFFileHTTPRequestArg::AT_Float:
					content += MFString::Format(a > 0 ? "&%s=%g" : "%s=%g", arg, pArgs[a].fValue);
					break;
				case MFFileHTTPRequestArg::AT_String:
					MFString_URLEncode(val, pArgs[a].pValue);
					content += MFString::Format(a > 0 ? "&%s=%s" : "%s=%s", arg, val);
					break;
			}
		}
	}

	// generate the request template
	request.Sprintf("POST %s HTTP/1.1\r\nHost: %s:%d\r\nUser-Agent: WarriorLords Client/1.0\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: %d\r\n\r\n%s", pResourcePath, pServer, port, content.NumBytes(), content.CStr());

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
	if(MFSockets_GetAddressInfo(server.CStr(), MFStr("%d", port), NULL, &pAddrInfo) != 0)
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

	MFSockets_Send(s, request.CStr(), request.NumBytes(), 0);
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

HTTPResponse *HTTPResponse::Create(MFSocket s)
{
//	MFDebug_Log(0, "---------------------- Begin Transfer ----------------------");

	char revcBuffer[2048];
	int bytes = MFSockets_Recv(s, revcBuffer, sizeof(revcBuffer)-1, 0);
	revcBuffer[bytes] = 0;

//	MFDebug_Log(0, MFStr("Recv: %d", bytes));

	// parse HTTP header
	HTTPResponse *pHeader = new HTTPResponse;

	pHeader->error = HTTP_UnknownError;
	pHeader->transferEncoding = HTTP_TE_Identity;

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

	pHeader->httpVersion = MFString_AsciiToInteger(pBig, false, 10)*100 + MFString_AsciiToInteger(pSmall, false, 10);

	char *pResponseCode = MFSeekNextWord(pSmall);
	pHeader->responseCode = MFString_AsciiToInteger(pResponseCode, false, 10);

	pHeader->responseMessage = MFSeekNextWord(pResponseCode);

	// read header
	const char *pLine = MFTokeniseLine(pNextLine, &pNextLine);
	while(pLine)
	{
		if(!MFString_Length(pLine))
			break;
		else
		{
			ResponseField &field = pHeader->values.push();

			const char *pColon = MFString_Chr(pLine, ':');
			field.field = MFString(pLine, pColon - pLine);

			pColon = MFSkipWhite(++pColon);
			field.value = pColon;
		}

		pLine = MFTokeniseLine(pNextLine, &pNextLine);
	}

	for(size_t a=0; a<pHeader->values.size(); ++a)
	{
		// populate string pair
		ResponseField &field = pHeader->values[a];

		// parse common fields
		if(field.field.EqualsInsensitive("Content-Length"))
		{
			pHeader->dataSize = field.value.ToInt();

			// allocate data buffer
			pHeader->pData = (char*)MFHeap_Alloc(pHeader->dataSize + 1);
			pHeader->pData[pHeader->dataSize] = 0;
		}
		else if(field.field.EqualsInsensitive("Host"))
		{
			pHeader->host = field.value;
		}
		else if(field.field.EqualsInsensitive("Server"))
		{
			pHeader->server = field.value;
		}
		else if(field.field.EqualsInsensitive("Content-Type"))
		{
			pHeader->contentType = field.value;
		}
		else if(field.field.EqualsInsensitive("Content-Encoding"))
		{
			pHeader->contentEncoding = field.value;
		}
		else if(field.field.EqualsInsensitive("Transfer-Encoding"))
		{
			pHeader->transferEncoding = HTTP_TE_Unknown;

			for(int b=0; b<HTTP_TE_Max; ++b)
			{
				if(field.value.EqualsInsensitive(gTransferEncodings[b]))
				{
					pHeader->transferEncoding = (HTTPTransferEncoding)b;
					break;
				}
			}
		}
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

//				MFDebug_Log(0, MFStr("RemainingBuffer: %d\r\n%s", MFString_Length(pPacketData), pPacketData));

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

HTTPResponse::~HTTPResponse()
{
	MFHeap_Free(pData);
}

const char *HTTPResponse::GetResponseParameter(const char *pKey)
{
	return NULL;
}
