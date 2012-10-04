/*
 * base64.c
 *
 *  Created on: 2012-9-19
 *      Author: lch
 */

#include "base64.h"
#include <stdio.h>
#include <stdlib.h>

static const char base64_alphabet[] =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
/*
 * must free return
 */
char* base64_encode(const char* string, int len) {
	if(len > 2048){
		fprintf(stderr,"%s\n","data length is more than 2048");
	}
	int mod = len % 3;
	const char* p_cur = string;
	const char* p_end = string + len -2 -mod;
	char* out = (char*)malloc(len / 3 * 4 +5 + (len/3*4 % 76));
	char* out_head = out;
	unsigned char c1,c2,c3;
	int count = 0;
	while(p_cur < p_end){
		if(count > 18){
			*out++ = '\n';
			count = 0;
		}
		c1 = *p_cur++;
		c2 = *p_cur++;
		c3 = *p_cur++;
		*out++ = base64_alphabet[c1 >> 2];
		*out++ = base64_alphabet[((c1&0x3) << 4) |( c2 >> 4)];
		*out++ = base64_alphabet[(c2&0x0f)<<2|c3>>6];
		*out++ = base64_alphabet[c3&0x3f];
		count++;
	}
	switch (mod) {
		case 2:
			c1 = *p_cur++;
			c2 = *p_cur;
			*out++ = base64_alphabet[c1 >> 2];
			*out++ = base64_alphabet[((c1 & 0x3) <<4)|(c2 >> 4)];
			*out++ = base64_alphabet[(c2&0x0f) <<2];
			*out = '=';
			break;
		case 1:
			*out++ = base64_alphabet[*p_cur >> 2];
			*out++ = base64_alphabet[(*p_cur & 0x3) <<4];
			*out++ = '=';
			*out = '=';
			break;
		default:
			break;
	}
	*++out = '\0';
	return out_head;
}

