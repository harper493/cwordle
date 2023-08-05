#include "word_list.h"
#include "entropy.h"

word_list word_list::filter(const wordle_word::match_target &mt) const
{
    word_list result(my_dict);
    if (unfilled) {
        for (size_t i : irange(0ul, my_dict.size())) {
            const wordle_word &w = my_dict[i];
            if (mt.conforms(w)) {
                result.insert(i);
            }
        }
    } else {
        for (dictionary::word_index_t i : *this) {
            const wordle_word &w = my_dict[i];
            if (mt.conforms(w)) {
                result.insert(i);
            }
        }
    }
    return result;
}

word_list word_list::sorted() const
{
    word_list result(my_dict);
    set<string> words;
    fill();
    for (dictionary::word_index_t i : *this) {
        words.insert(my_dict[i].str());
    }
    for (const string &w : words) {
        result.insert(my_dict.find(w).value_or(0));
    }
    return result;
}

void word_list::fill() const
{
    if (unfilled) {
        word_list *unconst_this = const_cast<word_list*>(this);
        for (dictionary::word_index_t w : irange(0ul, my_dict.get_words().size())) {
            unconst_this->insert(w);
        }
    }
}

float word_list::entropy(const wordle_word &target) const
{
    vector<float> counts;
    counts.resize(1 << (WORD_LENGTH*2));
    std::fill(counts.begin(), counts.end(), 0.0);
    for (const auto &idx : *this) {
        auto mr = target.match(my_dict[idx]);
        counts[mr.get_hash()] += 1;
    }
    return ::entropy(counts);
}


