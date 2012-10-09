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


static struct Owner* getOwner(xmlNodePtr node) {
	if (!strcmp((char*) node->name, "Owner")) {
		xmlNodePtr cur = node->children;
		struct Owner* owner = (struct Owner*) malloc(sizeof(struct Owner));
		assert(owner!=NULL);
		while (cur) {
			if (cur->type == XML_ELEMENT_NODE && !strcmp((char*)cur->name, "ID")) {
				owner->id = (char*)xmlNodeGetContent(cur);
			}
			if (cur->type == XML_ELEMENT_NODE && !strcmp((char*)cur->name, "DisplayName")) {
				owner->displayName = (char*)xmlNodeGetContent(cur);
			}
			cur = cur->next;
		}
		return owner;
	}
	return NULL ;
}
static struct Bucket* getBucket(xmlNodePtr node){
	if(!strcmp((char*)node->name,"Bucket")){
		xmlNodePtr cur = node->children;
		struct Bucket* bucket = (struct Bucket*)malloc(sizeof(struct Bucket));
		assert(bucket!=NULL);
		while(cur){
			if(cur->type==XML_ELEMENT_NODE&&!strcmp((char*)cur->name,"Name")){
				bucket->name = (char*)xmlNodeGetContent(cur);
			}
			if(cur->type==XML_ELEMENT_NODE&&!strcmp((char*)cur->name,"CreationDate")){
				bucket->creationDate = (char*)xmlNodeGetContent(cur);
			}
			cur = cur->next;
		}
		return bucket;
	}
	return NULL;
}
static List getListBucket(xmlNodePtr node){
	if(!strcmp((char*)node->name,"Buckets")){
		xmlNodePtr cur = node->children;
		List list = listInit();
		while(cur){
			if(cur->type==XML_ELEMENT_NODE&&!strcmp((char*)cur->name,"Bucket")){
				struct Bucket* bucket = getBucket(cur);
				if(bucket) listAdd(list,bucket);
			}
			cur = cur->next;
		}
		return list;
	}
	return NULL;
}

//按字母顺序升序排列
static void sort_list_asc(List ls){
	List last = ls;
	while(ls->next!=last){
		List node = ls->next;
		List next = node->next;
		while(next!=last){
			struct pair* a = (struct pair*)node->ptr;
			struct pair* b = (struct pair*)next->ptr;
			char* a_key = (char*)a->key;
			char* b_key = (char*)b->key;
			if(strcoll(a_key,b_key)>0){
				node->prev->next = node->next;
				node->next = next->next;
				next->prev = node->prev;
				node->prev = next;
				next->next = node;
				next = node->next;
			}
			if(strcoll(a_key,b_key)==0){
				int size = strlen((char*)a->value)+strlen((char*)b->value)+2;
				char* new_string = malloc(size);
				strcat(new_string,(char*)a->value);
				strcat(new_string,",");
				strcat(new_string,(char*)b->value);
				free(a->value);
				a->value = (void*)new_string;
				free(b->key);
				free(b->value);
				free(b);
				listDel(next);
				next = node->next;
			}else{
				node = next;
				next = next->next;
			}
		}
		last = last->prev;
	}
}


M_str oss_authorizate(const char* key, const char* method,
		struct HashTable* headers, const char* resource) {
	char* date = (char*) hash_table_get(headers, "Date");
	char* content_md5 = (char*) hash_table_get(headers, "Content-Md5");
	char* content_type = (char*) hash_table_get(headers, "Content-Type");
	assert(resource!=NULL);
	assert(date!=NULL);
	char buf[200] = {};
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
	List list = hash_table_get_key_list(headers);
	sort_list_asc(list);
	List node;
	for_each(node,list){
		struct pair* p = (struct pair*)node->ptr;
		if(strcasestr((char*)(p->key),"x-oss-")!=NULL){
			strcat(buf,(char*)(p->key));
			strcat(buf,":");
			strcat(buf,(char*)(p->value));
			strcat(buf,"\n");
		}
	}
	listFree(list);
	strcat(buf, resource);
	fprintf(stderr,"%s\n",buf);
	return hmac_base64(buf, strlen(buf), key, strlen(key));
}

List oss_ListAllMyBucketsResult(const char* xml,struct Owner* owner) {
	xmlDocPtr doc;
	xmlNodePtr root;
	xmlNodePtr cur;
	List list;
	doc = xmlReadMemory(xml, strlen(xml), "noname.xml", NULL, 0);
	assert(doc!=NULL);
	root = xmlDocGetRootElement(doc);
	cur = root->children;
	while (cur != NULL ) {
		if (!strcmp((char*) cur->name, "Owner")&&owner!=NULL) {
			struct Owner* temp = getOwner(cur);
			owner->id = temp->id;
			owner->displayName = temp->displayName;
			free(temp);
		}
		if (strcmp((char*)cur->name, "Buckets") == 0) {
			list = getListBucket(cur);
		}
		cur = cur->next;
	}
	xmlFreeDoc(doc);
	return list;
}
