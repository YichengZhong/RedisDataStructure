#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "my_intset.h"
#include "malloc.h"

//默认采用小端
/* Note that these encodings are ordered, so:
 * INTSET_ENC_INT16 < INTSET_ENC_INT32 < INTSET_ENC_INT64. */
 /*
  * intset 的编码方式
  */
#define INTSET_ENC_INT16 (sizeof(int16_t))
#define INTSET_ENC_INT32 (sizeof(int32_t))
#define INTSET_ENC_INT64 (sizeof(int64_t))

  /* Create an empty intset.
   *
   * 创建并返回一个新的空整数集合
   *
   * T = O(1)
   */
intset *intsetNew(void) 
{

	// 为整数集合结构分配空间
	intset *is = (intset *)malloc(sizeof(intset));

	// 设置初始编码
	is->encoding = INTSET_ENC_INT16;

	// 初始化元素数量
	is->length = 0;

	return is;
}