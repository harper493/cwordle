#include "tests.h"
#include "styled_text.h"
#include "wordle_word.h"
#include "formatted.h"
#include "entropy.h"

void tests::do_test(int t)
{
    switch (t) {
    case 1:
        test1();
        break;
    case 2:
        test2();
        break;
    default:
        break;
    }
}

string tests::t(const string &w1, const string &w2, const string &correct,
         const vector<string> &good, const vector<string> &bad)
{
    wordle_word ww1(w1);
    wordle_word ww2(w2);
    wordle_word::match_result c(correct);
    auto m = ww2.match(w1);
    auto result =  ww2.styled_str(m);
    styled_text sw1(w1, styled_text::black);
    styled_text sw2(w2, styled_text::black);
    if (m!=c) {
        sw1.set_color(styled_text::red);
        sw2.set_color(styled_text::red);
    }
    styled_text matches;
    wordle_word::match_target mt(ww2, m);
    for (const auto &g : good) {
        if (mt.conforms(wordle_word(g))) {
            matches.append(g, styled_text::green);
        } else {
            matches.append(g, styled_text::red);
        }
        matches.append(",", styled_text::black);
    }
    matches.append("  ");
    for (const auto &b : bad) {
        if (mt.conforms(wordle_word(b))) {
            matches.append(b, styled_text::red, styled_text::color_none, styled_text::crossed);
        } else {
            matches.append(b, styled_text::green, styled_text::color_none, styled_text::crossed);
        }
        matches.append(",", styled_text::black);
    }
    return formatted("%s  %s  %s  %s", sw1, sw2, result, matches);
}

void tests::test1()
{
    cout << t("ploop", "poppy", "21100",
              { "ploop", "plopo" },
              { "plank", "popps", "poops" }) << '\n';
    cout << t("poppy", "ploop", "20101",
              { "poppy", "prapo", "pppox" },
              { "ppops", "pppoo" }) << '\n';
    cout << t("beech", "evade", "10001",
              { "beers", "beech", "newer" },
              { "never", "eiger", "eager", "begin" }) << '\n';
    cout << t("heart", "thear", "11111",
              { "reath", "heart" },
              { "thear", "ethar", "theas" }) << '\n';
    cout << t("beech", "chest", "11200",
              { "beech", "leech" },
              { "cheer", "leach", "leesh" }) << '\n';
    cout << t("beech", "flinx", "00000",
              { "beech", "romad" },
              { "flinx", "loops", "exact" }) << '\n';
    cout << t("beech", "beech", "22222",
              { "beech" },
              { "beach", "flinx" }) << '\n';
    cout << t("heart", "chest", "01102",
              { "heart", "havet", "hatet" },
              { "thare", "shart", "bleat" }) << '\n';
    cout << t("heart", "steel", "01100",
              { "heart", "prate", "terpy", "eract" },
              { "evade", "attic", "lemon" }) << '\n';
    cout << t("poppy", "appit", "01200",
              { "poppy", "popol", "poper", "hoppy" },
              { "paper", "upper" }) << '\n';
    cout << t("poppy", "appip", "01201",
              { "poppy", "puppy" },
              { "upppu", "popop", "potop" }) << '\n';
    cout << t("spoof", "pippy", "10000",
              { "spoof", "opals", "arpen", "stoop" },
              { "petal", "poppy", "appit" }) << '\n';
    cout << t("spoop", "pippy", "10100",
              { "spoop" },
              { "popop", "poopp" }) << '\n';
    cout << t("visas", "adman", "00020",
              {  },
              {  }) << '\n';
    cout << t("poppy", "steep", "00001",
              { "poppy", "ploof", "popov" },
              { "creep" }) << '\n';
    cout << t("steep", "pappy", "10000",
              { "steep", "opens" },
              { "spoop", "hoppy", "hospy", "proof" }) << '\n';
    cout << t("ethor", "etate", "22000",
              { "ethor", "etrik" },
              { "etter", "spets", "etexx", "ettox" }) << '\n';
    cout << t("speft", "etate", "11000",
              { "speft", "terik" },
              { "etrik", "teets", "teeps", "testy" }) << '\n';
    cout << t("pewsy", "pepep", "22000",
              { "pewsy", "perik" },
              { "pepwq", "peeps", "peres", "pelpp" }) << '\n';
    cout << t("weppo", "pepep", "12200",
              { "weppo", "wepop" },
              { "pepwq", "peeps", "peres", "pelpp" }) << '\n';
    cout << t("water", "abode", "10001", {}, {}) << "\n";
    cout << t("abode", "water", "01010", {}, {}) << "\n";
}

void tests::test2()
{
    vector<float> d1;
    for (size_t i : irange(0,16)) {
        d1.push_back(4);
    }
    cout << entropy_slowest(d1) << " " << entropy_slow(d1) << " " << entropy(d1) << "\n";
    d1[1] = 10;
    cout << entropy_slowest(d1) << " " << entropy_slow(d1) << " " << entropy(d1) << "\n";
    d1[1] = 100;
    cout << entropy_slowest(d1) << " " << entropy_slow(d1) << " " << entropy(d1) << "\n";
    d1[1] = 1000;
    cout << entropy_slowest(d1) << " " << entropy_slow(d1) << " " << entropy(d1) << "\n";
    d1[1] = 4;
    for (size_t i : irange(0,8)) {
        d1[i*2] = 0;
    }
    cout << entropy_slowest(d1) << " " << entropy_slow(d1) << " " << entropy(d1) << "\n";
    for (size_t i : irange(0,16)) {
        d1[i] = i;
    }
    cout << entropy_slowest(d1) << " " << entropy_slow(d1) << " " << entropy(d1) << "\n";
}
