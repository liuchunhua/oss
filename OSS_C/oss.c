    /*
 * oss.c
 *
 *  Created on: 2012-10-7
 *      Author: lch
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <curl/curl.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#ifdef DEBUG
#include <syslog.h>
#endif


#include "oss.h"
#include "ossutil.h"
#include "String.h"
#include "service.h"
#include "http.h"
#include "oss_http.h"
#include "log.h"


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

static void
free_http_response(struct HttpResponse* response)
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
static char* acl[] =
  { "public-read-write", "public-read", "private" };

static size_t
write_memory(void *buffer, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  MemoryBlock* mem = (MemoryBlock*) userp;

  mem->memory = realloc(mem->memory, mem->size + realsize + 1);
  if (mem->memory == NULL )
    {
      /* out of memory! */
      printf("not enough memory (realloc returned NULL)\n");
      exit(EXIT_FAILURE);
    }

  memcpy(&(mem->memory[mem->size]), buffer, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}
static size_t
write_buf(void *buffer, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  MemoryBlock* mem = (MemoryBlock*) userp;
#ifdef DEBUG
  syslog(LOG_MAKEPRI(LOG_USER,LOG_WARNING), "read %ld bytes\n",(long int)realsize);
#endif
  memcpy(&(mem->memory[mem->size]), buffer, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;
  return realsize;
}
static size_t
write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
  size_t written = fwrite(ptr, size, nmemb, (FILE *) stream);
  return written;
}

static size_t
read_callback(void *ptr, size_t size, size_t nmemb, void *stream)
{
  size_t retcode;
  curl_off_t nread;

  /* in real-world cases, this would probably get this data differently
   as this fread() stuff is exactly what the library already would do
   by default internally */
  retcode = fread(ptr, size, nmemb, stream);

  nread = (curl_off_t) retcode;

  fprintf(stderr, "*** We read %" CURL_FORMAT_CURL_OFF_T
  " bytes from file\n", nread);

  return retcode;
}

static const char*
header(char* buf, char* key, char* value)
{
  memset(buf, 0x0, 200);
  strcat(buf, key);
  strcat(buf, ":");
  strcat(buf, value);
  return buf;
}
static struct HttpResponse*
response_init()
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

/*此方法已放弃，使用oss_http.h*/
static struct HttpResponse*
http_request_old(OSSPtr oss, const char* method, const char* requestresource,
    struct HashTable* headers)
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

      chunk = curl_slist_append(chunk, header(buf, authorhead, authorization));
//		chunk = curl_slist_append(chunk, header(buf, "Date", date));
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
      curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L); curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
     
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
      curl_easy_setopt(curl, CURLOPT_WRITEHEADER, (void*)response->header);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)response->body);
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

static struct HttpResponse*
http_upload(OSSPtr oss, const char* method, const char* requestresource,
    char* file, struct HashTable* headers)
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

      chunk = curl_slist_append(chunk, header(buf, authorhead, authorization));
//		chunk = curl_slist_append(chunk, header(buf, "Date", date));
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
              (curl_off_t)(fileinfo.st_size));
        }
      if (strcasestr(method, "post") != NULL )
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
      if (oss->proxy != NULL )
        {
          curl_easy_setopt(curl, CURLOPT_PROXY, oss->proxy);
        }
      res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)response);
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

static struct HttpResponse*
http_download(OSSPtr oss, const char* method, const char* requestresource,
    size_t content_length, char* dest_file, struct HashTable* headers,
    unsigned short redownnload)
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
      response->code == 200;
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
      chunk = curl_slist_append(chunk, header(buf, authorhead, authorization));
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
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)file);
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

static struct HttpResponse*
http_download_memory(OSSPtr oss, const char* method,
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
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)block);
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

static OSSObject*
oss_GetObject(const char* httpheader)
{
  char *str1, *str2, *token, *subtoken;
  char *savePtr1, *savePtr2;
  OSSObject* object = (OSSObject*) malloc(sizeof(OSSObject));
  memset(object, 0x0, sizeof(object));
  char* content_length;
  size_t content_size;
  for (str1 = httpheader;; str1 = NULL )
    {
      token = strtok_r(str1, "\n", &savePtr1);
      if (token == NULL )
        break;
      int index = indexOf(token, ':');
      if (index == -1)
        continue;
      String* key = substring(token, 0, index - 1);
      String*value = substring(token, index + 1, strlen(token));
      if (strcasecmp(key->str, "Last-Modified") == 0)
        object->mtime = StrGmtToLocaltime(value->str);
      if (strcasecmp(key->str, "Content-Type") == 0)
        object->minetype = strdup(value->str);
      if (strcasecmp(key->str, "Content-Length") == 0)
        object->size = atol(value->str);
      if (strcasecmp(key->str, "ETag") == 0)
        object->etag = strdup(value->str);
      free_string(key);
      free_string(value);
    }
  return object;
}

/*
 * public method
 */

OSSPtr
new_ossptr()
{
  OSSPtr oss = (OSSPtr) malloc(sizeof(OSS));
  if (oss == NULL )
    {
      fprintf(stderr, "%s\n", "OSS malloc failed");
      exit(EXIT_FAILURE);
    }
  return oss;
}

void
free_ossptr(OSSPtr oss)
{
  if (oss->host)
    free(oss->host);
  if (oss->access_id)
    free(oss->access_id);
  if (oss->access_key)
    free(oss->access_key);
  if (oss)
    free(oss);
}

void
free_ossobject(OSSObject* obj)
{
  if (obj)
    {
      free(obj->etag);
      free(obj->minetype);
      free(obj->name);
      free(obj);
    }
}

OSSPtr
oss_init(const char* host, const char* id, const char* key)
{
  OSSPtr oss = new_ossptr();
  oss->host = strdup(host);
  oss->access_id = strdup(id);
  oss->access_key = strdup(key);
  oss->proxy = NULL;
  return oss;
}

List
GetService(OSSPtr oss)
{
  HttpResponse* response;
  HttpRequest *request = HttpRequestClass.init();
  Logger.debug("request init");
  request->method = strdup("GET");
  request->url = strdup("/");
  Logger.debug(request->method);
  Logger.debug("http requst");
  response = OSSHttpClass.request(request, oss);
  if (response && response->code == 200)
    {
      Logger.debug(response->body->blk);
      BucketsResult *result = bucket_result_parse(response->body->blk);
      HttpResponseClass.destroy(response);
      return result->buckets;
    }
  if (response)
    HttpResponseClass.destroy(response);
  HttpRequestClass.destroy(request);
  return NULL ;
}

int
PutBucket(OSSPtr oss, char* bucket)
{
  struct HttpResponse* response;
  char buf[20] =
    { };
  char* method = "PUT";
  char* resource = buf;
  if ('/' != *bucket)
    strcat(buf, "/");
  resource = strcat(buf, bucket);
  response = http_request_old(oss, method, resource, NULL );
  if (response && response->code == 200)
    {
      return EXIT_SUCCESS;
    }
  if (response)
    switch (response->code)
      {
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

int
PutBucketACL(OSSPtr oss, char* bucket, ACL a)
{
  char* method = "PUT";
  char buf[20] =
    { };
  char* resource = buf;
  if ('/' != *bucket)
    strcat(buf, "/");
  resource = strcat(buf, bucket);
  char* permission = acl[a];
  struct HashTable* table = hash_table_init();
  hash_table_put(table, "x-oss-acl", permission);
  struct HttpResponse* response = http_request_old(oss, method, resource, table);

  if (response->code == 200)
    return EXIT_SUCCESS;
  else if (response->code == 403)
    {
      fprintf(stderr, "ERROR:%s\n", "AccessDenied");
      return EXIT_FAILURE;
    }
  else
    {
      fprintf(stderr, "ERROR:%s\n", response->body->memory);
      return EXIT_FAILURE;
    }
  free_http_response(response);
  hash_table_free(table);
}

ListBucketResult*
ListObject(OSSPtr oss,const char* bucket, const char* prefix, unsigned int maxkeys,
    const char* marker, const char* delimiter)
{
  char* method = "GET";
  ListBucketResult* result;
  char buf[256] =
    { };
  List list = listInit();
  char max_keys[5];
  sprintf(max_keys, "%d", maxkeys);
  char* resource = buf;
  if ('/' != *bucket)
    strcat(buf, "/");
  strcat(buf, bucket);
  if (prefix && strlen(prefix) > 0)
    {
      struct pair* p = (struct pair*) malloc(sizeof(struct pair));
      p->key = "prefix";
      p->value = prefix;
      listAdd(list, p);
    }
  if (maxkeys > 0)
    {
      struct pair* p = (struct pair*) malloc(sizeof(struct pair));
      p->key = "max-keys";
      p->value = max_keys;
      listAdd(list, p);
    }
  if (marker && strlen(marker) > 0)
    {
      struct pair* p = (struct pair*) malloc(sizeof(struct pair));
      p->key = "marker";
      p->value = marker;
      listAdd(list, p);
    }
  if (delimiter && strlen(delimiter) > 0)
    {
      struct pair* p = (struct pair*) malloc(sizeof(struct pair));
      p->key = "delimiter";
      p->value = delimiter;
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
  struct HttpResponse* response = http_request_old(oss, method, resource, NULL );
  if (response->code == 200)
    {
      result = oss_ListBucketResult(response->body->memory);
    }
  else
    {
#ifdef DEBUG
      syslog(LOG_MAKEPRI(LOG_USER,LOG_WARNING), "%s\n", response->body->memory);
#endif
    }
  free_http_response(response);
  listFreeObject(list);
  listFree(list);
  return result;
}

ACL
GetBucketACL(OSSPtr oss, char* bucket)
{
  char buf[256] =
    { };
  char* resource = buf;
  if ('/' != *bucket)
    strcat(buf, "/");
  strcat(buf, bucket);
  strcat(buf, "?acl");
  struct HttpResponse* response = http_request_old(oss, "GET", buf, NULL );
  if (response->code == 200)
    {
      M_str result = oss_GetBucketAcl(response->body->memory);
      if (!strcasestr(result, acl[RO]) != NULL )
        {
          free(result);
          return RO;
        }
      if (!strcasestr(result, acl[RW]) != NULL )
        {
          free(result);
          return RW;
        }
      if (!strcasestr(result, acl[PRIVATE]) != NULL )
        {
          free(result);
          return PRIVATE;
        }
    }
  return -1;
}

int
DeleteBucket(OSSPtr oss, char* bucket)
{
  struct HttpResponse* response;
  char buf[20] =
    { };
  char* method = "DELETE";
  char* resource = buf;
  if ('/' != *bucket)
    strcat(buf, "/");
  resource = strcat(buf, bucket);
  response = http_request_old(oss, method, resource, NULL );
  if (response && response->code == 200)
    {
      return EXIT_SUCCESS;
    }
  if (response)
    switch (response->code)
      {
    case 409:
      fprintf(stderr, "%s:%s\n", bucket, "BucketNotEmpty");
      break;
    case 403:
      fprintf(stderr, "%s:%s\n", bucket, "AccessDenied");
      break;
    case 404:
      fprintf(stderr, "%s:%s\n", bucket, "NoSuchBucket");
      break;
    default:
      break;
      }
  free_http_response(response);
  return EXIT_FAILURE;
}
int
PutObject(OSSPtr oss, char* bucket, char* objectname, char* file,
    struct HashTable* table)
{
  struct HttpResponse* response;
  char buf[1024] =
    { };
  char* method = "PUT";
  char* resource = buf;
  if ('/' != *bucket)
    strcat(buf, "/");
  strcat(buf, bucket);
  if ('/' != *objectname)
    strcat(buf, "/");
  strcat(buf, objectname);
  response = http_upload(oss, method, resource, file, table);
  if (response && response->code == 200)
    {
      return EXIT_SUCCESS;
    }
  if (response)
    switch (response->code)
      {
    case 411:
      fprintf(stderr, "%s:%s\n", bucket, "MissingContentLength");
      break;
    case 400:
      fprintf(stderr, "%s:%s\n", bucket, "RequestTimeout");
      break;
    case 404:
      fprintf(stderr, "%s:%s\n", bucket, "NoSuchBucket");
      break;
      }
  free_http_response(response);
  return EXIT_FAILURE;
}

size_t
GetObject(OSSPtr oss, char* object, char* desfile, struct HashTable* table,
    unsigned short redownnload)
{
  struct HttpResponse* response;
  char* method = "GET";
  size_t content_size;
  MemoryBlock* header = HeadObject(oss, object);
  content_size = oss_GetObjectSize(header->memory);
  free(header->memory);
  free(header);
  response = http_download(oss, method, object, content_size, desfile, table,
      redownnload);
  free_http_response(response);
  return content_size;
}
size_t
GetObjectIntoMemory(OSSPtr oss, const char* object, char* buf, size_t size,
    off_t offset, struct HashTable* table)
{
  struct HttpResponse* response;
  char* method = "GET";
  response = http_download_memory(oss, method, object, buf, size, offset,
      NULL );
  free_http_response(response);
  return size;
}
OSSObject*
HeadObject(OSSPtr oss, const char* object)
{
  struct HttpResponse* response;
  char* method = "HEAD";
  response = http_request_old(oss, method, object, NULL );
  OSSObject* ossobject;
  if (response && response->header && response->code == 200)
    {
      ossobject = oss_GetObject(response->header->memory);
      ossobject->name = strdup(object);
    }
  else
    {
#ifdef DEBUG
      syslog(LOG_MAKEPRI(LOG_USER,LOG_WARNING), "response code:%d",response&&response->code);
#endif
      free_http_response(response);
      return NULL ;
    }
  free_http_response(response);
  return ossobject;
}

int
CopyObject(OSSPtr oss, char* source, char* des)
{
  char* method = "PUT";
  struct HashTable* table = hash_table_init();
  hash_table_put(table, "x-oss-copy-source", source);
  struct HttpResponse* response = http_request_old(oss, method, des, table);
  if (response->code == 200)
    return EXIT_SUCCESS;
  else
    {
      fprintf(stderr, "%s\n", response->header->memory);
      return EXIT_FAILURE;
    }
}
int
DeleteObject(OSSPtr oss, char* object)
{
  struct HttpResponse* response;
  char buf[20] =
    { };
  char* method = "DELETE";
  char* resource = buf;
  if ('/' != *object)
    strcat(buf, "/");
  resource = strcat(buf, object);
  response = http_request_old(oss, method, resource, NULL );
  if (response && response->code == 200)
    {
      return EXIT_SUCCESS;
    }
  if (response)
    switch (response->code)
      {
    case 204:
      fprintf(stderr, "%s:%s\n", object, "No Content");
      break;
    case 404:
      fprintf(stderr, "%s:%s\n", object, "NoSuchBucket");
      break;
    default:
      break;
      }
  free_http_response(response);
  return EXIT_FAILURE;

}
