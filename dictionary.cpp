#include "dictionary.h"
#include "random.h"
#include <fstream>

/************************************************************************
 * load_file - load words from a file. Return false if there was
 * a problem.
 ***********************************************************************/

bool dictionary::load_file(const string &filename)
{
    bool result = false;
    std::ifstream istr(filename);
    if (istr.good()) {
        result = true;
        string line;
        while (istr.good()) {
            getline(istr, line);
            if (!line.empty()) {
                insert(line);
            }
        }
    }
    return result;
}

/************************************************************************
 * insert - insert a word. Return false if the word is badly formed or
 * if it already present.
 ***********************************************************************/

bool dictionary::insert(const string &w)
{
    bool result = false;
    string groomed = wordle_word::groom(w);
    if (!groomed.empty()) {
        if (word_map.find(groomed) == word_map.end()) {
            size_t i = words.size();
            words.emplace_back(groomed);
            wordle_word w2;
            w2.set_word_2(groomed);
            if (!words.back().identical(w2)) {
                std::cout << formatted("Inconsisent values for '%s'\n", groomed);
            }                
            word_map[groomed] = i;
            result = true;
        }
    }
    return result;
}

/************************************************************************
 * find - find the index in the dictionary for a word. Return a null
 * value if it is not found.
 ***********************************************************************/

optional<dictionary::word_index_t> dictionary::find(const string &w) const
{
    optional<word_index_t> result;
    auto iter = word_map.find(w);
    if (iter != word_map.end()) {
        result = iter->second;
    }
    return result;
}

/************************************************************************
 * find_word - see if a word is present, returning the wordle_word
 * for it is if so. Return a null value if the word is not found.
 ***********************************************************************/

optional<const wordle_word*> dictionary::find_word(const string &w) const
{
    optional<const wordle_word*> result;
    auto ww = find(w);
    if (ww) {
        result = &(*this)[ww.value()];
    }
    return result;
}

/************************************************************************
 * get_random - get a random word from the dictionary.
 ***********************************************************************/

string dictionary::get_random() const
{
    return get_string(random::get_int(size()));
}
