#ifndef OSS_CONFIG_H_
#define OSS_CONFIG_H_

//阿里云访问参数
typedef struct
{
	char *host;//地址
	char *access_id;
	char *access_key;
	char *proxy;//代理
	char *bucket;
}OSS;

typedef OSS * OSSPtr;

//OSS操作
typedef struct
{
	OSSPtr (*init)(const char *, const char *, const char*);//host,id,key
	void (*destroy)(OSSPtr);
}OSSOperation;

//OSS访问权限
typedef enum{RW,RO,PRIVATE} ACL;

//OSS操作类
extern OSSOperation OSSClass;

#endif
