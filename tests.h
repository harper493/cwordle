#ifndef __TESTS
#define __TESTS

#include "types.h"

class tests
{
public:
    static void do_test(int t);
private:
    static void test1();
    static void test2();
    static string t(const string &w1, const string &w2, const string &correct,
             const vector<string> &good, const vector<string> &bad);
};

#endif
