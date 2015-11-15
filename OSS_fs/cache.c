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
#include "log.h"

static struct HashTable* TABLE;
static List node_changed;
static char* cache_dir;
static uid_t uid;
static gid_t gid;
static OSSPtr oss;
static mode_t default_dir = S_IFDIR | S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH
| S_IXOTH;
static mode_t default_file = S_IFREG | S_IRWXU | S_IRGRP | S_IROTH;
/*private function*/
static oss_node* new_node(oss_node* parent, mode_t mode);

static void delete_node(oss_node* node);

static void add_node(oss_node* parent, ListBucketResult* result);

static inline oss_node* getNodeFromParent(oss_node* parent, const char* path)
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

static oss_node* get_parent_node(const char* path);

static void getBucket(const char *path, char **bucket);

static void getObject(const char *path, char **object);

static void getParent(const char *path, char **parent);

/*public function*/
void oss_init_cache(OSSPtr _oss)
{
    oss = _oss;
    log_debug("%s", "初始化缓存");
    char* home = getenv("HOME");
    char* dir = "/.cache/OSS/";
    cache_dir = StringClass.concat(2, home, dir);

    uid = getuid();
    gid = getgid();
    log_debug("%s", "初始化hashtable");
    TABLE = hash_table_init_size(OSS_TABLE_SIZE);
    node_changed = listInit();
    List buckets = GetService(oss);
    oss_node* root = new_node(NULL, default_dir);
    root->name = strdup("/");
    root->path = strdup("/");
    root->cache_path = strdup(cache_dir);
    oss_node* trash = new_node(root, default_dir);
    trash->name = strdup("/.Trash"); 
    trash->path = strdup("/.Trash");
    HashTableClass.put(TABLE,trash->path, trash); 
    log_msg("process bucket list");
    List bucket_node;
    for_each(bucket_node,buckets)
    {
        struct Bucket* bucket = (struct Bucket*) bucket_node->ptr;
        char* str_bucket = StringClass.concat(2, "/", bucket->name);
        log_msg("%s\n", str_bucket);
        oss_node* node = new_node(root, default_dir);
        node->name = strdup(bucket->name);
        node->path = str_bucket;
        hash_table_put(TABLE, node->path, node);
        ListBucketResult* result = ListObject(oss, bucket->name, NULL, 0, NULL,
                "/");
        log_msg("add to cache");
        add_node(node, result);
        ListBucketResultClass.destroy(result);
    }
    hash_table_put(TABLE, "/", root);
    ListClass.destroy_fun(buckets, (void (*)(void *))BucketClass.destroy);
}

void oss_close_cache(int sync)
{
    free(cache_dir);
    HashTableClass.destroy(TABLE);
    ListClass.destroy(node_changed);
    OSSClass.destroy(oss);
}

static oss_node* new_node(oss_node *parent, mode_t mode)
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
void oss_reatattr(oss_node* node, struct stat* st)
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

oss_node* oss_get_cache(const char* path)
{
    oss_node* node = (oss_node*) hash_table_get(TABLE, path);
    if (node)
        return node;
    log_debug("%s can't be found from cache,try to find from parent", path);
    char* parent = NULL;
    log_msg("try to find it in its parent");
    getParent(path, &parent);
    if(parent != NULL)
    {
        oss_node* parent_node = oss_get_cache(parent);
        if (parent_node != NULL)
        {
            node = getNodeFromParent(parent_node, path);
        }
        free(parent);
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
static void add_node(oss_node* parent, ListBucketResult* result)
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
        log_msg("%s",dir);
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
        char *bucket = NULL;
        char *object = NULL;
        getBucket(node->path, &bucket);
        getObject(node->path, &object);
        node->cache_path = concat(2, cache_dir, "oss_XXXXXX");
        fd = mkstemp(node->cache_path);
        log_msg("create file %s\n", node->cache_path);
        chmod(node->cache_path, node->mode);
        close(fd);
        GetObject(oss, bucket, object, node->cache_path);
        free(bucket);
        free(object);
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
void oss_read_dir(oss_node* parent){
    if (S_ISDIR(parent->mode))
    {
        char *bucket = NULL;
        char *object = NULL;
        getBucket(parent->path, &bucket);
        getObject(parent->path, &object);
        ListBucketResult* result = ListObject(oss, bucket, object, 0, NULL,
                "/");
        if (bucket)
            free(bucket);
        if(object)
            free(object);
        add_node(parent, result);
        //TODO:释放result
        ListBucketResultClass.destroy(result);
    }
}

oss_node* make_node(const char* path)
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

static oss_node* get_parent_node(const char* path)
{
    char *parent = NULL;
    getParent(path,&parent);
    oss_node* parentNode = oss_get_cache(path);
    free(parent);
    return parentNode;
}


static void getBucket(const char *path, char **bucket)
{
    if(path == NULL || strlen(path) <= 1)
    {
        log_error("can not get bucket form %s", path);
        return;
    }
    int index = StringClass.indexOf(path+1, '/');
    if(index == -1)
    {
        *bucket = strdup(path+1);
        return;
    }
    else
    {
        *bucket = StringClass.substring(path, 1, index-1);
    }
}

static void getObject(const char *path, char **object)
{
    if(path == NULL)
    {
        log_error("can not get object from %s", path);
        return;
    }

    int index = StringClass.indexOf(path, '/');
    if(index == -1)
    {
        log_error("There is none object in %s", path);
        return;
    }
    else
    {
        *object = StringClass.substring(path, index + 1, strlen(path));
    }
}
static void getParent(const char *path, char **parent)
{
    if(path == NULL)
    {
        log_error("path is null");
    }

    int index = StringClass.indexOf(path+1, '/');
    if(index == -1)
    {
        log_error("%s doesn't have parent path", path);
        return;
    }

    *parent = StringClass.substring(path, 0, index-1);
}
