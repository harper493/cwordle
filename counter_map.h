#ifndef __COUNTER_MAP
#define __COUNTER_MAP

#include "types.h"

template<class COUNTEE, class COUNTER>
class counter_map
{
public:
    typedef std::map<COUNTEE,COUNTER> counter_t;
    typedef counter_t::iterator iterator;
    typedef counter_t::const_iterator const_iterator;
private:
    counter_t counters;
public:
    bool empty() const { return counters.empty(); }
    size_t size() const { return counters.size(); };
    iterator begin() { return counters.begin(); };
    iterator end() { return counters.end(); };
    iterator begin() const { return counters.begin(); };
    iterator end() const { return counters.end(); };
    void count(const COUNTEE &ch)
    {
        auto iter = counters.find(ch);
        if (iter==counters.end()) {
            counters[ch] = 1;
        } else {
            ++(iter->second);
        }
    }
    COUNTER get(const COUNTEE &ch) const
    {
        auto iter = counters.find(ch);
        if (iter==counters.end()) {
            return 0;
        } else {
            return iter->second;
        }            
    }
    bool contains(const COUNTEE &ch) const
    {
        return counters.find(ch) != counters.end();
    }
};

#endif
