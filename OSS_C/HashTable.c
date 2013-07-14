/*
 *HashTable.c
 *
 * Created on: 2012-9-24
 *     Author: lch
 */

#include "HashTable.h"
#include "List.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>

HashTableOpration HashTableClass = {
        .init = hash_table_init,
        .init_size = hash_table_init_size,
        .destroy = hash_table_free,
        .destroy_fun = hash_table_free_fun,
        .put = hash_table_put,
        .get = hash_table_get,
        .get_list = hash_table_get_list,
        .get_all = hash_table_get_all
};

static void free_pair(struct pair *p){
	if(p){
		free(p->key);
		free(p);
	}
}

struct HashTable *hash_table_init_size(unsigned int size) {
	if(size < HASH_TABLE_LEN) size = HASH_TABLE_LEN;
	struct HashTable *hash_table = (struct HashTable*) malloc(sizeof(struct HashTable));
	memset(hash_table,0x0,sizeof(struct HashTable));
	hash_table->hash = strlen;
	int i = 0;
	for (i = 0; i < HASH_TABLE_LEN; ++i) {
		hash_table->elements[i] = NULL;
		hash_table->keys[i] = NONE;
	}
	return hash_table;
}
struct HashTable *hash_table_init(){
	return hash_table_init_size(0);
}
int hash_table_put(struct HashTable *table,const char *key, void *value) {
	assert(table->hash!=NULL);
	int v = (table->hash)(key)%HASH_TABLE_LEN;
	if (NULL != table->elements[v]) {
		if (table->keys[v] == SINGLE) {//将单点改为链表
			struct pair *key_value = (struct pair*) table->elements[v];
			table->elements[v] = listInit();
			listAdd((List) table->elements[v], key_value);
			table->keys[v] = LIST;
		}
		struct pair *new_pair = (struct pair*) malloc(sizeof(struct pair));
		new_pair->key = strdup(key);
		new_pair->value = value;
		listAdd((List) table->elements[v], new_pair);
		return 0;
	}
	table->keys[v] = SINGLE;
	table->elements[v] = (struct pair*) malloc(sizeof(struct pair));
	((struct pair*) (table->elements[v]))->key = strdup(key);
	((struct pair*) (table->elements[v]))->value = value;
	return 0;
}
struct pair *hash_table_get(struct HashTable *table,const char *key) {
	int v = (table->hash)(key)%HASH_TABLE_LEN;
	if (table->keys[v] == NONE)
		return NULL ;
	if (table->keys[v] == SINGLE) {
		return ((struct pair*)table->elements[v])->value;
	} else {
		List list = (List) (table->elements[v]);
		List node = list->next;
		while (list != node->next) {
			struct pair *map = (struct pair*) node->ptr;
			if (strcmp(key, map->key) == 0)
				return ((struct pair*) (node->ptr))->value;
			node = node->next;
		}
	}
	return NULL ;

}

List hash_table_get_list(struct HashTable *table,const char *key)
{
	List ls = listInit();
	int v = (table->hash)(key)%HASH_TABLE_LEN;
	if (table->keys[v] == NONE)
		return NULL ;
	if (table->keys[v] == SINGLE) {
		struct pair *pair = table->elements[v];
		listAdd(ls,pair->value);
		return ls;
	} else {
		List list = (List) (table->elements[v]);
		List node = list->next;
		while (list != node->next) {
			struct pair *map = (struct pair*) node->ptr;
			if (strcmp(key, map->key) == 0)
				listAdd(ls, ((struct pair*) (node->ptr))->value);
			node = node->next;
		}
		return ls;
	}

}
List hash_table_get_key_list(struct HashTable *table)
{
  return hash_table_get_all(table);
}
List hash_table_get_all(struct HashTable *table) {
	int i = 0;
	List list = listInit();
	for (; i < HASH_TABLE_LEN; i++) {
		switch (table->keys[i]) {
			case NONE:
				continue;
				break;
			case SINGLE:
				{
					struct pair *p = (struct pair*) (table->elements[i]);
					listAdd(list, (void*) p);
					break;
				}
			case LIST:
				{
					List ls = (List) (table->elements[i]);
					List node;
					for_each(node,ls)
					{
						listAdd(list, node->ptr);
					}
					break;
				}
			default:
				break;
		}
	}
	return list;
}

void hash_table_free(struct HashTable *table){
    hash_table_free_fun(table, NULL);
}
void hash_table_free_fun(HashTable *table, void (*free_fun)(void *)){
	int i = 0;
	for (; i < HASH_TABLE_LEN; i++) {
		switch (table->keys[i]) {
			case NONE:
				continue;
				break;
			case SINGLE:
    			{
    				struct pair *pair = table->elements[i]; 
    				if(free_fun != NULL)
    					(*free_fun)(pair->value);
    				free_pair(pair);
    				break;
    			}
			case LIST:
				{
					List ls = (List) (table->elements[i]);
					List node;
					for_each(node,ls)
					{
        				struct pair *pair = table->elements[i]; 
        				if(free_fun != NULL)
        					(*free_fun)(pair->value);
        				free_pair(pair);
					}
					listFree(ls);
					break;
				}
			default:
				break;
		}
	}
	free(table);
}
