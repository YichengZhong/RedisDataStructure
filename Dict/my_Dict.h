#pragma once

#include<string>
#include<vector>
#include<list>
#include<iostream>
using namespace std;
/*
 * �ֵ�Ĳ���״̬
 */
 // �����ɹ�
#define DICT_OK 0
// ����ʧ�ܣ������
#define DICT_ERR 1

/*
 * ��ϣ��ڵ�
 */
typedef struct dictEntry 
{

	// ��
	string *dictNodeName;

	// ֵ
	int *pt_dictNodeValue;

	// ָ���¸���ϣ��ڵ㣬�γ�����
	struct dictEntry *next;

} dictEntry;

typedef struct dictht {

	// ��ϣ������
	dictEntry **table;

	// ��ϣ���С
	unsigned long size;

	// ��ϣ���С���룬���ڼ�������ֵ
	// ���ǵ��� size - 1
	unsigned long sizemask;

	// �ù�ϣ�����нڵ������
	unsigned long used;

} dictht;

/*
 * �ֵ�
 */
typedef struct dict {
	
	// ��ϣ��
	dictht ht[2];

	// rehash ����
	// �� rehash ���ڽ���ʱ��ֵΪ -1
	int rehashidx; /* rehashing not in progress if rehashidx == -1 */
} dict;

/* This is the initial size of every hash table */
/*
 * ��ϣ��ĳ�ʼ��С
 */
#define DICT_HT_INITIAL_SIZE     2048

// �鿴�ֵ��Ƿ����� rehash
#define dictIsRehashing(ht) ((ht)->rehashidx != -1)

// ����������Ĺ�ϣֵ
unsigned int dictHashKey(const string *IPPoolName);

//�Ƚ�����key�Ƿ����
bool dictCompareKeys(dict *d, void *key1, void *key2);

/* API */
//����һ���յ��ֵ�
dict *dictCreate();

//��ʼ��һ���ֵ�
int _dictInit(dict *d);

//����һ��Hash��
void _dictReset(dictht *ht);

//����һ����ֵ��
int dictAdd(dict *d, string *dictNodeName, int *pt_dictNodeValue);

dictEntry *dictAddRaw(dict *d, string *IPPoolName);

int _dictKeyIndex(dict *d, const string *IPPoolName);

//�滻һ����ֵ��
int dictReplace(dict *d, void *key, void *val);

//���ظ�������ֵ
void *dictFetchValue(dict *d, const void *key);

//ɾ�������ļ�ֵ��
int dictDelete(dict *d, const void *key);

//���¼�ֵ��
int dictUpdate(dict *d, void *key, void *val);

int dictExpand(dict *d, unsigned long size);

unsigned long _dictNextPower(unsigned long size);

void dictSetVal(dict *d, dictEntry *entry, int  *pt_dictNodeValue);