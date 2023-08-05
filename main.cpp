#include "wordle_word.h"
#include "formatted.h"
#include "styled_text.h"
#include "commands.h"
#include <istream>
#include <fstream>
#include <sstream>
#include <boost/program_options.hpp>
#include <boost/algorithm/string/join.hpp>

namespace po = boost::program_options;

po::options_description od("Available options");
po::variables_map options;

void run()
{
    cwordle cw;
    cw.load_words();
    commands cmds(cw);
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
    styled_text::set_renderer(styled_text::iso6429);
    run();
    return 0;
}

