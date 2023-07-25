
#ifndef __FORMATTED
#define __FORMATTED

#include "types.h"
#include <boost/format.hpp>

using boost::format;

/************************************************************************
 * Function to encapsulate (format("...") % ...).str() design pattern
 ***********************************************************************/

inline void _formatted(format &fmt) // recursion terminator
{
}

template<typename ARG, typename... ARGS>
inline void _formatted(format &fmt, const ARG &arg1, const ARGS &...args)
{
    fmt % arg1;
    _formatted(fmt, args...);
}

template<typename... ARGS>
inline string formatted(const string &fmt, const ARGS &...args)
{
    format f(fmt);
    _formatted(f, args...);
    return f.str();
}

#endif
