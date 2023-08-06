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


bool dictionary::insert(const string &w)
{
    bool result = false;
    string groomed = wordle_word::groom(w);
    if (!groomed.empty()) {
        if (word_map.find(groomed) == word_map.end()) {
            size_t i = words.size();
            words.emplace_back(groomed);
            word_map[groomed] = i;
            result = true;
        }
    }
    return result;
}

optional<dictionary::word_index_t> dictionary::find(const string &w) const
{
    optional<word_index_t> result;
    auto iter = word_map.find(w);
    if (iter != word_map.end()) {
        result = iter->second;
    }
    return result;
}

optional<const wordle_word*> dictionary::find_word(const string &w) const
{
    optional<const wordle_word*> result;
    auto ww = find(w);
    if (ww) {
        result = &(*this)[ww.value()];
    }
    return result;
}


string dictionary::get_random() const
{
    return get_string(random::get_int(size()));
}
