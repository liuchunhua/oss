#include <errno.h>

#include "oss_http.h"
#include "log.h"
#include "String.h"
#include "ossutil.h"
#include "authen.h"

//OSSHttp操作类
OSSHttpOpration OSSHttpClass =
{ .request = oss_http_request, .request_download = oss_http_request_download,
        .request_upload = oss_http_request_upload };

static size_t write_memory(void *buffer, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    MemBlk* mem = userp;

    log_debug("%d", realsize);

    mem->blk = realloc(mem->blk, mem->size + realsize + 1);
    if (mem->blk == NULL )
    {
        /* out of blk! */
        log_error("not enough blk (realloc returned NULL)\n");
        exit(EXIT_FAILURE);
    }

    memcpy(&(mem->blk[mem->size]), buffer, realsize);
    mem->size += realsize;
    mem->blk[mem->size] = 0;

    return realsize;
}

static size_t write_file(void *ptr, size_t size, size_t nmemb, void *stream)
{
    size_t writted = fwrite(ptr, size, nmemb, (FILE *) stream);
    log_debug("%d", writted);
    return writted;
}

static size_t read_file(void *ptr, size_t size, size_t nmemb, void *stream)
{
    log_debug("%s", "#");
    int fd = *((int *) stream);
    size_t count = read(fd, ptr, size * nmemb);
    return (curl_off_t) count;
}

static void oss_http_request_init(HttpRequest *httprequest, OSSPtr oss)
{

    const char *url = httprequest->url;
    const char *method = httprequest->method;
    char *date = localtime_gmt();

    if (httprequest->headers == NULL )
    {
        httprequest->headers = HashTableClass.init();
        log_debug("init request header");
    }

    HashTableClass.put(httprequest->headers, "Date", date);
    //HashTableClass.put(httprequest->headers, "Expect", "");
    //HashTableClass.put(httprequest->headers, "Content-Type",
    //        strdup("text/plain"));

    //Host根据bucket是否存在判断
    if (oss->bucket)
    {
        HashTableClass.put(httprequest->headers, "Host",
                StringClass.concat(3, oss->bucket, ".", oss->host));
    }
    else
    {
        HashTableClass.put(httprequest->headers, "Host", strdup(oss->host));
    }
    log_debug("计算验证码");
    char *key = new_oss_authorizate(oss, method, httprequest->headers, url);

    char *auth = StringClass.concat(4, "OSS ", oss->access_id, ":", key);
    log_debug("date %s", date);

    log_debug("Authorization : %s", auth);
    HashTableClass.put(httprequest->headers, "Authorization", auth);
    free(key);
}

static void curl_chunk_init(struct curl_slist *chunk, HttpRequest * request)
{
    List list = HashTableClass.get_all(request->headers);
    List node;
    for_each(node, list)
    {
        struct pair *p = node->ptr;
        char *s = StringClass.concat(3, (char *) p->key, ":",
                (char *) p->value);
        log_debug("%s", s);
        chunk = curl_slist_append(chunk, s);
    }
    ListClass.destroy(list);
}

HttpResponse *oss_http_request(HttpRequest *httprequest, OSSPtr oss)
{
    CURL *curl;
    CURLcode res;
    HttpResponse *response;
    char *url;

    oss_http_request_init(httprequest, oss);

    curl = curl_easy_init();

    log_debug("curl start");
    if (curl)
    {
        char *method = httprequest->method;
        struct curl_slist *chunk = NULL;

        url = StringClass.concat(2, oss->host, httprequest->url);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);

        if (strcasecmp(method, "get") == 0)
        {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
        }
        if (strcasecmp(method, "put") == 0)
        {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method);
        }
        if (strstr(method, "post") == 0)
        {
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
        }
        if (strcasecmp(method, "delete") == 0)
        {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
        }
        if (strcasecmp(method, "head") == 0)
        {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "HEAD");
        }
        if (oss->proxy != NULL )
        {
            curl_easy_setopt(curl, CURLOPT_PROXY, oss->proxy);
        }
        log_debug("http header init");
        chunk = curl_slist_append(chunk, "Content-Length:");
        chunk = curl_slist_append(chunk, "Content-Type:");
        curl_chunk_init(chunk, httprequest);
        log_debug("http header done");

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
        response = HttpResponseClass.init();

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory);
        curl_easy_setopt(curl, CURLOPT_WRITEHEADER, (void * )response->header);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void * )response->body);

        log_debug("%s %s", method, url);
        res = curl_easy_perform(curl);
        log_debug("return code %d", res);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &(response->code));
        if (res != CURLE_OK)
        {
            strcat((char *) response->body->blk, curl_easy_strerror(res));
        }
        log_debug("body:\n%s", response->body->blk);
        free(url);
        curl_slist_free_all(chunk);
        curl_easy_cleanup(curl);
        return response;
    }
    return NULL ;
}

HttpResponse *oss_http_request_download(HttpRequest *httprequest, OSSPtr oss,
        const char *path)
{
    size_t start;
    CURL *curl;
    CURLcode res;
    HttpResponse *response;
    char *url;

    //断点续传
    if (httprequest->headers)
    {
        struct pair *p = HashTableClass.get(httprequest->headers, "Range");
        char *range = p->value;
        if (range)
        {
            range = strstr(range, "bytes=");
            int mid = StringClass.indexOf(range, '-');
            char *num = StringClass.substring(range, 0, mid);
            if (num)
            {
                start = atol(num);
            }
        }
    }

    FILE *f = NULL;
    if (start == 0)
    {
        f = fopen(path, "w");
    }
    else
    {
        f = fopen(path, "a+");
        fseek(f, start, SEEK_SET);
    }

    if (!f)
    {
        log_msg(strerror(errno));
        return NULL ;
    }

    oss_http_request_init(httprequest, oss);

    response = HttpResponseClass.init();

    curl = curl_easy_init();
    if (curl)
    {
        url = StringClass.concat(2, oss->host, httprequest->url);
        struct curl_slist *chunk = NULL;

        chunk = curl_slist_append(chunk, "");
        curl_chunk_init(chunk, httprequest);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_file);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void * )f);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, write_memory);
        curl_easy_setopt(curl, CURLOPT_WRITEHEADER, (void * )response->header);
        if (oss->proxy != NULL )
        {
            curl_easy_setopt(curl, CURLOPT_PROXY, oss->proxy);
        }
        res = curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &(response->code));
        if (res != CURLE_OK)
        {
            strcat((char *) response->body->blk, curl_easy_strerror(res));
        }
        //      log_debug("%s", response->header);
        log_debug("%s", "close file");
        if (f)
            fclose(f);
        log_debug("%s", "download complete.");
        free(url);
        curl_slist_free_all(chunk);
        curl_easy_cleanup(curl);
        return response;
    }
    fclose(f);
    return NULL ;
}
HttpResponse *oss_http_request_upload(HttpRequest *httprequest, OSSPtr oss,
        const char *path)
{
    CURL *curl;
    CURLcode res;
    HttpResponse *response;
    int fd;
    char *url;

    fd = open(path, O_RDONLY);
    if (fd == -1)
    {
        log_msg(strerror(errno));
        return NULL ;
    }

    oss_http_request_init(httprequest, oss);

    curl = curl_easy_init();

    if (curl)
    {
        struct curl_slist *chunk = NULL;
        struct stat filestat;
        fstat(fd, &filestat);

        url = StringClass.concat(2, oss->host, httprequest->url);
        response = HttpResponseClass.init();
        chunk = curl_slist_append(chunk, "");
        curl_chunk_init(chunk, httprequest);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
        if (strcasecmp(httprequest->method, "put") == 0)
        {
            curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
            curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_file);
            curl_easy_setopt(curl, CURLOPT_READDATA, &fd);
            curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE,
                    (curl_off_t )filestat.st_size);
        }

        if (oss->proxy != NULL )
        {
            curl_easy_setopt(curl, CURLOPT_PROXY, oss->proxy);
        }

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory);
        curl_easy_setopt(curl, CURLOPT_WRITEHEADER, (void * )response->header);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void * )response->body);
        res = curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &(response->code));
        if (res != CURLE_OK)
        {
            strcat((char *) response->body->blk, curl_easy_strerror(res));
        }
        log_debug("%s", response->body->blk);
        close(fd);
        free(url);
        curl_slist_free_all(chunk);
        curl_easy_cleanup(curl);
        return response;
    }

    close(fd);
    return NULL ;
}

