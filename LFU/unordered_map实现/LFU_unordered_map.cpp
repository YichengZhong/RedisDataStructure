#include<iostream>
#include<unordered_map>
#include<algorithm>
using namespace std;

class LFUCache
{
public:
    LFUCache(int capacity)
    {
        this->capacity = capacity;
    }

    void set(int key, int value)
    {
        if (mapData.find(key) != mapData.end())
        {
            int tmp = mapData[key].second;
            mapData[key] = make_pair(value, tmp);
        }
        else
        {
            if (mapData.size() < capacity)
            {
                mapData.insert(make_pair(key, make_pair(value, 0)));
            }
            else
            {
                for (unordered_map<int, list<int>>::iterator it = count_sort.begin(); it != count_sort.end(); it++)
                {
                    if (!it->second.empty())
                    {
                        int topNum = it->second.front();
                        it->second.pop_front();
                        mapData.erase(topNum);
                        mapData.insert(make_pair(key, make_pair(value, 0)));
                        break;
                    }
                }
            }
        }
        update(key);
    }

    int get(int key)
    {
        if (mapData.find(key) == mapData.end())
        {
            return -1;
        }
        update(key);
        return mapData[key].first;
    }

    //统一更新访问次数
    void update(int key)
    {
        list<int> ::iterator it = find(count_sort[mapData[key].second].begin(), count_sort[mapData[key].second].end(), key);
        if (it != count_sort[mapData[key].second].end())
        {
            count_sort[mapData[key].second].erase(it);
        }
        mapData[key].second++;
        count_sort[mapData[key].second].push_back(key);
        //printFun(count_sort);
    }

private:
    unordered_map<int, list<int>> count_sort;//frequency,key of mapData
    int capacity;
    unordered_map<int, pair<int, int>> mapData;//key,value,count
};


int main()
{
    LFUCache cache(3);
    cache.set(2, 2);
    cache.set(1, 1);
    cout << cache.get(2) << ",";
    cout << cache.get(1) << ",";
    cout << cache.get(2) << ",";
    cache.set(3, 3);
    cache.set(4, 4);
    cout << cache.get(3) << ",";
    cout << cache.get(2) << ",";
    cout << cache.get(1) << ",";
    cout << cache.get(4) << endl;

    system("pause");
    return 0;
}