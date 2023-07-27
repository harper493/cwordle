#ifndef __COMMANDS
#define __COMMANDS

#include "types.h"
#include "keyword_table.h"
#include "cwordle.h"

class commands
{
public:
    class syntax_exception : public std::exception
    {
    private:
        string text;
    public:
        syntax_exception(const string &t)
            : text(t) { };
        string what() const
        {
            return text;
        }
    };
private:
    keyword_table keywords;
    cwordle the_wordle;
    const char **my_argv = NULL;
    int my_argc = 0;
public:
    bool do_command(const char *argv[], int argc);
private:
    void do_best();
    void do_entropy();
    void do_exit();
    void do_help();
    void do_new();
    void do_recap();
    void do_remaining();
    void do_reveal();
    void do_set();
    void do_try();
    void do_undo();
    void do_words();
    string next_arg();
    void show_error(const string &err);
};

#endif
