/*

*/

#include <curl/curl.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>

#include "HashTable.h"

typedef struct
{
	char *blk;
	size_t size;
}MemBlk;

typedef struct
{
	MemBlk *(*init)();
	void (*destroy)(MemBlk *);
}MemBlkOpration;

MemBlk *memblk_init();
void memblk_destroy(MemBlk *mem);

MemBlkOpration MemBlkClass = {
		.init = memblk_init,
		.destroy = memblk_destroy
	};

struct HttpResponse
{
	size_t code;
	MemBlk *header;
	MemBlk *body;
};

typedef struct HttpResponse HttpResponse;
typedef struct{
	HttpResponse *(*init)();
	void (*destroy)(HttpResponse *);
}HttpResponseOpration;

HttpResponse *http_response_init();
void http_response_destroy(HttpResponse *httpresponse);

HttpResponseOpration HttpResponseClass = {
	.init = http_response_init,
	.destroy = http_response_destroy
};

typedef struct{
	char *method;
	char *url;
	HashTable *headers;
}HttpRequest;

typedef struct{
	HttpRequest *(*init)();
	void (*destroy)(HttpRequest *);
}HttpRequestOpration;

HttpRequest *http_request_init();
void http_request_destroy(HttpRequest *httprequest);
HttpRequestOpration HttpRequestClass = {
	.init = http_request_init,
	.destroy = http_request_destroy
};
