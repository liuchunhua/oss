/*
 * hello.c
 *
 *  Created on: 2012-9-17
 *      Author: lch
 */

/*
 FUSE: Filesystem in Userspace
 Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

 This program can be distributed under the terms of the GNU GPL.
 See the file COPYING.

 gcc -Wall `pkg-config fuse --cflags --libs` hello.c -o hello
 */

#define FUSE_USE_VERSION 26
#define _FILE_OFFSET_BITS 64

#include "oss.h"
#include "List.h"
#include "ossutil.h"

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <syslog.h>

static OSSPtr oss;

static int
hello_getattr(const char *path, struct stat *stbuf)
{
  syslog(LOG_MAKEPRI(LOG_USER,LOG_WARNING), "path:%s\n", path);
  int res = 0;
  memset(stbuf, 0, sizeof(struct stat));
  if (strstr(path, ".") == NULL )
    {
      stbuf->st_mode = S_IFDIR | 0755;
      stbuf->st_nlink = 2;
    }
  else
    {
      stbuf->st_mode = S_IFREG | 0444;
      stbuf->st_nlink = 1;
      OSSObject* object = HeadObject(oss, path);
      stbuf->st_size = object->size;
    }

  return res;
}

static int
hello_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
    struct fuse_file_info *fi)
{
  syslog(LOG_MAKEPRI(LOG_USER,LOG_WARNING), "%s\n", path);
  (void) offset;
  (void) fi;
  if (strcmp(path, "/") != 0)
    {
      filler(buf, ".", NULL, 0);
      filler(buf, "..", NULL, 0);
      syslog(LOG_MAKEPRI(LOG_USER,LOG_WARNING), "connect to oss\n");
      ListBucketResult* result = ListObject(oss, path, NULL, 0, NULL, "/");
      if (result == NULL )
        return -EIO;
      List node;
      for_each(node,result->contents)
        {
          Contents* content = (Contents*) node->ptr;
          syslog(LOG_MAKEPRI(LOG_USER,LOG_WARNING), "%s\n", content->key);
          filler(buf, content->key, NULL, 0);
        }
      syslog(LOG_MAKEPRI(LOG_USER,LOG_WARNING), "common prefixes\n");
      if (result->commonprefixes)
        {
          for_each(node,result->commonprefixes)
            {
              syslog(LOG_MAKEPRI(LOG_USER,LOG_WARNING), "%s\n",
                  (char*) node->ptr);
              filler(buf, (char*) (node->ptr), NULL, 0);
            }
        }
      syslog(LOG_MAKEPRI(LOG_USER,LOG_WARNING), "to free object\n");
      free_ListBucketResult(result);
      syslog(LOG_MAKEPRI(LOG_USER,LOG_WARNING), "return from hello_readdir\n");
      return 0;
    }
  syslog(LOG_MAKEPRI(LOG_USER,LOG_WARNING), "connect to oss\n");
  List list = GetService(oss);
  filler(buf, ".", NULL, 0);
  filler(buf, "..", NULL, 0);
  if (list == NULL )
    return 0;
  List node;
  for_each(node,list)
    {
      struct Bucket* bucket = (struct Bucket*) node->ptr;
      syslog(LOG_MAKEPRI(LOG_USER,LOG_WARNING), "%s\n", bucket->name);
      filler(buf, bucket->name, NULL, 0);
    }
  listFreeObjectByFun(list, (void*) free_Bucket);
  listFree(list);
  return 0;
}

static int
hello_open(const char *path, struct fuse_file_info *fi)
{
  syslog(LOG_MAKEPRI(LOG_USER,LOG_WARNING), "hello_open");
//  if (strcmp(path, hello_path) != 0)
//    return -ENOENT;
//
//  if ((fi->flags & 3) != O_RDONLY)
//    return -EACCES;

  return 0;
}

static int
hello_read(const char *path, char *buf, size_t size, off_t offset,
    struct fuse_file_info *fi)
{
  syslog(LOG_MAKEPRI(LOG_USER,LOG_WARNING), "hello_read");
  OSSObject* object = HeadObject(oss, path);
  size_t len = object->size;
  (void) fi;

  if (offset < len)
    {
      if (offset + size > len)
        size = len - offset;
      syslog(LOG_MAKEPRI(LOG_USER,LOG_WARNING), "read from %ld : %ld bytes\n",(long int)offset,(long int)size);
      GetObjectIntoMemory(oss, path, buf, size, offset, NULL );
    }
  else
    size = 0;

  return size;
}

static struct fuse_operations hello_oper =
  { .getattr = hello_getattr, .readdir = hello_readdir, .open = hello_open,
      .read = hello_read, };

int
main(int argc, char *argv[])
{
  oss = oss_init("storage.aliyun.com", "abysmn89uz488l1dfycon3qa",
      "qfEZ+LNuGJUP/FlRw1R3aKpwiwY=");
  int result = fuse_main(argc, argv, &hello_oper, NULL);
  free_ossptr(oss);
  return result;
}
