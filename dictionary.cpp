#include "dictionary.h"

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
