#include "word_list.h"
#include "entropy.h"
#include "timers.h"

/************************************************************************
 * word_list - representation of a list of words from the dictionary.
 *
 * The list can be 'unfilled', in which case the list hasn't been
 * explicitly populated and the list consists of all words
 * in the dictionary.
 *
 * The words are held as indices into the dictionary.
 ***********************************************************************/

/************************************************************************
 * filter - give a match_target, return a word_list containing only
 * the words from my list that also match the target.
 *
 * There are two cases, one for when the list is unfilled and one
 * for when we have an explciit list of words.
 ***********************************************************************/

word_list word_list::filter(const wordle_word::match_target &mt) const
{
    word_list result(my_dict);
    if (unfilled) {
        for (size_t i : irange(0ul, my_dict.size())) {
            const wordle_word &w = my_dict[i];
            timers::conforms_timer.restart();
            bool c = mt.conforms(w);
            timers::conforms_timer.pause();
            if (c) {
                result.insert(i);
            }
        }
    } else {
        result.unfilled = false;
        for (dictionary::word_index_t i : *this) {
            const wordle_word &w = my_dict[i];
            timers::conforms_timer.restart();
            bool c = mt.conforms(w);
            timers::conforms_timer.pause();
            if (c) {
                result.insert(i);
            }
        }
    }
    return result;
}

/************************************************************************
 * sorted - return a word_list in which the words have been
 * sorted into alphabetical order
 ***********************************************************************/

word_list word_list::sorted() const
{
    word_list result(my_dict);
    set<string> words;
    fill();
    for (dictionary::word_index_t i : *this) {
        words.insert(string(my_dict[i].str()));
    }
    for (const string &w : words) {
        result.insert(my_dict.find(w).value_or(0));
    }
    return result;
}

/************************************************************************
 * fill - if the list if 'unfilled', then explicitly fill it in
 * with the contents of the dictionary.
 ***********************************************************************/

void word_list::fill() const
{
    if (unfilled) {
        word_list *unconst_this = const_cast<word_list*>(this);
        unconst_this->unfilled = false;
        for (dictionary::word_index_t w : irange(0ul, my_dict.get_words().size())) {
            unconst_this->insert(w);
        }
    }
}

/************************************************************************
 * entropy - calculate tne entropy of applying a given word to this list.
 * We do this by finding the match_result for each word against the
 * target, and counting the number of each distinct match result.
 *
 * Then we use the entropy() function to calculate the entropy of the
 * resulting distribition.
 *
 * There are 1024 possible values of a match result, but in fact only
 * 243 (3^5) of those are valid, since an exact match eclipses a partial
 * match of the same letter. We don't try to take advantage of that.
 ***********************************************************************/

float word_list::entropy(const wordle_word &target) const
{
    vector<float> counts;
    counts.resize(1 << (WORD_LENGTH*2));
    std::fill(counts.begin(), counts.end(), 0.0);
    for (const auto &idx : *this) {
        timers::match_timer.restart();
        auto mr = target.match(my_dict[idx]);
        timers::match_timer.pause();
        counts[mr.get_hash()] += 1;
    }
    timers::entropy_timer.restart();
    float result = ::entropy(counts);
    timers::entropy_timer.pause();
    return result;
}


