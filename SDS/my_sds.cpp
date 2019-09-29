#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <iostream>
#include "my_sds.h"
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

int main()
{
	sds  myredis=sdsnew("My_Redis");
	cout << myredis << endl;
	return 0;
}

