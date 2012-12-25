/*
 * String.c
 *
 *  Created on: 2012-12-6
 *      Author: lch
 */
#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include "String.h"

String*
new_string()
{
  String* s = (String*) malloc(sizeof(String));
  if (s == NULL )
    return NULL ;
  memset(s, 0x0, sizeof(String));
  return s;
}
void
free_string(String* s)
{
  if (s)
    {
      if (s->str)
        free(s->str);
      free(s);
    }
}
int
indexOf(const char* s, int c)
{
  char* first = strchr(s, c);
  if (first)
    return first - s;
  else
    return -1;
}
int
lastIndexOf(const char* s, int c)
{
  char* last = strrchr(s, c);
  if (last)
    return last - s;
  else
    return -1;
}
char*
substring(const char* s, int start, int end)
{
  int length = strlen(s);
  if (start >= length || end <= start)
    return NULL ;
  if (start < 0)
    start = 0;
  if (end >= length)
    end = length - 1;
  String str;
  str.length = end - start + 1;
  str.str = (char*) malloc(str.length + 1);
  memset(str.str, 0x0, str.length + 1);
  strncpy(str.str, s + start, str.length);
  return str.str;
}

char*
concat(int i, ...)
{
  va_list ap;
  va_start(ap, i);
  String str;
  str.str = (char*) calloc(256, sizeof(char));
  str.length = 255;
  int j = 0;
  char* arg;
  for (; j < i; j++)
    {
      arg = va_arg(ap,char*);
      int total = strlen(str.str) + strlen(arg);

      if (total > str.length)
        {
          str.str = (char*) realloc(str.str, total);
          str.length = total;
          strcat(str.str, arg);
        }
      else
        {
          strcat(str.str, arg);
        }

    }
  va_end(ap);
  str.length = strlen(str.str);
  char* new_string = strdup(str.str);
  free(str.str);
  return new_string;
}
