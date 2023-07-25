#ifndef __DICTIONARY
#define __DICTIONARY

#include "types.h"
#include "wordle_word.g"

class dictionary
{
public:
    typedef vector<wordle_word> words_t;
    typedef U32 word_index_t;
    typedef map<string, word_index_t> word_map_t;
    typedef vector<word_index_t> word_list_t;
private:
    words_t words;
    word_map_t index;
public:
    bool insert(const string &w);
    const wordle_word &operator[](word_index_t idx)
    {
        return words[idx];
    }
    string get(word_index_t idx);
    {
        return words[idx].str();
    }
    word_index_t find(const string &w);
    
};

#endif
