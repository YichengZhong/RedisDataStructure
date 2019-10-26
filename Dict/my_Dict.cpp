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

/* ����һ���µĹ�ϣ���������ֵ�������ѡ����������һ�����������У�
 *
 * 1) ����ֵ�� 0 �Ź�ϣ��Ϊ�գ���ô���¹�ϣ������Ϊ 0 �Ź�ϣ��
 * 2) ����ֵ�� 0 �Ź�ϣ��ǿգ���ô���¹�ϣ������Ϊ 1 �Ź�ϣ��
 *    �����ֵ�� rehash ��ʶ��ʹ�ó�����Կ�ʼ���ֵ���� rehash
 *
 * size ���������󣬻��� rehash �Ѿ��ڽ���ʱ������ DICT_ERR ��
 *
 * �ɹ����� 0 �Ź�ϣ������ 1 �Ź�ϣ��ʱ������ DICT_OK ��
 *
 * T = O(N)
 */
int dictExpand(dict *d, unsigned long size)
{
	// �¹�ϣ��
	dictht n; /* the new hash table */

	// ���� size �����������ϣ��Ĵ�С
	// T = O(1)
	unsigned long realsize = _dictNextPower(size);

	 // �������ֵ����� rehash ʱ����
	 // size ��ֵҲ����С�� 0 �Ź�ϣ��ĵ�ǰ��ʹ�ýڵ�
	if (dictIsRehashing(d) || d->ht[0].used > size)
		return DICT_ERR;

	// Ϊ��ϣ�����ռ䣬��������ָ��ָ�� NULL
	n.size = realsize;
	n.sizemask = realsize - 1;
	// T = O(N)
	n.table = (dictEntry**)calloc(realsize * sizeof(dictEntry*),1);
	n.used = 0;

	 // ��� 0 �Ź�ϣ��Ϊ�գ���ô����һ�γ�ʼ����
	 // �����¹�ϣ���� 0 �Ź�ϣ���ָ�룬Ȼ���ֵ�Ϳ��Կ�ʼ�����ֵ���ˡ�
	if (d->ht[0].table == NULL) {
		d->ht[0] = n;
		return DICT_OK;
	}

	// ��� 0 �Ź�ϣ��ǿգ���ô����һ�� rehash ��
	// �����¹�ϣ������Ϊ 1 �Ź�ϣ��
	// �����ֵ�� rehash ��ʶ�򿪣��ó�����Կ�ʼ���ֵ���� rehash
	d->ht[1] = n;
	d->rehashidx = 0;
	return DICT_OK;
}

/*
 * ���Խ������뵽�ֵ���
 *
 * ������Ѿ����ֵ���ڣ���ô���� NULL
 *
 * ����������ڣ���ô���򴴽��µĹ�ϣ�ڵ㣬
 * ���ڵ�ͼ������������뵽�ֵ䣬Ȼ�󷵻ؽڵ㱾��
 *
 * T = O(N)
 */
dictEntry *dictAddRaw(dict *d, void *key)
{
	int index;
	dictEntry *entry;
	dictht *ht;

	// �����������Ļ������е��� rehash
	// T = O(1)
	if (dictIsRehashing(d)) _dictRehashStep(d);

	/* Get the index of the new element, or -1 if
	 * the element already exists. */
	 // ������ڹ�ϣ���е�����ֵ
	 // ���ֵΪ -1 ����ô��ʾ���Ѿ�����
	 // T = O(N)
	if ((index = _dictKeyIndex(d, key)) == -1)
		return NULL;

	// T = O(1)
	/* Allocate the memory and store the new entry */
	// ����ֵ����� rehash ����ô���¼���ӵ� 1 �Ź�ϣ��
	// ���򣬽��¼���ӵ� 0 �Ź�ϣ��
	ht = dictIsRehashing(d) ? &d->ht[1] : &d->ht[0];
	// Ϊ�½ڵ����ռ�
	entry = (dictEntry *)malloc(sizeof(*entry));
	// ���½ڵ���뵽�����ͷ
	entry->next = ht->table[index];
	ht->table[index] = entry;
	// ���¹�ϣ����ʹ�ýڵ�����
	ht->used++;

	/* Set the hash entry fields. */
	// �����½ڵ�ļ�
	// T = O(1)
	dictSetKey(d, entry, key);

	return entry;
}

/*
 * ���Խ�������ֵ����ӵ��ֵ���
 *
 * ֻ�и����� key ���������ֵ�ʱ����Ӳ����Ż�ɹ�
 *
 * ��ӳɹ����� DICT_OK ��ʧ�ܷ��� DICT_ERR
 *
 * � T = O(N) ��ƽ̲ O(1)
 */
int dictAdd(dict *d, void *key, void *val)
{
	// ������Ӽ����ֵ䣬�����ذ�������������¹�ϣ�ڵ�
	// T = O(N)
	dictEntry *entry = dictAddRaw(d, key);

	// ���Ѵ��ڣ����ʧ��
	if (!entry) return DICT_ERR;

	// �������ڣ����ýڵ��ֵ
	// T = O(1)
	dictSetVal(d, entry, val);

	// ��ӳɹ�
	return DICT_OK;
}

void dictSetVal(dict *d, dictEntry *entry, int *pt_dictNodeValue)
{

}



int main()
{
	return 0;
}