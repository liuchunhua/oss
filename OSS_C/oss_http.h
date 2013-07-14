/*
	oss http请求方法：
		普通方法，返回xml。
		下载文件。
		上传文件。
*/
#ifndef OSS_HTTP_H_
#define OSS_HTTP_H_

#include "http.h"
#include "oss.h"

typedef struct{
	HttpResponse *(*request)(HttpRequest *, OSSPtr);
	HttpResponse *(*request_download)(HttpRequest *, OSSPtr, char *);
	HttpResponse *(*request_upload)(HttpRequest *, OSSPtr, char *);
}OSSHttpOpration;

HttpResponse *oss_http_request(HttpRequest *httprequest, OSSPtr oss);

HttpResponse *oss_http_request_download(HttpRequest *httprequest, \
									OSSPtr oss, char *path);

HttpResponse *oss_http_request_upload(HttpRequest *httprequest, \
									OSSPtr oss, char *path);

extern OSSHttpOpration OSSHttpClass;


#endif
