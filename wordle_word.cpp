#include "wordle_word.h"
#include "styled_text.h"
#include "formatted.h"
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
 * many_mask: as above, but only for letters which appear
 * three or more times
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
 * many_letters: a letter mask with a bit set for every letter that
 * appears three or more times
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

bool wordle_word::verbose = false;

/************************************************************************
 * Utility functions
 ***********************************************************************/

/************************************************************************
 * or256_i32 - find the logical or of the 8 32-bit ints in an __m256i
 ***********************************************************************/

U32 or256_i32(__m256i x)
{
    __m256i x0 = _mm256_permute2x128_si256(x,x,1);
    __m256i x1 = _mm256_or_si256(x,x0);
    __m256i x2 = _mm256_shuffle_epi32(x1,0b01001110);
    __m256i x3 = _mm256_or_si256(x1,x2);
    __m256i x4 = _mm256_shuffle_epi32(x3, 0b11100001);
    __m256i x5 = _mm256_or_si256(x3, x4);
    return _mm_cvtsi128_si32(_mm256_castsi256_si128(x5)) ;
}

/************************************************************************
 * add256_i32 - find the sum of the 8 32-bit ints in an __m256i
 ***********************************************************************/

U32 add256_i32(__m256i x)
{
    __m128i sum128 = _mm_add_epi32( 
                 _mm256_castsi256_si128(x),
                 _mm256_extracti128_si256(x, 1));
    __m128i hi64  = _mm_unpackhi_epi64(sum128, sum128);
    __m128i sum64 = _mm_add_epi32(hi64, sum128);
    __m128i hi32  = _mm_shuffle_epi32(sum64, _MM_SHUFFLE(2, 3, 0, 1));    // Swap the low two elements
    return _mm_cvtsi128_si32(_mm_add_epi32(sum64, hi32));
}

/************************************************************************
 * set_word - set up a wordle_wodr for a particular 5-letter word, setting
 * up all the fields decsribed in the header comment.
 ***********************************************************************/

void wordle_word::set_word(const string &w)
{
    memset(this, 0, sizeof(wordle_word));
    letter_mask once;
    letter_mask twice;
    letter_mask many;
    for (size_t i : irange(0, WORD_LENGTH)) {
        char ch = w[i];
        text[i] = ch;
        letter_mask m(ch);
        if (once.contains(m)) {
            once.remove(m);
            twice |= m;
        } else if (twice.contains(m)) {
            twice.remove(m);
            many |= m;
        } else {
            once |= m;
        }
    }
    repeated_letters = once | twice | many;
    all_letters = once | repeated_letters;
    once_letters = once;
    twice_letters = twice;
    many_letters = many;
    letter_mask seen;
    letter_mask seen2;
    all_mask = wordle_word::set_letters(all_letters); 
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
            for (int ll : irange(0, i)) {
                if (twice_mask[ll].contains(m)) {
                    many_mask[ll] = m;
                }
            }
            many_mask[i] = m;
        }
    }
}

/************************************************************************
 * masked_letters - return the exact letters of a word, only for
 * the letter positions identified by the mask.
 ***********************************************************************/

letter_mask wordle_word::masked_letters(match_mask mask) const
{
    letter_mask result;
    for (auto i : irange(0, WORD_LENGTH)) {
        if (mask.get() & (1 << i)) {
            result |= exact_mask[i];
        }
    }
    return result;
}

/************************************************************************
 * groom - a static function to convert a word to its canonical form
 * i.e. all lower case. It also checks whether it is a valid word,
 * i.e. the correct length.
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

string letter_mask::str() const
{
    string result;
    for (const auto *m : *this) {
        result += string(1, char(*m));
    }
    return result;
}

/************************************************************************
 * word_mask::str - return a sting representing all the letters at
 * each position in a word mask, separated by '|'. Handy when
 * debugging.
 ***********************************************************************/

string word_mask::str() const
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

#define SHOW_MASK(VAR) if (verbose) cout << formatted("%20s: %s\n", #VAR, VAR.str())
#define SHOW_M256(VAR) if (verbose) cout << formatted("%20s: %s\n", #VAR, wms(VAR))

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattributes"   // suppress warning about always_inline
wordle_word::match_result wordle_word::do_match(const wordle_word &target, bool verbose) const
{
#pragma GCC diagnostic pop
    SHOW_MASK(exact_mask);
    SHOW_MASK(target.exact_mask);
    word_mask exact = exact_mask & target.exact_mask;
    SHOW_MASK(exact);
    letter_mask exact_letters = exact.all_letters();
    word_mask target_all = wordle_word::set_letters(target.all_letters & ~exact_letters);
    word_mask once_m = exact.and_not(once_mask);
    SHOW_MASK(target_all);
    SHOW_MASK(once_m);
    word_mask partial1 = target_all & once_m;
    SHOW_MASK(partial1);
    word_mask target_twice = wordle_word::set_letters(target.twice_letters | target.many_letters);
    SHOW_MASK(target_twice);
    word_mask twice_m = exact.and_not(twice_mask);
    SHOW_MASK(twice_m);
    word_mask partial2 = partial1 | (target_twice & twice_m);
    SHOW_MASK(partial2);
    match_mask partial_result = partial2.to_mask();
    for (const auto *m : many_letters) {
        if (word_mask(exact).count_letter(*m)==0) {
            word_mask wm = wordle_word::set_letters(*m);
            U32 count = exact_mask.count_letter(*m);
            word_mask possible_partials = wm & exact_mask;
            word_mask target_many_mask = wordle_word::set_letters(target.many_letters);
            word_mask p = possible_partials & target_many_mask;
            match_mask m2 = p.to_mask();
            match_mask m3 = m2.reduce_bitcount(count);
            partial_result |= m2;
        }
    }
    letter_mask dups = repeated_letters & exact_letters;
    match_mask exact_result = word_mask(exact).to_mask();
    for (const auto *m : dups) {
        U32 target_count = target.exact_mask.count_letter(*m);
        U32 my_count = exact_mask.count_letter(*m);
        U32 exact_count = word_mask(exact).count_letter(*m);
        U32 max_partial = std::min(target_count, my_count) - exact_count;
        match_mask m1 = exact_mask.match_letter(*m);
        m1 &= ~exact_result;
        partial_result &= ~m1;
        m1 = m1.reduce_bitcount(max_partial);
        partial_result |= m1;
    }
    partial_result |= exact_result;
    return match_result(exact_result, partial_result);
}

wordle_word::match_result wordle_word::match(const wordle_word &target) const
{
    if (unlikely(verbose)) {
        return do_match(target, true);
    } else {
        return do_match(target, false);
    }
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

bool wordle_word::match_result::parse(const string &m)
{
    bool result = true;
    U16 e = 0;
    U16 p = 0;
    if (m.size()==WORD_LENGTH) {
        for (size_t i : irange(0, WORD_LENGTH)) {
            if (m[i]=='1') {
                p |= (1 << i);
            } else if (m[i]=='2') {
                e |= (1 << i);
            } else if (m[i]!='0') {
                result = false;
                break;
            }
        }
    } else {
        result = false;
    }
    p |= e;
    *this = match_result(e, p);
    return result;
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
    match_mask only_partial = my_mr.partial_match & ~my_mr.exact_match;
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

/************************************************************************
 * count_letter - count the number of times that a given letter is
 * contained in the word, provided either as a char or as a mask
 ***********************************************************************/

U32 word_mask::count_letter(char letter) const
{
    return count_letter(letter_mask(letter));
}

U32 word_mask::count_letter(letter_mask m) const
{
    word_mask matched = avx::bool_and(masks, set_letters(m).get());
    return matched.to_mask().size();
}

/************************************************************************
 * count_letters - return a letter_map for the letters in the word
 ***********************************************************************/

letter_counter word_mask::count_letters() const
{
    letter_counter result;
    letter_mask letters(or256_i32(masks));
    for (const auto m : letters) {
        result.count(char(*m), count_letter(*m));
    }
    return result;
}

/************************************************************************
 * match_letter - return a match_mask for the corresponding letter
 ***********************************************************************/

match_mask word_mask::match_letter(char letter) const
{
    return match_letter(letter_mask(letter));
}

match_mask word_mask::match_letter(letter_mask m) const
{
    auto m1 = avx::set1(masks, m.get());
    return word_mask(avx::bool_and(masks, m1)).to_mask();
}

/************************************************************************
 * wm, wms - convenience functions for debugging
 ***********************************************************************/

word_mask wm(__m256i m)
{
    return word_mask(m);
}

string wms(__m256i m)
{
    word_mask w(m);
    return w.str();
}



