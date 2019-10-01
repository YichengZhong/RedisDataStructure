#pragma once

 /*
  * 双端链表节点
  */
typedef struct listNode {

	// 前置节点
	struct listNode *prev;

	// 后置节点
	struct listNode *next;

	// 节点的值
	void *value;

} listNode;

/*
 * 双端链表迭代器
 */
typedef struct listIter {

	// 当前迭代到的节点
	listNode *next;

	// 迭代的方向
	int direction;

} listIter;

/*
 * 双端链表结构
 */
typedef struct list {

	// 表头节点
	listNode *head;

	// 表尾节点
	listNode *tail;

	// 节点值复制函数
	void *(*dup)(void *ptr);

	// 节点值释放函数
	void(*free)(void *ptr);

	// 节点值对比函数
	int(*match)(void *ptr, void *key);

	// 链表所包含的节点数量
	unsigned long len;

} list;

/* Functions implemented as macros */
// 返回给定链表所包含的节点数量
// T = O(1)
#define listLength(l) ((l)->len)
// 返回给定链表的表头节点
// T = O(1)
#define listFirst(l) ((l)->head)
// 返回给定链表的表尾节点
// T = O(1)
#define listLast(l) ((l)->tail)
// 返回给定节点的前置节点
// T = O(1)
#define listPrevNode(n) ((n)->prev)
// 返回给定节点的后置节点
// T = O(1)
#define listNextNode(n) ((n)->next)
// 返回给定节点的值
// T = O(1)
#define listNodeValue(n) ((n)->value)

// 将链表 l 的值复制函数设置为 m
// T = O(1)
#define listSetDupMethod(l,m) ((l)->dup = (m))
// 将链表 l 的值释放函数设置为 m
// T = O(1)
#define listSetFreeMethod(l,m) ((l)->free = (m))
// 将链表的对比函数设置为 m
// T = O(1)
#define listSetMatchMethod(l,m) ((l)->match = (m))

// 返回给定链表的值复制函数
// T = O(1)
#define listGetDupMethod(l) ((l)->dup)
// 返回给定链表的值释放函数
// T = O(1)
#define listGetFree(l) ((l)->free)
// 返回给定链表的值对比函数
// T = O(1)
#define listGetMatchMethod(l) ((l)->match)

/* Prototypes */
//创建一个空的链表
list *listCreate(void);

//释放链表，以及链表节点
void listRelease(list *list);

//在链表头新增一个节点
list *listAddNodeHead(list *list, void *value);

//在链表尾新增一个节点
list *listAddNodeTail(list *list, void *value);

//在old_node前或者后新增节点
list *listInsertNode(list *list, listNode *old_node, void *value, int after);

//删除指定节点
void listDelNode(list *list, listNode *node);

//为给定链表创建一个迭代器
listIter *listGetIterator(list *list, int direction);

//返回迭代器当前所指向的节点
listNode *listNext(listIter *iter);

//释放迭代器
void listReleaseIterator(listIter *iter);

//复制整个链表
list *listDup(list *orig);

//查找链表 list 中值和 key 匹配的节点
listNode *listSearchKey(list *list, void *key);

//返回链表在给定索引上的值
listNode *listIndex(list *list, long index);


void listRewind(list *list, listIter *li);
void listRewindTail(list *list, listIter *li);

//取出链表的表尾节点，并将它移动到表头，成为新的表头节点。
void listRotate(list *list);

/* Directions for iterators
 *
 * 迭代器进行迭代的方向
 */
 // 从表头向表尾进行迭代
#define AL_START_HEAD 0
// 从表尾到表头进行迭代
#define AL_START_TAIL 1

