#ifndef __COMMANDS
#define __COMMANDS

#include "types.h"
#include "cwordle.h"
#include "formatted.h"
#include "dictionary.h"
#include <boost/algorithm/string/predicate.hpp>

class timing_reporter;

class commands
{
public:
    class syntax_exception : public std::exception
    {
    private:
        string what_str;
    public:
        syntax_exception(const string &t)
            : what_str(t) { };
        template<typename... ARGS>
        syntax_exception(const string &f, const ARGS &...args)
        {
            what_str = formatted(f, args...);
        }
        const char * what() const noexcept
        {
            return what_str.c_str();
        }
    };
    typedef void(commands::*fn_t)();
    struct keyword
    {
    public:
        string full;
        string minimal;
        string help;
        fn_t my_fn;
    public:
        keyword(const string &f, const string &m, const string &h, fn_t fn)
            : full(f), minimal(m), help(h), my_fn(fn) { };
        keyword(const keyword &other)
            : full(other.full), minimal(other.minimal), help(other.help), my_fn(other.my_fn) { };
    };
    struct keyword_table
    {
    public:
        vector<keyword> keywords;
    public:
        template<class COLL>
        keyword_table(const COLL &c)
        {
            for (const keyword &k : c) {
                add_keyword(k);
            }
        }
        void add_keyword(const keyword &kw)
        {
            keywords.emplace_back(kw);
        }
        const keyword *find(const string &kw) const;
    };
private:
    string my_line;
    string rest_of_line;
    string cur_arg;
    bool show_timing = false;
public:
    commands();
    bool do_command(const string &line);
    void set_timing(bool t)
    {
        show_timing = t;
    }
public:
    void do_best();
    void do_entropy();
    void do_exit();
    void do_explain();
    void do_help();
    void do_new();
    void do_recap();
    void do_remaining();
    void do_result();
    void do_reveal();
    void do_set();
    void do_test();
    void do_try();
    void do_undo();
    void do_words();
    string next_arg(bool end_ok=false);
    optional<int> next_arg_int(bool end_ok=false);
private:
    wordle_word validate_word(const string &w);
    void show_error(const string &err);
    void check_started() const;
    void check_finished();
    dictionary &get_dict();
    void display_time(timing_reporter &timer, const string &label);
};

#define KEYWORDS(NAME) commands::keyword_table NAME ( vector<commands::keyword> {
#define KEYWORD(F,M,FN,H) commands::keyword(F, M, H, &commands::FN),
#define KEYWORDS_END() });

#endif
