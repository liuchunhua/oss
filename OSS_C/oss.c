/*
 * oss.c
 *
 *  Created on: 2012-10-7
 *      Author: lch
 */

#include "oss.h"
#include "ossutil.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <curl/curl.h>

struct HttpResponse {
	size_t code;
	char *memory;
	size_t size;
};

static void free_http_response(struct HttpResponse* response) {
	if (response) {
		free(response->memory);
		free(response);
	}
}
static char* acl[] = { "public-read-write", "public-read", "private" };

static size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp) {
	size_t realsize = size * nmemb;
	struct HttpResponse *mem = (struct HttpResponse *) userp;

	mem->memory = realloc(mem->memory, mem->size + realsize + 1);
	if (mem->memory == NULL ) {
		/* out of memory! */
		printf("not enough memory (realloc returned NULL)\n");
		exit(EXIT_FAILURE);
	}

	memcpy(&(mem->memory[mem->size]), buffer, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}

static const char* header(char* buf, char* key, char* value) {
	memset(buf, 0x0, 200);
	strcat(buf, key);
	strcat(buf, ":");
	strcat(buf, value);
	return buf;
}

static struct HttpResponse* http_request(OSSPtr oss, const char* method,
		const char* requestresource, struct HashTable* headers) {
	CURL *curl;
	CURLcode res;
	struct HashTable* table;
	char* ACCESS_KEY = oss->access_key;
	int isInit = 0;
	struct HttpResponse* response = (struct HttpResponse*) malloc(
			sizeof(struct HttpResponse));
	response->memory = malloc(1);
	response->size = 0;
	M_str date = localtime_gmt();
	char* host = oss->host;
	char buf[200];
	char authorhead[100] = { };
	strcat(authorhead, "Authorization: OSS ");
	strcat(authorhead, oss->access_id);
	char* url = malloc(strlen(host) + strlen(requestresource) + 1);
	memset(url, 0x0, strlen(host) + strlen(requestresource) + 1);
	strcat(url, host);
	strcat(url, requestresource);
	if (headers == NULL ) {
		isInit = 1;
		table = hash_table_init();
	} else {
		table = headers;
	}
	hash_table_put(table, "Date", date);
	M_str authorization = oss_authorizate(ACCESS_KEY, method, table,
			requestresource);
	curl = curl_easy_init();
	if (curl) {
		struct curl_slist *chunk = NULL;

		chunk = curl_slist_append(chunk,
				header(buf, authorhead, authorization));
		chunk = curl_slist_append(chunk, header(buf, "Date", date));
		List list = hash_table_get_key_list(table);
		List node;
		for_each(node,list)
		{
			struct pair* p = (struct pair*) node->ptr;
			if (strcasestr((char*) (p->key), "x-oss-") != NULL ) {
				curl_slist_append(chunk,
						header(buf, (char*) p->key, (char*) p->value));
			}
		}
		listFree(list);
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		if (strcasestr(method, "put") != NULL ) {
			curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
			curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)0L);
		}
		if (strcasestr(method, "post") != NULL )
			curl_easy_setopt(curl, CURLOPT_POST, 1L);

		res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)response);
		res = curl_easy_perform(curl);
		if (res == CURLE_OK)
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &(response->code));
		else {
			fprintf(stderr, "ERROR:%s\n", curl_easy_strerror(res));
			response = NULL;
		}
		if (isInit)
			hash_table_free(table);
		free(url);
		free(date);
		free(authorization);
		curl_slist_free_all(chunk);
		curl_easy_cleanup(curl);
		fprintf(stderr, "%s\n", response->memory);
		return response;
	}
	fprintf(stderr, "ERROR:%s\n", "request failed");
	return NULL ;
}

OSSPtr new_ossptr() {
	OSSPtr oss = (OSSPtr) malloc(sizeof(OSS));
	if (oss == NULL ) {
		fprintf(stderr, "%s\n", "OSS malloc failed");
		exit(EXIT_FAILURE);
	}
	return oss;
}

void free_ossptr(OSSPtr oss) {
	if (oss->host)
		free(oss->host);
	if (oss->access_id)
		free(oss->access_id);
	if (oss->access_key)
		free(oss->access_key);
	if (oss)
		free(oss);
}

OSSPtr oss_init(const char* host, const char* id, const char* key) {
	OSSPtr oss = new_ossptr();
	oss->host = strdup(host);
	oss->access_id = strdup(id);
	oss->access_key = strdup(key);
	return oss;
}

List GetService(OSSPtr oss) {
	struct HttpResponse* response;
	char* method = "GET";
	char* resource = "/";
	response = http_request(oss, method, resource, NULL );
	if (response && response->code == 200) {
		List list = oss_ListAllMyBucketsResult(response->memory, NULL );
		free_http_response(response);
		return list;
	}
	if (response)
		free_http_response(response);
	return NULL ;
}

int PutBucket(OSSPtr oss, char* bucket) {
	struct HttpResponse* response;
	char buf[20] = { };
	char* method = "PUT";
	char* resource = strcat(buf, "/");
	resource = strcat(buf, bucket);
	response = http_request(oss, method, resource, NULL );
	if (response && response->code == 200) {
		return EXIT_SUCCESS;
	}
	if (response)
		switch (response->code) {
		case 409:
			fprintf(stderr, "%s:%s\n", bucket, "BucketAlreadyExists");
			break;
		case 400:
			fprintf(stderr, "%s:%s\n", bucket, "InvalidBucketName");
			break;
		default:
			break;
		}
	free_http_response(response);
	return EXIT_FAILURE;
}

int PutBucketACL(OSSPtr oss, char* bucket, ACL a) {
	char* method = "PUT";
	char buf[20] = { };
	char* resource = strcat(buf, "/");
	resource = strcat(buf, bucket);
	char* permission = acl[a];
	struct HashTable* table = hash_table_init();
	hash_table_put(table, "x-oss-acl", permission);
	struct HttpResponse* response = http_request(oss, method, resource, table);

	if (response->code == 200)
		return EXIT_SUCCESS;
	else if (response->code == 403) {
		fprintf(stderr, "ERROR:%s\n", "AccessDenied");
		return EXIT_FAILURE;
	} else {
		fprintf(stderr, "ERROR:%s\n", response->memory);
		return EXIT_FAILURE;
	}
	free_http_response(response);
	hash_table_free(table);
}

char* GetBucket(OSSPtr oss, char* bucket) {
	char* method = "GET";
	char buf[50] = { };
	char* resource = strcat(buf, "/");
	resource = strcat(buf, bucket);
	struct HttpResponse* response = http_request(oss, method, resource, NULL );
	if (response->code == 200) {
		char* result = strdup(response->memory);
		return result;
	}
	free_http_response(response);
}
