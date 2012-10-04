/*
 * test.c
 *
 *  Created on: 2012-9-19
 *      Author: lch
 */

#include "base64.h"
#include "ossutil.h"
#include "HashTable.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <curl/curl.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <assert.h>

static const char* buckets =
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\
<ListAllMyBucketsResult>\
  <Owner>\
    <ID>1721545645056998</ID>\
    <DisplayName>1721545645056998</DisplayName>\
  </Owner>\
  <Buckets>\
    <Bucket>\
      <Name>welcome2myspace</Name>\
      <CreationDate>2012-08-22T14:32:25.000Z</CreationDate>\
    </Bucket>\
    <Bucket>\
      <Name>liuchunhua</Name>\
      <CreationDate>2012-08-22T14:33:34.000Z</CreationDate>\
    </Bucket>\
  </Buckets>\
</ListAllMyBucketsResult>";
void test_base64_encode(const char*);
void test_localtime_gmt();
void test_hmac_base64();
void test_curl();

void test_bucket_xmls();

int main() {
	//test_base64_encode("��䦦�µm�T*Jl4�");
//	test_localtime_gmt();
	//test_hmac_base64();
	//test_curl();
	test_bucket_xmls();
	return 0;
}

void test_base64_encode(const char* string) {
	printf("%s\n", base64_encode(string, strlen(string)));
}

void test_localtime_gmt() {
	printf("%s\n", localtime_gmt());
}

void test_hmac_base64() {
//	char* origin = "good night， liuchunhua";
//	char* key = "helloword";
//	M_str s =  hmac_base64(origin,strlen(origin),key,strlen(key));
//	printf("%s\n",s);
//	free((void*)s);
}

static const char* header(char* buf, char* key, char* value) {
	memset(buf, 0x0, 200);
	strcat(buf, key);
	strcat(buf, ":");
	strcat(buf, value);
	return buf;
}

/*
 <?xml version="1.0" encoding="UTF-8"?>
 <ListAllMyBucketsResult>
 <Owner>
 <ID>1721545645056998</ID>
 <DisplayName>1721545645056998</DisplayName>
 </Owner>
 <Buckets>
 <Bucket>
 <Name>welcome2myspace</Name>
 <CreationDate>2012-08-22T14:32:25.000Z</CreationDate>
 </Bucket>
 <Bucket>
 <Name>liuchunhua</Name>
 <CreationDate>2012-08-22T14:33:34.000Z</CreationDate>
 </Bucket>
 </Buckets>
 </ListAllMyBucketsResult>
 */
void test_curl() {
	CURL *curl;
	CURLcode res;
	//const char* ACCESS_ID = "abysmn89uz488l1dfycon3qa";
	char* ACCESS_KEY = "qfEZ+LNuGJUP/FlRw1R3aKpwiwY=";
	char* method = "GET";
	M_str date = localtime_gmt();
	char* host = "storage.aliyun.com";
	char* content_type = "text/html";
	char buf[200];
	struct HashTable* headers = hash_table_init();
	hash_table_put(headers, "Content-Type", content_type);
	hash_table_put(headers, "Date", date);
	M_str authorization = oss_authorizate(ACCESS_KEY, method, headers, "/");
	curl = curl_easy_init();
	if (curl) {
		struct curl_slist *chunk = NULL;

		chunk = curl_slist_append(chunk,
				header(buf, "Authorization:OSS abysmn89uz488l1dfycon3qa",
						authorization));
		chunk = curl_slist_append(chunk,
				header(buf, "Content-Type", content_type));
		chunk = curl_slist_append(chunk, header(buf, "Date", date));

		curl_easy_setopt(curl, CURLOPT_URL, host);
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

		res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
		res = curl_easy_perform(curl);

		if (res != CURLE_OK) {
			printf("ERROR:%s\n", curl_easy_strerror(res));
		} else {

		}
		free(date);
		free(authorization);
		curl_slist_free_all(chunk);
		curl_easy_cleanup(curl);
	}
}

struct Owner* getOwner(xmlNodePtr node) {
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
struct Bucket* getBucket(xmlNodePtr node){
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
List getListBucket(xmlNodePtr node){
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
void test_bucket_xmls() {
	xmlDocPtr doc;
	xmlNodePtr root;
	xmlNodePtr cur;
	doc = xmlReadMemory(buckets, strlen(buckets), "noname.xml", NULL, 0);
	if (doc == NULL ) {
		fprintf(stderr, "Failed to parse document\n");
		return;
	}
	root = xmlDocGetRootElement(doc);
	cur = root->children;
	while (cur != NULL ) {
		fprintf(stderr, "%s\n", cur->name);
		if (!strcmp((char*) cur->name, "Owner")) {
			struct Owner* owner = getOwner(cur);
			fprintf(stderr,"ID:%s\n",owner->id);
			fprintf(stderr,"DisplayName:%s\n",owner->displayName);
			xmlFree(owner->id);
			xmlFree(owner->displayName);
			free(owner);
		}
		if (strcmp((char*)cur->name, "Buckets") == 0) {
			List list = getListBucket(cur);
			List element = list->next;
			while(element!=list){
				struct Bucket* bucket = element->ptr;
				fprintf(stderr,"name:%s\n",bucket->name);
				fprintf(stderr,"date:%s\n",bucket->creationDate);
				element =element->next;
			}
		}
		cur = cur->next;
	}
	xmlFreeDoc(doc);
	return;
}
