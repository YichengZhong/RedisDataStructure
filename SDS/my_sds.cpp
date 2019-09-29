#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <iostream>
#include "my_sds.h"
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

int main()
{
	sds  myredis=sdsnew("My_Redis");
	cout << myredis << endl;
	return 0;
}

