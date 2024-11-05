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

cwordle *the_wordle;
int word_length;
commands * the_commands;

bool do_options(int argc, char *argv[])
{
    od.add_options()
        ("help", "produce help message")
        ("dict,d", po::value<string>()->default_value(""), "dictionary file name")
        ("length,l", po::value<int>()->default_value(DEFAULT_WORD_LENGTH), "word length")
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
    if (dict_file.empty()) {
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
        if (!the_wordle->load_file(dict_file)) {
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

