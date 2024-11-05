#include "wordle_word.h"
#include "formatted.h"
#include "styled_text.h"
#include "commands.h"
#include "timers.h"
#include <istream>
#include <fstream>
#include <sstream>
#include <boost/program_options.hpp>
#include <boost/algorithm/string/join.hpp>

namespace po = boost::program_options;

po::options_description od("Available options");
po::variables_map options;

extern vector<string> wordle_words;
extern vector<string> other_words;

timing_reporter timers::entropy_timer(true);
timing_reporter timers::match_timer(true);
timing_reporter timers::conforms_timer(true);

cwordle *the_wordle = NULL;
int word_length = DEFAULT_WORD_LENGTH;
commands *the_commands = NULL;
string the_language;
string the_path;
bool strict_mode = false;
bool sutom_mode = false;

bool do_options(int argc, char *argv[])
{
    od.add_options()
        ("help", "produce help message")
        ("dict,d", po::value<string>()->default_value(""), "dictionary file name")
        ("language,L", po::value<string>()->default_value(""), "language")
        ("length,l", po::value<int>()->default_value(DEFAULT_WORD_LENGTH), "word length")
        ("path,p", po::value<string>()->default_value(DEFAULT_PATH), "path top language dictionaries")
        ("sutom,S", "play using Sutom rules")
        ("verbose,V", "show details of comparison operations")
        ("vocab,v", po::value<string>()->default_value(""), "select builtin vocabulary (wordle or other)")
        ("time,t", "show timing information");
    try {
        po::store(po::parse_command_line(argc, argv, od), options);
    } catch (std::exception &exc) {
        cout << "Error in command line: " << exc.what() << '\n';
        return false;
    }
    po::notify(options);
    if (options.count("help") > 0) {
        cout << od << '\n';
        return false;
    }
    return true;
}

void run()
{
    the_wordle = new cwordle();
    word_length = options["length"].as<int>();
    string dict_file = options["dict"].as<string>();
    the_path = options["path"].as<string>();
    if (!algorithm::ends_with(the_path, "/")) {
        the_path += '/';
    }
    the_language = algorithm::to_lower_copy(options["language"].as<string>());
    sutom_mode = options.count("sutom") > 0;
    strict_mode = sutom_mode;
    if (dict_file.empty() && the_language.empty()) {
        the_language = DEFAULT_LANGUAGE;
        string vocab = options["vocab"].as<string>();
        if (vocab.empty()) {
            vocab = "other";
        }
        if (vocab=="other") {
            the_wordle->load_words(other_words);
        } else if (vocab=="wordle") {
            the_wordle->load_words(wordle_words);
        } else {
            cout << formatted("Unknown vocabulary '%s'\n", vocab);
            return;
        }
    } else {
        if (dict_file.empty()) {
            dict_file = the_path + the_language + "/words.txt";
        }
        if (the_wordle->load_file(dict_file)) {
            cout << formatted("Loaded %d words from dictionary '%s'\n",
                              the_wordle->get_dictionary().size(), dict_file);
        } else {
            cout << formatted("Failed to load dictionary file '%s'\n", dict_file);
            return;
        }
    }
    commands cmds;
    the_commands = &cmds;
    cmds.set_timing(options.count("time") > 0);
    wordle_word::set_verbose(options.count("verbose") > 0);
    while (std::cin.good()) {
        cout << "cwordle> ";
        string line;
        getline(std::cin, line);
        if (!cmds.do_command(line)) {
            break;
        }
    }
}

int main(int argc, char *argv[])
{
    if (!do_options(argc, argv)) {
        return 1;
    }
    styled_text::set_renderer(styled_text::iso6429);
    run();
    return 0;
}

