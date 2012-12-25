/*
 * List.h
 *
 *  Created on: 2012-7-12
 *      Author: lch
 */

#ifndef LIST_H_
#define LIST_H_

struct _List{
	void* ptr;
	struct _List* next;
	struct _List* prev;
};
typedef struct _list  ListNode;
typedef struct _List* List;

#define for_each(node,list) for(node=list->next;list!=node;node=node->next)

/**
 * 初始化链表
 */
List listInit();
/*
 * 添加数据到末尾
 */
List  listAdd(List head,void* data);
/**
 * 删除数据
 */
int listDel(List data);
/**
 * 清空列表
 */
void listFree(List head);
/**
 *
 */
int listIsEmpty(List head);
/**
 * 释放分配内存
 */
void listFreeObject(List head);
void listFreeObjectByFun(List head,void (*fun)(void*));
#endif /* LIST_H_ */
