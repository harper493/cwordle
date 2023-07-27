#ifndef __ENTROPY
#define __ENTROPY

#include "types.h"

extern float entropy(const vector<float> &data);
extern float entropy_slow(const vector<float> &data);
extern float entropy_slowest(const vector<float> &data);

#endif
