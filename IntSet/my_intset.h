#pragma once

#include <stdint.h>

void memrev16(void *p);
void memrev32(void *p);
void memrev64(void *p);
uint16_t intrev16(uint16_t v);
uint32_t intrev32(uint32_t v);
uint64_t intrev64(uint64_t v);

//默认采用小端
/* variants of the function doing the actual convertion only if the target
 * host is big endian */
#define memrev16ifbe(p)
#define memrev32ifbe(p)
#define memrev64ifbe(p)
#define intrev16ifbe(v) (v)
#define intrev32ifbe(v) (v)
#define intrev64ifbe(v) (v)

 /* The functions htonu64() and ntohu64() convert the specified value to
  * network byte ordering and back. In big endian systems they are no-ops. */
#define htonu64(v) intrev64(v)
#define ntohu64(v) intrev64(v)

typedef struct intset {

	// 编码方式
	uint32_t encoding;

	// 集合包含的元素数量
	uint32_t length;

	// 保存元素的数组
	int8_t contents[];

} intset;

intset *intsetNew(void);
intset *intsetAdd(intset *is, int64_t value, uint8_t *success);
intset *intsetRemove(intset *is, int64_t value, int *success);
uint8_t intsetFind(intset *is, int64_t value);
int64_t intsetRandom(intset *is);
uint8_t intsetGet(intset *is, uint32_t pos, int64_t *value);
uint32_t intsetLen(intset *is);
size_t intsetBlobLen(intset *is);