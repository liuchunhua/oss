/*
 * HashTable.h
 *
 *  Created on: 2012-9-24
 *      Author: lch
 */

#ifndef HASHTABLE_H_
#define HASHTABLE_H_

#include <stddef.h>

#define HASH_TABLE_LEN 50

typedef enum{NONE,SINGLE,LIST} HASHKEY;


struct HashTable{
	size_t (*hash)(const char*);
	void* elements[HASH_TABLE_LEN];
	HASHKEY keys[HASH_TABLE_LEN];
};

struct pair{
	void* key;
	void* value;
};

/*
* 初始化hash table
*/
struct HashTable*  hash_table_init();

/*
* 添加key-value
*/
int hash_table_put(struct HashTable* table, char* key, void* value);

/*
* 获得key-value
*/

void* hash_table_get(struct HashTable* table,char* key);

#endif /* HASHTABLE_H_ */
