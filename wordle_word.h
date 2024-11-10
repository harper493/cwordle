#ifndef __WORDLE_WORD
#define __WORDLE_WORD

#include "types.h"
#include "styled_text.h"
#include "counter_map.h"
#include "avx.h"
#include <immintrin.h>

typedef counter_map<char, U16> letter_counter;

/************************************************************************
 * match_mask - wrapper around _mmask8,providing all the obvious
 * logical operators
 ***********************************************************************/

class match_mask
{
public:
    typedef __mmask8 mask_t;
private:
    mask_t my_mask;
public:
    match_mask()
        : my_mask(0)
    {
    }
    match_mask(const __mmask8 &m)
        : my_mask(m)
    {
    }
    explicit match_mask(const IntegralType auto &v)
        : my_mask(v)
    {
    }
    match_mask &operator=(const match_mask &other)
    {
        my_mask = other.my_mask;
        return *this;
    }
    match_mask &operator=(const IntegralType auto &v)
    {
        my_mask = v;
        return *this;
    }
    U16 get() const
    {
        return my_mask;
    }
    bool operator==(const match_mask &other) const
    {
        return my_mask == other.my_mask;
    }
    bool operator!=(const match_mask &other) const
    {
        return my_mask != other.my_mask;
    }
    explicit operator bool() const
    {
        return my_mask != 0;
    }
    match_mask operator~() const
    {
        return match_mask(~my_mask);
    }
    match_mask operator&(const match_mask &other) const
    {
        return match_mask(my_mask & other.my_mask);
    }
    match_mask operator|(const match_mask &other) const
    {
        return match_mask(my_mask | other.my_mask);
    }
    match_mask &operator&=(const match_mask &other)
    {
        my_mask &= other.my_mask;
        return *this;
    }
    match_mask &operator|=(const match_mask &other)
    {
        my_mask |= other.my_mask;
        return *this;
    }
    U32 size() const
    {
        return __builtin_popcount(my_mask);
    }
    static mask_t all()
    {
        return (1 << word_length) - 1;
    }

    /************************************************************************
     * reduce_bitcount - reduce the number of set bits to 'target_size',
     * starting with the highest order bits
     ***********************************************************************/
    match_mask reduce_bitcount(U32 target_size)
    {
        match_mask result(*this);
        while (result.size() > target_size) {
            U32 lz = __builtin_clz(result.my_mask);
            U32 m = 1 << (31 - lz);
            result.my_mask &= ~m;
        }
        return result;
    }
};

/************************************************************************
 * letter_mask - a U32 with one bit for each letter.
 ***********************************************************************/

class letter_mask
{
private:
    U32 mask = 0;
public:
    letter_mask() { };
    explicit letter_mask(char ch)
        : mask(ch=='-' ? 0 : 1 << (ch - 'a')) { };
    letter_mask(U32 m) : mask(m) { };
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
    bool operator<(const letter_mask &other) const
    {
        return mask < other.mask;
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
        return letter_mask(U32(all().mask & ~mask));
    }
    bool contains(const letter_mask &other) const
    {
        return bool((*this) & other);
    }
    bool contains(char ch) const
    {
        return bool((*this) & letter_mask(ch));
    }
    U32 size() const
    {
        return __builtin_popcount(mask);
    }
    U32 get() const
    {
        return mask;
    }
    static letter_mask all()
    {
        return letter_mask(U32((1 << (ALPHABET_SIZE+1)) - 1));
    }
    /************************************************************************
     * iterator - return each letter covered by the mask (as a one-bit
     * mask) in turn
     ***********************************************************************/
    class const_iterator
    {
    public:
        typedef std::forward_iterator_tag iterator_category;
        typedef letter_mask value_type;
        typedef letter_mask* pointer;
        typedef letter_mask& reference;
    private:
        U32 remaining = 0;
        U32 current = 0;
    public:
        const_iterator() { };
        const_iterator(const letter_mask& m)
            : remaining(m.get())
        {
            next();
        }
        const_iterator(const const_iterator &other)
            : remaining(other.remaining), current(other.current)
        { };
        bool operator==(const const_iterator &other)
        {
            return remaining==other.remaining && current==other.current;
        }
        bool operator!=(const const_iterator &other)
        {
            return !(*this==other);
        }
        const letter_mask &operator&() const
        {
            return reinterpret_cast<const letter_mask&>(current);
        }
        const letter_mask *operator*() const
        {
            return reinterpret_cast<const letter_mask*>(&current);
        }
        const_iterator &operator++()
        {
            next();
            return *this;
        }
    private:
        void next()
        {
            if (remaining == 0) {
                current = 0;
            } else {
                U32 next_rem = remaining & (remaining - 1);
                current = remaining & ~next_rem;
                remaining =next_rem;
            }
        }
    };
    const_iterator begin() const
    {
        return const_iterator(*this);
    }
    const_iterator end() const
    {
        return const_iterator();
    }
};

/************************************************************************
 * word_mask - a 256 or 512 bit value with a letter mask for each position
 * in the word
 ***********************************************************************/

class word_mask
{
public:
    typedef __m256i mask_t;
    typedef array<letter_mask, MAX_WORD_LENGTH> just_letters_t;
    typedef just_letters_t::iterator iterator;
    typedef just_letters_t::const_iterator const_iterator;
private:
    mask_t masks;
public:
    word_mask() : masks(avx::zero(mask_t())) { };
    word_mask(mask_t m) : masks(m) { };
    word_mask(const string_view &s)
    {
        masks = _mm256_setzero_si256();
        for (int i : irange(0, int(s.size()))) {
            (*this)[i] = letter_mask(s[i]);
        }
    }
    word_mask &operator=(const word_mask &other)
    {
        masks = other.masks;
        return *this;
    }
    const mask_t &as_mask() const { return masks; };
    mask_t &as_mask() { return masks; };
    mask_t get() const { return masks; };
    iterator begin() { return as_letters().begin(); }
    iterator end() { return as_letters().end(); }
    explicit operator bool() const
    {
        return !avx::is_zero(masks);
    }
    bool operator==(const word_mask &other) const
    {
        return avx::equal(masks, other.masks);
    }
    bool operator!=(const word_mask &other) const
    {
        return !avx::equal(masks, other.masks);
    }
    #if 0
    letter_mask operator[](size_t idx) const
    {
        return letter_mask(avx::extract(masks, idx));
    }
    #else
    const letter_mask &operator[](size_t idx) const
    {
        return as_letters()[idx];
    }
    letter_mask &operator[](size_t idx)
    {
        return as_letters()[idx];
    }
    #endif
    void insert(letter_mask v, size_t idx)
    {
        (*this)[idx] = v;
    }
    word_mask operator&(const word_mask &other) const
    {
        return word_mask(avx::bool_and(masks, other.masks));
    }
    word_mask operator|(const word_mask &other) const
    {
        return word_mask(avx::bool_or(masks, other.masks));
    }
    word_mask &operator&=(const word_mask &other)
    {
        masks = avx::bool_and(masks, other.masks);
        return *this;
    }
    word_mask &operator|=(const word_mask &other)
    {
        masks = avx::bool_or(masks, other.masks);
        return *this;
    }
    word_mask and_not(const word_mask &other)
    {
        return avx::and_not(masks, other.masks);
    }
    word_mask select(match_mask m)
    {
        return word_mask(avx::mask_blend(m.get(), avx::zero(mask_t()), masks));
    }
    word_mask blend(match_mask m, const word_mask &other)
    {
        return word_mask(avx::mask_blend(m.get(), other.masks, masks));
    }
    
    /************************************************************************
     * count_bits - return a word_mask containing at each position, the
     * number of bits set at that position in the input. Uses the classic
     * bit-counting algorithm, AVX-ified.
     ***********************************************************************/
    
    word_mask count_bits() const
    {
        mask_t zero(avx::zero(mask_t()));
        mask_t ones(avx::set1(mask_t(), 1));
        auto done = avx::cmpeq_mask(masks, zero);
        mask_t result = zero;
        mask_t m = masks;
        while (!avx::is_zero(m)) {
            done |= avx::cmpeq_mask(m, zero);
            result = avx::add(result, avx::mask_blend(~done, zero, ones));
            m = avx::bool_and(m, avx::sub(m, ones));
        }
        return word_mask(result);
    }
    U32 count_letter(char letter) const;
    U32 count_letter(letter_mask m) const;
    match_mask match_letter(char letter) const;
    match_mask match_letter(letter_mask m) const;
    bool match_text(const word_mask &other) const;
    letter_mask all_letters() const
    {
        return letter_mask(avx::or_i32(masks));
    }
    match_mask to_mask() 
    {
        mask_t zeros = _mm256_setzero_si256();
        return avx::cmpgt_mask(masks, zeros);
    }
    size_t count_matches()
    {
        return __builtin_popcount(to_mask().get());
    }
    letter_counter count_letters() const;
    string str() const;
    static word_mask set_letters(letter_mask m)
    {
        return word_mask(avx::set1(mask_t(), m.get()));
    }
    static word_mask set_letters(letter_mask m, match_mask mask)
    {
        return word_mask(avx::set1_masked(mask_t(), m.get(), mask.get()));
    }
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

/************************************************************************
 * wordle_word class - repersentation of a single 5-letter word in 
 * a way that makes algorthm execution efficient
 ***********************************************************************/

class wordle_word
{
public:
    class match_result
    {
    private:
        match_mask exact_match = 0;
        match_mask partial_match = 0;
    public:
        match_result() {};
        match_result(const match_result &other)
            : exact_match(other.exact_match), partial_match(other.partial_match) { };
        match_result(match_mask e, match_mask p)
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
        U16 get_hash() const
        {
            return (partial_match.get() << 5) | exact_match.get();
        }
        bool parse(const string &s);
        static U16 good_bits()
        {
            return (1 << word_length) - 1;
        }
    friend class wordle_word;
    friend class match_target;
    };
    class match_target
    {
    public:
        struct letter_target {
            word_mask mask;
            U16 count = 0;
            bool greater_ok = false;
        public:
            letter_target() { };
            letter_target(char letter, match_mask exact_mask, U16 c, bool gtok)
                : count(c), greater_ok(gtok)
            {
                word_mask target_all = set_letters(letter_mask(letter));
                __m256i zeros = _mm256_setzero_si256();
                mask = avx::mask_blend(exact_mask.get(), target_all.get(), avx::zero(__m256i()));
            }
        };
    private:
        const wordle_word &my_word;
        match_result my_mr;
        letter_mask partial_letters;
        letter_mask exact_letters;
        letter_mask absent_letters;
        letter_mask required_letters;
        word_mask partial_mask;
        word_mask only_partial_mask;
        word_mask exact_mask;
        U16 partial_match_count = 0;
        U16 exact_match_count = 0;
        vector<letter_target> letter_targets;
        letter_target letters;
    public:
        match_target(const wordle_word &target, const match_result &mr);
        bool conforms(const wordle_word &other) const;
        bool conforms_exact(const string_view &w) const;
        const match_result &get_result() const
        {
            return my_mr;
        }
        styled_text show() const
        {
            return my_word.styled_str(my_mr);
        }
    };
private:
    word_mask exact_mask;
    word_mask all_mask;
    word_mask once_mask;
    word_mask twice_mask;
    word_mask many_mask;
    letter_mask all_letters;
    letter_mask once_letters;
    letter_mask twice_letters;
    letter_mask many_letters;
    letter_mask repeated_letters;
    string text;
    static bool verbose;
public:
    wordle_word()
    {
    }
    wordle_word(const string_view &w)
    {
        set_word(w);
    }
    wordle_word(const string_view &w, int method)
    {
        switch (method) {
        case 1:
            set_word_basic(w);
            break;
        case 2:
            set_word_2(w);
            break;
        case 0:
        case 3:
        default:
            set_word(w);
            break;
        }
    }
    void set_word(const string_view &w);
    void set_word_basic(const string_view &w);
    void set_word_2(const string_view &w);
    bool good() const
    {
        return text[0] != 0;
    }
    string_view str() const
    {
        return text;
    }
    string explain() const;
    size_t size() const
    {
        return text.size();
    }
    bool operator<(const wordle_word &other) const
    {
        return text < other.text;
    }
    bool operator==(const wordle_word &other) const
    {
        return text == other.text;
    }
    bool operator!=(const wordle_word &other) const
    {
        return text != other.text;
    }
    bool identical(const wordle_word &other) const;
    styled_text styled_str(const match_result &mr) const;
    match_result match(const wordle_word &target) const;
    letter_mask masked_letters(match_mask mask) const;
    bool match_text(const word_mask &other) const
    {
        return exact_mask.match_text(other);
    }
    word_mask get_exact_mask() const { return exact_mask; };
    word_mask get_all_mask() const { return all_mask; };
    word_mask get_once_mask() const { return once_mask; };
    word_mask get_twice_mask() const { return twice_mask; };
    word_mask get_many_mask() const { return many_mask; };
    letter_mask get_all_letters() const { return all_letters; };
    letter_mask get_once_letters() const { return once_letters; };
    letter_mask get_twice_letters() const { return twice_letters; };
    letter_mask get_many_letters() const { return many_letters; };
    static match_mask make_letters_mask()
    {
        return match_mask((1<<word_length) - 1);
    }
    static word_mask set_letters(letter_mask letters)
    {
        return avx::set1_masked(__m256i(), letters.get(), make_letters_mask().get());
    }
    static string groom(const string_view&w);
    static void set_verbose(bool v) { verbose = v; };
private:
    match_result do_match(const wordle_word &target, bool verbose) const _always_inline;
    static __mmask8 to_mask(__m256i matched)
    {
        __m256i zeros = _mm256_setzero_si256();
        return avx::cmpgt_mask(matched, zeros);
    }
#ifdef AVX512
    static __mmask16 to_mask(__m512i matched)
    {
        __m512i zeros = _mm512_setzero_si512();
        return avx::cmpgt_mask(matched, zeros);
    }
#endif
    static size_t count_matches(__m256i matched)
    {
        return __builtin_popcount(to_mask(matched));
    }
friend class match_target;
};

word_mask wm(__m256i m);
string wms(__m256i m);

#endif
