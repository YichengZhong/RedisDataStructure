#include"my_Dict.h"

//根据地址池名，计算给定键的哈希值
//采用DJB算法
unsigned int dictHashKey(const string *IPPoolName)
{
	unsigned int hash = 5381;
	int len = IPPoolName->size();

	if (0 == len)
		return 0;

	while (len >= 0)
	{
		hash= ((hash << 5) + hash) + ((*IPPoolName)[len]); 
		len--;
	}

	return hash;

}

/* Create a new hash table */
/*
 * 创建一个新的字典
 *
 * T = O(1)
 */
dict *dictCreate()
{
	dict *d = new dict;

	_dictInit(d);

	return d;
}

/* Initialize the hash table */
/*
 * 初始化哈希表
 *
 * T = O(1)
 */
int _dictInit(dict *d)
{
	// 初始化两个哈希表的各项属性值
	// 但暂时还不分配内存给哈希表数组
	_dictReset(&d->ht[0]);
	_dictReset(&d->ht[1]);

	// 设置哈希表 rehash 状态
	d->rehashidx = -1;

	return DICT_OK;
}

void _dictReset(dictht *ht)
{
	ht->table = NULL;
	ht->size = 0;
	ht->sizemask = 0;
	ht->used = 0;
}

/* Our hash table capability is a power of two */
/*
 * 计算第一个大于等于 size 的 2 的 N 次方，用作哈希表的值
 *
 * T = O(1)
 */
static unsigned long _dictNextPower(unsigned long size)
{
	unsigned long i = DICT_HT_INITIAL_SIZE;

	if (size >= LONG_MAX) return LONG_MAX;
	while (1) {
		if (i >= size)
			return i;
		i *= 2;
	}
}

void dictSetVal(dict *d, dictEntry *entry, IPPoolInfo *pt_IPPoolInfo)
{

}



int main()
{
	return 0;
}