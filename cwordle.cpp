#include "cwordle.h"
#include "random.h"
#include "partial_sorted_list.h"

/************************************************************************
 * A cwordle object tracks the state of matching successive words
 * against the target. For each word that has been tried,
 * it tracks the match_target(combined word and match_result),
 * and the list of still-permissible words.
 *
 * The functions are called form the commands object, in response
 * to user-entered commands.
 ***********************************************************************/

/************************************************************************
 * load_words - load the default dictionary
 ***********************************************************************/

void cwordle::load_words(const vector<string> &w)
{
    my_dict.load(w);
}

void cwordle::load_words(const string_view &s)
{
    my_dict.load(s);
}

/************************************************************************
 * load_file - the the dictionary from a file. Return false if
 * there is a problem with the file.
 ***********************************************************************/

bool cwordle::load_file(const string &filename)
{
    return my_dict.load_file(filename);
}

/************************************************************************
 * best - return the words that give the best entropy amongst
 * the remaining words.
 ***********************************************************************/

cwordle::result_list_t cwordle::best(size_t how_many)
{
    result_list_t result(how_many);
    const word_list &wl = word_lists.empty() ? all_my_words : word_lists.back();
    auto *r = strict_mode && !the_wordle->empty() ? &the_wordle->get_last_result() : NULL;
    for (const auto &w : my_dict.get_words()) {
        if (r==NULL || r->conforms_exact(w.str())) {
            float e = wl.entropy(w);
            result.insert(&w, e);
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
    if (word_lists.empty()) {
        result = all_my_words.entropy(w);
    } else {
        result = word_lists.back().entropy(w);
    }
    return result;
}

/************************************************************************
 * new_word - reinitialise with a new randomly chosen word
 ***********************************************************************/
    
void cwordle::new_word()
{
    set_word(my_dict.get_random());
}

/************************************************************************
 * remaining - list remaining usable words
 ***********************************************************************/

const word_list &cwordle::remaining()
{
    return word_lists.empty() ? all_my_words : word_lists.back();
}

/************************************************************************
 * ge6_current_word - get the current word
 ***********************************************************************/
    
const wordle_word &cwordle::get_current_word() const
{
    return current_word;
}

/************************************************************************
 * set_word - set a new word. Return false iff this
 * is not a valid word.
 ***********************************************************************/
    
bool cwordle::set_word(const string_view &w)
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
 * set_result - add a word and its result to the current result list
 ***********************************************************************/

void cwordle::set_result(const wordle_word &w, const wordle_word::match_result &mr)
{
    results.emplace_back(w, mr);
    wordle_word::match_target mt(w, mr);
    word_list base_wl(my_dict);
    const word_list &wl = word_lists.empty() ? base_wl : word_lists.back();
    word_lists.emplace_back(wl.filter(mt));
}

/************************************************************************
 * try_word - try a word against the current word and return
 * the results of the match
 ***********************************************************************/

wordle_word::match_result cwordle::try_word(const wordle_word &w)
{
    wordle_word::match_result mr(w.match(current_word));
    set_result(w, mr);
    return mr;
}

/************************************************************************
 * undo - remove the last attempt
 ***********************************************************************/
    
void cwordle::undo()
{
    if (size() > 0) {
        results.pop_back();
        word_lists.pop_back();
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

/************************************************************************
 * test_exact - return true iff the given word is an exact match for
 * the latest try
 ***********************************************************************/

bool cwordle::test_exact(const string_view &w)
{
    return empty() || results.back().conforms_exact(w);
}
