#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "my_intset.h"
#include "malloc.h"

//默认采用小端
/* Note that these encodings are ordered, so:
 * INTSET_ENC_INT16 < INTSET_ENC_INT32 < INTSET_ENC_INT64. */
 
/*
* intset 的编码方式
*/
#define INTSET_ENC_INT16 (sizeof(int16_t))
#define INTSET_ENC_INT32 (sizeof(int32_t))
#define INTSET_ENC_INT64 (sizeof(int64_t))

  /* Return the required encoding for the provided value.
   *
   * 返回适用于传入值 v 的编码方式
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
 * 调整整数集合的内存空间大小
 *
 * 如果调整后的大小要比集合原来的大小要大，
 * 那么集合中原有元素的值不会被改变。
 *
 * 返回值：调整大小后的整数集合
 *
 * T = O(N)
 */
static intset *intsetResize(intset *is, uint32_t len) {

	// 计算数组的空间大小
	uint32_t size = len * intrev32ifbe(is->encoding);

	// 根据空间大小，重新分配空间
	// 注意这里使用的是 zrealloc ，
	// 所以如果新空间大小比原来的空间大小要大，
	// 那么数组原有的数据会被保留
	is = (intset *)realloc(is, sizeof(intset) + size);

	return is;
}

/* Upgrades the intset to a larger encoding and inserts the given integer.
 *
 * 根据值 value 所使用的编码方式，对整数集合的编码进行升级，
 * 并将值 value 添加到升级后的整数集合中。
 *
 * 返回值：添加新元素之后的整数集合
 *
 * T = O(N)
 */
static intset *intsetUpgradeAndAdd(intset *is, int64_t value) {

	// 当前的编码方式
	uint8_t curenc = intrev32ifbe(is->encoding);

	// 新值所需的编码方式
	uint8_t newenc = _intsetValueEncoding(value);

	// 当前集合的元素数量
	int length = intrev32ifbe(is->length);

	// 根据 value 的值，决定是将它添加到底层数组的最前端还是最后端
	// 注意，因为 value 的编码比集合原有的其他元素的编码都要大
	// 所以 value 要么大于集合中的所有元素，要么小于集合中的所有元素
	// 因此，value 只能添加到底层数组的最前端或最后端
	int prepend = value < 0 ? 1 : 0;

	/* First set new encoding and resize */
	// 更新集合的编码方式
	is->encoding = intrev32ifbe(newenc);
	// 根据新编码对集合（的底层数组）进行空间调整
	// T = O(N)
	is = intsetResize(is, intrev32ifbe(is->length) + 1);

	/* Upgrade back-to-front so we don't overwrite values.
	 * Note that the "prepend" variable is used to make sure we have an empty
	 * space at either the beginning or the end of the intset. */
	 // 根据集合原来的编码方式，从底层数组中取出集合元素
	 // 然后再将元素以新编码的方式添加到集合中
	 // 当完成了这个步骤之后，集合中所有原有的元素就完成了从旧编码到新编码的转换
	 // 因为新分配的空间都放在数组的后端，所以程序先从后端向前端移动元素
	 // 举个例子，假设原来有 curenc 编码的三个元素，它们在数组中排列如下：
	 // | x | y | z | 
	 // 当程序对数组进行重分配之后，数组就被扩容了（符号 ？ 表示未使用的内存）：
	 // | x | y | z | ? |   ?   |   ?   |
	 // 这时程序从数组后端开始，重新插入元素：
	 // | x | y | z | ? |   z   |   ?   |
	 // | x | y |   y   |   z   |   ?   |
	 // |   x   |   y   |   z   |   ?   |
	 // 最后，程序可以将新元素添加到最后 ？ 号标示的位置中：
	 // |   x   |   y   |   z   |  new  |
	 // 上面演示的是新元素比原来的所有元素都大的情况，也即是 prepend == 0
	 // 当新元素比原来的所有元素都小时（prepend == 1），调整的过程如下：
	 // | x | y | z | ? |   ?   |   ?   |
	 // | x | y | z | ? |   ?   |   z   |
	 // | x | y | z | ? |   y   |   z   |
	 // | x | y |   x   |   y   |   z   |
	 // 当添加新值时，原本的 | x | y | 的数据将被新值代替
	 // |  new  |   x   |   y   |   z   |
	 // T = O(N)
	while (length--)
		_intsetSet(is, length + prepend, _intsetGetEncoded(is, length, curenc));

	/* Set the value at the beginning or the end. */
	// 设置新值，根据 prepend 的值来决定是添加到数组头还是数组尾
	if (prepend)
		_intsetSet(is, 0, value);
	else
		_intsetSet(is, intrev32ifbe(is->length), value);

	// 更新整数集合的元素数量
	is->length = intrev32ifbe(intrev32ifbe(is->length) + 1);

	return is;
}

  /* Create an empty intset.
   *
   * 创建并返回一个新的空整数集合
   *
   * T = O(1)
   */
intset *intsetNew(void) 
{

	// 为整数集合结构分配空间
	intset *is = (intset *)malloc(sizeof(intset));

	// 设置初始编码
	is->encoding = INTSET_ENC_INT16;

	// 初始化元素数量
	is->length = 0;

	return is;
}

/* Insert an integer in the intset
 *
 * 尝试将元素 value 添加到整数集合中。
 *
 * *success 的值指示添加是否成功：
 * - 如果添加成功，那么将 *success 的值设为 1 。
 * - 因为元素已存在而造成添加失败时，将 *success 的值设为 0 。
 *
 * T = O(N)
 */
intset *intsetAdd(intset *is, int64_t value, uint8_t *success) {

	// 计算编码 value 所需的长度
	uint8_t valenc = _intsetValueEncoding(value);
	uint32_t pos;

	// 默认设置插入为成功
	if (success) *success = 1;

	 // 如果 value 的编码比整数集合现在的编码要大
	 // 那么表示 value 必然可以添加到整数集合中
	 // 并且整数集合需要对自身进行升级，才能满足 value 所需的编码
	if (valenc > intrev32ifbe(is->encoding)) {
		/* This always succeeds, so we don't need to curry *success. */
		// T = O(N)
		return intsetUpgradeAndAdd(is, value);
	}
	else {
		// 运行到这里，表示整数集合现有的编码方式适用于 value

		/* Abort if the value is already present in the set.
		 * This call will populate "pos" with the right position to insert
		 * the value when it cannot be found. */
		 // 在整数集合中查找 value ，看他是否存在：
		 // - 如果存在，那么将 *success 设置为 0 ，并返回未经改动的整数集合
		 // - 如果不存在，那么可以插入 value 的位置将被保存到 pos 指针中
		 //   等待后续程序使用
		if (intsetSearch(is, value, &pos)) {
			if (success) *success = 0;
			return is;
		}

		// 运行到这里，表示 value 不存在于集合中
		// 程序需要将 value 添加到整数集合中

		// 为 value 在集合中分配空间
		is = intsetResize(is, intrev32ifbe(is->length) + 1);
		// 如果新元素不是被添加到底层数组的末尾
		// 那么需要对现有元素的数据进行移动，空出 pos 上的位置，用于设置新值
		// 举个例子
		// 如果数组为：
		// | x | y | z | ? |
		//     |<----->|
		// 而新元素 n 的 pos 为 1 ，那么数组将移动 y 和 z 两个元素
		// | x | y | y | z |
		//         |<----->|
		// 这样就可以将新元素设置到 pos 上了：
		// | x | n | y | z |
		// T = O(N)
		if (pos < intrev32ifbe(is->length)) intsetMoveTail(is, pos, pos + 1);
	}

	// 将新值设置到底层数组的指定位置中
	_intsetSet(is, pos, value);

	// 增一集合元素数量的计数器
	is->length = intrev32ifbe(intrev32ifbe(is->length) + 1);

	// 返回添加新元素后的整数集合
	return is;
}