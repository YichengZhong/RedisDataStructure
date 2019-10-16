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
* ���ݸ����ַ��� init ������һ������ͬ���ַ����� sds
*
* ����
*  init ���������Ϊ NULL ����ô����һ���հ� sds
*         �����´����� sds �а����� init ������ͬ�ַ���
*
* ����ֵ
*  sds �������ɹ����� sdshdr ���Ӧ�� sds
*        ����ʧ�ܷ��� NULL
*
* ���Ӷ�
*  T = O(N)
*/
/* Create a new sds string starting from a null termined C string. */
sds sdsnew(const char *init) {
	size_t initlen = (init == NULL) ? 0 : strlen(init);
	return sdsnewlen(init, initlen);
}

/*
 * ���ݸ����ĳ�ʼ���ַ��� init ���ַ������� initlen
 * ����һ���µ� sds
 *
 * ����
 *  init ����ʼ���ַ���ָ��
 *  initlen ����ʼ���ַ����ĳ���
 *
 * ����ֵ
 *  sds �������ɹ����� sdshdr ���Ӧ�� sds
 *        ����ʧ�ܷ��� NULL
 *
 * ���Ӷ�
 *  T = O(N)
 */
sds sdsnewlen(const void *init, size_t initlen) {

	struct sdshdr *sh;

	// �����Ƿ��г�ʼ�����ݣ�ѡ���ʵ����ڴ���䷽ʽ
	// T = O(N)
	if (init) {
		// zmalloc ����ʼ����������ڴ�
		sh = (sdshdr *)malloc(sizeof(struct sdshdr) + initlen + 1);
	}
	else {
		// zcalloc ��������ڴ�ȫ����ʼ��Ϊ 0
		sh = (sdshdr *)calloc(sizeof(struct sdshdr) + initlen + 1,1);
	}

	// �ڴ����ʧ�ܣ�����
	if (sh == NULL) return NULL;

	// ���ó�ʼ������
	sh->len = initlen;
	// �� sds ��Ԥ���κοռ�
	sh->free = 0;
	// �����ָ����ʼ�����ݣ������Ǹ��Ƶ� sdshdr �� buf ��
	// T = O(N)
	if (initlen && init)
		memcpy(sh->buf, init, initlen);
	// �� \0 ��β
	sh->buf[initlen] = '\0';

	// ���� buf ���֣����������� sdshdr
	return (char*)sh->buf;
}

/*
 * ����������һ��ֻ�����˿��ַ��� "" �� sds
 *
 * ����ֵ
 *  sds �������ɹ����� sdshdr ���Ӧ�� sds
 *        ����ʧ�ܷ��� NULL
 *
 * ���Ӷ�
 *  T = O(1)
 */
sds sdsempty(void) {
	return sdsnewlen("", 0);
}

/*
 * �ͷŸ����� sds
 *
 * ���Ӷ�
 *  T = O(N)
 */
void sdsfree(sds s) {
	if (s == NULL) return;
	free(s - sizeof(struct sdshdr));
}

/*
 * ���Ƹ��� sds �ĸ���
 *
 * ����ֵ
 *  sds �������ɹ��������� sds �ĸ���
 *        ����ʧ�ܷ��� NULL
 *
 * ���Ӷ�
 *  T = O(N)
 */
sds sdsdup(const sds s) {
	return sdsnewlen(s, sdslen(s));
}

/*
 * �ڲ��ͷ� SDS ���ַ����ռ������£�
 * ���� SDS ��������ַ���Ϊ���ַ�����
 *
 * ���Ӷ�
 *  T = O(1)
 */
void sdsclear(sds s) {

	// ȡ�� sdshdr
	struct sdshdr *sh = (sdshdr*)(s - (sizeof(struct sdshdr)));

	// ���¼�������
	sh->free += sh->len;
	sh->len = 0;

	// ���������ŵ���ǰ�棨�൱�ڶ��Ե�ɾ�� buf �е����ݣ�
	sh->buf[0] = '\0';
}

/*
 * �������ַ��� t ׷�ӵ� sds ��ĩβ
 *
 * ����ֵ
 *  sds ��׷�ӳɹ������� sds ��ʧ�ܷ��� NULL
 *
 * ���Ӷ�
 *  T = O(N)
 */
sds sdscat(sds s, const char *t) {
	return sdscatlen(s, t, strlen(t));
}

/*
 * ������Ϊ len ���ַ��� t ׷�ӵ� sds ���ַ���ĩβ
 *
 * ����ֵ
 *  sds ��׷�ӳɹ������� sds ��ʧ�ܷ��� NULL
 *
 * ���Ӷ�
 *  T = O(N)
 */
sds sdscatlen(sds s, const void *t, size_t len) {

	struct sdshdr *sh;

	// ԭ���ַ�������
	size_t curlen = sdslen(s);

	// ��չ sds �ռ�
	// T = O(N)
	s = sdsMakeRoomFor(s, len);

	// �ڴ治�㣿ֱ�ӷ���
	if (s == NULL) return NULL;

	// ���� t �е����ݵ��ַ�����
	// T = O(N)
	sh = (sdshdr*)(s - (sizeof(struct sdshdr)));
	memcpy(s + curlen, t, len);

	// ��������
	sh->len = curlen + len;
	sh->free = sh->free - len;

	// ����½�β����
	s[curlen + len] = '\0';

	// ������ sds
	return s;
}

/*
 * �� sds �� buf �ĳ��Ƚ�����չ��ȷ���ں���ִ��֮��
 * buf ���ٻ��� addlen + 1 ���ȵĿ���ռ�
 * ������� 1 �ֽ���Ϊ \0 ׼���ģ�
 *
 * ����ֵ
 *  sds ����չ�ɹ�������չ��� sds
 *        ��չʧ�ܷ��� NULL
 *
 * ���Ӷ�
 *  T = O(N)
 */
sds sdsMakeRoomFor(sds s, size_t addlen) {

	struct sdshdr *sh, *newsh;

	// ��ȡ s Ŀǰ�Ŀ���ռ䳤��
	size_t free = sdsavail(s);

	size_t len, newlen;

	// s Ŀǰ�Ŀ���ռ��Ѿ��㹻�������ٽ�����չ��ֱ�ӷ���
	if (free >= addlen) return s;

	// ��ȡ s Ŀǰ��ռ�ÿռ�ĳ���
	len = sdslen(s);
	sh = (sdshdr*)(s - (sizeof(struct sdshdr)));

	// s ������Ҫ�ĳ���
	newlen = (len + addlen);

	// �����³��ȣ�Ϊ s �����¿ռ�����Ĵ�С
	if (newlen < SDS_MAX_PREALLOC)
		// ����³���С�� SDS_MAX_PREALLOC 
		// ��ôΪ���������������賤�ȵĿռ�
		newlen *= 2;
	else
		// ���򣬷��䳤��ΪĿǰ���ȼ��� SDS_MAX_PREALLOC
		newlen += SDS_MAX_PREALLOC;
	// T = O(N)
	newsh = (sdshdr*)realloc(sh, sizeof(struct sdshdr) + newlen + 1);

	// �ڴ治�㣬����ʧ�ܣ�����
	if (newsh == NULL) return NULL;

	// ���� sds �Ŀ��೤��
	newsh->free = newlen - len;

	// ���� sds
	return newsh->buf;
}

/*
 * ����һ�� sds ׷�ӵ�һ�� sds ��ĩβ
 *
 * ����ֵ
 *  sds ��׷�ӳɹ������� sds ��ʧ�ܷ��� NULL
 *
 * ���Ӷ�
 *  T = O(N)
 */
sds sdscatsds(sds s, const sds t) {
	return sdscatlen(s, t, sdslen(t));
}

/*
 * ���ַ������Ƶ� sds ���У�
 * ����ԭ�е��ַ���
 *
 * ��� sds �ĳ��������ַ����ĳ��ȣ���ô��չ sds ��
 *
 * ���Ӷ�
 *  T = O(N)
 *
 * ����ֵ
 *  sds �����Ƴɹ������µ� sds �����򷵻� NULL
 */
sds sdscpy(sds s, const char *t) {
	return sdscpylen(s, t, strlen(t));
}

/*
 * ���ַ��� t ��ǰ len ���ַ����Ƶ� sds s ���У�
 * �����ַ������������ս����
 *
 * ��� sds �ĳ������� len ���ַ�����ô��չ sds
 *
 * ���Ӷ�
 *  T = O(N)
 *
 * ����ֵ
 *  sds �����Ƴɹ������µ� sds �����򷵻� NULL
 */
sds sdscpylen(sds s, const char *t, size_t len) {

	struct sdshdr *sh = (sdshdr*)(s - (sizeof(struct sdshdr)));

	// sds ���� buf �ĳ���
	size_t totlen = sh->free + sh->len;

	// ��� s �� buf ���Ȳ����� len ����ô��չ��
	if (totlen < len) {
		// T = O(N)
		s = sdsMakeRoomFor(s, len - sh->len);
		if (s == NULL) return NULL;
		sh = (sdshdr*)(s - (sizeof(struct sdshdr)));
		totlen = sh->free + sh->len;
	}

	// ��������
	// T = O(N)
	memcpy(s, t, len);

	// ����ս����
	s[len] = '\0';

	// ��������
	sh->len = len;
	sh->free = totlen - len;

	// �����µ� sds
	return s;
}

/*
 * �� sds ������ָ�����ȣ�δʹ�õĿռ��� 0 �ֽ���䡣
 *
 * ����ֵ
 *  sds ������ɹ������� sds ��ʧ�ܷ��� NULL
 *
 * ���Ӷȣ�
 *  T = O(N)
 */
sds sdsgrowzero(sds s, size_t len) {
	struct sdshdr *sh = (sdshdr*)(s - (sizeof(struct sdshdr)));
	size_t totlen, curlen = sh->len;

	// ��� len ���ַ��������г���С��
	// ��ôֱ�ӷ��أ���������
	if (len <= curlen) return s;

	// ��չ sds
	// T = O(N)
	s = sdsMakeRoomFor(s, len - curlen);
	// ����ڴ治�㣬ֱ�ӷ���
	if (s == NULL) return NULL;

	/* Make sure added region doesn't contain garbage */
	// ���·���Ŀռ��� 0 ��䣬��ֹ������������
	// T = O(N)
	sh = (sdshdr*)(s - (sizeof(struct sdshdr)));
	memset(s + curlen, 0, (len - curlen + 1)); /* also set trailing \0 byte */

	// ��������
	totlen = sh->len + sh->free;
	sh->len = len;
	sh->free = totlen - sh->len;

	// �����µ� sds
	return s;
}

//����SDS�Ƚ�
int sdscmp(sds s1, sds s2)
{
	return strcmp(s1, s2);
}
