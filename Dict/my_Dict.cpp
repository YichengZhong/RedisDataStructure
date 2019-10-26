#include"my_Dict.h"

//���ݵ�ַ����������������Ĺ�ϣֵ
//����DJB�㷨
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
 * ����һ���µ��ֵ�
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
 * ��ʼ����ϣ��
 *
 * T = O(1)
 */
int _dictInit(dict *d)
{
	// ��ʼ��������ϣ��ĸ�������ֵ
	// ����ʱ���������ڴ����ϣ������
	_dictReset(&d->ht[0]);
	_dictReset(&d->ht[1]);

	// ���ù�ϣ�� rehash ״̬
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
 * �����һ�����ڵ��� size �� 2 �� N �η���������ϣ���ֵ
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