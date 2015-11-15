/*

 */

#include <stdlib.h>
#include <string.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <assert.h>

#include "service.h"
#include "List.h"
#include "ossutil.h"

#define OWNER "Owner"
#define BUCKETS "Buckets"

BucketsResultOpration BucketsResultClass =
  { .init = bucket_result_init, .parse = bucket_result_parse, .destroy =
      bucket_result_destroy };


OwnerOpration OwnerClass =
  { .init = owner_init, .destroy = owner_destroy };

ListBucketResultOperation ListBucketResultClass = 
{   .destroy = free_ListBucketResult };
Bucket *bucket_init()
{
  Bucket *bucket = malloc(sizeof(Bucket));
  memset(bucket, 0x0, sizeof(Bucket));
  return bucket;
}

void bucket_destroy(Bucket *bucket)
{
  if (bucket->name)
    free(bucket->name);
  if (bucket->creationDate)
    free(bucket->creationDate);
  free(bucket);
}

Owner *owner_init()
{
  Owner *owner = malloc(sizeof(Owner));
  memset(owner, 0x0, sizeof(Owner));
  return owner;
}

void owner_destroy(Owner *owner)
{
  if (owner->id)
    free(owner->id);
  if (owner->displayName)
    free(owner->displayName);
  free(owner);
}

static Owner *getOwner(xmlNodePtr node)
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

static struct Bucket *getBucket(xmlNodePtr node)
{
  if (!strcmp((char*) node->name, "Bucket"))
    {
      xmlNodePtr cur = node->children;
      struct Bucket *bucket = BucketClass.init();
      assert(bucket!=NULL);
      while (cur)
        {
          GetNodeValue(cur, bucket, name);
          GetNodeValue(cur, bucket, creationDate);
          cur = cur->next;
        }
      return bucket;
    }
  return NULL ;
}

static List getListBucket(xmlNodePtr node)
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
              struct Bucket *bucket = getBucket(cur);
              if (bucket)
                listAdd(list, bucket);
            }
          cur = cur->next;
        }
      return list;
    }
  return NULL ;
}
BucketsResult *bucket_result_init()
{
  BucketsResult *bucket_result = malloc(sizeof(BucketsResult));
  memset(bucket_result, 0x0, sizeof(BucketsResult));

  bucket_result->owner = OwnerClass.init();
  bucket_result->buckets = ListClass.init();

  return bucket_result;
}

void bucket_result_destroy(BucketsResult *bucket_result)
{
  if (bucket_result)
    {
      if (bucket_result->owner)
        OwnerClass.destroy(bucket_result->owner);
      if (bucket_result->buckets)
        {
          ListClass.destroy_fun(bucket_result->buckets, (void
          (*)(void *)) BucketClass.destroy);
        }
      free(bucket_result);
    }
}

BucketsResult *bucket_result_parse(const char *xml)
{
  xmlDocPtr doc;
  xmlNodePtr root;
  xmlNodePtr cur;
  BucketsResult * result = BucketsResultClass.init();

  doc = xmlReadMemory(xml, strlen(xml), "noname.xml", NULL, 0);
  assert(doc != NULL);
  root = xmlDocGetRootElement(doc);
  cur = root->children;

  while (cur != NULL )
    {
      if (!strcmp((char *) cur->name, OWNER))
        {
          Owner *owner = getOwner(cur);
          if (owner)
            {
              OwnerClass.destroy(result->owner);
              result->owner = owner;
            }
        }
      if (!strcmp((char *) cur->name, BUCKETS))
        {
          List list = getListBucket(cur);
          if (!ListClass.isEmpty(list))
            {
              ListClass.destroy(result->buckets);
              result->buckets = list;
            }
        }
      cur = cur->next;
    }

  return result;
}


void free_ListBucketResult(ListBucketResult *result)
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
        listFreeObjectByFun(result->contents, (void (*)(void *))free_Contents);
        listFree(result->contents);
    }
}


