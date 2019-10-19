#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include "malloc.h"
#include "my_ziplist.h"
#include "endianconv.h"

/*
 * ziplist 末端标识符，以及 5 字节长长度标识符
 */
#define ZIP_END 255
#define ZIP_BIGLEN 254

 /* Different encoding/length possibilities */
 /*
  * 字符串编码和整数编码的掩码
  */
#define ZIP_STR_MASK 0xc0
#define ZIP_INT_MASK 0x30

/*
* 字符串编码类型
*/
#define ZIP_STR_06B (0 << 6)
#define ZIP_STR_14B (1 << 6)
#define ZIP_STR_32B (2 << 6)

/*
* 整数编码类型
*/
#define ZIP_INT_16B (0xc0 | 0<<4)
#define ZIP_INT_32B (0xc0 | 1<<4)
#define ZIP_INT_64B (0xc0 | 2<<4)
#define ZIP_INT_24B (0xc0 | 3<<4)
#define ZIP_INT_8B 0xfe

/* 4 bit integer immediate encoding
*
* 4 位整数编码的掩码和类型
*/
#define ZIP_INT_IMM_MASK 0x0f
#define ZIP_INT_IMM_MIN 0xf1    /* 11110001 */
#define ZIP_INT_IMM_MAX 0xfd    /* 11111101 */
#define ZIP_INT_IMM_VAL(v) (v & ZIP_INT_IMM_MASK)

/*
* 24 位整数的最大值和最小值
*/
#define INT24_MAX 0x7fffff
#define INT24_MIN (-INT24_MAX - 1)

/* Macro to determine type
*
* 查看给定编码 enc 是否字符串编码
*/
#define ZIP_IS_STR(enc) (((enc) & ZIP_STR_MASK) < ZIP_STR_MASK)

/* Utility macros */
/*
* ziplist 属性宏
*/
// 定位到 ziplist 的 bytes 属性，该属性记录了整个 ziplist 所占用的内存字节数
// 用于取出 bytes 属性的现有值，或者为 bytes 属性赋予新值
#define ZIPLIST_BYTES(zl)       (*((uint32_t*)(zl)))
// 定位到 ziplist 的 offset 属性，该属性记录了到达表尾节点的偏移量
// 用于取出 offset 属性的现有值，或者为 offset 属性赋予新值
#define ZIPLIST_TAIL_OFFSET(zl) (*((uint32_t*)((zl)+sizeof(uint32_t))))
// 定位到 ziplist 的 length 属性，该属性记录了 ziplist 包含的节点数量
// 用于取出 length 属性的现有值，或者为 length 属性赋予新值
#define ZIPLIST_LENGTH(zl)      (*((uint16_t*)((zl)+sizeof(uint32_t)*2)))
// 返回 ziplist 表头的大小
#define ZIPLIST_HEADER_SIZE     (sizeof(uint32_t)*2+sizeof(uint16_t))
// 返回指向 ziplist 第一个节点（的起始位置）的指针
#define ZIPLIST_ENTRY_HEAD(zl)  ((zl)+ZIPLIST_HEADER_SIZE)
// 返回指向 ziplist 最后一个节点（的起始位置）的指针
#define ZIPLIST_ENTRY_TAIL(zl)  ((zl)+intrev32ifbe(ZIPLIST_TAIL_OFFSET(zl)))
// 返回指向 ziplist 末端 ZIP_END （的起始位置）的指针
#define ZIPLIST_ENTRY_END(zl)   ((zl)+intrev32ifbe(ZIPLIST_BYTES(zl))-1)

/*
 * 保存 ziplist 节点信息的结构
 */
typedef struct zlentry {

	// prevrawlen ：前置节点的长度
	// prevrawlensize ：编码 prevrawlen 所需的字节大小
	unsigned int prevrawlensize, prevrawlen;

	// len ：当前节点值的长度
	// lensize ：编码 len 所需的字节大小
	unsigned int lensize, len;

	// 当前节点 header 的大小
	// 等于 prevrawlensize + lensize
	unsigned int headersize;

	// 当前节点值所使用的编码类型
	unsigned char encoding;

	// 指向当前节点的指针
	unsigned char *p;

} zlentry;

/* Insert item at "p". */
/*
 * 根据指针 p 所指定的位置，将长度为 slen 的字符串 s 插入到 zl 中。
 *
 * 函数的返回值为完成插入操作之后的 ziplist
 *
 * T = O(N^2)
 */
static unsigned char *__ziplistInsert(unsigned char *zl, unsigned char *p, unsigned char *s, unsigned int slen) {
	// 记录当前 ziplist 的长度
	size_t curlen = intrev32ifbe(ZIPLIST_BYTES(zl)), reqlen, prevlen = 0;
	size_t offset;
	int nextdiff = 0;
	unsigned char encoding = 0;
	long long value = 123456789; /* initialized to avoid warning. Using a value
									that is easy to see if for some reason
									we use it uninitialized. */
	zlentry entry, tail;

	/* Find out prevlen for the entry that is inserted. */
	if (p[0] != ZIP_END) {
		// 如果 p[0] 不指向列表末端，说明列表非空，并且 p 正指向列表的其中一个节点
		// 那么取出 p 所指向节点的信息，并将它保存到 entry 结构中
		// 然后用 prevlen 变量记录前置节点的长度
		// （当插入新节点之后 p 所指向的节点就成了新节点的前置节点）
		// T = O(1)
		entry = zipEntry(p);
		prevlen = entry.prevrawlen;
	}
	else {
		// 如果 p 指向表尾末端，那么程序需要检查列表是否为：
		// 1)如果 ptail 也指向 ZIP_END ，那么列表为空；
		// 2)如果列表不为空，那么 ptail 将指向列表的最后一个节点。
		unsigned char *ptail = ZIPLIST_ENTRY_TAIL(zl);
		if (ptail[0] != ZIP_END) {
			// 表尾节点为新节点的前置节点

			// 取出表尾节点的长度
			// T = O(1)
			prevlen = zipRawEntryLength(ptail);
		}
	}

	/* See if the entry can be encoded */
	// 尝试看能否将输入字符串转换为整数，如果成功的话：
	// 1)value 将保存转换后的整数值
	// 2)encoding 则保存适用于 value 的编码方式
	// 无论使用什么编码， reqlen 都保存节点值的长度
	// T = O(N)
	if (zipTryEncoding(s, slen, &value, &encoding)) {
		/* 'encoding' is set to the appropriate integer encoding */
		reqlen = zipIntSize(encoding);
	}
	else {
		/* 'encoding' is untouched, however zipEncodeLength will use the
		 * string length to figure out how to encode it. */
		reqlen = slen;
	}
	/* We need space for both the length of the previous entry and
	 * the length of the payload. */
	 // 计算编码前置节点的长度所需的大小
	 // T = O(1)
	reqlen += zipPrevEncodeLength(NULL, prevlen);
	// 计算编码当前节点值所需的大小
	// T = O(1)
	reqlen += zipEncodeLength(NULL, encoding, slen);

	/* When the insert position is not equal to the tail, we need to
	 * make sure that the next entry can hold this entry's length in
	 * its prevlen field. */
	 // 只要新节点不是被添加到列表末端，
	 // 那么程序就需要检查看 p 所指向的节点（的 header）能否编码新节点的长度。
	 // nextdiff 保存了新旧编码之间的字节大小差，如果这个值大于 0 
	 // 那么说明需要对 p 所指向的节点（的 header ）进行扩展
	 // T = O(1)
	nextdiff = (p[0] != ZIP_END) ? zipPrevLenByteDiff(p, reqlen) : 0;

	/* Store offset because a realloc may change the address of zl. */
	// 因为重分配空间可能会改变 zl 的地址
	// 所以在分配之前，需要记录 zl 到 p 的偏移量，然后在分配之后依靠偏移量还原 p 
	offset = p - zl;
	// curlen 是 ziplist 原来的长度
	// reqlen 是整个新节点的长度
	// nextdiff 是新节点的后继节点扩展 header 的长度（要么 0 字节，要么 4 个字节）
	// T = O(N)
	zl = ziplistResize(zl, curlen + reqlen + nextdiff);
	p = zl + offset;

	/* Apply memory move when necessary and update tail offset. */
	if (p[0] != ZIP_END) {
		// 新元素之后还有节点，因为新元素的加入，需要对这些原有节点进行调整

		/* Subtract one because of the ZIP_END bytes */
		// 移动现有元素，为新元素的插入空间腾出位置
		// T = O(N)
		memmove(p + reqlen, p - nextdiff, curlen - offset - 1 + nextdiff);

		/* Encode this entry's raw length in the next entry. */
		// 将新节点的长度编码至后置节点
		// p+reqlen 定位到后置节点
		// reqlen 是新节点的长度
		// T = O(1)
		zipPrevEncodeLength(p + reqlen, reqlen);

		/* Update offset for tail */
		// 更新到达表尾的偏移量，将新节点的长度也算上
		ZIPLIST_TAIL_OFFSET(zl) =
			intrev32ifbe(intrev32ifbe(ZIPLIST_TAIL_OFFSET(zl)) + reqlen);

		/* When the tail contains more than one entry, we need to take
		 * "nextdiff" in account as well. Otherwise, a change in the
		 * size of prevlen doesn't have an effect on the *tail* offset. */
		 // 如果新节点的后面有多于一个节点
		 // 那么程序需要将 nextdiff 记录的字节数也计算到表尾偏移量中
		 // 这样才能让表尾偏移量正确对齐表尾节点
		 // T = O(1)
		tail = zipEntry(p + reqlen);
		if (p[reqlen + tail.headersize + tail.len] != ZIP_END) {
			ZIPLIST_TAIL_OFFSET(zl) =
				intrev32ifbe(intrev32ifbe(ZIPLIST_TAIL_OFFSET(zl)) + nextdiff);
		}
	}
	else {
		/* This element will be the new tail. */
		// 新元素是新的表尾节点
		ZIPLIST_TAIL_OFFSET(zl) = intrev32ifbe(p - zl);
	}

	/* When nextdiff != 0, the raw length of the next entry has changed, so
	 * we need to cascade the update throughout the ziplist */
	 // 当 nextdiff != 0 时，新节点的后继节点的（header 部分）长度已经被改变，
	 // 所以需要级联地更新后续的节点
	if (nextdiff != 0) {
		offset = p - zl;
		// T  = O(N^2)
		zl = __ziplistCascadeUpdate(zl, p + reqlen);
		p = zl + offset;
	}

	/* Write the entry */
	// 一切搞定，将前置节点的长度写入新节点的 header
	p += zipPrevEncodeLength(p, prevlen);
	// 将节点值的长度写入新节点的 header
	p += zipEncodeLength(p, encoding, slen);
	// 写入节点值
	if (ZIP_IS_STR(encoding)) {
		// T = O(N)
		memcpy(p, s, slen);
	}
	else {
		// T = O(1)
		zipSaveInteger(p, value, encoding);
	}

	// 更新列表的节点数量计数器
	// T = O(1)
	ZIPLIST_INCR_LENGTH(zl, 1);

	return zl;
}

/* Create a new empty ziplist.
 *
 * 创建并返回一个新的 ziplist
 *
 * T = O(1)
 */
unsigned char *ziplistNew(void) {

	// ZIPLIST_HEADER_SIZE 是 ziplist 表头的大小
	// 1 字节是表末端 ZIP_END 的大小
	unsigned int bytes = ZIPLIST_HEADER_SIZE + 1;

	// 为表头和表末端分配空间
	unsigned char *zl = (unsigned char *)malloc(bytes);

	// 初始化表属性
	ZIPLIST_BYTES(zl) = intrev32ifbe(bytes);
	ZIPLIST_TAIL_OFFSET(zl) = intrev32ifbe(ZIPLIST_HEADER_SIZE);
	ZIPLIST_LENGTH(zl) = 0;

	// 设置表末端
	zl[bytes - 1] = ZIP_END;

	return zl;
}

/*
 * 将长度为 slen 的字符串 s 推入到 zl 中。
 *
 * where 参数的值决定了推入的方向：
 * - 值为 ZIPLIST_HEAD 时，将新值推入到表头。
 * - 否则，将新值推入到表末端。
 *
 * 函数的返回值为添加新值后的 ziplist 。
 *
 * T = O(N^2)
 */
unsigned char *ziplistPush(unsigned char *zl, unsigned char *s, unsigned int slen, int where) {

	// 根据 where 参数的值，决定将值推入到表头还是表尾
	unsigned char *p;
	p = (where == ZIPLIST_HEAD) ? ZIPLIST_ENTRY_HEAD(zl) : ZIPLIST_ENTRY_END(zl);

	// 返回添加新值后的 ziplist
	// T = O(N^2)
	return __ziplistInsert(zl, p, s, slen);
}