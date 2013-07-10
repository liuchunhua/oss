#include "http.h"

HttpResponse *
http_response_init()
{
  HttpResponse *httpresponse = malloc(sizeof(HttpResponse));
  memset(httpresponse, 0x0, sizeof(HttpResponse));
  return httpresponse;
}

void
http_response_destroy(HttpResponse *httpresponse)
{
  if (httpresponse)
    {
      if (httpresponse->header)
        {
          free(httpresponse->header);
        }
      if (httpresponse->body)
        {
          free(httpresponse->body);
        }
      free(httpresponse);
    }
}

HttpRequest *
http_request_init()
{
  HttpRequest *httprequest = malloc(sizeof(HttpRequest));
  memset(httprequest, 0x0, sizeof(HttpRequest));
  return httprequest;
}

void
http_request_destroy(HttpRequest *httprequest)
{
  if (httprequest)
    {
      if (httprequest->method)
        {
          free(httprequest->method);
        }
      if (httprequest->url)
        {
          free(httprequest->url);
        }
      if (httprequest->headers)
        {
          HashTableClass.destroy_fun(httprequest->headers, free);
        }
      free(httprequest);
    }
}

