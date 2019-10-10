#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "my_intset.h"
#include "malloc.h"

//Ĭ�ϲ���С��
/* Note that these encodings are ordered, so:
 * INTSET_ENC_INT16 < INTSET_ENC_INT32 < INTSET_ENC_INT64. */
 /*
  * intset �ı��뷽ʽ
  */
#define INTSET_ENC_INT16 (sizeof(int16_t))
#define INTSET_ENC_INT32 (sizeof(int32_t))
#define INTSET_ENC_INT64 (sizeof(int64_t))

  /* Create an empty intset.
   *
   * ����������һ���µĿ���������
   *
   * T = O(1)
   */
intset *intsetNew(void) 
{

	// Ϊ�������Ͻṹ����ռ�
	intset *is = (intset *)malloc(sizeof(intset));

	// ���ó�ʼ����
	is->encoding = INTSET_ENC_INT16;

	// ��ʼ��Ԫ������
	is->length = 0;

	return is;
}