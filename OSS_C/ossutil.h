/*
 *ossutil.h
 *
 * Created on: 2012-9-22
 *     Author: lch
 */

#ifndef OSSUTIL_H_
#define OSSUTIL_H_
#include <time.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include "HashTable.h"
#include "List.h"
#include "service.h"
#include "oss_config.h"

#define OSS_LEN 50

#define OSS_TIME_FORMAT "%a, %d %b %Y %H:%M:%S GMT"

#define GetNodeValue(node,structure,member)  if(node->type==XML_ELEMENT_NODE&&!strcasecmp((char*)node->name,#member)){ \
                                                                                                                        structure->member = (char*)xmlNodeGetContent(node);}
void
free_Bucket(struct Bucket*);

void
free_Owner(struct Owner*);

void
free_Contents(Contents*);

//the allocated memory
typedef char *M_str;

char oss_buf[OSS_LEN];

/*
 *@description:返回当前时间 "%a, %d %b %Y %H:%M:%S GMT"
 *@return:	当前时间字符串
 */
char *localtime_gmt();

time_t StrGmtToLocaltime(const char*);

time_t GmtToLocaltime(const char *s);

char *hmac_base64(const char *string, size_t len, const char *key, int key_len);


/*
 *@description:得到所有buckets
 *@param:	xml oss返回的xml
 *@param:	owner id NULL
 *@return:	List<struct Bucket*>
 */
List oss_ListAllMyBucketsResult(const char *xml, struct Owner *owner);
/*
 *@description:list object
 *@param:
 */
ListBucketResult *oss_ListBucketResult(const char *xml);

M_str oss_GetBucketAcl(const char *xml);
/*
 *得到文件大小
 */
size_t oss_GetObjectSize(const char *httpheader);

char *http_request(OSS *oss, const char *url, const char *method);

#endif
/*OSSUTIL_H_ */
