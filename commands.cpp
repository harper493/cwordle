#include "commands.h"
#include "styled_text.h"
#include "tests.h"
#include "timing_reporter.h"
#include "timers.h"
#include <boost/algorithm/string.hpp>
#include <regex>

using std::regex;
using std::smatch;

KEYWORDS(command_list)
KEYWORD("best", "b", do_best, "show best word(s) to filter remaining words")
KEYWORD("entropy", "ent", do_entropy, "show entropy for a word against current remaining")
KEYWORD("exit", "ex", do_exit, "exit cwordle")
KEYWORD("help", "h", do_help, "show help text")
KEYWORD("new", "n", do_new, "select a new random word")
KEYWORD("recap", "rec", do_recap, "recap worsd tried so far")
KEYWORD("remaining", "rem", do_remaining, "show remaining matching words")
KEYWORD("reveal", "rev", do_reveal, "reveal the current word (i.e. cheat)")
KEYWORD("set", "set", do_set, "set an explicit word")
KEYWORD("test", "test", do_test, "run numbered development test")
KEYWORD("try", "t", do_try, "try a word against the current word")
KEYWORD("undo", "un", do_undo, "undo the last tried word")
KEYWORDS_END()

regex rx_command("(\\S+)\\s*(.*)", std::regex_constants::ECMAScript);

auto output_color = styled_text::green;

commands::commands(cwordle &cw)
    : the_wordle(cw)
{
    the_wordle.new_word();
}

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

void commands::do_best()
{
    auto how_many = next_arg_int(true);
    auto result = the_wordle.best(how_many.value_or(1));
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

void commands::do_entropy()
{
    const wordle_word &w = validate_word(next_arg());
    float entropy = the_wordle.entropy(w);
    cout << styled_text(formatted("Entropy of '%s' is %.3f", w.str(), entropy), output_color)
         << "\n";
}

void commands::do_exit()
{
}

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

void commands::do_new()
{
    the_wordle.new_word();
}

void commands::do_recap()
{
    check_started();
    for (const auto &w : the_wordle.get_results()) {
        cout << w.show () << "\n";
    }
}

void commands::do_remaining()
{
    check_started();
    const auto &wl = the_wordle.remaining();
    int sz = wl.size();
    vector<string> words;
    string suffix;
    if (sz > 20) {
        sz = 20;
        suffix = ", ...";
    }
    for (size_t i : irange(0, sz)) {
        words.emplace_back(get_dict().get_string(wl[i]));
    }
    string words2 = boost::algorithm::join(words, ", ");
    cout << styled_text(formatted("%d words remaining: %s%s", wl.size(), words2, suffix), output_color) << "\n";
}

void commands::do_reveal()
{
    cout << styled_text(formatted("The current word is '%s'", the_wordle.get_current_word().str()),
                        output_color)
         << "\n";
}

void commands::do_set()
{
    the_wordle.set_word(validate_word(next_arg()).str());
}

void commands::do_test()
{
    tests::do_test(next_arg_int().value());
}

void commands::do_try()
{
    const wordle_word &ww = validate_word(next_arg());
    auto mr = the_wordle.try_word(ww);
    if (the_wordle.remaining().size()==1 && ww==the_wordle.get_current_word()) {
        cout << styled_text(formatted("Success! The word is '%s'", the_wordle.get_current_word().str()),
                            styled_text::magenta) << "\n";
    } else {
        cout << ww.styled_str(mr) << "\n";
    }
    if (show_timing) {
        display_time(timers::conforms_timer, "Conforms: ");
    }
}

void commands::do_undo()
{
    check_started();
    the_wordle.undo();
}

void commands::do_words()
{
}

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

const wordle_word &commands::validate_word(const string &w) const
{
    string groomed = wordle_word::groom(w);
    if (groomed.empty()) {
        throw syntax_exception("'%s' is not a valid wordle word (too long or short, too many repeats)", w);
    }
    auto ww = get_dict().find_word(groomed);
    if (!ww) {
        throw syntax_exception("'%s' is not in the dictionary", groomed);
    }
    return *ww.value();
}

void commands::check_started() const
{
    if (the_wordle.size()==0) {
        throw syntax_exception("You haven't tried anything yet");
    }
}

const dictionary &commands::get_dict() const
{
    return the_wordle.get_dictionary();
}

void commands::display_time(timing_reporter &timer, const string &label)
{
    string r = timer.report("", label);
    cout << styled_text(r, styled_text::deep_blue, styled_text::color_none, styled_text::italic);
}


