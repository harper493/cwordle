#ifndef __DICTIONARY
#define __DICTIONARY

#include "types.h"
#include "wordle_word.h"

class dictionary
{
public:
    typedef vector<wordle_word> words_t;
    typedef words_t::iterator iterator;
    typedef words_t::const_iterator const_iterator;
    typedef U32 word_index_t;
    typedef map<string, word_index_t> word_map_t;
    typedef vector<word_index_t> word_list_t;
    typedef std::set<word_index_t> word_set_t;
private:
    words_t words;
    word_map_t word_map;
    word_list_t allowed_words;
    word_set_t allowed_word_set;
public:
    size_t size() const
    {
        return words.size();
    }
    size_t allowed_size() const
    {
        return allowed_words.size();
    }
    const words_t &get_words() const
    {
        return words;
    }
    bool insert(const string_view &w, int method=0);
    bool insert_allowed(const string_view &w);
    const wordle_word &operator[](word_index_t idx) const
    {
        return words[idx];
    }
    string_view get_string(word_index_t idx) const
    {
        return words[idx].str();
    }
    optional<word_index_t> find(const string_view &w) const;
    optional<const wordle_word*> find_word(const string_view &w) const;
    string_view get_allowed() const;
    bool is_allowed(const string_view &w) const;
    void load(const string_view &s);
    bool load_file(const string &filename)
    {
        return load_file_base(filename,
                       [&](const string_view &w){ return insert(w, 0); });
    }
    void load_allowed(const string_view &s);
    bool load_file_allowed(const string &filename)
    {
        return load_file_base(filename,
                              [&](const string_view &w){ return insert_allowed(w); });
    }
    const_iterator begin() const
    {
        return words.begin();
    }
    const_iterator end() const
    {
        return words.end();
    }
    static void init();
private:
    void load_base(const string_view &s, std::function<bool(const string_view&)> inserter);
    bool load_file_base(const string &filename, std::function<bool(const string_view&)> inserter);
    size_t count_words(const string_view &s);
};

#endif
