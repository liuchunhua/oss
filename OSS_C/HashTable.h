/*
 * HashTable.h
 *
 *  Created on: 2012-9-24
 *      Author: lch
 */

#ifndef HASHTABLE_H_
#define HASHTABLE_H_

#include <stddef.h>
#include "List.h"

#define HASH_TABLE_LEN 50

typedef enum{NONE,SINGLE,LIST} HASHKEY;


struct HashTable{
	size_t (*hash)(const char*);
	void *elements[HASH_TABLE_LEN];
	HASHKEY keys[HASH_TABLE_LEN];
};

typedef struct HashTable HashTable;

struct pair{
	void *key;
	void *value;
};

typedef struct {
	 HashTable *(*init)();
	 HashTable *(*init_size)(unsigned int);
	 void (*destroy)(HashTable *);
	 void (*destroy_fun)(HashTable *, void (*free)(void *));
	 struct pair *(*get)(HashTable *, const char *key);
	 int (*put)(HashTable *, const char *key, void *value);
	 List (*get_list)(HashTable *, const char *key);
	 List (*get_all)(HashTable *);
}HashTableOpration;
/*
 *初始化hash table
*/
struct HashTable *hash_table_init();

struct HashTable *hash_table_init_size(unsigned int size );

/*
 *添加key-value
*/
int hash_table_put(struct HashTable *table, const char *key, void *value);

/*
 *获得key-value
*/
struct pair *hash_table_get(struct HashTable *table,const char *key);
	
//获得key的list值
List hash_table_get_list(struct HashTable *table,const char *key);

//获得所有key-value
List hash_table_get_all(struct HashTable *table);

void hash_table_free(struct HashTable *table);
void hash_table_free_fun(HashTable *table, void (*free)(void *));

HashTableOpration HashTableClass = {
	.init = hash_table_init,
	.init_size = hash_table_init_size,
	.destroy = hash_table_free,
	.destroy_fun = hash_table_free_fun,
	.get = hash_table_get,
	.get_list = hash_table_get_list,
	.get_all = hash_table_get_all
};
#endif /* HASHTABLE_H_ */
