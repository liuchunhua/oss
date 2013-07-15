/*
 * oss.h
 *
 *  Created on: 2012-10-7
 *      Author: lch
 */

#ifndef OSS_H_
#define OSS_H_

#include "List.h"
#include "ossutil.h"
#include "oss_config.h"

#include <stdio.h>




typedef struct OSSObject OSSObject;
struct OSSObject{
  char* name;
  char* minetype;
  char* etag;
  time_t mtime;
  size_t size;
};
typedef struct{
        char *memory;
        size_t size;
} MemoryBlock;




OSSPtr new_ossptr();

void free_ossptr(OSSPtr);

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

ListBucketResult* ListObject(OSSPtr oss, const char* bucket,const char* prefix,unsigned int maxkeys,const char* marker,const char* delimiter);

int PutObject(OSSPtr oss,char* bucket,char* objectname,char* file,struct HashTable* table);
/*
 * @return 文件大小
 */
size_t GetObject(OSSPtr oss,char* object,char* desfile,struct HashTable* table,unsigned short redownnload);
size_t GetObjectIntoMemory(OSSPtr oss,const char* object,char* buf,size_t size, off_t offset,struct HashTable* table);

OSSObject* HeadObject(OSSPtr oss,const char* object);

int CopyObject(OSSPtr oss,char* source,char* des);

int DeleteObject(OSSPtr oss,char* object);

#endif /* OSS_H_ */
