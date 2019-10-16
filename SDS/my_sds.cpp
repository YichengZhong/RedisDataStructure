#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <iostream>
#include "my_sds.h"
#include "my_adlist.h"
using namespace std;

/*
* 根据给定字符串 init ，创建一个包含同样字符串的 sds
*
* 参数
*  init ：如果输入为 NULL ，那么创建一个空白 sds
*         否则，新创建的 sds 中包含和 init 内容相同字符串
*
* 返回值
*  sds ：创建成功返回 sdshdr 相对应的 sds
*        创建失败返回 NULL
*
* 复杂度
*  T = O(N)
*/
/* Create a new sds string starting from a null termined C string. */
sds sdsnew(const char *init) {
	size_t initlen = (init == NULL) ? 0 : strlen(init);
	return sdsnewlen(init, initlen);
}

/*
 * 根据给定的初始化字符串 init 和字符串长度 initlen
 * 创建一个新的 sds
 *
 * 参数
 *  init ：初始化字符串指针
 *  initlen ：初始化字符串的长度
 *
 * 返回值
 *  sds ：创建成功返回 sdshdr 相对应的 sds
 *        创建失败返回 NULL
 *
 * 复杂度
 *  T = O(N)
 */
sds sdsnewlen(const void *init, size_t initlen) {

	struct sdshdr *sh;

	// 根据是否有初始化内容，选择适当的内存分配方式
	// T = O(N)
	if (init) {
		// zmalloc 不初始化所分配的内存
		sh = (sdshdr *)malloc(sizeof(struct sdshdr) + initlen + 1);
	}
	else {
		// zcalloc 将分配的内存全部初始化为 0
		sh = (sdshdr *)calloc(sizeof(struct sdshdr) + initlen + 1,1);
	}

	// 内存分配失败，返回
	if (sh == NULL) return NULL;

	// 设置初始化长度
	sh->len = initlen;
	// 新 sds 不预留任何空间
	sh->free = 0;
	// 如果有指定初始化内容，将它们复制到 sdshdr 的 buf 中
	// T = O(N)
	if (initlen && init)
		memcpy(sh->buf, init, initlen);
	// 以 \0 结尾
	sh->buf[initlen] = '\0';

	// 返回 buf 部分，而不是整个 sdshdr
	return (char*)sh->buf;
}

/*
 * 创建并返回一个只保存了空字符串 "" 的 sds
 *
 * 返回值
 *  sds ：创建成功返回 sdshdr 相对应的 sds
 *        创建失败返回 NULL
 *
 * 复杂度
 *  T = O(1)
 */
sds sdsempty(void) {
	return sdsnewlen("", 0);
}

/*
 * 释放给定的 sds
 *
 * 复杂度
 *  T = O(N)
 */
void sdsfree(sds s) {
	if (s == NULL) return;
	free(s - sizeof(struct sdshdr));
}

/*
 * 复制给定 sds 的副本
 *
 * 返回值
 *  sds ：创建成功返回输入 sds 的副本
 *        创建失败返回 NULL
 *
 * 复杂度
 *  T = O(N)
 */
sds sdsdup(const sds s) {
	return sdsnewlen(s, sdslen(s));
}

/*
 * 在不释放 SDS 的字符串空间的情况下，
 * 重置 SDS 所保存的字符串为空字符串。
 *
 * 复杂度
 *  T = O(1)
 */
void sdsclear(sds s) {

	// 取出 sdshdr
	struct sdshdr *sh = (sdshdr*)(s - (sizeof(struct sdshdr)));

	// 重新计算属性
	sh->free += sh->len;
	sh->len = 0;

	// 将结束符放到最前面（相当于惰性地删除 buf 中的内容）
	sh->buf[0] = '\0';
}

/*
 * 将给定字符串 t 追加到 sds 的末尾
 *
 * 返回值
 *  sds ：追加成功返回新 sds ，失败返回 NULL
 *
 * 复杂度
 *  T = O(N)
 */
sds sdscat(sds s, const char *t) {
	return sdscatlen(s, t, strlen(t));
}

/*
 * 将长度为 len 的字符串 t 追加到 sds 的字符串末尾
 *
 * 返回值
 *  sds ：追加成功返回新 sds ，失败返回 NULL
 *
 * 复杂度
 *  T = O(N)
 */
sds sdscatlen(sds s, const void *t, size_t len) {

	struct sdshdr *sh;

	// 原有字符串长度
	size_t curlen = sdslen(s);

	// 扩展 sds 空间
	// T = O(N)
	s = sdsMakeRoomFor(s, len);

	// 内存不足？直接返回
	if (s == NULL) return NULL;

	// 复制 t 中的内容到字符串后部
	// T = O(N)
	sh = (sdshdr*)(s - (sizeof(struct sdshdr)));
	memcpy(s + curlen, t, len);

	// 更新属性
	sh->len = curlen + len;
	sh->free = sh->free - len;

	// 添加新结尾符号
	s[curlen + len] = '\0';

	// 返回新 sds
	return s;
}

/*
 * 对 sds 中 buf 的长度进行扩展，确保在函数执行之后，
 * buf 至少会有 addlen + 1 长度的空余空间
 * （额外的 1 字节是为 \0 准备的）
 *
 * 返回值
 *  sds ：扩展成功返回扩展后的 sds
 *        扩展失败返回 NULL
 *
 * 复杂度
 *  T = O(N)
 */
sds sdsMakeRoomFor(sds s, size_t addlen) {

	struct sdshdr *sh, *newsh;

	// 获取 s 目前的空余空间长度
	size_t free = sdsavail(s);

	size_t len, newlen;

	// s 目前的空余空间已经足够，无须再进行扩展，直接返回
	if (free >= addlen) return s;

	// 获取 s 目前已占用空间的长度
	len = sdslen(s);
	sh = (sdshdr*)(s - (sizeof(struct sdshdr)));

	// s 最少需要的长度
	newlen = (len + addlen);

	// 根据新长度，为 s 分配新空间所需的大小
	if (newlen < SDS_MAX_PREALLOC)
		// 如果新长度小于 SDS_MAX_PREALLOC 
		// 那么为它分配两倍于所需长度的空间
		newlen *= 2;
	else
		// 否则，分配长度为目前长度加上 SDS_MAX_PREALLOC
		newlen += SDS_MAX_PREALLOC;
	// T = O(N)
	newsh = (sdshdr*)realloc(sh, sizeof(struct sdshdr) + newlen + 1);

	// 内存不足，分配失败，返回
	if (newsh == NULL) return NULL;

	// 更新 sds 的空余长度
	newsh->free = newlen - len;

	// 返回 sds
	return newsh->buf;
}

/*
 * 将另一个 sds 追加到一个 sds 的末尾
 *
 * 返回值
 *  sds ：追加成功返回新 sds ，失败返回 NULL
 *
 * 复杂度
 *  T = O(N)
 */
sds sdscatsds(sds s, const sds t) {
	return sdscatlen(s, t, sdslen(t));
}

/*
 * 将字符串复制到 sds 当中，
 * 覆盖原有的字符。
 *
 * 如果 sds 的长度少于字符串的长度，那么扩展 sds 。
 *
 * 复杂度
 *  T = O(N)
 *
 * 返回值
 *  sds ：复制成功返回新的 sds ，否则返回 NULL
 */
sds sdscpy(sds s, const char *t) {
	return sdscpylen(s, t, strlen(t));
}

/*
 * 将字符串 t 的前 len 个字符复制到 sds s 当中，
 * 并在字符串的最后添加终结符。
 *
 * 如果 sds 的长度少于 len 个字符，那么扩展 sds
 *
 * 复杂度
 *  T = O(N)
 *
 * 返回值
 *  sds ：复制成功返回新的 sds ，否则返回 NULL
 */
sds sdscpylen(sds s, const char *t, size_t len) {

	struct sdshdr *sh = (sdshdr*)(s - (sizeof(struct sdshdr)));

	// sds 现有 buf 的长度
	size_t totlen = sh->free + sh->len;

	// 如果 s 的 buf 长度不满足 len ，那么扩展它
	if (totlen < len) {
		// T = O(N)
		s = sdsMakeRoomFor(s, len - sh->len);
		if (s == NULL) return NULL;
		sh = (sdshdr*)(s - (sizeof(struct sdshdr)));
		totlen = sh->free + sh->len;
	}

	// 复制内容
	// T = O(N)
	memcpy(s, t, len);

	// 添加终结符号
	s[len] = '\0';

	// 更新属性
	sh->len = len;
	sh->free = totlen - len;

	// 返回新的 sds
	return s;
}

/*
 * 将 sds 扩充至指定长度，未使用的空间以 0 字节填充。
 *
 * 返回值
 *  sds ：扩充成功返回新 sds ，失败返回 NULL
 *
 * 复杂度：
 *  T = O(N)
 */
sds sdsgrowzero(sds s, size_t len) {
	struct sdshdr *sh = (sdshdr*)(s - (sizeof(struct sdshdr)));
	size_t totlen, curlen = sh->len;

	// 如果 len 比字符串的现有长度小，
	// 那么直接返回，不做动作
	if (len <= curlen) return s;

	// 扩展 sds
	// T = O(N)
	s = sdsMakeRoomFor(s, len - curlen);
	// 如果内存不足，直接返回
	if (s == NULL) return NULL;

	/* Make sure added region doesn't contain garbage */
	// 将新分配的空间用 0 填充，防止出现垃圾内容
	// T = O(N)
	sh = (sdshdr*)(s - (sizeof(struct sdshdr)));
	memset(s + curlen, 0, (len - curlen + 1)); /* also set trailing \0 byte */

	// 更新属性
	totlen = sh->len + sh->free;
	sh->len = len;
	sh->free = totlen - sh->len;

	// 返回新的 sds
	return s;
}

//两个SDS比较
int sdscmp(sds s1, sds s2)
{
	return strcmp(s1, s2);
}
