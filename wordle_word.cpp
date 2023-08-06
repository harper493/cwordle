#include "wordle_word.h"
#include "styled_text.h"
#include <cctype>

/************************************************************************
 * The wordle_word class and its member classes are the heart
 * of this program.
 *
 * A wordle_word is a representation of a word. It includes both the
 * text and also various representations and interpretations to
 * facilitate the algorithms.
 *
 * Member classes are as follows:
 *
 * letter_mask: representation of one or more letters as a bit
 * mask, using 26 out of 32 bits in a U32.
 *
 * word_mask: representation of all 5 letters of a word, as 5
 * distinct letter_masks (so fitting into a single _mm256). The
 * same class is also used to represent a choice of letters at
 * each position.
 *
 * match_result - the result of comparing a word with a target.
 * It contains two 5-bit masks, one for exact matches (i.e.
 * the right letter in the right place) and one for partial
 * matches (the right letter but in the wrong place).
 *
 * match_target - a "digest" of a word used when testing whether
 * a given wordle_word matches the match_result. It is more
 * fully described under the 'conforms' function below.
 *
 * A wordle_word contains, in addition to the text of its word:
 *
 * exact_mask: a "one hot" word mask with a bit set for eacvh
 * exact letter position
 *
 * all_mask: a word mask with bits set at each position for
 * every letter in the word
 *
 * once_mask: as above, but only for letters which appear
 * exactly once
 *
 * twice_mask: as above, but only for letters which appear
 * exactly twice
 *
 * thrice_mask: as above, but only for letters which appear
 * exactly three times (rare but it happens)
 *
 * all_letters: a letter mask with a bit set of every letter that
 * appears in the word
 *
 * once_letters: a letter mask with a bit set for every letter that
 * appears exactly once
 *
 * twice_letters: a letter mask with a bit set for every letter that
 * appears exactly twice
 *
 * thrice_letters: a letter mask with a bit set for every letter that
 * appears exactly three times
 *
 * Holding this information greatly speeds up the 'match' and 'conforms'
 * functions.
 *
 ***********************************************************************/

/************************************************************************
 * Static data
 ***********************************************************************/

const styled_text::color_e unmatched_color = styled_text::black;
const styled_text::color_e matched_color = styled_text::green;
const styled_text::color_e part_matched_color = styled_text::orange;

/************************************************************************
 * set_word - set up a wordle_wodr for a particular 5-letter word, setting
 * up all the fields decsribed in the header comment.
 ***********************************************************************/

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

/************************************************************************
 * masked_letters - return the exact letters of a word, only for
 * the letter positions identified by the mask.
 ***********************************************************************/

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

/************************************************************************
 * groom - a static function to convert a word to its canonical form
 * i.e. all lower case. It also checks whether tisis a valid word,
 * i.e. the correct length and no letter repeated more than three times.
 *
 * Returns either the groomed word, or an empty string if the word
 * is not good.
 ***********************************************************************/
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

/************************************************************************
 * styled_str - return a styled_text object representing the
 * word with a match_result applied. Letters which are an exact
 * match are colored green, partial matches are colored orange, and
 * unmatched letters are black.
 ***********************************************************************/

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

/************************************************************************
 * letter_mask::str - return a string representing the letters in a
 * letter mask.
 ***********************************************************************/

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

/************************************************************************
 * word_mask::str - return a sting representing all the letters at
 * each position in a word mask, separated by '|'. Handy when
 * debugging.
 ***********************************************************************/

string wordle_word::word_mask::str() const
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

/************************************************************************
 * match - compare two words, erturning a match_result indicating which
 * letters are partial and exact matches.
 *
 * If there were never any repeated letters, this would be easy.Bit
 * there are. We have to treat repeated letters separately, otherwise
 * we would get false positives. For example, if we match "never"
 * against "evict" we must get just one partial match, and another 
 * non-match. But if we match it against "elate" we get two partial
 * matches, as also we do against "beeef" (not really a word of course).
 *
 * The technique is:
 *
 * 1. Find the exact match, if any.
 * 2. Remove the exact match(es) from the all_letters letter_mask,
 *    then propagate the latter to all positions.
 * 3. Compare that with the once-only letters.
 * 4. Repeat the above for the two and three-repeat letters.
 * 5. Or the three partial match results.
 * 6. Now we have letter_masks with at least one non-zero bit
 *    wherever there is a match. We use the cmp..._mask instruction
 *    (AVX512 only) to turn this into the corresonding 5-bit mask,
 *    with a 1 wherever there a non-zero letter_mask in the
 *    word_mask.
 ***********************************************************************/

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

/************************************************************************
 * match_result::parse - turn a string of 0/1/2 into teh corresponding
 * match result, where:
 *
 * 0 => no match
 * 1 => partial match
 * 2 => exact match
 *
 * This is used by the unit tests.
 ***********************************************************************/

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

/************************************************************************
 * match_target constructor - given a word and a match_result, constuct
 * a match_target object that can be used very efficiently in the
 * 'conforms' function.
 *
 * The description under 'conforms' below explains the
 * algorithm. This constructor sets up the required letter and
 * word masks to allow ;conforms' to work efficiently.
 ***********************************************************************/

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

/************************************************************************
 * match_target::conforms - return true iff the match target, i.e. a
 * word and its match result, will match the given word.
 *
 * As with match(), this would be much simpler if words never
 * contained repeated letters. But they do. It works as follows:
 *
 * 1. Ensure that no letters are present that are absent in
 *    the target word.
 * 2. Ensure that all required letters (exact or partial match) are
 *    present.
 * 3. Ensure that all exact matches are present.
 * 4. Ensure there are no exact matches that shouldn't be there.
 *
 * If there are no repeated letters, this is sufficient to match
 * the word.
 *
 * Repeated letters are dealt with individually. There may be one or
 * two of them. There are two cases:
 *
 * 1. There are no 'absent' places for the letter - for example, 
 *    'roger' has two partial matches and no absences with 'lorry'.
 *    In this case we must match at least 2, but it would be OK to
 *    match 3 - e.g. with 'error'.
 * 2. The repeated letter is also an absent letter, e.g. 'roger'
 *    matched with 'hairy'. In this case there must be exactly
 *    one match, so 'forty' is OK but not 'lorry' or 'error'.
 *
 * A letter_target object is created for each repeated letter,
 * containing a word_mask for where it is permitted to occur, a
 * match count, and whether or not it is OK to exceed the count.
 *
 * These are applied in turn for each epeated letter.
 ***********************************************************************/

bool wordle_word::match_target::conforms(const wordle_word &other) const
{
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




