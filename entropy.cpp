#include "entropy.h"
#include "../avx_mathfun/avx_mathfun.h"
#include <functional>
#include <cmath>
#include <numeric>
#include <immintrin.h>

/************************************************************************
 * add_horizontal - return the sum of the 8 float32 values in a
 * 256-bit __m256
 ***********************************************************************/

float add_horizontal(__m256 values)
{
    const __m128 r4 = _mm_add_ps( _mm256_castps256_ps128(values), _mm256_extractf128_ps(values, 1) );
    const __m128 r2 = _mm_add_ps( r4, _mm_movehl_ps( r4, r4 ) );
    const __m128 r1 = _mm_add_ss( r2, _mm_movehdup_ps( r2 ) );
    return _mm_cvtss_f32( r1 );
}

/************************************************************************
 * entropy - calculate the entropy of the values in a vector.
 *
 * This uses the classic formula:
 *
 * entropy = -sum ( p(n) * log(p(n)) ) / log (sum(p(n)))
 *
 * which has value 1 if all values are equal. Zero values are ignored.
 *
 * This implementation uses AVX and also some mathematical trickery
 * to avoid needing to pre-calculate the sum.
 *
 * This code is a vectorized version of entropy_slow (below). The only
 * complication is that since log(0) is infinite, we have to replace
 * zeros by 1 before calculting the log.
 *
 * The other two implementations are to verify that this one
 * gives the right answers.
 ***********************************************************************/

float entropy(const vector<float> &data)
{
    __m256 ent256 = _mm256_setzero_ps();
    __m256 sum256 = _mm256_setzero_ps();
    __m512 zeros = _mm512_setzero_ps();
    __m256 ones = _mm256_set1_ps(1.0);
    size_t n = 0;
    for (size_t i = 0; i < data.size(); i += 8) {
        __m256 d = _mm256_loadu_ps(&data[i]);
        __mmask8 nonzero_mask = ~(_mm512_cmpeq_ps_mask(_mm512_castps256_ps512(d), zeros));
        if (nonzero_mask != 0) {
            sum256 = _mm256_add_ps(sum256, d);
            n += __builtin_popcount(nonzero_mask);
            __m256 nonzero = _mm256_mask_blend_ps(nonzero_mask, ones, d);
            __m256 l = log256_ps(nonzero);
            ent256 = _mm256_fmadd_ps(l, d, ent256);
        }
    }
    float ent = add_horizontal(ent256);
    float sum = add_horizontal(sum256);
    float result = (((-(ent - sum * log(sum)) / sum)));
    if (isnan(result)) {
        result = 0.0;
    }
    return result;
    
}

/************************************************************************
 * Non-AVX implementation but including the mathematical trick of
 * working with the raw values then normalising them to probabilities
 * at the end.
 ***********************************************************************/

float entropy_slow(const vector<float> &data)
{
    float sum = 0;
    float e = 0;
    float n = 0;
    for (size_t i : irange(0ul, data.size())) {
        float d = data[i];
        if (d > 0) {
            e += d * log(d);
            sum += d;
            n += 1;
        }
    }
    return (((-(e - sum * log(sum)) / sum)));
}

/************************************************************************
 * Simplest implementation, doing everything by the book, incuding 
 * pre-calculating the sum.
 ***********************************************************************/

float entropy_slowest(const vector<float> &data)
{
    float sum = std::accumulate(data.begin(), data.end(), 0.0);
    float invsum = 1 / sum;
    float e = 0;
    float n = 0;
    for (size_t i : irange(0ul, data.size())) {
        float d = data[i];
        if (d > 0) {
            e += d * invsum * log(d * invsum);
            n += 1;
        }
    }
    return -e;
}
