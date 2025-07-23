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
#include <set>
#include <ranges>
#include <string_view>
#include <memory>
#include <boost/lexical_cast.hpp>
#include <boost/range/irange.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

using std::vector;
using std::optional;
using std::string;
using std::string_view;
using std::map;
using std::array;
using std::pair;
using std::bind;
using std::mutex;
using std::set;
using std::unique_ptr;
using std::function;
using boost::irange;
using boost::lexical_cast;
namespace algorithm = boost::algorithm;

using std::cout;
using namespace std::placeholders;

using namespace std::chrono;
namespace views = std::views;
namespace bfs = boost::filesystem;
namespace po = boost::program_options;

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

class cwordle;
class commands;
class dictionary;

extern po::variables_map options;
extern cwordle *the_wordle;
extern commands *the_commands;
extern dictionary *the_dictionary;
extern int word_length;
extern int max_guesses;
extern string the_language;
extern string the_path;
extern bool sutom_mode;
extern bool strict_mode;
extern string wordle_words;
extern string allowed_words;

pair<string,string> get_dict_filenames();
bool do_options(int argc, char *argv[]);

#define likely(X) __builtin_expect(!!(X), 1)
#define unlikely(X) __builtin_expect(!!(X), 0)
#define _always_inline __attribute__((always_inline))

#define MIN_WORD_LENGTH 4
#define MAX_WORD_LENGTH 16
#define DEFAULT_WORD_LENGTH 5
#define DEFAULT_MAX_GUESSES 6
#define DEFAULT_LANGUAGE "english"
#define DEFAULT_PATH "languages/"
#define ALPHABET_SIZE 26
#define AVX512

template<typename T>
concept IntegralType = requires(T param)
{
    requires std::is_integral_v<T>;
};

#endif
