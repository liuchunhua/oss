/*
 oss http请求方法：
 普通方法，返回xml。
 下载文件。
 上传文件。
 */
#ifndef OSS_HTTP_H_
#define OSS_HTTP_H_

#include "http.h"
#include "oss_config.h"

//Http相关操作
typedef struct
{
    HttpResponse *(*request)(HttpRequest *, OSSPtr);
    HttpResponse *(*request_download)(HttpRequest *, OSSPtr, const char *);
    HttpResponse *(*request_upload)(HttpRequest *, OSSPtr, const char *);
} OSSHttpOpration;

//普通http方位操作
HttpResponse *oss_http_request(HttpRequest *httprequest, OSSPtr oss);

//下载文件
HttpResponse *oss_http_request_download(HttpRequest *httprequest, OSSPtr oss,
        const char *path);
//上传文件
HttpResponse *oss_http_request_upload(HttpRequest *httprequest, OSSPtr oss,
        const char *path);

//http访问类
extern OSSHttpOpration OSSHttpClass;

#endif
