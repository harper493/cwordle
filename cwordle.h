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
    struct solve_result
    {
        typedef pair<string, wordle_word::match_result> one_result_t;
        bool success = false;
        vector<one_result_t> words;
    };
private:
    dictionary &my_dict;
    word_list all_my_words;
    vector<wordle_word::match_target> results;
    vector<word_list> word_lists;
    wordle_word current_word;
    string best_start;
    string requested_start;
    bool abandoned = false;
public:
    cwordle(dictionary *dict)
        : my_dict(*dict), all_my_words(my_dict) { };
    void load_words(const vector<string> &w);
    void load_words(const string_view &s);
    bool load_file(const string &filename);
    void load_words_allowed(const string_view &s);
    bool load_file_allowed(const string &filename);
    size_t size() const
    {
        return results.size();
    }
    bool empty() const
    {
        return results.empty();
    }
    dictionary &get_dictionary()
    {
        return my_dict;
    }
    const wordle_word::match_target &get_last_result() const
    {
        return results.back();
    }
    cwordle::result_list_t best(size_t how_many);
    float entropy(const wordle_word &w);
    void new_word();
    const vector<wordle_word::match_target> &get_results()
    {
        return results;
    }
    const word_list remaining() const;
    bool set_word(const string_view &w);
    void set_result(const wordle_word &w, const wordle_word::match_result &mr);
    void set_requested_start(const string_view &w)
    {
        if (w.empty()) {
            requested_start.clear();
        } else {
            requested_start = w;
        }
    }
    wordle_word::match_result try_word(const wordle_word &w);
    void undo();
    void clear();
    solve_result solve();
    bool add_word(const string &w);
    bool test_exact(const string_view &w);
    bool is_won() const
    {
        return !empty() && remaining().size()==1 && get_last_result().str()==current_word.str();
    }
    bool is_lost() const
    {
        return !empty() && results.size()>=size_t(max_guesses) && get_last_result().str()!=current_word.str();
    }
    bool is_over() const
    {
        return is_won() || is_lost();
    }
    bool is_abandoned() const
    {
        return abandoned;
    }
    void abandon()
    {
        abandoned = true;
    }
    bool is_valid_word(const string_view &w) const
    {
        return (bool)my_dict.find(w);
    }
    vector<string> get_guesses() const
    {
        vector<string> result;
        for (const auto &w : results) {
            result.emplace_back(w.str());
        }
        return result;
    }
    const wordle_word &get_current_word() const;
};

#endif
