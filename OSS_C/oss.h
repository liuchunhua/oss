/*
 * oss.h
 *
 *  Created on: 2012-10-7
 *      Author: lch
 */

#ifndef OSS_H_
#define OSS_H_

#include <stdlib.h>

#include "oss_config.h"
#include "service.h"



typedef struct OSSObject OSSObject;

struct OSSObject
{
    char* name;
    char* minetype;
    char* etag;
    time_t mtime;
    size_t size;
};

typedef struct
{
    OSSObject *(*init)();
    void (*destroy)(OSSObject *);
}OSSObjectOperation;

typedef struct
{
    char *memory;
    size_t size;
} MemoryBlock;

extern OSSObjectOperation OSSObjectClass;

OSSPtr new_ossptr();

void free_ossptr(OSSPtr);

//初始化
OSSObject *ossobject_init();

//释放
void free_ossobject(OSSObject*);

/*
 * @description:	初始化
 * @param:	host
 * @param:	id: access id
 * @param:	key:access key
 * @return
 */
OSSPtr oss_init(const char* host, const char* id, const char* key);

/*
 * @description:    得到所有的bucket
 * @return List<struct Bucket*>
 */
List GetService(OSSPtr);

int PutBucket(OSSPtr oss, char* bucket);

int PutBucketACL(OSSPtr oss, char* bucket, ACL a);

ACL GetBucketACL(OSSPtr oss, char* bucket);

int DeleteBucket(OSSPtr oss, char* bucket);

ListBucketResult* ListObject(OSSPtr oss, const char* bucket, const char* prefix,
        unsigned int maxkeys, const char* marker, const char* delimiter);

int PutObject(OSSPtr oss, const char* bucket, const char* objectname, const char* file);
/*
 * @return 文件大小
 */
size_t GetObject(OSSPtr oss, const char *bucket, const char* object, const char *desfile);

OSSObject* HeadObject(OSSPtr oss, const char *bucket, const char *object);

int CopyObject(OSSPtr oss, const char *bucket, const char *source, char *des);

int DeleteObject(OSSPtr oss, const char *bucket, const char *object);

#endif /* OSS_H_ */
