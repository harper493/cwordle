#include "wordle_word.h"
#include "styled_text.h"
#include <cctype>

/************************************************************************
 * Static data
 ***********************************************************************/

const styled_text::color_e unmatched_color = styled_text::black;
const styled_text::color_e matched_color = styled_text::green;
const styled_text::color_e part_matched_color = styled_text::orange;

void wordle_word::set_word(const string &w)
{
    memset(this, 0, sizeof(wordle_word));
    letter_mask once;
    letter_mask twice;
    letter_mask thrice;
    for (size_t i : irange(0, WORD_LENGTH)) {
        char ch = w[i];
        text[i] = ch;
        letter_mask m(ch);
        if (once.contains(m)) {
            once.remove(m);
            twice |= m;
        } else if (twice.contains(m)) {
            twice.remove(m);
            thrice |= m;
        } else {
            once |= m;
        }
    }
    all_letters = once | twice | thrice;
    once_letters = once;
    twice_letters = twice;
    thrice_letters = thrice;
    letter_mask seen;
    letter_mask seen2;
    for (int i : irange(0, WORD_LENGTH)) {
        char ch = text[i];
        letter_mask m(ch);
        exact_mask[i] = m;
        if (!seen.contains(m)) {
            once_mask[i] = m;
            seen |= m;
        } else if (!seen2.contains(m)) {
            twice_mask[i] = m;
            for (int j : irange(0, i)) {
                if (once_mask[j].contains(m)) {
                    twice_mask[j] = m;
                    break;
                }
            }
            seen2 |= m;
        } else {
            for (int k : irange(0, i)) {
                if (twice_mask[k].contains(m)) {
                    thrice_mask[k] = m;
                }
            }
            thrice_mask[i] = m;
        }
        all_mask[i] = m;
    }
}

wordle_word::letter_mask wordle_word::masked_letters(U16 mask) const
{
    letter_mask result;
    for (auto i : irange(0, WORD_LENGTH)) {
        if (mask & (1 << i)) {
            result |= exact_mask[i];
        }
    }
    return result;
}

string wordle_word::groom(const string &w)
{
    string result;
    letter_counter letter_count;
    if (w.size() == WORD_LENGTH) {
        for (char ch : w) {
            if (isalpha(ch)) {
                if (isupper(ch)) {
                    ch = tolower(ch);
                }
                result += ch;
                letter_count.count(ch);
            } else {
                result.clear();
                break;
            }
        }
    }
    for (auto i : letter_count) {
        if (i.second >= 4) {
            result.clear();
            break;
        }
    }
    return result;
}

styled_text wordle_word::styled_str(const match_result &mr) const
{
    styled_text result;
    for (size_t i : irange(0, WORD_LENGTH)) {
        if (mr.is_exact(i)) {
            result.append(styled_text(string(1, text[i]), matched_color));
        } else if (mr.is_partial(i)) {
            result.append(styled_text(string(1, text[i]), part_matched_color));
        } else {
            result.append(styled_text(string(1, text[i]), unmatched_color));
        }
    }
    return result;
}

string wordle_word::letter_mask::str() const
{
    string result;
    for (int i : irange(0, ALPHABET_SIZE)) {
        if (*this & letter_mask(i)) {
            result += string(1, 'a' + i);
        }
    }
    return result;
 }

string wordle_word::masks_t::str() const
{
    string result;
    for (size_t i : irange(0, WORD_LENGTH)) {
        auto &lm = (*this)[i];
        if (lm) {
            result += lm.str();
        } else {
            result += "-";
        }
        if (i < WORD_LENGTH - 1) {
            result += "|";
        }
    }
    return result;
}

wordle_word::match_result wordle_word::match(const wordle_word &target) const
{
    __m256i exact = _mm256_and_si256(exact_mask.as_m256i(), target.exact_mask.as_m256i());
    __m256i target_all = _mm256_set1_epi32(target.all_letters.get());
    __m256i once_m = _mm256_andnot_si256(exact, _mm256_load_si256(&once_mask.as_m256i()));
    __m256i partial = _mm256_and_si256(target_all, once_m);
    __m256i target_twice = _mm256_set1_epi32(target.twice_letters.get() | target.thrice_letters.get());
    __m256i twice_m = _mm256_andnot_si256(exact, _mm256_loadu_si256(&twice_mask.as_m256i()));
    partial = _mm256_or_si256(partial, _mm256_and_si256(target_twice, twice_m));
    __m256i target_thrice = _mm256_set1_epi32(target.thrice_letters.get());
    __m256i thrice_m = _mm256_loadu_si256(&thrice_mask.as_m256i());
    partial = _mm256_or_si256(partial, _mm256_and_si256(target_thrice, thrice_m));
    partial = _mm256_or_si256(partial, exact);
    __m256i zeros = _mm256_setzero_si256();
    __mmask8 exact_result = _mm256_cmpgt_epu32_mask(exact, zeros);
    __mmask8 partial_result = _mm256_cmpgt_epu32_mask(partial, zeros);
    return match_result(exact_result, partial_result);
}

void wordle_word::match_result::parse(const string &m)
{
    U16 e = 0;
    U16 p = 0;
    if (m.size()==WORD_LENGTH) {
        for (size_t i : irange(0, WORD_LENGTH)) {
            if (m[i]=='1') {
                p |= (1 << i);
            } else if (m[i]=='2') {
                e |= (1 << i);
            }
        }
    }
    p |= e;
    *this = match_result(e, p);
}

wordle_word::match_target::match_target(const wordle_word &target, const match_result &mr)
    : my_word(target), my_mr(mr)
{
    if (target.str() == "beers") {
        int i = 0;
    }
    U16 only_partial = my_mr.partial_match & ~my_mr.exact_match;
    partial_letters = my_word.masked_letters(only_partial);
    exact_letters = my_word.masked_letters(my_mr.exact_match);
    auto x1 = partial_letters | exact_letters;
    auto x2 = ~x1;
    absent_letters = my_word.all_letters & x2;
    required_letters = partial_letters | exact_letters;
    letter_counter partial_count;
    letter_counter exact_count;
    letter_counter absent_count;
    for (size_t i : irange(0, WORD_LENGTH)) {
        U16 b = 1 << i;
        char ch = my_word.text[i];
        if (only_partial & b) {
            partial_count.count(ch);
            partial_mask[i] = partial_letters;
            only_partial_mask[i] = letter_mask(ch);
        } else if (my_mr.exact_match & b) {
            exact_count.count(ch);
            partial_mask[i] = letter_mask();
            only_partial_mask[i] = letter_mask();
            exact_mask[i] = letter_mask(ch);
            ++exact_match_count;
        } else {
            absent_count.count(ch);
            partial_mask[i] = partial_letters;
            only_partial_mask[i] = letter_mask();
        }
    }
    partial_match_count = partial_count.size();
    letter_mask already_seen;
    for (size_t i : irange(0, WORD_LENGTH)) {
        char ch = my_word.text[i];
        if (!already_seen.contains(ch)) {
            already_seen |= ch;
            U16 pc = partial_count.get(ch);
            if (pc || exact_count.get(ch)) {
                if (absent_count.get(ch)) {
                    letter_targets.emplace_back(ch, my_mr.exact_match, pc, false);
                } else if (exact_count.get(ch) || pc > 1) {
                    letter_targets.emplace_back(ch, my_mr.exact_match, pc, true);
                }
            }
        }
    }    
}

bool wordle_word::match_target::conforms(const wordle_word &other) const
{
    if (other.str() == "beers") {
        int i = 0;
    }
    bool result = false;
    if (!(absent_letters & other.all_letters)) {
        if ((required_letters & other.all_letters) == required_letters) {
            auto exact = _mm256_and_si256(exact_mask.as_m256i(), other.exact_mask.as_m256i());
            if (count_matches(exact) == exact_match_count) {
                auto partial = _mm256_and_si256(only_partial_mask.as_m256i(), other.exact_mask.as_m256i());
                if (count_matches(partial) == 0) {
                    result = true;
                    for (const letter_target &lt : letter_targets) {
                        auto ltm = _mm256_and_si256(lt.mask.as_m256i(), other.exact_mask.as_m256i());
                        auto ltmsz = count_matches(ltm);
                        if (!(lt.greater_ok ?
                              ltmsz >= lt.count
                              : ltmsz == lt.count)) {
                            result = false;
                            break;
                        }
                    }
                }
            }
        }
    }
    return result;
}




