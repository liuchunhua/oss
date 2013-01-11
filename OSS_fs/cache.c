/*
 * cache.c
 *
 *  Created on: 2012-11-26
 *      Author: lch
 */

#define OSS_TABLE_SIZE 256
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <syslog.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>

#include "cache.h"
#include "HashTable.h"
#include "List.h"
#include "String.h"
#include "oss.h"

static struct HashTable* TABLE;
static List node_changed;
static char* cache_dir;
static uid_t uid;
static gid_t gid;
static OSSPtr oss;
static List buckets;
static mode_t default_dir = S_IFDIR | S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH
    | S_IXOTH;
static mode_t default_file = S_IFREG | S_IRWXU | S_IRGRP | S_IROTH;
/*private function*/
static oss_node*
new_node(oss_node* parent, mode_t mode);

static void
delete_node(oss_node* node);

static void
add_node(oss_node* parent, ListBucketResult* result);

static inline oss_node*
getNodeFromParent(oss_node* parent, const char* path)
{
  oss_node* node = parent->childs;
  while (node)
    {
      if (strcmp(path, node->path) == 0)
        return node;
      node = node->next;
    }
  return node;
}
static oss_node*
get_parent_node(const char* path);

/*public function*/
void
oss_init_cache(const char* cache)
{
  log_msg("%s\n", "初始化缓存");
  if (!cache)
    {
      char* home = getenv("HOME");
      char* dir = "/.cache/OSS/";
      cache_dir = (char*) malloc(strlen(home) + strlen(dir) + 1);
      strcat(cache_dir, home);
      strcat(cache_dir, dir);
    }
  else
    {
      cache_dir = strdup(cache);
    }

  uid = getuid();
  gid = getgid();
  log_msg("%s\n", "初始化hashtable");
  TABLE = hash_table_init_size(OSS_TABLE_SIZE);
  node_changed = listInit();
  oss = oss_init("storage.aliyun.com", "abysmn89uz488l1dfycon3qa",
      "qfEZ+LNuGJUP/FlRw1R3aKpwiwY=");
//  oss->proxy = "192.168.0.142:808";
  buckets = GetService(oss);
  log_msg("GetService");
  oss_node* root = new_node(NULL, default_dir);
  root->name = strdup("/");
  root->path = strdup("/");
  root->cache_path = strdup(cache_dir);
  log_msg("process bucket list");
  List bucket_node;
  for_each(bucket_node,buckets)
    {
      struct Bucket* bucket = (struct Bucket*) bucket_node->ptr;
      char* str_bucket = concat(2, "/", bucket->name);
      log_msg("%s\n", str_bucket);
      oss_node* node = new_node(root, default_dir);
      node->name = strdup(bucket->name);
      node->path = str_bucket;
      hash_table_put(TABLE, node->path, node);
      ListBucketResult* result = ListObject(oss, str_bucket, NULL, 0, NULL,
          "/");
      log_msg("add to cache");
      add_node(node, result);
      free_ListBucketResult(result);
    }
  log_msg("root");
  hash_table_put(TABLE, "/", root);
  log_msg("root");
}

void
oss_close_cache(int sync)
{
  free(cache_dir);
  hash_table_free(TABLE);
  listFree(node_changed);
  free_ossptr(oss);
  listFreeObjectByFun(buckets, (void*) free_Bucket);
  listFree(buckets);
}

static oss_node*
new_node(oss_node *parent, mode_t mode)
{
  oss_node* node = (oss_node*) malloc(sizeof(oss_node));
  memset(node, 0x0, sizeof(oss_node));
  node->parent = parent;
  node->childs = NULL;
  if (parent)
    {
      if (S_ISDIR(mode))
        {
          ++parent->nref;
        }
      node->next = parent->childs;
      parent->childs = node;
      parent->size++;
    }
  node->path = NULL;
  node->name = NULL;
  node->cache_path = NULL;
  node->etag = NULL;
  node->mime_type = NULL;
  node->name = NULL;
  node->handles = NULL;
  node->size = 0;
  node->atime = time(NULL );
  node->mtime = node->atime;
  node->ctime = node->atime;
  node->smtime = 0;
  node->utime = 0;
  node->lock_expire = 0;
  if (S_ISDIR(mode))
    {
      node->mode = mode;
      node->nref = 2;
    }
  else
    {
      node->mode = mode;
      node->nref = 1;
    }
  node->dirty = 0;
  node->uid = uid;
  node->gid = gid;

  return node;
}
void
oss_reatattr(oss_node* node, struct stat* st)
{
  if (node->cache_path)
    {
      stat(node->cache_path, st);
      return;
    }
  else
    {
      st->st_mode = node->mode;
      st->st_uid = node->uid;
      st->st_gid = node->gid;
      st->st_mtim.tv_sec = node->mtime;
      st->st_atim.tv_sec = node->atime;
      st->st_ctim.tv_sec = node->ctime;
      st->st_nlink = 0;
      st->st_size = node->size;
    }
}

oss_node*
oss_get_cache(const char* path)
{
  oss_node* node = (oss_node*) hash_table_get(TABLE, path);
  if (node)
    return node;
  log_msg("%s can't be found from cache", path);
  char* parent = NULL;
  if (!node)
    {
      log_msg("try to find it in its parent");
      if (strlen(strrchr(path, '/')) == 1)
        { //Dir
          char* tmp = substring(path, 0, strlen(path) - 2);
          int index = lastIndexOf(tmp, '/');
          free(tmp);
          parent = substring(path, 0, index);
        }
      else
        { //
          int index = lastIndexOf(path, '/');
          if (index > 0) //root is 0
            index -= 1;
          parent = substring(path, 0, index);
        }
      oss_node* parent_node = hash_table_get(TABLE, parent);
      if (parent)
        free(parent);
      if (parent_node)
        node = getNodeFromParent(parent_node, path);
//      if(!node)
//        node = make_node(path);
    }

  return node;
}

static void
delete_node(oss_node* node)
{
  if (!node)
    return;
  if (node->path)
    free(node->path);
  if (node->name)
    free(node->path);
  if (node->cache_path)
    free(node->cache_path);
  if (node->etag)
    free(node->etag);
  if (node->mime_type)
    free(node->mime_type);
  while (node->handles)
    {
      oss_handle* tofree = node->handles;
      node->handles = node->handles->next;
      close(tofree->fd);
      free(tofree);
    }
  free(node);
}
static void
add_node(oss_node* parent, ListBucketResult* result)
{
  if (!result)
    return;
  List object_node;
  for_each(object_node,result->contents)
    {
      Contents* content = (Contents*) object_node->ptr;
      char* name = substring(content->key, lastIndexOf(content->key, '/') + 1,
          strlen(content->key) - 1);
      char* path = concat(3, parent->path, "/", name);
      log_msg("%s %s", name, path);
      oss_node* node = new_node(parent, default_file);
      log_msg("create new node %s", name);
      node->name = strdup(name);
      node->path = strdup(path);
      node->etag = strdup(content->etag);
      node->mime_type = strdup(content->type);
      node->size = atol(content->size);
      node->mtime = GmtToLocaltime(content->lastmodified);
      node->utime = node->mtime;
      node->smtime = node->mtime;
      free(name);
      free(path);
      log_msg("create end");
    }
  List dir_node;
  for_each(dir_node,result->commonprefixes)
    {
      char* dir = (char*) dir_node->ptr;
      log_msg(dir);
      dir = substring(dir, 0, strlen(dir) - 2);
      char* name = substring(dir, lastIndexOf(dir, '/'), strlen(dir) - 1);
      char* path = concat(3, parent->path, "/", name);

      log_msg("create new node %s", name);
      oss_node* node = new_node(parent, default_dir);
//      dir = substring(dir, 0, strlen(dir) - 2);
      node->name = strdup(name);
      free(dir);
      node->path = strdup(path);
      hash_table_put(TABLE, node->path, node); //for alone node to find parent
      free(name);
      free(path);
    }
}

int
oss_open_node(const char* path, int flags)
{
  int fd;
  oss_node* node = oss_get_cache(path);
  if (!node)
    return ENOENT;
  if (!node->cache_path)
    {
      node->cache_path = concat(2, cache_dir, "oss_XXXXXX");
      fd = mkstemp(node->cache_path);
      log_msg("create file %s\n", node->cache_path);
      chmod(node->cache_path, node->mode);
      close(fd);
      GetObject(oss, node->path, node->cache_path, NULL, 0);
    }
  fd = open(node->cache_path, flags);
  struct oss_handle* handle = (struct oss_handle*) malloc(
      sizeof(struct oss_handle));
  handle->fd = fd;
  handle->uid = uid;
  if (node->handles)
    handle->next = node->handles;
  node->handles = handle->next;
  return fd;
}
void
log_msg(const char* format, ...)
{
  va_list sp;
  va_start(sp, format);
#ifdef DEBUG
  vprintf(format,sp);
  printf("\n");
#else
  vsyslog(LOG_MAKEPRI(LOG_USER,LOG_DEBUG), format, sp);
#endif
}
void
oss_read_dir(oss_node* parent){
  if (S_ISDIR(parent->mode))
     {
       oss_node* bucket = parent;
       while (lastIndexOf(bucket->path, '/') != 0)
         {
           bucket = bucket->parent;
         }
       char* relative_path = substring(parent->path, strlen(bucket->path) + 1, strlen(parent->path)-1);
       char* prefix = concat(2,relative_path,"/");
       ListBucketResult* result = ListObject(oss, bucket->path, prefix, 0, NULL,
           "/");
       if (prefix)
         free(prefix);
       if(relative_path)
         free(relative_path);
       add_node(parent, result);
     }
}


oss_node*
make_node(const char* path)
{
  /*
   * 先找到父节点
   *      如果父节点不存在，继续找父节点
   *      找到后，向网络查询字节点
   *      检查path是否存在
   *      存在，返回该节点
   *      不存在，返回NULL
   *
   * */
  oss_node* node = NULL;
  oss_node* parent = get_parent_node(path);
  if (!parent)
    {
      int index = lastIndexOf(path, '/');
      if (index == 0 && parent == NULL )
        return NULL ;
      if (index > 0)
        index -= 1;
      char* parent_path = substring(path, 0, index);
      parent = make_node(parent_path);
      free(parent_path);
    }
  if (parent->nref == 2)
    {
      oss_read_dir(parent);
      node = parent->childs;
      while (strcmp(path, node->path) != 0)
        {
          node = node->next;
        }
    }
  return node;
}
/*private function implement*/

static oss_node*
get_parent_node(const char* path)
{
  int index = lastIndexOf(path, '/');
  if (index > 0)
    index -= 1;
  char* parent_path = substring(path, 0, index);
  oss_node* parent = hash_table_get(TABLE, parent_path);
  free(parent_path);
  return parent;
}
