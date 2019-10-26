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

/* 创建一个新的哈希表，并根据字典的情况，选择以下其中一个动作来进行：
 *
 * 1) 如果字典的 0 号哈希表为空，那么将新哈希表设置为 0 号哈希表
 * 2) 如果字典的 0 号哈希表非空，那么将新哈希表设置为 1 号哈希表，
 *    并打开字典的 rehash 标识，使得程序可以开始对字典进行 rehash
 *
 * size 参数不够大，或者 rehash 已经在进行时，返回 DICT_ERR 。
 *
 * 成功创建 0 号哈希表，或者 1 号哈希表时，返回 DICT_OK 。
 *
 * T = O(N)
 */
int dictExpand(dict *d, unsigned long size)
{
	// 新哈希表
	dictht n; /* the new hash table */

	// 根据 size 参数，计算哈希表的大小
	// T = O(1)
	unsigned long realsize = _dictNextPower(size);

	 // 不能在字典正在 rehash 时进行
	 // size 的值也不能小于 0 号哈希表的当前已使用节点
	if (dictIsRehashing(d) || d->ht[0].used > size)
		return DICT_ERR;

	// 为哈希表分配空间，并将所有指针指向 NULL
	n.size = realsize;
	n.sizemask = realsize - 1;
	// T = O(N)
	n.table = (dictEntry**)calloc(realsize * sizeof(dictEntry*),1);
	n.used = 0;

	 // 如果 0 号哈希表为空，那么这是一次初始化：
	 // 程序将新哈希表赋给 0 号哈希表的指针，然后字典就可以开始处理键值对了。
	if (d->ht[0].table == NULL) {
		d->ht[0] = n;
		return DICT_OK;
	}

	// 如果 0 号哈希表非空，那么这是一次 rehash ：
	// 程序将新哈希表设置为 1 号哈希表，
	// 并将字典的 rehash 标识打开，让程序可以开始对字典进行 rehash
	d->ht[1] = n;
	d->rehashidx = 0;
	return DICT_OK;
}

/*
 * 尝试将键插入到字典中
 *
 * 如果键已经在字典存在，那么返回 NULL
 *
 * 如果键不存在，那么程序创建新的哈希节点，
 * 将节点和键关联，并插入到字典，然后返回节点本身。
 *
 * T = O(N)
 */
dictEntry *dictAddRaw(dict *d, void *key)
{
	int index;
	dictEntry *entry;
	dictht *ht;

	// 如果条件允许的话，进行单步 rehash
	// T = O(1)
	if (dictIsRehashing(d)) _dictRehashStep(d);

	/* Get the index of the new element, or -1 if
	 * the element already exists. */
	 // 计算键在哈希表中的索引值
	 // 如果值为 -1 ，那么表示键已经存在
	 // T = O(N)
	if ((index = _dictKeyIndex(d, key)) == -1)
		return NULL;

	// T = O(1)
	/* Allocate the memory and store the new entry */
	// 如果字典正在 rehash ，那么将新键添加到 1 号哈希表
	// 否则，将新键添加到 0 号哈希表
	ht = dictIsRehashing(d) ? &d->ht[1] : &d->ht[0];
	// 为新节点分配空间
	entry = (dictEntry *)malloc(sizeof(*entry));
	// 将新节点插入到链表表头
	entry->next = ht->table[index];
	ht->table[index] = entry;
	// 更新哈希表已使用节点数量
	ht->used++;

	/* Set the hash entry fields. */
	// 设置新节点的键
	// T = O(1)
	dictSetKey(d, entry, key);

	return entry;
}

/*
 * 尝试将给定键值对添加到字典中
 *
 * 只有给定键 key 不存在于字典时，添加操作才会成功
 *
 * 添加成功返回 DICT_OK ，失败返回 DICT_ERR
 *
 * 最坏 T = O(N) ，平滩 O(1)
 */
int dictAdd(dict *d, void *key, void *val)
{
	// 尝试添加键到字典，并返回包含了这个键的新哈希节点
	// T = O(N)
	dictEntry *entry = dictAddRaw(d, key);

	// 键已存在，添加失败
	if (!entry) return DICT_ERR;

	// 键不存在，设置节点的值
	// T = O(1)
	dictSetVal(d, entry, val);

	// 添加成功
	return DICT_OK;
}

void dictSetVal(dict *d, dictEntry *entry, int *pt_dictNodeValue)
{

}



int main()
{
	return 0;
}