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
#include "cache.h"

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
  syslog(LOG_MAKEPRI(LOG_USER,LOG_WARNING), "hello_getattr:%s\n", path);
  oss_node* node = oss_get_cache(path);
  if (!node)
    {
      node = make_node(path);
      if (!node)
        return -ENOENT;
    }
  memset(stbuf, 0, sizeof(struct stat));
  stat("/home/liuchunhua", stbuf);
  if (S_ISDIR(node->mode))
    {
      log_msg("%s is dir", path);
      stbuf->st_mode = node->mode;
      stbuf->st_nlink = 2;
      stbuf->st_atim.tv_sec = node->atime;
      stbuf->st_mtim.tv_sec = node->mtime;
      stbuf->st_gid = node->gid;
      stbuf->st_uid = node->uid;
      stbuf->st_ctim.tv_sec = node->ctime;
    }
  else
    {
      log_msg("%s is reg file", path);
      stbuf->st_mode = node->mode;
      stbuf->st_nlink = 1;
      stbuf->st_size = node->size;
      stbuf->st_atim.tv_sec = node->atime;
      stbuf->st_mtim.tv_sec = node->mtime;
      stbuf->st_gid = node->gid;
      stbuf->st_uid = node->uid;
      stbuf->st_ctim.tv_sec = node->ctime;
    }

  return 0;
}

static int
hello_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
    struct fuse_file_info *fi)
{
  log_msg("hello_readdir : %s", path);
  if (strcmp(".", path) == 0 || strcmp(".", path) == 0)
    filler(buf, path, NULL, 0);
  oss_node* node = oss_get_cache(path);
  if (!node)
    {
      node = make_node(path);
      if (!node)
        return -ENOENT;
    }
  if(S_ISDIR(node->mode)&&node->nref==2)
    oss_read_dir(node);
  log_msg("child num is %d", node->nref);
  if (S_ISDIR(node->mode))
    {

      oss_node* child = node->childs;
      while (child)
        {
          log_msg("%s", child->name);
          filler(buf, child->name, NULL, 0);
          child = child->next;
        }
    }
  return 0;
}

static int
hello_open(const char *path, struct fuse_file_info *fi)
{
  log_msg("hello_open %s", path);
//  if (strcmp(path, hello_path) != 0)
//    return -ENOENT;
//
//  if ((fi->flags & 3) != O_RDONLY)
//    return -EACCES;
  int fd = oss_open_node(path, fi->flags);
  if (fd < 0)
    return EACCES;
  fi->fh = fd;
  return 0;
}

static int
hello_read(const char *path, char *buf, size_t size, off_t offset,
    struct fuse_file_info *fi)
{
  log_msg("hello_read : %s", path);
  oss_node* node = oss_get_cache(path);
  if (!node)
    {
      node = make_node(path);
      if (!node)
        return -ENOENT;
    }
  int count = 0;
  count = pread(fi->fh, buf, size, offset);
  if (count < 0)
    return EIO;

  return count;
}

static struct fuse_operations hello_oper =
  { .getattr = hello_getattr, .readdir = hello_readdir, .open = hello_open,
      .read = hello_read, };

int
main(int argc, char *argv[])
{
  oss = oss_init("storage.aliyun.com", "abysmn89uz488l1dfycon3qa",
      "qfEZ+LNuGJUP/FlRw1R3aKpwiwY=");
  oss_init_cache(NULL );
  log_msg("cache ready");
  int result = fuse_main(argc, argv, &hello_oper, NULL);
  free_ossptr(oss);
  return result;
}
