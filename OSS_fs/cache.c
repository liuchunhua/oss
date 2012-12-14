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
mode_t default_dir_mask = S_IWGRP | S_IWOTH;
mode_t default_file_mask = S_IWGRP | S_IXGRP | S_IWOTH | S_IXOTH;
/*private function*/
static oss_node*
new_node(oss_node* parent, mode_t mode);

static void
delete_node(oss_node* node);

static void
add_node(oss_node* parent, ListBucketResult* result);

static inline oss_node*
getNodeFromParent(oss_node* parent,char* path){
  oss_node* node = parent->childs;
  while(node){
      if(strcmp(path,node->path)==0)
        return node;
      node = node->next;
  }
  return NULL;
}

static oss_node*
get_node(char* path);
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
  buckets = listInit();
  oss = oss_init("storage.aliyun.com", "abysmn89uz488l1dfycon3qa",
      "qfEZ+LNuGJUP/FlRw1R3aKpwiwY=");
  listAdd(buckets, strdup("welcome2myspace"));
  listAdd(buckets, strdup("liuchunhua"));
  oss_node* root =
      new_node(NULL, S_IFDIR | S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
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
      node->mode = mode & ~default_dir_mask;
      node->nref = 2;
    }
  else
    {
      node->mode = mode & ~default_file_mask;
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
  oss_node* node = (oss_node*)hash_table_get(TABLE,path);
  if(!node){
      if(strlen(strrchr(path,'/'))==1){
          char* tmp = substring(path,0,strlen(path)-2);
          int index = lastIndexOf(tmp,'/');
          free(tmp);
          char* parent = substring(path,0,index);
          oss_node* parent_node = hash_table_get(TABLE,parent);
          free(parent);
          if(parent_node)
            node = getNodeFromParent(parent_node,path);
          if(!node){
              node = new_node(NULL,S_IFDIR|S_IRWXU);
              node->path = strdup(path);
              node->name = substring(path,index,strlen(path)-1);
          }
      }
  }
  if(!node){
      //TODO:get from server
      OSSObject* object = HeadObject(oss,path);
      if(!object)
        return NULL;
      node = new_node(NULL,S_IFREG | S_IRWXU | S_IRGRP | S_IROTH);
      node->name = substring(object->name,lastIndexOf(object->name, '/'),
          strlen(object->name) - 1);
      node->path = strdup(path);
      node->etag = strdup(object->etag);
      node->mime_type = strdup(object->minetype);
      node->mtime = object->mtime;
      node->size = object->size;
  }
  hash_table_put(TABLE,path,node);
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
      oss_node* node = new_node(parent, S_IFREG | S_IRWXU | S_IRGRP | S_IROTH);
      Contents* content = (Contents*) object_node->ptr;
      node->name = substring(content->key, lastIndexOf(content->key, '/'),
          strlen(content->key) - 1);
      node->path = concat(parent->path, node->name);
      node->etag = strdup(content->etag);
      node->mime_type = strdup(content->type);
      node->size = atol(content->size);
      node->mtime = StrGmtToLocaltime(content->lastmodified);
      node->utime = node->mtime;
      node->smtime = node->mtime;
    }
  List dir_node;
  for_each(dir_node,result->commonprefixes)
    {
      oss_node* node =
          new_node(parent,
              S_IFDIR | S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
      char* dir = (char*) dir_node->ptr;
      dir = substring(dir, 0, strlen(dir) - 2);
      node->name = substring(dir,lastIndexOf(dir, '/'), strlen(dir) - 1);
      free(dir);
      node->path = concat(parent->path, node->name, "/");
    }
}
static oss_node*
get_node(char* path){
  char *str1,*token,*saveptr;
  char *tmp = strdup(path);
  for(str1 = tmp;;str1=NULL){
      token = strtok_r(str1,"/",&saveptr);
      if(token==NULL)
        break;
  }
  free(tmp);
}
