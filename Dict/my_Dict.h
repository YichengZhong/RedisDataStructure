#pragma once

#include<string>
#include<vector>
#include<list>
#include<iostream>
using namespace std;
/*
 * 字典的操作状态
 */
 // 操作成功
#define DICT_OK 0
// 操作失败（或出错）
#define DICT_ERR 1

class IPsegment
{
public:
	string StartIP;
	string EndIP;
};

class IPPoolInfo
{
public:
	string *IPPoolName;
	string *DDNName;
	list<IPsegment>*IPsegments;
	long IPTotalNums;
	long IPUsedNums;
};

/*
 * 哈希表节点
 */
typedef struct dictEntry 
{

	// 键
	string *IPPoolName;

	// 值
	IPPoolInfo *pt_IPPoolInfo;

	// 指向下个哈希表节点，形成链表
	struct dictEntry *next;

} dictEntry;

typedef struct dictht {

	// 哈希表数组
	dictEntry **table;

	// 哈希表大小
	unsigned long size;

	// 哈希表大小掩码，用于计算索引值
	// 总是等于 size - 1
	unsigned long sizemask;

	// 该哈希表已有节点的数量
	unsigned long used;

} dictht;

/*
 * 字典
 */
typedef struct dict {
	
	// 哈希表
	dictht ht[2];

	// rehash 索引
	// 当 rehash 不在进行时，值为 -1
	int rehashidx; /* rehashing not in progress if rehashidx == -1 */
} dict;

/* This is the initial size of every hash table */
/*
 * 哈希表的初始大小
 */
#define DICT_HT_INITIAL_SIZE     2048

// 查看字典是否正在 rehash
#define dictIsRehashing(ht) ((ht)->rehashidx != -1)

// 根据地址池名，计算给定键的哈希值
unsigned int dictHashKey(const string *IPPoolName);

//比较两个key是否相等
bool dictCompareKeys(dict *d, void *key1, void *key2);

/* API */
//创建一个空的字典
dict *dictCreate();

//初始化一个字典
int _dictInit(dict *d);

//重置一个Hash表
void _dictReset(dictht *ht);

//新增一个键值对
int dictAdd(dict *d, string *IPPoolName, IPPoolInfo *pt_IPPoolInfo);

dictEntry *dictAddRaw(dict *d, string *IPPoolName);

int _dictKeyIndex(dict *d, const string *IPPoolName);

//替换一个键值对
int dictReplace(dict *d, void *key, void *val);

//返回给定键的值
void *dictFetchValue(dict *d, const void *key);

//删除给定的键值对
int dictDelete(dict *d, const void *key);

//更新键值对
int dictUpdate(dict *d, void *key, void *val);

int dictExpand(dict *d, unsigned long size);

unsigned long _dictNextPower(unsigned long size);

void dictSetVal(dict *d, dictEntry *entry, IPPoolInfo *pt_IPPoolInfo);