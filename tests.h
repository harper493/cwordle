#ifndef __TESTS
#define __TESTS

#include "types.h"

class tests
{
public:
    static void do_test(int t, commands &cmd);
private:
    static void test1(commands &cmd);
    static void test2(commands &cmd);
    static void test3(commands &cmd);
    static void test4(commands &cmd);
    static void test5(commands &cmd);
    static string t(const string &w1, const string &w2, const string &correct,
             const vector<string> &good, const vector<string> &bad);
};

#endif
