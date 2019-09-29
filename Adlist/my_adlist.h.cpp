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
