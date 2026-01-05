#ifndef __TESTS
#define __TESTS

#include "types.h"

class tests
{
public:
    static void do_test(int t, optional<int>);
private:
    static void test1(optional<int>);
    static void test2(optional<int>);
    static void test3(optional<int>);
    static void test4(optional<int>);
    static void test5(optional<int>);
    static string t(const string &w1, const string &w2, const string &correct,
             const vector<string> &good, const vector<string> &bad);
};

#endif
