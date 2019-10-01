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

/*
 * ������ list ��ɾ�������ڵ� node 
 * 
 * �Խڵ�˽��ֵ(private value of the node)���ͷŹ����ɵ����߽��С�
 *
 * T = O(1)
 */
void listDelNode(list *list, listNode *node)
{
    // ����ǰ�ýڵ��ָ��
    if (node->prev)
        node->prev->next = node->next;
    else
        list->head = node->next;

    // �������ýڵ��ָ��
    if (node->next)
        node->next->prev = node->prev;
    else
        list->tail = node->prev;

    // �ͷ�ֵ
    if (list->free) list->free(node->value);

    // �ͷŽڵ�
    free(node);

    // ��������һ
    list->len--;
}

/*
 * Ϊ����������һ����������
 * ֮��ÿ�ζ�������������� listNext �����ر�������������ڵ�
 *
 * direction ���������˵������ĵ�������
 *  AL_START_HEAD ���ӱ�ͷ���β����
 *  AL_START_TAIL ���ӱ�β���ͷ����
 *
 * T = O(1)
 */
listIter *listGetIterator(list *list, int direction)
{
    // Ϊ�����������ڴ�
    listIter *iter;
    if ((iter = (listIter *)malloc(sizeof(*iter))) == NULL) return NULL;

    // ���ݵ����������õ���������ʼ�ڵ�
    if (direction == AL_START_HEAD)
        iter->next = list->head;
    else
        iter->next = list->tail;

    // ��¼��������
    iter->direction = direction;

    return iter;
}

/*
 * ���ص�������ǰ��ָ��Ľڵ㡣
 *
 * ɾ����ǰ�ڵ�������ģ��������޸�������������ڵ㡣
 *
 * ����Ҫô����һ���ڵ㣬Ҫô���� NULL ���������÷��ǣ�
 *
 * iter = listGetIterator(list,<direction>);
 * while ((node = listNext(iter)) != NULL) {
 *     doSomethingWith(listNodeValue(node));
 * }
 *
 * T = O(1)
 */
listNode *listNext(listIter *iter)
{
    listNode *current = iter->next;

    if (current != NULL) {
        // ���ݷ���ѡ����һ���ڵ�
        if (iter->direction == AL_START_HEAD)
            // ������һ���ڵ㣬��ֹ��ǰ�ڵ㱻ɾ�������ָ�붪ʧ
            iter->next = current->next;
        else
            // ������һ���ڵ㣬��ֹ��ǰ�ڵ㱻ɾ�������ָ�붪ʧ
            iter->next = current->prev;
    }

    return current;
}

/*
 * �ͷŵ�����
 *
 * T = O(1)
 */
void listReleaseIterator(listIter *iter) 
{
    free(iter);
	iter=NULL;
}

/*
 * ������������
 *
 * ���Ƴɹ�������������ĸ�����
 * �����Ϊ�ڴ治�����ɸ���ʧ�ܣ����� NULL ��
 *
 * �������������ֵ���ƺ��� dup ����ô��ֵ�ĸ��ƽ�ʹ�ø��ƺ������У�
 * �����½ڵ㽫�;ɽڵ㹲��ͬһ��ָ�롣
 *
 * ���۸����ǳɹ�����ʧ�ܣ�����ڵ㶼�����޸ġ�
 *
 * T = O(N)
 */
list *listDup(list *orig)
{
    list *copy;
    listIter *iter;
    listNode *node;

    // ����������
    if ((copy = listCreate()) == NULL)
        return NULL;

    // ���ýڵ�ֵ������
    copy->dup = orig->dup;
    copy->free = orig->free;
    copy->match = orig->match;

    // ����������������
    iter = listGetIterator(orig, AL_START_HEAD);
    while((node = listNext(iter)) != NULL)
	{
        void *value;

        // ���ƽڵ�ֵ���½ڵ�
        if (copy->dup) 
		{
            value = copy->dup(node->value);
            if (value == NULL) 
			{
                listRelease(copy);
                listReleaseIterator(iter);
                return NULL;
            }
        } 
		else
		{
			value = node->value;
		}

        // ���ڵ���ӵ�����
        if (listAddNodeTail(copy, value) == NULL)
		{
            listRelease(copy);
            listReleaseIterator(iter);
            return NULL;
        }
    }

    // �ͷŵ�����
    listReleaseIterator(iter);

    // ���ظ���
    return copy;
}

/* 
 * �������� list ��ֵ�� key ƥ��Ľڵ㡣
 * 
 * �ԱȲ���������� match ����������У�
 * ���û������ match ������
 * ��ôֱ��ͨ���Ա�ֵ��ָ���������Ƿ�ƥ�䡣
 *
 * ���ƥ��ɹ�����ô��һ��ƥ��Ľڵ�ᱻ���ء�
 * ���û��ƥ���κνڵ㣬��ô���� NULL ��
 *
 * T = O(N)
 */
listNode *listSearchKey(list *list, void *key)
{
    listIter *iter;
    listNode *node;

    // ������������
    iter = listGetIterator(list, AL_START_HEAD);
    while((node = listNext(iter)) != NULL) 
	{
        
        // �Ա�
        if (list->match)
		{
            if (list->match(node->value, key))
			{
                listReleaseIterator(iter);
                // �ҵ�
                return node;
            }
        } 
		else 
		{
            if (key == node->value) 
			{
                listReleaseIterator(iter);
                // �ҵ�
                return node;
            }
        }
    }
    
    listReleaseIterator(iter);

    // δ�ҵ�
    return NULL;
}

/*
 * ���������ڸ��������ϵ�ֵ��
 *
 * ������ 0 Ϊ��ʼ��Ҳ�����Ǹ����� -1 ��ʾ�������һ���ڵ㣬������ࡣ
 *
 * �������������Χ��out of range�������� NULL ��
 *
 * T = O(N)
 */
listNode *listIndex(list *list, long index) 
{
    listNode *n;

    // �������Ϊ�������ӱ�β��ʼ����
    if (index < 0) 
	{
        index = (-index)-1;
        n = list->tail;
        while(index-- && n) n = n->prev;
    // �������Ϊ�������ӱ�ͷ��ʼ����
    } else 
	{
        n = list->head;
        while(index-- && n) n = n->next;
    }

    return n;
}

/*
 * ���������ķ�������Ϊ AL_START_HEAD ��
 * ��������ָ������ָ���ͷ�ڵ㡣
 *
 * T = O(1)
 */
void listRewind(list *list, listIter *li) {
    li->next = list->head;
    li->direction = AL_START_HEAD;
}

/*
 * ���������ķ�������Ϊ AL_START_TAIL ��
 * ��������ָ������ָ���β�ڵ㡣
 *
 * T = O(1)
 */
void listRewindTail(list *list, listIter *li)
{
    li->next = list->tail;
    li->direction = AL_START_TAIL;
}

/*
 * ȡ������ı�β�ڵ㣬�������ƶ�����ͷ����Ϊ�µı�ͷ�ڵ㡣
 *
 * T = O(1)
 */
void listRotate(list *list) 
{
    listNode *tail = list->tail;

    if (listLength(list) <= 1) return;

    /* Detach current tail */
    // ȡ����β�ڵ�
    list->tail = tail->prev;
    list->tail->next = NULL;

    /* Move it as head */
    // ���뵽��ͷ
    list->head->prev = tail;
    tail->prev = NULL;
    tail->next = list->head;
    list->head = tail;
}

//�ж������Ƿ�Ϊ��
//T(1)
bool listIsEmpty(list *list)
{
	if (0==listLength(list))
	{
		return true;
	}

	return false;
}

//����resize(n)��list�ĳ��ȸ�Ϊֻ����n��Ԫ�أ�������Ԫ�ؽ���ɾ����
//���n��listԭ���ĳ��ȳ�����ôĬ�ϳ����Ĳ���Ԫ����Ϊ0��
list * listResize(list *list,int n)
{
	//����
	if(NULL==list || 0==n)
	{
		return NULL;
	}

	if (n==listLength(list))
	{
		return list;
	}

	if (n<listLength(list))
	{

	} 
	else
	{

	}
}