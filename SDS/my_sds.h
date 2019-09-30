#pragma once
#define SDS_MAX_PREALLOC (1024*1024)

#include <sys/types.h>
#include <stdarg.h>

/*
 * ���ͱ���������ָ�� sdshdr �� buf ����
 */
typedef char *sds;

/*
 * �����ַ�������Ľṹ
 */
struct sdshdr {

	// buf ����ռ�ÿռ�ĳ���
	int len;

	// buf ��ʣ����ÿռ�ĳ���
	int free;

	// ���ݿռ�
	char buf[];
};

/*
 * ���� sds ʵ�ʱ�����ַ����ĳ���
 *
 * T = O(1)
 */
static inline size_t sdslen(const sds s) {
	struct sdshdr *sh = (sdshdr*)(s - (sizeof(struct sdshdr)));
	return sh->len;
}

/*
 * ���� sds ���ÿռ�ĳ���
 *
 * T = O(1)
 */
static inline size_t sdsavail(const sds s) {
	struct sdshdr *sh = (sdshdr*)(s - (sizeof(struct sdshdr)));
	return sh->free;
}

//����һ����������C�ַ�����SDS
sds sdsnew(const char *init);

//���ݸ����ĳ�ʼ���ַ��� init ���ַ������� initlen,����һ���µ� sds
sds sdsnewlen(const void *init, size_t initlen);

//�����մ�
sds sdsempty(void);

//�ͷŸ�����SDS
void sdsfree(sds s);

//����һ������SDS�ĸ���
sds sdsdup(const sds s);

//���SDS������ַ�������
void sdsclear(sds s);

//��������C�ַ���ƴ�ӵ�SDS�ַ�����ĩβ
sds sdscat(sds s, const char *t);

//��������SDS�ַ���ƴ�ӵ�SDS�ַ�����ĩβ
sds sdscatsds(sds s, const sds t);

sds sdscatlen(sds s, const void *t, size_t len);

//��������C�ַ������Ƶ�SDS������ԭ������
sds sdscpy(sds s, const char *t);

sds sdscpylen(sds s, const char *t, size_t len);

//���մ����䵽len����
sds sdsgrowzero(sds s, size_t len);

//����SDS����ĵĺ��������ٿռ�ʹ��
sds sdsMakeRoomFor(sds s, size_t addlen);