#include "wordle_word.h"
#include "formatted.h"
#include "styled_text.h"
#include "commands.h"
#include "timers.h"
#include <istream>
#include <fstream>
#include <sstream>
#include <boost/algorithm/string/join.hpp>


extern string wordle_words;
extern string allowed_words;
extern string other_words;
extern vector<string> the_languages;

bool do_options(int argc, char *argv[]);

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

pair<string,string> get_dict_filenames()
{
    string dict_file = options["dict"].as<string>();
    string allowed_path;
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
            return std::make_pair("", "");;
        }
    } else {
        string dict_path = the_path + the_language;
        if (dict_file.empty()) {
            dict_file = dict_path + "/words.txt";
        } else if (dict_file.find('/')==string::npos) {
            dict_file = dict_path + "/" + dict_file;
        }
        string allowed_file = options["allowed"].as<string>();
        if (allowed_file.empty()) {
            allowed_file = "allowed.txt";
        }
        allowed_path = allowed_file;
        if (allowed_file.find('/')==string::npos) {
            allowed_path = dict_path + "/" + allowed_file;
        }
    }
   return std::make_pair(dict_file, allowed_path);
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
    dictionary::init();
    the_wordle = new cwordle(the_dictionary);
    word_length = options["length"].as<int>();
    max_guesses = options["guesses"].as<int>();
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

