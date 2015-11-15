/*
 * test.c
 *
 *  Created on: 2012-9-19
 *      Author: lch
 */

#include "base64.h"
#include "ossutil.h"
#include "HashTable.h"
#include "oss.h"
#include "String.h"
#include "log.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <curl/curl.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <assert.h>

struct MemoryStruct
{
    char *memory;
    size_t size;
};
/*
static const char* buckets =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\
        <ListAllMyBucketsResult>\
        <Owner>\
        <ID>1721545645056998</ID>\
        <DisplayName>1721545645056998</DisplayName>\
        </Owner>\
        <Buckets>\
        <Bucket>\
        <Name>welcome2myspace</Name>\
        <CreationDate>2012-08-22T14:32:25.000Z</CreationDate>\
        </Bucket>\
        <Bucket>\
        <Name>liuchunhua</Name>\
        <CreationDate>2012-08-22T14:33:34.000Z</CreationDate>\
        </Bucket>\
        </Buckets>\
        </ListAllMyBucketsResult>";
        */
/*
   void
   test_base64_encode(const char*);
   void
   test_localtime_gmt();
   void
   test_strtotime();
   void
   test_hmac_base64();
   void
   test_curl();

   void
   test_bucket_xmls();

   void
   test_bucket_xmls2(char*);
 */
void
test_get_service();
void
test_put_bucket();

void
test_GetBucketACL();
void
test_listobject();

void 
test_DeleteObject();

void test_HeadObject();

void
test_PutObject();

void
test_GetObject();
void
test_CopyObject();

void
test_delete_bucket();

void 
test_put_bucket_acl();

    int
main()
{
    log_open();
    //	test_base64_encode("��䦦�µm�T*Jl4�");
    //	test_localtime_gmt();
    //	test_hmac_base64();
    //	test_curl();
    //	test_bucket_xmls();
    //	test_get_service();
    //	test_put_bucket();
    //  test_listobject();
    //	test_GetBucketACL();
//    	test_PutObject();
        test_GetObject();
    //  test_put_bucket_acl();
    //	test_HeadObject();
    //  test_CopyObject();
    //  test_DeleteObject();
    //  test_strtotime();
    //    test_delete_bucket();

    log_close();
    return 0;
}
/*
   void
   test_base64_encode(const char* string)
   {
   printf("%s\n", base64_encode(string, strlen(string)));
   }

   void
   test_localtime_gmt()
   {
   printf("%s\n", localtime_gmt());
   }
   void
   test_strtotime(){
   char* s = "Wed, 05 Dec 2012 14:26:10 GMT";
   char* s1 = "2012-09-28T14:34:25.000Z";
   printf("orign time : %s\n",s);
   time_t t = StrGmtToLocaltime(s);
//  printf("long time: %ld",t);
printf("%s\n",ctime(&t));
time(&t);
printf("%s\n",ctime(&t));
gmtime(&t);
printf("%s\n",asctime(gmtime(&t)));
t = GmtToLocaltime(s1);
printf("s2 %s\n",ctime(&t));
}
void
test_hmac_base64()
{
//	char* origin = "good night， liuchunhua";
//	char* key = "helloword";
//	M_str s =  hmac_base64(origin,strlen(origin),key,strlen(key));
//	printf("%s\n",s);
//	free((void*)s);
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

static size_t
write_data(void *buffer, size_t size, size_t nmemb, void *userp)
{
size_t realsize = size * nmemb;
struct MemoryStruct *mem = (struct MemoryStruct *) userp;

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
<?xml version="1.0" encoding="UTF-8"?>
<ListAllMyBucketsResult>
<Owner>
<ID>1721545645056998</ID>
<DisplayName>1721545645056998</DisplayName>
</Owner>
<Buckets>
<Bucket>
<Name>welcome2myspace</Name>
<CreationDate>2012-08-22T14:32:25.000Z</CreationDate>
</Bucket>
<Bucket>
<Name>liuchunhua</Name>
<CreationDate>2012-08-22T14:33:34.000Z</CreationDate>
</Bucket>
</Buckets>
</ListAllMyBucketsResult>
*/
/*
   void
   test_curl()
   {
   CURL *curl;
   CURLcode res;
//const char* ACCESS_ID = "abysmn89uz488l1dfycon3qa";
char* ACCESS_KEY = "qfEZ+LNuGJUP/FlRw1R3aKpwiwY=";
char* method = "GET";
struct MemoryStruct content;
content.memory = malloc(1);
content.size = 0;
M_str date = localtime_gmt();
char* host = "oss.aliyuncs.com";
char* content_type = "text/html";
char buf[200];
struct HashTable* headers = hash_table_init();
hash_table_put(headers, "Content-Type", content_type);
hash_table_put(headers, "Date", date);
M_str authorization = oss_authorizate(ACCESS_KEY, method, headers, "/");
curl = curl_easy_init();
if (curl)
{
struct curl_slist *chunk = NULL;

chunk = curl_slist_append(chunk,
header(buf, "Authorization:OSS abysmn89uz488l1dfycon3qa",
authorization));
chunk = curl_slist_append(chunk,
header(buf, "Content-Type", content_type));
chunk = curl_slist_append(chunk, header(buf, "Date", date));

curl_easy_setopt(curl, CURLOPT_URL, host);
curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&content);
res = curl_easy_perform(curl);

if (res != CURLE_OK)
{
printf("ERROR:%s\n", curl_easy_strerror(res));
}
else
{
test_bucket_xmls2(content.memory);
fprintf(stderr, "%d\n", content.size);
}
free(content.memory);
free(date);
free(authorization);
curl_slist_free_all(chunk);
curl_easy_cleanup(curl);
}
}

struct Owner*
getOwner(xmlNodePtr node)
{
if (!strcmp((char*) node->name, "Owner"))
{
xmlNodePtr cur = node->children;
struct Owner* owner = (struct Owner*) malloc(sizeof(struct Owner));
assert(owner!=NULL);
while (cur)
{
if (cur->type == XML_ELEMENT_NODE && !strcmp((char*) cur->name, "ID"))
{
owner->id = (char*) xmlNodeGetContent(cur);
}
if (cur->type == XML_ELEMENT_NODE
        && !strcmp((char*) cur->name, "DisplayName"))
{
    owner->displayName = (char*) xmlNodeGetContent(cur);
}
cur = cur->next;
}
return owner;
}
return NULL ;
}
    struct Bucket*
getBucket(xmlNodePtr node)
{
    if (!strcmp((char*) node->name, "Bucket"))
    {
        xmlNodePtr cur = node->children;
        struct Bucket* bucket = (struct Bucket*) malloc(sizeof(struct Bucket));
        assert(bucket!=NULL);
        while (cur)
        {
            if (cur->type == XML_ELEMENT_NODE
                    && !strcmp((char*) cur->name, "Name"))
            {
                bucket->name = (char*) xmlNodeGetContent(cur);
            }
            if (cur->type == XML_ELEMENT_NODE
                    && !strcmp((char*) cur->name, "CreationDate"))
            {
                bucket->creationDate = (char*) xmlNodeGetContent(cur);
            }
            cur = cur->next;
        }
        return bucket;
    }
    return NULL ;
}
    List
getListBucket(xmlNodePtr node)
{
    if (!strcmp((char*) node->name, "Buckets"))
    {
        xmlNodePtr cur = node->children;
        List list = listInit();
        while (cur)
        {
            if (cur->type == XML_ELEMENT_NODE
                    && !strcmp((char*) cur->name, "Bucket"))
            {
                struct Bucket* bucket = getBucket(cur);
                if (bucket)
                    listAdd(list, bucket);
            }
            cur = cur->next;
        }
        return list;
    }
    return NULL ;
}
    void
test_bucket_xmls()
{
    xmlDocPtr doc;
    xmlNodePtr root;
    xmlNodePtr cur;
    doc = xmlReadMemory(buckets, strlen(buckets), "noname.xml", NULL, 0);
    if (doc == NULL )
    {
        fprintf(stderr, "Failed to parse document\n");
        return;
    }
    root = xmlDocGetRootElement(doc);
    cur = root->children;
    while (cur != NULL )
    {
        fprintf(stderr, "%s\n", cur->name);
        if (!strcmp((char*) cur->name, "Owner"))
        {
            struct Owner* owner = getOwner(cur);
            fprintf(stderr, "ID:%s\n", owner->id);
            fprintf(stderr, "DisplayName:%s\n", owner->displayName);
            xmlFree(owner->id);
            xmlFree(owner->displayName);
            free(owner);
        }
        if (strcmp((char*) cur->name, "Buckets") == 0)
        {
            List list = getListBucket(cur);
            List element = list->next;
            while (element != list)
            {
                struct Bucket* bucket = element->ptr;
                fprintf(stderr, "name:%s\n", bucket->name);
                fprintf(stderr, "date:%s\n", bucket->creationDate);
                element = element->next;
            }
        }
        cur = cur->next;
    }
    xmlFreeDoc(doc);
    return;
}
    void
test_bucket_xmls2(char* content)
{
    struct Owner* owner = (struct Owner*) malloc(sizeof(struct Owner));
    List list = oss_ListAllMyBucketsResult(buckets, owner);
    List node;
    printf(owner->id);
    free(owner->id);
    free(owner->displayName);
    free(owner);
    for_each(node,list)
    {
        struct Bucket* bucket = (struct Bucket*) node->ptr;
        fprintf(stderr, "%s\n", bucket->name);
        free(bucket->name);
        free(bucket->creationDate);
        free(bucket);
    }
    listFree(list);
}
*/
    void
test_get_service()
{
    OSSPtr oss = OSSClass.init("oss.aliyuncs.com", "abysmn89uz488l1dfycon3qa",
            "qfEZ+LNuGJUP/FlRw1R3aKpwiwY=");
    //  oss->proxy = "192.168.0.142:808";
    List list = GetService(oss);
    List node;
    for_each(node,list)
    {
        struct Bucket* bucket = (struct Bucket*) node->ptr;
        fprintf(stderr, "%s\n", bucket->name);
    }
    ListClass.destroy_fun(list, (void (*)(void *))free_Bucket);
    OSSClass.destroy(oss);
}
    void
test_put_bucket()
{
    char* bucket = "code";
    OSSPtr oss = OSSClass.init("oss.aliyuncs.com", "abysmn89uz488l1dfycon3qa",
            "qfEZ+LNuGJUP/FlRw1R3aKpwiwY=");
    int result = PutBucket(oss,bucket);
    //int result = PutBucketACL(oss, bucket, PRIVATE);
    printf("%d\n", result);
    OSSClass.destroy(oss);
}
    void
test_put_bucket_acl()
{
    char* bucket = "code";
    OSSPtr oss = OSSClass.init("oss.aliyuncs.com", "abysmn89uz488l1dfycon3qa",
            "qfEZ+LNuGJUP/FlRw1R3aKpwiwY=");
    int result = PutBucketACL(oss, bucket, RO);
    printf("%d\n", result);
    OSSClass.destroy(oss);
}
    void
test_listobject()
{
    char* bucket = "liuchunhua";
    OSSPtr oss = OSSClass.init("oss.aliyuncs.com", "abysmn89uz488l1dfycon3qa",
            "qfEZ+LNuGJUP/FlRw1R3aKpwiwY=");
    ListBucketResult* result = ListObject(oss, bucket, NULL, 0, NULL, "/" );
    if (result)
    {
        List node;
        for_each(node,result->contents)
        {
            Contents* con = (Contents*) node->ptr;
            fprintf(stderr, "%s\n", con->key);
        }
    }
    free_ListBucketResult(result);
    OSSClass.destroy(oss);
}


   void
   test_PutObject()
   {
   char* bucket = "liuchunhua";
   OSSPtr oss = OSSClass.init("oss.aliyuncs.com", "abysmn89uz488l1dfycon3qa",
   "qfEZ+LNuGJUP/FlRw1R3aKpwiwY=");
   char* file = "/home/liuchunhua/me.jpg";
   PutObject(oss, bucket, "/myface.jpg", file);
   OSSClass.destroy(oss);
   }

   void
   test_GetObject()
   {
   OSSPtr oss = oss_init("oss.aliyuncs.com", "abysmn89uz488l1dfycon3qa",
   "qfEZ+LNuGJUP/FlRw1R3aKpwiwY=");
   char* file = "/home/lch/oss_test.txt";
   GetObject(oss, "welcome2myspace", "/oss_test.txt", file);
   }

   void
   test_HeadObject()
   {
   OSSPtr oss = OSSClass.init("oss.aliyuncs.com", "abysmn89uz488l1dfycon3qa",
   "qfEZ+LNuGJUP/FlRw1R3aKpwiwY=");
   OSSObject* obj = HeadObject(oss, "welcome2myspace", "/oss/oss_test.txt");
   if(obj){
   printf("name:%s,content-type:%s,content-length:%ld",obj->name,obj->minetype,obj->size);
   }
   else{
   printf("can't found");
   }

   OSSClass.destroy(oss);
   OSSObjectClass.destroy(obj);
   }


void
test_CopyObject(){
    OSSPtr oss = OSSClass.init("oss.aliyuncs.com", "abysmn89uz488l1dfycon3qa",
            "qfEZ+LNuGJUP/FlRw1R3aKpwiwY=");
    char* source = "/test.py";
    char* des = "/liuchunhua/test1.py";

    CopyObject(oss,"welcome2myspace",source,des);
    
    OSSClass.destroy(oss);
}

    void
test_DeleteObject()
{
    OSSPtr oss = OSSClass.init("oss.aliyuncs.com", "abysmn89uz488l1dfycon3qa",
            "qfEZ+LNuGJUP/FlRw1R3aKpwiwY=");
    DeleteObject(oss, "liuchunhua", "/test1.py");
    OSSClass.destroy(oss);
}
    void
test_delete_bucket()
{
    OSSPtr oss = OSSClass.init("oss.aliyuncs.com", "abysmn89uz488l1dfycon3qa","qfEZ+LNuGJUP/FlRw1R3aKpwiwY=");
    int result = BucketClass.delete(oss, "test");
    printf("%d\n", result);
    OSSClass.destroy(oss);
}

    void
test_GetBucketACL()
{
    static char* acl[] =
    { "public-read-write", "public-read", "private" };
    char* bucket = "welcome2myspace";
    OSSPtr oss = oss_init("oss.aliyuncs.com", "abysmn89uz488l1dfycon3qa",
            "qfEZ+LNuGJUP/FlRw1R3aKpwiwY=");
    ACL a = GetBucketACL(oss, bucket);
    fprintf(stderr, "%s\n", acl[a]);
}
