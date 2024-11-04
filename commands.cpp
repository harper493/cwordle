#include "commands.h"
#include "styled_text.h"
#include "tests.h"
#include "timing_reporter.h"
#include "wordle_word.h"
#include "timers.h"
#include <boost/algorithm/string.hpp>
#include <regex>

using std::regex;
using std::smatch;

/************************************************************************
 * The singleton commands object provides the user interface to the cwordle object,
 * which is where all the real work happens.
 *
 * It also manages and reports the timing_reporter objects which are
 * used to report the time spent in critical algorithms.
 ***********************************************************************/

/************************************************************************
 * Command keyword table.
 ***********************************************************************/

KEYWORDS(command_list)
KEYWORD("best", "b", do_best, "show best word(s) to filter remaining words")
KEYWORD("entropy", "ent", do_entropy, "show entropy for a word against current remaining")
KEYWORD("exit", "ex", do_exit, "exit cwordle")
KEYWORD("explain", "exp", do_explain, "explain how a word is analysed")
KEYWORD("help", "h", do_help, "show help text")
KEYWORD("new", "n", do_new, "select a new random word")
KEYWORD("recap", "rec", do_recap, "recap worsd tried so far")
KEYWORD("remaining", "rem", do_remaining, "show remaining matching words")
KEYWORD("result", "res", do_result, "supply result of a test")
KEYWORD("reveal", "rev", do_reveal, "reveal the current word (i.e. cheat)")
KEYWORD("set", "set", do_set, "set an explicit word")
KEYWORD("test", "test", do_test, "run numbered development test")
KEYWORD("try", "t", do_try, "try a word against the current word")
KEYWORD("undo", "un", do_undo, "undo the last tried word")
KEYWORD("words", "word", do_words, "add one or more words to the dictionary")
KEYWORDS_END()

regex rx_command("(\\S+)\\s*(.*)", std::regex_constants::ECMAScript);

auto output_color = styled_text::green;

/************************************************************************
 * Constructor - ensure we have a new random word.
 ***********************************************************************/

commands::commands()
{
    the_wordle->new_word();
}

/************************************************************************
 * do_command - execute one command. Normally, pick off the first
 * lexeme, whcih is the command, and look inthe command table to
 * find it.
 *
 * Exceptionally, the 'exit' command is handled inline.
 ***********************************************************************/

bool commands::do_command(const string &line)
{
    bool result = true;
    my_line = rest_of_line = line;
    timers::entropy_timer.reset();
    timers::match_timer.reset();
    timers::conforms_timer.reset();
    if (!line.empty()) {
        try {
            string cmd = next_arg();
            const keyword *kw = command_list.find(cmd);
            if (kw) {
                if (kw->full=="exit") {
                    result = false;
                } else {
                    timing_reporter tr;
                    (this->*(kw->my_fn))();
                    if (show_timing) {
                        cout << styled_text(formatted("Completed in %s", tr.show_time()),
                                            styled_text::deep_blue, styled_text::color_none, styled_text::italic)
                                         << "\n";
                    }
                }
            } else {
                throw syntax_exception("Unknown command '%s'", cmd);
            }
        } catch (syntax_exception &exc) {
            cout << styled_text(exc.what(), styled_text::red) << "\n";
        }
    }
    return result;
}

/************************************************************************
 * do_best - find the best word given the esults we have had so far.
 ***********************************************************************/

void commands::do_best()
{
    auto how_many = next_arg_int(true);
    check_finished();
    auto result = the_wordle->best(how_many.value_or(1));
    std::multimap<float, string> result_map;
    for (const auto &r : result) {
        if (r.value > 0) {
            result_map.insert(std::pair(r.value, r.key->str()));
        }
    }
    for (auto r2=result_map.rbegin(); r2!=result_map.rend(); ++r2) {
        cout << styled_text(formatted("%-7s %.3f", r2->second, r2->first), output_color) << "\n";
    }
    if (show_timing) {
        display_time(timers::match_timer, "Match: ");
        display_time(timers::entropy_timer, "Entropy: ");
    }
}

/************************************************************************
 * do_entropy - calculate teh entropy for a given word in the 
 * context of the results so far.
 ***********************************************************************/

void commands::do_entropy()
{
    const wordle_word &w = validate_word(next_arg());
    check_finished();
    float entropy = the_wordle->entropy(w);
    cout << styled_text(formatted("Entropy of '%s' is %.3f", w.str(), entropy), output_color)
         << "\n";
}

/************************************************************************
 * do_exit - this function is never called but is required by the
 * keyword table.
 ***********************************************************************/

void commands::do_exit()
{
}

/************************************************************************
 * do_explain - show all the masks of a given word
 ***********************************************************************/

void commands::do_explain()
{
    wordle_word w(next_arg());
    check_finished();
    cout << formatted("%20s: %s\n", "exact_mask", w.get_exact_mask().str());
    cout << formatted("%20s: %s\n", "all_mask", w.get_all_mask().str());
    cout << formatted("%20s: %s\n", "once_mask", w.get_once_mask().str());
    cout << formatted("%20s: %s\n", "twice_mask", w.get_twice_mask().str());
    cout << formatted("%20s: %s\n", "many_mask", w.get_many_mask().str());
    cout << formatted("%20s: %s\n", "all_letters", w.get_all_letters().str());
    cout << formatted("%20s: %s\n", "once_letters", w.get_once_letters().str());
    cout << formatted("%20s: %s\n", "twice_letters", w.get_twice_letters().str());
    cout << formatted("%20s: %s\n", "many_letters", w.get_many_letters().str());
}

/************************************************************************
 * do_help - use the keyword table to provide help for the
 * available commands.
 ***********************************************************************/

void commands::do_help()
{
    std::map<string, const keyword*> cmds;
    string topic = next_arg(true);
    for (const auto &k : command_list.keywords) {
        if (topic.empty() || k.full==topic) {
            cmds[k.full] = &k;
        }
    }
    if (cmds.empty()) {
        throw syntax_exception("'%s' is not a wordle command", topic);
    }
    for (const auto &i : cmds) {
        cout << styled_text(formatted("%-10s", i.first), output_color)
             << styled_text(i.second->help, styled_text::magenta)
             << "\n";
    }
}

/************************************************************************
 * do_new - choose a new random word
 ***********************************************************************/

void commands::do_new()
{
    check_finished();
    the_wordle->new_word();
}

/************************************************************************
 * do_recap - recall the results so far for the current word
 ***********************************************************************/

void commands::do_recap()
{
    check_started();
    check_finished();
    for (const auto &w : the_wordle->get_results()) {
        cout << w.show () << "\n";
    }
}

/************************************************************************
 * do_remaining - list the remaining words after applying all the
 * results so far. If there are more than 20, just list the first 20.
 *
 * A word list is not alphabetical, so we sort it forst.
 ***********************************************************************/

void commands::do_remaining()
{
    check_started();
    check_finished();
    const auto &wl = the_wordle->remaining();
    int sz = wl.size();
    auto sorted_list = wl.sorted();
    vector<string> words;
    string suffix;
    if (sz > 0) { 
        if (sz > 20) {
            sz = 20;
            suffix = ", ...";
        }
        for (size_t i : irange(0, sz)) {
            words.emplace_back(get_dict().get_string(wl[i]));
        }
        string words2 = boost::algorithm::join(words, ", ");
        cout << styled_text(formatted("%d words remaining: %s%s", wl.size(), words2, suffix), output_color) << "\n";
    } else {
        cout << styled_text("No remaining words\n", output_color);
    }
}

/************************************************************************
 * do_result - take a test result of the form <word> <match result>
 * and feed it into the current word, e.g:
 *
 * result plank 10210
 ***********************************************************************/

void commands::do_result()
{
    wordle_word word = validate_word(next_arg());
    wordle_word::match_result mr;
    if (!mr.parse(next_arg())) {
        throw syntax_exception("match string must contain only 0 for miss, 1 for partial match, 2 for exact match");
    }
    check_finished();
    the_wordle->set_result(word, mr);
}

/************************************************************************
 * do_reveal - reveal the current word
 ***********************************************************************/

void commands::do_reveal()
{
    check_finished();
    cout << styled_text(formatted("The current word is '%s'", the_wordle->get_current_word().str()),
                        output_color)
         << "\n";
}

/************************************************************************
 * do_set - set an explicit target word
 ***********************************************************************/

void commands::do_set()
{
    string w = next_arg();
    check_finished();
    the_wordle->set_word(validate_word(w).str());
}

/************************************************************************
 * do_test - followed by a test number, execute the corresponding unit test.
 ***********************************************************************/

void commands::do_test()
{
    tests::do_test(next_arg_int().value());
}

/************************************************************************
 * do_try - try a word against the current selection.
 ***********************************************************************/

void commands::do_try()
{
    const wordle_word &ww = validate_word(next_arg());
    check_finished();
    auto mr = the_wordle->try_word(ww);
    if (the_wordle->remaining().size()==1 && ww==the_wordle->get_current_word()) {
        cout << styled_text(formatted("Success! The word is '%s'", the_wordle->get_current_word().str()),
                            styled_text::magenta) << "\n";
    } else {
        cout << ww.styled_str(mr) << "\n";
        if (the_wordle->remaining().size()==0) {
            cout << styled_text("No remaining words\n", styled_text::red);
        }
    }
    if (show_timing) {
        display_time(timers::conforms_timer, "Conforms: ");
    }
}

/************************************************************************
 * do_undo - undo the most recent try
 ***********************************************************************/

void commands::do_undo()
{
    check_started();
    check_finished();
    the_wordle->undo();
}

/************************************************************************
 * do_words - add one or more words to the dictionary
 ***********************************************************************/

void commands::do_words()
{
    string w;
    while (!(w = next_arg(true)).empty()) {
        string g = wordle_word::groom(w);
        get_dict().insert(g);
    }
}

/************************************************************************
 * next_arg - get the next lexeme from the input line. If end_ok is
 * true, return an empty string at the end of the line. Otherwise,
 * throw an error.
 ***********************************************************************/

string commands::next_arg(bool end_ok)
{
    smatch m;
    if (std::regex_match(rest_of_line, m, rx_command)) {
        cur_arg = m[1].str();
        rest_of_line = m[2].str();
    } else if (end_ok) {
        cur_arg = "";
    } else {
        throw syntax_exception("Unexpected end of command");
    }
    return cur_arg;
}

/************************************************************************
 * next_arg_int - get the next lexeme and parse it as number
 ***********************************************************************/

optional<int> commands::next_arg_int(bool end_ok)
{
    optional<int> result;
    string i = next_arg(end_ok);
    if (!i.empty()) {
        try {
            result = lexical_cast<int>(i);
        } catch (const std::exception &exc) {
            throw syntax_exception("'%s' is not a valid integer", i);
        }
    }
    return result;
}

/************************************************************************
 * validate_word - given a word, ensure that it complies with the
 * rules for a wordle wprd, and that it is in the dictionary. Return
 * the word in its canonical form as returned by groom().
 ***********************************************************************/

wordle_word commands::validate_word(const string &w)
{
    string groomed = wordle_word::groom(w);
    if (groomed.empty()) {
        throw syntax_exception("'%s' is not a valid wordle word (too long or short, too many repeats)", w);
    }
    auto ww = get_dict().find_word(groomed);
    if (!ww) {
        // throw syntax_exception("'%s' is not in the dictionary", groomed);
    }
    return wordle_word(groomed);
}

/************************************************************************
 * check_started - throw an error if there have been no tries for the
 * current word.
 ***********************************************************************/

void commands::check_started() const
{
    if (the_wordle->size()==0) {
        throw syntax_exception("You haven't tried anything yet");
    }
}

/************************************************************************
 * check_finished - ensure we are at the end of the command line
 ***********************************************************************/

void commands::check_finished()
{
    string lexeme = next_arg(true);
    if (!lexeme.empty()) {
        throw syntax_exception("Unexpected items at end of command '%s...'", lexeme);
    }
}

/************************************************************************
 * get_dict - get the dictionary, which is held by the cwordle object
 ***********************************************************************/

dictionary &commands::get_dict()
{
    return the_wordle->get_dictionary();
}

/************************************************************************
 * display_time - display the result of a timming_reporter
 ***********************************************************************/

void commands::display_time(timing_reporter &timer, const string &label)
{
    string r = timer.report("", label);
    cout << styled_text(r, styled_text::deep_blue, styled_text::color_none, styled_text::italic);
}

/************************************************************************
 * keyword_table::find - find the given string in the keyword table.
 * It must be a substring of one of the full commands, and
 * at least as long as the minimal version.
 ***********************************************************************/

const commands::keyword *commands::keyword_table::find(const string &kw) const
{
    const keyword *result = NULL;
    for (const auto &k : keywords) {
        if (boost::starts_with(k.full, kw) && boost::starts_with(kw, k.minimal)) {
            result = &k;
            break;
        }
    }
    return result;
}



