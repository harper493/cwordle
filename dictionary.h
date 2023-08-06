#ifndef __DICTIONARY
#define __DICTIONARY

#include "types.h"
#include "wordle_word.h"

class dictionary
{
public:
    typedef vector<wordle_word> words_t;
    typedef U32 word_index_t;
    typedef map<string, word_index_t> word_map_t;
    typedef vector<word_index_t> word_list_t;
private:
    words_t words;
    word_map_t word_map;
public:
    size_t size() const
    {
        return words.size();
    }
    const words_t &get_words() const
    {
        return words;
    }
    bool insert(const string &w);
    const wordle_word &operator[](word_index_t idx) const
    {
        return words[idx];
    }
    string get_string(word_index_t idx) const
    {
        return words[idx].str();
    }
    optional<word_index_t> find(const string &w) const;
    optional<const wordle_word*> find_word(const string &w) const;
    string get_random() const;
    template<class RANGE>
    void load(const RANGE &r)
    {
        for (const auto &w : r) {
            insert(w);
        }
    }
    bool load_file(const string &filename);
};

#endif
