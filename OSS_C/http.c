#include "http.h"

MemBlkOpration MemBlkClass = {
                .init = memblk_init,
                .destroy = memblk_destroy
        };
HttpResponseOpration HttpResponseClass = {
        .init = http_response_init,
        .destroy = http_response_destroy
};
HttpRequestOpration HttpRequestClass = {
        .init = http_request_init,
        .destroy = http_request_destroy
};
MemBlk *memblk_init()
{
	MemBlk *mem = malloc(sizeof(MemBlk));
	if(mem)
	{
		mem->blk = malloc(512);
		memset(mem->blk, 0x0, 512);
		mem->size = 0;
	}
    return mem;
}

void memblk_destroy(MemBlk *mem)
{
	if(mem)
	{
		if(mem->blk)
			free(mem->blk);
		free(mem);
	}
}
HttpResponse *
http_response_init()
{
  HttpResponse *httpresponse = malloc(sizeof(HttpResponse));
  memset(httpresponse, 0x0, sizeof(HttpResponse));
  httpresponse->header = MemBlkClass.init();
  httpresponse->body = MemBlkClass.init();
  return httpresponse;
}

void
http_response_destroy(HttpResponse *httpresponse)
{
  if (httpresponse)
    {
      if (httpresponse->header)
        {
          MemBlkClass.destroy(httpresponse->header);
        }
      if (httpresponse->body)
        {
          MemBlkClass.destroy(httpresponse->body);
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

