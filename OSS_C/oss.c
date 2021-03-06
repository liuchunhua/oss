/*
 * oss.c
 *
 *  Created on: 2012-10-7
 *      Author: lch
 */

#define _GNU_SOURCE


#include "ossutil.h"
#include "oss.h"
#include "String.h"
#include "service.h"
#include "http.h"
#include "oss_http.h"
#include "log.h"
#include "service.h"
#include "oss_config.h"

OSSOperation OSSClass =
{ .init = oss_init, .destroy = free_ossptr };

BucketOpration BucketClass =
{ .init = bucket_init, .destroy = bucket_destroy, .delete = DeleteBucket, .put =
    PutBucket, .putAcl = PutBucketACL, .getAcl = GetBucketACL };

OSSObjectOperation OSSObjectClass = {.init = ossobject_init, .destroy = free_ossobject};

struct HttpResponse
{
    size_t code;
    MemoryBlock* body;
    MemoryBlock* header;
};

struct Range
{
    size_t start;
    size_t end;
};
/**
static void free_http_response(struct HttpResponse* response)
{
    if (response)
    {
        if (response->body)
        {
            free(response->body->memory);
            free(response->body);
        }
        if (response->header)
        {
            free(response->header->memory);
            free(response->header);
        }
        free(response);
    }
}
**/
static char* acl[] =
{ "public-read-write", "public-read", "private" };

/*
static size_t write_memory(void *buffer, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    MemoryBlock* mem = (MemoryBlock*) userp;

    mem->memory = realloc(mem->memory, mem->size + realsize + 1);
    if (mem->memory == NULL )
    {
        printf("not enough memory (realloc returned NULL)\n");
        exit(EXIT_FAILURE);
    }

    memcpy(&(mem->memory[mem->size]), buffer, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

static size_t write_buf(void *buffer, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    MemoryBlock* mem = (MemoryBlock*) userp;
    memcpy(&(mem->memory[mem->size]), buffer, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    return realsize;
}

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
    size_t written = fwrite(ptr, size, nmemb, (FILE *) stream);
    return written;
}

static size_t read_callback(void *ptr, size_t size, size_t nmemb, void *stream)
{
    size_t retcode;
    curl_off_t nread;

    retcode = fread(ptr, size, nmemb, stream);

    nread = (curl_off_t) retcode;

    fprintf(stderr, "*** We read %" CURL_FORMAT_CURL_OFF_T
            " bytes from file\n", nread);

    return retcode;
}

static const char* header(char* buf, char* key, char* value)
{
    memset(buf, 0x0, 200);
    strcat(buf, key);
    strcat(buf, ":");
    strcat(buf, value);
    return buf;
}
static struct HttpResponse* response_init()
{
    struct HttpResponse* response = (struct HttpResponse*) malloc(
            sizeof(struct HttpResponse));
    memset(response, 0x0, sizeof(struct HttpResponse));
    response->body = (MemoryBlock*) malloc(sizeof(MemoryBlock));
    response->header = (MemoryBlock*) malloc(sizeof(MemoryBlock));
    response->body->memory = malloc(1);
    response->body->size = 0;
    response->header->memory = malloc(1);
    response->header->size = 0;
    return response;
}
*/
/*此方法已放弃，使用oss_http.h
static struct HttpResponse* http_request_old(OSSPtr oss, const char* method,
        const char* requestresource, struct HashTable* headers)
{
    CURL *curl;
    CURLcode res;
    struct HashTable* table;
    char* ACCESS_KEY = oss->access_key;
    int isInit = 0;
    struct HttpResponse* response = response_init();
    M_str date = localtime_gmt();
    char* host = oss->host;
    char buf[200] =
    { };
    char authorhead[100] =
    { };
    strcat(authorhead, "Authorization: OSS ");
    strcat(authorhead, oss->access_id);
    char* url = malloc(strlen(host) + strlen(requestresource) + 1);
    memset(url, 0x0, strlen(host) + strlen(requestresource) + 1);
    strcat(url, host);
    strcat(url, requestresource);
    if (headers == NULL )
    {
        isInit = 1;
        table = hash_table_init();
    }
    else
    {
        table = headers;
    }
    hash_table_put(table, "Date", date);
    M_str authorization = oss_authorizate(ACCESS_KEY, method, table,
            requestresource);
    curl = curl_easy_init();
    if (curl)
    {
        struct curl_slist *chunk = NULL;

        chunk = curl_slist_append(chunk,
                header(buf, authorhead, authorization));
        //        chunk = curl_slist_append(chunk, header(buf, "Date", date));
        List list = hash_table_get_key_list(table);
        List node;
        for_each(node,list)
        {
            struct pair* p = (struct pair*) node->ptr;
            curl_slist_append(chunk,
                    header(buf, (char*) p->key, (char*) p->value));
        }
        listFree(list);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);

        if (strcasestr(method, "put") != NULL )
        {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method);
        }
        if (strcasestr(method, "post") != NULL )
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
        if (strcasestr(method, "delete") != NULL )
        {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
        }
        if (strcasestr(method, "head") != NULL )
        {
            curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
        }
        if (oss->proxy != NULL )
        {
            curl_easy_setopt(curl, CURLOPT_PROXY, oss->proxy);
        }
        res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory);
        curl_easy_setopt(curl, CURLOPT_WRITEHEADER, (void* )response->header);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void* )response->body);
#ifdef DEBUG
        syslog(LOG_MAKEPRI(LOG_USER,LOG_WARNING), "curl perform\n");
#endif
        res = curl_easy_perform(curl);
        if (res == CURLE_OK)
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &(response->code));
        else
        {
#ifdef DEBUG
            syslog(LOG_MAKEPRI(LOG_USER,LOG_WARNING), "%s\n", curl_easy_strerror(res));
#endif
        }
        if (isInit)
            hash_table_free(table);
        free(url);
        free(date);
        free(authorization);
        curl_slist_free_all(chunk);
        curl_easy_cleanup(curl);
        return response;
    }
    fprintf(stderr, "ERROR:%s\n", "request failed");
    return NULL ;
}
*/
/*
static struct HttpResponse* http_upload(OSSPtr oss, const char* method,
        const char* requestresource, char* file, struct HashTable* headers)
{
    CURL *curl;
    CURLcode res;
    struct HashTable* table;
    FILE* hd_src;
    int hd;
    struct stat fileinfo;
    char* ACCESS_KEY = oss->access_key;
    int isInit = 0;

    hd = open(file, O_RDONLY);
    fstat(hd, &fileinfo);
    close(hd);
    fprintf(stderr, "%d\n", fileinfo.st_size);
    hd_src = fopen(file, "rb");
    struct HttpResponse* response = response_init();
    M_str date = localtime_gmt();
    char* host = oss->host;
    char buf[200] =
    { };
    char authorhead[100] =
    { };
    strcat(authorhead, "Authorization: OSS ");
    strcat(authorhead, oss->access_id);
    char* url = malloc(strlen(host) + strlen(requestresource) + 1);
    memset(url, 0x0, strlen(host) + strlen(requestresource) + 1);
    strcat(url, host);
    strcat(url, requestresource);
    if (headers == NULL )
    {
        isInit = 1;
        table = hash_table_init();
    }
    else
    {
        table = headers;
    }
    hash_table_put(table, "Date", date);
    M_str authorization = oss_authorizate(ACCESS_KEY, method, table,
            requestresource);
    curl = curl_easy_init();
    if (curl)
    {
        struct curl_slist *chunk = NULL;

        chunk = curl_slist_append(chunk,
                header(buf, authorhead, authorization));
        //        chunk = curl_slist_append(chunk, header(buf, "Date", date));
        List list = hash_table_get_key_list(table);
        List node;
        for_each(node,list)
        {
            struct pair* p = (struct pair*) node->ptr;
            curl_slist_append(chunk,
                    header(buf, (char*) p->key, (char*) p->value));
        }
        listFree(list);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        if (strcasestr(method, "put") != NULL )
        {
            curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
            curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
            curl_easy_setopt(curl, CURLOPT_READDATA, hd_src);
            curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE,
                    (curl_off_t )(fileinfo.st_size));
        }
        if (strcasestr(method, "post") != NULL )
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
        if (oss->proxy != NULL )
        {
            curl_easy_setopt(curl, CURLOPT_PROXY, oss->proxy);
        }
        res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void* )response);
        res = curl_easy_perform(curl);
        fclose(hd_src);
        if (res == CURLE_OK)
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &(response->code));
        else
        {
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
        return response;
    }
    fprintf(stderr, "ERROR:%s\n", "request failed");
    return NULL ;
}

static struct HttpResponse* http_download(OSSPtr oss, const char* method,
        const char* requestresource, size_t content_length, char* dest_file,
        struct HashTable* headers, unsigned short redownnload)
{
    CURL *curl;
    CURLcode res;
    FILE* file;
    int hd;
    struct stat fileinfo;
    if (redownnload == 0)
        file = fopen(dest_file, "ab");
    else
        file = fopen(dest_file, "wb");
    hd = open(dest_file, O_RDONLY);
    fstat(hd, &fileinfo);
    close(hd);

    struct HashTable* table;
    char* ACCESS_KEY = oss->access_key;
    int isInit = 0;
    struct HttpResponse* response = response_init();
    if (redownnload == 0 && content_length <= fileinfo.st_size)
    {
        response->code = 200;
        return response;
    }
    char x_y[64] =
    { };
    M_str date = localtime_gmt();
    char* host = oss->host;
    char buf[200] =
    { };
    char authorhead[100] =
    { };
    strcat(authorhead, "Authorization: OSS ");
    strcat(authorhead, oss->access_id);
    char* url = malloc(strlen(host) + strlen(requestresource) + 1);
    memset(url, 0x0, strlen(host) + strlen(requestresource) + 1);
    strcat(url, host);
    strcat(url, requestresource);
    if (headers == NULL )
    {
        isInit = 1;
        table = hash_table_init();
    }
    else
    {
        table = headers;
    }
    hash_table_put(table, "Date", date);
    M_str authorization = oss_authorizate(ACCESS_KEY, method, table,
            requestresource);
    curl = curl_easy_init();
    if (curl)
    {
        struct curl_slist *chunk = NULL;
        sprintf(x_y, "bytes=%d-%d", fileinfo.st_size, content_length - 1);
        //      chunk = curl_slist_append(chunk, header(buf, "Date", date));
        chunk = curl_slist_append(chunk, header(buf, "Range", x_y));
        chunk = curl_slist_append(chunk,
                header(buf, authorhead, authorization));
        curl_slist_append(chunk, header(buf, "Accept", ""));

        List list = hash_table_get_key_list(table);
        List node;
        for_each(node,list)
        {
            struct pair* p = (struct pair*) node->ptr;
            curl_slist_append(chunk,
                    header(buf, (char*) p->key, (char*) p->value));
        }
        listFree(list);
        if (oss->proxy != NULL )
        {
            curl_easy_setopt(curl, CURLOPT_PROXY, oss->proxy);
        }
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

        res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void* )file);
        //      curl_easy_setopt(curl, CURLOPT_RANGE, x_y);
        res = curl_easy_perform(curl);
        fclose(file);
        if (res == CURLE_OK)
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &(response->code));
        else
        {
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
        return response;
    }
    fprintf(stderr, "ERROR:%s\n", "request failed");
    return NULL ;
}
*/
/**
static struct HttpResponse* http_download_memory(OSSPtr oss, const char* method,
        const char* requestresource, char* buf, size_t size, off_t offset,
        struct HashTable* headers)
{
    if (size == 0)
        return NULL ;
    CURL *curl;
    CURLcode res;
    struct HashTable* table;
    MemoryBlock* block = (MemoryBlock*) malloc(sizeof(MemoryBlock));
    memset(block, 0x0, sizeof(MemoryBlock));
    block->memory = buf;
    block->size = 0;
    char* ACCESS_KEY = oss->access_key;
    int isInit = 0;
    struct HttpResponse* response = response_init();
    char x_y[64] =
    { };
    M_str date = localtime_gmt();
    char* host = oss->host;
    char buffer[200] =
    { };
    char authorhead[100] =
    { };
    strcat(authorhead, "Authorization: OSS ");
    strcat(authorhead, oss->access_id);
    char* url = malloc(strlen(host) + strlen(requestresource) + 1);
    memset(url, 0x0, strlen(host) + strlen(requestresource) + 1);
    strcat(url, host);
    strcat(url, requestresource);
    if (headers == NULL )
    {
        isInit = 1;
        table = hash_table_init();
    }
    else
    {
        table = headers;
    }
    hash_table_put(table, "Date", date);
    M_str authorization = oss_authorizate(ACCESS_KEY, method, table,
            requestresource);
    curl = curl_easy_init();
    if (curl)
    {
        struct curl_slist *chunk = NULL;
        sprintf(x_y, "bytes=%d-%d", offset, offset + size - 1);
        //      chunk = curl_slist_append(chunk, header(buf, "Date", date));
        chunk = curl_slist_append(chunk, header(buffer, "Range", x_y));
        chunk = curl_slist_append(chunk,
                header(buffer, authorhead, authorization));
        curl_slist_append(chunk, header(buffer, "Accept", ""));

        List list = hash_table_get_key_list(table);
        List node;
        for_each(node,list)
        {
            struct pair* p = (struct pair*) node->ptr;
            curl_slist_append(chunk,
                    header(buffer, (char*) p->key, (char*) p->value));
        }
        listFree(list);
        if (oss->proxy != NULL )
        {
            curl_easy_setopt(curl, CURLOPT_PROXY, oss->proxy);
        }
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

        res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_buf);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void* )block);
        //      curl_easy_setopt(curl, CURLOPT_RANGE, x_y);
        res = curl_easy_perform(curl);
        if (res == CURLE_OK)
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &(response->code));
        else
        {
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
        return response;
    }
    fprintf(stderr, "ERROR:%s\n", "request failed");
    return NULL ;
}
**/
static OSSObject* oss_GetObject(const char* httpheader)
{
    char *str1, *token;
    char *savePtr1;
    OSSObject* object = OSSObjectClass.init();
    for (str1 = (char *)httpheader;; str1 = NULL )
    {
        token = strtok_r(str1, "\r\n", &savePtr1);
        if (token == NULL )
            break;
        int index = StringClass.indexOf(token, ':');
        if (index == -1)
            continue;
        char *key = StringClass.substring(token, 0, index - 1);
        char *value = StringClass.substring(token, index + 1, strlen(token));
        if (strcasecmp(key, "Last-Modified") == 0)
            object->mtime = StrGmtToLocaltime(value);
        if (strcasecmp(key, "Content-Type") == 0)
            object->minetype = strdup(value);
        if (strcasecmp(key, "Content-Length") == 0)
            object->size = atol(value);
        if (strcasecmp(key, "ETag") == 0)
            object->etag = strdup(value);
        free(key);
        free(value);
    }
    return object;
}

/*
 * public method
 */

OSSPtr new_ossptr()
{
    OSSPtr oss = (OSSPtr) malloc(sizeof(OSS));
    memset(oss, 0x0, sizeof(OSS));
    if (oss == NULL )
    {
        fprintf(stderr, "%s\n", "OSS malloc failed");
        exit(EXIT_FAILURE);
    }
    return oss;
}

void free_ossptr(OSSPtr oss)
{
    if (oss->host)
        free(oss->host);
    if (oss->access_id)
        free(oss->access_id);
    if (oss->access_key)
        free(oss->access_key);
    if (oss->proxy)
        free(oss->proxy);
    if (oss->bucket)
        free(oss->bucket);
    if (oss)
        free(oss);
}

OSSObject *ossobject_init()
{
    OSSObject *ossObject = malloc(sizeof(OSSObject));
    memset(ossObject, 0x0, sizeof(OSSObject));
    ossObject->minetype = NULL;
    return ossObject;
}
void free_ossobject(OSSObject* obj)
{
    if (obj)
    {
        if(obj->etag)
            free(obj->etag);
        if(obj->minetype)
            free(obj->minetype);
        if(obj->name)
            free(obj->name);
        free(obj);
    }
}

OSSPtr oss_init(const char* host, const char* id, const char* key)
{
    OSSPtr oss = new_ossptr();
    oss->host = strdup(host);
    oss->access_id = strdup(id);
    oss->access_key = strdup(key);
    oss->proxy = NULL;
    return oss;
}

List GetService(OSSPtr oss)
{
    HttpResponse* response;
    HttpRequest *request = HttpRequestClass.init();
    List buckets = NULL;
    request->method = strdup("GET");
    request->url = strdup("/");
    response = OSSHttpClass.request(request, oss);
    if (response && response->code == 200)
    {
        log_debug("%s", response->body->blk);
        BucketsResult *result = bucket_result_parse(response->body->blk);
        buckets = result->buckets;
        if (result->owner)
            OwnerClass.destroy(result->owner);
        free(result);
    }
    if (response)
        HttpResponseClass.destroy(response);
    HttpRequestClass.destroy(request);
    return buckets;
}

int PutBucket(OSSPtr oss, char* bucket)
{
    log_debug("Put %s", bucket);
    int result = EXIT_FAILURE;
    HttpResponse *response;
    HttpRequest *request = HttpRequestClass.init();

    request->method = strdup("PUT");
    request->url = strdup("/");
    request->headers = HashTableClass.init();
    //HashTableClass.put(request->headers, "Host", strdup(oss->host));
    oss->bucket = strdup(bucket);
    response = OSSHttpClass.request(request, oss);

    if (response && response->code == 200)
    {
        log_debug("Success put %s", bucket);
        result = EXIT_SUCCESS;
    }
    else if (response)
        switch (response->code)
        {
            case 409:
                log_debug("%s:%s", bucket, "BucketAlreadyExists")
                    ;
                break;
            case 400:
                log_debug("%s:%s\n", bucket, "InvalidBucketName")
                    ;
                break;
            default:
                break;
        }
    HttpRequestClass.destroy(request);
    HttpResponseClass.destroy(response);
    return result;
}

int PutBucketACL(OSSPtr oss, char* bucket, ACL a)
{
    int result = EXIT_FAILURE;
    HttpRequest *request;
    HttpResponse *response;

    request = HttpRequestClass.init();
    request->url = strdup("/");
    request->method = strdup("PUT");
    request->headers = HashTableClass.init();

    if(!oss->bucket)
        oss->bucket = strdup(bucket);

    HashTableClass.put(request->headers, "x-oss-acl", strdup(acl[a]));

    response = OSSHttpClass.request(request, oss);
    if (response->code == 200)
    {
        result = EXIT_SUCCESS;
    }
    else if (response->code == 403)
    {
        log_error("%s", "AccessDenied");
    }

    HttpRequestClass.destroy(request);
    HttpResponseClass.destroy(response);
    return result;
}

ListBucketResult* ListObject(OSSPtr oss, const char* bucket, const char* prefix,
        unsigned int maxkeys, const char* marker, const char* delimiter)
{
    HttpRequest *request;
    HttpResponse * response;
    ListBucketResult* result;

    request = HttpRequestClass.init();
    request->method = strdup("GET");

    if(!oss->bucket)
        oss->bucket = strdup(bucket);

    char buf[1024] =
    { };
    buf[0] = '/';
    List list = listInit();
    char max_keys[5];
    sprintf(max_keys, "%d", maxkeys);

    if (prefix && strlen(prefix) > 0)
    {
        struct pair* p = malloc(sizeof(struct pair));
        p->key = "prefix";
        p->value = (char *)prefix;
        listAdd(list, p);
    }
    if (maxkeys > 0)
    {
        struct pair* p = malloc(sizeof(struct pair));
        p->key = "max-keys";
        p->value = max_keys;
        listAdd(list, p);
    }
    if (marker && strlen(marker) > 0)
    {
        struct pair* p = malloc(sizeof(struct pair));
        p->key = "marker";
        p->value = (char *)marker;
        listAdd(list, p);
    }
    if (delimiter && strlen(delimiter) > 0)
    {
        struct pair* p = malloc(sizeof(struct pair));
        p->key = "delimiter";
        p->value = (char *)delimiter;
        listAdd(list, p);
    }
    if (!listIsEmpty(list))
    {
        strcat(buf, "?");
        List node;
        for_each(node,list)
        {
            if (strstr(buf, "=") != NULL )
                strcat(buf, "&");
            struct pair* p = (struct pair*) node->ptr;
            strcat(buf, p->key);
            strcat(buf, "=");
            strcat(buf, (char*) p->value);
        }
    }

    request->url = strdup(buf);
    request->headers = HashTableClass.init();

    response = OSSHttpClass.request(request, oss);
    if (response->code == 200)
    {
        result = oss_ListBucketResult(response->body->blk);
    }
    HttpRequestClass.destroy(request);
    HttpResponseClass.destroy(response);
    ListClass.destroy_fun(list, free);
    return result;
}

ACL GetBucketACL(OSSPtr oss, char* bucket)
{
    HttpRequest *request;
    HttpResponse *response;

    request = HttpRequestClass.init();
    request->url = strdup("/?acl");
    request->method = strdup("GET");
    if(!oss->bucket)
        oss->bucket = strdup(bucket);
    request->headers = HashTableClass.init();

    response = OSSHttpClass.request(request, oss);
    if (response->code == 200)
    {
        M_str result = oss_GetBucketAcl(response->body->blk);
        if (strcasestr(result, acl[RO]) != NULL )
        {
            free(result);
            return RO;
        }
        if (strcasestr(result, acl[RW]) != NULL )
        {
            free(result);
            return RW;
        }
        if (strcasestr(result, acl[PRIVATE]) != NULL )
        {
            free(result);
            return PRIVATE;
        }
    }
    return -1;
}

int DeleteBucket(OSSPtr oss, char* bucket)
{
    log_debug("Delete %s", bucket);
    int result = EXIT_FAILURE;
    HttpResponse *response;
    HttpRequest *request = HttpRequestClass.init();
    request->url = strdup("/");
    request->method = strdup("DELETE");
    if(!oss->bucket)
        oss->bucket = strdup(bucket);
    request->headers = HashTableClass.init();

    response = OSSHttpClass.request(request, oss);
    if (response && response->code == 200)
    {
        log_debug("Success delete %s", bucket);
        result = EXIT_SUCCESS;
    }
    if (response)
        switch (response->code)
        {
            case 409:
                log_error("%s:%s", bucket, "BucketNotEmpty")
                    ;
                break;
            case 403:
                log_error("%s:%s", bucket, "AccessDenied")
                    ;
                break;
            case 404:
                log_error("%s:%s", bucket, "NoSuchBucket")
                    ;
                break;
            default:
                break;
        }

    HttpRequestClass.destroy(request);
    HttpResponseClass.destroy(response);
    return result;
}
int PutObject(OSSPtr oss, const char *bucket, const char *objectname, const char *file)
{
    HttpResponse *response;
    HttpRequest *request;

    request = HttpRequestClass.init();
    request->url = strdup(objectname);
    request->method = strdup("PUT");
    if(!oss->bucket)
        oss->bucket = strdup(bucket);

    response = OSSHttpClass.request_upload(request, oss, file);
    if (response && response->code == 200)
    {
        return EXIT_SUCCESS;
    }
    if (response)
        switch (response->code)
        {
            case 411:
                log_error("%s:%s", bucket, "MissingContentLength");
                break;
            case 400:
                log_error("%s:%s", bucket, "RequestTimeout");
                break;
            case 404:
                log_error("%s:%s", bucket, "NoSuchBucket");
                break;
        }
    HttpRequestClass.destroy(request);
    HttpResponseClass.destroy(response);
    return EXIT_FAILURE;
}
size_t GetObject(OSSPtr oss, const char *bucket, const char* object, const char *desfile)
{
    size_t content_size = 0;
    HttpResponse *response;
    HttpRequest *request;

    request = HttpRequestClass.init();
    request->url = strdup(object);
    request->method = strdup("GET");
    if(!oss->bucket)
    {
        oss->bucket = strdup(bucket);
    }

    response = OSSHttpClass.request_download(request, oss, desfile);
    if(response->code == 200)
    {
        OSSObject *header = oss_GetObject(response->header->blk); 
        content_size = header->size;
        OSSObjectClass.destroy(header);
    }
    HttpResponseClass.destroy(response);
    HttpRequestClass.destroy(request);
    return content_size;
}

/**
size_t GetObjectIntoMemory(OSSPtr oss, const char* object, char* buf,
        size_t size, off_t offset, struct HashTable* table)
{
    struct HttpResponse* response;
    char* method = "GET";
    response = http_download_memory(oss, method, object, buf, size, offset,
            NULL );
    free_http_response(response);
    return size;
}
**/
OSSObject* HeadObject(OSSPtr oss, const char *bucket, const char* object)
{
    HttpResponse *response;
    HttpRequest *request;

    request = HttpRequestClass.init();
    request->url = strdup(object);
    request->method = strdup("HEAD");

    if(!oss->bucket)
        oss->bucket = strdup(bucket);
    response = OSSHttpClass.request(request, oss);
    OSSObject *ossobject = NULL;
    if (response && response->header && response->code == 200)
    {
        ossobject = oss_GetObject(response->header->blk);
        ossobject->name = strdup(object);
    }

    HttpResponseClass.destroy(response);
    HttpRequestClass.destroy(request);
    return ossobject;
}

int CopyObject(OSSPtr oss, const char *bucket, const char* source, char* des)
{
    HttpRequest *request;
    HttpResponse *response;

    request = HttpRequestClass.init();
    request->url = strdup(des);
    request->method = strdup("PUT");
    request->headers = HashTableClass.init();

    if(!oss->bucket)
        oss->bucket = strdup(bucket);

    HashTableClass.put(request->headers, "x-oss-copy-source", (char *)source);
    response = OSSHttpClass.request(request, oss); 
    if (response->code == 200)
        return EXIT_SUCCESS;
    else
    {
        log_error("%s", response->header->blk);
        return EXIT_FAILURE;
    }
}
int DeleteObject(OSSPtr oss, const char *bucket, const char *object)
{
    HttpResponse *response;
    HttpRequest *request;

    request = HttpRequestClass.init();
    request->url = strdup(object);
    request->method = strdup("DELETE");

    if(!oss->bucket)
        oss->bucket = strdup(bucket);
    response = OSSHttpClass.request(request, oss);

    if (response && response->code == 200)
    {
        return EXIT_SUCCESS;
    }
    if (response)
        switch (response->code)
        {
            case 204:
                log_error("%s:%s", object, "No Content");
                break;
            case 404:
                log_error("%s:%s", object, "NoSuchBucket");
                break;
            default:
                break;
        }
    HttpResponseClass.destroy(response);
    HttpRequestClass.destroy(request);
    return EXIT_FAILURE;
}
