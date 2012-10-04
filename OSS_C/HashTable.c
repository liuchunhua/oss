/*
 * HashTable.c
 *
 *  Created on: 2012-9-24
 *      Author: lch
 */

#include "HashTable.h"
#include "List.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>

struct HashTable* hash_table_init() {
	struct HashTable* hash_table = (struct HashTable*)malloc(sizeof(struct HashTable));
	hash_table->hash = strlen;
	int i = 0;
	for (i = 0; i < HASH_TABLE_LEN; ++i) {
		hash_table->elements[i] = NULL;
		hash_table->keys[i] = NONE;
	}
	return hash_table;
}

int hash_table_put(struct HashTable* table, char* key, void* value) {
	assert(table->hash!=NULL);
	int v = (table->hash)(key);
	if (NULL != table->elements[v]) {
		if (table->keys[v] == SINGLE) {
			struct pair* key_value = (struct pair*) table->elements[v];
			table->elements[v] = listInit();
			listAdd((List) table->elements[v], key_value);
			table->keys[v] = LIST;
		}
		struct pair* new_pair = (struct pair*) malloc(sizeof(struct pair));
		new_pair->key = key;
		new_pair->value = value;
		listAdd((List) table->elements[v], new_pair);
		return 0;
	}
	table->keys[v] = SINGLE;
	table->elements[v] = (struct pair*)malloc(sizeof(struct pair));
	((struct pair*) (table->elements[v]))->key = key;
	((struct pair*) (table->elements[v]))->value = value;
	return 0;
}
void* hash_table_get(struct HashTable* table,char* key){
	int v = table->hash(key);
	if(table->keys[v]==NONE) return NULL;
	if(table->keys[v]==SINGLE){
		return ((struct pair*)table->elements[v])->value;
	}else{
		List list = (List)(table->elements[v]);
		List node = list->next;
		while(list!=node->next) {
			struct pair* map = (struct pair*)node->ptr;
			if(strcmp(key,map->key)==0)
				return ((struct pair*)(node->ptr))->value;
			node = node->next;
		}
	}
	return NULL;

}

static struct Owner* getOwner(xmlNodePtr node) {
	if (!strcmp((char*) node->name, "Owner")) {
		xmlNodePtr cur = node->children;
		struct Owner* owner = (struct Owner*) malloc(sizeof(struct Owner));
		assert(owner!=NULL);
		while (cur) {
			if (cur->type == XML_ELEMENT_NODE && !strcmp((char*)cur->name, "ID")) {
				owner->id = (char*)xmlNodeGetContent(cur);
			}
			if (cur->type == XML_ELEMENT_NODE && !strcmp((char*)cur->name, "DisplayName")) {
				owner->displayName = (char*)xmlNodeGetContent(cur);
			}
			cur = cur->next;
		}
		return owner;
	}
	return NULL ;
}
static struct Bucket* getBucket(xmlNodePtr node){
	if(!strcmp((char*)node->name,"Bucket")){
		xmlNodePtr cur = node->children;
		struct Bucket* bucket = (struct Bucket*)malloc(sizeof(struct Bucket));
		assert(bucket!=NULL);
		while(cur){
			if(cur->type==XML_ELEMENT_NODE&&!strcmp((char*)cur->name,"Name")){
				bucket->name = (char*)xmlNodeGetContent(cur);
			}
			if(cur->type==XML_ELEMENT_NODE&&!strcmp((char*)cur->name,"CreationDate")){
				bucket->creationDate = (char*)xmlNodeGetContent(cur);
			}
			cur = cur->next;
		}
		return bucket;
	}
	return NULL;
}
static List getListBucket(xmlNodePtr node){
	if(!strcmp((char*)node->name,"Buckets")){
		xmlNodePtr cur = node->children;
		List list = listInit();
		while(cur){
			if(cur->type==XML_ELEMENT_NODE&&!strcmp((char*)cur->name,"Bucket")){
				struct Bucket* bucket = getBucket(cur);
				if(bucket) listAdd(list,bucket);
			}
			cur = cur->next;
		}
		return list;
	}
	return NULL;
}

