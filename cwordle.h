#ifndef __CWORDLE
#define __CWORDLE

#include "types.h"
#include "dictionary.h"
#include "wordle_word.h"
#include "partial_sorted_list.h"
#include "word_list.h"

class cwordle
{
public:
    typedef partial_sorted_list<const wordle_word*, float> result_list_t;
    typedef result_list_t::value_type best_result_t;
private:
    dictionary my_dict;
    word_list all_my_words;
    vector<wordle_word::match_target> results;
    vector<word_list> word_lists;
    wordle_word current_word;
public:
    cwordle()
        : all_my_words(my_dict) { };
    void load_words(const vector<string> &w);
    size_t size() const
    {
        return results.size();
    }
    const dictionary &get_dictionary() const
    {
        return my_dict;
    }
    cwordle::result_list_t best(size_t how_many);
    float entropy(const wordle_word &w);
    void new_word();
    const vector<wordle_word::match_target> &get_results()
    {
        return results;
    }
    const word_list &remaining();
    bool set_word(const string &w);
    wordle_word::match_result try_word(const wordle_word &w);
    void undo();
    bool add_word(const string &w);
    const wordle_word &get_current_word() const;
};

#endif
