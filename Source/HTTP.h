#if !defined(_HTTP_H)
#define _HTTP_H

#include "FileSystem/MFFileSystemHTTP.h"

const char *HTTP_Get(const char *pServer, int port, const char *pResourcePath);
const char *HTTP_Post(const char *pServer, int port, const char *pResourcePath, MFFileHTTPRequestArg *pArgs = NULL, int numArgs = 0);

#endif
