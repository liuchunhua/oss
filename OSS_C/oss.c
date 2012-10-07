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

static struct HttpResponse* http_request(OSSPtr oss, const char* method,
		const char* requestresource) {
	CURL *curl;
	CURLcode res;
	char* ACCESS_KEY = oss->access_key;
	struct HttpResponse* response = (struct HttpResponse*) malloc(
			sizeof(struct HttpResponse));
	response.memory = malloc(1);
	response.size = 0;
	M_str date = localtime_gmt();
	char* host = oss->host;
	char buf[200];
	char authorhead[100];
	sprintf(authorhead, "Authorization:OSS %s", oss->access_id);
	struct HashTable* headers = hash_table_init();
	hash_table_put(headers, "Date", date);
	M_str authorization = oss_authorizate(ACCESS_KEY, method, headers,
			requestresource);
	curl = curl_easy_init();
	if (curl) {
		struct curl_slist *chunk = NULL;

		chunk = curl_slist_append(chunk,
				header(buf, authorhead, authorization));
		chunk = curl_slist_append(chunk, header(buf, "Date", date));

		curl_easy_setopt(curl, CURLOPT_URL, host);
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

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
		free(date);
		free(authorization);
		curl_slist_free_all(chunk);
		curl_easy_cleanup(curl);
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
	response = http_request(oss, method, resource);
	if (response && response->code == 200) {
		List list = oss_ListAllMyBucketsResult(response->memory, NULL );
		return list;
	}
	return NULL ;
}

int PutBucket(OSSPtr oss, char* bucket) {
	struct HttpResponse* response;
	char buf[20];
	char* method = "PUT";
	char* resource = strcat(buf, "/");
	resource = strcat(buf, resource);
	response = http_request(oss, method, resource);
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
	return EXIT_FAILURE;
}
