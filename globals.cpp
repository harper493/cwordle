#include "types.h"
#include "timers.h"

timing_reporter timers::entropy_timer(true);
timing_reporter timers::match_timer(true);
timing_reporter timers::conforms_timer(true);

po::variables_map options;
cwordle *the_wordle = NULL;
int word_length = DEFAULT_WORD_LENGTH;
int max_guesses = DEFAULT_MAX_GUESSES;
commands *the_commands = NULL;
dictionary *the_dictionary = NULL;
string the_language;
vector<string> the_languages;
string the_path;
bool strict_mode = false;
bool sutom_mode = false;

