#include "dictionary.h"
#include "random.h"
#include <fstream>

/************************************************************************
 * init - create the single staic dictionary
 ***********************************************************************/

void dictionary::init()
{
    the_dictionary = new dictionary();
    the_dictionary->load(::wordle_words);
    the_dictionary->load_allowed(::allowed_words);
}

/************************************************************************
 * load_file - load words from a file. Return false if there was
 * a problem.
 ***********************************************************************/

bool dictionary::load_file_base(const string &filename, std::function<bool(const string_view&)> inserter)
{
    bool result = false;
    std::ifstream istr(filename);
    if (istr.good()) {
        result = true;
        string line;
        while (istr.good()) {
            getline(istr, line);
            if (!line.empty()) {
                inserter(line);
            }
        }
    }
    return result;
}

/************************************************************************
 * load - load whitespace separated words from a string
 ***********************************************************************/

void dictionary::load_base(const string_view &s, std::function<bool(const string_view&)> inserter)
{
    auto b = s.begin();
    auto e = s.end();
    while (b != e) {
        while (!isalpha(*b) && b != e) {
            ++b;
        }
        auto bb = b;
        while (isalpha(*b) && b != e) {
            ++b;
        }
        if (b != e) {
            inserter(string_view(bb, b));
        }
    }
}

/************************************************************************
 * count_words - count the number of words in a string
 ***********************************************************************/

size_t dictionary::count_words(const string_view &s)
{
    size_t count = 0;
    load_base(s, [&](const string_view&){ ++count; return true; });
    return count;
}

/************************************************************************
 * insert - insert a word. Return false if the word is badly formed or
 * if it already present.
 ***********************************************************************/

bool dictionary::insert(const string_view &w, int method)
{
    bool result = false;
    string groomed = wordle_word::groom(w);
    if (!groomed.empty()) {
        if (word_map.find(groomed) == word_map.end()) {
            size_t i = words.size();
            words.emplace_back(groomed, method);
            word_map[groomed] = i;
            result = true;
        }
    }
    return result;
}

/************************************************************************
 * insert_allowed - insert a word which is an allowed starting word
 ***********************************************************************/

bool dictionary::insert_allowed(const string_view &w)
{
    auto i = find(w);
    if (!i) {
        insert(w);
        i = find(w);
    }
    allowed_words.push_back(i.value());
    return true;
}

/************************************************************************
 * load and load_allowed - pre-allocate the vectors before loading
 ***********************************************************************/

void dictionary::load(const string_view &s)
{
    words.reserve(words.size() + count_words(s));
    load_base(s, [&](const string_view &w){ return insert(w, 0); });
}

void dictionary::load_allowed(const string_view &s)
{
    size_t wc = count_words(s);
    words.reserve(words.size() + wc);
    allowed_words.reserve(allowed_words.size() + wc);
    load_base(s,
              [&](const string_view &w){ return insert_allowed(w); });
}

/************************************************************************
 * find - find the index in the dictionary for a word. Return a null
 * value if it is not found.
 ***********************************************************************/

optional<dictionary::word_index_t> dictionary::find(const string_view &w) const
{
    optional<word_index_t> result;
    auto iter = word_map.find(string(w));
    if (iter != word_map.end()) {
        result = iter->second;
    }
    return result;
}

/************************************************************************
 * find_word - see if a word is present, returning the wordle_word
 * for it is if so. Return a null value if the word is not found.
 ***********************************************************************/

optional<const wordle_word*> dictionary::find_word(const string_view &w) const
{
    optional<const wordle_word*> result;
    auto ww = find(w);
    if (ww) {
        result = &(*this)[ww.value()];
    }
    return result;
}

/************************************************************************
 * get_allowed - get a random allowed word from the dictionary.
 ***********************************************************************/

string_view dictionary::get_allowed() const
{
    if (allowed_words.empty()) {
        return get_string(random::get_int(size()));
    } else {
        return get_string(allowed_words[random::get_int(allowed_words.size())]);
    }
}
