#include <stdlib.h>
#include "my_adlist.h"
#include "malloc.h"

 /*
  * 创建一个新的链表
  *
  * 创建成功返回链表，失败返回 NULL 。
  *
  * T = O(1)
  */
list *listCreate(void)
{
	struct list *list;

	// 分配内存
	if ((list = (struct list *)malloc(sizeof(*list))) == NULL)
		return NULL;

	// 初始化属性
	list->head = list->tail = NULL;
	list->len = 0;
	list->dup = NULL;
	list->free = NULL;
	list->match = NULL;

	return list;
}
