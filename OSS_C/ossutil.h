/*
 * ossutil.h
 *
 *  Created on: 2012-9-22
 *      Author: lch
 */

#ifndef OSSUTIL_H_
#define OSSUTIL_H_

#include "HashTable.h"
#include "List.h"

#define OSS_LEN 50
#define OSS_TIME_FORMAT "%a, %d %b %Y %H:%M:%S GMT"


struct Bucket{
	char* name;
	char* creationDate;
};
struct Owner{
	char* id;
	char* displayName;
};

//the allocated memory
typedef char* M_str;

char oss_buf[OSS_LEN];


/*
 * @description:返回当前时间 "%a, %d %b %Y %H:%M:%S GMT"
 * @return:	当前时间字符串
 */
M_str localtime_gmt();



/*
 * @description:身份验证
 * @param:	key	AccessKey
 * @param:	method	http GET、PUT、POST
 * @param:	headers	必须包含"Date","CanonicalizedResource"
 * @param:	resource 获取的资源
 * @return
 */

M_str oss_authorizate(const char* key,const char* method, struct HashTable* headers,const char* resource);

/*
 * @description:得到所有buckets
 * @param:	owner id NULL
 * @param:	List<struct Bucket*>
 * @return:	List<struct Bucket*>
 */
List oss_ListAllMyBucketsResult(const char* xml,struct Owner* owner);

#endif /* OSSUTIL_H_ */
