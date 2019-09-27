#pragma once
#define SDS_MAX_PREALLOC (1024*1024)

#include <sys/types.h>
#include <stdarg.h>

/*
 * 类型别名，用于指向 sdshdr 的 buf 属性
 */
typedef char *sds;

/*
 * 保存字符串对象的结构
 */
struct sdshdr {

	// buf 中已占用空间的长度
	int len;

	// buf 中剩余可用空间的长度
	int free;

	// 数据空间
	char buf[];
};

/*
 * 返回 sds 实际保存的字符串的长度
 *
 * T = O(1)
 */
static inline size_t sdslen(const sds s) {
	struct sdshdr *sh = (sdshdr*)(s - (sizeof(struct sdshdr)));
	return sh->len;
}

/*
 * 返回 sds 可用空间的长度
 *
 * T = O(1)
 */
static inline size_t sdsavail(const sds s) {
	struct sdshdr *sh = (sdshdr*)(s - (sizeof(struct sdshdr)));
	return sh->free;
}

//创建一个包含给定C字符串的SDS
sds sdsnew(const char *init);

//根据给定的初始化字符串 init 和字符串长度 initlen,创建一个新的 sds
sds sdsnewlen(const void *init, size_t initlen);
