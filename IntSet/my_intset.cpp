#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "my_intset.h"
#include "malloc.h"
#include "endianconv.h"

/*
 * intset �ı��뷽ʽ
 */
#define INTSET_ENC_INT16 (sizeof(int16_t))
#define INTSET_ENC_INT32 (sizeof(int32_t))
#define INTSET_ENC_INT64 (sizeof(int64_t))

 /*���������ڴ���ֵ v �ı��뷽ʽ
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

/* Return the value at pos, using the configured encoding.
 *
 * ���ݼ��ϵı��뷽ʽ�����صײ������� pos �����ϵ�ֵ
 *
 * T = O(1)
 */
static int64_t _intsetGet(intset *is, int pos) 
{
	return _intsetGetEncoded(is, pos, intrev32ifbe(is->encoding));
}

/*
 * ��ǰ���Ⱥ��ƶ�ָ��������Χ�ڵ�����Ԫ��
 *
 * �������е� MoveTail ��ʵ��һ�������Ե����֣�
 * �������������ǰ������ƶ�Ԫ�أ�
 * �������������
 *
 * �������Ԫ�ص�����ʱ������Ҫ��������ƶ���
 * ��������ʾ���£�����ʾһ��δ������ֵ�Ŀռ䣩��
 * | x | y | z | ? |
 *     |<----->|
 * ����Ԫ�� n �� pos Ϊ 1 ����ô���齫�ƶ� y �� z ����Ԫ��
 * | x | y | y | z |
 *         |<----->|
 * ���žͿ��Խ���Ԫ�� n ���õ� pos ���ˣ�
 * | x | n | y | z |
 *
 * ����������ɾ��Ԫ��ʱ������Ҫ������ǰ�ƶ���
 * ��������ʾ���£����� b ΪҪɾ����Ŀ�꣺
 * | a | b | c | d |
 *         |<----->|
 * ��ô����ͻ��ƶ� b �������Ԫ����ǰһ��Ԫ�ص�λ�ã�
 * �Ӷ����� b �����ݣ�
 * | a | c | d | d |
 *     |<----->|
 * ��󣬳����ٴ�����ĩβɾ��һ��Ԫ�صĿռ䣺
 * | a | c | d |
 * �����������ɾ��������
 *
 * T = O(N)
 */
static void intsetMoveTail(intset *is, uint32_t from, uint32_t to)
{

	void *src, *dst;

	// Ҫ�ƶ���Ԫ�ظ���
	uint32_t bytes = intrev32ifbe(is->length) - from;

	// ���ϵı��뷽ʽ
	uint32_t encoding = intrev32ifbe(is->encoding);

	// ���ݲ�ͬ�ı���
	// src = (Enc_t*)is->contents+from ��¼�ƶ���ʼ��λ��
	// dst = (Enc_t*)is_.contents+to ��¼�ƶ�������λ��
	// bytes *= sizeof(Enc_t) ����һ��Ҫ�ƶ������ֽ�
	if (encoding == INTSET_ENC_INT64) 
	{
		src = (int64_t*)is->contents + from;
		dst = (int64_t*)is->contents + to;
		bytes *= sizeof(int64_t);
	}
	else if (encoding == INTSET_ENC_INT32) 
	{
		src = (int32_t*)is->contents + from;
		dst = (int32_t*)is->contents + to;
		bytes *= sizeof(int32_t);
	}
	else 
	{
		src = (int16_t*)is->contents + from;
		dst = (int16_t*)is->contents + to;
		bytes *= sizeof(int16_t);
	}

	// �����ƶ�
	// T = O(N)
	memmove(dst, src, bytes);
}

/* �����������ϵ��ڴ�ռ��С
 *
 * ���������Ĵ�СҪ�ȼ���ԭ���Ĵ�СҪ��
 * ��ô������ԭ��Ԫ�ص�ֵ���ᱻ�ı䡣
 *
 * ����ֵ��������С�����������
 *
 * T = O(N)
 */
static intset *intsetResize(intset *is, uint32_t len) 
{

	// ��������Ŀռ��С
	uint32_t size = len * intrev32ifbe(is->encoding);

	// ���ݿռ��С�����·���ռ�
	// ע������ʹ�õ��� zrealloc ��
	// ��������¿ռ��С��ԭ���Ŀռ��СҪ��
	// ��ô����ԭ�е����ݻᱻ����
	is = (intset *)realloc(is, sizeof(intset) + size);

	return is;
}

/* ���ݼ��ϵı��뷽ʽ�����ײ������� pos λ���ϵ�ֵ��Ϊ value ��
 *
 * T = O(1)
 */
static void _intsetSet(intset *is, int pos, int64_t value)
{

	// ȡ�����ϵı��뷽ʽ
	uint32_t encoding = intrev32ifbe(is->encoding);

	// ���ݱ��� ((Enc_t*)is->contents) ������ת������ȷ������
	// Ȼ�� ((Enc_t*)is->contents)[pos] ��λ������������
	// ���� ((Enc_t*)is->contents)[pos] = value ��ֵ��������
	// ��� ((Enc_t*)is->contents)+pos ��λ���ո����õ���ֵ�� 
	// �������Ҫ�Ļ��� memrevEncifbe ����ֵ���д�С��ת��
	if (encoding == INTSET_ENC_INT64)
	{
		((int64_t*)is->contents)[pos] = value;
		memrev64ifbe(((int64_t*)is->contents) + pos);
	}
	else if (encoding == INTSET_ENC_INT32)
	{
		((int32_t*)is->contents)[pos] = value;
		memrev32ifbe(((int32_t*)is->contents) + pos);
	}
	else 
	{
		((int16_t*)is->contents)[pos] = value;
		memrev16ifbe(((int16_t*)is->contents) + pos);
	}
}

/* �ڼ��� is �ĵײ������в���ֵ value ���ڵ�������
 *
 * �ɹ��ҵ� value ʱ���������� 1 ������ *pos ��ֵ��Ϊ value ���ڵ�������
 *
 * ����������û�ҵ� value ʱ������ 0 ��
 * ���� *pos ��ֵ��Ϊ value ���Բ��뵽�����е�λ�á�
 *
 * T = O(log N)
 */
static uint8_t intsetSearch(intset *is, int64_t value, uint32_t *pos)
{
	int min = 0, max = intrev32ifbe(is->length) - 1, mid = -1;
	int64_t cur = -1;

	/* The value can never be found when the set is empty */
	// ���� is Ϊ��ʱ�����
	if (intrev32ifbe(is->length) == 0) 
	{
		if (pos) *pos = 0;
		return 0;
	}
	else 
	{
		/* Check for the case where we know we cannot find the value,
		 * but do know the insert position. */
		 // ��Ϊ�ײ�����������ģ���� value �����������һ��ֵ��Ҫ��
		 // ��ô value �϶��������ڼ����У�
		 // ����Ӧ�ý� value ��ӵ��ײ��������ĩ��
		if (value > _intsetGet(is, intrev32ifbe(is->length) - 1)) 
		{
			if (pos) *pos = intrev32ifbe(is->length);
			return 0;
			// ��Ϊ�ײ�����������ģ���� value ����������ǰһ��ֵ��ҪС
			// ��ô value �϶��������ڼ����У�
			// ����Ӧ�ý�����ӵ��ײ��������ǰ��
		}
		else if (value < _intsetGet(is, 0)) 
		{
			if (pos) *pos = 0;
			return 0;
		}
	}

	// �����������н��ж��ֲ���
	// T = O(log N)
	while (max >= min) 
	{
		mid = (min + max) / 2;
		cur = _intsetGet(is, mid);
		if (value > cur) 
		{
			min = mid + 1;
		}
		else if (value < cur) 
		{
			max = mid - 1;
		}
		else 
		{
			break;
		}
	}

	// ����Ƿ��Ѿ��ҵ��� value
	if (value == cur) 
	{
		if (pos) *pos = mid;
		return 1;
	}
	else
	{
		if (pos) *pos = min;
		return 0;
	}
}


/* ���ݸ����ı��뷽ʽ enc �����ؼ��ϵĵײ������� pos �����ϵ�Ԫ�ء�
 *
 * T = O(1)
 */
static int64_t _intsetGetEncoded(intset *is, int pos, uint8_t enc) 
{
	int64_t v64;
	int32_t v32;
	int16_t v16;

	// ((ENCODING*)is->contents) ���Ƚ�����ת���ر����������
	// Ȼ�� ((ENCODING*)is->contents)+pos �����Ԫ���������е���ȷλ��
	// ֮�� member(&vEnc, ..., sizeof(vEnc)) �ٴ������п�������ȷ�������ֽ�
	// �������Ҫ�Ļ��� memrevEncifbe(&vEnc) ��Կ��������ֽڽ��д�С��ת��
	// ���ֵ����
	if (enc == INTSET_ENC_INT64) 
	{
		memcpy(&v64, ((int64_t*)is->contents) + pos, sizeof(v64));
		memrev64ifbe(&v64);
		return v64;
	}
	else if (enc == INTSET_ENC_INT32) 
	{
		memcpy(&v32, ((int32_t*)is->contents) + pos, sizeof(v32));
		memrev32ifbe(&v32);
		return v32;
	}
	else 
	{
		memcpy(&v16, ((int16_t*)is->contents) + pos, sizeof(v16));
		memrev16ifbe(&v16);
		return v16;
	}
}

/* ����ֵ value ��ʹ�õı��뷽ʽ�����������ϵı������������
 * ����ֵ value ��ӵ�����������������С�
 *
 * ����ֵ�������Ԫ��֮�����������
 *
 * T = O(N)
 */
static intset *intsetUpgradeAndAdd(intset *is, int64_t value)
{

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

	// ���¼��ϵı��뷽ʽ
	is->encoding = intrev32ifbe(newenc);
	// �����±���Լ��ϣ��ĵײ����飩���пռ����
	// T = O(N)
	is = intsetResize(is, intrev32ifbe(is->length) + 1);

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

/* 
����������һ���µĿ���������
 *
 * T = O(1)
 */
intset *intsetNew(void)
{

	// Ϊ�������Ͻṹ����ռ�
	intset *is = (intset *)malloc(sizeof(intset));

	// ���ó�ʼ����
	is->encoding = intrev32ifbe(INTSET_ENC_INT16);

	// ��ʼ��Ԫ������
	is->length = 0;

	return is;
}

/* ���Խ�Ԫ�� value ��ӵ����������С�
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
	if (valenc > intrev32ifbe(is->encoding)) 
	{
		// T = O(N)
		return intsetUpgradeAndAdd(is, value);
	}
	else
	{
		// ���е������ʾ�����������еı��뷽ʽ������ value

		/* Abort if the value is already present in the set.
		 * This call will populate "pos" with the right position to insert
		 * the value when it cannot be found. */
		 // �����������в��� value �������Ƿ���ڣ�
		 // - ������ڣ���ô�� *success ����Ϊ 0 ��������δ���Ķ�����������
		 // - ��������ڣ���ô���Բ��� value ��λ�ý������浽 pos ָ����
		 //   �ȴ���������ʹ��
		if (intsetSearch(is, value, &pos)) 
		{
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

/* ������������ɾ��ֵ value ��
 *
 * *success ��ֵָʾɾ���Ƿ�ɹ���
 * - ��ֵ�����ڶ����ɾ��ʧ��ʱ��ֵΪ 0 ��
 * - ɾ���ɹ�ʱ��ֵΪ 1 ��
 *
 * T = O(N)
 */
intset *intsetRemove(intset *is, int64_t value, int *success)
{
	// ���� value �ı��뷽ʽ
	uint8_t valenc = _intsetValueEncoding(value);
	uint32_t pos;

	// Ĭ�����ñ�ʶֵΪɾ��ʧ��
	if (success) *success = 0;

	// �� value �ı����СС�ڻ���ڼ��ϵĵ�ǰ���뷽ʽ��˵�� value �п��ܴ����ڼ��ϣ�
	// ���� intsetSearch �Ľ��Ϊ�棬��ôִ��ɾ��
	// T = O(log N)
	if (valenc <= intrev32ifbe(is->encoding) && intsetSearch(is, value, &pos)) 
	{

		// ȡ�����ϵ�ǰ��Ԫ������
		uint32_t len = intrev32ifbe(is->length);

		/* We know we can delete */
		// ���ñ�ʶֵΪɾ���ɹ�
		if (success) *success = 1;

		/* Overwrite value with tail and update length */
		// ��� value ����λ�������ĩβ
		// ��ô��Ҫ��ԭ��λ�� value ֮���Ԫ�ؽ����ƶ�
		//
		// �ٸ����ӣ���������ʾ���£��� b Ϊɾ����Ŀ��
		// | a | b | c | d |
		// ��ô intsetMoveTail �� b ֮�������������ǰ�ƶ�һ��Ԫ�صĿռ䣬
		// ���� b ԭ��������
		// | a | c | d | d |
		// ֮�� intsetResize ��С�ڴ��Сʱ��
		// ����ĩβ�������һ��Ԫ�صĿռ佫���Ƴ�
		// | a | c | d |
		if (pos < (len - 1)) intsetMoveTail(is, pos + 1, pos);
		// ��С����Ĵ�С���Ƴ���ɾ��Ԫ��ռ�õĿռ�
		// T = O(N)
		is = intsetResize(is, len - 1);
		// ���¼��ϵ�Ԫ������
		is->length = intrev32ifbe(len - 1);
	}

	return is;
}

/* ������ֵ value �Ƿ񼯺��е�Ԫ�ء�
 *
 * �Ƿ��� 1 �����Ƿ��� 0 ��
 *
 * T = O(log N)
 */
uint8_t intsetFind(intset *is, int64_t value) 
{

	// ���� value �ı���
	uint8_t valenc = _intsetValueEncoding(value);

	// ��� value �ı�����ڼ��ϵĵ�ǰ���룬��ô value һ���������ڼ���
	// �� value �ı���С�ڵ��ڼ��ϵĵ�ǰ����ʱ��
	// ����ʹ�� intsetSearch ���в���
	return valenc <= intrev32ifbe(is->encoding) && intsetSearch(is, value, NULL);
}

/*
 * ȡ�����ϵײ�����ָ��λ���е�ֵ�����������浽 value ָ���С�
 *
 * ��� pos û���������������Χ����ô���� 1 �����������������ô���� 0 ��
 *
 * p.s. ����ԭ�ĵ��ĵ�˵���������������ֵ�����Ǵ���ġ�
 *
 * T = O(1)
 */
uint8_t intsetGet(intset *is, uint32_t pos, int64_t *value) 
{

	// pos < intrev32ifbe(is->length) 
	// ��� pos �Ƿ��������ķ�Χ
	if (pos < intrev32ifbe(is->length)) 
	{

		// ����ֵ��ָ��
		*value = _intsetGet(is, pos);

		// ���سɹ�ָʾֵ
		return 1;
	}

	// ����������Χ
	return 0;
}

/* ���������������е�Ԫ�ظ���
 *
 * T = O(1)
 */
uint32_t intsetLen(intset *is) 
{
	return intrev32ifbe(is->length);
}

/* ����������������ռ�õ��ֽ�������
 * ������������������ϵĽṹ��С���Լ�������������Ԫ�ص��ܴ�С
 *
 * T = O(1)
 */
size_t intsetBlobLen(intset *is) 
{
	return sizeof(intset) + intrev32ifbe(is->length)*intrev32ifbe(is->encoding);
}