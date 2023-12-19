#ifndef __PARTIAL_SORTED_LIST
#define __PARTIAL_SORTED_LIST

#include "types.h"
#include <float.h>

/************************************************************************
 * partial_sorted_list class - maintain a list of bounded size
 * of key/value pairs, sorted based on the value (decreasing by default,
 * increasing if 'decreasing' is false at construction).
 ***********************************************************************/

template<class KEY, class VALUE>
class partial_sorted_list
{
public:
    typedef partial_sorted_list<KEY,VALUE> my_type_t;
    struct entry
    {
        KEY key;
        VALUE value;
        bool operator<(const entry &other) const { return value < other.value; };
        entry()
            : key(KEY()), value(VALUE()) { };
        entry(KEY k, VALUE v) : key(k), value(v) { };
        entry(const entry &other)
            : key(other.key), value(other.value) { };
        entry &operator=(const entry &other)
        {
            key = other.key;
            value = other.value;
            return *this;
        }
    };
    typedef vector<entry> entries_t;
    typedef typename entries_t::iterator iterator;
    typedef typename entries_t::const_iterator const_iterator;
    typedef typename entries_t::value_type value_type;
private:
    vector<entry> entries;
    size_t max_size;
    bool sorted = false;
    bool decreasing = true;
    VALUE worst_key = FLT_MAX;
public:
    partial_sorted_list(size_t sz=1, bool d=true)
        : max_size(sz), decreasing(d)
    {
        entries.reserve(max_size);
    };
    iterator begin() { return entries.begin(); };
    iterator end() { return entries.end(); };
    const_iterator begin() const { return entries.begin(); };
    const_iterator end() const { return entries.end(); };
    size_t size() const { return entries.size(); };
    VALUE get_worst_key() const { return worst_key; };
    void insert(const KEY &k, const VALUE &v)
    {
        if (entries.size() < max_size) {
            entries.emplace_back(k, v);
            worst_key = FLT_MAX;
        } else if (max_size==1) {
            if (v > entries[0].value) {
                entries[0] = entry(k, v);
                worst_key = v;
            }
        } else {
            if (!sorted) {
                reorder();
            }
            if (v > worst_key) {
                entries[entries.size() - 1] = entry(k, v);
                sorted = false;
            }
        }
    }
    VALUE merge(const my_type_t &other)
    {
        for (const auto &e : other.entries) {
            entries.emplace_back(e);
        }
        sorted = false;
        reorder();
        return worst_key;
    }
    template<class ITER>
    VALUE merge(ITER &start, const ITER &end)
    {
        while (start != end) {
            for (const auto &e : *start++) {
                entries.emplace_back(e);
            }
        }
        sorted = false;
        reorder();
        return worst_key;
    }
    vector<size_t> get_values() const
    {
        vector<VALUE> output;
        for (const auto &r : entries) {
            output.emplace_back(r.value);
        }
        return output;
    }
private:
    void reorder()
    {
        if (!sorted) {
            if (decreasing) {
                std::partial_sort(entries.begin(), entries.begin() + max_size, entries.end(),
                                  [](const entry &e1, const entry &e2){ return e2 < e1; });
            } else {
                std::partial_sort(entries.begin(), entries.begin() + max_size, entries.end());
            }
            if (entries.size() > max_size) {
                entries.resize(max_size);
            }
            worst_key = entries.back().value;
            sorted = true;
        }
    }
};

#endif
