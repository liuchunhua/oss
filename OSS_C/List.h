/*
 * List.h
 *
 *  Created on: 2012-7-12
 *      Author: lch
 */

#ifndef LIST_H_
#define LIST_H_

#define for_each(node,list) for(node=list->next;list!=node;node=node->next)

//双向链表
struct _List
{
    void* ptr;
    struct _List* next;
    struct _List* prev;
};

//链表
typedef struct _List *List;

//链表节点
typedef List ListNode;

//链表操作类
typedef struct
{
    List (*init)();
    int (*size)(List);
    List (*add)(List, void *);
    int (*del)(List);
    void (*destroy)(List);
    void (*destroy_fun)(List, void (*free)(void *));
    int (*isEmpty)(List);
} ListOpration;

/**
 * 初始化链表
 */
List listInit();

/*
 * 添加数据到末尾
 */
List listAdd(List head, void* data);

/**
 * 删除数据
 */
int listDel(List data);

/**
 * 释放表,不释放数据
 */
void listFree(List head);

/*
 *  释放表和数据
 */
void listDestroy(List head, void (*free_fun)(void *));

/**
 *  判断链表是否为空
 */
int listIsEmpty(List head);

/**
 * 释放表中数据
 */
void listFreeObject(List head);

void listFreeObjectByFun(List head, void (*fun)(void*));

//链表数据个数
int listSize(List list);

//链表操作类
ListOpration ListClass;
#endif /* LIST_H_ */
