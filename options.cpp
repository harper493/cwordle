#include "types.h"

po::options_description od("Available options");

bool do_options(int argc, char *argv[])
{
    od.add_options()
        ("help,h", "produce help message")
        ("allowed,a", po::value<string>()->default_value(""), "allowed words file name")
        ("dict,d", po::value<string>()->default_value(""), "dictionary file name")
        ("guesses,g", po::value<int>()->default_value(DEFAULT_MAX_GUESSES), "max guesses")
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
