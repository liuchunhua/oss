/*

*/

#ifndef HTTP_H_
#define HTTP_H_

#include <curl/curl.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>

#include "HashTable.h"

//内存块
typedef struct
{
	char *blk;
	size_t size;
}MemBlk;

//内存块操作
typedef struct
{
	MemBlk *(*init)();
	void (*destroy)(MemBlk *);
}MemBlkOpration;

//内存块初始化
MemBlk *memblk_init();

//内存块释放
void memblk_destroy(MemBlk *mem);

//内存操作类
extern MemBlkOpration MemBlkClass;

//http返回结构体
typedef struct
{
	size_t code;
	MemBlk *header;
	MemBlk *body;
}HttpResponse;

//http返回结构体操作
typedef struct{
	HttpResponse *(*init)();
	void (*destroy)(HttpResponse *);
}HttpResponseOpration;

//初始化
HttpResponse *http_response_init();

//释放内存
void http_response_destroy(HttpResponse *httpresponse);

//http返回操作类
extern HttpResponseOpration HttpResponseClass;

//http响应结构体
typedef struct{
	char *method;
	char *url;
	HashTable *headers;
}HttpRequest;

//http响应操作
typedef struct{
	HttpRequest *(*init)();
	void (*destroy)(HttpRequest *);
}HttpRequestOpration;

//初始化-对headers不初始化
HttpRequest *http_request_init();

//释放内存，同时释放headers
void http_request_destroy(HttpRequest *httprequest);

//Http响应操作类
extern HttpRequestOpration HttpRequestClass;

#endif
