/*
 OSS验证
 author:lch
 */
#ifndef _AUTHEN.H
#define _AUTHEN.H

#include "HashTable.h"

//构建 CanonicalizedOSSHeaders 的方法
char *
canonicalizedOSSHeaders(HashTable *headers);

//构建 CanonicalizedResource 的方法:
char *
canonicalizedResource(const char * resource);

/*
 * base64(hmac-sha1(VERB + "\n"
 + CONTENT-MD5 + "\n"
 + CONTENT-TYPE + "\n"
 + DATE + "\n"
 + CanonicalizedOSSHeaders
 + CanonicalizedResource))

 */
char *
oss_authorizate(const char *key, const char *method, const char *headers,
    const char* resource);

#endif
