/*
 author:lch
 */
#ifndef SERVICE_H_
#define SERVICE_H_

#include "List.h"
#include "oss_config.h"

struct Bucket
{
  char* name;
  char* creationDate;
};
typedef struct Bucket Bucket;

struct Owner
{
  char* id;
  char* displayName;
};
typedef struct Owner Owner;

typedef struct
{
  Owner *owner;
  List buckets;
} BucketsResult;

typedef struct
{
  BucketsResult *
  (*init)();
  void
  (*destroy)(BucketsResult *);
  BucketsResult *
  (*parse)(const char *);
} BucketsResultOpration;

typedef struct
{
  Bucket *
  (*init)();
  void
  (*destroy)(Bucket *);
  int (*delete)(OSSPtr, char *);
  int (*put)(OSSPtr, char *);
  int (*putAcl)(OSSPtr, char *, ACL);
} BucketOpration;

typedef struct
{
  Owner *
  (*init)();
  void
  (*destroy)(Owner *);
} OwnerOpration;

Bucket *
bucket_init();

void
bucket_destroy(Bucket *bucket);

Owner *
owner_init();

void
owner_destroy(Owner *owner);

BucketsResult *
bucket_result_parse(const char *xml);

BucketsResult *
bucket_result_init();

void
bucket_result_destroy(BucketsResult *bucket_result);

extern BucketsResultOpration BucketsResultClass;

extern BucketOpration BucketClass;

extern OwnerOpration OwnerClass;

#endif
