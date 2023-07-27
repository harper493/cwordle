#ifndef __CWORDLE
#define __CWORDLE

#include "types.h"
#include "dictionary.h"
#include "wordle_word.h"
#include "vocabulary.h"
#include "word_list.h"

class cwordle
{
    typedef partial_sorted_list<wordle_word&, float> result_list_t;
    typedef result_list_t::value_type best_result_t;
private:
    dictionary my_dict;
    vector<wordle_word::match_target> results;
    vector<word_list> word_lists;
    wordle_word current_word;
public:
    void load_words();
    vector<best_result_t> best(size_t how_many);
    float entropy(const wordle_word &w);
    void new_word();
    const vector<wordle_word::match_target> &get_results()
    {
        return results;
    }
    const word_list &remaining();
    string reveal();
    bool set_word(const string &w);
    wordle_word::match_result try_word(const wordle_word &w);
    void undo();
    bool add_word(const string &w);
};

#endif
