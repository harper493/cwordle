#ifndef __TYPES__
#define __TYPES__

#include <vector>
#include <map>
#include <optional>
#include <string>
#include <array>
#include <iostream>
#include <boost/format.hpp>
#include <chrono>
#include <functional>
#include <mutex>
#include <boost/range/irange.hpp>

using std::vector;
using std::optional;
using std::string;
using std::map;
using std::array;
using std::pair;
using std::bind;
using std::mutex;
using boost::irange;

using std::cout;
using namespace std::placeholders;

using namespace std::chrono;

using boost::format;

typedef __uint128_t U128;
typedef unsigned long long U64;
typedef unsigned int U32;
typedef unsigned short U16;
typedef unsigned char U8;

typedef __int128_t S128;
typedef signed long long S64;
typedef signed int S32;
typedef signed short S16;
typedef signed char S8;

#define WORD_LENGTH 5
#define ALPHABET_SIZE 26

#endif
