#include "cwordle.h"
#include "random.h"
#include "partial_sorted_list.h"

extern vector<string> wordle_words;

/************************************************************************
 * load_words - load the default dictionary
 ***********************************************************************/

void cwordle::load_words()
{
    my_dict.load(wordle_words);
}

/************************************************************************
 * best - return the words that give the best entropy amongst
 * the remaining words.
 ***********************************************************************/

cwordle::result_list_t cwordle::best(size_t how_many)
{
    result_list_t result(how_many);
    if (!word_lists.empty()) {
        for (const auto &w : my_dict.get_words()) {
            float e = words_lists.back().entropy(w);
            result.insert(w, e);
        }
    }
    return result;
}

/************************************************************************
 * entropy - return the entropy of the given word with respect
 * to the remaining valid words.
 ***********************************************************************/

float cwordle::entropy(const wordle_word &w)
{
    float result = 0;
    if (!word_lists.empty()) {
        result = word_lists.back().entropy(w);
    }
    return result;
}

/************************************************************************
 * new_word - reinitialise with a new randomly chosen word
 ***********************************************************************/
    
void cwordle::new_word()
{
    set_word(my_dict.get_string(random::get_int(my_dict.size())));
}

const word_list &cwordle::remaining()

/************************************************************************
 * reveal - show the current word
 ***********************************************************************/
    
string cwordle::reveal()
{
    return current_word.str();
}

/************************************************************************
 * set_word - set a new word. Return false iff this
 * is not a valid word.
 ***********************************************************************/
    
bool cwordle::set_word(const string &w)
{
    string groomed = wordle_word::groom(w);
    if (groomed.empty()) {
        return false;
    } else {
        results.clear();
        word_lists.clear();
        current_word.set_word(groomed);
        return true;
    }
}

/************************************************************************
 * try_word - try a word against the current word and return
 * the results of the match
 ***********************************************************************/

wordle_word::match_result cwordle::try_word(const wordle_word &w)
{
    wordle_word::match_result mr(current_word.match(w));
    results.emplace_back(mr);
    wordle_word::match_target mt(current_word, mr);
    word_list base_wl(my_dict);
    const word_list &wl = word_lists.empty() ? base_wl : word_lists.back();
    words_lists.emplace_back(wl.filter(mt));
    return mr;
}

/************************************************************************
 * undo - remove the last attempt
 ***********************************************************************/
    
void cwordle::undo()
{
    if (!results.empty()) {
        results.resize(results.size() - 1);
        word_lists.resize(word_lists.size() - 1);
    }
}

/************************************************************************
 * add_word - add a word to the dictionary. Return false iff this
 * is not a valid word.
 ***********************************************************************/
    
bool cwordle::add_word(const string &w)
{
    string groomed = wordle_word::groom(w);
    if (groomed.empty()) {
        return false;
    } else {
        my_dict.insert(groomed);
        return true;
    }
}