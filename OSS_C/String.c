/*
 * String.c
 *
 *  Created on: 2012-12-6
 *      Author: lch
 */
#include <stddef.h>

#include "String.h"

static String* new_string(){
  String* s = (String*)malloc(sizeof(String));
  if(s==NULL)
    return NULL;
  memset(s,0x0,sizeof(String));
  return s;
}
void
free_string(String* s){
  if(s){
      if(s->str)
        free(s->str);
    free(s);
  }
}
int
indexOf(const char* s, int c)
{
  char* first = strchr(s,c);
  if(first)
    return first - s ;
  else
    return -1;
}

String*
substring(const char* s , int start, int end)
{
  int length = strlen(s);
  if(start>=length||end <= start)
    return NULL;
  if(start<0)
    start = 0;
  if(end >= length)
    end = length-1;
  String* str = new_string();
  if(str){
      str->length = end - start + 1;
      str->str = (char*)malloc(str->length + 1);
      memset(str->str,0x0,str->length+1);
      strncpy(str->str,s+start,str->length);
  }
  return str;
}
