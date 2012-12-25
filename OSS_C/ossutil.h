/*
 * ossutil.h
 *
 *  Created on: 2012-9-22
 *      Author: lch
 */

#ifndef OSSUTIL_H_
#define OSSUTIL_H_
#include <time.h>

#include "HashTable.h"
#include "List.h"

#define OSS_LEN 50
#define OSS_TIME_FORMAT "%a, %d %b %Y %H:%M:%S GMT"

struct Bucket
{
  char* name;
  char* creationDate;
};
struct Owner
{
  char* id;
  char* displayName;
};

typedef struct
{
  char* key;
  char* lastmodified;
  char* etag;
  char* type;
  char* size;
  char* storageclass;
  struct Owner* owner;

} Contents;

typedef struct
{
  char* name;
  char* prefix;
  char* marker;
  char* maxkeys;
  char* nextMarker;
  char* delimiter;
  char* istruncated;
  List contents;          //List<Contents>
  List commonprefixes;    //List<char*>

} ListBucketResult;
void
free_Bucket(struct Bucket*);
void
free_Owner(struct Owner*);
void
free_Contents(Contents*);
void
free_ListBucketResult(ListBucketResult*);
//the allocated memory
typedef char* M_str;

char oss_buf[OSS_LEN];

/*
 * @description:返回当前时间 "%a, %d %b %Y %H:%M:%S GMT"
 * @return:	当前时间字符串
 */
M_str
localtime_gmt();

time_t
StrGmtToLocaltime(const char*);

time_t
GmtToLocaltime(const char* s);
/*
 * @description:身份验证
 * @param:	key	AccessKey
 * @param:	method	http GET、PUT、POST
 * @param:	headers	必须包含"Date","CanonicalizedResource"
 * @param:	resource 获取的资源
 * @return
 */

M_str
oss_authorizate(const char* key, const char* method, struct HashTable* headers,
    const char* resource);

/*
 * @description:得到所有buckets
 * @param:	xml oss返回的xml
 * @param:	owner id NULL
 * @return:	List<struct Bucket*>
 */
List
oss_ListAllMyBucketsResult(const char* xml, struct Owner* owner);
/*
 * @description:list object
 * @param:
 */
ListBucketResult*
oss_ListBucketResult(const char* xml);

M_str
oss_GetBucketAcl(const char* xml);
/*
 * 得到文件大小
 */
size_t
oss_GetObjectSize(const char* httpheader);



#endif /* OSSUTIL_H_ */
