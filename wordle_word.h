#ifndef __WORDLE_WORD
#define __WORDLE_WORD

#include "types.h"
#include "styled_text.h"
#include "counter_map.h"
#include <immintrin.h>

/************************************************************************
 * wordle_word class - repersentation of a single 5-letter word in 
 * a way that makes algorthm execution efficient
 ***********************************************************************/

class wordle_word
{
public:
    class letter_mask
    {
    private:
        U32 mask = 0;
    public:
        letter_mask() { };
        explicit letter_mask(char ch) : mask(1 << (ch - 'a')) { };
        explicit letter_mask(int i) : mask(1 << i) { };
        letter_mask(U32 m, int) : mask(m) { };
        explicit operator bool() const
        {
            return mask != 0;
        }
        explicit operator char() const
        {
            if (mask) {
                return 'a' - 1 + __builtin_ffs(mask);
            } else {
                return ' ';
            }
        }
        string str() const;
        bool empty() const
        {
            return mask == 0;
        }
        bool operator==(const letter_mask &other) const
        {
            return mask == other.mask;
        }
        bool operator!=(const letter_mask &other) const
        {
            return mask == other.mask;
        }
         letter_mask &operator|=(const letter_mask &other)
        {
            mask |= other.mask;
            return *this;
        }
        letter_mask &operator|=(char ch)
        {
            mask |= letter_mask(ch).mask;
            return *this;
        }
        letter_mask operator|(const letter_mask &other) const
        {
            letter_mask result = *this;
            result |= other;
            return result;
        }
        letter_mask operator&(const letter_mask &other) const
        {
            letter_mask result = *this;
            result &= other;
            return result;
        }
        letter_mask &operator&=(const letter_mask &other)
        {
            mask &= other.mask;
            return *this;
        }
        letter_mask &operator&=(char ch)
        {
            mask &= letter_mask(ch).mask;
            return *this;
        }
        letter_mask &remove(const letter_mask &other)
        {
            mask &= ~other.mask;
            return *this;
        }
        letter_mask &remove(char ch)
        {
            mask &= ~letter_mask(ch).mask;
            return *this;
        }
        letter_mask operator~() const
        {
            return letter_mask(all().mask & ~mask, 0);
        }
        bool contains(const letter_mask &other) const
        {
            return bool((*this) & other);
        }
        bool contains(char ch) const
        {
            return bool((*this) & letter_mask(ch));
        }
        U32 get() const
        {
            return mask;
        }
        static letter_mask all()
        {
            return letter_mask((1 << (ALPHABET_SIZE+1)) - 1, 0);
        }
    };
    class masks_t
    {
    public:
        typedef array<letter_mask, WORD_LENGTH> just_letters_t;
        typedef just_letters_t::iterator iterator;
        typedef just_letters_t::const_iterator const_iterator;
    private:
        __m256i masks;
    public:
        masks_t() : masks(_mm256_setzero_si256()) { };
        const __m256i &as_m256i() const { return masks; };
        __m256i &as_m256i() { return masks; };
        iterator begin() { return as_letters().begin(); }
        iterator end() { return as_letters().end(); }
        // const iterator begin() const { return as_letters().begin(); }
        // const iterator end() const { return as_letters().end(); }
        letter_mask &operator[](size_t idx)
        {
            return as_letters()[idx];
        }
        const letter_mask &operator[](size_t idx) const
        {
            return as_letters()[idx];
        }
        string str() const;
    private:
        just_letters_t &as_letters()
        {
            return reinterpret_cast<just_letters_t&>(masks);
        }
        const just_letters_t &as_letters() const
        {
            return reinterpret_cast<const just_letters_t&>(masks);
        }        
    };
    typedef array<char, WORD_LENGTH> text_t;
    typedef counter_map<char, U16> letter_counter;
    class match_result
    {
    private:
        U16 exact_match = 0;
        U16 partial_match = 0;
    public:
        match_result() {};
        match_result(const match_result &other)
            : exact_match(other.exact_match), partial_match(other.partial_match) { };
        match_result(U16 e, U16 p)
            : exact_match(e & good_bits()), partial_match(p & good_bits()) { };
        match_result(const string &m)
        {
            parse(m);
        }
        bool operator==(const match_result &other) const
        {
            return exact_match==other.exact_match && partial_match==other.partial_match;
        }
        bool operator!=(const match_result &other) const
        {
            return !(*this==other);
        }
        bool is_exact(size_t i) const
        {
            return (exact_match & (1 << i)) != 0;
        }
        bool is_partial(size_t i) const
        {
            return (partial_match & (1 << i)) != 0;
        }
        void parse(const string &s);
        static U16 good_bits()
        {
            return (1 << WORD_LENGTH) - 1;
        }
    friend class wordle_word;
    friend class match_target;
    };
    class match_target
    {
    public:
        struct letter_target {
            masks_t mask;
            U16 count = 0;
            bool greater_ok = false;
        public:
            letter_target() { };
            letter_target(char letter, U16 exact_mask, U16 c, bool gtok)
                : count(c), greater_ok(gtok)
            {
                __m256i target_all = _mm256_set1_epi32(letter_mask(letter).get());
                __m256i zeros = _mm256_setzero_si256();
                __mmask8 m2 = (1 << WORD_LENGTH) - 1;
                _mm256_mask_storeu_epi32(&mask, m2, _mm256_mask_blend_epi32(exact_mask, target_all, zeros));
            }
        };
    private:
        const wordle_word &my_word;
        match_result my_mr;
        letter_mask partial_letters;
        letter_mask exact_letters;
        letter_mask absent_letters;
        letter_mask required_letters;
        masks_t partial_mask;
        masks_t only_partial_mask;
        masks_t exact_mask;
        U16 partial_match_count = 0;
        U16 exact_match_count = 0;
        vector<letter_target> letter_targets;
        letter_target letters;
    public:
        match_target(const wordle_word &target, const match_result &mr);
        bool conforms(const wordle_word &other);
    };
private:
    masks_t exact_mask;
    masks_t all_mask;
    masks_t once_mask;
    masks_t twice_mask;
    masks_t thrice_mask;
    letter_mask all_letters;
    letter_mask once_letters;
    letter_mask twice_letters;
    letter_mask thrice_letters;
    text_t text;
public:
    wordle_word(const string &w);
    bool good() const
    {
        return text[0] != 0;
    }
    string str() const
    {
        return string(text.begin(), text.end());
    }
    styled_text styled_str(const match_result &mr) const;
    match_result match(const wordle_word &target);
    letter_mask masked_letters(U16 mask) const;
    static string groom(const string &w);
private:
    static __mmask8 to_mask(__m256i matched)
    {
        __m256i zeros = _mm256_set1_epi32(0);
        return _mm256_cmpgt_epu32_mask(matched, zeros);
    }
    static size_t count_matches(__m256i matched)
    {
        return __builtin_popcount(to_mask(matched));
    }
friend class match_target;
};

#endif