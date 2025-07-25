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
 * once_mask: as above, but only for the first occurrence of
 * each letter (so for every occurrence of letters which only
 * occur once)./
 *
 * twice_mask: as above, for letters that occur at least
 * twice, and only for the first and second
 * occurrences of each letter (so for every occurrence of
 * letters that occur exactly twice)
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
 * set_word_basic - set up a wordle_word for a particular 5-letter word, setting
 * up all the fields described in the header comment.
 *
 * Works by individually examining each letter, and retaining state for 
 * where each letter has occurred.
 ***********************************************************************/

void wordle_word::set_word_basic(const string_view &w)
{
    letter_mask once;
    letter_mask twice;
    letter_mask many;
    text = w;
    for (char ch : text) {
        letter_mask m(ch);
        if (once.contains(m)) {
            once.remove(m);
            twice |= m;
        } else if (twice.contains(m)) {
            twice.remove(m);
            many |= m;
        } else if (!many.contains(m)) {
            once |= m;
        }
    }
    repeated_letters = twice | many;
    all_letters = once | repeated_letters;
    once_letters = once;
    twice_letters = twice;
    many_letters = many;
    letter_mask seen;
    letter_mask seen2;
    all_mask = set_letters(all_letters); 
    for (int i : irange((size_t)0, text.size())) {
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
 * set_word_2 - as above but use the AVX CONFLICT instruction. This 
 * compares every letter with every other letter, and marks repeats,
 * identifying what they repeat. So the bit count at each position says
 * how many times it has occurred previously: 1 for a first repeat,
 * 2 for a second repeat, and so on.
 *
 * Using that information, we can scan the word letter by letter,
 * using the combination of the repeat count seen here, and the
 * final repeat count for the letter, to make the appropraite entries.
 ***********************************************************************/

void wordle_word::set_word_2(const string_view &w)
{
    text = w;
    exact_mask = word_mask(w);
    word_mask conflict(avx::conflict(exact_mask.get()));
    std::map<letter_mask, size_t> seen;
    for (int i : views::iota((size_t)0, text.size()) | views::reverse) {
        letter_mask ch(text[i]);
        letter_mask m = conflict[i];
        size_t sz = m.size();
        if (seen.contains(ch)) {
            size_t seensz = seen[ch];
            if (sz==0) {
                once_mask.insert(ch, i);
            }
            if (sz<=1 && seensz>=1) {
                twice_mask.insert(ch, i);
            }
            if (seensz>1) {
                many_mask.insert(ch, i);
            }
        } else {
            seen[ch] = sz;
            switch (sz) {
            case 0:             // letter only present once
                once_letters |= ch;
                once_mask.insert(ch, i);
                break;
            case 1:             // letter present twice
                twice_letters |= ch;
                twice_mask.insert(ch, i);
                break;
            default:            // letter present more than twice
                many_letters |= ch;
                many_mask.insert(ch, i);
                break;
            }
        }
    }
    repeated_letters = twice_letters | many_letters;
    all_letters = once_letters | repeated_letters;
    all_mask = set_letters(all_letters); 
}

/************************************************************************
 * set_word - as above but do everything with AVX instuctions. 
 * There is no code which looks individually at the letters.
 *
 * This is a bit tricky. First we create three masks: onces,
 * twices, and manys, for where he CONFLICT instruction has told us
 * about the corresponding number of repeats.
 *
 * Then, we create a mask (many_mask / twice_mask) of the 
 * corresponding letters. We AND this with all letters (exact_mask) to pick out ALL
 * occurrences ofthe letters, including the first and second (which CONFLICT
 * doesn't see as repeats).
 *
 * That allows us to create many_mask and twice_mask according to
 * their definition above.
 *
 * Performance is about equal to set_word_2, above.
 ***********************************************************************/

void wordle_word::set_word(const string_view &w)
{
#ifndef AVX512
    set_word_basic(w);
    return;
#endif
    text = w;
    exact_mask = word_mask(w);
    all_letters = exact_mask.all_letters();
    auto zero(avx::zero(word_mask::mask_t()));
    auto ones = avx::set1(word_mask::mask_t(), 1);
    word_mask conflict(avx::conflict(exact_mask.get()));
    conflict = conflict.select(match_mask::all());
    conflict = conflict.count_bits();
    auto onces = avx::cmplt_mask(conflict.as_mask(), ones) & match_mask::all();
    auto manys = avx::cmpgt_mask(conflict.as_mask(), ones);
    many_letters = exact_mask.select(manys).all_letters();
    auto many_select(set_letters(many_letters));
    auto many2 = (exact_mask & many_select).to_mask();
    many_mask = exact_mask.select(many2);
    auto twices = avx::cmpeq_mask(conflict.as_mask(), ones);
    twice_letters = exact_mask.select(twices).all_letters() & ~many_letters;
    auto twice1 = exact_mask.select(twices & ~manys);
    auto twice_select(set_letters(twice1.all_letters()));
    auto twice2 = (exact_mask & twice_select).select((onces | twices) & ~manys).to_mask();
    twice_mask = exact_mask.select(twice2);
    once_mask = exact_mask.select(onces);
    once_letters = all_letters & ~(many_letters | twice_letters);
    repeated_letters = twice_letters | many_letters;
    all_mask = set_letters(all_letters); 
}

/************************************************************************
 * masked_letters - return the exact letters of a word, only for
 * the letter positions identified by the mask.
 ***********************************************************************/

letter_mask wordle_word::masked_letters(match_mask mask) const
{
    letter_mask result;
    for (auto i : irange((size_t)0, text.size())) {
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
string wordle_word::groom(const string_view &w)
{
    string result;
    letter_counter letter_count;
    if (w.size() == word_length) {
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
    for (size_t i : irange((size_t)0, text.size())) {
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
 * explain - produce a multi-line string showing all the various masks etc
 ***********************************************************************/

string wordle_word::explain() const
{
    string result;
    result += formatted("%20s: %s\n", "exact_mask", get_exact_mask().str());
    result += formatted("%20s: %s\n", "all_mask", get_all_mask().str());
    result += formatted("%20s: %s\n", "once_mask", get_once_mask().str());
    result += formatted("%20s: %s\n", "twice_mask", get_twice_mask().str());
    result += formatted("%20s: %s\n", "many_mask", get_many_mask().str());
    result += formatted("%20s: %s\n", "all_letters", get_all_letters().str());
    result += formatted("%20s: %s\n", "once_letters", get_once_letters().str());
    result += formatted("%20s: %s\n", "twice_letters", get_twice_letters().str());
    result += formatted("%20s: %s\n", "many_letters", get_many_letters().str());
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
 * word_mask::str - return a string representing all the letters at
 * each position in a word mask, separated by '|'. Handy when
 * debugging.
 ***********************************************************************/

string word_mask::str() const
{
    string result;
    for (size_t i : irange(0, word_length)) {
        auto lm = (*this)[i];
        if (lm) {
            result += lm.str();
        } else {
            result += "-";
        }
        if (i < word_length - 1) {
            result += "|";
        }
    }
    return result;
}

/************************************************************************
 * match - compare two words, returning a match_result indicating which
 * letters are partial and exact matches.
 *
 * If there were never any repeated letters, this would be easy. But
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
 * identical - return true iff all of the derived fields are the same
 ***********************************************************************/

bool wordle_word::identical(const wordle_word &other) const
{
    return exact_mask==other.exact_mask
        && all_mask==other.all_mask
        && once_mask==other.once_mask
        && twice_mask==other.twice_mask
        && many_mask==other.many_mask
        && all_letters==other.all_letters
        && once_letters==other.once_letters
        && twice_letters==other.twice_letters
        && many_letters==other.many_letters
        && repeated_letters==other.repeated_letters;
}

/************************************************************************
 * match_result::parse - turn a string of 0/1/2 into the corresponding
 * match result, where:
 *
 * 0 => no match
 * 1 => partial match
 * 2 => exact match
 ***********************************************************************/

bool wordle_word::match_result::parse(const string &m)
{
    bool result = true;
    U16 e = 0;
    U16 p = 0;
    if (m.size()==word_length) {
        for (size_t i : irange(0, word_length)) {
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
    : my_word(new wordle_word(target.str())), my_mr(mr)
{
    match_mask only_partial = my_mr.partial_match & ~my_mr.exact_match;
    partial_letters = my_word->masked_letters(only_partial);
    exact_letters = my_word->masked_letters(my_mr.exact_match);
    auto x1 = partial_letters | exact_letters;
    auto x2 = ~x1;
    absent_letters = my_word->all_letters & x2;
    required_letters = partial_letters | exact_letters;
    letter_counter partial_count;
    letter_counter exact_count;
    letter_counter absent_count;
    for (size_t i : irange(0, word_length)) {
        U16 b = 1 << i;
        char ch = my_word->text[i];
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
    for (size_t i : irange(0, word_length)) {
        char ch = my_word->text[i];
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
            auto exact = exact_mask & other.exact_mask;
            if (exact.count_matches() == exact_match_count) {
                auto partial = only_partial_mask & other.exact_mask;
                if (partial.count_matches() == 0) {
                    result = true;
                    for (const letter_target &lt : letter_targets) {
                        auto ltm = lt.mask & other.exact_mask;
                        auto ltmsz = ltm.count_matches();
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
 * conforms_exact - return iff the exact letters in the result match the
 * given word
 ***********************************************************************/

bool wordle_word::match_target::conforms_exact(const string_view &w) const
{
    word_mask wm(w);
    auto em = my_word->get_exact_mask().select(my_mr.exact_match);
    return (wm & em)==em;
}

/************************************************************************
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
    letter_mask letters(avx::or_i32(masks));
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
 * match_text - return true iff this word matches the other,
 * including wild-cards
 ***********************************************************************/

bool word_mask::match_text(const word_mask & other) const
{
    return word_mask(avx::bool_and(masks, other.masks)).to_mask() ==
        wordle_word::match_result::good_bits();
}

