#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "my_intset.h"
#include "malloc.h"

//Ĭ�ϲ���С��
/* Note that these encodings are ordered, so:
 * INTSET_ENC_INT16 < INTSET_ENC_INT32 < INTSET_ENC_INT64. */
 
/*
* intset �ı��뷽ʽ
*/
#define INTSET_ENC_INT16 (sizeof(int16_t))
#define INTSET_ENC_INT32 (sizeof(int32_t))
#define INTSET_ENC_INT64 (sizeof(int64_t))

  /* Return the required encoding for the provided value.
   *
   * ���������ڴ���ֵ v �ı��뷽ʽ
   *
   * T = O(1)
   */
static uint8_t _intsetValueEncoding(int64_t v) 
{
	if (v < INT32_MIN || v > INT32_MAX)
		return INTSET_ENC_INT64;
	else if (v < INT16_MIN || v > INT16_MAX)
		return INTSET_ENC_INT32;
	else
		return INTSET_ENC_INT16;
}

/* Resize the intset
 *
 * �����������ϵ��ڴ�ռ��С
 *
 * ���������Ĵ�СҪ�ȼ���ԭ���Ĵ�СҪ��
 * ��ô������ԭ��Ԫ�ص�ֵ���ᱻ�ı䡣
 *
 * ����ֵ��������С�����������
 *
 * T = O(N)
 */
static intset *intsetResize(intset *is, uint32_t len) {

	// ��������Ŀռ��С
	uint32_t size = len * intrev32ifbe(is->encoding);

	// ���ݿռ��С�����·���ռ�
	// ע������ʹ�õ��� zrealloc ��
	// ��������¿ռ��С��ԭ���Ŀռ��СҪ��
	// ��ô����ԭ�е����ݻᱻ����
	is = (intset *)realloc(is, sizeof(intset) + size);

	return is;
}

/* Upgrades the intset to a larger encoding and inserts the given integer.
 *
 * ����ֵ value ��ʹ�õı��뷽ʽ�����������ϵı������������
 * ����ֵ value ��ӵ�����������������С�
 *
 * ����ֵ�������Ԫ��֮�����������
 *
 * T = O(N)
 */
static intset *intsetUpgradeAndAdd(intset *is, int64_t value) {

	// ��ǰ�ı��뷽ʽ
	uint8_t curenc = intrev32ifbe(is->encoding);

	// ��ֵ����ı��뷽ʽ
	uint8_t newenc = _intsetValueEncoding(value);

	// ��ǰ���ϵ�Ԫ������
	int length = intrev32ifbe(is->length);

	// ���� value ��ֵ�������ǽ�����ӵ��ײ��������ǰ�˻�������
	// ע�⣬��Ϊ value �ı���ȼ���ԭ�е�����Ԫ�صı��붼Ҫ��
	// ���� value Ҫô���ڼ����е�����Ԫ�أ�ҪôС�ڼ����е�����Ԫ��
	// ��ˣ�value ֻ����ӵ��ײ��������ǰ�˻�����
	int prepend = value < 0 ? 1 : 0;

	/* First set new encoding and resize */
	// ���¼��ϵı��뷽ʽ
	is->encoding = intrev32ifbe(newenc);
	// �����±���Լ��ϣ��ĵײ����飩���пռ����
	// T = O(N)
	is = intsetResize(is, intrev32ifbe(is->length) + 1);

	/* Upgrade back-to-front so we don't overwrite values.
	 * Note that the "prepend" variable is used to make sure we have an empty
	 * space at either the beginning or the end of the intset. */
	 // ���ݼ���ԭ���ı��뷽ʽ���ӵײ�������ȡ������Ԫ��
	 // Ȼ���ٽ�Ԫ�����±���ķ�ʽ��ӵ�������
	 // ��������������֮�󣬼���������ԭ�е�Ԫ�ؾ�����˴Ӿɱ��뵽�±����ת��
	 // ��Ϊ�·���Ŀռ䶼��������ĺ�ˣ����Գ����ȴӺ����ǰ���ƶ�Ԫ��
	 // �ٸ����ӣ�����ԭ���� curenc ���������Ԫ�أ��������������������£�
	 // | x | y | z | 
	 // ���������������ط���֮������ͱ������ˣ����� �� ��ʾδʹ�õ��ڴ棩��
	 // | x | y | z | ? |   ?   |   ?   |
	 // ��ʱ����������˿�ʼ�����²���Ԫ�أ�
	 // | x | y | z | ? |   z   |   ?   |
	 // | x | y |   y   |   z   |   ?   |
	 // |   x   |   y   |   z   |   ?   |
	 // ��󣬳�����Խ���Ԫ����ӵ���� �� �ű�ʾ��λ���У�
	 // |   x   |   y   |   z   |  new  |
	 // ������ʾ������Ԫ�ر�ԭ��������Ԫ�ض���������Ҳ���� prepend == 0
	 // ����Ԫ�ر�ԭ��������Ԫ�ض�Сʱ��prepend == 1���������Ĺ������£�
	 // | x | y | z | ? |   ?   |   ?   |
	 // | x | y | z | ? |   ?   |   z   |
	 // | x | y | z | ? |   y   |   z   |
	 // | x | y |   x   |   y   |   z   |
	 // �������ֵʱ��ԭ���� | x | y | �����ݽ�����ֵ����
	 // |  new  |   x   |   y   |   z   |
	 // T = O(N)
	while (length--)
		_intsetSet(is, length + prepend, _intsetGetEncoded(is, length, curenc));

	/* Set the value at the beginning or the end. */
	// ������ֵ������ prepend ��ֵ����������ӵ�����ͷ��������β
	if (prepend)
		_intsetSet(is, 0, value);
	else
		_intsetSet(is, intrev32ifbe(is->length), value);

	// �����������ϵ�Ԫ������
	is->length = intrev32ifbe(intrev32ifbe(is->length) + 1);

	return is;
}

  /* Create an empty intset.
   *
   * ����������һ���µĿ���������
   *
   * T = O(1)
   */
intset *intsetNew(void) 
{

	// Ϊ�������Ͻṹ����ռ�
	intset *is = (intset *)malloc(sizeof(intset));

	// ���ó�ʼ����
	is->encoding = INTSET_ENC_INT16;

	// ��ʼ��Ԫ������
	is->length = 0;

	return is;
}

/* Insert an integer in the intset
 *
 * ���Խ�Ԫ�� value ��ӵ����������С�
 *
 * *success ��ֵָʾ����Ƿ�ɹ���
 * - �����ӳɹ�����ô�� *success ��ֵ��Ϊ 1 ��
 * - ��ΪԪ���Ѵ��ڶ�������ʧ��ʱ���� *success ��ֵ��Ϊ 0 ��
 *
 * T = O(N)
 */
intset *intsetAdd(intset *is, int64_t value, uint8_t *success) {

	// ������� value ����ĳ���
	uint8_t valenc = _intsetValueEncoding(value);
	uint32_t pos;

	// Ĭ�����ò���Ϊ�ɹ�
	if (success) *success = 1;

	 // ��� value �ı���������������ڵı���Ҫ��
	 // ��ô��ʾ value ��Ȼ������ӵ�����������
	 // ��������������Ҫ����������������������� value ����ı���
	if (valenc > intrev32ifbe(is->encoding)) {
		/* This always succeeds, so we don't need to curry *success. */
		// T = O(N)
		return intsetUpgradeAndAdd(is, value);
	}
	else {
		// ���е������ʾ�����������еı��뷽ʽ������ value

		/* Abort if the value is already present in the set.
		 * This call will populate "pos" with the right position to insert
		 * the value when it cannot be found. */
		 // �����������в��� value �������Ƿ���ڣ�
		 // - ������ڣ���ô�� *success ����Ϊ 0 ��������δ���Ķ�����������
		 // - ��������ڣ���ô���Բ��� value ��λ�ý������浽 pos ָ����
		 //   �ȴ���������ʹ��
		if (intsetSearch(is, value, &pos)) {
			if (success) *success = 0;
			return is;
		}

		// ���е������ʾ value �������ڼ�����
		// ������Ҫ�� value ��ӵ�����������

		// Ϊ value �ڼ����з���ռ�
		is = intsetResize(is, intrev32ifbe(is->length) + 1);
		// �����Ԫ�ز��Ǳ���ӵ��ײ������ĩβ
		// ��ô��Ҫ������Ԫ�ص����ݽ����ƶ����ճ� pos �ϵ�λ�ã�����������ֵ
		// �ٸ�����
		// �������Ϊ��
		// | x | y | z | ? |
		//     |<----->|
		// ����Ԫ�� n �� pos Ϊ 1 ����ô���齫�ƶ� y �� z ����Ԫ��
		// | x | y | y | z |
		//         |<----->|
		// �����Ϳ��Խ���Ԫ�����õ� pos ���ˣ�
		// | x | n | y | z |
		// T = O(N)
		if (pos < intrev32ifbe(is->length)) intsetMoveTail(is, pos, pos + 1);
	}

	// ����ֵ���õ��ײ������ָ��λ����
	_intsetSet(is, pos, value);

	// ��һ����Ԫ�������ļ�����
	is->length = intrev32ifbe(intrev32ifbe(is->length) + 1);

	// ���������Ԫ�غ����������
	return is;
}