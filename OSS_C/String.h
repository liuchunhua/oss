/*
 * String.h
 *
 *  Created on: 2012-12-6
 *      Author: lch
 */

#ifndef STRING_H_
#define STRING_H_

typedef struct String String;

struct String{
  char* str;
  size_t length;
};

/*
 *return the position of the character in the String,not found return -1;
 */

int indexOf(const char* s,int c);
/*
 * String between 'start' and 'end'(include)
 */
String* substring(const char* s,int start, int end);


#endif /* STRING_H_ */
