#ifndef OSS_CONFIG_H_
#define OSS_CONFIG_H_

typedef struct
{
	char *host;
	char *access_id;
	char *access_key;
	char *proxy;
}OSS;

typedef OSS * OSSPtr;


typedef struct
{
	OSSPtr (*init)(const char *, const char *, const char*);//host,id,key
	void (*destroy)(OSSPtr);
}OSSOperation;

typedef enum{RW,RO,PRIVATE} ACL;

extern OSSOperation OSSClass;

#endif
