/*
 *ossutil.c
 *
 * Created on: 2012-9-22
 *     Author: lch
 */

#include "ossutil.h"
#include "base64.h"
#include "HashTable.h"
#include "List.h"
#include "String.h"
#include "service.h"

#include <time.h>
#include <string.h>
#include <openssl/hmac.h>
#include <assert.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>


M_str
localtime_gmt()
{
  time_t cur_time;
  memset(oss_buf, 0x0, sizeof(oss_buf));
  time(&cur_time);
  struct tm *tm_time = gmtime(&cur_time);
  strftime(oss_buf, OSS_LEN, OSS_TIME_FORMAT, tm_time);
  return strdup(oss_buf);
}
time_t
StrGmtToLocaltime(const char *s)
{
  int i = 0;
  char week[4] =
    { };
  char month[4] =
    { };
  char *weeks[7] =
    { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
  char *monthes[12] =
    { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct",
        "Nov", "Dec" };
  struct tm t;
  memset(&t, 0x0, sizeof(struct tm));
  //Wed, 05 Dec 2012 14:26:10 GMT
  sscanf(s, "%[^,],%d %s %d %d:%d:%d GMT", week, &(t.tm_mday), month,
      &(t.tm_year), &(t.tm_hour), &(t.tm_min), &(t.tm_sec));
  t.tm_year -= 1900;
  for (i = 0; i < 7; i++)
    {
      if (strcmp(week, weeks[i]) == 0)
        break;
    }
  if (i != 7)
    t.tm_wday = i;
  for (i = 0; i < 12; i++)
    {
      if (strcmp(month, monthes[i]) == 0)
        break;
    }
  if (i != 12)
    t.tm_mon = i;
  return mktime(&t) - timezone;
}
/*
 *2012-09-28T14:34:25.000Z
 */
time_t
GmtToLocaltime(const char *s)
{
  struct tm t;
  sscanf(s, "%d-%d-%dT%d:%d:%d.000Z", &t.tm_year, &t.tm_mon, &t.tm_mday,
      &t.tm_hour, &t.tm_min, &t.tm_sec);
  t.tm_year -= 1900;
  t.tm_mon -= 1;
  return mktime(&t) - timezone;
}
/*
 *@description: 加密，编码
 *@param:	string 需要加密的字符串
 *@param:	len string的长度
 *@param:	key 密钥
 *@param:	key_len 密钥长度
 *@return:	加密后字符串
 */

char *
hmac_base64(const char *string, size_t len, const char *key, int key_len)
{
//	HMAC_CTX ctx;
  unsigned int out_len = 0;
  memset(oss_buf, 0x0, OSS_LEN);
//	HMAC_CTX_init(&ctx);
//	HMAC_Init(&ctx,key,key_len,EVP_sha1());
//	HMAC_Update(&ctx,string,len);
//	HMAC_Final(&ctx,oss_buf,&out_len);
//	HMAC_CTX_cleanup(&ctx);
  HMAC(EVP_sha1(), key, key_len, (const unsigned char *)string, len, ( unsigned char *)oss_buf, &out_len);
  return strdup(base64_encode(oss_buf, out_len));

}

static Owner *
getOwner(xmlNodePtr node)
{
  if (!strcmp((char*) node->name, "Owner"))
    {
      xmlNodePtr cur = node->children;
      Owner *owner = OwnerClass.init();
      assert(owner!=NULL);
      while (cur)
        {
          GetNodeValue(cur, owner, id);
          GetNodeValue(cur, owner, displayName);
          cur = cur->next;
        }
      return owner;
    }
  return NULL ;
}

static Contents*
getContents(xmlNodePtr node)
{
  if (!strcmp((char*) node->name, "Contents"))
    {
      xmlNodePtr cur = node->children;
      Contents *contents = (Contents*) malloc(sizeof(Contents));
      memset(contents, 0x0, sizeof(Contents));
      while (cur)
        {
          GetNodeValue(cur, contents, key);
          GetNodeValue(cur, contents, lastmodified);
          GetNodeValue(cur, contents, etag);
          GetNodeValue(cur, contents, type);
          GetNodeValue(cur, contents, size);
          GetNodeValue(cur, contents, storageclass);
          if (cur->type == XML_ELEMENT_NODE
              && !strcasecmp((char*) node->name, "Owner"))
            contents->owner = getOwner(cur);
          cur = cur->next;
        }
      return contents;
    }
  return NULL ;
}
static List
getCommonPrefixes(xmlNodePtr node)
{
  List list = listInit();
  if (!strcmp((char*) node->name, "CommonPrefixes"))
    {
      xmlNodePtr cur = node->children;

      while (cur)
        {
          if (cur->type == XML_ELEMENT_NODE)
            listAdd(list, xmlNodeGetContent(cur));
          cur = cur->next;
        }
    }
  return list;
}
static ListBucketResult*
getListBucketResult(xmlNodePtr node)
{
  if (!strcmp((char*) node->name, "ListBucketResult"))
    {
      xmlNodePtr cur = node->children;
      ListBucketResult *lbr = (ListBucketResult*) malloc(
          sizeof(ListBucketResult));
      memset(lbr, 0x0, sizeof(ListBucketResult));
      List con_list = listInit();
      List pre_list = listInit();
      while (cur)
        {
          if (cur->type == XML_TEXT_NODE)
            {
              cur = cur->next;
              continue;
            }
          GetNodeValue(cur, lbr, name);
          GetNodeValue(cur, lbr, prefix);
          GetNodeValue(cur, lbr, marker);
          GetNodeValue(cur, lbr, maxkeys);
          GetNodeValue(cur, lbr, nextMarker);
          GetNodeValue(cur, lbr, delimiter);
          GetNodeValue(cur, lbr, istruncated);
          if (cur->type == XML_ELEMENT_NODE
              && !strcasecmp((char*) cur->name, "Contents"))
            listAdd(con_list, getContents(cur));
          if (cur->type == XML_ELEMENT_NODE
              && !strcasecmp((char*) cur->name, "CommonPrefixes"))
            lbr->commonprefixes = getCommonPrefixes(cur);
          cur = cur->next;
        }
      lbr->contents = con_list;
      if (!lbr->commonprefixes)
        lbr->commonprefixes = pre_list;
      else
        listFree(pre_list);
      return lbr;
    }
  return NULL ;
}

//按字母顺序升序排列,合并相同
static void
sort_list_asc(List ls)
{
  List last = ls;
  while (ls->next != last)
    {
      List node = ls->next;
      List next = node->next;
      while (next != last)
        {
          struct pair *a = (struct pair*) node->ptr;
          struct pair *b = (struct pair*) next->ptr;
          char *a_key = (char*) a->key;
          char *b_key = (char*) b->key;
          if (strcoll(a_key, b_key) > 0)
            {
              node->prev->next = node->next;
              node->next = next->next;
              next->prev = node->prev;
              node->prev = next;
              next->next = node;
              next = node->next;
            }
          if (strcoll(a_key, b_key) == 0)
            {
              int size = strlen((char*) a->value) + strlen((char*) b->value)
                  + 2;
              char *new_string = malloc(size);
              strcat(new_string, (char*) a->value);
              strcat(new_string, ",");
              strcat(new_string, (char*) b->value);
              free(a->value);
              a->value = (void*) new_string;
              free(b->key);
              free(b->value);
              free(b);
              listDel(next);
              next = node->next;
            }
          else
            {
              node = next;
              next = next->next;
            }
        }
      last = last->prev;
    }
}

static M_str
oss_authorizate(const char *key, const char *method, struct HashTable *headers,
    const char *resource)
{
  char *date = (char*) hash_table_get(headers, "Date");
  char *content_md5 = (char*) hash_table_get(headers, "Content-Md5");
  char *content_type = (char*) hash_table_get(headers, "Content-Type");
  int n = strcspn(resource, "?");
  char *res = strndup(resource, n);
  if (res && strlen(res) > 0 && strcasestr(resource, "acl") == NULL )
    resource = res;
  assert(resource!=NULL);
  assert(date!=NULL);
  char buf[200] =
    { };
  strcat(buf, method); //method
  strcat(buf, "\n");
  if (content_md5 != NULL )
    strcat(buf, content_md5);
  strcat(buf, "\n"); //content-md5
  if (content_type != NULL )
    strcat(buf, content_type);
  strcat(buf, "\n");
  strcat(buf, date);
  strcat(buf, "\n");
  List list = hash_table_get_all(headers);
  sort_list_asc(list);
  List node;
  for_each(node,list)
    {
      struct pair *p = (struct pair*) node->ptr;
      if (strcasestr((char*) (p->key), "x-oss-") != NULL
      // override 查询字符
      || strcasestr((char*) (p->key), "response") != NULL )
        {
          strcat(buf, (char*) (p->key));
          strcat(buf, ":");
          strcat(buf, (char*) (p->value));
          strcat(buf, "\n");
        }
    }
  listFree(list);
  strcat(buf, resource);
  char *code = hmac_base64(buf, strlen(buf), key, strlen(key));
  free(res);
  return code;
}

List
oss_ListAllMyBucketsResult(const char *xml, struct Owner *owner)
{
  xmlDocPtr doc;
  xmlNodePtr root;
  xmlNodePtr cur;
  List list;
  doc = xmlReadMemory(xml, strlen(xml), "noname.xml", NULL, 0);
  assert(doc!=NULL);
  root = xmlDocGetRootElement(doc);
  cur = root->children;
  while (cur != NULL )
    {
      if (!strcmp((char*) cur->name, "Owner") && owner != NULL )
        {
          struct Owner *temp = getOwner(cur);
          owner->id = temp->id;
          owner->displayName = temp->displayName;
          free(temp);
        }
      if (strcmp((char*) cur->name, "Buckets") == 0)
        {
//          list = getListBucket(cur);
        }
      cur = cur->next;
    }
  xmlFreeDoc(doc);
  return list;
}

ListBucketResult*
oss_ListBucketResult(const char *xml)
{
  xmlDocPtr doc;
  xmlNodePtr root;

  doc = xmlReadMemory(xml, strlen(xml), "noname.xml", NULL, 0);
  assert(doc!=NULL);
  root = xmlDocGetRootElement(doc);
  ListBucketResult *result = getListBucketResult(root);
  xmlFreeDoc(doc);
  return result;
}

M_str
oss_GetBucketAcl(const char *xml)
{
  xmlDocPtr doc;
  xmlNodePtr root;
  M_str result;
  doc = xmlReadMemory(xml, strlen(xml), "noname.xml", NULL, 0);
  assert(doc!=NULL);
  root = xmlDocGetRootElement(doc);
  xmlNodePtr cur = root->children;
  while (cur)
    {
      if (cur->type == XML_ELEMENT_NODE
          && strcasecmp(cur->name, "AccessControlList") == 0)
        {
          result = strdup(xmlNodeGetContent(cur));
        }
      cur = cur->next;
    }
  xmlFreeDoc(doc);
  return result;
}
void
free_Contents(Contents *content)
{
  xmlFree(content->etag);
  xmlFree(content->key);
  xmlFree(content->lastmodified);
  xmlFree(content->size);
  xmlFree(content->storageclass);
  xmlFree(content->type);
  if (content->owner)
    free_Owner(content->owner);
}

void
free_ListBucketResult(ListBucketResult *result)
{
  xmlFree(result->istruncated);
  xmlFree(result->delimiter);
  xmlFree(result->marker);
  xmlFree(result->maxkeys);
  xmlFree(result->name);
  xmlFree(result->nextMarker);
  xmlFree(result->prefix);
  if (result->commonprefixes)
    {
      listFreeObject(result->commonprefixes);
      listFree(result->commonprefixes);
    }
  if (result->contents)
    {
      listFreeObjectByFun(result->contents, free_Contents);
      listFree(result->contents);
    }
}

void
free_Bucket(struct Bucket *bucket)
{
  xmlFree(bucket->name);
  xmlFree(bucket->creationDate);
  free(bucket);
}
void
free_Owner(struct Owner *owner)
{
  xmlFree(owner->id);
  xmlFree(owner->displayName);
  free(owner);
}

size_t
oss_GetObjectSize(const char *httpheader)
{
  char *str1, *str2, *token, *subtoken;
  char *savePtr1, *savePtr2;
  char *content_length;
  size_t content_size;
  for (str1 = httpheader;; str1 = NULL )
    {
      token = strtok_r(str1, "\n", &savePtr1);
      if (token == NULL )
        break;
      if (strcasestr(token, "content-length") == NULL )
        continue;
      for (str2 = token;; str2 = NULL )
        {
          subtoken = strtok_r(str2, ":", &savePtr2);
          if (subtoken == NULL )
            break;
          content_length = subtoken;
        }
    }
  content_size = atol(content_length);
  return content_size;
}
