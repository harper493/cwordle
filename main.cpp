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

extern string wordle_words;
extern string allowed_words;
extern string other_words;

timing_reporter timers::entropy_timer(true);
timing_reporter timers::match_timer(true);
timing_reporter timers::conforms_timer(true);

cwordle *the_wordle = NULL;
int word_length = DEFAULT_WORD_LENGTH;
commands *the_commands = NULL;
string the_language;
vector<string> the_languages;
string the_path;
bool strict_mode = false;
bool sutom_mode = false;

bool do_options(int argc, char *argv[])
{
    od.add_options()
        ("help,h", "produce help message")
        ("allowed,a", po::value<string>()->default_value(""), "allowed words file name")
        ("dict,d", po::value<string>()->default_value(""), "dictionary file name")
        ("language,L", po::value<string>()->default_value(""), "language")
        ("length,l", po::value<int>()->default_value(DEFAULT_WORD_LENGTH), "word length")
        ("path,p", po::value<string>()->default_value(DEFAULT_PATH), "path to language dictionaries")
        ("strict", "use strict mode")
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

void find_languages()
{
    the_languages.clear();
    bfs::path p(the_path);
    if (bfs::exists(p) && is_directory(p)) {
        for (auto pi = bfs::directory_iterator(p); pi!=bfs::directory_iterator(); ++pi) {
            if (is_directory(*pi) && bfs::exists(*pi / "words.txt")) {
                the_languages.push_back(pi->path().stem().string());
            }
        }
    }
}

string choose_language(const string_view &ll)
{
    find_languages();
    string best;
    for (const auto &lang : the_languages) {
        if (lang.starts_with(ll)) {
            if (best.empty()) {
                best = lang;
            } else {
                cout << formatted("Language name '%s' is ambiguous\n", ll);
                return "";
            }
        }
    }
    if (best.empty()) {
        cout << formatted("Unknown language '%s'\n", ll);
    }
    return best;
}

bool load_dict()
{
    string dict_file = options["dict"].as<string>();
   if (dict_file.empty() && the_language.empty()) {
        the_language = DEFAULT_LANGUAGE;
        string vocab = options["vocab"].as<string>();
        if (vocab.empty()) {
            vocab = "wordle";
        }
        if (vocab=="other") {
            the_wordle->load_words(other_words);
        } else if (vocab=="wordle") {
            the_wordle->load_words(wordle_words);
            the_wordle->load_words_allowed(allowed_words);
        } else {
            cout << formatted("Unknown vocabulary '%s'\n", vocab);
            return false;
        }
    } else {
        string dict_path = the_path + the_language;
        if (dict_file.empty()) {
            dict_file = dict_path + "/words.txt";
        } else if (dict_file.find('/')==string::npos) {
            dict_file = dict_path + "/" + dict_file;
        }
        if (the_wordle->load_file(dict_file)) {
            cout << formatted("Loaded %d words from dictionary '%s'\n",
                              the_wordle->get_dictionary().size(), dict_file);
        } else {
            cout << formatted("Failed to load dictionary file '%s'\n", dict_file);
            return false;
        }
        string allowed_file = options["allowed"].as<string>();
        if (allowed_file.empty()) {
            allowed_file = "allowed.txt";
        }
        string allowed_path = allowed_file;
        if (allowed_file.find('/')==string::npos) {
            allowed_path = dict_path + "/" + allowed_file;
        }
        if (the_wordle->load_file_allowed(allowed_path)) {
            cout << formatted("Loaded %d allowed words from file '%s'\n",
                              the_wordle->get_dictionary().allowed_size(), allowed_path);
        }
    }
    return true;
}

int run()
{
    the_wordle = new cwordle();
    word_length = options["length"].as<int>();
    the_path = options["path"].as<string>();
    if (!algorithm::ends_with(the_path, "/")) {
        the_path += '/';
    }
    string lang(algorithm::to_lower_copy(options["language"].as<string>()));
    if (!lang.empty()) {
        the_language = choose_language(lang);
        if (the_language.empty()) {
            return 1;
        }
    }
    sutom_mode = options.count("sutom") > 0;
    strict_mode = sutom_mode || options.count("strict") > 0;
    if (!load_dict()) {
        return 1;
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
    return 0;
}

int main(int argc, char *argv[])
{
    if (!do_options(argc, argv)) {
        return 1;
    }
    styled_text::set_renderer(styled_text::iso6429);
    return run();
}

