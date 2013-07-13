#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <curl/curl.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>

#include "oss_http.h"
#include "authen.h"
#include "List.h"
#include "log.h"
#include "String.h"
#include "HashTable.h"

static size_t
write_memory(void *buffer, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  MemBlk* mem = userp;

  mem->blk = realloc(mem->blk, mem->size + realsize + 1);
  if (mem->blk == NULL )
    {
      /* out of blk! */
      log_msg("not enough blk (realloc returned NULL)\n");
      exit(EXIT_FAILURE);
    }

  memcpy(&(mem->blk[mem->size]), buffer, realsize);
  mem->size += realsize;
  mem->blk[mem->size] = 0;

  return realsize;
}

static size_t
write_file(void *ptr, size_t size, size_t nmemb, void *stream)
{
  size_t writted = fwrite(ptr, size, nmemb, (FILE *) stream);
  return writted;
}
static size_t
read_file(void *ptr, size_t size, size_t nmemb, void *stream)
{
  int fd = *((int *) stream);
  size_t count = read(fd, ptr, size * nmemb);
  return (curl_off_t) count;
}
static void
oss_http_request_init(HttpRequest *httprequest, OSSPtr oss)
{
  const char *url = httprequest->url;
  const char *method = httprequest->url;
  char *date = localtime_gmt();

  if (httprequest->headers == NULL )
    {
      httprequest->headers = HashTableClass.init();
    }

  HashTableClass.put(httprequest->headers, "Date", date);

  char *key = oss_authorizate(oss->access_key, method, httprequest->headers,
      url);
  char *auth = StringClass.concat(4, "OSS ", oss->access_id, ":", key);
  HashTableClass.put(httprequest->headers, "Authorization", auth);
  free(key);
}
static void
curl_chunk_init(struct curl_slist *chunk, HttpRequest * request)
{
  List list = HashTableClass.get_all(request->headers);
  List node;
  for_each(node, list)
    {
      struct pair *p = node->ptr;
      curl_slist_append(chunk,
          StringClass.concat(3, (char *) p->key, ":", (char *) p->value));
    }
  ListClass.destroy(list);
}
HttpResponse *
oss_http_request(HttpRequest *httprequest, OSSPtr oss)
{
  CURL *curl;
  CURLcode res;
  HttpResponse *response;
  char *url;

  oss_http_request_init(httprequest, oss);

  curl = curl_easy_init();

  if (curl)
    {
      char *method = httprequest->method;
      struct curl_slist *chunk = NULL;

      curl_chunk_init(chunk, httprequest);

      url = StringClass.concat(2, oss->host, httprequest->url);
      curl_easy_setopt(curl, CURLOPT_URL, url);
      curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
      curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);

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
          curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
        }
      if (oss->proxy != NULL )
        {
          curl_easy_setopt(curl, CURLOPT_PROXY, oss->proxy);
        }
      res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
      response = HttpResponseClass.init();

      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory);
      curl_easy_setopt(curl, CURLOPT_WRITEHEADER, (void * )response->header);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void * )response->body);

      res = curl_easy_perform(curl);
      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &(response->code));
      if (res != CURLE_OK)
        {
          strcat((char *) response->body->blk, curl_easy_strerror(res));
        }

      free(url);
      curl_slist_free_all(chunk);
      curl_easy_cleanup(curl);
      return response;
    }
  return NULL ;
}

HttpResponse *
oss_http_request_download(HttpRequest *httprequest, OSSPtr oss, char *path)
{
  CURL *curl;
  CURLcode res;
  HttpResponse *response;
  char *url;

  FILE *f = fopen(path, "w");
  if (!f)
    {
      log_msg(strerror(errno));
      return NULL ;
    }

  oss_http_request_init(httprequest, oss);

  curl = curl_easy_init();
  if (curl)
    {
      url = StringClass.concat(2, oss->host, httprequest->url);
      struct curl_slist *chunk = NULL;

      curl_chunk_init(chunk, httprequest);

      curl_easy_setopt(curl, CURLOPT_URL, url);
      curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_file);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void * )f);
      if (oss->proxy != NULL )
        {
          curl_easy_setopt(curl, CURLOPT_PROXY, oss->proxy);
        }
      res = curl_easy_perform(curl);
      response = HttpResponseClass.init();
      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &(response->code));
      if (res != CURLE_OK)
        {
          strcat((char *) response->body->blk, curl_easy_strerror(res));
        }
      free(url);
      fclose(f);
      curl_slist_free_all(chunk);
      curl_easy_cleanup(curl);
      return response;
    }
  fclose(f);
  return NULL ;
}
HttpResponse *
oss_http_request_upload(HttpRequest *httprequest, OSSPtr oss, char *path)
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
      curl_chunk_init(chunk, httprequest);

      curl_easy_setopt(curl, CURLOPT_URL, url);
      curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

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

      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory);
      curl_easy_setopt(curl, CURLOPT_WRITEHEADER, (void * )response->header);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void * )response->body);
      res = curl_easy_perform(curl);
      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &(response->code));
      if (res != CURLE_OK)
        {
          strcat((char *) response->body->blk, curl_easy_strerror(res));
        }
      close(fd);
      free(url);
      curl_slist_free_all(chunk);
      curl_easy_cleanup(curl);
      return response;
    }

  close(fd);
  return NULL ;
}
