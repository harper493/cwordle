#ifndef __WORD_LIST
#define __WORD_LIST

#include "types.h"
#include "dictionary.h"
#include "wordle_word.h"

class word_list
{
public:
    typedef vector<dictionary::word_index_t> word_vector_t;
    typedef word_vector_t::iterator iterator;
    typedef word_vector_t::const_iterator const_iterator;
private:
    const dictionary &my_dict;
    bool unfilled = true;
    word_vector_t my_words;
public:
    word_list(const dictionary &d) : my_dict(d) { };
    bool empty() const { return !unfilled || my_words.empty(); }
    size_t size() const { return unfilled ? my_dict.size() : my_words.size(); }
    iterator begin() { fill(); return my_words.begin(); }
    iterator end() { fill(); return my_words.end(); }
    const_iterator begin() const { fill(); return my_words.begin(); }
    const_iterator end() const { fill(); return my_words.end(); }
    dictionary::word_index_t operator[](size_t idx) const { fill(); return my_words[idx]; }
    word_list filter(const wordle_word::match_target &mt) const;
    word_list filter_exact(const wordle_word::match_target &mt) const;
    word_list filter_pred(function<bool(const string_view &w)> pred) const;
    word_list sorted() const;
    float entropy(const wordle_word &w) const;
private:
    void fill() const;
    void insert(dictionary::word_index_t word)
    {
        unfilled = false;
        my_words.emplace_back(word);
    }
};

#endif
