/*
 * oss.h
 *
 *  Created on: 2012-10-7
 *      Author: lch
 */

#ifndef OSS_H_
#define OSS_H_

#include "List.h"

typedef struct{
	char* host;
	char* access_id;
	char* access_key;
} OSS;

typedef OSS* OSSPtr;

typedef enum{RW,RO,PRIVATE} ACL;

OSSPtr new_ossptr();

void free_ossptr(OSSPtr);

/*
 * @description:	初始化
 * @param:	host
 * @param:	id: access id
 * @param:	key:access key
 * @return
 */
OSSPtr oss_init(const char* host, const char* id, const char* key);

List GetService(OSSPtr);

int PutBucket(OSSPtr oss, char* bucket);

int PutBucketACL(OSSPtr oss, char* bucket, ACL a);

char* GetBucket(OSSPtr oss,char* bucket);
#endif /* OSS_H_ */
