#include <iostream>
#include <list>  
using namespace std;

class LRUCache {
public:
    LRUCache(int capacity) 
    {
        m_capacity = capacity;
    }

    int get(int key) {
        int retValue = -1;

        list<pair<int, int>>::iterator it = findkey(key);

        //如果在Cashe中，将记录移动到链表的最前端  
        if (it != caches.end())
        {
            retValue = it->second;
            //移动到最前端  
            list<pair<int, int>> ::iterator ptrPair = it;
            pair<int, int> tmpPair = *ptrPair;
            caches.erase(ptrPair);
            caches.push_front(tmpPair);
        }
        return retValue;
    }

    void set(int key, int value) 
    {
        list<pair<int, int>>::iterator it=findkey(key);

        if (it!=caches.end()) //已经存在其中  
        {
            caches.erase(it);
        }
        else //不存在其中  
        {
            pair<int, int > tmpPair = make_pair(key, value);


            if (m_capacity == caches.size()) //已经满  
            {
                caches.pop_back(); //删除最后一个  
                                 
                cout << "满了" << endl;
            }

            caches.push_front(tmpPair);
        }
    }

    //寻找是否存在key
    list<pair<int, int> >::iterator findkey(int key)
    {
        for (list<pair<int, int> >::iterator it = caches.begin(); it != caches.end(); ++it)
        {
            if ((*it).first == key)
            {
                return it;
            }
        }
        return caches.end();
    }


private:
    int m_capacity;           //cashe的大小  
    list<pair<int, int>> caches;    //用一个双链表存储cashe的内容  
};


int main(int argc, char **argv)
{
    LRUCache s(2);
    s.set(1, 1);
    s.set(2, 3);
    cout << s.get(2) << endl;
    s.set(4, 1);
    s.set(5, 2);
    cout << s.get(5) << endl;
    cout << s.get(4) << endl;
    getchar();
    return 0;
}