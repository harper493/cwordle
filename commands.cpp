#include "commands.h"

KEYWORDS(keyword_list)
KEYWORD("t", "try", &command_try());
KEYWORDS_END()

bool commands::do_command(const string &line)
{
    my_line = rest_of_line = line;
    if (!line.empty()) {
        try {
            string cmd = next_arg();
            keywords.action(cmd);
        } catch (syntax_exception &exc) {
        }
    }
}

void commands::do_best()
{
}

void commands::do_entropy()
{
}

void commands::do_exit()
{
}

void commands::do_help()
{
}

void commands::do_new()
{
}

void commands::do_recap()
{
}

void commands::do_remaining()
{
}

void commands::do_reveal()
{
}

void commands::do_set()
{
}

void commands::do_try()
{
}

void commands::do_undo()
{
}

void commands::do_words()
{
}

string commands::next_arg()
{
}

