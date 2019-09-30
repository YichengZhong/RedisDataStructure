#include <stdlib.h>
#include "my_adlist.h"
#include "malloc.h"

 /*
  * ����һ���µ�����
  *
  * �����ɹ���������ʧ�ܷ��� NULL ��
  *
  * T = O(1)
  */
list *listCreate(void)
{
	struct list *list;

	// �����ڴ�
	if ((list = (struct list *)malloc(sizeof(*list))) == NULL)
		return NULL;

	// ��ʼ������
	list->head = list->tail = NULL;
	list->len = 0;
	list->dup = NULL;
	list->free = NULL;
	list->match = NULL;

	return list;
}

/*
 * �ͷ����������Լ����������нڵ�
 *
 * T = O(N)
 */
void listRelease(list *list)
{
	unsigned long len;
	listNode *current, *next;

	// ָ��ͷָ��
	current = list->head;
	// ������������
	len = list->len;
	while (len--) 
	{
		next = current->next;

		// ���������ֵ�ͷź�������ô������
		if (list->free) list->free(current->value);

		// �ͷŽڵ�ṹ
		free(current);

		current = next;
	}

	// �ͷ�����ṹ
	free(list);
}

/*
 * ��һ�������и���ֵָ�� value ���½ڵ���ӵ�����ı�ͷ
 *
 * ���Ϊ�½ڵ�����ڴ������ô��ִ���κζ����������� NULL
 *
 * ���ִ�гɹ������ش��������ָ��
 *
 * T = O(1)
 */
list *listAddNodeHead(list *list, void *value)
{
	listNode *node;

	// Ϊ�ڵ�����ڴ�
	if ((node = (listNode *)malloc(sizeof(*node))) == NULL)
		return NULL;

	// ����ֵָ��
	node->value = value;

	// ��ӽڵ㵽������
	if (list->len == 0) 
	{
		list->head = list->tail = node;
		node->prev = node->next = NULL;
	}
	else 
	{
		// ��ӽڵ㵽�ǿ�����
		node->prev = NULL;
		node->next = list->head;
		list->head->prev = node;
		list->head = node;
	}

	// ��������ڵ���
	list->len++;

	return list;
}

/*
 * ��һ�������и���ֵָ�� value ���½ڵ���ӵ�����ı�β
 *
 * ���Ϊ�½ڵ�����ڴ������ô��ִ���κζ����������� NULL
 *
 * ���ִ�гɹ������ش��������ָ��
 *
 * T = O(1)
 */
list *listAddNodeTail(list *list, void *value)
{
	listNode *node;

	// Ϊ�½ڵ�����ڴ�
	if ((node = (listNode *)malloc(sizeof(*node))) == NULL)
		return NULL;

	// ����ֵָ��
	node->value = value;

	// Ŀ������Ϊ��
	if (list->len == 0) 
	{
		list->head = list->tail = node;
		node->prev = node->next = NULL;
	}
	else 
	{
		// Ŀ������ǿ�
		node->prev = list->tail;
		node->next = NULL;
		list->tail->next = node;
		list->tail = node;
	}

	// ��������ڵ���
	list->len++;

	return list;
}

/*
 * ����һ������ֵ value ���½ڵ㣬���������뵽 old_node ��֮ǰ��֮��
 *
 * ��� after Ϊ 0 �����½ڵ���뵽 old_node ֮ǰ��
 * ��� after Ϊ 1 �����½ڵ���뵽 old_node ֮��
 *
 * T = O(1)
 */
list *listInsertNode(list *list, listNode *old_node, void *value, int after) 
{
	listNode *node;

	// �����½ڵ�
	if ((node = (listNode *)malloc(sizeof(*node))) == NULL)
		return NULL;

	// ����ֵ
	node->value = value;

	// ���½ڵ���ӵ������ڵ�֮��
	if (after)
	{
		node->prev = old_node;
		node->next = old_node->next;
		// �����ڵ���ԭ��β�ڵ�
		if (list->tail == old_node)
		{
			list->tail = node;
		}
		// ���½ڵ���ӵ������ڵ�֮ǰ
	}
	else 
	{
		node->next = old_node;
		node->prev = old_node->prev;
		// �����ڵ���ԭ��ͷ�ڵ�
		if (list->head == old_node) 
		{
			list->head = node;
		}
	}

	// �����½ڵ��ǰ��ָ��
	if (node->prev != NULL) 
	{
		node->prev->next = node;
	}
	// �����½ڵ�ĺ���ָ��
	if (node->next != NULL) 
	{
		node->next->prev = node;
	}

	// ��������ڵ���
	list->len++;

	return list;
}