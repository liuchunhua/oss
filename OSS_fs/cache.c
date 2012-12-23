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

#include "cache.h"
#include "HashTable.h"
#include "List.h"
#include "String.h"
#include "oss.h"

struct HashTable* TABLE;
List node_changed;
char* cache_dir;
uid_t uid;
gid_t gid;
OSSPtr oss;
List buckets;
mode_t default_dir = S_IFDIR | S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
mode_t default_file = S_IFREG | S_IRWXU | S_IRGRP | S_IROTH;
/*private function*/
static oss_node*
new_node(oss_node* parent, mode_t mode);

static void
delete_node(oss_node* node);

static void
add_node(oss_node* parent, ListBucketResult* result);

static inline oss_node*
getNodeFromParent(oss_node* parent, char* path)
{
  oss_node* node = parent->childs;
  while (node)
    {
      if (strcmp(path, node->path) == 0)
        return node;
      node = node->next;
    }
  return NULL ;
}
static oss_node*
get_parent_node(char* path);
static oss_node*
make_node(char* path);
/*public function*/
void
oss_init_cache(const char* cache)
{
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
      cache_dir = cache;
    }

  uid = getuid();
  gid = getgid();

  TABLE = hash_table_init_size(OSS_TABLE_SIZE);
  node_changed = listInit();
  oss = oss_init("storage.aliyun.com", "abysmn89uz488l1dfycon3qa",
      "qfEZ+LNuGJUP/FlRw1R3aKpwiwY=");
  buckets = GetService(oss);
  oss_node* root = new_node(NULL, default_dir);
  root->name = strdup("/");
  root->path = strdup("/");
  root->cache_path = strdup(cache_dir);
  List bucket_node;
  for_each(bucket_node,buckets)
    {
      char* bucket = concat("/", (char*) bucket_node->ptr);
      ListBucketResult* result = ListObject(oss, bucket, NULL, 0, NULL, "/");
      free(bucket);
      add_node(root, result);
    }
  listFreeObject(buckets);
  listFree(buckets);
  hash_table_put(TABLE, "/", root);
}

void
oss_close_cache(int sync)
{
  free(cache_dir);
  hash_table_free(TABLE);
  listFree(node_changed);
  free_ossptr(oss);
  listFreeObject(buckets);
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
  node->lock_expire;
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
  if (!node)
    {
      if (strlen(strrchr(path, '/')) == 1)
        { //Dir
          char* tmp = substring(path, 0, strlen(path) - 2);
          int index = lastIndexOf(tmp, '/');
          free(tmp);
          char* parent = substring(path, 0, index);
          oss_node* parent_node = hash_table_get(TABLE, parent);
          free(parent);
          if (parent_node)
            node = getNodeFromParent(parent_node, path);
          if (!node)
            {
              node = make_node(path);
            }
        }
      else
        { //FILE
          node = make_node(path);
        }
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
  List object_node;
  for_each(object_node,result->contents)
    {
      Contents* content = (Contents*) object_node->ptr;
      char* name = ubstring(content->key, lastIndexOf(content->key, '/'),
          strlen(content->key) - 1);
      char* path = concat(parent->path, name);
      oss_node* cache = oss_get_cache(path);
      if (cache)
        {
          if (!cache->parent)
            {
              cache->parent = parent;
              cache->next = parent->childs;
              parent->childs = cache;
            }
          free(name);
          free(path);
          continue;
        }
      oss_node* node = new_node(parent, default_file);

      node->name = name;
      node->path = path;
      node->etag = strdup(content->etag);
      node->mime_type = strdup(content->type);
      node->size = atol(content->size);
      node->mtime = StrGmtToLocaltime(content->lastmodified);
      node->utime = node->mtime;
      node->smtime = node->mtime;
      free(name);
      free(path);
    }
  List dir_node;
  for_each(dir_node,result->commonprefixes)
    {
      char* dir = (char*) dir_node->ptr;
      dir = substring(dir, 0, strlen(dir) - 2);
      char* name = substring(dir, lastIndexOf(dir, '/'), strlen(dir) - 1);
      char* path = concat(parent->path, name, "/");
      oss_node* cache = oss_get_cache(path);
      if (cache)
        {
          free(name);
          free(path);
          continue;
        }
      oss_node* node = new_node(parent, default_dir);
      char* dir = (char*) dir_node->ptr;
      dir = substring(dir, 0, strlen(dir) - 2);
      node->name = name;
      free(dir);
      node->path = path;
      hash_table_put(TABLE, path, node); //for alone node to find parent
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
      node->cache_path = concat(cache_dir, "oss_XXXXXX");
      fd = mkstemp(node->cache_path);
      log_msg("create file %s\n", node->cache_path);
      chmod(node->cache_path, node->mode);
      close(fd);
      GetObject(oss, node->path, node->cache_path, NULL, 0);
    }
  else
    {
      fd = open(node->cache_path, flags);
    }
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

  vsyslog(LOG_MAKEPRI(LOG_USER,LOG_DEBUG), format, sp);
}
/*private function implement*/

static oss_node*
make_node(char* path)
{
//  char *str1, *token, *saveptr;
//  char *tmp = strdup(path);
//  for (str1 = tmp;; str1 = NULL )
//    {
//      token = strtok_r(str1, "/", &saveptr);
//      if (token == NULL )
//        break;
//    }
//  free(tmp);
  int lastSlash = lastIndexOf(path, '/');
  mode_t mode;
  if (lastSlash == strlen(path) - 1)
    {
      mode = default_dir;
      char* bucket = substring(path, 0, indexOf(path + 1, '/'));
      char* prefix = strchr(path + 1, '/');
      ListBucketResult* result = ListObject(oss, bucket, prefix, 0, NULL, "/");
      if (result)
        {
          oss_node* parent = get_parent_node(path);
          oss_node* dir = new_node(parent, mode);
          dir->path = strdup(path);
          hash_table_put(TABLE, path, (void*) dir);
          add_node(dir, result);
          return dir;
        }
    }
  else
    {
      mode = default_file;
      OSSObject* object = HeadObject(oss, path);
      if (!object)
        return NULL ;
      oss_node* parent = get_parent_node(path);
      oss_node* node = new_node(parent, mode);
      node->name = substring(object->name, lastIndexOf(object->name, '/'),
          strlen(object->name) - 1);
      node->path = strdup(path);
      node->etag = strdup(object->etag);
      node->mime_type = strdup(object->minetype);
      node->mtime = object->mtime;
      node->size = object->size;
      hash_table_put(TABLE, path, node);
      return node;
    }
  return NULL ;
}
static oss_node*
get_parent_node(char* path)
{
  int len = strlen(path);
  int index = len - 1;
  for (; index >= 0; index--)
    {
      if (path[index] == '/')
        {
          if (index == len - 1)
            {
              continue;
            }
          break;
        }
    }
  char* parent_path = substring(path, 0, index);
  oss_node* parent = hash_table_get(TABLE, parent_path);
  free(parent_path);
  return parent;

}
