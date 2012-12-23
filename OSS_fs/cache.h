/*
 * cache.h
 *
 *  Created on: 2012-11-26
 *      Author: lch
 */

#ifndef CACHE_H_
#define CACHE_H_

#include <fcntl.h>
#include <time.h>

#define OSS_MASK  (S_IRWXU|S_IRWXG|S_IRWXO|S_ISUID|S_ISGID|S_ISVTX)

typedef struct oss_handle oss_handle;
struct oss_handle
{
  oss_handle *next; /* Next in the list. */
  int fd; /* File descriptor of the open file. */
  int flags; /* Access mode flags only. */
  pid_t pid; /* id of requesting process. */
  pid_t pgid; /* Group id of requesting process. */
  uid_t uid; /* User id of requesting process. */
};

typedef struct oss_node oss_node;
struct oss_node
{
  /*parent of the node*/
  oss_node *parent;
  /*first node of childs if the node is directory*/
  oss_node *childs;
  /*the next of the child list of parent*/
  oss_node *next;
  /*The unescaped absolute path of the resource on the oss.*/
  char* path;
  /*file name on the oss*/
  char* name;
  /*the cache path in the locale,
   for directory,this file contains the xml returned from server,
   for file,the local copy of the file*/
  char *cache_path;
  /*Etag from the server,file md5*/
  char* etag;
  /*media type */
  char* mime_type;
  /*a linked list handles for open files*/
  oss_handle* handles;
  /*File:the content-length of the http response,
   Directory:the size of the directory entries*/
  off_t size;
  /* Time of last access.  */
  time_t atime;
  /* Time of last modification.  */
  time_t mtime;
  /* Time of last status change.  */
  time_t ctime;
  /*the last-modified time from remote time */
  time_t smtime;
  /* Time when the node has last been updated from the server. */
  time_t utime;
  /* Files: Time when the lock expires.
   0 if not locked. -1 locked infinitely. */
  time_t lock_expire;
  /* Directories: Number of subdirectories, including "." and "..".
   Files: Allways 1. */
  int nref;
  /* Files: File exists on the server. For locally created files that have
   not been put to the server it will be 0.
   Note: Some server create an empty file when locking. Maybe the RFC will
   be changed this way. */
  int remote_exists;
  /* Files: Dirty flag. Must be set, when a file is opened for writing, and
   reset, when the file is saved back. */
  int dirty;
  /* mode, uid and gid are initially set to the default values, but may be
   changed by dav_setattr(). */
  mode_t mode;
  uid_t uid;
  gid_t gid;
};


void
oss_init_cache(const char* dir);

/*sync is 1 not to save back to server*/
void
oss_close_cache(int sync);

/*if cache_path exists,return stat of the cache file,not return the stat createdd*/
void
oss_reatattr(oss_node* node,struct stat* stat);
/*get oss_node from path*/
oss_node*
oss_get_cache(const char* path);
int
oss_open_node(const char* path,int flags);

void
log_msg(const char* format,...);
#endif /* CACHE_H_ */
