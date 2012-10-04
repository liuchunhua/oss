/*
 * ossutil.c
 *
 *  Created on: 2012-9-22
 *      Author: lch
 */

#include "ossutil.h"
#include "base64.h"
#include "HashTable.h"
#include "List.h"

#include <time.h>
#include <string.h>
#include <openssl/hmac.h>
#include <assert.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

M_str localtime_gmt() {
	time_t cur_time;
	memset(oss_buf, 0x0, sizeof(oss_buf));
	time(&cur_time);
	struct tm* tm_time = gmtime(&cur_time);
	strftime(oss_buf, OSS_LEN, OSS_TIME_FORMAT, tm_time);
	return strdup(oss_buf);
}

/*
 * @description: 加密，编码
 * @param:	string 需要加密的字符串
 * @param:	len string的长度
 * @param:	key 密钥
 * @param:	key_len 密钥长度
 * @return:	加密后字符串
 */

static M_str hmac_base64(const char* string, int len, const char* key,
		int key_len) {
//	HMAC_CTX ctx;
	unsigned int out_len = 0;
	memset(oss_buf, 0x0, OSS_LEN);
//	HMAC_CTX_init(&ctx);
//	HMAC_Init(&ctx,key,key_len,EVP_sha1());
//	HMAC_Update(&ctx,string,len);
//	HMAC_Final(&ctx,oss_buf,&out_len);
//	HMAC_CTX_cleanup(&ctx);
	HMAC(EVP_sha1(), key, key_len, string, len, oss_buf, &out_len);
	return strdup(base64_encode(oss_buf, out_len));

}

M_str oss_authorizate(const char* key, const char* method,
		struct HashTable* headers, const char* resource) {
	char* date = (char*) hash_table_get(headers, "Date");
	char* content_md5 = (char*) hash_table_get(headers, "Content-Md5");
	char* content_type = (char*) hash_table_get(headers, "Content-Type");
	assert(resource!=NULL);
	assert(date!=NULL);
	char buf[200];
	memset(buf, 0x0, 200);
	strcat(buf, method); //method
	strcat(buf, "\n");
	if (content_md5 != NULL )
		strcat(buf, content_md5);
	strcat(buf, "\n"); //content-md5
	if (content_type != NULL )
		strcat(buf, content_type);
	strcat(buf, "\n");
	strcat(buf, date);
	strcat(buf, "\n");
	strcat(buf, resource);
	return hmac_base64(buf, strlen(buf), key, strlen(key));
}

List oss_ListAllMyBucketsResult(const char* xml) {
	xmlDocPtr doc;
	doc = xmlReadMemory(xml, strlen(xml), "noname.xml", NULL, 0);
	assert(doc!=NULL);

	xmlFreeDoc(doc);
}
