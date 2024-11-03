#ifndef __AVX
#define __AVX

#include "types.h"
#include <immintrin.h>

/************************************************************************
 * AVX helper functions, designed to hide 256/512 bit-ness.
 *
 * This set of functions has two purposes:
 *
 * 1. Hide some of the messy detals of the AVX512 intrinsics
 * 2. Allow code to be written without knowing whether it is dealing
 *    with 256 or 512 bit values.
 ***********************************************************************/

namespace avx
{
    /************************************************************************
     * 256 bit functions
     ***********************************************************************/
    
    inline __m256i zero(__m256i)
    {
        return _mm256_setzero_si256();
    }
    inline __m256i load(__m256i *src)
    {
        return _mm256_loadu_si256(src);
    }
    inline void storeu(__m256i &dst, __m256i value)
    {
        _mm256_storeu_si256(&dst, value);
    }
    inline void mask_storeu(__m256i &dst, __mmask8 mask, __m256i value)
    {
        _mm256_mask_storeu_epi32(&dst, mask, value);
    }
    #if 0
    inline U32 extract(__m256i src, size_t index)
    {
        return _mm256_extract_epi32(src, index);
    }
    inline void insert(__m256i dst, U32 value, size_t index)
    {
        _mm256_insert_epi32(dst, value, index);
    }
    #endif
    inline bool is_zero(__m256i p)
    {
        return _mm256_testz_si256(p, p);
    }
    inline bool equal(__m256i p, __m256i q)
    {
        return _mm256_cmpneq_epi32_mask(p, q)==0;
    }
    inline __m256i add(__m256i p, __m256i q)
    {
        return _mm256_add_epi32(p, q);
    }
    inline __m256i sub(__m256i p, __m256i q)
    {
        return _mm256_sub_epi32(p, q);
    }
    inline __m256i bool_and(__m256i p, __m256i q)
    {
        return _mm256_and_si256(p, q);
    }
    inline __m256i bool_or(__m256i p, __m256i q)
    {
        return _mm256_or_si256(p, q);
    }
    inline __m256i and_not(__m256i p, __m256i q)
    {
        return _mm256_andnot_si256(p, q);
    }
    inline __m256i set1(__m256i, U32 x)
    {
        return _mm256_set1_epi32(x);
    }
    inline __m256i conflict(__m256i src)
    {
        return _mm256_conflict_epi32(src);
    }
    inline __m256i set1_masked(__m256i, U32 x, __mmask8 mask)
    {
        __m256i r1 = _mm256_set1_epi32(x);
        __m256i r2 = _mm256_mask_blend_epi32(mask, zero(__m256i()), r1);
        return r2;
    }
    inline __m256i cmpeq(__m256i x, __m256i y)
    {
        return _mm256_cmpeq_epi32(x, y);
    }
    inline __m256i cmpne(__m256i x, __m256i y)
    {
        return _mm256_xor_si256(_mm256_cmpeq_epi32(x, y), _mm256_set1_epi32(-1));        
    }
    inline __m256i cmpgt(__m256i x, __m256i y)
    {
        return _mm256_cmpgt_epi32(x, y);
    }
    inline __m256i cmplt(__m256i x, __m256i y)
    {
        return _mm256_cmpgt_epi32(y, x);
    }
    inline __mmask8 cmpeq_mask(__m256i x, __m256i y)
    {
        return _mm256_cmpeq_epu32_mask(x, y);
    }
    inline __mmask8 cmpne_mask(__m256i x, __m256i y)
    {
        return _mm256_cmpneq_epu32_mask(x, y);
    }
    inline __mmask8 cmpgt_mask(__m256i x, __m256i y)
    {
        return _mm256_cmpgt_epu32_mask(x, y);
    }
    inline __mmask8 cmplt_mask(__m256i x, __m256i y)
    {
        return _mm256_cmplt_epu32_mask(x, y);
    }
    inline __m256i mask_blend(U16 mask, __m256i x, __m256i y)
    {
        return _mm256_mask_blend_epi32(mask, x, y);
    }
    inline U32 or_i32(__m256i x)
    {
        __m256i x0 = _mm256_permute2x128_si256(x,x,1);
        __m256i x1 = _mm256_or_si256(x,x0);
        __m256i x2 = _mm256_shuffle_epi32(x1,0b01001110);
        __m256i x3 = _mm256_or_si256(x1,x2);
        __m256i x4 = _mm256_shuffle_epi32(x3, 0b11100001);
        __m256i x5 = _mm256_or_si256(x3, x4);
        return _mm_cvtsi128_si32(_mm256_castsi256_si128(x5)) ;
    }
    inline U32 add_i32(__m256i x)
    {
        __m128i sum128 = _mm_add_epi32(_mm256_castsi256_si128(x),
                                       _mm256_extracti128_si256(x, 1));
        __m128i hi64  = _mm_unpackhi_epi64(sum128, sum128);
        __m128i sum64 = _mm_add_epi32(hi64, sum128);
        __m128i hi32  = _mm_shuffle_epi32(sum64, _MM_SHUFFLE(2, 3, 0, 1));    // Swap the low two elements
        return _mm_cvtsi128_si32(_mm_add_epi32(sum64, hi32));
    }
    
    /************************************************************************
     * 512 bit functions
     ***********************************************************************/
    
    inline __m512i zero(__m512i)
    {
        return _mm512_setzero_si512();
    }
    inline __m512i load(__m512i *src)
    {
        return _mm512_loadu_si512(src);
    }
    inline void storeu(__m512i &dst, __m512i value)
    {
        _mm512_storeu_si512(&dst, value);
    }
    inline void  mask_storeu(__m512i &dst, __mmask16 mask, __m512i value)
    {
        _mm512_mask_storeu_epi32(&dst, mask, value);
    }
    #if 0
    inline U32 extract(__m512i src, size_t index)
    {
        return _mm512_extract_epi32(src, index);
    }
    inline void insert(__m512i dst, U32 value, size_t index)
    {
        _mm512_insert_epi32(dst, value, index);
    }
    #endif
    inline bool equal(__m512i p, __m512i q)
    {
        return _mm512_cmpneq_epi32_mask(p, q)==0;
    }
    inline __m512i bool_and(__m512i p, __m512i q)
    {
        return _mm512_and_si512(p, q);
    }
    inline __m512i bool_or(__m512i p, __m512i q)
    {
        return _mm512_or_si512(p, q);
    }
    inline __m512i and_not(__m512i p, __m512i q)
    {
        return _mm512_andnot_si512(p, q);
    }
    inline __m512i set1(__m512i, U32 x)
    {
        return _mm512_set1_epi32(x);
    }
    inline __m512i conflict(__m512i src)
    {
        return _mm512_conflict_epi32(src);
    }
    inline __m512i set1_masked(__m512i, U32 x, __mmask16 mask)
    {
        __m512i r1 = _mm512_set1_epi32(x);
        __m512i r2 = _mm512_mask_blend_epi32(mask, zero(__m512i()), r1);
        return r2;
    }
    inline __mmask16 cmpgt(__m512i x, __m512i y)
    {
        return _mm512_cmpgt_epu32_mask(x, y);
    }
    inline __m512i mask_blend(U16 mask, __m512i x, __m512i y)
    {
        return _mm512_mask_blend_epi32(mask, x, y);
    }
#if 0
    inline U32 or_i32(__m512i x)
    {
        __m256i x0 = _mm512_castsi512_si256(_mm512_shuffle_epi32(x, _MM_SHUFFLE(1, 2, 3, 4)));
        __m256i x1 = _mm256_or_si256(_mm512_castsi512_si256(x), x0);
        return or_i32(x1);
    }
    inline U32 add_i32(__m512i x)
    {
        __m256i x0 = _mm512_castsi512_si256(x);
        __m256i x1 = _mm512_castsi512_si256(_mm512_shuffle_epi32(x, _MM_SHUFFLE(1, 2, 3, 4)));
        __m256i sum256 = _mm256_add_epi32(x0, x1);
        return add_i32(sum256);
    }
#endif
};

#endif
