#ifndef __TIMERS
#define __TIMERS

#include "types.h"
#include "timing_reporter.h"

class timers
{
public:
    static timing_reporter entropy_timer;
    static timing_reporter match_timer;
    static timing_reporter conforms_timer;
};

#endif
