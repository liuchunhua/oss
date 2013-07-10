/*
 * List.c
 *
 *  Created on: 2012-7-12
 *      Author: lch
 */

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "List.h"


List listInit(){
	List ls = (List)malloc(sizeof(List));
	ls->next=ls->prev=ls;
	return ls;
}

List listAdd(List head,void* data){
	assert(NULL!=head);
	List node = (List)malloc(sizeof(List));
	assert(node!=NULL);
	if(NULL==node){
		return NULL;
	}
	memset((void*)node,0x0,sizeof(List));
	node->ptr = data;
	head->prev->next = node;
	node->next = head;
	node->prev = head->prev;
	head->prev = node;
	return node;
}
/**
 *
 *
 */
int listDel(List data){
	data->prev->next = data->next;
	data->next->prev = data->prev;
	free(data);
	return 0;
}

void listFree(List head){
	listDestroy(head, NULL);
}

void listDestroy(List head, void (*free_fun)(void *)){
	List node = head->next;
	while(head&&node!=head){
		List t = node->next;
		if(free_fun != NULL)
			(*free_fun)(node->ptr);
		free(node);
		node = t;
	}
	free(head);
}
int listIsEmpty(List head){
	return head == head->next;
}

void listFreeObject(List head){
	List node = head->next;
	while(node&&node!=head){
		free(node->ptr);
		node = node->next;
	}
}

void listFreeObjectByFun(List head,void (*fun)(void*)){
        List node = head->next;
        while(node&&node!=head){
                (*fun)(node->ptr);
                node = node->next;
        }
}
