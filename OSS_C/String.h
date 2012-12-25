/*
 * String.h
 *
 *  Created on: 2012-12-6
 *      Author: lch
 */

#ifndef STRING_H_
#define STRING_H_

typedef struct String String;

struct String
{
  char* str;
  size_t length;
};

/*
 *return the position of the character in the String,not found return -1;
 */
String*
new_string();
void
free_string(String* s);
int
indexOf(const char* s, int c);

/*from the end to start ,firest position of the 'c'*/
int
lastIndexOf(const char* s, int c);

/*
 * String between 'start' and 'end'(include)
 */
char*
substring(const char* s, int start, int end);

char*
concat(int i, ...);

#endif /* STRING_H_ */
/*concat all string*/
