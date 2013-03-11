#if !defined(_HTTP_H)
#define _HTTP_H

#include "Fuji/MFThread.h"
#include "Fuji/MFSockets.h"
#include "Fuji/FileSystem/MFFileSystemHTTP.h"

enum HTTPError
{
	HTTP_UnknownError = -1,

	HTTP_OK = 0,
	HTTP_UnsupportedTransferEncodingType,

	HTTP_MaxError
};

enum HTTPTransferEncoding
{
	HTTP_TE_Unknown = -1,

	HTTP_TE_Identity = 0,
	HTTP_TE_Chunked,
	HTTP_TE_Compress,
	HTTP_TE_Deflate,
	HTTP_TE_GZip,

	HTTP_TE_Max
};

class HTTPResponse
{
public:
	static HTTPResponse *Create(MFSocket s);
	void Destroy(bool bDestroyData = true);

	const char *GetResponseParameter(const char *pKey);

	uint32 GetDataSize() { return dataSize; }
	char *GetData() { return pData; }

private:
	struct ResponseField
	{
		const char *pField;
		const char *pValue;
	};

	HTTPError error;

	int httpVersion;

	int responseCode;
	const char *pResponseMessage;

	const char *pHost;
	const char *pServer;

	HTTPTransferEncoding transferEncoding;

	const char *pContentType;
	const char *pContentEncoding;

	ResponseField *pValues;
	int valueCount;

	uint32 dataSize;
	char *pData;
};

class HTTPRequest
{
public:
	enum Status
	{
		CS_ResolvingHost = -4,
		CS_WaitingForHost = -3,
		CS_Pending = -2,
		CS_NotStarted = -1,

		CS_Succeeded = 0,
		CS_CouldntResolveHost = 1,
		CS_CouldntConnect = 2,
		CS_ConnectionLost = 3,
		CS_HTTPError = 4
	};

	typedef FastDelegate1<Status> HTTPEventDelegate;

	HTTPRequest();
	~HTTPRequest();

	static void UpdateHTTPEvents();

	void Get(const char *pServer, int port, const char *pResourcePath);
	void Post(const char *pServer, int port, const char *pResourcePath, MFFileHTTPRequestArg *pArgs = NULL, int numArgs = 0);

	void Lock() { if(mutex) MFThread_LockMutex(mutex); }
	void Unlock() { if(mutex) MFThread_ReleaseMutex(mutex); }

	bool IsFinished();
	bool RequestPending();
	Status GetStatus();
	void Reset();

	void SetEventDelegate(HTTPEventDelegate handler) { eventDelegate = handler; }
	void SetCompleteDelegate(HTTPEventDelegate handler) { completeDelegate = handler; }

	HTTPResponse *GetResponse();

protected:
	void CreateConnection();
	void SetStatus(Status status, bool bFinished = false);

	MFThread transferThread;
	MFMutex mutex;

	HTTPResponse *pResponse;

	const char *pServer;
	int port;
	const char *pResourcePath;

	bool bFinished;
	Status status;
	bool bOldFinished;
	Status oldStatus;

	char *pRequestBuffer;
	int reqLen, reqAlloc;

	HTTPEventDelegate eventDelegate;
	HTTPEventDelegate completeDelegate;

private:
	static int RequestThread(void *pRequest);
	void HandleRequest();
	void UpdateEvents();

	static const int MAX_ACTIVE_REQUESTS = 1024;
	static HTTPRequest *pUpdateList[MAX_ACTIVE_REQUESTS];
	static int numLiveRequests;
};

HTTPResponse *HTTP_Get(const char *pServer, int port, const char *pResourcePath);
HTTPResponse *HTTP_Post(const char *pServer, int port, const char *pResourcePath, MFFileHTTPRequestArg *pArgs = NULL, int numArgs = 0);

#endif
