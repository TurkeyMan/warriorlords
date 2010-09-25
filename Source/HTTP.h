#if !defined(_HTTP_H)
#define _HTTP_H

#include "MFSockets.h"
#include "FileSystem/MFFileSystemHTTP.h"

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

HTTPResponse *HTTP_Get(const char *pServer, int port, const char *pResourcePath);
HTTPResponse *HTTP_Post(const char *pServer, int port, const char *pResourcePath, MFFileHTTPRequestArg *pArgs = NULL, int numArgs = 0);

#endif
