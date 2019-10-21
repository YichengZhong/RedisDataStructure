#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include "malloc.h"
#include "my_ziplist.h"
#include "endianconv.h"

/*
 * ziplist ĩ�˱�ʶ�����Լ� 5 �ֽڳ����ȱ�ʶ��
 */
#define ZIP_END 255
#define ZIP_BIGLEN 254

 /* Different encoding/length possibilities */
 /*
  * �ַ���������������������
  */
#define ZIP_STR_MASK 0xc0
#define ZIP_INT_MASK 0x30

/*
* �ַ�����������
*/
#define ZIP_STR_06B (0 << 6)
#define ZIP_STR_14B (1 << 6)
#define ZIP_STR_32B (2 << 6)

/*
* ������������
*/
#define ZIP_INT_16B (0xc0 | 0<<4)
#define ZIP_INT_32B (0xc0 | 1<<4)
#define ZIP_INT_64B (0xc0 | 2<<4)
#define ZIP_INT_24B (0xc0 | 3<<4)
#define ZIP_INT_8B 0xfe

/* 4 bit integer immediate encoding
*
* 4 λ������������������
*/
#define ZIP_INT_IMM_MASK 0x0f
#define ZIP_INT_IMM_MIN 0xf1    /* 11110001 */
#define ZIP_INT_IMM_MAX 0xfd    /* 11111101 */
#define ZIP_INT_IMM_VAL(v) (v & ZIP_INT_IMM_MASK)

/*
* 24 λ���������ֵ����Сֵ
*/
#define INT24_MAX 0x7fffff
#define INT24_MIN (-INT24_MAX - 1)

/* Macro to determine type
*
* �鿴�������� enc �Ƿ��ַ�������
*/
#define ZIP_IS_STR(enc) (((enc) & ZIP_STR_MASK) < ZIP_STR_MASK)

/* Utility macros */
/*
* ziplist ���Ժ�
*/
// ��λ�� ziplist �� bytes ���ԣ������Լ�¼������ ziplist ��ռ�õ��ڴ��ֽ���
// ����ȡ�� bytes ���Ե�����ֵ������Ϊ bytes ���Ը�����ֵ
#define ZIPLIST_BYTES(zl)       (*((uint32_t*)(zl)))
// ��λ�� ziplist �� offset ���ԣ������Լ�¼�˵����β�ڵ��ƫ����
// ����ȡ�� offset ���Ե�����ֵ������Ϊ offset ���Ը�����ֵ
#define ZIPLIST_TAIL_OFFSET(zl) (*((uint32_t*)((zl)+sizeof(uint32_t))))
// ��λ�� ziplist �� length ���ԣ������Լ�¼�� ziplist �����Ľڵ�����
// ����ȡ�� length ���Ե�����ֵ������Ϊ length ���Ը�����ֵ
#define ZIPLIST_LENGTH(zl)      (*((uint16_t*)((zl)+sizeof(uint32_t)*2)))
// ���� ziplist ��ͷ�Ĵ�С
#define ZIPLIST_HEADER_SIZE     (sizeof(uint32_t)*2+sizeof(uint16_t))
// ����ָ�� ziplist ��һ���ڵ㣨����ʼλ�ã���ָ��
#define ZIPLIST_ENTRY_HEAD(zl)  ((zl)+ZIPLIST_HEADER_SIZE)
// ����ָ�� ziplist ���һ���ڵ㣨����ʼλ�ã���ָ��
#define ZIPLIST_ENTRY_TAIL(zl)  ((zl)+intrev32ifbe(ZIPLIST_TAIL_OFFSET(zl)))
// ����ָ�� ziplist ĩ�� ZIP_END ������ʼλ�ã���ָ��
#define ZIPLIST_ENTRY_END(zl)   ((zl)+intrev32ifbe(ZIPLIST_BYTES(zl))-1)

/*
 * ���� ziplist �ڵ���Ϣ�Ľṹ
 */
typedef struct zlentry {

	// prevrawlen ��ǰ�ýڵ�ĳ���
	// prevrawlensize ������ prevrawlen ������ֽڴ�С
	unsigned int prevrawlensize, prevrawlen;

	// len ����ǰ�ڵ�ֵ�ĳ���
	// lensize ������ len ������ֽڴ�С
	unsigned int lensize, len;

	// ��ǰ�ڵ� header �Ĵ�С
	// ���� prevrawlensize + lensize
	unsigned int headersize;

	// ��ǰ�ڵ�ֵ��ʹ�õı�������
	unsigned char encoding;

	// ָ��ǰ�ڵ��ָ��
	unsigned char *p;

} zlentry;

/* Decode the length of the previous element, from the perspective of the entry
 * pointed to by 'ptr'.
 *
 * ���� ptr ָ�룬
 * ȡ������ǰ�ýڵ㳤��������ֽ�����
 * ��������ֽ������浽 prevlensize �С�
 *
 * Ȼ����� prevlensize ���� ptr ��ȡ��ǰ�ýڵ�ĳ���ֵ��
 * �����������ֵ���浽 prevlen �����С�
 *
 * T = O(1)
 */
#define ZIP_DECODE_PREVLEN(ptr, prevlensize, prevlen) do {                     \
                                                                               \
    /* �ȼ��㱻���볤��ֵ���ֽ��� */                                           \
    ZIP_DECODE_PREVLENSIZE(ptr, prevlensize);                                  \
                                                                               \
    /* �ٸ��ݱ����ֽ�����ȡ������ֵ */                                         \
    if ((prevlensize) == 1) {                                                  \
        (prevlen) = (ptr)[0];                                                  \
    } else if ((prevlensize) == 5) {                                           \
        assert(sizeof((prevlensize)) == 4);                                    \
        memcpy(&(prevlen), ((char*)(ptr)) + 1, 4);                             \
        memrev32ifbe(&prevlen);                                                \
    }                                                                          \
} while(0);

/* Return a struct with all information about an entry.
 *
 * �� p ��ָ����б�ڵ����Ϣȫ�����浽 zlentry �У������ظ� zlentry ��
 *
 * T = O(1)
 */
static zlentry zipEntry(unsigned char *p) {
	zlentry e;

	// e.prevrawlensize �����ű���ǰһ���ڵ�ĳ���������ֽ���
	// e.prevrawlen ������ǰһ���ڵ�ĳ���
	// T = O(1)
	ZIP_DECODE_PREVLEN(p, e.prevrawlensize, e.prevrawlen);

	// p + e.prevrawlensize ��ָ���ƶ����б�ڵ㱾��
	// e.encoding �����Žڵ�ֵ�ı�������
	// e.lensize �����ű���ڵ�ֵ����������ֽ���
	// e.len �����Žڵ�ֵ�ĳ���
	// T = O(1)
	ZIP_DECODE_LENGTH(p + e.prevrawlensize, e.encoding, e.lensize, e.len);

	// ����ͷ�����ֽ���
	e.headersize = e.prevrawlensize + e.lensize;

	// ��¼ָ��
	e.p = p;

	return e;
}

/* Return the total number of bytes used by the entry pointed to by 'p'.
 *
 * ����ָ�� p ��ָ��Ľڵ�ռ�õ��ֽ����ܺ͡�
 *
 * T = O(1)
 */
static unsigned int zipRawEntryLength(unsigned char *p) {
	unsigned int prevlensize, encoding, lensize, len;

	// ȡ������ǰ�ýڵ�ĳ���������ֽ���
	// T = O(1)
	ZIP_DECODE_PREVLENSIZE(p, prevlensize);

	// ȡ����ǰ�ڵ�ֵ�ı������ͣ�����ڵ�ֵ����������ֽ������Լ��ڵ�ֵ�ĳ���
	// T = O(1)
	ZIP_DECODE_LENGTH(p + prevlensize, encoding, lensize, len);

	// ����ڵ�ռ�õ��ֽ����ܺ�
	return prevlensize + lensize + len;
}

/* Insert item at "p". */
/*
 * ����ָ�� p ��ָ����λ�ã�������Ϊ slen ���ַ��� s ���뵽 zl �С�
 *
 * �����ķ���ֵΪ��ɲ������֮��� ziplist
 *
 * T = O(N^2)
 */
static unsigned char *__ziplistInsert(unsigned char *zl, unsigned char *p, unsigned char *s, unsigned int slen) {
	// ��¼��ǰ ziplist �ĳ���
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
		// ��� p[0] ��ָ���б�ĩ�ˣ�˵���б�ǿգ����� p ��ָ���б������һ���ڵ�
		// ��ôȡ�� p ��ָ��ڵ����Ϣ�����������浽 entry �ṹ��
		// Ȼ���� prevlen ������¼ǰ�ýڵ�ĳ���
		// ���������½ڵ�֮�� p ��ָ��Ľڵ�ͳ����½ڵ��ǰ�ýڵ㣩
		// T = O(1)
		entry = zipEntry(p);
		prevlen = entry.prevrawlen;
	}
	else {
		// ��� p ָ���βĩ�ˣ���ô������Ҫ����б��Ƿ�Ϊ��
		// 1)��� ptail Ҳָ�� ZIP_END ����ô�б�Ϊ�գ�
		// 2)����б�Ϊ�գ���ô ptail ��ָ���б�����һ���ڵ㡣
		unsigned char *ptail = ZIPLIST_ENTRY_TAIL(zl);
		if (ptail[0] != ZIP_END) {
			// ��β�ڵ�Ϊ�½ڵ��ǰ�ýڵ�

			// ȡ����β�ڵ�ĳ���
			// T = O(1)
			prevlen = zipRawEntryLength(ptail);
		}
	}

	/* See if the entry can be encoded */
	// ���Կ��ܷ������ַ���ת��Ϊ����������ɹ��Ļ���
	// 1)value ������ת���������ֵ
	// 2)encoding �򱣴������� value �ı��뷽ʽ
	// ����ʹ��ʲô���룬 reqlen ������ڵ�ֵ�ĳ���
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
	 // �������ǰ�ýڵ�ĳ�������Ĵ�С
	 // T = O(1)
	reqlen += zipPrevEncodeLength(NULL, prevlen);
	// ������뵱ǰ�ڵ�ֵ����Ĵ�С
	// T = O(1)
	reqlen += zipEncodeLength(NULL, encoding, slen);

	/* When the insert position is not equal to the tail, we need to
	 * make sure that the next entry can hold this entry's length in
	 * its prevlen field. */
	 // ֻҪ�½ڵ㲻�Ǳ���ӵ��б�ĩ�ˣ�
	 // ��ô�������Ҫ��鿴 p ��ָ��Ľڵ㣨�� header���ܷ�����½ڵ�ĳ��ȡ�
	 // nextdiff �������¾ɱ���֮����ֽڴ�С�������ֵ���� 0 
	 // ��ô˵����Ҫ�� p ��ָ��Ľڵ㣨�� header ��������չ
	 // T = O(1)
	nextdiff = (p[0] != ZIP_END) ? zipPrevLenByteDiff(p, reqlen) : 0;

	/* Store offset because a realloc may change the address of zl. */
	// ��Ϊ�ط���ռ���ܻ�ı� zl �ĵ�ַ
	// �����ڷ���֮ǰ����Ҫ��¼ zl �� p ��ƫ������Ȼ���ڷ���֮������ƫ������ԭ p 
	offset = p - zl;
	// curlen �� ziplist ԭ���ĳ���
	// reqlen �������½ڵ�ĳ���
	// nextdiff ���½ڵ�ĺ�̽ڵ���չ header �ĳ��ȣ�Ҫô 0 �ֽڣ�Ҫô 4 ���ֽڣ�
	// T = O(N)
	zl = ziplistResize(zl, curlen + reqlen + nextdiff);
	p = zl + offset;

	/* Apply memory move when necessary and update tail offset. */
	if (p[0] != ZIP_END) {
		// ��Ԫ��֮���нڵ㣬��Ϊ��Ԫ�صļ��룬��Ҫ����Щԭ�нڵ���е���

		/* Subtract one because of the ZIP_END bytes */
		// �ƶ�����Ԫ�أ�Ϊ��Ԫ�صĲ���ռ��ڳ�λ��
		// T = O(N)
		memmove(p + reqlen, p - nextdiff, curlen - offset - 1 + nextdiff);

		/* Encode this entry's raw length in the next entry. */
		// ���½ڵ�ĳ��ȱ��������ýڵ�
		// p+reqlen ��λ�����ýڵ�
		// reqlen ���½ڵ�ĳ���
		// T = O(1)
		zipPrevEncodeLength(p + reqlen, reqlen);

		/* Update offset for tail */
		// ���µ����β��ƫ���������½ڵ�ĳ���Ҳ����
		ZIPLIST_TAIL_OFFSET(zl) =
			intrev32ifbe(intrev32ifbe(ZIPLIST_TAIL_OFFSET(zl)) + reqlen);

		/* When the tail contains more than one entry, we need to take
		 * "nextdiff" in account as well. Otherwise, a change in the
		 * size of prevlen doesn't have an effect on the *tail* offset. */
		 // ����½ڵ�ĺ����ж���һ���ڵ�
		 // ��ô������Ҫ�� nextdiff ��¼���ֽ���Ҳ���㵽��βƫ������
		 // ���������ñ�βƫ������ȷ�����β�ڵ�
		 // T = O(1)
		tail = zipEntry(p + reqlen);
		if (p[reqlen + tail.headersize + tail.len] != ZIP_END) {
			ZIPLIST_TAIL_OFFSET(zl) =
				intrev32ifbe(intrev32ifbe(ZIPLIST_TAIL_OFFSET(zl)) + nextdiff);
		}
	}
	else {
		/* This element will be the new tail. */
		// ��Ԫ�����µı�β�ڵ�
		ZIPLIST_TAIL_OFFSET(zl) = intrev32ifbe(p - zl);
	}

	/* When nextdiff != 0, the raw length of the next entry has changed, so
	 * we need to cascade the update throughout the ziplist */
	 // �� nextdiff != 0 ʱ���½ڵ�ĺ�̽ڵ�ģ�header ���֣������Ѿ����ı䣬
	 // ������Ҫ�����ظ��º����Ľڵ�
	if (nextdiff != 0) {
		offset = p - zl;
		// T  = O(N^2)
		zl = __ziplistCascadeUpdate(zl, p + reqlen);
		p = zl + offset;
	}

	/* Write the entry */
	// һ�и㶨����ǰ�ýڵ�ĳ���д���½ڵ�� header
	p += zipPrevEncodeLength(p, prevlen);
	// ���ڵ�ֵ�ĳ���д���½ڵ�� header
	p += zipEncodeLength(p, encoding, slen);
	// д��ڵ�ֵ
	if (ZIP_IS_STR(encoding)) {
		// T = O(N)
		memcpy(p, s, slen);
	}
	else {
		// T = O(1)
		zipSaveInteger(p, value, encoding);
	}

	// �����б�Ľڵ�����������
	// T = O(1)
	ZIPLIST_INCR_LENGTH(zl, 1);

	return zl;
}

/* Create a new empty ziplist.
 *
 * ����������һ���µ� ziplist
 *
 * T = O(1)
 */
unsigned char *ziplistNew(void) {

	// ZIPLIST_HEADER_SIZE �� ziplist ��ͷ�Ĵ�С
	// 1 �ֽ��Ǳ�ĩ�� ZIP_END �Ĵ�С
	unsigned int bytes = ZIPLIST_HEADER_SIZE + 1;

	// Ϊ��ͷ�ͱ�ĩ�˷���ռ�
	unsigned char *zl = (unsigned char *)malloc(bytes);

	// ��ʼ��������
	ZIPLIST_BYTES(zl) = intrev32ifbe(bytes);
	ZIPLIST_TAIL_OFFSET(zl) = intrev32ifbe(ZIPLIST_HEADER_SIZE);
	ZIPLIST_LENGTH(zl) = 0;

	// ���ñ�ĩ��
	zl[bytes - 1] = ZIP_END;

	return zl;
}

/*
 * ������Ϊ slen ���ַ��� s ���뵽 zl �С�
 *
 * where ������ֵ����������ķ���
 * - ֵΪ ZIPLIST_HEAD ʱ������ֵ���뵽��ͷ��
 * - ���򣬽���ֵ���뵽��ĩ�ˡ�
 *
 * �����ķ���ֵΪ�����ֵ��� ziplist ��
 *
 * T = O(N^2)
 */
unsigned char *ziplistPush(unsigned char *zl, unsigned char *s, unsigned int slen, int where) {

	// ���� where ������ֵ��������ֵ���뵽��ͷ���Ǳ�β
	unsigned char *p;
	p = (where == ZIPLIST_HEAD) ? ZIPLIST_ENTRY_HEAD(zl) : ZIPLIST_ENTRY_END(zl);

	// ���������ֵ��� ziplist
	// T = O(N^2)
	return __ziplistInsert(zl, p, s, slen);
}